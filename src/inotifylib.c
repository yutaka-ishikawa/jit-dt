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
reset_watch(int ntfydir, int ntfyfile)
{
    int		wd, i;

    if (ntfydir == 0 || ntfyfile == 0) return;
    for (i = 0; i < MAX_KEEPDIR; i++) {
	if ((wd = qrmdir[i]) > 0) {
	    if (inotify_rm_watch(ntfydir, wd) < 0) perror("inotify_rm_watch");
	    if (inotify_rm_watch(ntfyfile, wd) < 0) perror("inotify_rm_watch");
	}
    }
    curqrmd = 0;
}

/*
 * Actual removing watching directory is postponed in rm_watch
 */
static void
rm_watch(int wd, int ntfydir, int ntfyfile, int flag)
{
    if (qrmdir[curqrmd] > 0) {
	VMODE {
	    fprintf(stderr,"leaving the directory %s (curdir=%d)\n",
		    getwdir(curdir), curdir);
	}
	if (inotify_rm_watch(ntfydir, wd) < 0) perror("inotify_rm_watch");
	if (inotify_rm_watch(ntfyfile, wd) < 0) perror("inotify_rm_watch");
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
		"Warning: cannot find file: %s\n", avpath);
	return -1;
    }
    return 0;
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
restart:
    ntfydir = inotify_init();  ntfyfile = inotify_init();
    if (ntfydir < 0 || ntfyfile < 0) {
	perror("inotify_init:");
	exit(-1);
    }
    reset_watch(ntfydir, ntfyfile);
    FD_ZERO(&readfds); FD_SET(ntfydir, &readfds); FD_SET(ntfyfile, &readfds);
    nfds = ntfyfile + 1;
    cc = add_watch(ntfydir, topdir, IN_CREATE);
    strcpy(getwdir(cc), topdir);
    curdir = cc;
    cc = add_watch(ntfyfile, topdir, IN_CLOSE_WRITE|IN_MOVED_TO);
    /* starting directory */
    if (startdir) {
	char	*idx, *sp = startdir;
	fformat(sp);
	while ((idx = index(sp, '/'))) {
	    *idx = 0;
	    strcpy(avpath, getwdir(curdir));
	    strcat(avpath, sp);
	    DBG { fprintf(stderr, "DIR:%s\n", avpath); }
	    curdir = add_watch(ntfydir, avpath, IN_CREATE);
	    strcpy(getwdir(curdir), avpath);
	    fformat(getwdir(curdir));
	    cc = add_watch(ntfyfile, avpath, IN_CLOSE_WRITE|IN_MOVED_TO);
	    sp = idx + 1;
	}
    }
    VMODE {
	fprintf(stderr, "now watching directory %s, dirid(%d)\n",
		getwdir(curdir), curdir);
    }
    while (select(nfds, &readfds, NULL, NULL, NULL) > 0) {
	if (FD_ISSET(ntfydir, &readfds)) {
	    /* a new directory might be created, IN_CREAT */
	    /* if it is a new file, then skip */
	    if ((sz = read(ntfydir, nevtbuf, BUFSIZE)) < 0) {
		perror("read");	exit(-1);
	    }
	    DBG {
		fprintf(stderr, "*** new event for directory (%0x)***\n", iep->mask);
		fprintf(stderr, "\t\t%s\n", iep->name);
	    }
	    if (iep->mask&IN_IGNORED) goto skip; /* inotify_rm_watch issued */
	    strcpy(avpath, getwdir(iep->wd));
	    strcat(avpath, iep->name);
	    if ((cc = stat(avpath, &sbuf)) != 0) {
		DBG { fprintf(stderr, "directory ??? %s\n", avpath); }
		goto skip;
	    }
	    if (!S_ISDIR(sbuf.st_mode)) goto next; /* not a directory */
	    DBG {
		fprintf(stderr, "DIR: dirid(%d) name(%s) curdir(%d)\n",
			iep->wd, iep->name, curdir);
	    }
	    if (iep->wd != curdir) {
		/*
		 * The upper level directory is created. This means
		 * no more wating the current directory.
		 */
		rm_watch(curdir, ntfydir, ntfyfile, flag);
	    }
	    if (IS_EXHAUST(curdir)) goto resetting;
	    curdir = add_watch(ntfydir, avpath, IN_CREATE);
	    strcpy(getwdir(curdir), avpath);
	    fformat(getwdir(curdir));
	    VMODE {
		fprintf(stderr, "now watching directory %s, dirid(%d)\n",
			getwdir(curdir), curdir);
	    }
	    cc = add_watch(ntfyfile, avpath, IN_CLOSE_WRITE|IN_MOVED_TO);
	}
	skip:
	if (FD_ISSET(ntfyfile, &readfds)) {
	    /* a new/modified file is closed or moved, IN_CLOSE_WRITE */
	    if ((sz = read(ntfyfile, nevtbuf, BUFSIZE)) < 0) {
		perror("read");	exit(-1);
	    }
	    DBG {
		fprintf(stderr, "*** new event for file (%0x)***\n", iep->mask);
	    }
	    if (iep->mask&IN_IGNORED) goto next; /* inotify_rm_watch issued */
	    strcpy(avpath, getwdir(iep->wd));
	    strcat(avpath, iep->name);
	    if (iep->name[0] == '.') {
                if ((cc = stat(avpath, &sbuf)) != 0) {
		    /* rsync might create this file and rename it */
		    DBG_VMODE {
			fprintf(stderr, "%s disapears and rescue\n", iep->name);
		    }
		    /* avpath will be reset */
		    if (rescuefile(avpath, getwdir(iep->wd),iep->name) < 0) goto next;
                } else {
		    /* ignore */
		    DBG {
			fprintf(stderr, "ignore file: %s exists\n", iep->name);
		    }
		    goto next;
		}
	    }
	    func(avpath, args);
	}
    next:
	FD_ZERO(&readfds);
	FD_SET(ntfydir, &readfds);
	FD_SET(ntfyfile, &readfds);
    }
    return 0;
resetting:
    close(ntfydir); close(ntfyfile);
    /* startdir is newly defined using the current avpath */
    strcpy(tmppath, &avpath[strlen(topdir)]);
    startdir = tmppath;
    VMODE {
	fprintf(stderr, "restarting from %s\n",	startdir);
    }
    goto restart;
}
