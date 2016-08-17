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

char	fname[PATH_MAX];
char	buf[BUFSIZE];
char	timebuf[DBUF_SIZE];
char	timefmtbuf[DBUF_SIZE];


int
main(int argc, char **argv)
{
    char	*place;
    int		i;
    int		fd;
    int		cc;
    ssize_t	sz, totsz;
    struct timeval	time;
    struct timezone	tzone;


    if (argc < 2) {
	fprintf(stderr, "usage: %s <watching directory>\n", argv[0]);
	return -1;
    }
    place = argv[1];
    for (i = 0; i < ITER; i++) {
	memset(timebuf, 0, DBUF_SIZE);
	if ((fd = jitopen(place, fname)) < 0) {
	    fprintf(stderr, "Cannot open\n");
	    return -1;
	}
	//printf("fname=%s\n", fname);
	cc = system(buf);
	if (cc < 0) {
	    fprintf(stderr, "The \"system\" system call returns %d\n", cc);
	    perror("system:");
	}
	while ((sz = read(fd, buf, BUFSIZE)) > 0) {
	    totsz += sz;
	}
	jitclose(fd);
	mygettime(&time, &tzone);
	dateconv(&time, timebuf, timefmtbuf);
	printf("%s was read at %s\n", fname, timefmtbuf);
    }
    return 0;
}
