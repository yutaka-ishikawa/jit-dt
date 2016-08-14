/*
 *	Just In Time Data Transfer
 *	13/08/2016 Adding sftp protocol
 *			by Yutaka Ishikawa, RIKEN AICS 
 *	15/12/2015 Written by Yutaka Ishikawa, RIKEN AICS
 *			yutaka.ishikawa@riken.jp
 */
#include "translib.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/select.h>

#define MAX_DIRS	(24*60*2+1)
//#define PATH_WATCH	"/home/ishikawa/tmp/123"
#define PATH_WATCH	"./"
#define BUFSIZE		(PATH_MAX + 1 + sizeof(struct inotify_event))
char	nevtbuf[BUFSIZE];
char	wapath[PATH_MAX];
char	avpath[PATH_MAX];
int	vflag;
int	dflag;

fd_set	readfds;
char	wdirs[MAX_DIRS][PATH_MAX];
int	curdir;

static void
fformat(char *path)
{
    if (path[strlen(path) - 1] != '/') strcat(path, "/");
}

int
main(int argc, char **argv)
{
    int		opt;
    char	*url;
    int		ntfydir, ntfyfile, nfds, curdir, cc;
    int		ttype;
    ssize_t	sz;
    double	sec;
    struct stat		 sbuf;
    struct inotify_event *iep = (struct inotify_event *) nevtbuf;

    if (argc < 3 || argc > 5) {
	fprintf(stderr,
		"USAGE: %s <url> <watching directory path> [-d] [-v]\n",
		argv[0]);
	fprintf(stderr, "e.g.: %s http://kncc-login1.kncc.cc.u-tokyo.ac.jp \\\n"
		"            /home/yisikawa/work/FileTransfer/tmp/\n", argv[0]);
	return -1;
    }
    if (strlen(argv[2]) >= PATH_MAX - 1) {
	fprintf(stderr, "Too long directory path (%ld)\n", strlen(argv[2]));
	return -1;
    }
    url = argv[1];
    strcpy(wapath, (argc < 3) ? PATH_WATCH : argv[2]);
    fformat(wapath);
    dflag = 0;
    if (argc > 3) {
	while ((opt = getopt(argc, argv, "dv")) != -1) {
	    switch (opt) {
	    case 'd': dflag = 1; break;
	    case 'v':
		vflag = 1;
		break;
	    }
	}
    }
    DBG {
	fprintf(stderr, "IN_CREATE=0x%x IN_WRITE=0x%x\n",
		IN_CREATE, IN_CLOSE_WRITE);
    }
    ttype = trans_type(url);
    ntfydir = inotify_init();  ntfyfile = inotify_init();
    if (ntfydir < 0 || ntfyfile < 0) {
	perror("inotify_init:");
	exit(-1);
    }
    FD_ZERO(&readfds); FD_SET(ntfydir, &readfds); FD_SET(ntfyfile, &readfds);
    nfds = ntfyfile + 1;
    /* inotify_add_watch returns a watch descriptor starts from 1 */
    if ((cc = inotify_add_watch(ntfydir, wapath, IN_CREATE)) < 0) {
 	perror("inotify_add_watch:"); exit(-1);
    }
    strcpy(wdirs[cc], wapath);
    curdir = cc;
    if ((cc = inotify_add_watch(ntfyfile, wapath, IN_CLOSE_WRITE)) < 0) {
 	perror("inotify_add_watch:"); exit(-1);
    }
    VMODE {
	fprintf(stderr, "now watching directory %s, dirid(%d)\n",
		wdirs[curdir], curdir);
    }
    while (select(nfds, &readfds, NULL, NULL, NULL) > 0) {
	DBG {
	    fprintf(stderr, "*** new event (%0x)***\n", iep->mask);
	}
	if (FD_ISSET(ntfydir, &readfds)) {
	    /* a new directory might be created, IN_CREAT */
	    /* if it is a new file, then skip */
	    if ((sz = read(ntfydir, nevtbuf, BUFSIZE)) < 0) {
		perror("read");	exit(-1);
	    }
	    if (iep->mask&IN_IGNORED) goto next; /* inotify_rm_watch issued */
	    strcpy(avpath, wdirs[iep->wd]);
	    strcat(avpath, iep->name);
	    if ((cc = stat(avpath, &sbuf)) != 0) {
		VMODE { fprintf(stderr, "??? %s\n", avpath); }
		goto next;
	    }
	    if (!S_ISDIR(sbuf.st_mode)) goto next; /* not a directory */
	    DBG {
		fprintf(stderr, "DIR: dirid(%d) name(%s) curdir(%d)\n",
			iep->wd, iep->name, curdir);
	    }
	    if (iep->wd < curdir) {
		/*
		 * The upper level directory is created. This means
		 * no more wating the current directory.
		 */
		VMODE {
		    fprintf(stderr,"leaving the directory %s (curdir=%d)\n",
			    wdirs[curdir], curdir);
		}
		inotify_rm_watch(ntfydir, curdir);
		inotify_rm_watch(ntfyfile, curdir);
	    }
	    curdir = inotify_add_watch(ntfydir, avpath, IN_CREATE);
	    strcpy(wdirs[curdir], avpath);
	    fformat(wdirs[curdir]);
	    VMODE {
		fprintf(stderr, "now watching directory %s, dirid(%d)\n",
			wdirs[curdir], curdir);
	    }
	    cc = inotify_add_watch(ntfyfile, avpath, IN_CLOSE_WRITE);
	} else if (FD_ISSET(ntfyfile, &readfds)) {
	    /* a new/modified file is closed, IN_CLOSE_WRITE */
	    if ((sz = read(ntfyfile, nevtbuf, BUFSIZE)) < 0) {
		perror("read");	exit(-1);
	    }
	    if (iep->mask&IN_IGNORED) goto next; /* inotify_rm_watch issued */
	    strcpy(avpath, wdirs[iep->wd]);
	    strcat(avpath, iep->name);
	    sec = (*ttable[ttype])(url, avpath);
	    VMODE {
		fprintf(stderr, "%s, %f\n", avpath, sec); fflush(stderr);
	    }
	}
    next:
	FD_ZERO(&readfds);
	FD_SET(ntfydir, &readfds);
	FD_SET(ntfyfile, &readfds);
    }
    return 0;
}
