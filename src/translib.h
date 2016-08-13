//#define DEBUG
extern int	dflag;
#ifdef DEBUG
#define DBG	if (dflag)
#else
#define DBG	if (0)
#endif

#define TRANS_HTTP	0
#define TRANS_SCP	1
#define TRANS_SFTP	2
#define TRANS_LOCK	3
#define TRANS_NONE	-1

extern int	trans_type(char *url);
extern double	http_put(char *url, char *fname);
extern double	scp_put(char *url, char *fname);
extern double	sftp_put(char *url, char *fname);
extern double	locked_move(char *url, char *fname);
