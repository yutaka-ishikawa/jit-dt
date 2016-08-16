/*
 *	Just In Time Data Transfer
 *	16/08/2016 notification mechanism is genaralized
 *	15/08/2016 Adding locked move function, Yutaka Ishikawa
 *	13/08/2016 Adding sftp protocol
 *			by Yutaka Ishikawa, RIKEN AICS 
 *	15/12/2015 Written by Yutaka Ishikawa, RIKEN AICS
 *			yutaka.ishikawa@riken.jp
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include "translib.h"
#include "inotifylib.h"

#define VMODE if (tflag&TRANS_VERBOSE)

#define PATH_WATCH	"./"
static int	nflag, tflag;
static int	keep_proc;

static char	topdir[PATH_MAX];
static char	startdir[PATH_MAX];
static char	lntfyfile[PATH_MAX];
static char	rntfyfile[PATH_MAX];
static char	combuf[PATH_MAX + 128];

static void
usage(char **argv)
{
    fprintf(stderr,
	    "USAGE: %s <url>\n"
	    "       <watching directory path>"
	    "[-s <start directory path>] [-k]\n"
	    "       [-n <local file>:<remote notification file>] [-d] [-v]\n",
	    argv[0]);
    fprintf(stderr, "e.g.:\n");
    fprintf(stderr, "       %s scp:kncc-login1.kncc.cc.u-tokyo.ac.jp \\\n"
	    "            /home/yisikawa/work/JIT-DT/tmp/\n", argv[0]);
    fprintf(stderr, "       %s scp:kncc-login1.kncc.cc.u-tokyo.ac.jp \\\n"
	    "            /home/yisikawa/work/JIT-DT/tmp/\\\n"
	    "            -s 00/10/20/\n",
	    argv[0]);
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

void
transfer(char *fname, void **args)
{
    double	(*transfunc)(char*, char*, char*, void**);
    char	*host_name, *remote_path;
    void	*opt[3];
    double	sec;

    transfunc = (double (*)(char*, char*, char*, void**)) args[0];
    host_name = (char*) args[1];
    remote_path = (char*) args[2];
    opt[0] = args[3]; /* keep process */
    opt[1] =  args[4]; /* local notify file */
    opt[2] =  args[5]; /* remote notify file */
    printf("keep(%d) remote_path(%s) local ntfy(%s) remote ntfy(%s)\n",
	   (int) (long long) opt[0], remote_path, (char*) opt[1], (char*) opt[2]);
    sec = transfunc(host_name, remote_path, fname, opt);
    VMODE {
	fprintf(stderr, "%s, %f\n", fname, sec); fflush(stderr);
    }
}

void
showoptions(char *top, char *start, char *host, char *remote,
	    char *lntf, char *rntf)
{
    VMODE {
	fprintf(stderr, "*************************** Conditions ***************************\n");
	fprintf(stderr, "  Keeping sftp process        : %s\n",
		keep_proc > 0 ? "yes" : "no");
	fprintf(stderr, "  Top directory               : %s\n", top);
	fprintf(stderr, "  Starting directory          : %s\n",
		*start == 0 ? top : start);
	if (host) {
	    fprintf(stderr, "  Host name                   : %s\n", host);
	}
	fprintf(stderr, "  Remote file path            : %s\n", remote);
	fprintf(stderr, "  Local file for notifiaction : %s\n", lntf);
	fprintf(stderr, "  Remote file for notifiaction: %s\n", rntf);
	fprintf(stderr, "******************************************************************\n");
    }
}

int
main(int argc, char **argv)
{
    int		opt;
    char	*url;
    int		ttype;
    char	*host_name, *remote_path;
    void	*args[6];

    if (argc < 3 || argc > 8) {
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
	while ((opt = getopt(argc, argv, "dkvs:n:")) != -1) {
	    switch (opt) {
	    case 'd': /* debug mode */
		nflag |= MYNOTIFY_DEBUG; tflag |= TRANS_DEBUG;
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
		    *idx = 0; /* terminate for local file name */
		    strcpy(rntfyfile, idx + 1); /* remote file name */
		    sprintf(combuf, "echo %d > %s\n", getpid(), lntfyfile);
		    cc = system(combuf);
		    VMODE {
			fprintf(stderr, "creating local notify file %s: cc=%d\n",
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
    host_name = NULL; remote_path = NULL;
    ttype = trans_type(url, &host_name, &remote_path);
    if (ttype < 0) {
	(ttype == TRANS_UNKNOWN) ?
	    fprintf(stderr, "Unknown transfer method: %s\n", url)
	    : fprintf(stderr, "No transfer method is specified");
	exit(-1);
    }
    showoptions(topdir, startdir, host_name, remote_path, lntfyfile, rntfyfile);
    trans_setflag(tflag);
    signal(SIGINT, terminate);
    args[0] = ttable[ttype]; args[1] = host_name; args[2] = remote_path;
    args[3] = (void*) (long long) keep_proc;
    args[4] = (void*) lntfyfile;
    args[5] = (void*) rntfyfile;
    mynotify(topdir, startdir, transfer, args, nflag);
    terminate(0);
    return 0;
}
