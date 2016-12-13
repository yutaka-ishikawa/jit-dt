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
#include "misclib.h"

#define USE_LOCKF	1
#define ROOT		0
#define FD_MAX	1024
static int	lckfds[FD_MAX];
static char	fpath[PATH_MAX+1];

#ifdef MPIENV
static int
is_myrank() {
    static int	myrank = -1;
    if (myrank < 0) MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    return myrank;
}
#endif


char *
jitname(int fd)
{
    return locked_name(lckfds[fd]);
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
