/* scan.c - a lexical scanner for eTalk */
/*
        Copyright (c) 1988, by David Michael Betz
        All rights reserved
*/

#include <string.h>
#include <setjmp.h>
#include "etalk.h"

/* useful definitions */
#define mapupper(ch)    (islower(ch) ? toupper(ch) : ch)
#define LSIZE   200

/* keyword table */
struct { char *kt_keyword; int kt_token; } ktab[] = {
{ "FUNCTION",   T_FUNCTION      },
{ "CLASS",      T_CLASS         },
{ "IVARS",      T_IVARS         },
{ "CVARS",      T_CVARS         },
{ "METHOD",     T_METHOD        },
{ "CMETHOD",    T_CMETHOD       },
{ "END",        T_END           },
{ "IF",         T_IF            },
{ "THEN",       T_THEN          },
{ "ELSE",       T_ELSE          },
{ "WHILE",      T_WHILE         },
{ "DO",         T_DO            },
{ "RETURN",     T_RETURN        },
{ "SUPER",      T_SUPER         },
{ NULL,         0               }};

/* token name table */
char *t_names[] = {
"<EOF>",
"<STRING>",
"<IDENTIFIER>",
"<NUMBER>",
"FUNCTION",
"CLASS",
"IVARS",
"CVARS",
"METHOD",
"CMETHOD",
"END",
"IF",
"THEN",
"ELSE",
"WHILE",
"DO",
"RETURN",
":=",
"SUPER"};

/* global variables */
int t_value;            /* numeric value */
char t_token[TKNSIZE+1];/* token string */

/* external variables */
extern FILE *ifp;       /* input file pointer */

/* local variables */
static int savetkn;     /* look ahead token */
static int savech;      /* look ahead character */
static int lastch;      /* last input character */
static char line[LSIZE];/* last input line */
static char *lptr;      /* line pointer */
static int lnum;        /* line number */

static int rtoken(void);
static int getstring(void);
static int getid(int ch);
static int getnumber(int ch);
static int skipspaces(void);
static int isidchar(int ch);
static int getch(void);

/* sinit - initialize the scanner */
void sinit(void)
{
    /* setup the line buffer */
    lptr = line; *lptr = '\0';
    lnum = 0;

    /* no lookahead yet */
    savetkn = T_NOTOKEN;
    savech = '\0';

    /* no last character */
    lastch = '\0';
}

/* token - get the next token */
int token(void)
{
    int tkn;

    if ((tkn = savetkn) != T_NOTOKEN)
        savetkn = T_NOTOKEN;
    else
        tkn = rtoken();
    return (tkn);
}

/* stoken - save a token */
void stoken(int tkn)
{
    savetkn = tkn;
}

/* tkn_name - get the name of a token */
char *tkn_name(int tkn)
{
    static char tname[2];
    if (tkn < ' ')
        return (t_names[tkn]);
    tname[0] = tkn;
    tname[1] = '\0';
    return (tname);
}

/* rtoken - read the next token */
static int rtoken(void)
{
    int ch;

    /* check the next character */
    for (;;)
        switch (ch = skipspaces()) {
        case EOF:       return (T_EOF);
        case '"':       return (getstring());
        case ':':       if ((ch = getch()) == '=')
                            return (T_ASSIGN);
                        else {
                            savech = ch;
                            return (':');
                        }
        default:        if (isdigit(ch))
                            return (getnumber(ch));
                        else if (isidchar(ch))
                            return (getid(ch));
                        else {
                            t_token[0] = ch;
                            t_token[1] = '\0';
                            return (ch);
                        }
        }
}

/* getstring - get a string */
static int getstring(void)
{
    char *p;
    int ch;

    /* get the string */
    p = t_token;
    while ((ch = getch()) != EOF && ch != '"')
        *p++ = ch;
    if (ch == EOF)
        savech = EOF;
    *p = '\0';
    return (T_STRING);
}

/* getid - get an identifier */
static int getid(int ch)
{
    char *p;
    int i;

    /* get the identifier */
    p = t_token; *p++ = mapupper(ch);
    while ((ch = getch()) != EOF && isidchar(ch))
        *p++ = mapupper(ch);

    /* allow ':' to terminate identifiers */
    if (ch == ':') *p++ = ch;
    else savech = ch;
    *p = '\0';

    /* check to see if it is a keyword */
    for (i = 0; ktab[i].kt_keyword != NULL; ++i)
        if (strcmp(ktab[i].kt_keyword,t_token) == 0)
            return (ktab[i].kt_token);
    return (T_IDENTIFIER);
}

/* getnumber - get a number */
static int getnumber(int ch)
{
    char *p;

    /* get the number */
    p = t_token; *p++ = ch; t_value = ch - '0';
    while ((ch = getch()) != EOF && isdigit(ch)) {
        t_value = t_value * 10 + ch - '0';
        *p++ = ch;
    }
    savech = ch;
    *p = '\0';
    return (T_NUMBER);
}

/* skipspaces - skip leading spaces */
static int skipspaces(void)
{
    int ch;
    while ((ch = getch()) && isspace(ch))
        ;
    return (ch);
}

/* isidchar - is this an identifier character */
static int isidchar(int ch)
{
    return (isupper(ch)
         || islower(ch)
         || isdigit(ch)
         || ch == '_');
}

/* getch - get the next character */
static int getch(void)
{
    int ch;
    
    /* check for a lookahead character */
    if ((ch = savech) != '\0')
        savech = '\0';

    /* check for a buffered character */
    else {
        while ((ch = *lptr++) == '\0') {

            /* check for being at the end of file */
            if (lastch == EOF)
                return (EOF);

            /* read the next line */
            lptr = line;
            while ((lastch = getc(ifp)) != EOF && lastch != '\n')
                *lptr++ = lastch;
            *lptr++ = '\n'; *lptr = '\0';
            lptr = line;
            ++lnum;
        }
    }

    /* return the current character */
    return (ch);
}

/* parse_error - report an error in the current line */
void parse_error(char *msg)
{
    extern jmp_buf error_trap;
    char buf[LSIZE],*src,*dst;

    /* redisplay the line with the error */
    sprintf(buf,">>> %s <<<\n>>> in line %d <<<\n%s",msg,lnum,line);
    osputs(buf);

    /* point to the position immediately following the error */
    for (src = line, dst = buf; src < lptr; ++src)
        *dst++ = (*src == '\t' ? '\t' : ' ');
    *dst++ = '^'; *dst++ = '\n'; *dst = '\0';
    osputs(buf);

    /* invoke the error trap */
    longjmp(error_trap,1);
}
