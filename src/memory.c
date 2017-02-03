/* memory.c - the eTalk memory manager */
/*
        Copyright (c) 1988, by David Michael Betz
        All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "etalk.h"

/* external variables */
extern jmp_buf error_trap;      /* the error trap target */
extern unsigned char *cbuff;    /* code buffer */

/* global variables */
VALUE *symbols;         /* the symbol table */
VALUE *class;           /* the 'Class' object */
VALUE *object;          /* the 'Object' object */
VALUE *dictionary;      /* the 'Dictionary' object */
VALUE *dict_entry;      /* the 'Dictionary_entry' object */

static VALUE *allocvector(int n);

/* initialize - initialize the virtual machine */
void initialize(int smax, int cmax)
{
    VALUE *objclass;
    
    /* setup an error trap handler */
    if (setjmp(error_trap) != 0)
        return;

    /* allocate the stack */
    if ((stkbase = (VALUE *)malloc(smax*sizeof(VALUE))) == NULL)
        error("Insufficient memory - stack");
    stktop = sp = stkbase + smax;

    /* allocate the code buffer */
    if ((cbuff = (unsigned char *)malloc(cmax)) == NULL)
        error("Insufficient memory - code buffer");

    /* create the 'Class' object */
    class = allocvector(_CLSIZE);
    class[OB_CLASS].v_type = DT_OBJECT;
    class[OB_CLASS].v.v_object = class;
    class[CL_IVARS].v_type = DT_VECTOR;
    class[CL_IVARS].v.v_vector = newvector(_CLSIZE-1);
    addivar(class,"SUPERCLASS",5);
    addivar(class,"IVARS",4);
    addivar(class,"CVARS",3);
    addivar(class,"ISIZE",2);
    addivar(class,"METHODS",1);
    class[CL_ISIZE].v_type = DT_INTEGER;
    class[CL_ISIZE].v.v_integer = _CLSIZE;

    /* create the '[Object class]' object */
    objclass = newobject(class);
    objclass[CL_SUPER].v_type = DT_OBJECT;
    objclass[CL_SUPER].v.v_object = class;
    objclass[CL_IVARS].v_type = DT_VECTOR;
    objclass[CL_IVARS].v.v_vector = newvector(0);
    objclass[CL_ISIZE].v_type = DT_INTEGER;
    objclass[CL_ISIZE].v.v_integer = _CLSIZE;

    /* create the 'Object' object */
    object = newobject(objclass);
    object[CL_SUPER].v_type = DT_NIL;
    object[CL_IVARS].v_type = DT_VECTOR;
    object[CL_IVARS].v.v_vector = newvector(0);
    object[CL_ISIZE].v_type = DT_INTEGER;
    object[CL_ISIZE].v.v_integer = _OBSIZE;

    /* fixup the superclass of 'Class' */
    class[CL_SUPER].v_type = DT_OBJECT;
    class[CL_SUPER].v.v_object = object;

    /* create the 'Dictionary' object */
    dictionary = newclass(object,_DISIZE-1);
    addivar(dictionary,"CONTENTS",1);

    /* fixup the class of the dictionaries used by 'Dictionary' */
    dictionary[CL_CVARS].v.v_object[OB_CLASS].v.v_object = dictionary;
    dictionary[CL_METHODS].v.v_object[OB_CLASS].v.v_object = dictionary;
    dictionary[OB_CLASS].v.v_object[CL_METHODS].v.v_object[OB_CLASS].v.v_object = dictionary;

    /* create the 'Dictionary_entry' object */
    dict_entry = newclass(object,_DESIZE-1);
    addivar(dict_entry,"KEY",3);
    addivar(dict_entry,"VALUE",2);
    addivar(dict_entry,"NEXT",1);

    /* add 'Class' class variable dictionary and methods */
    class[CL_CVARS].v_type = DT_OBJECT;
    class[CL_CVARS].v.v_object = dict_new();
    class[CL_METHODS].v_type = DT_OBJECT;
    class[CL_METHODS].v.v_object = dict_new();
    addmethod(class,"NEW",cls_new);

    /* add '[Object class]' class variable dictionary and methods */
    objclass[CL_CVARS].v_type = DT_OBJECT;
    objclass[CL_CVARS].v.v_object = dict_new();
    objclass[CL_METHODS].v_type = DT_OBJECT;
    objclass[CL_METHODS].v.v_object = dict_new();

    /* add 'Object' class variable dictionary and methods */
    object[CL_CVARS] = objclass[CL_CVARS]; /* share with metaclass */
    object[CL_METHODS].v_type = DT_OBJECT;
    object[CL_METHODS].v.v_object = dict_new();
    addmethod(object,"CLASS",obj_class);

    /* create the symbol table */
    symbols = dict_new();

    /* enter the base classes */
    dict_add(symbols,"OBJECT",object);
    dict_add(symbols,"CLASS",class);
    dict_add(symbols,"DICTIONARY",dictionary);
    dict_add(symbols,"DICTIONARYENTRY",dict_entry);

    /* enter the built-in functions */
    add_function("PRINT",fcn_print);
}

/* execute - execute eTalk bytecode */
int execute(char *name)
{
    VALUE *sym;
    
    /* setup an error trap handler */
    if (setjmp(error_trap) != 0)
        return (FALSE);

    /* lookup the symbol */
    if ((sym = sfind(symbols,name)) == NULL)
        error("Undefined function: '%s'",name);

    /* dispatch on its data type */
    switch (sym[DE_VALUE].v_type) {
    case DT_CODE:
        (*sym[DE_VALUE].v.v_code)();
        break;
    case DT_BYTECODE:
        interpret(sym[DE_VALUE].v.v_bytecode);
        break;
    }
    return (TRUE);
}

/* newclass - create a new class and metaclass */
VALUE *newclass(VALUE *superclass, int ivcnt)
{
    VALUE *supermeta,*metaclass,*theclass;

    /* make sure there is enough stack space */
    check(2);

    /* get the metaclass of the superclass */
    supermeta = superclass[OB_CLASS].v.v_object;

    /* create the metaclass object */
    metaclass = newobject(class); push_object(metaclass);
    metaclass[CL_SUPER].v_type = DT_OBJECT;
    metaclass[CL_SUPER].v.v_object = superclass[OB_CLASS].v.v_object;
    metaclass[CL_ISIZE].v_type = DT_INTEGER;
    metaclass[CL_ISIZE].v.v_integer = supermeta[CL_ISIZE].v.v_integer;
    metaclass[CL_METHODS].v_type = DT_OBJECT;
    metaclass[CL_METHODS].v.v_object = dict_new();
    
    /* allocate space for the instance variable names */
    metaclass[CL_IVARS].v_type = DT_VECTOR;
    metaclass[CL_IVARS].v.v_vector = newvector(0);

    /* allocate a class variable dictionary */
    metaclass[CL_CVARS].v_type = DT_OBJECT;
    metaclass[CL_CVARS].v.v_object = dict_new();
    
    /* create the class object */
    theclass = newobject(metaclass); push_object(theclass);
    theclass[CL_SUPER].v_type = DT_OBJECT;
    theclass[CL_SUPER].v.v_object = superclass;
    theclass[CL_ISIZE].v_type = DT_INTEGER;
    theclass[CL_ISIZE].v.v_integer = superclass[CL_ISIZE].v.v_integer + ivcnt;
    theclass[CL_METHODS].v_type = DT_OBJECT;
    theclass[CL_METHODS].v.v_object = dict_new();

    /* allocate space for the instance variable names */
    theclass[CL_IVARS].v_type = DT_VECTOR;
    theclass[CL_IVARS].v.v_vector = newvector(ivcnt);
    
    /* use the same class variable dictionary as the metaclass */
    theclass[CL_CVARS] = metaclass[CL_CVARS];
    
    /* clean the stack */
    sp += 2;

    /* return the new class */
    return (theclass);
}

/* addivar - add an instance variable to a class */
void addivar(VALUE *class, char *ivar, int n)
{
    VALUE *v;
    v = class[CL_IVARS].v.v_vector;
    v[n].v_type = DT_STRING;
    v[n].v.v_string = makestring(ivar);
}

/* addcvar - add a class variable to a class */
void addcvar(VALUE *class, char *cvar)
{
    dict_add(class[CL_CVARS].v.v_object,cvar,NULL);
}

/* addmethod - add a method to a class */
void addmethod(VALUE *class, char *selector, void (*fcn)())
{
    VALUE value;
    value.v_type = DT_CODE;
    value.v.v_code = fcn;
    dict_add(class[CL_METHODS].v.v_object,selector,&value);
}

/* addcmethod - add a class method to a class */
void addcmethod(VALUE *class, char *selector, void (*fcn)())
{
    addmethod(class[CL_SUPER].v.v_object,selector,fcn);
}

/* dict_new - create a new method dictionary */
VALUE *dict_new(void)
{
    VALUE *obj;
    obj = allocvector(_DISIZE);
    obj[OB_CLASS].v_type = DT_OBJECT;
    obj[OB_CLASS].v.v_object = dictionary;
    return (obj);
}

/* dict_add - add an entry to a dictionary */
VALUE *dict_add(VALUE *dict, char *key, VALUE *value)
{
    VALUE *obj;
    if ((obj = dict_find(dict,key)) == NULL) {
        obj = newobject(dict_entry);
        obj[DE_KEY].v_type = DT_STRING;
        obj[DE_KEY].v.v_string = makestring(key);
        obj[DE_NEXT] = dict[DI_CONTENTS];
        dict[DI_CONTENTS].v_type = DT_OBJECT;
        dict[DI_CONTENTS].v.v_object = obj;
    }
    if (value)
        obj[DE_VALUE] = *value;
    return (obj);
}

/* dict_find - find an entry in a dictionary */
VALUE *dict_find(VALUE *dict, char *key)
{
    VALUE *entry;
    if (dict[DI_CONTENTS].v_type != DT_NIL) {
        entry = dict[DI_CONTENTS].v.v_object;
        for (;;) {
            if (strcmp(key,(char *)entry[DE_KEY].v.v_string->s_data) == 0)
                return (entry);
            if (entry[DE_NEXT].v_type == DT_NIL)
                break;
            entry = entry[DE_NEXT].v.v_object;
        }
    }
    return (NULL);
}

/* makestring - make an initialized string */
STRING *makestring(char *str)
{
    STRING *val;
    val = newstring(strlen(str)+1);
    strcpy((char *)val->s_data,str);
    return (val);
}

/* newstring - allocate a new string object */
STRING *newstring(int n)
{
    STRING *val;
    if ((val = (STRING *)malloc(sizeof(STRING)+n-1)) == NULL)
        error("Insufficient memory");
    val->s_length = n;
    return (val);
}

/* newobject - allocate a new object */
VALUE *newobject(VALUE *class)
{
    VALUE *val;
    val = allocvector((int)class[CL_ISIZE].v.v_integer);
    val[OB_CLASS].v_type = DT_OBJECT;
    val[OB_CLASS].v.v_object = class;
    return (val);
}

/* newvector - allocate a new vector */
VALUE *newvector(int n)
{
    VALUE *val;
    val = allocvector(n+1);
    val[0].v_type = DT_INTEGER;
    val[0].v.v_integer = n;
    return (val);
}

/* allocvector - allocate a new vector */
static VALUE *allocvector(int n)
{
    VALUE *val,*p;
    if ((val = (VALUE *)malloc(n * sizeof(VALUE))) == NULL)
        error("Insufficient memory");
    for (p = val; --n >= 0; ++p)
        p->v_type = DT_NIL;
    return (val);
}
