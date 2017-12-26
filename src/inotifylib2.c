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

#define DBG		if (flag&MYNOTIFY_DEBUG)
#define VMODE		if (flag&MYNOTIFY_VERBOSE)
#define DBG_VMODE	if (flag&(MYNOTIFY_DEBUG|MYNOTIFY_DEBUG))
#define MAX_DIRS	(24*60*2+1)
#define MAX_KEEPDIR	4
#define PATH_WATCH	"./"
#define BUFSIZE		(PATH_MAX + 1 + sizeof(struct inotify_event))
#define IS_EXHAUST(idx)	(idx > (MAX_DIRS - 1))
int	vflag;
int	dflag;

fd_set	readfds;
int	curdir;
char	wdirs[MAX_DIRS][PATH_MAX];
char	nevtbuf[BUFSIZE];
char	avpath[PATH_MAX];
char	tmppath[PATH_MAX];
int	qrmdir[MAX_KEEPDIR];
int	curqrmd;

static char *
getwdir(int curid)
{
    unsigned int	idx = ((unsigned int) curid) % MAX_DIRS;
    return wdirs[idx];
}

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

static void
reset_watch(int ntfydir)
{
    int		wd, i;

    if (ntfydir == 0) return;
    for (i = 0; i < MAX_KEEPDIR; i++) {
	if ((wd = qrmdir[i]) > 0) {
	    if (inotify_rm_watch(ntfydir, wd) < 0) perror("inotify_rm_watch");
	}
    }
    curqrmd = 0;
}

/*
 * Actual removing watching directory is postponed in rm_watch
 */
static void
rm_watch(int wd, int ntfydir, int flag)
{
    int		dir;
    if ((dir = qrmdir[curqrmd]) > 0) {
	VMODE {
	    fprintf(stderr,"leaving the directory %s (dir=%d)\n",
		    getwdir(dir), dir);  fflush(stderr);
	}
	if (inotify_rm_watch(ntfydir, dir) < 0) perror("inotify_rm_watch");
    }
    qrmdir[curqrmd] = wd;
    curqrmd = (curqrmd + 1) % MAX_KEEPDIR;
}

/*
 * e.g.,
 *	XXX.dat.X1X9S3 == > XXX.dat
 */
void
rmtails(char *name)
{
    int		idx = strlen(name);
    while (--idx > 0) {
	if (name[idx] == '.') {
	    name[idx] = 0;
	    break;
	}
    }
}

/* file name is something like
 * ".kobe_20160919142330_A08_pawr_vr.dat.X1X9S3"
 * removing the first '.' and the last string ".X1X9S3"
 */
int
rescuefile(char *avpath, char *path, char *fname)
{
    int	cc;
    struct stat		sbuf;
    rmtails(&fname[1]);
    strcpy(avpath, path);
    strcat(avpath, &fname[1]);
    if ((cc = stat(avpath, &sbuf)) != 0) {
	fprintf(stderr,
		"Warning: cannot find file: %s\n", avpath);  fflush(stderr);
	return -1;
    }
    return 0;
}


int
mynotify(char *topdir, char *startdir,
	 void (*func)(char*, void**), void **args, int flag)
{
    int			ntfydir, nfds, curdir, cc;
    ssize_t		sz;
    struct stat		sbuf;
    struct inotify_event *iep = (struct inotify_event *) nevtbuf;

    DBG {
	fprintf(stderr, "IN_CREATE=0x%x IN_WRITE=0x%x\n",
		IN_CREATE, IN_CLOSE_WRITE);  fflush(stderr);
    }
restart:
    ntfydir = inotify_init();
    if (ntfydir < 0) {
	perror("inotify_init:");
	exit(-1);
    }
    reset_watch(ntfydir);
    FD_ZERO(&readfds); FD_SET(ntfydir, &readfds);
    nfds = ntfydir + 1;
    cc = add_watch(ntfydir, topdir, IN_CREATE|IN_CLOSE_WRITE|IN_MOVED_TO);
    strcpy(getwdir(cc), topdir);
    curdir = cc;
    /* starting directory */
    if (startdir) {
	char	*idx, *sp = startdir;
	fformat(sp);
	while ((idx = index(sp, '/'))) {
	    *idx = 0;
	    strcpy(avpath, getwdir(curdir)); strcat(avpath, sp);
	    DBG { fprintf(stderr, "DIR:%s\n", avpath);  fflush(stderr); }
	    curdir = add_watch(ntfydir, avpath,
			       IN_CREATE|IN_CLOSE_WRITE|IN_MOVED_TO);
	    strcpy(getwdir(curdir), avpath);
	    fformat(getwdir(curdir));
	    sp = idx + 1;
	}
    }
    VMODE {
	fprintf(stderr, "now watching directory %s, dirid(%d)\n",
		getwdir(curdir), curdir);  fflush(stderr);
    }
    while (select(nfds, &readfds, NULL, NULL, NULL) > 0) {
	if ((sz = read(ntfydir, nevtbuf, BUFSIZE)) < 0) {
	    perror("read");	exit(-1);
	}
	DBG {
	    fprintf(stderr, "*** new event for directory (%0x) name(%s) ",
		    iep->mask, iep->name);  fflush(stderr);
	}
	if (iep->mask&IN_IGNORED) goto next; /* inotify_rm_watch issued */
	strcpy(avpath, getwdir(iep->wd)); strcat(avpath, iep->name);
	if (S_ISDIR(sbuf.st_mode)) {
	    if (!(iep->mask & IN_CREATE)) goto next;
	    /* a directory */
	    DBG {
		fprintf(stderr, "DIR: dirid(%d) curdir(%d)\n",
			iep->wd, curdir);  fflush(stderr);
	    }
	    if (iep->wd != curdir) {
		/*
		 * The upper level directory is created. This means
		 * no more wating the current directory.
		 */
		rm_watch(curdir, ntfydir, flag);
	    }
	    if (IS_EXHAUST(curdir)) goto resetting;
	    curdir = add_watch(ntfydir, avpath,
			       IN_CREATE|IN_CLOSE_WRITE|IN_MOVED_TO);
	    /* setup curdir array */
	    strcpy(getwdir(curdir), avpath);
	    fformat(getwdir(curdir));
	    VMODE {
		fprintf(stderr, "now watching directory %s, dirid(%d)\n",
			getwdir(curdir), curdir);  fflush(stderr);
	    }
	} else { /* file */
	    DBG {
		fprintf(stderr, "FILE\n");  fflush(stderr);
	    }
	    /* checking if a file has been closed or moved */
	    if (!(iep->mask & (IN_CLOSE_WRITE|IN_MOVED_TO))) goto next;
	    if (iep->name[0] == '.') {/* might be temoraly file for rsync */
		goto next;
	    }
	    func(avpath, args);
	}
    next:
	FD_ZERO(&readfds);
	FD_SET(ntfydir, &readfds);
    }
    return 0;
resetting:
    close(ntfydir);
    /* startdir is newly defined using the current avpath */
    strcpy(tmppath, &avpath[strlen(topdir)]);
    startdir = tmppath;
    VMODE {
	fprintf(stderr, "restarting from %s\n",	startdir);  fflush(stderr);
    }
    goto restart;
}
