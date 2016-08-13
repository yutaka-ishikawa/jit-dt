#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

# define TIMEVAL_TO_TIMESPEC(tv, ts) {		\
    (ts)->tv_sec = (tv)->tv_sec;		\
    (ts)->tv_nsec = (tv)->tv_usec * 1000;	\
}

int
clock_gettime (clockid_t clock_id, struct timespec *tp)
{
    struct timeval tv;
    int retval = -1;
    retval = gettimeofday (&tv, NULL);
    if (retval == 0)
	TIMEVAL_TO_TIMESPEC (&tv, tp);
    return retval;
}
