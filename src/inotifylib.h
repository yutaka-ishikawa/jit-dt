/*
 * The following constants should be the same value of
 * TRANS_DEBUG/TRANS_VERBOSE
 */
#define MYNOTIFY_DEBUG		1
#define MYNOTIFY_VERBOSE	2

extern int mynotify(char *topdir, char *startdir,
		    int (*fnc)(char*, void**), void **, int flg);
