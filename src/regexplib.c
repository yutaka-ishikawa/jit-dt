#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "regexplib.h"

#define STR_LEN	128
#define TKN_ERR_LEN	-1
#define TKN_ERR_CHR	-2
#define	TKN_OK		0
#define TKN_INT		1
#define TKN_POS		2
#define TKN_DLM		3
#define TKN_STR		4
#define TKN_BGN		5
#define TKN_END		6
#define TKN_EQU		7
#define TKN_KEY_BGN	8
#define TKN_KEY_RGN	8
#define TKN_KEY_SYN	9
#define TKN_KEY_FNM	10
#define TKN_KEY_PAT	11
#define TKN_KEY_DAT	12
#define TKN_KEY_TYP	13
#define IS_ALPHA(ch)	(isalpha(ch) || ch == '-' || ch == '_')

#define NMATCH	10
#define MAX_CONF	40
#define MAX_SYNCFILES	10

#define PARSE_KEYSTRING(type) {				\
    tkn = gettoken(fp, buf, &val);			\
    ntkn = gettoken(fp, buf, &val);			\
    if (tkn != TKN_EQU && ntkn != TKN_STR) goto err1;	\
    strcpy(conf[cent].c_##type, buf);			\
}

struct conf {
    char	c_area[STR_LEN];
    char	c_pattern[STR_LEN];
    char	c_name[STR_LEN];
    char	c_type[STR_LEN];
    char	c_date[STR_LEN];
    char	c_fnam[STR_LEN];
    regex_t	c_preg;
};

static char	*key[] = {
    "region", "sync", "fname", "pattern", "date", "type", 0 };
static int		line;
static struct conf	conf[MAX_CONF+1];
static char		synctype[MAX_SYNCFILES][STR_LEN];
static int		confent;
static int		syncent;
static char		errbuf[1024];

char
skipblank(FILE *fp)
{
    char	ch;
retry:
    while ((ch = getc(fp)) != EOF) {
	if (ch == '\n') line++;
	if (!isspace(ch)) break;
    }
    if (ch == '#') {
	while ((ch = getc(fp)) != EOF) if (ch == '\n') break;
	line++;
	goto retry;
    }
    return ch;
}


int
gettoken(FILE *fp, char *str, int *val)
{
    int		tktype, i;
    char	ch;
    char	buf[STR_LEN];

retry:
    ch = skipblank(fp);
    if (IS_ALPHA(ch)) {
	i = 0;
	do {
	    buf[i++] = ch;
	    ch = getc(fp);
	} while (i < STR_LEN && (IS_ALPHA(ch)||isdigit(ch)) );
	ungetc(ch, fp);
	if (i == STR_LEN) return TKN_ERR_LEN;
	buf[i] = 0;
	i = 0;
	while (key[i]) {
	    if (!strcmp(buf, key[i])) {
		return TKN_KEY_BGN + i;
	    }
	    i++;
	}
	strcpy(str, buf);
	return TKN_STR;
    } else if (isdigit(ch)) {
	tktype = TKN_INT;
	goto numhandling;
    } else if (ch == '$') {
	ch = getc(fp);
	tktype = TKN_POS;
	goto numhandling;
    } else if (ch == ';') {
	goto retry;
    } else if (ch == '"') {
	i = 0;
	do {
	    ch = getc(fp);
	    if (ch == '\\') ch = getc(fp);
	    buf[i++] = ch;
	} while (i < STR_LEN && isprint(ch) && ch != '"');
	if (i == STR_LEN) return TKN_ERR_LEN;
	buf[i - 1] = 0;
	strcpy(str, buf);
	return TKN_STR;
    } else if (ch == '{') {
	return TKN_BGN;
    } else if (ch == '}') {
	return TKN_END;
    } else if (ch == '=') {
	return TKN_EQU;
    }
    return TKN_ERR_CHR;

numhandling:
    i = 0;
    do {
	buf[i++] = ch;
	ch = getc(fp);
    } while (i < STR_LEN && isdigit(ch));
    if (i == STR_LEN) return TKN_ERR_LEN;
    ungetc(ch, fp);
    buf[i] = 0;
    *val = atoi(buf);
    return tktype;
}

void
readconf(char *fname)
{
    FILE	*fp;
    char	buf[STR_LEN];
    int		tkn, ntkn, val;
    int		cent, sent;

    line = 1;
    if ((fp = fopen(fname, "r")) == NULL) {
	fprintf(stderr, "Cannot open a configuration file:%s\n", fname);
	exit(-1);
    }
    cent = 0; sent = 0;
    while ((tkn = gettoken(fp, buf, &val)) > TKN_OK) {
	if (tkn == TKN_KEY_RGN) {
	    if (cent >= MAX_CONF) goto err2;
	    tkn = gettoken(fp, buf, &val);
	    ntkn = gettoken(fp, buf, &val);
	    if (tkn != TKN_STR && ntkn != TKN_BGN) goto err1;
	    strcpy(conf[cent].c_area, buf);
	    while ((tkn = gettoken(fp, buf, &val)) > TKN_OK) {
		switch (tkn) {
		case TKN_END:
		    ++cent;
		    goto next;
		case TKN_KEY_PAT:
		    PARSE_KEYSTRING(pattern); break;
		case TKN_KEY_DAT:
		    PARSE_KEYSTRING(date); 	break;
		case TKN_KEY_TYP:
		    PARSE_KEYSTRING(type);	break;
		case TKN_KEY_FNM:
		    PARSE_KEYSTRING(fnam);	break;
		default:
		    goto err1;
		}
	    }
	} else if (tkn == TKN_KEY_SYN) {
	    tkn = gettoken(fp, buf, &val);
	    if (tkn != TKN_BGN) goto err1;
	    while ((tkn = gettoken(fp, buf, &val)) > TKN_OK) {
		switch (tkn) {
		case TKN_END:
		    goto next;
		case TKN_KEY_TYP:
		    tkn = gettoken(fp, buf, &val);
		    ntkn = gettoken(fp, buf, &val);
		    if (tkn != TKN_EQU && ntkn != TKN_STR) goto err1;
		    if (sent >= MAX_SYNCFILES) goto err2;
		    strcpy(synctype[sent], buf);
		    sent++;
		    break;
		default:
		    goto err1;
		}
	    }
	} else {
	    goto err1;
	}
    next:
	;
    }
    confent = cent;
    syncent = sent;
    return;
err1:
    fprintf(stderr, "configuration: format error in line %d\n", line);
    exit(-1);
err2:
    fprintf(stderr, "configuration: too many areas in line %d\n", line);
    exit(-1);
}


void
regex_init(char *cfnm)
{
    int		i, cc;

    readconf(cfnm);
    printf("confent = %d\n", confent);
    for (i = 0; i < confent; i++) {
	printf("pattern=%s\n", conf[i].c_pattern);
	if ((cc = regcomp(&conf[i].c_preg, conf[i].c_pattern, 0)) < 0) {
	    fprintf(stderr, "regexinit: compile error: %s\n",
		    conf[i].c_pattern);
	    regerror(cc, &conf[i].c_preg, errbuf, 1024);
	    fprintf(stderr, "\t%s\n", errbuf);
	    exit(-1);
	}
    }
    for (i = 0; i < syncent; i++) {
	printf("synctype=%s\n", synctype[i]);
    }
}

void
matchedcopy(char *dst, char *fmt, char *pattern, regmatch_t *pmatch)
{
    int		pos, len;
    char	*cp;

    for (cp = fmt; *cp; cp++) {
	if (*cp == '$') { /* position */
	    pos = *++cp - '0';
	    len = pmatch[pos].rm_eo - pmatch[pos].rm_so;
	    strncpy(dst, &pattern[pmatch[pos].rm_so], len);
	    dst += len;
	} else {
	    *dst++ = *cp;
	}
    }
    *dst = 0;
}

int
regex_match(char *pattern, char *date, char *type, char *fnam)
{
    int		cc;
    struct conf	*cnfp;
    regmatch_t	pmatch[NMATCH];

    cnfp = conf;
    while (cnfp->c_area[0]) {
	if ((cc = regexec(&cnfp->c_preg, pattern, NMATCH, pmatch, 0)) < 0) {
	    fprintf(stderr, "regmatch: regexec error: %s\n",
		    cnfp->c_pattern);
	    regerror(cc, &cnfp->c_preg, errbuf, 1024);
	    fprintf(stderr, "\t%s\n", errbuf);
	    return -1;
	}
	if (cc == 0) break;
	cnfp++;
    }
    if (cnfp->c_area[0] == 0) return -1;
    if (date) matchedcopy(date, cnfp->c_date, pattern, pmatch);
    if (type) matchedcopy(type, cnfp->c_type, pattern, pmatch);
    if (fnam) matchedcopy(fnam, cnfp->c_fnam, pattern, pmatch);
    return 1;
}

int
sync_nsize()
{
    return syncent;
}

/*
 * The sync_entry function is used to decide place of the file record
 */
int
sync_entry(char *type)
{
    int		i;

    for (i = 0; i < syncent; i++) {
	if (!strcmp(synctype[i], type)) return i;
    }
    return -1;
}
