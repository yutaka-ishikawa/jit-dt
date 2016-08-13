//#define DEBUG
extern int	dflag;
#ifdef DEBUG
#define DBG	if (dflag)
#else
#define DBG	if (0)
#endif

extern double	http_put(char *url, char *fname);
struct timeval	getTime();
double	duration(struct timeval start, struct timeval end);
