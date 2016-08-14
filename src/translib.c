#include "translib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

double (*ttable[TRANS_TMAX])(char*, char*) =
{ http_put, scp_put, sftp_put, locked_move };


#define BSIZE	1024
static char	combuf[BSIZE];

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
put_cmd(FILE *wfp, int rfd, const char *fmt, char *arg)
{
    int		cc, sz;

    fprintf(wfp, fmt, arg); fflush(wfp);
    sz = read(rfd, combuf, BSIZE);
    if (sz <= 0) {
	perror("Something wrong\n");
	exit(-1);
    }
    DBG {
	cc = write(1, "return= ", strlen("return= "));
	if (cc != strlen("return= ")) {
	    perror("Something wrong\n");
	    exit(-1);
	}
	sz = write(1, combuf, sz);
    } else {
	sz = 0;
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
 *	Just In Time Data Transfer
 *	13/08/2016 Adding sftp protocol
 *			by Yutaka Ishikawa, RIKEN AICS, 
 *	15/12/2015 Created
 *			by Yutaka Ishikawa, RIKEN AICS,yutaka.ishikawa@riken.jp
 */
int
trans_type(char *url)
{
    if (!strncmp(url, "http:", 5)) {
	return TRANS_HTTP;
    } else if (!strncmp(url, "scp:", 4) || !strncmp(url, "ssh:", 4)) {
	return TRANS_SCP;
    } else if (!strncmp(url, "sftp:", 5)) {
	return TRANS_SFTP;
    } else if (!strncmp(url, "lock:", 5)) {
	return TRANS_LOCK;
    }
    return TRANS_NONE;
}

double
scp_put(char *url, char *fname)
{
    char		cmdbuf[NAME_MAX];
    struct timeval	start, end;
    int			cc;
    double		sec;

    if (index(url, ':')) {
	sprintf(cmdbuf, "scp %s %s", fname, &url[strlen("ssh:")]);
    } else {
	sprintf(cmdbuf, "scp %s %s:", fname, &url[strlen("ssh:")]);
    }
    DBG {
	fprintf(stderr, "cmd=%s\n", cmdbuf);
    }
    start = getTime();
    cc = system(cmdbuf);
    if (cc != 0) {
	if (cc == -1) {
	    fprintf(stderr, "Cannot invoke command: %s\n", cmdbuf);
	} else {
	    fprintf(stderr, "Return code %d of command: %s\n", cc, cmdbuf);
	}
	return -1;
    }
    end = getTime();
    sec = duration(start, end);
    return sec;
}


double
sftp_put(char *url, char *fname)
{
    static int	first = 0;
    static FILE	*wfp = NULL;
    static int	rfd;
    int		pid;
    int		to_sftp[2];
    int		from_sftp[2];
    int		cc;
    struct timeval	start, end;
    double		sec;

    start = getTime();
    if (first == 0) {
	first = 1;
	if (pipe(to_sftp) == -1 || pipe(from_sftp) == -1) {
	    perror("Creating pipe: failed");
	    exit(-1);
	}
	if ((pid = fork()) == 0) {
	    /* child */
	    // close(pipefd[1]); //close write side from parents
	    if (setup_childpipe(to_sftp, from_sftp) < 0) {
		fprintf(stderr, "Cannot set up file descriptor\n");
		exit(-1);
	    }
	    cc = execl("/usr/bin/sftp", "sftp",
		       "-b", "-", "-o", "Compression=yes", 
		       &url[strlen("sftp:")], NULL);
	    if (cc < 0) {
		perror("Cannot exec sftp");
		exit(cc);
	    }
	} else if (pid < 0) {
	    perror("Creating pipe: failed");
	    exit(-1);
	}
	rfd = from_sftp[0]; close(from_sftp[1]);
	wfp = fdopen(to_sftp[1], "w");	close(to_sftp[0]);
    }
    if (wfp == NULL) {
	perror("Something wrong\n");
	exit(-1);
    }
    put_cmd(wfp, rfd, "put %s\n", fname);
    put_cmd(wfp, rfd, "pwd\n", NULL);
    end = getTime();
    sec = duration(start, end);
    return sec;
}

double
locked_move(char *url, char *fname)
{
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
http_put(char *url, char *fname)
{
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
