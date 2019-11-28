#ifndef JIT_MISCLIB
#define JIT_MISCLIB
#include <sys/time.h>

#define LOG_PRINT(...) fprintf(stderr, __VA_ARGS__); fflush(stderr);
#define LOG_MAXENTRIES	(2*2*60*2)	/* 2 hours */

extern int	mydaemonize();
extern void	logfupdate();
extern void	mygettime(struct timeval *sp, struct timezone *tp);
extern void	timeconv(struct timeval *tsec, char *fmtbuf);
extern void	dateconv(struct timeval	*tsec, char *dbufp, char *fmtbuf);

extern char	*locked_name(int);
extern int	locked_lock(char *path);
extern void	locked_unlock(int);
extern void	locked_unlock_nullify(int);
extern void	locked_write(int, char *info);
extern int	locked_read(int, char *buf, int size);

#define FNAME_MAX	3
typedef struct histdata {
    unsigned long long	date;		/* 20161208172500 (14 digit) */
    char		*fname[FNAME_MAX];
} histdata;

extern void	histinit(int);
extern int	histsize();
extern histdata	*histget();
extern void	histremove();
extern void	histwait();
extern void	histput(char *path, long long date, int type);
extern int	mydaemonize(char*);

#endif /* JIT_MISCLIB */
