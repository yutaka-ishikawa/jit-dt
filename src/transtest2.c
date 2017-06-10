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
#include "jitclient.h"

#define ITER		10
#define BUFSIZE		(40*1024*1024)
#define DBUF_SIZE	128

#ifdef MPIENV
static int	nprocs, myrank;
#endif

static char	fname[PATH_MAX+1];
static char	timebuf[DBUF_SIZE];
static char	timefmtbuf[DBUF_SIZE];
static char	place[PATH_MAX+1];

struct vrze {
    char	vr[BUFSIZE];
    char	ze[BUFSIZE];
    char	qc[BUFSIZE];
} data;
/*
 * vrzeqc_size[0] : IN  buffer sizes
 * vrzeqc_size[1] : OUT read sizes
 */
struct vrze_size {
    int		vsize;
    int		zsize;
    int		qsize;
} vrzeqc_size[2];

int
main(int argc, char **argv)
{
    int		i, iter;
    struct timeval	time;
    struct timezone	tzone;


    if (argc < 2) {
	fprintf(stderr, "usage: %s [<watching directory>|<host name>] "
		"[# of iter]\n", argv[0]);
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
    vrzeqc_size[0].vsize
	= vrzeqc_size[0].zsize
	= vrzeqc_size[0].qsize = BUFSIZE;
    for (i = 0; i < iter; i++) {
	memset(timebuf, 0, DBUF_SIZE);
	if (jitget(place, fname, &data, &vrzeqc_size) < 0) {
	    printf("Cannot get a file\n");
	    continue;
	}
	mygettime(&time, &tzone);
	dateconv(&time, timebuf, timefmtbuf);
#ifdef MPIENV
	if (myrank == 0) {
#endif
	    printf("vr(%p) size(%d)\n", data.vr, vrzeqc_size[1].vsize);
	    printf("ze(%p) size(%d)\n", data.ze, vrzeqc_size[1].zsize);
	    printf("ze(%p) size(%d)\n", data.qc, vrzeqc_size[1].qsize);
	    printf("%s,%s\n", fname, timefmtbuf);
	    fflush(stdout);
#ifdef MPIENV
	}
#endif
    }
    return 0;
}
