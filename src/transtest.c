#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include "translocklib.h"

#define ITER	10
#define BUFSIZE 1024
char	fname[PATH_MAX];
char	buf[BUFSIZE];

int
main(int argc, char **argv)
{
    char	*place;
    int		i;
    int		fd;
    ssize_t	sz;

    if (argc < 2) {
	fprintf(stderr, "usage: %s <watching directory>\n", argv[0]);
	return -1;
    }
    place = argv[1];
    for (i = 0; i < ITER; i++) {
	if ((fd = jitopen(place, fname)) < 0) {
	    fprintf(stderr, "Cannot open\n");
	    return -1;
	}
	printf("lock file is %s\n", locked_name(fd));
	while ((sz = read(fd, buf, BUFSIZE)) > 0) {
	    if (write(1, buf, sz) < 0) perror("write:");
	}
	jitclose(fd);
    }
    return 0;
}
