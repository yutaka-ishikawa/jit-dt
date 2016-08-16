#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include "translib.h"
#include "translocklib.h"
#include "inotifylib.h"

#define DBG if (nflag&MYNOTIFY_DEBUG)

static char	combuf[PATH_MAX];
static int	nflag = 0;

void
transfer(char *fname, void **args)
{
    char	*outdir;
    int		fd, lckfd, newlckfd;
    ssize_t	sz;

    outdir = (char*) args[0];
    lckfd = (int) (long long) args[1];
    DBG {
	fprintf(stderr, "lock file = %s\n", locked_name(lckfd));
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
	fprintf(stderr, "kwatcher: catching event in %s. file %s is ready in %s\n", outdir, combuf, outdir);
    }
    locked_write(lckfd, combuf);
    locked_unlock(lckfd);
    /* new lock */
    newlckfd = locked_lock(outdir);
    args[1] = (void*) (long long) newlckfd;
    DBG {
	fprintf(stderr, "new lock file = %s\n", locked_name(newlckfd));
    }
}

int
main(int argc, char **argv)
{
    int		opt;
    char	*indir, *outdir;
    int		lckfd;
    void	*args[3];

    if (argc < 3) {
	fprintf(stderr, "usage: %s <watching directory> <data directory>\n", argv[0]);
	return -1;
    }
    indir = argv[1]; outdir = argv[2];
    if (argc > 3) {
	while ((opt = getopt(argc, argv, "dv")) != -1) {
	    switch (opt) {
	    case 'd': /* debug mode */   nflag |= MYNOTIFY_DEBUG;   break;
	    case 'v': /* verbose mode */ nflag |= MYNOTIFY_VERBOSE; break;
	    }
	}
    }
    args[0] = outdir;
    lckfd = locked_lock(outdir);
    args[1] = (void*) (long long) lckfd;
    mynotify(indir, 0, transfer, args, nflag);
    return 0;
}
