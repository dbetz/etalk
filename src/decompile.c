/* decompile.c - eTalk decompiler */

#include "etalk.h"

/* instruction output formats */
#define FMT_NONE        0
#define FMT_BYTE        1
#define FMT_WORD        2

typedef struct { int ot_code; char *ot_name; int ot_fmt; } OTDEF;
OTDEF otab[] = {
{       OP_BRT,         "BRT",          FMT_WORD        },
{       OP_BRF,         "BRF",          FMT_WORD        },
{       OP_BR,          "BR",           FMT_WORD        },
{       OP_LIT,         "LIT",          FMT_BYTE        },
{       OP_VAR,         "VAR",          FMT_BYTE        },
{       OP_VSET,        "VSET",         FMT_BYTE        },
{       OP_ARG,         "ARG",          FMT_BYTE        },
{       OP_ASET,        "ASET",         FMT_BYTE        },
{       OP_TMP,         "TMP",          FMT_BYTE        },
{       OP_TSET,        "TSET",         FMT_BYTE        },
{       OP_IVAR,        "IVAR",         FMT_BYTE        },
{       OP_IVSET,       "IVSET",        FMT_BYTE        },
{       OP_CALL,        "CALL",         FMT_BYTE        },
{       OP_RETURN,      "RETURN",       FMT_NONE        },
{       OP_SEND,        "SEND",         FMT_BYTE        },
{       OP_SENDSUPER,   "SENDSUPER",    FMT_BYTE        },
{       OP_TSPACE,      "TSPACE",       FMT_BYTE        },
{       OP_TRUE,        "TRUE",         FMT_NONE        },
{       OP_FALSE,       "FALSE",        FMT_NONE        },
{       OP_PUSH,        "PUSH",         FMT_NONE        },
{       OP_NOT,         "NOT",          FMT_NONE        },
{       OP_ADD,         "ADD",          FMT_NONE        },
{       OP_SUB,         "SUB",          FMT_NONE        },
{       OP_MUL,         "MUL",          FMT_NONE        },
{       OP_DIV,         "DIV",          FMT_NONE        },
{       OP_REM,         "REM",          FMT_NONE        },
{       OP_BAND,        "BAND",         FMT_NONE        },
{       OP_BOR,         "BOR",          FMT_NONE        },
{       OP_BNOT,        "BNOT",         FMT_NONE        },
{       OP_LT,          "LT",           FMT_NONE        },
{       OP_EQ,          "EQ",           FMT_NONE        },
{       OP_GT,          "GT",           FMT_NONE        },
{0,0,0}
};

/* decode_procedure - decode the instructions in a code object */
void decode_procedure(VALUE *code)
{
    int len,lc,n;
    len = code[1].v.v_string->s_length;
    for (lc = 0; lc < len; lc += n)
        n = decode_instruction(code,lc);
}

/* decode_instruction - decode a single bytecode instruction */
int decode_instruction(VALUE *code, int lc)
{
    unsigned char *cp;
    char buf[100];
    OTDEF *op;
    int n=1;

    /* get a pointer to the bytecodes for this instruction */
    cp = code[1].v.v_string->s_data + lc;

    /* show the address and opcode */
    sprintf(buf,"%04x %02x ",lc,*cp);
    osputs(buf);

    /* display the operands */
    for (op = otab; op->ot_name; ++op)
        if (*cp == op->ot_code) {
            switch (op->ot_fmt) {
            case FMT_NONE:
                sprintf(buf,"      %s\n",op->ot_name);
                osputs(buf);
                break;
            case FMT_BYTE:
                sprintf(buf,"%02x    %s %02x\n",cp[1],op->ot_name,cp[1]);
                osputs(buf);
                n += 1;
                break;
            case FMT_WORD:
                sprintf(buf,"%02x %02x %s %02x%02x\n",cp[1],cp[2],
                        op->ot_name,cp[2],cp[1]);
                osputs(buf);
                n += 2;
                break;
            }
            return (n);
        }
    
    /* unknown opcode */
    sprintf(buf,"      <UNKNOWN>\n");
    osputs(buf);
    return 1;
}
