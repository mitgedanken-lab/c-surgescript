/*
 * SurgeScript
 * A lightweight programming language for computer games and interactive apps
 * Copyright (C) 2017  Alexandre Martins <alemartf(at)gmail(dot)com>
 *
 * compiler/codegen.c
 * SurgeScript Compiler: Code Generator
 */

#include <string.h>
#include "codegen.h"
#include "symtable.h"
#include "../runtime/program.h"
#include "../runtime/program_pool.h"
#include "../util/util.h"

#ifdef F
#undef F
#endif
#ifdef U
#undef U
#endif
#ifdef B
#undef B
#endif
#ifdef I
#undef I
#endif
#define _SSASMX(_0, _1, _2, X, ...)     X
#define _SSASM2(op, a, b)               surgescript_program_add_line(context.program, (op), (a), (b))
#define _SSASM1(op, a)                  surgescript_program_add_line(context.program, (op), (a), U(0))
#define _SSASM0(op)                     surgescript_program_add_line(context.program, (op), U(0), U(0))

/* utility macros */
#define SSASM(...)                      _SSASMX(__VA_ARGS__, _SSASM2, _SSASM1, _SSASM0)(__VA_ARGS__)
#define TEXT(text)                      U(surgescript_program_add_text(context.program, (text)))
#define LABEL(label)                    surgescript_program_add_label(context.program, (label))
#define NEWLABEL()                      surgescript_program_new_label(context.program)
#define F(x)                            surgescript_program_operand_f(x)
#define U(x)                            surgescript_program_operand_u(x)
#define B(x)                            surgescript_program_operand_b(x)
#define I(x)                            surgescript_program_operand_i(x)
#define T0                              U(0)
#define T1                              U(1)
#define T2                              U(2)
#define T3                              U(3)
#define BREAKPOINT(str)                 emit_breakpoint(context, (str))


/* objects */
void emit_object_header(surgescript_nodecontext_t context, surgescript_program_label_t start, surgescript_program_label_t end)
{
    SSASM(SSOP_JMP, U(end));
    LABEL(start);
}

void emit_object_footer(surgescript_nodecontext_t context, surgescript_program_label_t start, surgescript_program_label_t end)
{
    surgescript_program_label_t aloc = NEWLABEL();

    SSASM(SSOP_RET);
    LABEL(end);
        SSASM(SSOP_MOVF, T2, F(surgescript_symtable_count(context.symtable)));
        LABEL(aloc);
            SSASM(SSOP_JE, U(start));
            SSASM(SSOP_ALOC);
            SSASM(SSOP_DEC, T2);
            SSASM(SSOP_JMP, U(aloc));
}

/* declarations */
void emit_vardecl(surgescript_nodecontext_t context, const char* identifier)
{
    if(!surgescript_symtable_has_symbol(context.symtable, identifier))
        surgescript_symtable_put_heap_symbol(context.symtable, identifier, (surgescript_heapptr_t)surgescript_symtable_count(context.symtable));
    surgescript_symtable_emit_write(context.symtable, identifier, context.program, 0);
}

/* expressions */
void emit_assignexpr(surgescript_nodecontext_t context, const char* assignop, const char* identifier, int line)
{
    /* put the identifier on the symbol table, and assign the symbol an address */
    if(!surgescript_symtable_has_parent(context.symtable))
        ssfatal("Compile Error: invalid attribution (\"%s %s ...\") in object \"%s\" (%s:%d) - only a single attribution is allowed.", identifier, assignop, context.object_name, context.source_file, line);
    else if(!surgescript_symtable_has_symbol(context.symtable, identifier))
        surgescript_symtable_put_stack_symbol(context.symtable, identifier, (surgescript_stackptr_t)(1 + surgescript_symtable_count(context.symtable) - surgescript_program_arity(context.program)));

    /* perform the assignment operation */
    switch(*assignop) {
        case '=':
            surgescript_symtable_emit_write(context.symtable, identifier, context.program, 0);
            break;

        case '+': {
            surgescript_program_label_t cat = NEWLABEL();
            surgescript_program_label_t end = NEWLABEL();
            surgescript_symtable_emit_read(context.symtable, identifier, context.program, 1);
            SSASM(SSOP_TCHKS, T1);
            SSASM(SSOP_JE, U(cat));
            SSASM(SSOP_TCHKS, T0);
            SSASM(SSOP_JE, U(cat));
            SSASM(SSOP_ADD, T1, T0);
            SSASM(SSOP_JMP, U(end));
            LABEL(cat);
            SSASM(SSOP_CAT, T1, T0);
            LABEL(end);
            surgescript_symtable_emit_write(context.symtable, identifier, context.program, 1);
            SSASM(SSOP_XCHG, T0, T1);
            break;
        }

        case '-':
            surgescript_symtable_emit_read(context.symtable, identifier, context.program, 1);
            SSASM(SSOP_SUB, T1, T0);
            surgescript_symtable_emit_write(context.symtable, identifier, context.program, 1);
            SSASM(SSOP_XCHG, T0, T1);
            break;

        case '*':
            surgescript_symtable_emit_read(context.symtable, identifier, context.program, 1);
            SSASM(SSOP_MUL, T1, T0);
            surgescript_symtable_emit_write(context.symtable, identifier, context.program, 1);
            SSASM(SSOP_XCHG, T0, T1);
            break;

        case '/':
            surgescript_symtable_emit_read(context.symtable, identifier, context.program, 1);
            SSASM(SSOP_DIV, T1, T0);
            surgescript_symtable_emit_write(context.symtable, identifier, context.program, 1);
            SSASM(SSOP_XCHG, T0, T1);
            break;
    }
}

void emit_conditionalexpr1(surgescript_nodecontext_t context, surgescript_program_label_t nope, surgescript_program_label_t done)
{
    SSASM(SSOP_TEST, T0, T0);
    SSASM(SSOP_JE, U(nope));
}

void emit_conditionalexpr2(surgescript_nodecontext_t context, surgescript_program_label_t nope, surgescript_program_label_t done)
{
    SSASM(SSOP_JMP, U(done));
    LABEL(nope);
}

void emit_conditionalexpr3(surgescript_nodecontext_t context, surgescript_program_label_t nope, surgescript_program_label_t done)
{
    LABEL(done);
}

void emit_logicalorexpr1(surgescript_nodecontext_t context, surgescript_program_label_t done)
{
    /* short-circuit evaluation */
    SSASM(SSOP_TEST, T0, T0);
    SSASM(SSOP_JNE, U(done));
}

void emit_logicalorexpr2(surgescript_nodecontext_t context, surgescript_program_label_t done)
{
    LABEL(done);
}

void emit_logicalandexpr1(surgescript_nodecontext_t context, surgescript_program_label_t done)
{
    /* short-circuit evaluation */
    SSASM(SSOP_TEST, T0, T0);
    SSASM(SSOP_JE, U(done));
}

void emit_logicalandexpr2(surgescript_nodecontext_t context, surgescript_program_label_t done)
{
    LABEL(done);
}

void emit_equalityexpr1(surgescript_nodecontext_t context)
{
    SSASM(SSOP_PUSH, T0);
}

void emit_equalityexpr2(surgescript_nodecontext_t context, const char* equalityop)
{
    surgescript_program_label_t done = NEWLABEL();

    SSASM(SSOP_POP, T1);
    SSASM(SSOP_CMP, T1, T0);
    SSASM(SSOP_MOVB, T0, B(true));
    if(strcmp(equalityop, "==") == 0) {
        SSASM(SSOP_JE, U(done));
        SSASM(SSOP_MOVB, T0, B(false));
    }
    else if(strcmp(equalityop, "!=") == 0) {
        SSASM(SSOP_JNE, U(done));
        SSASM(SSOP_MOVB, T0, B(false));
    }
    LABEL(done);
}

void emit_relationalexpr1(surgescript_nodecontext_t context)
{
    SSASM(SSOP_PUSH, T0);
}

void emit_relationalexpr2(surgescript_nodecontext_t context, const char* relationalop)
{
    surgescript_program_label_t done = NEWLABEL();

    SSASM(SSOP_POP, T1);
    SSASM(SSOP_CMP, T1, T0);
    SSASM(SSOP_MOVB, T0, B(true));
    if(strcmp(relationalop, ">=") == 0) {
        SSASM(SSOP_JGE, U(done));
        SSASM(SSOP_MOVB, T0, B(false));
    }
    else if(strcmp(relationalop, ">") == 0) {
        SSASM(SSOP_JG, U(done));
        SSASM(SSOP_MOVB, T0, B(false));
    }
    else if(strcmp(relationalop, "<") == 0) {
        SSASM(SSOP_JL, U(done));
        SSASM(SSOP_MOVB, T0, B(false));
    }
    else if(strcmp(relationalop, "<=") == 0) {
        SSASM(SSOP_JLE, U(done));
        SSASM(SSOP_MOVB, T0, B(false));
    }
    LABEL(done);
}

void emit_additiveexpr1(surgescript_nodecontext_t context)
{
    SSASM(SSOP_PUSH, T0);
}

void emit_additiveexpr2(surgescript_nodecontext_t context, const char* additiveop)
{
    SSASM(SSOP_POP, T1);
    switch(*additiveop) {
        case '+': {
            surgescript_program_label_t cat = NEWLABEL();
            surgescript_program_label_t end = NEWLABEL();
            SSASM(SSOP_TCHKS, T1);
            SSASM(SSOP_JE, U(cat));
            SSASM(SSOP_TCHKS, T0);
            SSASM(SSOP_JE, U(cat));
            SSASM(SSOP_ADD, T0, T1);
            SSASM(SSOP_JMP, U(end));
            LABEL(cat);
            SSASM(SSOP_CAT, T1, T0);
            SSASM(SSOP_XCHG, T1, T0);
            LABEL(end);
            break;
        }

        case '-':
            SSASM(SSOP_SUB, T1, T0);
            SSASM(SSOP_XCHG, T1, T0);
            break;
    }
}

void emit_multiplicativeexpr1(surgescript_nodecontext_t context)
{
    SSASM(SSOP_PUSH, T0);
}

void emit_multiplicativeexpr2(surgescript_nodecontext_t context, const char* multiplicativeop)
{
    SSASM(SSOP_POP, T1);
    switch(*multiplicativeop) {
        case '*':
            SSASM(SSOP_MUL, T0, T1);
            break;

        case '/':
            SSASM(SSOP_DIV, T1, T0);
            SSASM(SSOP_XCHG, T1, T0);
            break;
    }
}

void emit_unarysign(surgescript_nodecontext_t context, const char* op)
{
    if(*op == '-')
        SSASM(SSOP_NEG, T0, T0);
}

void emit_unaryincdec(surgescript_nodecontext_t context, const char* op, const char* identifier, int line)
{
    if(surgescript_symtable_has_symbol(context.symtable, identifier)) {
        surgescript_symtable_emit_read(context.symtable, identifier, context.program, 0);
        if(strcmp(op, "++") == 0)
            SSASM(SSOP_INC, T0);
        else if(strcmp(op, "--") == 0)
            SSASM(SSOP_DEC, T0);
        surgescript_symtable_emit_write(context.symtable, identifier, context.program, 0);
    }
    else
        ssfatal("Compile Error: undefined symbol \"%s\" in %s:%d.", identifier, context.source_file, line);
}

void emit_unarynot(surgescript_nodecontext_t context)
{
    SSASM(SSOP_LNOT, T0, T0);
}

void emit_unarytype(surgescript_nodecontext_t context)
{
    surgescript_program_label_t str = NEWLABEL();
    surgescript_program_label_t obj = NEWLABEL();
    surgescript_program_label_t bol = NEWLABEL();
    surgescript_program_label_t nul = NEWLABEL();
    surgescript_program_label_t end = NEWLABEL();

    SSASM(SSOP_TCHKF, T0);
    SSASM(SSOP_JNE, U(str));
    SSASM(SSOP_MOVS, T0, TEXT("number"));
    SSASM(SSOP_JMP, U(end));

    LABEL(str);
    SSASM(SSOP_TCHKS, T0);
    SSASM(SSOP_JNE, U(obj));
    SSASM(SSOP_MOVS, T0, TEXT("string"));
    SSASM(SSOP_JMP, U(end));

    LABEL(obj);
    SSASM(SSOP_TCHKO, T0);
    SSASM(SSOP_JNE, U(bol));
    SSASM(SSOP_MOVS, T0, TEXT("object"));
    SSASM(SSOP_JMP, U(end));

    LABEL(bol);
    SSASM(SSOP_TCHKB, T0);
    SSASM(SSOP_JNE, U(nul));
    SSASM(SSOP_MOVS, T0, TEXT("boolean"));
    SSASM(SSOP_JMP, U(end));

    LABEL(nul);
    SSASM(SSOP_MOVS, T0, TEXT("null"));

    LABEL(end);
}

void emit_postincdec(surgescript_nodecontext_t context, const char* op, const char* identifier, int line)
{
    if(surgescript_symtable_has_symbol(context.symtable, identifier)) {
        surgescript_symtable_emit_read(context.symtable, identifier, context.program, 0);
        SSASM(SSOP_MOV, T1, T0);
        if(strcmp(op, "++") == 0)
            SSASM(SSOP_INC, T1);
        else if(strcmp(op, "--") == 0)
            SSASM(SSOP_DEC, T1);
        surgescript_symtable_emit_write(context.symtable, identifier, context.program, 1);
    }
    else
        ssfatal("Compile Error: undefined symbol \"%s\" in %s:%d.", identifier, context.source_file, line);
}

void emit_pushparam(surgescript_nodecontext_t context)
{
    SSASM(SSOP_PUSH, T0);
}

void emit_popparams(surgescript_nodecontext_t context, int n)
{
    SSASM(SSOP_POPN, U(n));
}

void emit_funcall(surgescript_nodecontext_t context, const char* fun_name, int num_params)
{
    /*BREAKPOINT(fun_name);*/
    SSASM(SSOP_CALL, TEXT(fun_name), U(num_params));
}

void emit_dictset1(surgescript_nodecontext_t context, const char* assignop, const char* identifier, int line)
{
    if(!surgescript_symtable_has_parent(context.symtable))
        ssfatal("Compile Error: invalid attribution (\"%s %s ...\") in object \"%s\" (%s:%d) - only a single attribution is allowed.", identifier, assignop, context.object_name, context.source_file, line);

    /* save <expr> */
    SSASM(SSOP_PUSH, T0);
}

void emit_dictset2(surgescript_nodecontext_t context, const char* assignop, const char* identifier, int line)
{
    /* where's our object? */
    if(!surgescript_symtable_has_symbol(context.symtable, identifier))
        ssfatal("Compile Error: undefined symbol \"%s\" in %s:%d.", identifier, context.source_file, line);

    /* prepare the stack */
    SSASM(SSOP_POP, T3);  /* <expr> */
    SSASM(SSOP_PUSH, T0); /*<assignexpr> */
    surgescript_symtable_emit_read(context.symtable, identifier, context.program, 1);
    SSASM(SSOP_PUSH, T1); /* t1 is the object */
    SSASM(SSOP_PUSH, T3); /* t3 is <expr> */

    /* perform the assignment operation */
    switch(*assignop) {
        case '=':
            SSASM(SSOP_PUSH, T0); /* t0 is <assignexpr> */
            SSASM(SSOP_CALL, TEXT("set"), U(2));
            SSASM(SSOP_POP, T0); /* return <assignexpr> */
            SSASM(SSOP_POPN, U(3));
            break;

        case '+':
        case '-':
        case '*':
        case '/':
            SSASM(SSOP_CALL, TEXT("get"), U(1));
            SSASM(SSOP_POP, T3); /* <expr> */
            SSASM(SSOP_POP, T1); /* the object */
            SSASM(SSOP_POP, T1); /* <assignexpr> */
            if(*assignop == '+') {
                surgescript_program_label_t cat = NEWLABEL();
                surgescript_program_label_t end = NEWLABEL();
                SSASM(SSOP_TCHKS, T1);
                SSASM(SSOP_JE, U(cat));
                SSASM(SSOP_TCHKS, T0);
                SSASM(SSOP_JE, U(cat));
                SSASM(SSOP_ADD, T0, T1); /* t0 = dict.get(<expr>) + <assignexpr> */
                SSASM(SSOP_JMP, U(end));
                LABEL(cat);
                SSASM(SSOP_CAT, T0, T1);
                LABEL(end);
            }
            else if(*assignop == '-')
                SSASM(SSOP_SUB, T0, T1); /* t0 = dict.get(<expr>) - <assignexpr> */
            else if(*assignop == '*')
                SSASM(SSOP_MUL, T0, T1); /* t0 = dict.get(<expr>) * <assignexpr> */
            else
                SSASM(SSOP_DIV, T0, T1); /* t0 = dict.get(<expr>) / <assignexpr> */
            surgescript_symtable_emit_read(context.symtable, identifier, context.program, 1);
            SSASM(SSOP_PUSH, T1); /* the object */
            SSASM(SSOP_PUSH, T3); /* <expr> */
            SSASM(SSOP_PUSH, T0); /* the result that is desired */
            SSASM(SSOP_CALL, TEXT("set"), U(2));
            SSASM(SSOP_POP, T0); /* return <assignexpr> */
            SSASM(SSOP_POPN, U(2));
            break;
    }
}

void emit_dictget(surgescript_nodecontext_t context, const char* identifier, int line)
{
    if(!surgescript_symtable_has_symbol(context.symtable, identifier))
        ssfatal("Compile Error: undefined symbol \"%s\" in %s:%d.", identifier, context.source_file, line);

    surgescript_symtable_emit_read(context.symtable, identifier, context.program, 1);
    SSASM(SSOP_PUSH, T1);
    SSASM(SSOP_PUSH, T0);
    SSASM(SSOP_CALL, TEXT("get"), U(1));
    SSASM(SSOP_POPN, U(2));
}

/* statements */
void emit_if(surgescript_nodecontext_t context, surgescript_program_label_t nope)
{
    SSASM(SSOP_TEST, T0, T0);
    SSASM(SSOP_JE, U(nope));
}

void emit_else(surgescript_nodecontext_t context, surgescript_program_label_t nope, surgescript_program_label_t done)
{
    SSASM(SSOP_JMP, U(done));
    LABEL(nope);
}

void emit_endif(surgescript_nodecontext_t context, surgescript_program_label_t done)
{
    LABEL(done);
}

/* functions */
int emit_function_header(surgescript_nodecontext_t context)
{
    return SSASM(SSOP_NOP); /* to be filled later */
}

void emit_function_footer(surgescript_nodecontext_t context, int num_locals, int fun_header)
{
    if(num_locals > 0)
        surgescript_program_chg_line(context.program, fun_header, SSOP_PUSHN, U(num_locals), U(0));
    SSASM(SSOP_MOVN, T0);
    /*SSASM(SSOP_POPN, U(num_locals));*/ /* not needed, since popenv() clears the stack frame for us */
    SSASM(SSOP_RET);
}

void emit_function_argument(surgescript_nodecontext_t context, const char* identifier, int line, int idx, int argc)
{
    /* idx is the index of the argument; it is such that 0 <= idx < argc (left-to-right) */
    if(!surgescript_symtable_has_local_symbol(context.symtable, identifier))
        surgescript_symtable_put_stack_symbol(context.symtable, identifier, (surgescript_stackptr_t)(idx-argc));
    else
        ssfatal("Duplicate function parameter name \"%s\" in %s:%d.", identifier, context.source_file, line);
}

void emit_ret(surgescript_nodecontext_t context)
{
    SSASM(SSOP_RET);
}

/* constants & variables */
void emit_app(surgescript_nodecontext_t context)
{
    SSASM(SSOP_MOVR, T0);
}

void emit_this(surgescript_nodecontext_t context)
{
    SSASM(SSOP_MOVC, T0);
}

void emit_state(surgescript_nodecontext_t context)
{
    SSASM(SSOP_MOVT, T0);
}

void emit_identifier(surgescript_nodecontext_t context, const char* identifier, int line)
{
    if(surgescript_symtable_has_symbol(context.symtable, identifier))
        surgescript_symtable_emit_read(context.symtable, identifier, context.program, 0);
    else
        ssfatal("Compile Error: undefined symbol \"%s\" in %s:%d.", identifier, context.source_file, line);
}

void emit_null(surgescript_nodecontext_t context)
{
    SSASM(SSOP_MOVN, T0);
}

void emit_bool(surgescript_nodecontext_t context, bool value)
{
    SSASM(SSOP_MOVB, T0, B(value));
}

void emit_number(surgescript_nodecontext_t context, float value)
{
    SSASM(SSOP_MOVF, T0, F(value));
}

void emit_string(surgescript_nodecontext_t context, const char* value)
{
    SSASM(SSOP_MOVS, T0, TEXT(value));
}

void emit_zero(surgescript_nodecontext_t context)
{
    SSASM(SSOP_XOR, T0, T0);
}

/* misc */
void emit_setstate(surgescript_nodecontext_t context)
{
    SSASM(SSOP_MOVT, T0, I(-1)); /* return value is in t[0] */
}

void emit_nop(surgescript_nodecontext_t context)
{
    SSASM(SSOP_NOP);
}

void emit_push(surgescript_nodecontext_t context)
{
    SSASM(SSOP_PUSH, T0);
}

void emit_pop(surgescript_nodecontext_t context)
{
    SSASM(SSOP_POP, T0);
}

void emit_breakpoint(surgescript_nodecontext_t context, const char* text)
{
    SSASM(SSOP_NOP, I(-1), TEXT(text));
}