#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include "misclib.h"

#define FBUF_SIZE	(1000*1000)	/* MB */
#define DBUF_SIZE	128

char	fbuf[FBUF_SIZE];
char	stimebuf[DBUF_SIZE], etimebuf[DBUF_SIZE];
char	stimefmtbuf[DBUF_SIZE], etimefmtbuf[DBUF_SIZE];

void
fbufinit()
{
    char	*bp = fbuf;
    ssize_t	i;
    char	d = 0;

    for(i = 0; i < FBUF_SIZE; i++) {
	*bp = d++;
    }
}

void
transfer(int size, char *dir, char *fname)
{
    static char	path[PATH_MAX];
    int		fd;
    int		i;
    int		sz;

    sprintf(path, "%s/%s", (dir == 0) ? "./" : dir, fname);
    if ((fd = open(path, O_CREAT|O_TRUNC|O_RDWR, 0666)) < 0) {
	perror("open;"); exit(-1);
    }
    for (i = 0; i < size; i++) {
	if ((sz = write(fd, fbuf, FBUF_SIZE)) != FBUF_SIZE) {
	    fprintf(stderr, "Cannot write date in %s\n", path);
	    perror("write:");
	    exit(-1);
	}
    }
    if (close(fd) < 0) {
	fprintf(stderr, "Cannot close %s\n", path);
	perror("close:"); exit(-1);
    }
    return;
}


int
main(int argc, char **argv)
{
    int			wsec = 0;
    char		*dir = "./";
    int			iter = 1;
    int			size;
    int			opt;
    struct timeval	start, end;
    struct timezone	tzone;

    if (argc < 2) {
	fprintf(stderr, "usage: %s <size in MB> \\\n"
		"       [-i <interval in seconds>]"
		" [-p directory] [-c iteration count]\n", argv[0]);
	return -1;
    }
    size = atoi(argv[1]);
    while ((opt = getopt(argc, argv, "i:p:c:")) != -1) {
	switch (opt) {
	case 'i': /* interval */
	    wsec = atoi(optarg); break;
	case 'p': /* directory */
	    dir = optarg; break;
	case 'c': /* iteration count */
	    iter = atoi(optarg); break;
	}
    }
    printf("data size      : %d MB\n", size);
    printf("iteration count: %d times\n", iter);
    printf("interval time  : %d sec\n", wsec);
    printf("directory      : %s\n", dir);
    fbufinit();
    /*
     * Transfer file whose name is date
     */
    while (iter--) {
	memset(stimebuf, 0, DBUF_SIZE);
	memset(etimebuf, 0, DBUF_SIZE);
	mygettime(&start, &tzone);
	dateconv(&start, stimebuf, stimefmtbuf);
	transfer(size, dir, stimefmtbuf);
	mygettime(&end, &tzone);
	dateconv(&end, etimebuf, etimefmtbuf);
	printf("File  \"%s\" was created at %s\n", stimefmtbuf, etimefmtbuf);
	if (wsec == 0) break;
	if (iter) {
	    printf("... sleep for %d\n", wsec);
	    sleep(wsec);
	}
    }
    return 0;
}

