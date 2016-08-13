#include <stdio.h>
#include <string.h>
#include <unistd.h>

void
pipetest()
{
    static int	first = 0;
    static FILE	*wfp = NULL;
    int		pipefd[2];

    if (first == 0) {
	first = 1;
	if (pipe(pipefd) == -1) {
	    perror("Creating pipe: failed");
	    exit(-1);
	}
	if ((pid = fork()) == 0) {
	    int		cc;
	    /* child */
	    close(pipefd[1]); //close write side from parents
            close(0); //close stdin
            dup(pipefd[0]); //connect pipe
	    cc = execl("/usr/bin/ftp", "ftp", 0);
	    if (cc < 0) {
		perror("Cannot exec sftp");
		exit(cc);
	    }
	} else if (pid < 0) {
	    perror("Creating pipe: failed");
	    exit(-1);
	}
	close(pipefd[0]);
	wfp = fdopen(pipefd[1], "w");
    }
    if (wfp == NULL) {
	perror("Something wrong\n");
	exit(-1);
    }
    fprintf(wfp, "put %s\n", fname); fflush(wfp);
}

main()
{
    pipetest();
}
