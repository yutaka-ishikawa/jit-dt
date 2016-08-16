#define MYNOTIFY_VERBOSE	1
#define MYNOTIFY_DEBUG		2

extern int mynotify(char *topdir, char *startdir,
		    void (*fnc)(char*, void**), void **, int flg);
