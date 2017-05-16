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
#include "jitclient.h"
#include "misclib.h"

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
_jitopen(char *place, char *fname, int type)
{
    int		lckfd, fd;
    int		sz;
    char	*files[FTYPE_NUM+1];

    lckfd = locked_lock(place);
    sz = locked_read(lckfd, fpath, PATH_MAX);
    if (sz <= 0) {
	fprintf(stderr, "Cannot get a file from %s\n", place);
	close(lckfd);
	return -1;
    }
    if (fpath[0] == 0) { /* kwacther daemon might die */
	close(lckfd);
	return -1;
    }
    fpath[sz] = 0;
    septype(fpath, files);
    fprintf(stderr, "_jitopen: open %s\n", files[type]);
    fd = open(files[type], O_RDONLY);
    if (fd > 0) {
	if (fname != NULL) strcpy(fname, files[type]);
	lckfds[fd] = lckfd;
    } else {
	if (fname != NULL) fname[0] = 0;
    }
    return fd;
}

int
_jitclose(int fd)
{
    int		cc;

    cc = close(fd);
    /* the content of the locked file become 4 byte zero data */
    locked_unlock_nullify(lckfds[fd]);
    return cc;
}

int
_jitread(int fd, void *buf, size_t size)
{
    int		sz;
    sz = read(fd, buf, size);
    return sz;
}

int
_jitget(char *place, char *fname, void *data, void *size)
{
    int		lckfd, fd;
    int		i, sz, ptr;
    char	*files[FTYPE_NUM+1];
    char	curname[PATH_MAX];
    obs_size	*szp = (obs_size*) size;
    int		*bsize, *rsize;

    lckfd = locked_lock(place);
    sz = locked_read(lckfd, fpath, PATH_MAX);
    if (fpath[0] == 0) { /* kwacther daemon might die */
	close(lckfd);
	return -1;
    }
    fpath[sz] = 0;
    septype(fpath, files);
    bsize = szp->elem; rsize = (szp + 1)->elem;
    fname[0] = 0;
    for (ptr = 0, i = 0; i < FTYPE_NUM; i++) {
	strcat(fname, files[i]);
	strcat(fname, FTSTR_SEPARATOR);
	strcpy(curname, place);
	strcat(curname, files[i]);
	if ((fd = open(curname, O_RDONLY)) < 0) {
	    fprintf(stderr, "jitget: cannot open file %s\n", curname);
	    sz = 0;
	} else {
	    sz = read(fd, ((char*)data) + ptr, bsize[i]);
	    close(fd);
	}
	rsize[i] = sz;
	ptr += bsize[i];
    }
    /* removing separator */
    fname[strlen(fname) - 1] = 0;
    locked_unlock_nullify(lckfd);
    return 0;
}
