//
//  acu_error.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 17/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "acu_int.h"



int _stackmgr_has_fatal(void)
{
    return (_g_acu.last_error != STACKMGR_ERROR_NONE);
}


void _stackmgr_raise_error(int in_error_code)
{
    if (_stackmgr_has_fatal()) return;
    assert(_g_acu.callbacks.stack_closed != NULL);
    
    _g_acu.last_error = in_error_code;
    _g_acu.callbacks.stack_closed(NULL, in_error_code);
}


void _acu_raise_error(int in_error_code)
{
    if (_stackmgr_has_fatal()) return;
    assert(_g_acu.callbacks.fatal_error != NULL);
    
    _g_acu.last_error = in_error_code;
    _g_acu.callbacks.fatal_error(in_error_code);
    
    abort();
}
