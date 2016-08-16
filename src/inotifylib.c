/*
 *	inotify library separated from jit-transfer
 *	16/08/2016 Written by Yutaka Ishikawa, RIKEN AICS
 *			yutaka.ishikawa@riken.jp
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include "inotifylib.h"

#define DBG	if (flag&MYNOTIFY_DEBUG)
#define VMODE	if (flag&MYNOTIFY_VERBOSE)
#define MAX_DIRS	(24*60*2+1)
#define PATH_WATCH	"./"
#define BUFSIZE		(PATH_MAX + 1 + sizeof(struct inotify_event))
int	vflag;
int	dflag;

fd_set	readfds;
int	curdir;
char	wdirs[MAX_DIRS][PATH_MAX];
char	nevtbuf[BUFSIZE];
char	avpath[PATH_MAX];

static inline void
fformat(char *path)
{
    if (path[strlen(path) - 1] != '/') strcat(path, "/");
}

static int
add_watch(int fd, const char *path, uint32_t mask)
{
    int	cc;
    /* inotify_add_watch returns a watch descriptor starts from 1 */
    if ((cc = inotify_add_watch(fd, path, mask)) < 0) {
 	perror("inotify_add_watch:");
	fprintf(stderr, "file = %s\n", path);
	exit(-1);
    }
    return cc;
}


int
mynotify(char *topdir, char *startdir,
	 void (*func)(char*, void**), void **args, int flag)
{
    int			ntfydir, ntfyfile, nfds, curdir, cc;
    ssize_t		sz;
    struct stat		sbuf;
    struct inotify_event *iep = (struct inotify_event *) nevtbuf;

    DBG {
	fprintf(stderr, "IN_CREATE=0x%x IN_WRITE=0x%x\n",
		IN_CREATE, IN_CLOSE_WRITE);
    }
    ntfydir = inotify_init();  ntfyfile = inotify_init();
    if (ntfydir < 0 || ntfyfile < 0) {
	perror("inotify_init:");
	exit(-1);
    }
    FD_ZERO(&readfds); FD_SET(ntfydir, &readfds); FD_SET(ntfyfile, &readfds);
    nfds = ntfyfile + 1;
    cc = add_watch(ntfydir, topdir, IN_CREATE);
    strcpy(wdirs[cc], topdir);
    curdir = cc;
    cc = add_watch(ntfyfile, topdir, IN_CLOSE_WRITE);
    /* starting directory */
    if (startdir) {
	char	*idx, *sp = startdir;
	fformat(sp);
	while ((idx = index(sp, '/'))) {
	    *idx = 0;
	    strcpy(avpath, wdirs[curdir]);
	    strcat(avpath, sp);
	    DBG { fprintf(stderr, "DIR:%s\n", avpath); }
	    curdir = add_watch(ntfydir, avpath, IN_CREATE);
	    strcpy(wdirs[curdir], avpath);
	    fformat(wdirs[curdir]);
	    cc = add_watch(ntfyfile, avpath, IN_CLOSE_WRITE);
	    sp = idx + 1;
	}
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
	    curdir = add_watch(ntfydir, avpath, IN_CREATE);
	    strcpy(wdirs[curdir], avpath);
	    fformat(wdirs[curdir]);
	    VMODE {
		fprintf(stderr, "now watching directory %s, dirid(%d)\n",
			wdirs[curdir], curdir);
	    }
	    cc = add_watch(ntfyfile, avpath, IN_CLOSE_WRITE);
	} else if (FD_ISSET(ntfyfile, &readfds)) {
	    /* a new/modified file is closed, IN_CLOSE_WRITE */
	    if ((sz = read(ntfyfile, nevtbuf, BUFSIZE)) < 0) {
		perror("read");	exit(-1);
	    }
	    if (iep->mask&IN_IGNORED) goto next; /* inotify_rm_watch issued */
	    strcpy(avpath, wdirs[iep->wd]);
	    strcat(avpath, iep->name);
	    func(avpath, args);
	}
    next:
	FD_ZERO(&readfds);
	FD_SET(ntfydir, &readfds);
	FD_SET(ntfyfile, &readfds);
    }
    return 0;
}
