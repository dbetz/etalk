/* error.c - the eTalk error handlers */
/*
        Copyright (c) 1988, by David Michael Betz
        All rights reserved
*/

#include <stdarg.h>
#include <setjmp.h>
#include "etalk.h"

/* global variables */
jmp_buf error_trap;

/* external variables */
extern char *lptr,line[];
extern int lnum;

/* info - display progress information */
void info(const char *fmt, ...)
{
    char buf1[100],buf2[100];
    va_list ap;
    va_start(ap,fmt);
    vsprintf(buf1,fmt,ap);
    va_end(ap);
    sprintf(buf2,"[ %s ]\n",buf1);
    osputs(buf2);
}

/* error - print an error message and exit */
void error(const char *fmt, ...)
{
    char buf1[100],buf2[100];
    va_list ap;
    va_start(ap,fmt);
    vsprintf(buf1,fmt,ap);
    va_end(ap);
    sprintf(buf2,"Error: %s\n",buf1);
    osputs(buf2);
    longjmp(error_trap,1);
}
