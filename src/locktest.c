#include <stdio.h>
#include <sys/file.h>

#define LCK_FILE	"flock"
int
main(int argc, char **argv)
{
    int		fd;
    int		cc;

    fd = open(LCK_FILE, O_CREAT|O_RDWR, 0666);
    if (fd < 0) {
	fprintf(stderr, "Cannot create lock file: %s\n", LCK_FILE);
	return -1;
    }
    do {
	printf("locking ...\n");
	cc = flock(fd, LOCK_EX);
	printf("cc: %d >", cc);
	(void) getc(stdin);
	printf("releasing\n");
	close(fd);
	printf("lock again?");
	(void) getc(stdin);
	fd = open(LCK_FILE, O_CREAT|O_RDWR, 0666);
	if (fd < 0) break;
    } while (1);
    return 0;
}
