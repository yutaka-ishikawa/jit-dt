/*
 *	Just In Time Data Transfer
 *	12/10/2016 For cluster environment, Yutaka Ishikawa
 */
#ifdef MPIENV
#include <mpi.h>
#endif /* MPIENV */
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include "jitclient.h"
#include "jitcclient.h"

extern int	_jitopen(char*, char*, int type);
extern int	_jitclose(int);
extern int	_jitread(int, void*, size_t);
extern int	_jitget(char*, char*, void*, int*, int);

#ifdef MPIENV
#define ROOT		0
static int
is_myrank() {
    static int	myrank = -1;
    if (myrank < 0) MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    return myrank;
}
#endif

int
jitopen(char *place, char *fname, int type)
{
    int		sock;
#ifdef MPIENV
    int		sz = 0;
    if (is_myrank() == ROOT) {
	sock = _jitopen(place, fname, type);
	if (fname != NULL) sz = strlen(fname);
    }
    MPI_Bcast(&sock, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    MPI_Bcast(&sz, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    if (sz > 0) {
	MPI_Bcast(fname, sz, MPI_BYTE, ROOT, MPI_COMM_WORLD);
	fname[sz] = 0;
    }
#else
    sock = _jitopen(place, fname, type);
#endif
    //printf("jitopen: fname=%s\n", fname);
    return sock;
}

int
jitclose(int sock)
{
    int		cc;
#ifdef MPIENV
    if (is_myrank() == ROOT) {
	cc = _jitclose(sock);
    }
    MPI_Bcast(&cc, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
#else
    cc = _jitclose(sock);
#endif
    return cc;
}

int
jitread(int sock, void *buf, size_t size)
{
    int		sz;
#ifdef MPIENV
    if (is_myrank() == ROOT) {
	sz = _jitread(sock, buf, size);
    }
    MPI_Bcast(&sz, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    if (sz > 0) {
	MPI_Bcast(buf, sz, MPI_BYTE, ROOT, MPI_COMM_WORLD);
    }
#else
    sz = _jitread(sock, buf, size);
#endif
    //printf("jitread: size(%d)\n", sz);
    return sz;
}

int
jitget(char *place, char *fname, void *data, int *bsize, int entries)
{
    int		cc;
#ifdef MPIENV
    int		i, ptr;
    int		rsize[TRANSOPT_SIZE];

    if (entries > TRANSOPT_SIZE) {
	fprintf(stderr, "Too many data entries. must be smaller than %d\n",
		TRANSOPT_SIZE);
	return -1;
    }
    if (is_myrank() == ROOT) {
	memcpy(rsize, bsize, sizeof(int)*entries);
	cc = _jitget(place, fname, data, rsize, entries);
    }
    MPI_Bcast(&cc, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    MPI_Bcast(rsize, entries, MPI_INT, ROOT, MPI_COMM_WORLD);
    ptr = 0;
    for (i = 0; i < entries; i++) {
	if (rsize[i] > 0) {
	    MPI_Bcast(((char*)data) + ptr, rsize[i], MPI_BYTE,
		      ROOT, MPI_COMM_WORLD);
	    ptr += bsize[i];	 /* next data area */
	    bsize[i] = rsize[i]; /* read size */
	} 
    }
#else
    cc = _jitget(place, fname, data, bsize, entries);
#endif
    //printf("jitread: size(%d)\n", sz);
    return cc;
}
