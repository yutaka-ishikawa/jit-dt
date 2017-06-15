#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
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

static char	lognmbase[PATH_MAX] = "./KWATCHLOG";
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
    fprintf(stderr, "*********************************************\n");
    fprintf(stderr, "    Daemon PID          : %d\n", pid);
    fprintf(stderr, "    Watching  directory : %s\n", indir);
    fprintf(stderr, "    History size        : %d\n", nhist);
    fprintf(stderr, "    Flags               : %s (%x)\n", flags, nflag);
    fprintf(stderr, "*********************************************\n");
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
	fprintf(stderr, "Cannot parse the file name (%s). skipping\n", path);
	return NULL;
    }
    dt = atoll(date);
    fprintf(stderr, "kwatcher: date %lld\n", dt);
    if ((entpoint = sync_entry(type)) < 0) {
	fprintf(stderr, "Not expected data for synchronization: %s\n",
		fnam);
	return NULL;
    }
    if ((cp = malloc(strlen(fnam) + 1)) == NULL) {
	fprintf(stderr, "mkhist:Cannot reserve memory\n"); exit(-1);
    }
    strcpy(cp, fnam);
    strcpy(oldname, outdir); strcat(oldname, path);
    strcpy(newname, outdir); strcat(newname, cp);
    if (rename(oldname, newname) != 0) {
	fprintf(stderr, "Cannot rename (%s, %s)\n", oldname, newname);
	return NULL;
    }
    entries = sync_nsize();
    *entsize = entries;
    if (dt == history[curhistp].date) {
	if (history[curhistp].fname[entpoint] != 0) {
	    fprintf(stderr, "Multiple receive type: %s\n", type);
	    return NULL;
	}
	history[curhistp].fname[entpoint] = cp;
	for (i = 0; i < entries; i++) {
	    if (history[curhistp].fname[i] == 0) goto nofilled;
	}
	/* all filled */
	return &history[curhistp];
    } else { /* new entry */
	curhistp = (curhistp + 1) % nhist;
	if (history[curhistp].date > 0) {
	    /* removing */
	    VMODE {
		fprintf(stderr, "kwatcher: removing %lld\n",
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
	fprintf(stderr, "reading file(%s) lock file(%s)\n",
		fname, locked_name(lckfd));
    }
    if ((fd = open(fname, O_RDONLY)) < 0) {
	fprintf(stderr, "Cannot open file %s\n", fname);
	return;
    }
    if ((sz = read(fd, namebuf, PATH_MAX)) < 0) {
	fprintf(stderr, "Cannot read file %s\n", fname);
	return;
    }
    namebuf[sz] = 0; /* terminating for string */
    /* adding history */
    VMODE {
	fprintf(stderr, "kwatcher: adding %s\n", namebuf);
    }
    if ((files = mkhist(namebuf, &entries)) == NULL) return;
    VMODE {
	fprintf(stderr, "kwatcher: available %lld\n", files->date);
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
	fprintf(stderr, "new lock file(%s)\n", locked_name(newlckfd));
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
	fprintf(stderr, "usage: %s <watching directory> <data directory> "
		"[-h history count] [-d] [-v]\n", argv[0]);
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
	fprintf(stderr, "Try to lock in (%s)\n", outdir);
    }
    lckfd = locked_lock(outdir);
    DBG {
	fprintf(stderr, "locked\n");
    }
    args[1] = (void*) (long long) lckfd;
    mynotify(indir, 0, transfer, args, nflag);
    return 0;
}
