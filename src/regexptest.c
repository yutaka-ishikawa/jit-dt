#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>

char	errbuf[1024];
char	*tpattern[] = {
    "/home/ishikawa/tmp/tmp/kobe_20161208110200_A08_pawr_vr.dat",
    "kobe_20161209171230_A08_pawr_ze.dat",
    "asdasasdasd"
};

int
main(int argc, char **argv)
{
//    char	*regex = "kobe_\\(.*\\)_A08_pawr_\\(.*\\).dat";
    char	*regex = ".*_\\(.*\\)_A08_pawr_\\(.*\\).dat";
    int		cc, i, j;
    unsigned long long	ll;
    regex_t	preg;
    size_t	sz;
    regmatch_t	pmatch[4];

    if ((cc = regcomp(&preg, regex, 0)) < 0) {
	printf("compile error\n");
	sz = regerror(cc, &preg, errbuf, 1024);
	printf("errbuf=%s\n", errbuf);
    }
    for (j = 0; j < 3; j++) {
	memset(pmatch, 0, sizeof(pmatch));
	if ((cc = regexec(&preg, tpattern[j], 4, pmatch, 0)) < 0) {
	    printf("error \n");
	    sz = regerror(cc, &preg, errbuf, 1024);
	    printf("errbuf=%s\n", errbuf);
	} else {
	    char buf[128];
	    if (cc == REG_NOMATCH) {
		printf("no match: %s\n", tpattern[j]);
		continue;
	    }
	    printf("macth: %s\n", tpattern[j]);
	    for (i = 0; i < 4; i++) {
		memset(buf, 0, 128);
		strncpy(buf, &tpattern[j][pmatch[i].rm_so],
			pmatch[i].rm_eo - pmatch[i].rm_so);
		printf("\t[%d] %s(%d,%d)\n", i, buf, pmatch[i].rm_so, pmatch[i].rm_eo);
	    }
	    strncpy(buf, &tpattern[j][pmatch[1].rm_so],
			pmatch[1].rm_eo - pmatch[1].rm_so);
	    ll = atoll(buf);
	    printf("date = %lld (%s)\n", ll, buf);
	}
    }
    regfree(&preg);
    return 0;
}
