#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <regex.h>
#include "misclib.h"

//#define USE_LOCKF	1

#ifndef USE_LOCK
#include <sys/file.h>
#endif /* ~USE_LOCK */

static int	pid;
static long	logage;
static int	logent;
static FILE	*logfp;
static char	logname[PATH_MAX];
static char	lognmbase[PATH_MAX];

static inline void
fformat(char *path)
{
    if (path[strlen(path) - 1] != '/') strcat(path, "/");
}

int
mydaemonize(char *lognm)
{
    if((pid = fork()) < 0) {
	perror("Cannot fork");
	exit(-1);
    } else if(pid != 0) {
	/* the parent process is terminated */
        _exit(0);
    }
    /* new session is created */
    setsid();
    signal(SIGHUP, SIG_IGN); /* really needed ? */
    /* setup stdout, stderr */
    logage = 1;
    strcpy(lognmbase, lognm);
    sprintf(logname, "%s.%ld", lognm, logage);
    if ((logfp = fopen(logname, "w")) == NULL) {
	/* where we have to print ? /dev/console ? */
	fprintf(stderr, "Cannot create the logfile: %s\n", logname);
	fflush(stderr);
	exit(-1);
    }
    /* printing daemon pid */
    pid = getpid();
    printf("%d\n", pid); fflush(stdout);
    close(0); close(1); close(2);
    fclose(stdin); fclose(stdout); fclose(stderr);
    stdout = logfp; stderr = logfp;
    return pid;
}

void
logfupdate()
{
    if (logfp == 0) {
	/* Not daemonize */
	return;
    }
    logent++;
    if (logent > LOG_MAXENTRIES) {
	fclose(logfp);
	logent = 0;
	logage++;
	sprintf(logname, "%s.%ld", lognmbase, logage);
	logfp = fopen(logname, "w");
	if (logfp == NULL) {
	    /* How we report this error ? */
	    exit(-1);
	}
	stdout = stderr = logfp;
    }
}


void
mygettime(struct timeval *sp, struct timezone *tp)
{
    if (gettimeofday(sp, tp) < 0) {
	perror("gettimeofday:"); exit(-1);
    }
}

void
timeconv(struct timeval *tsec, char *fmtbuf)
{
    struct tm	tmst;

    localtime_r(&tsec->tv_sec, &tmst);
    sprintf(fmtbuf, "%4d%02d%02d%02d%02d%02d.%d",
	    tmst.tm_year + 1900, tmst.tm_mon + 1, tmst.tm_mday,
	    tmst.tm_hour, tmst.tm_min, tmst.tm_sec,
	    ((int)(long long)tsec->tv_usec)/1000);
    return;
}

#define IDX_WDAY	0
#define IDX_MONTH	1
#define IDX_DATE	2
#define IDX_TIME	3
#define IDX_YEAR	4

static char	*mname[] = {
    "Jan",  "Feb",  "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
    "Nov", "Dec"};

static int
conv_month(char *month)
{
    int		i;
    for (i = 0; i < 12; i++) {
	if (!strcmp(mname[i], month)) {
	    return i+1;
	}
    }
    /* something wrong */
    return 0;
}

void
dateconv(struct timeval	*tsec, char *dbufp, char *fmtbuf)
{
    int		i;
    char	*idx, *nidx;
    char	*dp[10];

    ctime_r(&tsec->tv_sec, dbufp);
    for (i = 0, idx = dbufp, nidx = index(dbufp, ' ');
	 nidx != NULL; idx = nidx + 1, nidx = index(nidx + 1, ' '), i++) {
	while (*idx == ' ') { /* in case of Thu Dec  8 12:35:31 2016 */
	    idx++; nidx = index(nidx + 1, ' ');
	}
	dp[i] = idx;
	*nidx = 0;
    }
    dp[i] = idx; if ((idx = index(idx, '\n'))) *idx = 0;

    sprintf(fmtbuf, "%s:%d:%s:%s:%d",
	    dp[IDX_YEAR], conv_month(dp[IDX_MONTH]), dp[IDX_DATE], dp[IDX_TIME],
	    ((int)(long long)tsec->tv_usec)/1000);
    return;
}

/*
 * history[]: keeping the history file entries
 * nhist: the size of history array
 * numhist: the count of history entries
 * prodhistp: producer pointer, index of the current entry being registered
 * conshistp: consumer pointer, index of the current enntry for reader
 * E.g.
 *	arrive after: numhist = 1, prodhistp = 0, conshistp = 0
 *      arrive after: numhist = 2, prodhistp = 1, conshistp = 0
 *	read after:   numhist = 1, prodhistp = 1, conshistp = 1
 *	read after:   numhist = 0, prodhistp = 1, conshistp = 2
 */
//#define MAX_HISTORY	(2*60*2)	/* 2 hour in case of 30sec internal */
#define MAX_HISTORY	(2*10)
#define UPDATE_POINTER(v)	(v) = ((v) + 1) % nhist
#define REWIND_POINTER(v)	(v) = ((v) == 0 ? nhist: (v)) - 1
#define KEEP_FILE_COUNT	3
static pthread_mutex_t	mutex;
static pthread_cond_t	cond, cond2;
static histdata		history[MAX_HISTORY];
static int		nhist;
static int		prodhistp, conshistp;
static int		numhist, waithist, waithist2;

/*
 * NOTE:
 * The initial value of conshistp must be 1 because the first entry is
 * not be used at the first time.
 */
void
histinit(int nh)
{
    if (nh <= (KEEP_FILE_COUNT + 1)) {
	nh = KEEP_FILE_COUNT + 2;
	fprintf(stderr, "history count is too short, set to %d\n", nh);
    }
    nhist = nh;
    prodhistp = 0; conshistp = 0; numhist = waithist = 0;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_cond_init(&cond2, NULL);
}

int
histsize()
{
    return nhist;
}

void
_histremove()
{
    int		i;
    int		rmhistp;
    histdata	*hp = &history[conshistp];

    if ((nhist - numhist) < KEEP_FILE_COUNT) {
	goto update; /* just update the pointer and size */
    }
    /*
     * removing history file is the previous "KEEP_FILE_COUNT" backward file.
     */
    rmhistp = ((conshistp + nhist) - KEEP_FILE_COUNT) % nhist;
    hp = &history[rmhistp];
    /*
     * hp->date = 0; don't initialize it because the producer still looks 
     * at this entry whether or not the same dated data must be stored.
     * Though the all date generated by the same date are available,
     * The consumer reads this entry. Thus, the producer keeps to look at
     * this entry.
     */
    fprintf(stderr, "removing history[%d]: %lld\n", rmhistp, hp->date); fflush(stderr);
    for (i = 0; i < FNAME_MAX; i++) {
	if (hp->fname[i]) {
	    unlink(hp->fname[i]);
	    free(hp->fname[i]);
	    hp->fname[i] = 0;
	}
    }
update:
    UPDATE_POINTER(conshistp);
    --numhist;
}

void
histremove()
{
    pthread_mutex_lock(&mutex);
    _histremove();
    pthread_mutex_unlock(&mutex);
}

histdata *
histget()
{
    histdata	*hp;
    pthread_mutex_lock(&mutex);
retry:
    if (numhist > 0) {
	hp = &history[conshistp];
	// updated by hitremove()
	//UPDATE_POINTER(conshistp);
	//--numhist;
    } else {
	waithist++;
	pthread_cond_wait(&cond, &mutex);
	waithist--;
	goto retry;
    }
    pthread_mutex_unlock(&mutex);
    return hp;
}

void
histwait()
{
    pthread_mutex_lock(&mutex);
    waithist2++;
    pthread_cond_wait(&cond2, &mutex);
    waithist2--;
    pthread_mutex_unlock(&mutex);
    return;
}

void
histput(char *path, long long dt, int tent)
{
    char	*cp;

    if (tent < 0 || tent > FNAME_MAX) {
	fprintf(stderr, "histput: type out of range (%d)\n", tent);
	return;
    }
    if ((cp = malloc(strlen(path) + 1)) == NULL) {
	fprintf(stderr, "histput:Cannot reserve memory\n"); fflush(stderr);
	exit(-1);
    }
    strcpy(cp, path);

    pthread_mutex_lock(&mutex);
    if (dt == history[prodhistp].date) { /* keeping the same entry and reg. it*/
	history[prodhistp].fname[tent] = cp;
    } else if (dt > history[prodhistp].date) { /* move to the next entry */
	/* In case of the first time,
	 * prodhistp = 0 and history[prodhistp].date = 0 */
	if (history[prodhistp].date != 0) {
	    UPDATE_POINTER(prodhistp);
	}
	numhist++;
	/*
	 * Keeping the previous "KEEP_FILE_COUNT" backward file.
	 * This means the numhist must be less than (nhist - KEEP_FILE_COUNT)
	 */
	if (numhist > (nhist - KEEP_FILE_COUNT)) {
	    /* forcing consume and remove */
	    fprintf(stderr, "%s: prodhistp(%d) conshistp(%d)\n", __func__, prodhistp, conshistp); fflush(stderr);
	    _histremove();
	}
	history[prodhistp].date = dt; /* register */
	history[prodhistp].fname[tent] = cp;
    } else { /* dt < history[prodhistp].date */
	/* bug fixed reported by Otsuka-san, 2019/11/28 and 2019/12/01 */
	int	i, oldprodp;
	oldprodp = prodhistp; REWIND_POINTER(oldprodp);
	fprintf(stderr,
		"%s: dt = %lld < history[prodhistp=%d].date = %lld searching\n",
		__func__, dt, prodhistp, history[prodhistp].date);
	fflush(stderr);
	/* find the entry, at most 'nhist - 1' before */
	for (i = 0; i < nhist - 1; i++) {
	    if (dt == history[oldprodp].date) {
                goto history_found;
	    }
	    REWIND_POINTER(oldprodp);
	}
	/* not found */
        fprintf(stderr, "\tentry NOT found: unlink %s\n", path);
	fflush(stderr);
        unlink(cp);
	goto notfound;
    history_found:
	history[oldprodp].fname[tent] = cp;
    }
    if (waithist) {
	pthread_cond_signal(&cond);
    }
    if (waithist2) {
	pthread_cond_signal(&cond2);
    }
notfound:
    pthread_mutex_unlock(&mutex);
    return;
}

#define LCK_FILE_1	"JITDT-READY-1"
#define LCK_FILE_2	"JITDT-READY-2"
static int	ticktack = 0;
static char	lckpath[PATH_MAX+1]; /* temporary */
//static int	lckfd;
static char	*lockfname[2] = { LCK_FILE_1, LCK_FILE_2 };
#define FD_MAX	1024
static char	*lckname[FD_MAX];

char *
locked_name(int lckfd)
{
    return lckname[lckfd];
}

int
locked_lock(char *path)
{
    int		lckfd;
    int		cc;
    char	*idx;

    strcpy(lckpath, path);
    if ((idx = rindex(lckpath, '/'))) {
	if (*(idx + 1) != 0) {
	    /* file name is included. remove it */
	    *(idx + 1) = 0;
	} /* last '/' means a directory */
    } else {
	/* otherwise file name is specified */
	strcpy(lckpath, "./");
    }
    umask(000);
    fformat(lckpath);
    strcat(lckpath, lockfname[ticktack]);
    if ((lckfd = open(lckpath, O_CREAT|O_RDWR, 0666)) < 0) {
	fprintf(stderr, "Lock file %s cannot be created\n", lckpath);
	perror("open");	fflush(stderr);
	exit(-1);
    }
#ifdef USE_LOCKF
    if ((cc = lockf(lckfd, F_LOCK, 0)) < 0) {
	fprintf(stderr, "Lock file %s cannot be locked (%d)\n", lckpath, cc);
	perror("lockf"); fflush(stderr);
	exit(-1);
    }
#else
    if ((cc = flock(lckfd, LOCK_EX)) < 0) {
	fprintf(stderr, "Lock file %s cannot be locked (%d)\n", lckpath, cc);
	perror("flock");
	fflush(stderr);
	exit(-1);
    }
#endif
    lckname[lckfd] = lockfname[ticktack];
    ticktack ^= 1;
    return lckfd;
}

void
locked_unlock(int lckfd)
{
    int	cc;

#ifdef USE_LOCKF
    if ((cc = lockf(lckfd, F_ULOCK, 0)) < 0) {
	fprintf(stderr, "Cannot be unlocked (%d)\n", cc);
	perror("lockf"); fflush(stderr);
	exit(-1);
    }
#endif /* USE_LOCKF */
    if ((cc = close(lckfd)) < 0) {
	fprintf(stderr, "Somthing wrong in closing lock file (%d)\n", cc);
	perror("close"); fflush(stderr);
    }
}

void
locked_unlock_nullify(int lckfd)
{
    int		dat;
    /* NULL */
    if (lseek(lckfd, 0, SEEK_SET) != 0) {
	fprintf(stderr, "Cannot be seek\n");
	perror("lseek"); fflush(stderr);
	exit(-1);
    }
    dat = 0;
    if (write(lckfd, &dat, 4) != 4) {
	fprintf(stderr, "Cannot write\n");
	perror("write"); fflush(stderr);
	exit(-1);
    }
    locked_unlock(lckfd);
}


void
locked_write(int lckfd, char *info)
{
    int	cc;

    if ((cc = write(lckfd, info, strlen(info) + 1)) < 0) {
	fprintf(stderr, "Cannot write data in lock file (%d)\n", cc);
	perror("write"); fflush(stderr);
    }
}

int
locked_read(int lckfd, char *buf, int size)
{
    char	*idx;
    int		cc;

    if ((cc = read(lckfd, buf, size)) < 0) {
	fprintf(stderr, "Cannot read data in lock file\n");
	perror("read");  fflush(stderr);
    }
    if ((idx = index(buf, '\n'))) *idx = 0;
    return cc;
}

#if 0
#define NMATCH	4	/* all string, date, type, and terminate */
//static char	*regex = "kobe_\\(.*\\)_A08_pawr_\\(.*\\).dat";
static char	*regex = ".*_\\(.*\\)_A08_pawr_\\(.*\\).dat";
static regex_t	preg;
static char	errbuf[1024];

void
regex_init()
{
    int		cc;
    if ((cc = regcomp(&preg, regex, 0)) < 0) {
	fprintf(stderr, "regexinit: compile error: %s\n", regex);
	regerror(cc, &preg, errbuf, 1024);
	fprintf(stderr, "\t%s\n", errbuf);  fflush(stderr);
	exit(-1);
    }
}

int
regex_match(char *pattern, char *date, char *type)
{
    int		cc, len;
    regmatch_t	pmatch[NMATCH];

    if ((cc = regexec(&preg, pattern, NMATCH, pmatch, 0)) < 0) {
	fprintf(stderr, "regmatch: regexec error: %s\n", regex);
	regerror(cc, &preg, errbuf, 1024);
	fprintf(stderr, "\t%s\n", errbuf);  fflush(stderr);
	return -1;
    }
    if (cc == REG_NOMATCH) return -1;
    len = pmatch[1].rm_eo - pmatch[1].rm_so;
    strncpy(date, &pattern[pmatch[1].rm_so], len);
    len = pmatch[2].rm_eo - pmatch[2].rm_so;
    strncpy(type, &pattern[pmatch[2].rm_so], len);
    type[len] = 0; /* terminating for string */
    return 1;
}
#endif
