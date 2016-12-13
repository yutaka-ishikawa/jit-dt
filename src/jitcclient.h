#define HOST_DEFAULT	"localhost"
#define PORT_DEFAULT	8080

#define CMD_SIZE	4
#define CMD_ERROR	-1
/**/
#define CMD_OPEN	1
#define CMD_CLOSE	2
#define CMD_GET		3
#define CMD_EXIT	4
#define CMD_STATUS	5
#define CMD_READ	6
#define CMD_REPLY	7
#define CMD_MAX		8

#define SCMD_NULL	"    "
#define SCMD_OPEN	"OPEN"
#define SCMD_CLOSE	"CLOS"
#define SCMD_GET	"GET0"
#define SCMD_EXIT	"EXIT"
#define SCMD_STATUS	"STAT"
#define SCMD_READ	"READ"
#define SCMD_REPLY	"REPL"

struct trans_cmd {
    char	cmd[CMD_SIZE+1];
    int		opt1, opt2, opt3, opt4;
    long long	date;
    int		len;
};
#define TCMD_SIZE	(sizeof(struct trans_cmd))

struct trans_openrpl {
    int		offset1;
    int		offset2;
};

extern int	setupinet(struct sockaddr_in *saddrp, char *host, int port);
extern int	trans_getcmd(int sock, int *, int *, int *);
extern int	trans_replyopen(int sock, char *fpath, int rcc);
extern int	trans_replyread(int sock, char *buf, int size);
extern int	trans_replyget(int sock, unsigned long long,
			       char *buf, int totsz, int *retval);
