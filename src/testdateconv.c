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

#define DBUF_SIZE	1024
static char	timebuf[DBUF_SIZE];
static char	timefmtbuf[DBUF_SIZE];

int
main(int argc, char **argv)
{
    struct timeval	time;
    struct timezone	tzone;

    mygettime(&time, &tzone);
    dateconv(&time, timebuf, timefmtbuf);
    printf("%s\n", timefmtbuf);
    return 0;
}
