#ifdef MPIENV
#include <mpi.h>
#endif /* MPIENV */
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "translocklib.h"

#define ITER		10
#define BUFSIZE		1024
#define DBUF_SIZE	128

#ifdef MPIENV
static int	nprocs, myrank;
#endif

static char	fname[PATH_MAX+1];
static char	buf[BUFSIZE];
static char	timebuf[DBUF_SIZE];
static char	timefmtbuf[DBUF_SIZE];
static char	place[PATH_MAX+1];

int
main(int argc, char **argv)
{
    int		i, iter;
    int		fd;
    ssize_t	sz, totsz;
    struct timeval	time;
    struct timezone	tzone;


    if (argc < 2) {
	fprintf(stderr, "usage: %s <watching directory> [# of iter]\n", argv[0]);
	return -1;
    }
    if (argc == 3) {
	iter = atoi(argv[2]);
    } else {
	iter = ITER;
    }
    strcpy(place, argv[1]);
#ifdef MPIENV
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    if (myrank == 0) {
	printf("Tanstest: nprocs(%d)\n", nprocs);
    }
#endif
    for (i = 0; i < iter; i++) {
	memset(timebuf, 0, DBUF_SIZE);
	if ((fd = jitopen(place, fname)) < 0) {
	    printf("Cannot open file:%s\n", fname);
	    continue;
	}
	totsz = 0;
	while ((sz = jitread(fd, buf, BUFSIZE)) > 0) {
	    totsz += sz;
	}
	jitclose(fd);
	mygettime(&time, &tzone);
	dateconv(&time, timebuf, timefmtbuf);
#ifdef MPIENV
	if (myrank == 0) {
#endif
	    printf("%s,%s,%ld\n", fname, timefmtbuf, totsz);
	    fflush(stdout);
#ifdef MPIENV
	}
#endif
    }
    return 0;
}
