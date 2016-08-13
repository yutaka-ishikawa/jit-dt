#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

//#define DEBUG
static int	dflag;
#ifdef DEBUG
#define DBG	if (dflag)
#else
#define DBG	if (0)
#endif

struct timeval
getTime()
{
    struct timeval	tv;
    struct timezone	tz;

    gettimeofday(&tv, &tz);
    return tv;
}

void
printResult(struct timeval start, struct timeval end, char *fmt)
{
    double	sec;

    sec = (end.tv_sec - start.tv_sec);
    sec += ((double)(end.tv_usec - start.tv_usec))/1000000;
    printf(fmt, sec);
}

static size_t
replyhandler(void *ptr, size_t size, size_t nmemb, void *fp)
{
    DBG {
	fprintf(stderr, "replyhandler: size = %ld nmemb = %ld\n", size, nmemb);
    }
    return size*nmemb;
}

int
main(int argc, char **argv)
{
    char		*url, *fname;
    CURL		*curl;
    CURLcode		res;
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    struct timeval	start, end;

    if (argc < 3) {
	fprintf(stderr, "%s <url> <filename> [-d]\n", argv[0]);
	fprintf(stderr, "\t displayed time is in second\n");
	return -1;
    }
    url = argv[1];
    fname = argv[2];
    DBG {
	dflag = 0;
    }
    if (argc == 4 && !strcmp(argv[3], "-d")) {
	DBG {
	    dflag = 1;
	} else {
	    fprintf(stderr, "Cannot specify debug option. please recompile it with -DDEBUG option\n");
	    return -1;
	}
    }


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

    printResult(start, end, "%f\n");
    return 0;
err1:
    fprintf(stderr, "Cannot initialize.\n");
    return -1;
err2:
    fprintf(stderr, "Cannot send http request: %s\n", curl_easy_strerror(res));
    return -1;
}
