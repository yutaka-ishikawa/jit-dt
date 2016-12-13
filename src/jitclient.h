/*
 *	Just In Time Data Transfer
 *	15/08/2016 For locked move function, Yutaka Ishikawa
 */
#include "misclib.h"
#define LCK_FILE_1	"JITDT-READY-1"
#define LCK_FILE_2	"JITDT-READY-2"

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
