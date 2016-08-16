#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>

#define USE_LOCKF 1

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
#ifdef USE_LOCKF
	cc = lockf(fd, F_LOCK, 0);
#else
	cc = flock(fd, LOCK_EX);
#endif
	if (cc < 0) {
	    perror("flock:");
	    return -1;
	}
	printf("cc: %d >", cc);
	(void) getc(stdin);
	printf("releasing\n");
#ifdef USE_LOCKF
	cc = lockf(fd, F_ULOCK, 0);
#endif
	close(fd);
	printf("lock again?");
	(void) getc(stdin);
	fd = open(LCK_FILE, O_CREAT|O_RDWR, 0666);
	if (fd < 0) break;
    } while (1);
    return 0;
}
