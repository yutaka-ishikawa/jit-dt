#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include "translib.h"
#include "translocklib.h"
#include "inotifylib.h"

void
transfer(char *fname, void **args)
{
    char	*outdir;

    outdir = (char*) args[0];
    fprintf(stderr, "kwatcher: catching event in %s. file %s is ready in %s\n", outdir, fname, outdir);
    locked_lock(outdir);
    locked_write(fname);
    locked_unlock();
}

int
main(int argc, char **argv)
{
    int		opt;
    char	*indir;
    int		nflag = 0;
    void	*args[3];

    if (argc < 3) {
	fprintf(stderr, "usage: %s <watching directory> <data directory>\n", argv[0]);
	return -1;
    }
    if (argc > 3) {
	while ((opt = getopt(argc, argv, "dv")) != -1) {
	    switch (opt) {
	    case 'd': /* debug mode */   nflag |= MYNOTIFY_DEBUG;   break;
	    case 'v': /* verbose mode */ nflag |= MYNOTIFY_VERBOSE; break;
	    }
	}
    }
    indir = argv[1];
    args[0] = argv[2];
    mynotify(indir, 0, transfer, args, nflag);
    return 0;
}
