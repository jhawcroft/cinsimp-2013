/*
 
 Stack File Format
 stack_error.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Major fault/error handling
 
 (see header file for module description)
 
 */

#include "stack_int.h"


/* ensure that public functions take notice of failure conditions and don't try to keep working .... */
/* ensure that errors are suitably reported & handled in the application */

/**********
 Fault Handling
 */

void _stack_file_error_void(Stack *in_stack)
{
    in_stack->has_fatal_error = STACK_YES;
    if (in_stack->fatal_handler)
        in_stack->fatal_handler(in_stack, in_stack->fatal_context, STACK_ERR_IO);
}


void* _stack_file_error_null(Stack *in_stack)
{
    _stack_file_error_void(in_stack);
    return NULL;
}


void stack_set_fatal_error_handler(Stack *in_stack, StackFatalErrorHandler in_fatal_handler, void *in_context)
{
    in_stack->fatal_handler = in_fatal_handler;
    in_stack->fatal_context = in_context;
}


void _stack_panic_void(Stack *in_stack, int in_error_code)
{
    assert(0);
}


int _stack_panic_err(Stack *in_stack, int in_error_code)
{
    assert(0);
    return in_error_code;
}


int _stack_panic_false(Stack *in_stack, int in_error_code)
{
    assert(0);
    return 0;
}


void* _stack_panic_null(Stack *in_stack, int in_error_code)
{
    assert(0);
    return NULL;
}




