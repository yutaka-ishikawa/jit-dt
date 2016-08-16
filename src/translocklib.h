/*
 *	Just In Time Data Transfer
 *	15/08/2016 For locked move function, Yutaka Ishikawa
 */
#define LCK_FILE_1	"JITDT-READY-1"
#define LCK_FILE_2	"JITDT-READY-2"

extern int	jitopen(char*, char*);
extern int	jitclose(int);

extern char	*locked_name(int);
extern int	locked_lock(char *path);
extern void	locked_unlock(int);
extern void	locked_write(int, char *info);
extern int	locked_read(int, char *buf, int size);
