/*
 *	Just In Time Data Transfer
 *	15/08/2016 For locked move function, Yutaka Ishikawa
 */
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "translib.h"
#include "translocklib.h"

#define USE_LOCKF	1

static int	ticktack = 0;
static char	fpath[PATH_MAX];
static char	lckpath[PATH_MAX]; /* temporary */
//static int	lckfd;
static char	*lockfname[2] = { LCK_FILE_1, LCK_FILE_2 };
#define FD_MAX	1024
static int	lckfds[FD_MAX];
static char	*lckname[FD_MAX];

char *
locked_name(int lckfd)
{
    return lckname[lckfd];
}

int
locked_lock(char *path)
{
    int		lckfd;
    int		cc;
    char	*idx;

    strcpy(lckpath, path);
    if ((idx = rindex(lckpath, '/'))) {
	if (*(idx + 1) != 0) {
	    /* file name is included. remove it */
	    *(idx + 1) = 0;
	} /* last '/' means a directory */
    } else {
	/* otherwise file name is specified */
	strcpy(lckpath, "./");
    }
    fformat(lckpath);
    strcat(lckpath, lockfname[ticktack]);
    if ((lckfd = open(lckpath, O_CREAT|O_RDWR, 0666)) < 0) {
	fprintf(stderr, "Lock file %s cannot be created\n", lckpath);
	perror("open");
	exit(-1);
    }
#ifdef USE_LOCKF
    if ((cc = lockf(lckfd, F_LOCK, 0)) < 0) {
	fprintf(stderr, "Lock file %s cannot be locked\n", lckpath);
	perror("lockf");
	exit(-1);
    }
#else
    if ((cc = flock(lckfd, LOCK_EX)) < 0) {
	fprintf(stderr, "Lock file %s cannot be locked\n", lckpath);
	perror("flock");
	exit(-1);
    }
#endif
    lckname[lckfd] = lockfname[ticktack];
    ticktack ^= 1;
    return lckfd;
}

void
locked_unlock(int lckfd)
{
    int	cc;

#ifdef USE_LOCKF
    if ((cc = lockf(lckfd, F_ULOCK, 0)) < 0) {
	fprintf(stderr, "Cannot be unlocked\n");
	perror("lockf");
	exit(-1);
    }
#endif /* USE_LOCKF */
    if ((cc = close(lckfd)) < 0) {
	fprintf(stderr, "Somthing wrong in closing lock file\n");
	perror("close");
    }
}

void
locked_write(int lckfd, char *info)
{
    int	cc;

    if ((cc = write(lckfd, info, strlen(info) + 1)) < 0) {
	fprintf(stderr, "Cannot write data in lock file\n");
	perror("write");
    }
}

int
locked_read(int lckfd, char *buf, int size)
{
    char	*idx;
    int		cc;

    if ((cc = read(lckfd, buf, size)) < 0) {
	fprintf(stderr, "Cannot read data in lock file\n");
	perror("read");
    }
    if ((idx = index(buf, '\n'))) *idx = 0;
    return cc;
}

char *
jitname(int fd)
{
    return lckname[lckfds[fd]];
}

int
jitopen(char *place, char *fname)
{
    int		lckfd, fd;
    int		sz;

    lckfd = locked_lock(place);
    sz = locked_read(lckfd, fpath, PATH_MAX);
    if (sz <= 0) {
	fprintf(stderr, "Cannot get a file from %s\n", place);
	return -1;
    }
    fd = open(fpath, O_RDONLY);
    if (fd > 0) {
	if (fname != NULL) strcpy(fname, fpath);
	lckfds[fd] = lckfd;
    }
    return fd;
}

int
jitclose(int fd)
{
    int		cc;

    cc = close(fd);
    locked_unlock(lckfds[fd]);
    return cc;
}

void
mygettime(struct timeval *sp, struct timezone *tp)
{
    if (gettimeofday(sp, tp) < 0) {
	perror("gettimeofday:"); exit(-1);
    }
}

#define IDX_WDAY	0
#define IDX_MONTH	1
#define IDX_DATE	2
#define IDX_TIME	3
#define IDX_YEAR	4

void
dateconv(struct timeval	*tsec, char *dbufp, char *fmtbuf)
{
    int		i;
    char	*idx, *nidx;
    char	*dp[10];

    ctime_r(&tsec->tv_sec, dbufp);
    for (i = 0, idx = dbufp, nidx = index(dbufp, ' ');
	 nidx != NULL; idx = nidx + 1, nidx = index(nidx + 1, ' '), i++) {
	dp[i] = idx;
	*nidx = 0;
    }
    dp[i] = idx; if ((idx = index(idx, '\n'))) *idx = 0;

    sprintf(fmtbuf, "%s:%s:%s:%s:%d",
	    dp[IDX_YEAR], dp[IDX_MONTH], dp[IDX_DATE], dp[IDX_TIME],
	    ((int)(long long)tsec->tv_usec)/1000);
    return;
}
