/*
 *	Just In Time Data Transfer
 *	15/08/2016 For locked move function, Yutaka Ishikawa
 */
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include "translib.h"
#if 0
#include <sys/wait.h>
#include <signal.h>
#include <libgen.h>
#endif


static char	fpath[PATH_MAX];
static char	lckpath[PATH_MAX];
static int	lckfd;

void
locked_lock(char *path)
{
    static int	first = 0;
    int		cc;

    if (first == 0) {
	char	*idx;
	first = 1;
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
	strcat(lckpath, LCK_FILE);
    }
    if ((lckfd = open(lckpath, O_CREAT|O_TRUNC|O_RDWR, 0666)) < 0) {
	fprintf(stderr, "Lock file %s cannot be created\n", LCK_FILE);
	perror("open");
	exit(-1);
    }
    if ((cc = flock(lckfd, LOCK_EX)) < 0) {
	fprintf(stderr, "Lock file %s cannot be locked\n", LCK_FILE);
	perror("flock");
	exit(-1);
    }
}

void
locked_unlock()
{
    int	cc;
    if ((cc = close(lckfd)) < 0) {
	fprintf(stderr, "Somthing wrong in closing lock file %s\n", LCK_FILE);
	perror("close");
    }
}

void
locked_write(char *info)
{
    int	cc;

    if ((cc = write(lckfd, info, strlen(info) + 1)) < 0) {
	fprintf(stderr, "Cannot write data in lock file %s\n", LCK_FILE);
	perror("write");
    }
}

int
locked_read(char *buf, int size)
{
    int	cc;

    if ((cc = read(lckfd, buf, size)) < 0) {
	fprintf(stderr, "Cannot read data in lock file %s\n", LCK_FILE);
	perror("read");
    }
    return cc;
}


int
jitopen(char *place, char *fname)
{
    int		fd;
    int		sz;

    locked_lock(place);
    sz = locked_read(fpath, PATH_MAX);
    if (sz <= 0) {
	fprintf(stderr, "Cannot get a file from %s\n", place);
	return -1;
    }
    fd = open(fpath, O_RDONLY);
    if (fd > 0 && fname != NULL) {
	strcpy(fname, fpath);
    }
    return fd;
}

int
jitclose(int fd)
{
    int		cc;

    cc = close(fd);
    locked_unlock();
    return cc;
}
