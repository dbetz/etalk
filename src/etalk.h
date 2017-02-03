/* etalk.h - etalk definitions */
/*
        Copyright (c) 1988, by David Michael Betz
        All rights reserved
*/

#ifndef __ETALK__
#define __ETALK__

#include <stdio.h>
#include <ctype.h>

/* limits */
#define TKNSIZE         50      /* maximum token size */
#define SMAX            500     /* runtime stack size */
#define CMAX            32767   /* code buffer size */

/* useful definitions */
#define TRUE            1
#define FALSE           0

/* token definitions */
#define T_NOTOKEN       -1
#define T_EOF           0
#define T_STRING        1
#define T_IDENTIFIER    2
#define T_NUMBER        3
#define T_FUNCTION      4
#define T_CLASS         5
#define T_IVARS         6
#define T_CVARS         7
#define T_METHOD        8
#define T_CMETHOD       9
#define T_END           10
#define T_IF            11
#define T_THEN          12
#define T_ELSE          13
#define T_WHILE         14
#define T_DO            15
#define T_RETURN        16
#define T_ASSIGN        17
#define T_SUPER         18

/* stack manipulation macros */
#define check(n)        { if (sp - (n) < stkbase) stackover(); }
#define chktype(o,t)    { if (sp[o].v_type != t) badtype(o,t); }
#define push_integer(x) ( --sp,\
                          sp->v_type = DT_INTEGER,\
                          sp->v.v_integer = (x) )
#define push_object(x)  ( --sp,\
                          sp->v_type = DT_OBJECT,\
                          sp->v.v_object = (x) )
#define push_bytecode(x) ( --sp,\
                          sp->v_type = DT_CODE,\
                          sp->v.v_bytecode = (x) )
#define push_boolean(x) ( --sp,\
                          sp->v_type = DT_BOOLEAN,\
                          sp->v.v_boolean = (x) )
#define set_boolean(s,x) ( (s)->v_type = DT_BOOLEAN,\
                          (s)->v.v_boolean = (x) )
#define set_string(s,x) ( (s)->v_type = DT_STRING,\
                          (s)->v.v_string = (x) )

/* string value structure */
typedef struct string {
  int s_length;                 /* length */
  unsigned char s_data[1];      /* data */
} STRING;

/* value descriptor structure */
typedef struct value {
  int v_type;                   /* data type */
  union {                       /* value */
    struct value *v_object;
    struct value *v_vector;
    struct string *v_string;
    struct value *v_bytecode;
    void (*v_code)();
    long v_integer;
    int v_boolean;
  } v;
} VALUE;

/* object fields */
#define OB_CLASS        0       /* class */
#define _OBSIZE         1

/* 'Class' object fields */
#define CL_SUPER        1       /* superclass */
#define CL_IVARS        2       /* vector of instance variable names */
#define CL_CVARS        3       /* vector of class variable names */
#define CL_ISIZE        4       /* instance size */
#define CL_METHODS      5       /* dictionary of selector/method pairs */
#define _CLSIZE         6

/* 'Dictionary' object fields */
#define DI_CONTENTS     1       /* dictionary contents */
#define _DISIZE         2

/* 'Dictionary_entry' object fields */
#define DE_KEY          1       /* key for this entry */
#define DE_VALUE        2       /* value of this entry */
#define DE_NEXT         3       /* next entry */
#define _DESIZE         4

/* data types */
#define DT_NIL          0
#define DT_OBJECT       1
#define DT_VECTOR       2
#define DT_INTEGER      3
#define DT_STRING       4
#define DT_BOOLEAN      5
#define DT_BYTECODE     6
#define DT_CODE         7

/* boolean values */
#define BV_TRUE         1
#define BV_FALSE        0

/* function argument structure */
typedef struct argument {
    char *arg_name;             /* argument name */
    struct argument *arg_next;  /* next argument */
} ARGUMENT;

/* literal structure */
typedef struct literal {
    VALUE lit_value;            /* literal value */
    struct literal *lit_next;   /* next literal */
} LITERAL;

/* opcodes */
#define OP_BRT          0x01    /* branch on true */
#define OP_BRF          0x02    /* branch on false */
#define OP_BR           0x03    /* branch unconditionally */
#define OP_TRUE         0x04    /* load top of stack with true */
#define OP_FALSE        0x05    /* load top of stack with false */
#define OP_PUSH         0x06    /* push nil onto stack */
#define OP_NOT          0x07    /* logical negate top of stack */
#define OP_ADD          0x08    /* add top two stack entries */
#define OP_SUB          0x09    /* subtract top two stack entries */
#define OP_MUL          0x0A    /* multiply top two stack entries */
#define OP_DIV          0x0B    /* divide top two stack entries */
#define OP_REM          0x0C    /* remainder of top two stack entries */
#define OP_BAND         0x0D    /* bitwise and of top two stack entries */
#define OP_BOR          0x0E    /* bitwise or of top two stack entries */
#define OP_BNOT         0x0F    /* bitwise not of top two stack entries */
#define OP_SHL          0x10    /* shift left top two stack entries */
#define OP_SHR          0x11    /* shift right top two stack entries */
#define OP_LT           0x12    /* less than */
#define OP_EQ           0x13    /* equal to */
#define OP_GT           0x14    /* greater than */
#define OP_LIT          0x15    /* load literal */
#define OP_VAR          0x16    /* load a variable value */
#define OP_VSET         0x17    /* set the value of a variable */
#define OP_RETURN       0x1F    /* return from interpreter */
#define OP_CALL         0x20    /* call a function */
#define OP_ARG          0x28    /* load an argument value */
#define OP_ASET         0x29    /* set an argument value */
#define OP_TMP          0x2A    /* load a temporary variable value */
#define OP_TSET         0x2B    /* set a temporary variable */
#define OP_TSPACE       0x2C    /* allocate temporary variable space */
#define OP_SEND         0x33    /* send a message to an object */
#define OP_SENDSUPER    0x34    /* send a message to superclass of self */
#define OP_IVAR         0x35    /* load an instance variable value */
#define OP_IVSET        0x36    /* set an instance variable */

/* external variables */
extern VALUE *stkbase,*sp,*fp,*stktop;

/* external routines */
extern VALUE *newclass();
extern VALUE *newobject();
extern VALUE *newvector();
extern STRING *newstring();
extern STRING *makestring();

/* from compile.c */
void add_function(char *name, void (*fcn)());
int compile_file(char *fname);
VALUE *senter(VALUE *table, char *name);
VALUE *sfind(VALUE *table, char *name);

/* from decompile.c */
void decode_procedure(VALUE *code);
int decode_instruction(VALUE *code, int lc);

/* from scan.c */
void sinit(void);
int token(void);
void stoken(int tkn);
void parse_error(char *msg);

/* from interpret.c */
void interpret(VALUE *fcn);
void badtype(int off, int type);
void stackover(void);

/* from memory.c */
void initialize(int smax, int cmax);
int execute(char *name);
VALUE *newclass(VALUE *superclass, int ivcnt);
void addivar(VALUE *class, char *ivar, int n);
void addcvar(VALUE *class, char *cvar);
void addmethod(VALUE *class, char *selector, void (*fcn)());
void addcmethod(VALUE *class, char *selector, void (*fcn)());
VALUE *dict_new(void);
VALUE *dict_add(VALUE *dict, char *key, VALUE *value);
VALUE *dict_find(VALUE *dict, char *key);
STRING *makestring(char *str);
STRING *newstring(int n);
VALUE *newobject(VALUE *class);
VALUE *newvector(int n);

/* from methods.c */
void obj_class(int argc);
void cls_new(int argc);
void fcn_print(int argc);

/* from error.c */
void info(const char *fmt, ...);
void error(const char *fmt, ...);

/* os interface */
void osputs(const char *str);

#endif
