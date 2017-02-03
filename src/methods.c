/* methods.c - methods for built-in classes and built-in functions */
/*
        Copyright (c) 1988, by David Michael Betz
        All rights reserved
*/

#include "etalk.h"

static void print1(VALUE *val);

/* obj_class - 'Object' method for 'class' */
void obj_class(int argc)
{
    VALUE *obj;
    obj = sp[0].v.v_object[OB_CLASS].v.v_object;
    sp += 1;
    sp->v_type = DT_OBJECT;
    sp->v.v_object = obj;
}

/* cls_new - 'Class' method for 'new' */
void cls_new(int argc)
{
    VALUE *obj;
    obj = newobject(sp[0].v.v_object);
    sp += 1;
    sp->v_type = DT_OBJECT;
    sp->v.v_object = obj;
}

/* fcn_print - generic print function */
void fcn_print(int argc)
{
    while (--argc >= 0)
        print1(&sp[argc]);
}

/* print1 - print one value */
static void print1(VALUE *val)
{
    char buf[200];
    switch (val->v_type) {
    case DT_NIL:
        osputs("Nil");
        break;
    case DT_OBJECT:
        sprintf(buf,"#<Object:%p>",val->v.v_object);
        osputs(buf);
        break;
    case DT_VECTOR:
        sprintf(buf,"#<Vector:%p>",val->v.v_vector);
        osputs(buf);
        break;
    case DT_INTEGER:
        sprintf(buf,"%ld",val->v.v_integer);
        osputs(buf);
        break;
    case DT_STRING:
        osputs((char *)val->v.v_string->s_data);
        break;
    case DT_BOOLEAN:
        switch (val->v.v_boolean) {
        case BV_TRUE:
            osputs("True");
            break;
        case BV_FALSE:
            osputs("False");
            break;
        default:
            error("Bad boolean value: %d",val->v.v_boolean);
        }
        break;
    case DT_BYTECODE:
        sprintf(buf,"#<ByteCode:%p>",val->v.v_bytecode);
        osputs(buf);
        break;
    case DT_CODE:
        sprintf(buf,"#<Code:%p>",val->v.v_code);
        osputs(buf);
        break;
    default:
        error("Undefined type: %d",val->v_type);
    }
}

