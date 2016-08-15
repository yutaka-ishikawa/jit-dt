/*
 *	Just In Time Data Transfer
 *	15/08/2016 Adding locked move function, Yutaka Ishikawa
 *	13/08/2016 Adding sftp protocol
 *			by Yutaka Ishikawa, RIKEN AICS, 
 *	15/12/2015 Created
 *			by Yutaka Ishikawa, RIKEN AICS,yutaka.ishikawa@riken.jp
 */
//#define DEBUG
extern int	vflag;
extern int	dflag;
#ifdef DEBUG
#define DBG	if (dflag)
#else
#define DBG	if (0)
#endif

#define VMODE	if (vflag)
#define TRANS_HTTP	0
#define TRANS_SCP	1
#define TRANS_SFTP	2
#define TRANS_LOCK	3
#define TRANS_TMAX	4
#define TRANS_UNKNOWN	-1
#define TRANS_NONE	-2
#define LCK_FILE	"JITDT-READY"

extern int	trans_type(char *url, char **host, char **rpath);
extern double	http_put(char *host, char *rpath, char *fname);
extern double	scp_put(char *host, char *rpath, char *fname);
extern double	sftp_put(char *host, char *rpath, char *fname);
extern double	locked_move(char *, char *rhpath, char *fname);
extern double	(*ttable[TRANS_TMAX])(char*, char*, char*);
extern void	sftp_keep_process();
extern void	sftp_terminate();

extern void	locked_lock(char*);
extern void	locked_unlock();
extern void	locked_write(char*);
extern int	locked_read(char*, int);

static inline void
fformat(char *path)
{
    if (path[strlen(path) - 1] != '/') strcat(path, "/");
}
