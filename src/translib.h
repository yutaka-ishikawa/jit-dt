/*
 *	Just In Time Data Transfer
 *	15/08/2016 Adding locked move function, Yutaka Ishikawa
 *	13/08/2016 Adding sftp protocol
 *			by Yutaka Ishikawa, RIKEN AICS, 
 *	15/12/2015 Created
 *			by Yutaka Ishikawa, RIKEN AICS,yutaka.ishikawa@riken.jp
 */
#define TRANS_DEBUG	1
#define TRANS_VERBOSE	2

#define TRANS_HTTP	0
#define TRANS_SCP	1
#define TRANS_SFTP	2
#define TRANS_LOCK	3
#define TRANS_TMAX	4
#define TRANS_UNKNOWN	-1
#define TRANS_NONE	-2

extern void	trans_setflag(int flag);
extern int	trans_type(char *url, char **host, char **rpath);

extern double	http_put(char *host, char *rpath, char *fname, void **opt);
extern double	scp_put(char *host, char *rpath, char *fname, void **opt);
extern double	sftp_put(char *host, char *rpath, char *fname, void **opt);
extern double	locked_move(char *, char *rhpath, char *fname, void **opt);
extern double	(*ttable[TRANS_TMAX])(char*, char*, char*, void**);
extern void	sftp_terminate();

#if 0
#define LCK_FILE	"JITDT-READY"
extern int	locked_lock(char*, int);
extern void	locked_unlock();
extern void	locked_write(int, char*);
extern int	locked_read(int, char*, int);
#endif

static inline void
fformat(char *path)
{
    if (path[strlen(path) - 1] != '/') strcat(path, "/");
}
