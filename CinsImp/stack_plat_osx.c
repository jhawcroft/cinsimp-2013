//
//  stack_plat_osx.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 6/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "stack_int.h"

#include <libgen.h>


char const* stack_name(Stack *in_stack)
{
    if (in_stack->name) return in_stack->name;
    
    char *temp = _stack_clone_cstr(in_stack->pathname);
    in_stack->name = _stack_clone_cstr( basename(temp) );
    _stack_free(temp);
    
    long len = strlen(in_stack->name);
    for (long i = len-1; i >= 0; i--)
    {
        if (in_stack->name[i] == '.')
        {
            in_stack->name[i] = 0;
            break;
        }
    }
    
    return in_stack->name;
}

