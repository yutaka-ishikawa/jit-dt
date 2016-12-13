/*
 *	Just In Time Data Transfer
 *	07/12/2016 generational log file is turn on
 *	16/08/2016 notification mechanism is genaralized
 *	15/08/2016 Adding locked move function, Yutaka Ishikawa
 *	13/08/2016 Adding sftp protocol
 *			by Yutaka Ishikawa, RIKEN AICS 
 *	15/12/2015 Written by Yutaka Ishikawa, RIKEN AICS
 *			yutaka.ishikawa@riken.jp
 */
#include <limits.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <libgen.h>
#include "translib.h"
#include "inotifylib.h"
#include "misclib.h"

#define DBG   if (tflag & TRANS_DEBUG)
#define VMODE if (tflag & TRANS_VERBOSE)

#define PATH_WATCH	"./"
static int	pid;
static int	nflag, tflag;
static int	keep_proc;

static char	topdir[PATH_MAX];
static char	startdir[PATH_MAX];
static char	lntfyfile[PATH_MAX];
static char	rntfydir[PATH_MAX];
static char	combuf[PATH_MAX + 128];
static char	lognmbase[PATH_MAX] = "/tmp/JITDTLOG";

static void
usage(char **argv)
{
    fprintf(stderr,
	    "USAGE: %s <url>\n"
	    "       <watching directory path>"
	    "[-s <start directory path>] [-k]\n"
	    "       [-n <local file>:<remote notification directory>] [-D] [-f log file name] [-d] [-v]\n",
	    argv[0]);
    fprintf(stderr, "e.g.:\n");
    fprintf(stderr, "       %s scp:kncc-login1.kncc.cc.u-tokyo.ac.jp \\\n"
	    "            /home/yisikawa/work/JIT-DT/tmp/\n", argv[0]);
    fprintf(stderr, "       %s scp:kncc-login1.kncc.cc.u-tokyo.ac.jp \\\n"
	    "            /home/yisikawa/work/JIT-DT/tmp/\\\n"
	    "            -s 00/10/20/\n",
	    argv[0]);
    fflush(stderr);
}

static void
terminate(int num)
{
    if (keep_proc) {
	fprintf(stderr, "sftp_terminate\n");
	sftp_terminate();
    }
    if (lntfyfile[0]) unlink(lntfyfile);
}



static void
transfer(char *fname, void **args)
{
    double	(*transfunc)(char*, char*, char*, void**);
    char	*host_name, *remote_path;
    void	*opt[5];

    transfunc = (double (*)(char*, char*, char*, void**)) args[0];
    host_name = (char*) args[1];
    remote_path = (char*) args[2];
    opt[0] = args[3]; /* keep process */
    opt[1] = args[4]; /* local notify file */
    opt[2] = args[5]; /* remote notify dir */
    opt[3] = args[6]; /* working area */
    opt[4] = args[7]; /* port number */
    DBG {
	LOG_PRINT("keep(%d) remote_path(%s) local ntfy(%s) remote ntfy(%s)\n",
		(int) (long long) opt[0], remote_path,
		(char*) opt[1], (char*) opt[2]);
    }
    VMODE {
	double	sec;
	struct timeval	time;
	struct timezone	tzone;
	char	timefmtbuf[128];
	mygettime(&time, &tzone);
	sec = transfunc(host_name, remote_path, fname, opt);
	timeconv(&time, timefmtbuf);
	LOG_PRINT("%s, %s, %f\n", timefmtbuf, basename(fname), sec);
    } else {
	transfunc(host_name, remote_path, fname, opt);
    }
    logfupdate();
}

static void
showoptions(char *top, char *start, char *host, char *remote,
	    char *lntf, char *rntd)
{
    VMODE {
	LOG_PRINT("*************************** Conditions ***************************\n");
	LOG_PRINT("  Daemon PID                  : %d\n", pid);
	LOG_PRINT("  Keeping sftp process        : %s\n",
		keep_proc > 0 ? "yes" : "no");
	LOG_PRINT("  Top directory               : %s\n", top);
	LOG_PRINT("  Starting directory          : %s\n",
		*start == 0 ? top : start);
	if (host) {
	    LOG_PRINT("  Host name                   : %s\n", host);
	}
	LOG_PRINT("  Remote file path            : %s\n", remote);
	LOG_PRINT("  Local file for notifiaction : %s\n", lntf);
	LOG_PRINT("  Remote file for notifiaction: %s\n", rntd);
	LOG_PRINT("******************************************************************\n");
    }
}


int
main(int argc, char **argv)
{
    int		opt;
    int		dmflag = 0;
    char	*url;
    int		ttype;
    char	*host_name, *remote_path;
    int		portnum = 0;
    void	*args[10];

    if (argc < 3) {
	usage(argv);
	return -1;
    }
    if (strlen(argv[2]) >= PATH_MAX - 1) {
	fprintf(stderr, "Too long directory path (%ld)\n", strlen(argv[2]));
	return -1;
    }
    url = argv[1];
    strcpy(topdir, (argc < 3) ? PATH_WATCH : argv[2]);
    fformat(topdir);
    if (argc > 3) {
	while ((opt = getopt(argc, argv, "Ddkp:vs:n:")) != -1) {
	    switch (opt) {
	    case 'D': /* daemonize */
		dmflag = 1;
		break;
	    case 'f':
		strcpy(lognmbase, optarg);
		break;
	    case 'd': /* debug mode */
		nflag |= MYNOTIFY_DEBUG; tflag |= TRANS_DEBUG;
		break;
	    case 'p': /* port number */
		portnum = atoi(optarg);
		break;
	    case 'v': /* verbose mode */
		nflag |= MYNOTIFY_VERBOSE; tflag |= TRANS_VERBOSE;
		break;
	    case 's': /* starting directory */
		if (strlen(optarg) >= PATH_MAX - 1) {
		    fprintf(stderr, "Too long start directory path (%ld)\n",
			    strlen(argv[2]));
		    return -1;
		}
		strcpy(startdir, optarg);
		break;
	    case 'n': /* notify */
	        {
		    int		cc;
		    char	*idx;
		    strcpy(lntfyfile, optarg);
		    if ((idx = index(lntfyfile, ':')) == 0) {
			fprintf(stderr,
				"Error ... Incorrect notify option -n\n\n");
			usage(argv);
			return -1;
		    }
		    *idx = 0; /* terminate for local directory name */
		    strcpy(rntfydir, idx + 1); /* remote directory name */
		    fformat(rntfydir);
		    sprintf(combuf, "echo %d > %s\n", getpid(), lntfyfile);
		    cc = system(combuf);
		    VMODE {
			LOG_PRINT("creating local notify file %s: cc=%d\n",
				lntfyfile, cc);
		    }
		    break;
		}
	    case 'k': /* keeping sftp process */
		keep_proc = 1;
		break;
	    }
	}
    }
    if (dmflag == 1) {
	pid = mydaemonize(lognmbase);
    }
    host_name = NULL; remote_path = NULL;
    ttype = trans_type(url, &host_name, &remote_path);
    if (ttype < 0) {
	if (ttype == TRANS_UNKNOWN) {
	    LOG_PRINT("Unknown transfer method: %s\n", url);
	} else {
	    LOG_PRINT("No transfer method is specified");
	}
	exit(-1);
    }
    showoptions(topdir, startdir, host_name, remote_path, lntfyfile, rntfydir);
    trans_setflag(tflag);
    signal(SIGINT, terminate);
    args[0] = ttable[ttype]; args[1] = host_name; args[2] = remote_path;
    args[3] = (void*) (long long) keep_proc;
    args[4] = (void*) lntfyfile;
    args[5] = (void*) rntfydir;
    if ((args[6] = (void*) malloc(sizeof(int)*4)) == 0) {
	LOG_PRINT("cannot allocate memory\n"); exit(-1);
    }
    memset(args[6], 0, sizeof(int)*4);
    args[7] = (void*) (long long) portnum;
    mynotify(topdir, startdir, transfer, args, nflag);
    terminate(0);
    return 0;
}
