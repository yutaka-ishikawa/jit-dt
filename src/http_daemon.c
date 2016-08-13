#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <microhttpd.h>

//#define DEBUG
static int	dflag;
#ifdef DEBUG
#define DBG	if (dflag)
#else
#define DBG	if (0)
#endif


//#define MHD_FLAGS	(MHD_USE_DEBUG)
//#define MHD_FLAGS	(MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG | MHD_USE_SSL)
#define MHD_FLAGS	(MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG)
#define DEFAULT_PORT	8080
#define CONTIMEOUT	20
#define ROOTPATH	"./"

static char *message = "<html><head><title>Data Transfer Deamon</title></head><body><H1>Hello !!</H1></body></html>";
static char *errmsg = "<html><body>Error</body></html>";

struct context {
//    int				fd;
    FILE			*fp;
    const char			*key;
    const char			*fname;
    struct MHD_PostProcessor	*pp;
};

char *
strappend(char *c1, char *c2, int rflag)
{
    char	*dp;

    dp = malloc(strlen(c1) + strlen(c2) + 1);
    strcpy(dp, c1); strcat(dp, c2);
    if (rflag == 1) {
	free(c1);
    }
    return dp;
}

int
browsefile(char *url, struct MHD_Connection *con)
{
    int			ret = 0;
    int			ntry = 0;
    int			fd;
    char		*path;
    struct stat		sbuf;
    struct MHD_Response *res;

    path = strappend(ROOTPATH, url, 0);
retry:
    fd = open(path, O_RDONLY);
    if (fd < 0) {
	fprintf(stderr, "Cannot find file:%s\n", url);
	goto err;
    }
    ret = fstat(fd, &sbuf);
    if (ret < 0) {
	fprintf(stderr, "Cannot stat of file:%s\n", url);
	goto err;
    }
    if (S_ISDIR(sbuf.st_mode)) {
	/* directory */
	if (ntry > 0) {	/* index.html is directory */
	    goto err;
	}
	close(fd);
	path = strappend(path, "./index.html", 1);
	ntry++;
	goto retry;
    } else if (!S_ISREG(sbuf.st_mode)) {
	DBG {
	    fprintf(stderr, "file type = %d\n", sbuf.st_mode);
	}
	goto err;
    }
    res = MHD_create_response_from_fd(sbuf.st_size, fd);
    ret = MHD_queue_response(con, MHD_HTTP_OK, res);
    MHD_destroy_response (res);
    free(path);
    return ret;
err:
    res = MHD_create_response_from_buffer(strlen(errmsg), errmsg,
					  MHD_RESPMEM_PERSISTENT);
    MHD_queue_response(con, MHD_HTTP_NOT_FOUND, res); /* 404 */
    MHD_destroy_response (res);
    free(path);
    return -1;
}

static char *tcodemsg[] =
{
    "OK\n", "Error\n", "Timeout\n", "Deamon shutdown\n",
    "Read error\n", "Client abort\n"
};

void
reqcomplete(void *cls, struct MHD_Connection *con, void **con_cls,
	    enum MHD_RequestTerminationCode tcode)
{
    DBG {
	fprintf(stderr, "Request completed(%d): ", tcode);
    }
    if (tcode >= 0 && tcode <= MHD_REQUEST_TERMINATED_CLIENT_ABORT) {
	puts(tcodemsg[tcode]);
    }
}

int
post_progress(void *cls, enum MHD_ValueKind kind,
	      const char *key, const char *filename,
	      const char *ctype,
	      const char *encode,
	       const char *data, uint64_t off, size_t size)
{
    struct context	*cntxt = cls;
    FILE		*fp;
    size_t		sz;

    DBG {
	fprintf(stderr, "post_progress: contxt(%p), key=%s, fname=%s, type=%s, encode=%s\n",
	       cntxt, key, filename, ctype, encode);
	fprintf(stderr, "\toff = %ld size = %ld\n", off, size);
	if (size < 10) {
	    fprintf(stderr, "\tdata = %s\n", data);
	}
    }
    if (ctype) {
	/* application/x-tar or ... */
	fp = cntxt->fp;
	if (cntxt->fname && strcmp(cntxt->fname, filename) && off == 0) {
	    /* different file is now uploading */
	    int		ret;
	    ret = fclose(fp);
	    if (ret < 0) goto err3;
	    fp = NULL;
	    
	}
	if (fp == NULL) {
	    /* open the file */
	    DBG {
		fprintf(stderr, "create file: %s\n", filename);
	    }
	    fp = fopen(filename, "w");
	    if (fp == NULL) goto err2;
	    cntxt->fp = fp;
	    cntxt->key = key;
	    cntxt->fname = filename;
	}
	while(size > 0) {
	    sz = fwrite(data, size, 1, fp);
	    if (sz < 0) goto err1;
	    size -= sz;
	}
    }
    return MHD_YES;
err1:
    fprintf(stderr, "Cannot write file: %s\n", filename);
    return MHD_NO;
err2:
    fprintf(stderr, "Cannot create/open file: %s\n", filename);
    perror("");
err3:
    fprintf(stderr, "Cannot close file: %s\n", cntxt->fname);
    return MHD_NO;
}    


int
uploadfile(void *cls, struct MHD_Connection *con, const char *url,
	   const char *upload_data, size_t *upload_data_size, void **ptr)
{
    struct context		*cntxt = *ptr;
    struct MHD_PostProcessor	*pp;
    struct MHD_Response		*res;
    int				ret;

    DBG {
	fprintf(stderr, "uploadfile: cntxt(%p)\n", cntxt);
    }
    if (cntxt == NULL) {
	cntxt = (struct context*) malloc(sizeof(struct context));
	if (cntxt == NULL) goto err1;
	pp = MHD_create_post_processor(con, 1024, &post_progress, cntxt);
	if (pp == NULL) goto err2;
	cntxt->fp = NULL; /* not yet open */
	cntxt->key = 0; cntxt->fname = 0;
	cntxt->pp = pp;
	*ptr = cntxt;
	return MHD_YES;
    err1:
	free(cntxt);
    err2:
	fprintf(stderr, "Cannot create post processor\n");
	return MHD_NO;
    }
    pp = cntxt->pp;
    DBG {
	fprintf(stderr, "\tbefore post_process *upload_data_size=%ld\n",
		*upload_data_size);
    }
    MHD_post_process (pp, upload_data, *upload_data_size);
    DBG {
	fprintf(stderr, "\tafter post_process *upload_data_size=%ld\n",
		*upload_data_size);
    }
    if (*upload_data_size) {
	*upload_data_size = 0;
	return MHD_YES;
    } else {
	MHD_destroy_post_processor(pp);
    }
    /* upload is finished */
    DBG {
	fprintf(stderr,"\tended\n");
    }
    if (cntxt->fp != NULL) {
	ret = fclose(cntxt->fp);
	if (ret != 0) {
	    fprintf(stderr, "close with error\n");
	}
    }
    res = MHD_create_response_from_buffer(strlen(message),
					  (void *) message,
					  MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(con, MHD_HTTP_OK, res);
    MHD_destroy_response (res);
    return ret;
}

int
mhd_handler(void *cls, struct MHD_Connection *con, const char *url,
	    const char *method, const char *version,
	    const char *upload_data, size_t *upload_data_size, void **ptr)
{
    int			ret = 0;
    // struct MHD_Response *res;

    DBG {
	char	buf[128];
	fprintf(stderr, "mhd_handler:\tmethod = %s\n", method);
	buf[0] = 0;
	if (upload_data) {
	    strncpy(buf, upload_data, 127);
	    buf[127] = 0;
	}
	fprintf(stderr, "\t\turl=%s, version=%s, upload_data=%s\n",
		url, version, buf);
    }
#ifdef BASIC_AUTH
    {
	const char *page = "<html><body>Go away.</body></html>";
	char *pass, *user;

	pass = NULL;
	user = MHD_basic_auth_get_username_password (con, &pass);
	if (user == NULL || strcmp (user, "root")
	    || strcmp (pass, "assimilation")) {
	    res = MHD_create_response_from_buffer(strlen (page),
						  (void *) page,
						  MHD_RESPMEM_PERSISTENT);
	    ret = MHD_queue_basic_auth_fail_response (connection,
						      "my realm", res);
	    return ret;
	}
    }
#endif
    if (!strcmp(method, MHD_HTTP_METHOD_GET)) { /* GET METHOD */
	ret = browsefile((char*) url, con);
    } else if (!strcmp(method, MHD_HTTP_METHOD_POST)) { /* POST METHOD */
	ret = uploadfile(cls, con, url, upload_data, upload_data_size, ptr);
    }
    return ret;
}

int
main(int argc, char **argv)
{
    struct sockaddr_in	saddr;
    struct MHD_Daemon	*hndl;
    int			port = DEFAULT_PORT;
    // struct timeval	tv;
    // struct timeval	*tvp;
    // fd_set		rs, ws, es;
    // MHD_socket		max;
    // MHD_UNSIGNED_LONG_LONG mhd_timeout;

    if (argc < 3) {
	fprintf(stderr, "%s <my host ip address> <port number> [-d]\n", argv[0]);
	fprintf(stderr, "e.g. %s 133.11.249.234 80\n", argv[0]);
	fprintf(stderr, "     %s 134.160.185.65 8080\n", argv[0]);
	return -1;
    }
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(argv[1]);
    port = atoi(argv[2]);
    saddr.sin_port = htons(port);
    DBG {
	dflag = 0;
    }
    if (argc == 4 && !strcmp(argv[3], "-d")) {
#ifdef DEBUG
	    dflag = 1;
#else
	    fprintf(stderr, "Cannot specofy debug option. please recompile it with -DDEBUG option\n");
	    return -1;
#endif /* DEBUG */
    }
    /* port must be network order */
    hndl = MHD_start_daemon (MHD_FLAGS,
			     htons(port),  NULL, NULL,
			     &mhd_handler, NULL,
			     MHD_OPTION_SOCK_ADDR, (struct sockaddr *) &saddr,
			     MHD_OPTION_CONNECTION_TIMEOUT, CONTIMEOUT,
			     MHD_OPTION_NOTIFY_COMPLETED, &reqcomplete, NULL,
			     MHD_OPTION_END);
    if (hndl == NULL) {
	fprintf(stderr, "Cannot start http deamon\n");
	return -1;
    }
    (void) getc(stdin);
    MHD_stop_daemon(hndl);
    return 0;
}
