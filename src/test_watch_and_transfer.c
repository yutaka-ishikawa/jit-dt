/*
 *	Just In Time Data Transfer
 *	15/08/2016 Adding locked move function, Yutaka Ishikawa
 *	13/08/2016 Adding sftp protocol
 *			by Yutaka Ishikawa, RIKEN AICS 
 *	15/12/2015 Written by Yutaka Ishikawa, RIKEN AICS
 *			yutaka.ishikawa@riken.jp
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <signal.h>
#include "translib.h"

//#define PATH_WATCH	"/home/ishikawa/tmp/123"
#define PATH_WATCH	"./"
#define NAME_MAX	1024
#define BUFSIZE		(NAME_MAX + 1 + sizeof(struct inotify_event))
char	buf[BUFSIZE];
char	wapath[NAME_MAX];
char	avpath[NAME_MAX];
int	vflag;
int	dflag;

static void
terminate(int num)
{
    fprintf(stderr, "sftp_terminate\n");
    sftp_terminate();
}


int
main(int argc, char **argv)
{
    int		opt;
    char	*url, *hname, *rpath;
    int		notifyfd, cc;
    int		ttype;
    ssize_t	sz;
    double	sec;
    struct inotify_event	*iep;

    if (argc < 3 || argc > 5) {
	fprintf(stderr,
		"USAGE: %s <url> <watching directory path> [-d] [-v]\n",
		argv[0]);
	fprintf(stderr, "e.g.: %s http://kncc-login1.kncc.cc.u-tokyo.ac.jp \\\n"
		"            /home/yisikawa/work/FileTransfer/tmp/\n", argv[0]);
	return -1;
    }
    if (strlen(argv[2]) >= NAME_MAX - 1) {
	fprintf(stderr, "Too long directory path (%ld)\n", strlen(argv[2]));
	return -1;
    }
    url = argv[1];
    strcpy(wapath, (argc < 3) ? PATH_WATCH : argv[2]);
    if (wapath[strlen(wapath) - 1] != '/') {
	strcat(wapath, "/");
    }
    DBG {
	dflag = 0;
    }
    if (argc > 3) {
	while ((opt = getopt(argc, argv, "dv")) != -1) {
	    switch (opt) {
	    case 'd':
#ifdef DEBUG
		dflag = 1;
#else
		fprintf(stderr, "Cannot specify debug option. please recompile it with -DDEBUG option\n");
		return -1;
#endif /* DEBUG */
		break;
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
    hname = NULL; rpath = NULL;
    ttype = trans_type(url, &hname, &rpath);
    if (ttype < 0) {
	(ttype == TRANS_UNKNOWN) ?
	    fprintf(stderr, "Unknown transfer method: %s\n", url)
	    : fprintf(stderr, "No transfer method is specified");
	exit(-1);
    }
    notifyfd = inotify_init();
    if (notifyfd < 0) {
	perror("inotify_init:");
	exit(-1);
    }
    cc = inotify_add_watch(notifyfd, wapath, IN_CLOSE_WRITE);
    if (cc < 0) {
 	perror("inotify_add_watch:");
	exit(-1);
    }
    signal(SIGINT, terminate);
    VMODE {
	fprintf(stderr, "now watching %s\n", wapath);
    }
    while ((sz = read(notifyfd, buf, BUFSIZE)) > 0) {
	if (sz < 0) {
	    perror("read");
	    exit(-1);
	}
	iep = (struct inotify_event *) buf;
	DBG {
	    fprintf(stderr, "sz(%ld) len(%d) name(%s)\n",
		    sz, iep->len, iep->name);
	}
	strcpy(avpath, wapath);
	strcat(avpath, iep->name);
	VMODE {
	    fprintf(stderr, "sending %s to %s\n", avpath, url);
	}
	sec = (*ttable[ttype])(hname, rpath, avpath);
	VMODE {
	    fprintf(stderr, "%s, %f\n", avpath, sec); fflush(stderr);
	    fprintf(stderr, "watching %s\n", wapath);
	}
    }
    return 0;
}
