/* main.c - the eTalk main routine */
/*
        Copyright (c) 1988, by David Michael Betz
        All rights reserved
*/

#include <stdio.h>
#include "etalk.h"

#define BANNER  "eTalk v1.1 - Copyright (c) 1988, by David Betz"

/* main - the main routine */
int main(int argc, char *argv[])
{
    int i;

    /* display the banner */
    osputs(BANNER);
    osputs("\n");

    /* initialize */
    initialize(SMAX,CMAX);

    /* load and execute some code */
    printf("\nCompiling\n");
    for (i = 1; i < argc; ++i)
        compile_file(argv[i]);
    printf("\nRunning\n");
    execute("MAIN");
    
    return 0;
}

void osputs(const char *str)
{
    fputs(str,stdout);
}

