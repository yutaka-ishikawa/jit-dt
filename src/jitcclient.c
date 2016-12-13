/*
 *	Just In Time Data Transfer
 *	12/10/2016 For cluster environment, Yutaka Ishikawa
 */
#ifdef MPIENV
#include <mpi.h>
#endif /* MPIENV */
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include "jitclient.h"
#include "jitcclient.h"

#define COM_SIZE	1024
static char	url[PATH_MAX];

static char	*strcmd[] = {
    SCMD_NULL,    SCMD_OPEN,    SCMD_CLOSE, SCMD_GET,    SCMD_EXIT,
    SCMD_STATUS,  SCMD_READ,    SCMD_REPLY,   0 };

#ifdef MPIENV
#define ROOT		0
static int
is_myrank() {
    static int	myrank = -1;
    if (myrank < 0) MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    return myrank;
}
#endif

int
setupinet(struct sockaddr_in *saddrp, char *host, int port)
{
    int			cc;
    struct addrinfo	*ainfo, *ap;

    memset(saddrp, 0, sizeof(struct sockaddr_in));
    memset(&ainfo, 0, sizeof(ainfo));
    if ((cc = getaddrinfo(host, NULL, NULL, &ainfo)) < 0) {
	perror("getaddrinfo:"); exit(-1);
    }
    for (ap = ainfo; ap != NULL; ap = ap->ai_next) {
	if (ap->ai_family == AF_INET) break;
    }
    if (ap == 0) return -1;
    *saddrp = *(struct sockaddr_in*) ap->ai_addr;
    saddrp->sin_family = AF_INET;
    saddrp->sin_port = htons(port);
    return 0;
}

int
netopen(char *hname)
{
    char		*host, *cp;
    int			sock;
    int			port;
    int			cc;
    struct sockaddr_in	saddr;
    socklen_t		addrlen;

    strncpy(url, hname, PATH_MAX);
    host = url;
    if ((cp = index(url, ':'))) {
	*cp = 0;
	port = atoi(cp + 1);
    } else {
	port = PORT_DEFAULT;
    }
    if ((cc = setupinet(&saddr, host, port)) < 0) {
	fprintf(stderr, "No IP address of %s for AF_INET\n", host);
	exit(-1);
    }
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
	perror("socket"); exit(1);
    }
    addrlen = sizeof(saddr);
    if (connect(sock, (struct sockaddr*) &saddr, addrlen) < 0) {
	printf("sin_addr = %s sin_port = %d\n",
	       inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
	perror("connect");
	exit(-1);
    }
    return sock;
}

int
netread(int sock, char *buf, size_t size)
{
    int		cc;
    int		sz = 0;
    do {
	if ((cc = read(sock, &buf[sz], size - sz)) < 0) return cc;
	sz += cc;
    } while (sz != size);
    return sz;
}

int
netwrite(int sock, char *buf, size_t size)
{
    int		cc;
    int		sz = 0;

    /*
     * MSG_NOSIGNAL must be specified, otherwise SIGPIPE signal will be
     * received when the peer process dies */
    do {
	if ((cc = send(sock, &buf[sz], size - sz, MSG_NOSIGNAL)) < 0) return cc;
	sz += cc;
    } while (sz != size);
    return sz;
}

int
netsendreq(int sock, int cmd, int opt1, int opt2, int op3)
{
    int			cc;
    struct trans_cmd	tcmd;

    if (cmd >= CMD_MAX) return -1;
    strcpy(tcmd.cmd, strcmd[cmd]);
    tcmd.opt1 = opt1; tcmd.opt2 = opt2;
    tcmd.len = 0;
    cc = netwrite(sock, (char*) &tcmd, sizeof(tcmd));
    return cc;
}

int
netrecvreply(int sock, struct trans_cmd **prpl)
{
    int			cc;
    struct trans_cmd	reply;

    if ((cc = netread(sock, (char*) &reply, TCMD_SIZE)) != TCMD_SIZE) return -1;
    *prpl = (struct trans_cmd*) malloc(TCMD_SIZE + reply.len);
    **prpl = reply;
    if (prpl == NULL) return -1;
    if ((cc = netread(sock, (char*) ((*prpl) + 1), (*prpl)->len))
	!= (*prpl)->len) return -1;
    return (*prpl)->len;
}

int
trans_replyopen(int sock, char *fpath, int rcc)
{
    int			cc;
    struct trans_cmd	tcmd;

    strcpy(tcmd.cmd, strcmd[CMD_REPLY]);
    tcmd.opt1 = rcc;
    if (fpath == NULL) {
	tcmd.len = 0;
    } else {
	tcmd.len = strlen(fpath);
    }
    cc = netwrite(sock, (char*) &tcmd, sizeof(tcmd));
    if (cc < 0) return cc;
    if (fpath != NULL) {
	cc = netwrite(sock, (char*) fpath, tcmd.len);
    }
    return cc;
}

int
trans_replyread(int sock, char *buf, int size)
{
    int			cc;
    struct trans_cmd	tcmd;

    strcpy(tcmd.cmd, strcmd[CMD_REPLY]);
    tcmd.len = size;
    cc = netwrite(sock, (char*) &tcmd, sizeof(tcmd));
    if (cc < 0) return cc;
    cc = netwrite(sock, buf, tcmd.len);
    return cc;
}

int
trans_replyget(int sock, unsigned long long date, char *buf, int totsz, int *retval)
{
    int			cc;
    struct trans_cmd	tcmd;

    strcpy(tcmd.cmd, strcmd[CMD_REPLY]);
    tcmd.opt1 = retval[0];  tcmd.opt2 = retval[1]; tcmd.opt3 = retval[2];
    tcmd.date = date;
    tcmd.len = totsz;
    cc = netwrite(sock, (char*) &tcmd, sizeof(tcmd));
    if (cc < 0) return cc;
    cc = netwrite(sock, buf, totsz);
    return cc;
}


int
trans_getcmd(int sock, int *opt1, int *opt2, int *opt3)
{
    int		i, len;
    struct trans_cmd	tcmd;

    if ((len = netread(sock, (char*) &tcmd, TCMD_SIZE)) <= 0) {
	printf("trans_getcmd: %d\n", len);
	return CMD_EXIT;
    }
    *opt1 = tcmd.opt1; *opt2 = tcmd.opt2; *opt3 = tcmd.opt3;
    for (i = CMD_OPEN; i < CMD_MAX; i++) {
	if (!strncmp(tcmd.cmd, strcmd[i], CMD_SIZE)) return i;
    }
    return CMD_EXIT;
}


int
_jitopen(char *host, char *fname, int type)
{
    int		cc, sock;
    char	*fnm;
    struct trans_cmd		*reply;

    sock = netopen(host);
    if ((cc = netsendreq(sock, CMD_OPEN, type, 0, 0)) < 0) {
	return -1;
    }
    if ((cc = netrecvreply(sock, &reply)) < 0) {
	return -1;
    }
    if (reply->opt1 < 0) {
	fname[0] = 0;
	close(sock);
	free(reply);
	return -1;
    }
    fnm = (char*) (reply + 1);
    strcpy(fname, fnm);
    free(reply);
    return sock;
}

int
jitopen(char *place, char *fname, int type)
{
    int		sock;
#ifdef MPIENV
    int		sz = 0;
    if (is_myrank() == ROOT) {
	sock = _jitopen(place, fname, type);
	if (fname != NULL) sz = strlen(fname);
    }
    MPI_Bcast(&sock, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    MPI_Bcast(&sz, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    if (sz > 0) {
	MPI_Bcast(fname, sz, MPI_BYTE, ROOT, MPI_COMM_WORLD);
	fname[sz] = 0;
    }
#else
    sock = _jitopen(place, fname, type);
#endif
    //printf("jitopen: fname=%s\n", fname);
    return sock;
}

int
_jitclose(int sock)
{
    int			cc;

    cc = netsendreq(sock, CMD_CLOSE, 0, 0, 0);
    close(sock);
    return cc;
}

int
jitclose(int sock)
{
    int		cc;
#ifdef MPIENV
    if (is_myrank() == ROOT) {
	cc = _jitclose(sock);
    }
    MPI_Bcast(&cc, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
#else
    cc = _jitclose(sock);
#endif
    return cc;
}

int
_jitread(int sock, void *buf, size_t size)
{
    int			cc;
    struct trans_cmd	tcmd;

    if ((cc = netsendreq(sock, CMD_READ, size, 0, 0)) < 0) {
	return -1;
    }
    if ((cc = netread(sock, (char*) &tcmd, TCMD_SIZE)) <= 0) {
	printf("_jitread: %d\n", cc);
	return CMD_EXIT;
    }
    cc = netread(sock, buf, tcmd.len);
    return cc;
}

int
jitread(int sock, void *buf, size_t size)
{
    int		sz;
#ifdef MPIENV
    if (is_myrank() == ROOT) {
	sz = _jitread(sock, buf, size);
    }
    MPI_Bcast(&sz, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    if (sz > 0) {
	MPI_Bcast(buf, sz, MPI_BYTE, ROOT, MPI_COMM_WORLD);
    }
#else
    sz = _jitread(sock, buf, size);
#endif
    //printf("jitread: size(%d)\n", sz);
    return sz;
}

typedef struct obs_size {
    int		elem[2];
} obs_size;

int
_jitget(char *host, char *fname, void *data, void *size)
{
    int		sock;
    int		ptr, i;
    obs_size	*szp = (obs_size*) size;
    int		*bsize, *rsize;
    int			cc;
    struct trans_cmd	tcmd;

    bsize = szp->elem;  rsize = (szp + 1)->elem;
    sock = netopen(host);
    /* FTYPE_NUM is 2 */
    if ((cc = netsendreq(sock, CMD_GET, bsize[0], bsize[1], 0)) < 0) {
	return -1;
    }
    if ((cc = netread(sock, (char*) &tcmd, TCMD_SIZE)) <= 0) {
	return CMD_EXIT;
    }
    rsize[0] = tcmd.opt1; rsize[1] = tcmd.opt2;
    for (ptr = 0, i = 0; i < FTYPE_NUM; i++) {
	netread(sock, ((char*)data) + ptr, rsize[i]);
	ptr += bsize[i];
    }
    close(sock);
    return 0;
}

int
jitget(char *place, char *fname, void *data, void *size)
{
    int		cc;
#ifdef MPIENV
    int		i, *bsize, *rsize, ptr;
    obs_size	*szp = (obs_size*) size;

    if (is_myrank() == ROOT) {
	cc = _jitget(place, fname, data, size);
    }
    MPI_Bcast(&cc, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    bsize = szp->elem; rsize = (szp + 1)->elem;
    MPI_Bcast(rsize, sizeof(obs_size), MPI_BYTE, ROOT, MPI_COMM_WORLD);
    ptr = 0;
    for (i = 0; i < FTYPE_NUM; i++) {
	if (rsize[i] > 0) {
	    MPI_Bcast(((char*)data) + ptr, rsize[i], MPI_BYTE,
		      ROOT, MPI_COMM_WORLD);
	    ptr += bsize[i];
	} 
    }
//    if (cc > 0) {
//	MPI_Bcast(buf, sz, MPI_BYTE, ROOT, MPI_COMM_WORLD);
//    }
#else
    cc = _jitget(place, fname, data, size);
#endif
    //printf("jitread: size(%d)\n", sz);
    return cc;
}
