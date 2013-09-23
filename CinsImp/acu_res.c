//
//  acu_res.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 10/09/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "acu_int.h"



int _load_builtin_resources(void)
{
    char const *path = _g_acu.callbacks.builtin_resources_path();
    StackOpenStatus status;
    _g_acu.builtin_resources = stack_open(path, (StackFatalErrorHandler)&_stackmgr_handle_stack_error, NULL, &status);
    if (!_g_acu.builtin_resources) return ACU_FALSE;
    return ACU_TRUE;
}








