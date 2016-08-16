/*
 *	Just In Time Data Transfer
 *	15/08/2016 Adding locked move function, Yutaka Ishikawa
 *	13/08/2016 Adding sftp protocol
 *			by Yutaka Ishikawa, RIKEN AICS, 
 *	15/12/2015 Created
 *			by Yutaka Ishikawa, RIKEN AICS,yutaka.ishikawa@riken.jp
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <libgen.h>
#include <curl/curl.h>
#include "translib.h"
#include "translocklib.h"

#define BSIZE	1024*8
#define DBG	if (flag & TRANS_DEBUG)
#define VMODE	if (flag & TRANS_VERBOSE)

double (*ttable[TRANS_TMAX])(char*, char*, char*, void**) =
{ http_put, scp_put, sftp_put, locked_move };

static int	flag;
static char	fnbuf[PATH_MAX]; /* temporary use for locked_move */
static char	combuf[BSIZE]; /* temporary use */
static int	sftpid = 0;

static
int setup_childpipe(int *to, int *from)
{
    int	cc;
    close(0); //close stdin
    cc = dup(to[0]); //connect pipe
    if (cc != 0) goto err_return;
    close(1); //close stdout
    cc = dup(from[1]); //connect pipe
    if (cc != 1) goto err_return;
    return 0;
err_return:
    return -1;
}

static int
put_cmd(FILE *wfp, int rfd, const char *fmt, char *a1, char *a2)
{
    int		cc, sz;

    fprintf(wfp, fmt, a1, a2); fflush(wfp);
    sz = read(rfd, combuf, BSIZE);
    if (sz <= 0) {
	return -1;
    }
    DBG {
	cc = write(1, "return= ", strlen("return= "));
	if (cc != strlen("return= ")) {
	    perror("Something wrong\n");
	    exit(-1);
	}
	if (sz != write(1, combuf, sz)) {
	    perror("write:"); exit(-1);
	}
    }
    return sz;
}

static struct timeval
getTime()
{
    struct timeval	tv;
    struct timezone	tz;

    gettimeofday(&tv, &tz);
    return tv;
}

static double
duration(struct timeval start, struct timeval end)
{
    double	sec;

    sec = (end.tv_sec - start.tv_sec);
    sec += ((double)(end.tv_usec - start.tv_usec))/1000000;
    return sec;
}

/*
 *	scp:ishikawa@aics.riken.jp        | scp:ishikawa@aics.riken.jp:
 *	scp:ishikawa@aics.riken.jp:data   | scp:ishikawa@aics.riken.jp:tmp/
 *	sftp:ishikawa@aics.riken.jp:data  | sftp:ishikawa@aics.riken.jp:tmp/
 *	lock:/usr/local/var/
 */
static int
parse_url(char *url, char **host, char **rpath, char *proto)
{
    int		sz = strlen(proto);
    char	*idx;
    if (strncmp(url, proto, sz) != 0) return 0;
    if (host) *host = &url[sz];
    if ((idx = index(*host, ':'))) {
	*idx = 0;
	if (rpath) *rpath = idx + 1;
    }
    return 1;
}

void
trans_setflag(int flg)
{
    flag = flg;
}

int
trans_type(char *url, char **host, char **rpath)
{
    if (parse_url(url, host, rpath, "http:")) {
	return TRANS_HTTP;
    } else if (parse_url(url, host, rpath, "scp:")) {
	return TRANS_SCP;
    } else if (parse_url(url, host, rpath, "ssh:")) {
	return TRANS_SCP;
    } else if (parse_url(url, host, rpath, "sftp:")) {
	return TRANS_SFTP;
    } else if (parse_url(url, rpath, 0, "lock:")) {
	return TRANS_LOCK;
    } else if (index(url, ':')) {
	return TRANS_UNKNOWN;
    }
    return TRANS_NONE;
}

double
scp_put(char *host, char *rpath, char *fname, void **opt)
{
    struct timeval	start, end;
    int			cc;
    double		sec;

    if (rpath) {
	sprintf(combuf, "scp %s %s:%s", fname, host, rpath);
    } else {
	sprintf(combuf, "scp %s %s:", fname, host);
    }
    DBG {
	fprintf(stderr, "cmd=%s\n", combuf);
    }
    start = getTime();
    cc = system(combuf);
    if (cc != 0) {
	if (cc == -1) {
	    fprintf(stderr, "Cannot invoke command: %s\n", combuf);
	} else {
	    fprintf(stderr, "Return code %d of command: %s\n", cc, combuf);
	}
	return -1;
    }
    end = getTime();
    sec = duration(start, end);
    return sec;
}

void
sftp_terminate()
{
    if (sftpid) kill(sftpid, SIGKILL);
}

double
sftp_put(char *host, char *rpath, char *fname, void **opt)
{
    static int	first = 0;
    static int	isprocalive = 0;
    static FILE	*wfp = NULL;
    static int	rfd;
    int		keepproc;
    char	*lntfy, *rntfy;
    int		to_sftp[2], from_sftp[2];
    int		cc;
    struct timeval	start, end;
    double		sec;

    start = getTime();
    keepproc = (long long) opt[0];
    lntfy = (char*) opt[1]; rntfy = (char*) opt[2];
    DBG {
	fprintf(stderr, "sftp_put: keepproc = %d\n", keepproc);
    }
    if (first == 0) {
	first = 1; atexit(sftp_terminate);
    }
    if (isprocalive == 0) {
    	if (pipe(to_sftp) == -1 || pipe(from_sftp) == -1) {
	    perror("Creating pipe: failed");
	    exit(-1);
	}
	if ((sftpid = fork()) == 0) {
	    /* child */
	    if (setup_childpipe(to_sftp, from_sftp) < 0) {
		fprintf(stderr, "Cannot set up file descriptor\n");
		exit(-1);
	    }
	    cc = execl("/usr/bin/sftp", "sftp",
		       "-b", "-",
		       "-o", "Compression=yes", 
		       host, NULL);
	    if (cc < 0) {
		perror("Cannot exec sftp");
		exit(cc);
	    }
	} else if (sftpid < 0) {
	    perror("Creating pipe: failed");
	    exit(-1);
	}
	DBG {
	    fprintf(stderr, "sftpid = %d\n", sftpid);
	}
	rfd = from_sftp[0]; close(from_sftp[1]);
	wfp = fdopen(to_sftp[1], "w");	close(to_sftp[0]);
	DBG {
	    fprintf(stderr, "rfd(%d) wfd(%d)\n", from_sftp[0], to_sftp[1]);
	}
	isprocalive = 1;
    }
    if (wfp == NULL) {
	perror("Something wrong\n");
	exit(-1);
    }
    if (rpath) {
	char	*idx, *base;
	strcpy(combuf, fname);
	base = basename(combuf);
	strcpy(fnbuf, rpath);
	if ((idx = rindex(rpath, '/')) && *(idx + 1) == 0) {
	    /* last '/' means a directory */
	    strcat(fnbuf, base);
	} /* including a file name */
	DBG { fprintf(stderr, "put %s %s\n", fname, fnbuf); }
	cc = put_cmd(wfp, rfd, "put %s %s\n", fname, fnbuf);
    } else {
	DBG { fprintf(stderr, "put %s\n", fname); }
	cc = put_cmd(wfp, rfd, "put %s\n", fname, NULL);
	strcpy(fnbuf, "~/"); strcat(fnbuf, fname);
    }
    if (cc < 0) {
	fprintf(stderr, "sftp dies\n");
	exit(-1);
    }
    /* notification */
    if (lntfy) {
	sprintf(combuf, "echo %s > %s\n", fnbuf, lntfy);
	cc = system(combuf);
	if (cc < 0) {
	    fprintf(stderr, "Cannot exec: %s\n", combuf);
	    exit(-1);
	}
	DBG { fprintf(stderr, "put the notification file: from %s to %s\n",
		      lntfy, rntfy); }
	cc = put_cmd(wfp, rfd, "put %s %s\n", lntfy, rntfy);
    }
    if (keepproc) {
	put_cmd(wfp, rfd, "pwd\n", NULL, NULL);
    } else {
	int	stat;
	put_cmd(wfp, rfd, "quit\n", NULL, NULL);
	close(rfd); fclose(wfp);
	waitpid(sftpid, &stat, 0);
	isprocalive = 0;
    }
    end = getTime();
    sec = duration(start, end);
    return sec;
}

double
locked_move(char *host, char *rpath, char *fname, void **opt)
{
    int		rfd, wfd;
    int		lckfd;
    char	*idx, *base;
    ssize_t	szr, szw = 0;

    /* lock */
    printf("host(%s) rpath(%s) fname(%s)\n", host, rpath, fname);
    lckfd = locked_lock(rpath);
    if ((rfd = open(fname, O_RDONLY)) < 0) {
	fprintf(stderr, "Cannot open %s\n", fname);
	exit(-1);
    }
    strcpy(combuf, fname);
    base = basename(combuf);
    strcpy(fnbuf, rpath);
    if ((idx = rindex(fnbuf, '/')) && *(idx + 1) == 0) {
	/* last '/' means a directory */
	strcat(fnbuf, base);
    } /* otherwise file name is specified */
    if ((wfd = open(fnbuf, O_CREAT|O_TRUNC|O_RDWR, 0666)) < 0) {
	fprintf(stderr, "Cannot create %s\n", fnbuf);
	exit(-1);
    }
    /* copy */
    while ((szr = read(rfd, combuf, BSIZE)) > 0) {
	if ((szw = write(wfd, combuf, szr)) != szr) break;
    }
    if (szr < 0 || (szr != 0 && szw != szr)) {
	fprintf(stderr, "Error on copying from %s to %s\n", fname, fnbuf);
	(szr < 0) ? perror("read") : perror("write");
	exit(-1);
    }
    close(rfd); close(wfd);
    /* remove */
    unlink(fname);
    /* write and unlock */
    locked_write(lckfd, fnbuf);
    locked_unlock(lckfd);
    return 0;
}


static size_t
replyhandler(void *ptr, size_t size, size_t nmemb, void *fp)
{
    DBG {
	fprintf(stderr, "replyhandler: size = %ld nmemb = %ld\n", size, nmemb);
    }
    return size*nmemb;
}

double
http_put(char *host, char *rpath, char *fname, void **opt)
{
    char		*url = combuf;
    double		sec;
    CURL		*curl;
    CURLcode		res;
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    struct timeval	start, end;

    curl_global_init(CURL_GLOBAL_ALL);

    start = getTime();
    /* Fill in the file upload field */
    curl_formadd(&formpost,
		 &lastptr,
		 CURLFORM_COPYNAME, "sendfile",
		 CURLFORM_FILE, fname,
		 CURLFORM_END);
    /* Fill in the filename field */
    curl_formadd(&formpost,
		 &lastptr,
		 CURLFORM_COPYNAME, "filename",
		 CURLFORM_COPYCONTENTS, fname,
		 CURLFORM_END);
    curl = curl_easy_init();
    if (curl == NULL) goto err1;
    strcpy(url, host);
    if (rpath[0] != '/') strcat(url, "/");
    strcat(url, rpath);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, replyhandler);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    end = getTime();

    if(res != CURLE_OK) goto err2;
    curl_easy_cleanup(curl);
    curl_formfree(formpost);

    sec = duration(start, end);
    return sec;
err1:
    fprintf(stderr, "Cannot initialize.\n");
    return -1;
err2:
    fprintf(stderr, "Cannot send http request: %s\n", curl_easy_strerror(res));
    return -1;
}
