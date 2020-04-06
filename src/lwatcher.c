#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include "misclib.h"
#include "regexplib.h"
#include "translib.h"
#include "inotifylib.h"
#include "jitclient.h"
#include "jitcclient.h"

#define DBG   if (nflag & TRANS_DEBUG)
#define VMODE if (nflag & TRANS_VERBOSE)

#define MAX_HISTORY	(2*10)
//#define MAX_HISTORY	(2*60*2)	/* 2 hour in case of 30sec internal */
#define SSIZE	1024
#define DEFAULT_CONFNAME "/opt/nowcast/etc/conf"

int		dryflag;
int		sdirflag;
static char	lognmbase[PATH_MAX] = "./LWATCHLOG";
static char	confname[PATH_MAX];
static char	indir[PATH_MAX];
static char	flags[1024];
static int	nflag = 0;
static char	hostname[1024];
static int	pid;

static void
showsettings(struct sockaddr_in saddr)
{
    fprintf(stderr, "*********************************************\n");
    fprintf(stderr, "    Daemon PID          : %d\n", pid);
    fprintf(stderr, "    Listening IP address: %s\n",
	    inet_ntoa(saddr.sin_addr));
    fprintf(stderr, "    Listening Port      : %d\n", ntohs(saddr.sin_port));
    fprintf(stderr, "    Watching  directory : %s\n", indir);
    fprintf(stderr, "    History size        : %d\n", histsize());
    fprintf(stderr, "    Log file            : %s\n", lognmbase);
    fprintf(stderr, "    Flags               : %s (%x)\n", flags, nflag);
    fprintf(stderr, "*********************************************\n");
    fflush(stderr);
}

static void
dirnmcpy(char *dst, char *src)
{
    strcpy(dst, src);
    if (dst[strlen(dst) - 1] != '/') strcat(dst, "/");
}


int
init_transfer(char *host, int port, struct sockaddr_in *saddrp)
{
    int		sock, cc, on;

    if (!strcmp(host, "any")) {
	saddrp->sin_addr.s_addr = htonl(INADDR_ANY);
	saddrp->sin_family = AF_INET;
	saddrp->sin_port = htons(port);
    } else {
	if ((cc = setupinet(saddrp, host, port)) < 0) {
	    fprintf(stderr, "No IP address of %s for AF_INET\n", host);
	    exit(-1);
	}
    }
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
	perror("socket"); exit(1);
    }
    on = 1;
    cc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
    if (cc < 0) {
	perror("setsockopt(SO_REUSEADDR"); exit(1);
    }
    if ((cc = bind(sock, (struct sockaddr*)saddrp, sizeof(*saddrp))) < 0) {
	perror("bind");	exit(1);
    }
    if ((cc = listen(sock, 10)) < 0) {
	perror("listen"); exit(1);
    }
    return sock;
}

int
watcher(char *fname, void **args)
{
    static char	date[128], type[128];
    int		tp;
    long long	dt;

    /* adding history */
    VMODE {
	fprintf(stderr, "%s arrives\n", fname); fflush(stderr);
    }
    if (regex_match(fname, date, type, NULL, NULL) < 0) {
	fprintf(stderr, "Cannot parse the file name. skipping\n");
	return 0;
    }
    dt = atoll(date);
    tp = sync_entry(type);
    if (tp < 0) {
	unlink(fname);
    } else {
	histput(fname, dt, tp);
    }
    return 0;
}


void *
transfer(void *param)
{
    void		**args = (void**) param;
    struct sockaddr_in	*saddrp;
    int			acsock;
    int			nsize;

    nsize = sync_nsize();
    saddrp = (struct sockaddr_in *) args[0];
    acsock = (long long) args[1];
    showsettings(*saddrp);
    /*
     * Now starting
     */
    for (;;) {
	socklen_t	addrlen;
	int		cmd, opt[TRANSOPT_SIZE], retval[TRANSOPT_SIZE];
	int		totsz, sz, i;
	char		tmpstr[SCMD_STRSIZ];
	histdata	*hp;
	char		*fname = 0, *cp;
	int		sock;
	int		tfd = -1;

	addrlen = sizeof(*saddrp);
	DBG {
	    fprintf(stderr, "Waiting for request\n"); fflush(stderr);
	}
	if ((sock = accept(acsock, (struct sockaddr*)saddrp, &addrlen)) < 0) {
	    perror("accept"); exit(1);
	}
	{ /* 2020/04/06: not sure if this is effective  */
	    int cc, one = 1;
	    cc = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
			    (char *)&one, sizeof(one));
	    if (cc < 0) {
		fprintf(stderr, "cannot setsockopt(TCP_NODELAY): %s.\n",
			strerror(errno));
		exit(2);
	    }
	}
	for (;;) {
	    DBG {
		fprintf(stderr, "Waiting for command\n"); fflush(stderr);
	    }
	    cmd = trans_getcmd(sock, opt);
	    DBG {
		printf("cmd = %d opt1(%d) opt2(%d) opt3(%d)\n",
		       cmd, opt[0], opt[1], opt[2]);
	    }
	    switch (cmd) {
	    case CMD_OPEN: /* opt1:type */
	    retry_open:
		hp = histget();
		fname = hp->fname[opt[0]];
		if (fname == NULL) { /* Not available */
		    histwait();
		    goto retry_open;
		}
		tfd = open(fname, O_RDONLY);
		trans_replyopen(sock, fname, 0);
		VMODE {
		    fprintf(stderr, "open %s\n", fname); fflush(stderr);
		}
		break;
	    case CMD_READ: /* opt1:size */
		if (tfd > 0) {
		    cp = malloc(opt[0]);
		    sz = read(tfd, cp, opt[0]);
		    trans_replyread(sock, cp, sz);
		    free(cp);
		} else {
		    trans_replyread(sock, 0, 0);
		}
		break;
	    case CMD_GET:
	    retry_get:	/* debuged by Otsuka-san 2019/11/28 */
		hp = histget();
		/* checking if all types arrive */
		for (i = 0; i < nsize; i++) {
		    DBG {
			fprintf(stderr,
				"hp->fname[%d] = %s\n", i, hp->fname[i]);
		    }
		    if (hp->fname[i] == NULL) {
			histwait();
			goto retry_get;
		    }
		}
		VMODE {
		    fprintf(stderr, "%lld is sent to the client\n",
			    hp->date); fflush(stderr);
		}
		for (totsz = 0, i = 0; i < nsize; i++) {
		    if (opt[i] == 0) {
			/* request is smaller number of files */
			nsize = i;
			break;
		    }
		    totsz += opt[i];
		}
		if ((cp = (char*) malloc(totsz)) == NULL) {
		    fprintf(stderr, "Cannot allocate memory %d\n", totsz);
		    exit(-1);
		}
		tmpstr[0] = 0;
		for (totsz = 0, i = 0; i < nsize; i++) {
		    if (hp->fname[i] == 0) {
			/* the number handling files is smaller than
			 * request files */
			break;
		    }
		    if ((tfd = open(hp->fname[i], O_RDONLY)) > 0) {
			strcat(tmpstr, hp->fname[i]);
			strcat(tmpstr, FTSTR_SEPARATOR);
			sz = read(tfd, &cp[totsz], opt[i]);
			if (sz < 0) {
			    fprintf(stderr, "Cannot read file: %s\n",
				    hp->fname[i]);
			}
			retval[i] = sz;
			totsz += sz;
			close(tfd);
		    }
		}
		/* removing separation */
		tmpstr[strlen(tmpstr) - 1] = 0;
		if (trans_replyget(sock, hp->date, tmpstr, cp, totsz, retval) >= 0) {
		    /* successfuly transfered */
		    histremove();
		} else { /* client was dead, keeping the entry */
		    fprintf(stderr, "client died or network trouble.\n");
		}
		close(sock);
		free(cp);
		goto next;
	    case CMD_CLOSE:
		VMODE {
		    fprintf(stderr, "close %s\n", fname); fflush(stderr);
		}
		histremove();
		if (tfd >= 0) close(tfd);
		tfd = -1;
		goto next;
		break;
	    case CMD_EXIT:
		goto next;
	    default:
		fprintf(stderr, "The command %d is not yet implemtend\n", cmd);
		fflush(stderr);
		break;
	    }
	}
    next:;
    }
    fprintf(stderr, "exiting\n"); fflush(stderr);
    return NULL;
}

int
main(int argc, char **argv)
{
    int			dmflag = 0;
    int			opt;
    int			portnum = PORT_DEFAULT;
    int			sock;
    struct sockaddr_in	saddr;
    int			nhist;
    void		*t_args[3];
    void		*w_args[1];
    pthread_t		thread;

    if (argc < 2) {
	fprintf(stderr, "usage: %s "
		"[-c <conf directory>] [-h <history count>] "
		"[-f <log file>]\n"
		"\t\t[-p <port number>] [-H <host name>] [-D] [-d] [-v]\n"
		"\t\t<watching directory>\n", argv[0]);
	return -1;
    }
    nhist = MAX_HISTORY;
    strcpy(confname, DEFAULT_CONFNAME);
    //strcpy(hostname, HOST_DEFAULT);
    strcpy(hostname, "any");
    if (argc > 2) {
	while ((opt = getopt(argc, argv, "c:dDvh:H:p:f:")) != -1) {
	    switch (opt) {
	    case 'c': /* conf */
		strcpy(confname, optarg);
		break;
	    case 'd': /* debug mode */
		nflag |= MYNOTIFY_DEBUG;
		strcat(flags, " -d");
		break;
	    case 'D': /* daemonize */
		strcat(flags, " -D");
		dmflag = 1;
		break;
	    case 'f':
		strcpy(lognmbase, optarg);
		break;
	    case 'v': /* verbose mode */
		nflag |= MYNOTIFY_VERBOSE;
		strcat(flags, " -v");
		break;
	    case 'p':
		portnum = atoi(optarg);
		break;
	    case 'h':
		nhist = atoi(optarg);
		break;
	    case 'H':
		strncpy(hostname, optarg, 1023);
		break;
	    }
	}
    }
    sock = init_transfer(hostname, portnum, &saddr);
    if (dmflag == 1) {
	pid = mydaemonize(lognmbase);
    }
    regex_init(confname); histinit(nhist);
    dirnmcpy(indir, argv[optind]);
    t_args[0] = (void*) &saddr;
    t_args[1] = (void*) (long long) sock;
    pthread_create(&thread, NULL, transfer, (void*) t_args);
    w_args[0] = indir;
    mynotify(indir, 0, watcher, w_args, nflag);
    return 0;
}
