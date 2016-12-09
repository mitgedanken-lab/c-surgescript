/*
 * SurgeScript
 * A lightweight programming language for computer games and interactive apps
 * Copyright (C) 2016  Alexandre Martins <alemartf(at)gmail(dot)com>
 *
 * runtime/stack.h
 * SurgeScript stack
 */

#ifndef _SURGESCRIPT_RUNTIME_STACK_H
#define _SURGESCRIPT_RUNTIME_STACK_H

#include <stdlib.h>

/* types */
typedef struct surgescript_stack_t surgescript_stack_t;
struct surgescript_stack_t;

/* forward declarations */
struct surgescript_var_t;

/* public methods */
surgescript_stack_t* surgescript_stack_create();
surgescript_stack_t* surgescript_stack_destroy(surgescript_stack_t* stack);
struct surgescript_var_t* surgescript_stack_push(surgescript_stack_t* stack); /* allocates a var */
void surgescript_stack_pop(surgescript_stack_t* stack); /* deallocates a var */
void surgescript_stack_pushenv(surgescript_stack_t* stack, int num_vars); /* pushes an environment */
void surgescript_stack_popenv(surgescript_stack_t* stack); /* pops an environment */
struct surgescript_var_t* surgescript_stack_top(surgescript_stack_t* stack); /* gets the topmost element */
struct surgescript_var_t* surgescript_stack_at(surgescript_stack_t* stack, int offset); /* gets stack[base + offset], offset > 0 */
int surgescript_stack_empty(surgescript_stack_t* stack); /* is the stack empty? */

#endif
