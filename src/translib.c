#include "translib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

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

int
trans_type(char *url)
{
    if (!strncmp(url, "http:", 5)) {
	return TRANS_HTTP;
    } else if (!strncmp(url, "ssh:", 4)) {
	return TRANS_SCP;
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

    sprintf(cmdbuf, "scp %s %s:", fname, &url[strlen("ssh:")]);
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
    int		pid;
    int		pipefd[2];
    struct timeval	start, end;
    double		sec;

    start = getTime();
    if (first == 0) {
	first = 1;
	if (pipe(pipefd) == -1) {
	    perror("Creating pipe: failed");
	    exit(-1);
	}
	if ((pid = fork()) == 0) {
	    int		cc;
	    /* child */
	    close(pipefd[1]); //close write side from parents
            close(0); //close stdin
            dup(pipefd[0]); //connect pipe
	    cc = execl("/usr/bin/sftp", "sftp",
		       "-b", "-", "-o", "Compression=yes", NULL);
	    if (cc < 0) {
		perror("Cannot exec sftp");
		exit(cc);
	    }
	} else if (pid < 0) {
	    perror("Creating pipe: failed");
	    exit(-1);
	}
	close(pipefd[0]);
	wfp = fdopen(pipefd[1], "w");
    }
    if (wfp == NULL) {
	perror("Something wrong\n");
	exit(-1);
    }
    fprintf(wfp, "put %s\n", fname); fflush(wfp);
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
