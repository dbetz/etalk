/* compile.c - the eTalk compiler */
/*
        Copyright (c) 1988, by David Michael Betz
        All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "etalk.h"

/* global variables */
ARGUMENT *arguments;    /* argument list */
ARGUMENT *temporaries;  /* temporary variable list */
VALUE *methodclass;     /* class of the current method */
LITERAL *literals;      /* literal list */
FILE *ifp;              /* input file pointer */

/* code buffer */
unsigned char *cbuff;
int cptr;

/* external variables */
extern jmp_buf error_trap;      /* trap for compile errors */
extern VALUE *symbols;          /* symbol table */
extern VALUE *class;            /* the 'Class' class object */
extern VALUE *object;           /* the 'Object' class object */
extern VALUE *sp;               /* stack pointer */
extern int t_value;             /* token value */
extern char t_token[];          /* token string */

/* forward declarations */
static void do_function(void);
static void do_class(void);
static void do_method(char *type, VALUE *class);
static VALUE *do_code(int endtkn);
static void do_statement(void);
static void do_if(void);
static void do_while(void);
static void do_return(void);
static void do_expr(void);
static void do_expr2(void);
static void do_expr3(void);
static void do_primary(void);
static void do_identifier(void);
static void do_assignment(char *id);
static void do_call(void);
static void do_send(void);
static int get_id_list(ARGUMENT **list, int term);
static void addargument(ARGUMENT **list, char *name);
static void freelist(ARGUMENT **plist);
static int findarg(char *name);
static int findtmp(char *name);
static int findivar(char *name);
static VALUE *findcvar(char *name);
static int addliteral(LITERAL **list, LITERAL **pval);
static void freeliterals(LITERAL **plist);
VALUE *senter(VALUE *table, char *name);
VALUE *sfind(VALUE *table, char *name);
static void frequire(int rtkn);
static void require(int tkn, int rtkn);
static void do_lit_integer(long n);
static void do_lit_string(char *str);
static int make_lit_object(VALUE *obj);
static void code_reference(char *id);
static void code_assignment(char *id);
static void code_argument(int n);
static void code_setargument(int n);
static void code_temporary(int n);
static void code_settemporary(int n);
static void code_ivar(int n);
static void code_setivar(int n);
static void code_variable(int n);
static void code_setvariable(int n);
static void code_literal(int n);
static int putcbyte(int b);
static int putcword(int w);
static void fixup(int chn, int val);
static char *save(char *str);

/* add_function - add a built-in function */
void add_function(char *name, void (*fcn)())
{
    VALUE *senter(),*value;
    value = senter(symbols,name);
    value[DE_VALUE].v_type = DT_CODE;
    value[DE_VALUE].v.v_code = fcn;
}

/* compile_file - compile code from a file */
int compile_file(char *fname)
{
    int tkn;

    /* setup error handler */
    if (setjmp(error_trap) != 0) {
        if (ifp != NULL)
            fclose(ifp);
        return (FALSE);
    }

    /* open the input file */
    if ((ifp = fopen(fname,"r")) == NULL)
        error("Can't open input file: %s",fname);

    /* initialize */
    sinit();

    /* process statements until end of file */
    while ((tkn = token()) != T_EOF) {
        switch (tkn) {
        case T_FUNCTION:
            do_function();
            break;
        case T_CLASS:
            do_class();
            break;
        default:
            parse_error("Expecting a declaration");
            break;
        }
    }

    /* close the input file */
    fclose(ifp);
    return (TRUE);
}

/* do_function - handle function declarations */
static void do_function(void)
{
    VALUE *sym,*code;

    /* get the function name */
    frequire(T_IDENTIFIER);
info("Function '%s'",t_token);
    sym = senter(symbols,t_token);

    /* initialize */
    arguments = temporaries = NULL;
    methodclass = NULL;
    literals = NULL;

    /* get the argument list */
    frequire('(');
    get_id_list(&arguments,')');
    
    /* compile the body of the function */
    code = do_code(T_FUNCTION);

    /* store the function definition */
    sym[DE_VALUE].v_type = DT_BYTECODE;
    sym[DE_VALUE].v.v_bytecode = code;

    /* free the argument and temporary symbol lists */
    freelist(&arguments); freelist(&temporaries);
}

/* do_class - handle class declarations */
static void do_class(void)
{
    char cname[TKNSIZE+1],sname[TKNSIZE+1];
    VALUE *superclass,*theclass,*metaclass;
    ARGUMENT *ivars,*cvars,*p;
    int tkn,ivcnt,cvcnt,i;
    VALUE *cls,*sym;

    /* initialize */
    ivars = cvars = NULL;
    ivcnt = cvcnt = 0;
    
    /* get the class name */
    frequire(T_IDENTIFIER);
    cls = senter(symbols,t_token);
    strcpy(cname,t_token);
    
    /* get the optional superclass */
    if ((tkn = token()) == ':') {
        frequire(T_IDENTIFIER);
        strcpy(sname,t_token);
        sym = sfind(symbols,t_token);
        if (sym != NULL && sym[DE_VALUE].v_type == DT_OBJECT)
            superclass = sym[DE_VALUE].v.v_object;
        else {
            parse_error("Expecting a class");
            strcpy(sname,"Object");
            superclass = object;
        }
    }
    else {
        strcpy(sname,"Object");
        superclass = object;
        stoken(tkn);
    }
info("Class '%s', Superclass '%s'",cname,sname);

    /* handle each variable declaration */
    while ((tkn = token()) == T_IVARS || tkn == T_CVARS) {
        switch (tkn) {
        case T_IVARS:
            ivcnt += get_id_list(&ivars,';');
            break;
        case T_CVARS:
            cvcnt += get_id_list(&cvars,';');
            break;
        }
    }
    stoken(tkn);

    /* make sure there is enough stack space */
    check(1);

    /* create the new class object */
    theclass = newclass(superclass,ivcnt); push_object(theclass);
    metaclass = theclass[OB_CLASS].v.v_object;

    /* store the instance variable names */
    for (i = 1, p = ivars; i <= ivcnt; ++i, p = p->arg_next)
        addivar(theclass,p->arg_name,i);

    /* store the class variable names */
    for (i = 1, p = cvars; i <= cvcnt; ++i, p = p->arg_next)
        addcvar(theclass,p->arg_name);
    
    /* store the new class */
    cls[DE_VALUE].v_type = DT_OBJECT;
    cls[DE_VALUE].v.v_object = theclass;

    /* free the instance and class variable lists */
    freelist(&ivars); freelist(&cvars);

    /* handle each method declaration */
    while ((tkn = token()) == T_METHOD || tkn == T_CMETHOD) {
        switch (tkn) {
        case T_METHOD:
            do_method("Method",theclass);
            break;
        case T_CMETHOD:
            do_method("CMethod",metaclass);
            break;
        }
    }
    stoken(tkn);
    
    frequire(T_END);
    frequire(T_CLASS);

    /* cleanup the stack */
    sp += 1;
}

/* do_method - handle method declaration statement */
static void do_method(char *type, VALUE *class)
{
    char selector[100];
    VALUE *code,value;
    int tkn;
    
    /* initialize */
    arguments = temporaries = NULL;
    methodclass = class;
    literals = NULL;
    
    /* get 'self' */
    frequire('[');
    frequire(T_IDENTIFIER);
    addargument(&arguments,t_token);

    /* get the first part of the selector */
    frequire(T_IDENTIFIER);
    strcpy(selector,t_token);

    /* get each formal argument and each part of the message selector */
    while ((tkn = token()) != ']') {
        require(tkn,T_IDENTIFIER);
        addargument(&arguments,t_token);
        if ((tkn = token()) == ']')
            break;
        require(tkn,T_IDENTIFIER);
        strcat(selector,t_token);
    }
info("%s '%s'",type,selector);
    
    /* make the class object the first literal */
    make_lit_object(methodclass);
    
    /* compile the code */
    code = do_code(strcmp(type,"Method") == 0 ? T_METHOD : T_CMETHOD);

    /* store the new method */
    value.v_type = DT_BYTECODE;
    value.v.v_object = code;
    dict_add(methodclass[CL_METHODS].v.v_object,selector,&value);
    
    /* free the argument and temporary symbol lists */
    freelist(&arguments); freelist(&temporaries);
}

/* do_code - compile the code part of a function or method */
static VALUE *do_code(int endtkn)
{
    unsigned char *src,*dst;
    int tkn,nlits,i;
    LITERAL *lit;
    VALUE *code;

    /* initialize */
    cptr = 0;

    /* compile the code */
    putcbyte(OP_PUSH);
    if ((tkn = token()) != T_END) {
        do {
            stoken(tkn);
            do_statement();
        } while ((tkn = token()) != T_END);
    }
    putcbyte(OP_RETURN);
    frequire(endtkn);

    /* count the number of literals */
    for (nlits = 0, lit = literals; lit != NULL; lit = lit->lit_next)
        ++nlits;

    /* build the function */
    check(1);
    code = newvector(nlits+1); push_bytecode(code);
    
    /* create the code string */
    code[1].v_type = DT_STRING;
    code[1].v.v_string = newstring(cptr);
    src = &cbuff[0]; dst = &code[1].v.v_string->s_data[0];
    while (--cptr >= 0)
        *dst++ = *src++;
    
    /* copy the literals */
    for (i = 1, lit = literals; i <= nlits; lit = lit->lit_next)
        code[++i] = lit->lit_value;
    //decode_procedure(code);

    /* free the literal list */
    freeliterals(&literals);
    ++sp; /* cleanup the stack */

    /* return the code object */
    return (code);
}

/* do_statement - compile a statement */
static void do_statement(void)
{
    int tkn;
    switch (tkn = token()) {
    case T_IF:          do_if();        break;
    case T_WHILE:       do_while();     break;
    case T_RETURN:      do_return();    break;
    default:            stoken(tkn);
                        do_expr();
                        frequire(';');  break;
    }
}

/* do_if - compile the IF/THEN/ELSE expression */
static void do_if(void)
{
    int tkn,nxt,end;

    /* compile the test expression */
    do_expr();

    /* skip around the 'then' clause if the expression is false */
    putcbyte(OP_BRF);
    nxt = putcword(0);

    /* compile the 'then' clause */
    frequire(T_THEN);
    while ((tkn = token()) != T_ELSE && tkn != T_END) {
        stoken(tkn);
        do_statement();
    }

    /* compile the 'else' clause */
    if (tkn == T_ELSE) {
        putcbyte(OP_BR);
        end = putcword(0);
        fixup(nxt,cptr);
        do_statement();
        nxt = end;
    }
    else
        stoken(tkn);

    frequire(T_END);
    frequire(T_IF);

    /* handle the end of the statement */
    fixup(nxt,cptr);
}

/* do_while - compile the WHILE/DO expression */
static void do_while(void)
{
    int nxt,end,tkn;

    /* compile the test expression */
    nxt = cptr;
    do_expr();

    /* skip around the 'then' clause if the expression is false */
    putcbyte(OP_BRF);
    end = putcword(0);

    /* compile the loop body */
    frequire(T_DO);
    while ((tkn = token()) != T_END) {
        stoken(tkn);
        do_statement();
    }
    frequire(T_WHILE);

    /* branch back to the start of the loop */
    putcbyte(OP_BR);
    putcword(nxt);

    /* handle the end of the statement */
    fixup(end,cptr);
}

/* do_return - handle the RETURN expression */
static void do_return(void)
{
    do_expr();
    putcbyte(OP_RETURN);
}

/* do_expr - handle the '<', '=' and '>' operators */
static void do_expr(void)
{
    int tkn;
    do_expr2();
    while ((tkn = token()) == '<' || tkn == '=' || tkn == '>')
        switch (tkn) {
        case '<':
            putcbyte(OP_PUSH);
            do_expr2();
            putcbyte(OP_LT);
            break;
        case '=':
            putcbyte(OP_PUSH);
            do_expr2();
            putcbyte(OP_EQ);
            break;
        case '>':
            putcbyte(OP_PUSH);
            do_expr2();
            putcbyte(OP_GT);
            break;
        }
    stoken(tkn);
}

/* do_expr2 - handle the '+' and '-' operators */
static void do_expr2(void)
{
    int tkn;
    do_expr3();
    while ((tkn = token()) == '+' || tkn == '-')
        switch (tkn) {
        case '+':
            putcbyte(OP_PUSH);
            do_expr3();
            putcbyte(OP_ADD);
            break;
        case '-':
            putcbyte(OP_PUSH);
            do_expr3();
            putcbyte(OP_SUB);
            break;
        }
    stoken(tkn);
}

/* do_expr3 - handle the '*' and '/' operators */
static void do_expr3(void)
{
    int tkn;
    do_primary();
    while ((tkn = token()) == '*' || tkn == '/')
        switch (tkn) {
        case '*':
            putcbyte(OP_PUSH);
            do_primary();
            putcbyte(OP_MUL);
            break;
        case '/':
            putcbyte(OP_PUSH);
            do_primary();
            putcbyte(OP_DIV);
            break;
        }
    stoken(tkn);
}

/* do_primary - parse a primary expression */
static void do_primary(void)
{
    int tkn;
    switch (token()) {
    case '(':
        do_expr();
        frequire(')');
        switch (tkn = token()) {
        case '(':
            do_call();
            break;
        default:
            stoken(tkn);
            break;
        }
        break;
    case '[':
        do_send();
        break;
    case T_NUMBER:
        do_lit_integer((long)t_value);
        break;
    case T_STRING:
        do_lit_string(t_token);
        break;
    case T_IDENTIFIER:
        do_identifier();
        break;
    default:
        parse_error("Expecting a primary expression");
        break;
    }
}

/* do_identifier - compile an identifier */
static void do_identifier(void)
{
    char id[TKNSIZE+1];
    int tkn;

    /* remember the identifier */
    strcpy(id,t_token);

    /* check for a function call or assignment */
    switch (tkn = token()) {
    case '(':
        code_reference(id);
        do_call();
        break;
    case T_ASSIGN:
        do_assignment(id);
        break;
    default:
        stoken(tkn);
        code_reference(id);
        break;
    }
}

/* do_assignment - compile an assignment expression */
static void do_assignment(char *id)
{
    /* compile the value expression */
    do_expr();

    /* compile the assignment */
    code_assignment(id);
}

/* do_call - compile a function call */
static void do_call(void)
{
    int tkn,n=0;
    
    /* compile each argument expression */
    if ((tkn = token()) != ')') {
        stoken(tkn);
        do {
            putcbyte(OP_PUSH);
            do_expr();
            ++n;
        } while ((tkn = token()) == ',');
    }
    require(tkn,')');
    putcbyte(OP_CALL);
    putcbyte(n);
}

/* do_send - compile a message sending expression */
static void do_send(void)
{
    char selector[100];
    int opcode,tkn,n=1;
    LITERAL *lit;
    
    /* generate code to push the selector */
    code_literal(addliteral(&literals,&lit));

    /* compile the receiver expression */
    putcbyte(OP_PUSH);
    if ((tkn = token()) == T_SUPER) {
        opcode = OP_SENDSUPER;
        code_reference("SELF");
    }
    else {
        opcode = OP_SEND;
        stoken(tkn);
        do_expr();
    }

    /* get the first part of the selector */
    frequire(T_IDENTIFIER);
    strcpy(selector,t_token);

    /* get each formal argument and each part of the message selector */
    while ((tkn = token()) != ']') {
        stoken(tkn);
        putcbyte(OP_PUSH);
        do_expr();
        ++n;
        if ((tkn = token()) == ']')
            break;
        require(tkn,T_IDENTIFIER);
        strcat(selector,t_token);
    }
    
    /* fixup the selector literal */
    lit->lit_value.v_type = DT_STRING;
    lit->lit_value.v.v_string = makestring(selector);

    /* send the message */
    putcbyte(opcode);
    putcbyte(n);
}

/* get_id_list - get a comma separated list of identifiers */
static int get_id_list(ARGUMENT **list, int term)
{
    int tkn,cnt=0;
    if ((tkn = token()) != term) {
        stoken(tkn);
        do {
            frequire(T_IDENTIFIER);
            addargument(list,t_token);
            ++cnt;
        } while ((tkn = token()) == ',');
    }
    require(tkn,term);
    return (cnt);
}

/* addargument - add a formal argument */
static void addargument(ARGUMENT **list, char *name)
{
    ARGUMENT *arg;
    if ((arg = (ARGUMENT *)malloc(sizeof(ARGUMENT))) == NULL)
        error("Insufficient memory");
    arg->arg_name = save(name);
    arg->arg_next = *list;
    *list = arg;
}

/* freelist - free a list of arguments or temporaries */
static void freelist(ARGUMENT **plist)
{
    ARGUMENT *this,*next;
    for (this = *plist, *plist = NULL; this != NULL; this = next) {
        next = this->arg_next;
        free(this->arg_name);
        free(this);
    }
}

/* findarg - find an argument offset */
static int findarg(char *name)
{
    ARGUMENT *arg;
    int n;
    for (n = 0, arg = arguments; arg; n++, arg = arg->arg_next)
        if (strcmp(name,arg->arg_name) == 0)
            return (n);
    return (-1);
}

/* findtmp - find a temporary variable offset */
static int findtmp(char *name)
{
    ARGUMENT *tmp;
    int n;
    for (n = 0, tmp = temporaries; tmp; n++, tmp = tmp->arg_next)
        if (strcmp(name,tmp->arg_name) == 0)
            return (n);
    return (-1);
}

/* findivar - find an instance variable offset */
static int findivar(char *name)
{
    VALUE *class,*vars;
    int base,cnt,n;
    if ((class = methodclass) != NULL) {
        base = (int)class[CL_ISIZE].v.v_integer - 1;
        for (;;) {
            vars = class[CL_IVARS].v.v_vector;
            cnt = (int)vars[0].v.v_integer;
            base -= cnt;
            for (n = 1; n <= cnt; ++n)
                if (strcmp(name,(char *)vars[n].v.v_string->s_data) == 0)
                    return (base+n);
            if (class[CL_SUPER].v_type == DT_NIL)
                break;
            class = class[CL_SUPER].v.v_object;
        }
    }
    return (-1);
}

/* findcvar - find a class variable offset */
static VALUE *findcvar(char *name)
{
    VALUE *class,*sym;
    if ((class = methodclass) != NULL) {
        for (;;) {
            if ((sym = dict_find(class[CL_CVARS].v.v_object,name)) != NULL)
                return (sym);
            if (class[CL_SUPER].v_type == DT_NIL)
                break;
            class = class[CL_SUPER].v.v_object;
        }
    }
    return (NULL);
}

/* addliteral - add a literal */
static int addliteral(LITERAL **list, LITERAL **pval)
{
    LITERAL **plit,*lit;
    int n=2;
    for (plit = list; (lit = *plit) != NULL; plit = &lit->lit_next)
        ++n;
    if ((lit = (LITERAL *)malloc(sizeof(LITERAL))) == NULL)
        error("Insufficient memory");
    lit->lit_next = NULL;
    *pval = *plit = lit;
    return (n);
}

/* freeliterals - free a list of literals */
static void freeliterals(LITERAL **plist)
{
    LITERAL *this,*next;
    for (this = *plist, *plist = NULL; this != NULL; this = next) {
        next = this->lit_next;
        free(this);
    }
}

/* senter - enter a symbol into a symbol table */
VALUE *senter(VALUE *table, char *name)
{
    return (dict_add(table,name,NULL));
}

/* sfind - find a symbol in a symbol table */
VALUE *sfind(VALUE *table, char *name)
{
    return (dict_find(table,name));
}

/* frequire - fetch a token and check it */
static void frequire(int rtkn)
{
    require(token(),rtkn);
}

/* require - check for a required token */
static void require(int tkn, int rtkn)
{
    char msg[100],*tkn_name();
    if (tkn != rtkn) {
        sprintf(msg,"Expecting '%s', found '%s'",
                tkn_name(rtkn),
                tkn_name(tkn));
        parse_error(msg);
    }
}

/* do_lit_integer - compile a literal integer */
static void do_lit_integer(long n)
{
    LITERAL *lit;
    code_literal(addliteral(&literals,&lit));
    lit->lit_value.v_type = DT_INTEGER;
    lit->lit_value.v.v_integer = n;
}

/* do_lit_string - compile a literal string */
static void do_lit_string(char *str)
{
    LITERAL *lit;
    code_literal(addliteral(&literals,&lit));
    lit->lit_value.v_type = DT_STRING;
    lit->lit_value.v.v_string = makestring(str);
}

/* make_lit_object - make a literal object */
static int make_lit_object(VALUE *obj)
{
    LITERAL *lit;
    int n;
    n = addliteral(&literals,&lit);
    lit->lit_value.v_type = DT_OBJECT;
    lit->lit_value.v.v_object = obj;
    return (n);
}

/* code_reference - code a variable reference */
static void code_reference(char *id)
{    
    VALUE *sym;
    int n;
    if ((n = findarg(id)) >= 0)
        code_argument(n);
    else if ((n = findtmp(id)) >= 0)
        code_temporary(n);
    else if ((n = findivar(id)) >= 0)
        code_ivar(n);
    else if ((sym = findcvar(id)) != NULL)
        code_variable(make_lit_object(sym));
    else
        code_variable(make_lit_object(senter(symbols,id)));
}

/* code_assignment - code a variable assignment */
static void code_assignment(char *id)
{    
    VALUE *sym;
    int n;
    if ((n = findarg(id)) >= 0)
        code_setargument(n);
    else if ((n = findtmp(id)) >= 0)
        code_settemporary(n);
    else if ((n = findivar(id)) >= 0)
        code_setivar(n);
    else if ((sym = findcvar(id)) != NULL)
        code_setvariable(make_lit_object(sym));
    else
        code_setvariable(make_lit_object(senter(symbols,id)));
}

/* code_argument - compile an argument reference */
static void code_argument(int n)
{
    putcbyte(OP_ARG);
    putcbyte(n);
}

/* code_setargument - compile an argument assignment */
static void code_setargument(int n)
{
    putcbyte(OP_ASET);
    putcbyte(n);
}

/* code_temporary - compile a temporary variable reference */
static void code_temporary(int n)
{
    putcbyte(OP_TMP);
    putcbyte(n);
}

/* code_settemporary - compile a temporary variable assignment */
static void code_settemporary(int n)
{
    putcbyte(OP_TSET);
    putcbyte(n);
}

/* code_ivar - compile an instance variable reference */
static void code_ivar(int n)
{
    putcbyte(OP_IVAR);
    putcbyte(n);
}

/* code_setivar - compile an instance variable assignment */
static void code_setivar(int n)
{
    putcbyte(OP_IVSET);
    putcbyte(n);
}

/* code_variable - compile a variable reference */
static void code_variable(int n)
{
    putcbyte(OP_VAR);
    putcbyte(n);
}

/* code_setvariable - compile a variable assignment */
static void code_setvariable(int n)
{
    putcbyte(OP_VSET);
    putcbyte(n);
}

/* code_literal - compile a literal reference */
static void code_literal(int n)
{
    putcbyte(OP_LIT);
    putcbyte(n);
}

/* putcbyte - put a code byte into data space */
static int putcbyte(int b)
{
    if (cptr < CMAX)
        cbuff[cptr++] = b;
    else
        parse_error("Insufficient code space");
    return (cptr-1);
}

/* putcword - put a code word into data space */
static int putcword(int w)
{
    putcbyte(w);
    putcbyte(w >> 8);
    return (cptr-2);
}

/* fixup - fixup a reference chain */
static void fixup(int chn, int val)
{
    int hval,nxt;
    for (hval = val >> 8; chn != 0; chn = nxt) {
        if (chn < 0 || chn > CMAX-2)
            return;
        nxt = (cbuff[chn] & 0xFF) | (cbuff[chn+1] << 8);
        cbuff[chn] = val;
        cbuff[chn+1] = hval;
    }
}

/* save - allocate memory for a string */
static char *save(char *str)
{
    char *new;
    if ((new = malloc(strlen(str)+1)) == NULL)
        error("Insufficient memory");
    strcpy(new,str);
    return (new);
}
          
