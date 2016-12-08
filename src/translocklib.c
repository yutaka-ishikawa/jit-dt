/*
 *	Just In Time Data Transfer
 *	15/08/2016 For locked move function, Yutaka Ishikawa
 */
#ifdef MPIENV
#include <mpi.h>
#endif /* MPIENV */
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
#define ROOT		0

static int	ticktack = 0;
static char	fpath[PATH_MAX+1];
static char	lckpath[PATH_MAX+1]; /* temporary */
//static int	lckfd;
static char	*lockfname[2] = { LCK_FILE_1, LCK_FILE_2 };
#define FD_MAX	1024
static int	lckfds[FD_MAX];
static char	*lckname[FD_MAX];

#ifdef MPIENV
static int
is_myrank() {
    static int	myrank = -1;
    if (myrank < 0) MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    return myrank;
}
#endif

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
	fprintf(stderr, "Lock file %s cannot be locked (%d)\n", lckpath, cc);
	perror("lockf");
	exit(-1);
    }
#else
    if ((cc = flock(lckfd, LOCK_EX)) < 0) {
	fprintf(stderr, "Lock file %s cannot be locked (%d)\n", lckpath, cc);
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
	fprintf(stderr, "Cannot be unlocked (%d)\n", cc);
	perror("lockf");
	exit(-1);
    }
#endif /* USE_LOCKF */
    if ((cc = close(lckfd)) < 0) {
	fprintf(stderr, "Somthing wrong in closing lock file (%d)\n", cc);
	perror("close");
    }
}

void
locked_write(int lckfd, char *info)
{
    int	cc;

    if ((cc = write(lckfd, info, strlen(info) + 1)) < 0) {
	fprintf(stderr, "Cannot write data in lock file (%d)\n", cc);
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
_jitopen(char *place, char *fname)
{
    int		lckfd, fd;
    int		sz;

    lckfd = locked_lock(place);
    sz = locked_read(lckfd, fpath, PATH_MAX);
    if (sz <= 0) {
	fprintf(stderr, "Cannot get a file from %s\n", place);
	close(lckfd);
	return -1;
    }
    fpath[sz] = 0;
    fd = open(fpath, O_RDONLY);
    if (fd > 0) {
	if (fname != NULL) strcpy(fname, fpath);
	lckfds[fd] = lckfd;
    } else {
	if (fname != NULL) fname[0] = 0;
    }
    return fd;
}

int
jitopen(char *place, char *fname)
{
    int		fd;
#ifdef MPIENV
    int		sz = 0;
    if (is_myrank() == ROOT) {
	fd = _jitopen(place, fname);
	if (fname != NULL) sz = strlen(fname);
    }
    MPI_Bcast(&fd, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    MPI_Bcast(&sz, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    if (sz > 0) {
	MPI_Bcast(fname, sz, MPI_BYTE, ROOT, MPI_COMM_WORLD);
	fname[sz] = 0;
    }
#else
    fd = _jitopen(place, fname);
#endif
    return fd;
}

int
_jitclose(int fd)
{
    int		cc;

    cc = close(fd);
    locked_unlock(lckfds[fd]);
    return cc;
}

int
jitclose(int fd)
{
    int		cc;
#ifdef MPIENV
    if (is_myrank() == ROOT) {
	cc = _jitclose(fd);
    }
    MPI_Bcast(&cc, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
#else
    cc = _jitclose(fd);
#endif
    return cc;
}

int
jitread(int fd, void *buf, size_t size)
{
    int		sz;
#ifdef MPIENV
    if (is_myrank() == ROOT) {
	sz = read(fd, buf, size);
    }
    MPI_Bcast(&sz, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    if (sz > 0) {
	MPI_Bcast(buf, sz, MPI_BYTE, ROOT, MPI_COMM_WORLD);
    }
#else
    sz = read(fd, buf, size);
#endif
    return sz;
}


void
mygettime(struct timeval *sp, struct timezone *tp)
{
    if (gettimeofday(sp, tp) < 0) {
	perror("gettimeofday:"); exit(-1);
    }
}

void
timeconv(struct timeval *tsec, char *fmtbuf)
{
    struct tm	tmst;

    localtime_r(&tsec->tv_sec, &tmst);
    sprintf(fmtbuf, "%4d%02d%02d%02d%02d%02d.%d",
	    tmst.tm_year + 1900, tmst.tm_mon + 1, tmst.tm_mday,
	    tmst.tm_hour, tmst.tm_min, tmst.tm_sec,
	    ((int)(long long)tsec->tv_usec)/1000);
    return;
}

#define IDX_WDAY	0
#define IDX_MONTH	1
#define IDX_DATE	2
#define IDX_TIME	3
#define IDX_YEAR	4

static char	*mname[] = {
    "Jan",  "Feb",  "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
    "Nov", "Dec"};

static int
conv_month(char *month)
{
    int		i;
    for (i = 0; i < 12; i++) {
	if (!strcmp(mname[i], month)) {
	    return i+1;
	}
    }
    /* something wrong */
    return 0;
}

void
dateconv(struct timeval	*tsec, char *dbufp, char *fmtbuf)
{
    int		i;
    char	*idx, *nidx;
    char	*dp[10];

    ctime_r(&tsec->tv_sec, dbufp);
    for (i = 0, idx = dbufp, nidx = index(dbufp, ' ');
	 nidx != NULL; idx = nidx + 1, nidx = index(nidx + 1, ' '), i++) {
	while (*idx == ' ') { /* in case of Thu Dec  8 12:35:31 2016 */
	    idx++; nidx = index(nidx + 1, ' ');
	}
	dp[i] = idx;
	*nidx = 0;
    }
    dp[i] = idx; if ((idx = index(idx, '\n'))) *idx = 0;

    sprintf(fmtbuf, "%s:%d:%s:%s:%d",
	    dp[IDX_YEAR], conv_month(dp[IDX_MONTH]), dp[IDX_DATE], dp[IDX_TIME],
	    ((int)(long long)tsec->tv_usec)/1000);
    return;
}
