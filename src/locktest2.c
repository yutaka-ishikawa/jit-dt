#include <stdio.h>
#include <sys/file.h>
#include <string.h>

#define LCK_FILE	"flock"
int
main(int argc, char **argv)
{
    int		fd;
    int		cc;
    char	buf[128];

    strcpy(buf, "testing\n");
    do {
	printf("locking?");
	(void) getc(stdin);
	unlink(LCK_FILE);
	fd = open(LCK_FILE, O_CREAT|O_RDWR|O_EXCL, 0666);
	if (fd < 0) break;
	printf("buf=%s, size=%ld\n", buf, strlen(buf));
	cc = write(fd, buf, strlen(buf));
	printf("cc: %d >", cc);
	(void) getc(stdin);
	printf("releasing\n");
	close(fd);
    } while (1);
    return 0;
}
