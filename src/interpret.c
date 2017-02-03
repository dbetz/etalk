/* interpret.c - eTalk bytecode interpreter */
/*
        Copyright (c) 1988, by David Michael Betz
        All rights reserved
*/

#include "etalk.h"

/* global variables */
VALUE *stkbase;         /* the runtime stack */
VALUE *stktop;          /* the top of the stack */
VALUE *sp;              /* the stack pointer */
VALUE *fp;              /* the frame pointer */

/* local variables */
static unsigned char *cbase;    /* the base code address */
static unsigned char *pc;       /* the program counter */
static VALUE *code;             /* the current code vector */

static void send(VALUE *class, int n);
static int getwoperand(void);
static void nomethod(char *selector);

/* interpret - interpret bytecode instructions */
void interpret(VALUE *fcn)
{
    register int pcoff,n;
    register VALUE *obj;
    VALUE *topframe,val;
    
    /* initialize */
    sp = fp = stktop;
    cbase = pc = fcn[1].v.v_string->s_data;
    code = fcn;

    /* make a dummy call frame */
    check(4);
    push_bytecode(code);
    push_integer(0);
    push_integer(0);
    push_integer(0);
    fp = topframe = sp;
    
    /* execute each instruction */
    for (;;) {
        //decode_instruction(code,pc-code[1].v.v_string->s_data);
        switch (*pc++) {
        case OP_CALL:
                n = *pc++;
                switch (sp[n].v_type) {
                case DT_CODE:
                    (*sp[n].v.v_code)(n);
                    break;
                case DT_BYTECODE:
                    check(3);
                    code = sp[n].v.v_bytecode;
                    push_integer(n);
                    push_integer(stktop - fp);
                    push_integer(pc - cbase);
                    cbase = pc = code[1].v.v_string->s_data;
                    fp = sp;
                    break;
                default:
                    error("Call to non-procedure, Type %d",sp[n].v_type);
                    return;
                }
                break;
        case OP_RETURN:
                if (fp == topframe) return;
                val = *sp;
                sp = fp;
                pcoff = fp[0].v.v_integer;
                n = fp[2].v.v_integer;
                fp = stktop - fp[1].v.v_integer;
                code = fp[fp[2].v.v_integer+3].v.v_bytecode;
                cbase = code[1].v.v_string->s_data;
                pc = cbase + pcoff;
                sp += n + 3;
                *sp = val;
                break;
        case OP_TSPACE:
                n = *pc++;
                check(n);
                sp -= n;
                break;
        case OP_TMP:
                n = *pc++;
                *sp = fp[-n-1];
                break;
        case OP_TSET:
                n = *pc++;
                fp[-n-1] = *sp;
                break;
        case OP_ARG:
                n = *pc++;
                if (n >= fp[2].v.v_integer)
                    error("Too few arguments");
                *sp = fp[n+3];
                break;
        case OP_ASET:
                n = *pc++;
                if (n >= fp[2].v.v_integer)
                    error("Too few arguments");
                fp[n+3] = *sp;
                break;
        case OP_BRT:
                chktype(0,DT_BOOLEAN);
                if (sp[0].v.v_boolean == BV_TRUE)
                    pc = cbase + getwoperand();
                else
                    pc += 2;
                break;
        case OP_BRF:
                chktype(0,DT_BOOLEAN);
                if (sp[0].v.v_boolean == BV_FALSE)
                    pc = cbase + getwoperand();
                else
                    pc += 2;
                break;
        case OP_BR:
                pc = cbase + getwoperand();
                break;
        case OP_TRUE:
                set_boolean(sp,BV_TRUE);
                break;
        case OP_FALSE:
                set_boolean(sp,BV_FALSE);
                break;
        case OP_PUSH:
                check(1);
                push_boolean(BV_FALSE);
                break;
        case OP_NOT:
                chktype(0,DT_BOOLEAN);
                if (sp[0].v.v_boolean == BV_TRUE)
                    set_boolean(sp,BV_FALSE);
                else
                    set_boolean(sp,BV_TRUE);
                break;
        case OP_ADD:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                sp[1].v.v_integer += sp[0].v.v_integer;
                ++sp;
                break;
        case OP_SUB:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                sp[1].v.v_integer -= sp[0].v.v_integer;
                ++sp;
                break;
        case OP_MUL:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                sp[1].v.v_integer *= sp[0].v.v_integer;
                ++sp;
                break;
        case OP_DIV:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                if (sp[0].v.v_integer != 0)
                    sp[1].v.v_integer /= sp[0].v.v_integer;
                else
                    sp[1].v.v_integer = 0;
                ++sp;
                break;
        case OP_REM:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                if (sp[0].v.v_integer != 0)
                    sp[1].v.v_integer %= sp[0].v.v_integer;
                else
                    sp[1].v.v_integer = 0;
                ++sp;
                break;
        case OP_BAND:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                sp[1].v.v_integer %= sp[0].v.v_integer;
                ++sp;
                break;
        case OP_BOR:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                sp[1].v.v_integer |= sp[0].v.v_integer;
                ++sp;
                break;
        case OP_BNOT:
                chktype(0,DT_INTEGER);
                sp[0].v.v_integer = ~sp[0].v.v_integer;
                break;
        case OP_SHL:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                sp[1].v.v_integer <<= sp[0].v.v_integer;
                ++sp;
                break;
        case OP_SHR:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                sp[1].v.v_integer >>= sp[0].v.v_integer;
                ++sp;
                break;
        case OP_LT:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                if (sp[1].v.v_integer < sp[0].v.v_integer)
                    set_boolean(sp,BV_TRUE);
                else
                    set_boolean(sp,BV_FALSE);
                break;
        case OP_EQ:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                if (sp[1].v.v_integer == sp[0].v.v_integer)
                    set_boolean(sp,BV_TRUE);
                else
                    set_boolean(sp,BV_FALSE);
                break;
        case OP_GT:
                chktype(0,DT_INTEGER);
                chktype(1,DT_INTEGER);
                if (sp[1].v.v_integer > sp[0].v.v_integer)
                    set_boolean(sp,BV_TRUE);
                else
                    set_boolean(sp,BV_FALSE);
                break;
        case OP_LIT:
                *sp = code[*pc++];
                break;
        case OP_VAR:
                *sp = code[*pc++].v.v_object[DE_VALUE];
                break;
        case OP_VSET:
                code[*pc++].v.v_object[DE_VALUE] = *sp;
                break;
        case OP_SEND:
                n = *pc++;
                chktype(n-1,DT_OBJECT);
                send(sp[n-1].v.v_object[OB_CLASS].v.v_object,n);
                break;
        case OP_SENDSUPER:
                n = *pc++;
                chktype(n-1,DT_OBJECT);
                if (code[2].v.v_object[CL_SUPER].v_type != DT_OBJECT)
                    nomethod((char *)sp[n].v.v_string->s_data);
                send(code[2].v.v_object[CL_SUPER].v.v_object,n);
                break;
        case OP_IVAR:
                obj = fp[fp[2].v.v_integer+2].v.v_object;
                *sp = obj[*pc++];
                break;
        case OP_IVSET:
                obj = fp[fp[2].v.v_integer+2].v.v_object;
                obj[*pc++] = *sp;
                break;
        default:
                info("Bad opcode %02x",pc[-1]);
                break;
        }
    }
}

/* send - send a message to an object */
static void send(VALUE *class, int n)
{
    VALUE *val;
    char *selector;
    selector = (char *)sp[n].v.v_string->s_data;
    for (;;) {
        if ((val = dict_find(class[CL_METHODS].v.v_object,selector)) != NULL) {
            switch (val[DE_VALUE].v_type) {
            case DT_CODE:
                (*val[DE_VALUE].v.v_code)();
                return;
            case DT_BYTECODE:
                check(3);
                sp[n].v_type = DT_BYTECODE;
                sp[n].v.v_bytecode = code = val[DE_VALUE].v.v_bytecode;
                push_integer(n);
                push_integer(stktop - fp);
                push_integer(pc - cbase);
                cbase = pc = code[1].v.v_string->s_data;
                fp = sp;
                return;
            default:
                error("Bad method, Selector '%s', Type %d",
                      selector,
                      val[DE_VALUE].v_type);
            }
        }
        if (class[CL_SUPER].v_type == DT_NIL)
            break;
        class = class[CL_SUPER].v.v_object;
    }
    nomethod(selector);
}

/* getwoperand - get data word */
static int getwoperand(void)
{
    int b;
    b = *pc++;
    return ((*pc++ << 8) | b);
}

/* badtype - report a bad operand type */
void badtype(int off, int type)
{
    info("Offset %d, Type %d, Expected %d",off,sp[off].v_type,type);
    error("Bad argument type");
}

/* nomethod - report a failure to find a method for a selector */
static void nomethod(char *selector)
{
    error("No method for selector '%s'",selector);
}

/* stackover - report a stack overflow error */
void stackover(void)
{
    error("Stack overflow");
}

