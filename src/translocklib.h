/*
 *	Just In Time Data Transfer
 *	15/08/2016 For locked move function, Yutaka Ishikawa
 */
extern int	jitopen(char*, char*);
extern int	jitclose(int);

extern void	locked_lock(char *path);
extern void	locked_unlock();
extern void	locked_write(char *info);
extern int	locked_read(char *buf, int size);
