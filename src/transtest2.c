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
#define NUM_ENTRIES	2
#define DBUF_SIZE	128

#ifdef MPIENV
static int	nprocs, myrank;
#endif

static char	fname[PATH_MAX+1];
static char	timebuf[DBUF_SIZE];
static char	timefmtbuf[DBUF_SIZE];
static char	place[PATH_MAX+1];

struct data {
    char	d1[BUFSIZE];	/* 1st entry of the sync decl in conf */
    char	d2[BUFSIZE];	/* 2nd entry of the sync decl in conf */
} data;
/*
 * size : IN  buffer sizes, OUT read sizes
 */
int	sizes[NUM_ENTRIES];

int
main(int argc, char **argv)
{
    int		i, j, iter;
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
    for (i = 0; i < NUM_ENTRIES; i++) sizes[i] = BUFSIZE;
    for (i = 0; i < iter; i++) {
	memset(timebuf, 0, DBUF_SIZE);
	if (jitget(place, fname, &data, sizes, NUM_ENTRIES) < 0) {
	    printf("Cannot get a file\n");
	    continue;
	}
	mygettime(&time, &tzone);
	dateconv(&time, timebuf, timefmtbuf);
#ifdef MPIENV
	if (myrank == 0) {
#endif
	    printf("%s=> %s\n", timefmtbuf, fname);
	    for (j = 0; j < NUM_ENTRIES; j++) {
		printf("\tdata%d size(%d)\n", j, sizes[i]);
	    }
	    fflush(stdout);
#ifdef MPIENV
	}
#endif
    }
    return 0;
}
