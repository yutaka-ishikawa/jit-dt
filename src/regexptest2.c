#if 0
#
# Kobe
#	kobe_20161209171230_A08_pawr_ze.dat
# Osaka
#	20150806-140030.all.10000000.dat
#	20150806-140030.all.20000000.dat
#	20150806-140030.all_pawr_qcf.dat

kobe {
    pattern = ".*_\\(.*\\)_A08_pawr_\\(.*\\).dat";
    date = "$1";
    type = "$2";
}

osaka {
    pattern = "\\([0-9].*\\)-\\([0-9].*\\).all.1\\([0-9].*\\).dat";
    date = "$1$2";
    type = "ze";
}

osaka {
    pattern = "\\([0-9].*\\)-\\([0-9].*\\).all.2\\([0-9].*\\).dat";
    date = "$1$2";
    type = "vr";
}

osaka {
    pattern = "\\([0-9].*\\)-\\([0-9].*\\).all_pawr_qcf.dat";
    date = "$1$2";
    type = "qcf";
}
#endif

/*
 *	testing regular expression
 */
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>

char
skipblank(FILE *fp)
{
    char	ch;
retry:
    while ((ch = getc(fp)) != EOF) if (!isspace(ch)) break;
    if (ch == '#') {
	while ((ch = getc(fp)) != EOF) if (ch == '\n') break;
	goto retry;
    }
    return ch;
}

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
#define TKN_KEY_PAT	8
#define TKN_KEY_DAT	9
#define TKN_KEY_TYP	10
#define IS_ALPHA(ch)	(isalpha(ch) || ch == '-' || ch == '_')

char	*key[] = { "pattern", "date", "type", 0};

struct conf {
    char	c_area[STR_LEN];
    char	c_pattern[STR_LEN];
    char	c_name[STR_LEN];
    char	c_type[STR_LEN];
    char	c_date[STR_LEN];
    regex_t	c_preg;
};

#define MAX_CONF	40
static struct conf conf[MAX_CONF+1];
static confent;

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

#define PARSE_KEYSTRING(type) {				\
    tkn = gettoken(fp, buf, &val);			\
    ntkn = gettoken(fp, buf, &val);			\
    if (tkn != TKN_EQU && ntkn != TKN_STR) goto err1;	\
    strcpy(conf[ent].c_##type, buf);			\
}

void
readconf(char *fname)
{
    FILE	*fp;
    char	buf[STR_LEN];
    int		tkn, ntkn, val;
    int		ent;
    if ((fp = fopen(fname, "r")) == NULL) {
	fprintf(stderr, "Cannot open a configuration file:%s\n", fname);
	exit(-1);
    }
    ent = 0;
    while ((tkn = gettoken(fp, buf, &val)) > TKN_OK) {
	ntkn = gettoken(fp, buf, &val);
	if (tkn != TKN_STR && ntkn != TKN_BGN) goto err1;
	strcpy(conf[ent].c_area, buf);
	while ((tkn = gettoken(fp, buf, &val)) > TKN_OK) {
	    switch (tkn) {
	    case TKN_END:
		goto next;
	    case TKN_KEY_PAT:
		PARSE_KEYSTRING(pattern); break;
	    case TKN_KEY_DAT:
		PARSE_KEYSTRING(date); 	break;
	    case TKN_KEY_TYP:
		PARSE_KEYSTRING(type);	break;
	    default:
		goto err1;
	    }
	}
    next:
	if (++ent >= MAX_CONF) goto err2;
    }
    confent = ent;
    return;
err1:
	fprintf(stderr, "configuration: format error\n"); exit(-1);
err2:
	fprintf(stderr, "configuration: too many areas\n"); exit(-1);
}


char	errbuf[1024];
char	*tpattern[] = {
    "/home/ishikawa/tmp/tmp/kobe_20161208110200_A08_pawr_vr.dat",
    "kobe_20161209171230_A08_pawr_ze.dat",
    "asdasasdasd",
    "20150806-140030.all.10000000.dat",
    "20150806-140030.all.20000000.dat",
    "20150806-140030.all_pawr_qcf.dat"
};

void
regex_init()
{
    int		i, cc;

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

#define NMATCH	10
int
regex_match(char *pattern, char *date, char *type)
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

    matchedcopy(date, cnfp->c_date, pattern, pmatch);
    matchedcopy(type, cnfp->c_type, pattern, pmatch);
#if 0
    i = 0;
    for (cp = cnfp->c_date; *cp; cp++) {
	if (*cp == '$') { /* position */
	    pos = *++cp - '0';
	    len = pmatch[pos].rm_eo - pmatch[pos].rm_so;
	    strncpy(&date[i], &pattern[pmatch[pos].rm_so], len);
	    i += len;
	} else {
	    date[i++] = *cp;
	}
    }
    date[i] = 0;
    len = pmatch[1].rm_eo - pmatch[1].rm_so;
    strncpy(date, &pattern[pmatch[1].rm_so], len);
    len = pmatch[2].rm_eo - pmatch[2].rm_so;
    strncpy(type, &pattern[pmatch[2].rm_so], len);
    type[len] = 0; /* terminating for string */
#endif
    return 1;
}

int
main(int argc, char **argv)
{
    int		cc, i, j;
    char	date[128], type[128];

    readconf("./conf");
    i = 0;
    regex_init();
    while(tpattern[i]) {
	if (regex_match(tpattern[i], date, type) > 0) {
	    printf("match: date(%s) type(%s)\n", date, type);
	} else {
	    printf("no match\n");
	}
	i++;
    }
    return 0;
}
