#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include "misclib.h"
#include "translib.h"
#include "inotifylib.h"
#include "jitclient.h"
#include "jitcclient.h"

#define DBG   if (nflag & TRANS_DEBUG)
#define VMODE if (nflag & TRANS_VERBOSE)

#define MAX_HISTORY	(2*10)
//#define MAX_HISTORY	(2*60*2)	/* 2 hour in case of 30sec internal */
#define SSIZE	1024

static char	lognmbase[PATH_MAX] = "./LWATCHLOG";
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
    fprintf(stderr, "    Listening Port:       %d\n", ntohs(saddr.sin_port));
    fprintf(stderr, "    Watching  directory:  %s\n", indir);
    fprintf(stderr, "    History size:         %d\n", histsize());
    fprintf(stderr, "    Flags:                %s (%x)\n", flags, nflag);
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
    if ((cc = setupinet(saddrp, host, port)) < 0) {
	fprintf(stderr, "No IP address of %s for AF_INET\n", host);
	exit(-1);
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

void
watcher(char *fname, void **args)
{
    static char	date[128], type[128];
    int		tp;
    long long	dt;

    /* adding history */
    VMODE {
	fprintf(stderr, "%s arrives\n", fname); fflush(stderr);
    }
    if (regex_match(fname, date, type) < 0) {
	fprintf(stderr, "Cannot parse the file name. skipping\n");
	return;
    }
    dt = atoll(date);
    tp = asc2ent(type);
    histput(fname, dt, tp);
}


void *
transfer(void *param)
{
    void		**args = (void**) param;
    struct sockaddr_in	*saddrp;
    int			acsock;

    saddrp = (struct sockaddr_in *) args[0];
    acsock = (long long) args[1];
    showsettings(*saddrp);
    /*
     * Now starting
     */
    for (;;) {
	socklen_t	addrlen;
	int		cmd, opt[3], retval[3], totsz, sz, i;
	histdata	*hp;
	char		*fname = 0, *cp, *tmp;
	int		sock;
	int		tfd = -1;

	addrlen = sizeof(*saddrp);
	DBG {
	    fprintf(stderr, "Waiting for request\n"); fflush(stderr);
	}
	if ((sock = accept(acsock, (struct sockaddr*)saddrp, &addrlen)) < 0) {
	    perror("accept"); exit(1);
	}
	for (;;) {
	    DBG {
		fprintf(stderr, "Waiting for command\n"); fflush(stderr);
	    }
	    cmd = trans_getcmd(sock, &opt[0], &opt[1], &opt[2]);
	    DBG {
		printf("cmd = %d opt1(%d) opt2(%d) opt3(%d)\n",
		       cmd, opt[0], opt[1], opt[2]);
	    }
	    switch (cmd) {
	    case CMD_OPEN: /* opt1:type */
		hp = histget();
	    retry_open:
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
		hp = histget();
		/* checking if all types arrive */
	    retry_get:
		for (i = 0; i < FTYPE_NUM; i++) {
		    if (hp->fname[i] == NULL) {
			histwait();
			goto retry_get;
		    }
		}
		VMODE {
		    fprintf(stderr, "%lld is sent to the client\n",
			    hp->date); fflush(stderr);
		}
		for (totsz = 0, i = 0; i < 3; i++) {
		    totsz += opt[i];
		}
		if ((cp = (char*) malloc(totsz)) == NULL) {
		    fprintf(stderr, "Cannot allocate memory %d\n", totsz);
		    exit(-1);
		}
		for (totsz = 0, i = 0; i < FTYPE_NUM; i++) {
		    if ((tfd = open(hp->fname[i], O_RDONLY)) > 0) {
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
		tmp = basename(hp->fname[0]);
		if (trans_replyget(sock, hp->date, tmp, cp, totsz, retval) >= 0) {
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
	fprintf(stderr, "usage: %s <watching directory> "
		"[-h history count] [-D] [-d] [-v]\n", argv[0]);
	return -1;
    }
    nhist = MAX_HISTORY;
    strcpy(hostname, HOST_DEFAULT);
    if (argc > 2) {
	while ((opt = getopt(argc, argv, "dDvh:H:p:")) != -1) {
	    switch (opt) {
	    case 'd': /* debug mode */
		nflag |= MYNOTIFY_DEBUG;
		strcat(flags, " -d");
		break;
	    case 'D': /* daemonize */
		strcat(flags, " -D");
		dmflag = 1;
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
    regex_init(); histinit(nhist);
    dirnmcpy(indir, argv[optind]);
    t_args[0] = (void*) &saddr;
    t_args[1] = (void*) (long long) sock;
    pthread_create(&thread, NULL, transfer, (void*) t_args);
    w_args[0] = indir;
    mynotify(indir, 0, watcher, w_args, nflag);
    return 0;
}
