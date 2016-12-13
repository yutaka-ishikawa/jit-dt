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
#include "translib.h"
#include "inotifylib.h"

//#define MAX_HISTORY	(2*60*2)	/* 2 hour in case of 30sec internal */
#define MAX_HISTORY	(2*10)

#define DBG if (nflag&MYNOTIFY_DEBUG)
#define VMODE if (nflag & TRANS_VERBOSE)

static char	lognmbase[PATH_MAX] = "./KWATCHLOG";
static char	indir[PATH_MAX], outdir[PATH_MAX];
static char	combuf[PATH_MAX];
static int	nflag = 0;
static char	*history[MAX_HISTORY];
static int	nhist;
static int	curhistp;

static void
dirnmcpy(char *dst, char *src)
{
    strcpy(dst, src);
    if (dst[strlen(dst) - 1] != '/') strcat(dst, "/");
}

char *
mkhist(char *path)
{
    char	*cp;
    char	*old;
    if ((cp = malloc(strlen(path) + 1)) == NULL) {
	fprintf(stderr, "mkhist:Cannot reserve memory\n"); exit(-1);
    }
    strcpy(cp, path);
    old = history[curhistp];
    history[curhistp] = cp;
    curhistp = (curhistp + 1) % nhist;
    return old;
}

void
transfer(char *fname, void **args)
{
    char	*outdir;
    char	*old;
    int		fd, lckfd, newlckfd;
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
    if ((sz = read(fd, combuf, PATH_MAX)) < 0) {
	fprintf(stderr, "Cannot read file %s\n", fname);
	return;
    }
    DBG {
	fprintf(stderr, "kwatcher:%s is ready in %s\n", combuf, outdir);
    }
    locked_write(lckfd, combuf);
    locked_unlock(lckfd);

    /* adding history */
    if ((old = mkhist(combuf)) != NULL) {
	DBG {
	    fprintf(stderr, "kwatcher: removing %s\n", old);
	}
	unlink(old);
	free(old);
    }
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
    if (argc > 3) {
	while ((opt = getopt(argc, argv, "dDvh:")) != -1) {
	    switch (opt) {
	    case 'd': /* debug mode */   nflag |= MYNOTIFY_DEBUG;   break;
	    case 'D': /* daemonize */
		dmflag = 1; break;
	    case 'v': /* verbose mode */ nflag |= MYNOTIFY_VERBOSE; break;
	    case 'h':
		nhist = atoi(optarg);
		break;
	    }
	}
    }
    if (dmflag == 1) {
	mydaemonize(lognmbase);
    }
    dirnmcpy(indir, argv[optind]);
    dirnmcpy(outdir, argv[optind+1]);
    args[0] = outdir;
    lckfd = locked_lock(outdir);
    args[1] = (void*) (long long) lckfd;
    mynotify(indir, 0, transfer, args, nflag);
    return 0;
}
