#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include "translocklib.h"

#define BUFSIZE 1024
char	fname[PATH_MAX];
char	buf[BUFSIZE];

int
main(int argc, char **argv)
{
    char	*place;
    int		fd;
    ssize_t	sz;

    if (argc < 1) {
	fprintf(stderr, "usage: %s <watching directory>\n", argv[0]);
	return -1;
    }
    place = argv[1];
    if ((fd = jitopen(place, fname)) < 0) {
	fprintf(stderr, "Cannot open\n");
	return -1;
    }
    while ((sz = read(fd, buf, BUFSIZE)) > 0) {
	if (write(1, buf, sz) < 0) perror("write:");
    }
    jitclose(fd);
    return 0;
}
