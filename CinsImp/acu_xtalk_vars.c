//
//  acu_xtalk_vars.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 19/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "acu_int.h"

#define _MAX_DEBUG_CONTEXTS 1000


void _acu_unregister_vars(void)
{
    //printf("clearing vars\n");
    _acu_thread_mutex_lock(&(_g_acu.debug_information_m));
    for (int i = 0; i < _g_acu.debug_variable_count; i++)
    {
        ACUDebugVariable *var = &(_g_acu.debug_variables[i]);
        if (var->name) free(var->name);
        if (var->value) free(var->value);
    }
    if (_g_acu.debug_variables) free(_g_acu.debug_variables);
    _g_acu.debug_variables = NULL;
    _g_acu.debug_variable_count = 0;
    _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
}


/*
 *  _acu_copy_debug_information
 *  ---------------------------------------------------------------------------------------------
 *  Because the xTalk engine could be modifying our debug information at any time, and for ease
 *  of construction of the user interface, we copy the changed debug information when the UI 
 *  requests it.
 *
 *  The UI is then free to use this information until the next time it calls for an updated copy.
 *
 *  ! Mutex assumed already acquired when calling into this function
 */
static void _acu_copy_debug_information(void)
{
    /* currently we completely clear this existing copy */
    for (int i = 0; i < _g_acu.debug_vars_copy_count; i++)
    {
        ACUDebugVariable *var = &(_g_acu.debug_vars_copy[i]);
        if (var->name) free(var->name);
        if (var->value) free(var->value);
    }
    if (_g_acu.debug_vars_copy) free(_g_acu.debug_vars_copy);
    
    /* prepare the new copy */
    _g_acu.debug_vars_copy = malloc(sizeof(ACUDebugVariable) * _g_acu.debug_variable_count);
    if (!_g_acu.debug_vars_copy)
    {
        _acu_raise_error(ACU_ERROR_MEMORY);
        return;
    }
    
    /* perform the copying */
    for (int i = 0; i < _g_acu.debug_variable_count; i++)
    {
        ACUDebugVariable *s_var = &(_g_acu.debug_variables[i]);
        ACUDebugVariable *d_var = &(_g_acu.debug_vars_copy[i]);
        d_var->name = _acu_clone_cstr(s_var->name);
        d_var->value = _acu_clone_cstr(s_var->value);
    }
    _g_acu.debug_vars_copy_count = _g_acu.debug_variable_count;
}


void acu_debug_begin_inspection(void)
{
    _acu_thread_mutex_lock(&(_g_acu.debug_information_m));
    _acu_copy_debug_information();
}


void acu_debug_end_inspection(void)
{
    _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
}


char const* acu_debug_context(void)
{
    return _g_acu.debug_handler_name;
}


int acu_debug_variable_count(void)
{
    return _g_acu.debug_vars_copy_count;
}


char const* acu_debug_variable_name(int in_index)
{
    return _g_acu.debug_vars_copy[in_index].name;
}


char const* acu_debug_variable_value(int in_index)
{
    return _g_acu.debug_vars_copy[in_index].value;
}


void acu_debug_set_variable(int in_index, char const *in_name, char const *in_value)
{
    assert(_g_acu.current_stack != NULL);
    
    /* check for outstanding requests */
    /*_acu_xt_comms_lock(_g_acu.current_stack);
    if (_g_acu.current_stack->xt_sig_dbg_imut != _ACU_T_SIG_NOTICE)
    {
        _acu_xt_comms_unlock(_g_acu.current_stack);
        return ACU_FALSE;
    }*/
    
    /* submit a request to mutate a variable to the other thread */
    _acu_xt_comms_lock(_g_acu.current_stack);
    _g_acu.current_stack->xt_sig_dbg_imut = _ACU_T_SIG_REQ;
    _g_acu.current_stack->xt_imut_var_name = (char*)in_name;
    _g_acu.current_stack->xt_imut_var_value = (char*)in_value;
    
    /* wakeup the xtalk thread if it's sleeping */
    _acu_xt_comms_unlock(_g_acu.current_stack);
    _acu_xt_wakeup(_g_acu.current_stack);
    
    /* wait for acknowledgement of the set */
    for (;;)
    {
        _acu_xt_comms_lock(_g_acu.current_stack);
        if (_g_acu.current_stack->xt_sig_dbg_imut != _ACU_T_SIG_REQ)
        {
            _acu_xt_comms_unlock(_g_acu.current_stack);
            break;
        }
        _acu_xt_comms_unlock(_g_acu.current_stack);
        _acu_thread_usleep(50);
    }
    //return ACU_TRUE;
}



/********** OLD SHIT: **********/


char const* const* stackmgr_debug_var_contexts(void)
{
    if (_g_acu.current_stack == STACKMGR_INVALID_HANDLE) return NULL;
    
    return (char const* const*)_g_acu.debug_handlers;
}


int stackmgr_debug_var_count(int in_context)
{
    if (_g_acu.current_stack == STACKMGR_INVALID_HANDLE) return 0;
    
    if (in_context < 0) return _g_acu.debug_global_count;
    if (in_context != _g_acu.debug_handler_index)
    {
        _acu_debug_get_all_variables(in_context);
        return 0;
    }
    return _g_acu.debug_local_count;
}


char const* stackmgr_debug_var_name(int in_context, int in_index)
{
    if (in_context < 0)
        return _g_acu.debug_globals[in_index].name;
    if (in_index >= _g_acu.debug_local_count) return "";
    return _g_acu.debug_locals[in_index].name;
}


char const* stackmgr_debug_var_value(int in_context, int in_index)
{
    if (in_context < 0)
        return _g_acu.debug_globals[in_index].value;
    if (in_index >= _g_acu.debug_local_count) return "";
    return _g_acu.debug_locals[in_index].value;
}


void stackmgr_debug_var_set(int in_context, int in_index, char const *in_name, char const *in_value)
{
    assert(_g_acu.current_stack != NULL);

    /* submit a request to mutate a variable to the other thread */
    _acu_xt_comms_lock(_g_acu.current_stack);
    _g_acu.current_stack->xt_sig_dbg_imut = _ACU_T_SIG_REQ;
    _g_acu.current_stack->xt_imut_var_name = (char*)in_name;
    _g_acu.current_stack->xt_imut_handler_index = in_context;
    _g_acu.current_stack->xt_imut_var_value = (char*)in_value;
    
    /* wakeup the xtalk thread if it's sleeping */
    _acu_xt_comms_unlock(_g_acu.current_stack);
    _acu_xt_wakeup(_g_acu.current_stack);
    
    /* wait for acknowledgement of the set */
    for (;;)
    {
        _acu_xt_comms_lock(_g_acu.current_stack);
        if (_g_acu.current_stack->xt_sig_dbg_imut != _ACU_T_SIG_REQ)
        {
            _acu_xt_comms_unlock(_g_acu.current_stack);
            break;
        }
        _acu_xt_comms_unlock(_g_acu.current_stack);
        _acu_thread_usleep(5);
    }
}


void _acu_debug_get_all_variables(int in_context)
{
    assert(_g_acu.current_stack != NULL);
    
    _g_acu.debug_handler_index = in_context;
    
    /* submit a request for all variables to XT */
    _acu_xt_comms_lock(_g_acu.current_stack);
    _g_acu.current_stack->xt_sig_dbg_ireq = _ACU_T_SIG_REQ;
    _g_acu.current_stack->xt_ireq_handler_index = in_context;
    
    /* wakeup the xtalk thread if it's sleeping */
    _acu_xt_comms_unlock(_g_acu.current_stack);
    _acu_xt_wakeup(_g_acu.current_stack);
}


void _acu_debug_got_all_handlers(char const *in_handlers[], int in_handler_count)
{
    /* cleanup old handler list cache */
    for (int i = 0; i < _g_acu.debug_handler_count; i++)
    {
        if (_g_acu.debug_handlers[i]) free(_g_acu.debug_handlers[i]);
    }
    if (_g_acu.debug_handlers) free(_g_acu.debug_handlers);
    _g_acu.debug_handlers = NULL;
    if (_g_acu.current_stack == NULL) return;
    
    /* build new handler list cache */
    //printf("Handlers\n");
    _g_acu.debug_handlers = calloc(in_handler_count + 1, sizeof(char*));
    if (!_g_acu.debug_handlers)
    {
        _acu_raise_error(ACU_ERROR_MEMORY);
        return;
    }
    for (int i = 0; i < in_handler_count; i++)
    {
        _g_acu.debug_handlers[i] = _acu_clone_cstr(in_handlers[i]);
        //printf("  %s\n", _g_acu.debug_handlers[i]);
    }
    _g_acu.debug_handler_count = in_handler_count;
}



