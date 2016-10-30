#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LSIZE	1024
char	buf[LSIZE], cmnt[LSIZE];

void
ll2tm(long long dt, struct tm *tmp)
{
    memset(tmp, 0, sizeof(struct tm));
    tmp->tm_sec  = dt%100;
    tmp->tm_min  = (dt/100)%100;
    tmp->tm_hour = (dt/10000)%100;
    tmp->tm_mday  = (dt/1000000)%100;
    tmp->tm_mon  = (dt/100000000)%100;
    tmp->tm_year = dt/10000000000;
}

float
calc(long long date1, int msec, long long date2, float time)
{
    struct tm	tmst;
    time_t	tt1, tt2;
    float	result;

    ll2tm(date1, &tmst); tt1 = mktime(&tmst);
    ll2tm(date2, &tmst); tt2 = mktime(&tmst);
    result = (float) (tt1 - tt2) + ((float)msec/1000 + time);
    return result;
}

int
main(int argc, char **argv)
{
    long long	date1, date2;
    int		msec;
    float	tm, result;

    while(fgets(buf, LSIZE, stdin) != NULL) {
	sscanf(buf, "%qd.%d %qd %f %s", &date1, &msec, &date2, &tm, cmnt);
	result = calc(date1, msec, date2, tm);
	//printf("%f = %qd.%d -  %qd + %f\n", result, date1, msec, date2, tm);
	printf("%s %f\n", cmnt, result);
    }
    return 0;
}
