/*
 *	06/01/2020 Testing inotifylib2.c
 */

#include <limits.h>
#include <sys/stat.h>
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
#include "regexplib.h"

#define DBG   if (tflag & TRANS_DEBUG)
#define VMODE if (tflag & TRANS_VERBOSE)

#define PATH_WATCH	"./"
int		dryflag;
int		sdirflag;
static int	pid;
static int	nflag, tflag;

static char	topdir[PATH_MAX];
static char	startdir[PATH_MAX];
static char	lognmbase[PATH_MAX] = "/tmp/JITDTLOG";

static void
terminate(int num)
{
    LOG_PRINT("watch_and_transfer terminates\n");
}


static int
show(char *fname, void **args)
{
    int		cc;
    struct stat	sbuf;

    if ((cc = stat(fname, &sbuf)) != 0) {
	LOG_PRINT("Cannot find file: %s\n", fname);
    }
    LOG_PRINT("CATCH: %s size(%ld)\n", fname, sbuf.st_size);
    logfupdate();
    return 1;
}

static void
showoptions(char *top, char *start)
{
    VMODE {
	LOG_PRINT("*************************** Conditions ***************************\n");
	LOG_PRINT("  Daemon PID                  : %d\n", pid);
	LOG_PRINT("  Top directory               : %s\n", top);
	LOG_PRINT("  Starting directory          : %s\n",
		*start == 0 ? top : start);
	LOG_PRINT("******************************************************************\n");
    }
}


int
main(int argc, char **argv)
{
    int		opt;
    int		dmflag = 0;

    if (argc == 1) {
	fprintf(stderr, "Specifying a top directory at least\n"); exit(-1);
    }
    strcpy(topdir, argv[1]);
    fformat(topdir);
    startdir[0] = 0;
    while ((opt = getopt(argc, argv, "Ddkp:vs:n:f:c:")) != -1) {
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
	default:
	    fprintf(stderr, "Unknown option -%c\n", opt);
	}
    }
    if (dmflag == 1) {
	pid = mydaemonize(lognmbase);
    }
    showoptions(topdir, startdir);
    signal(SIGINT, terminate);
    mynotify(topdir, startdir, show, NULL, nflag);
    terminate(0);
    return 0;
}
