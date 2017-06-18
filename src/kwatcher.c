#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "misclib.h"
#include "regexplib.h"
#include "translib.h"
#include "inotifylib.h"
#include "jitclient.h"

//#define MAX_HISTORY	(2*60*2)	/* 2 hour in case of 30sec internal */
//#define MAX_HISTORY	(2*10)
#define MAX_HISTORY	(5)
#define DEFAULT_CONFNAME "/opt/nowcast/etc/conf"

#define DBG if (nflag&MYNOTIFY_DEBUG)
#define VMODE if (nflag & TRANS_VERBOSE)

static char	lognmbase[PATH_MAX] = "/tmp/KWATCHLOG";
static char	confname[PATH_MAX];
static char	indir[PATH_MAX], outdir[PATH_MAX];
static char	namebuf[PATH_MAX], lockedf[PATH_MAX*3+3];
static char	oldname[PATH_MAX], newname[PATH_MAX];
static int	nflag = 0;
static char	flags[1024];
static histdata	history[MAX_HISTORY];
static int	nhist;
static int	curhistp;
static int	pid;

static void
showsettings()
{
    LOG_PRINT("*********************************************\n");
    LOG_PRINT("    Daemon PID          : %d\n", pid);
    LOG_PRINT("    Watching  directory : %s\n", indir);
    LOG_PRINT("    History size        : %d\n", nhist);
    LOG_PRINT("    Log file            : %s\n", lognmbase);
    LOG_PRINT("    Flags               : %s (%x)\n", flags, nflag);
    LOG_PRINT("*********************************************\n");
}

static void
dirnmcpy(char *dst, char *src)
{
    strcpy(dst, src);
    if (dst[strlen(dst) - 1] != '/') strcat(dst, "/");
}

histdata *
mkhist(char *path, int *entsize)
{
    static char	date[128], type[128], fnam[128];
    int		i, entpoint, entries;
    long long	dt;
    char	*cp;

    if (regex_match(path, date, type, fnam) < 0) {
	LOG_PRINT("Cannot parse the file name (%s). skipping\n", path);
	return NULL;
    }
    dt = atoll(date);
    LOG_PRINT("kwatcher: date %lld\n", dt);
    if ((entpoint = sync_entry(type)) < 0) {
	LOG_PRINT("Not expected data for synchronization: %s\n",
		fnam);
	return NULL;
    }
    if ((cp = malloc(strlen(fnam) + 1)) == NULL) {
	LOG_PRINT("mkhist:Cannot reserve memory\n"); exit(-1);
    }
    strcpy(cp, fnam);
    strcpy(oldname, outdir); strcat(oldname, path);
    strcpy(newname, outdir); strcat(newname, cp);
    if (rename(oldname, newname) != 0) {
	LOG_PRINT("Cannot rename (%s, %s) : %d\n", oldname, newname, errno);
	return NULL;
    }
    entries = sync_nsize();
    *entsize = entries;
    if (dt == history[curhistp].date) {
    retry:
	if (history[curhistp].fname[entpoint] != 0) {
	    LOG_PRINT("Multiple receive type: %s\n", type);
	    return NULL;
	}
	history[curhistp].fname[entpoint] = cp;
	for (i = 0; i < entries; i++) {
	    if (history[curhistp].fname[i] == 0) goto nofilled;
	}
	/* all filled */
	return &history[curhistp];
    } else if (dt < history[curhistp].date) {
	LOG_PRINT("Late arrival: %s\n", cp);
	/* find the entry, at most two before */
	for (i = 0; i < 2; i++) {
	    curhistp = (curhistp - 1) % nhist;
	    if (dt == history[curhistp].date) goto retry;
	}
    } else { /* new entry */
	curhistp = (curhistp + 1) % nhist;
	if (dt == history[curhistp].date) {
	    /* A file at the time represented by dt has arrived before */
	    goto retry;
	} else if (history[curhistp].date > 0) {
	    /* removing */
	    VMODE {
		LOG_PRINT("kwatcher: removing %lld\n",
			history[curhistp].date);
	    }
	    for (i = 0; i < entries; i++) {
		char	*tcp;
		if ((tcp = history[curhistp].fname[i])) {
		    unlink(tcp);
		    free(tcp);
		    history[curhistp].fname[i] = 0;
		}
	    }
	}
	history[curhistp].date = dt;
	history[curhistp].fname[entpoint] = cp;
    }
nofilled:
    return NULL;
}

void
transfer(char *fname, void **args)
{
    char	*outdir;
    histdata	*files;
    int		i, fd, lckfd, newlckfd, entries;
    ssize_t	sz;

    outdir = (char*) args[0];
    lckfd = (int) (long long) args[1];
    DBG {
	LOG_PRINT("reading file(%s) lock file(%s)\n",
		fname, locked_name(lckfd));
    }
    if ((fd = open(fname, O_RDONLY)) < 0) {
	LOG_PRINT("Cannot open file %s\n", fname);
	return;
    }
    if ((sz = read(fd, namebuf, PATH_MAX)) < 0) {
	LOG_PRINT("Cannot read file %s\n", fname);
	return;
    }
    namebuf[sz] = 0; /* terminating for string */
    /* adding history */
    VMODE {
	LOG_PRINT("kwatcher: adding %s\n", namebuf);
    }
    if ((files = mkhist(namebuf, &entries)) == NULL) return;
    VMODE {
	LOG_PRINT("kwatcher: available %lld\n", files->date);
    }
    lockedf[0] = 0;
    for (i = 0; i < entries; i++) {
	strcat(lockedf, files->fname[i]); strcat(lockedf, FTSTR_SEPARATOR); 
    }
    locked_write(lckfd, lockedf);
    locked_unlock(lckfd);

    /* new lock */
    newlckfd = locked_lock(outdir);
    args[1] = (void*) (long long) newlckfd;
    DBG {
	LOG_PRINT("new lock file(%s)\n", locked_name(newlckfd));
    }
}

int
main(int argc, char **argv)
{
    int		dmflag = 0;
    int		opt;
    int		lckfd;
    void	*args[3];

    if (argc < 3) {
	LOG_PRINT("usage: %s <watching directory> <data directory> "
		"[-c <conf. file>] "
		"[-h <history count>] "
		"[-f <log file name>] "
		"[-D] [-d] [-v]\n", argv[0]);
	return -1;
    }
    curhistp = 0; nhist = MAX_HISTORY;
    strcpy(confname, DEFAULT_CONFNAME);
    if (argc > 3) {
	while ((opt = getopt(argc, argv, "c:dDvh:")) != -1) {
	    switch (opt) {
	    case 'c': /* conf */
		strcpy(confname, optarg);
		break;
	    case 'd': /* debug mode */
		nflag |= MYNOTIFY_DEBUG;
		strcat(flags, "-d ");
		break;
	    case 'D': /* daemonize */
		dmflag = 1;
		strcat(flags, "-D ");
		break;
	    case 'f':
		strcpy(lognmbase, optarg);
		break;
	    case 'v': /* verbose mode */
		nflag |= MYNOTIFY_VERBOSE;
		strcat(flags, "-v ");
		break;
	    case 'h':
		nhist = atoi(optarg);
		break;
	    }
	}
    }
    if (dmflag == 1) {
	pid = mydaemonize(lognmbase);
    }
    regex_init(confname);
    dirnmcpy(indir, argv[optind]);
    dirnmcpy(outdir, argv[optind+1]);
    showsettings();
    args[0] = outdir;
    DBG {
	LOG_PRINT("Try to lock in (%s)\n", outdir); fflush(stderr);
    }
    lckfd = locked_lock(outdir);
    DBG {
	LOG_PRINT("locked\n"); fflush(stderr);
    }
    args[1] = (void*) (long long) lckfd;
    mynotify(indir, 0, transfer, args, nflag);
    return 0;
}
