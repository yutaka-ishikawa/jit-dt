/*
 *	Just In Time Data Transfer
 *	15/08/2016 For locked move function, Yutaka Ishikawa
 */
#include "misclib.h"
#define LCK_FILE_1	"JITDT-READY-1"
#define LCK_FILE_2	"JITDT-READY-2"

typedef struct obs_size {
    int		elem[2];
} obs_size;

extern int	jitopen(char*, char*, int type);
extern int	jitclose(int);
extern int	jitread(int, void*, size_t);
extern int	jitget(char*, char*, void*, void*);
extern char	*jitname(int);

#define FTYPE_VR	0
#define FTYPE_ZE	1
#define FTYPE_FLAG	2
/*
 *
 */
#define FTYPE_VR_ENT	0
#define FTYPE_ZE_ENT	1
#define FTYPE_NUM	2
//#define FTYPE_FLAG_ENT	2
//#define FTYPE_NUM	3
#define FTSTR_VR	"vr"
#define FTSTR_ZE	"ze"
//#define FTSTR_FLAG	"fl"

#define FTSTR_SEPARATOR	";"
#define FTCHR_SEPARATOR	';'

static inline int
asc2ent(char *type)
{
    if (!strcmp(type, FTSTR_VR)) return FTYPE_VR_ENT;
    if (!strcmp(type, FTSTR_ZE)) return FTYPE_ZE_ENT;
//    if (!strcmp(type, FTSTR_FLAG)) return FTYPE_FLAG_ENT;
    return FTYPE_VR_ENT; /* no match */
}

static inline void
septype(char *str, char **files)
{
    int		i;
    char	*cp, *next;

    for (next = str, i = 0; i < FTYPE_NUM; i++) {
	if ((cp = index(next, FTCHR_SEPARATOR))) {
	    files[i] = next;
	    *cp = 0;
	    next = cp + 1;
	}
    }
    return;
}
