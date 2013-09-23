/*
 
 Application Control Unit - xTalk Thread-side Glue
 acu_xtalk_tglue.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Glue code that implements CinsImp environment specific functionality on behalf of the generic
 xTalk engine.  This glue is 'thread-side', in that it is invoked by the xtalk thread and 
 generally should not talk directly to any UI specific code.
 
 Access to the Stack Unit directly from this glue is safe, since the Stack Unit is indirectly 
 accessed via the ACU and the ACU prohibits outside access while xTalk is running.
 
 *************************************************************************************************
 */

#include "acu_int.h"


/*
 *  _handle_script_error
 *  ---------------------------------------------------------------------------------------------
 *  xTalk has raised a script error (syntax/runtime).  We take a copy of the error and post it
 *  to the UT.
 */
static void _handle_script_error(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_handler_object, long in_source_line,
                                 char const *in_template, char const *in_arg1, char const *in_arg2, char const *in_arg3, int in_runtime)
{
    _acu_xt_comms_lock(in_stack);
    
    _stackmgr_clear_scripting_error(in_stack);
    
    in_stack->ut_sig_script_error = _ACU_T_SIG_NOTICE;
    in_stack->thread_xte_error_template = _acu_clone_cstr(in_template);
    in_stack->thread_xte_error_arg1 = _acu_clone_cstr(in_arg1);
    in_stack->thread_xte_error_arg2 = _acu_clone_cstr(in_arg2);
    in_stack->thread_xte_error_arg3 = _acu_clone_cstr(in_arg3);
    in_stack->thread_xte_error_is_runtime = in_runtime;
    in_stack->thread_xte_source_line = (int)in_source_line;
    in_stack->ut_script_error_object = acu_retain(xte_variant_object_data_ptr(in_handler_object));
    
    _acu_xt_comms_unlock(in_stack);
}


/*
 *  _handle_script_checkpoint
 *  ---------------------------------------------------------------------------------------------
 *  xTalk is reporting to the debugger (if any) that the handler has stopped at a checkpoint.
 *
 *  The UI will eventually display this in the script editor.
 */
static void _handle_script_checkpoint(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_handler_object, long in_source_line)
{
    //printf("Got checkpoint\n");
    _acu_xt_comms_lock(in_stack);
    
    in_stack->ut_sig_script_chkpt = _ACU_T_SIG_NOTICE;
    acu_release(in_stack->ut_script_error_object);
    in_stack->ut_script_error_object = acu_retain(xte_variant_object_data_ptr(in_handler_object));
    in_stack->thread_xte_source_line = (int)in_source_line;
    
    _acu_xt_comms_unlock(in_stack);
}


/*
 *  _handle_message_result
 *  ---------------------------------------------------------------------------------------------
 *  xTalk has a result for the message previously invoked using xte_message().
 *
 *  The UI will eventually display this in the message box.
 */
static void _handle_message_result(XTE *in_engine, StackMgrStack *in_stack, const char *in_result)
{
    in_stack->xt_sic.param[0].value.pointer = (void*)in_result;
    
    void _acu_ut_message_result();
    _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_message_result);
}


/*
 *  _handle_debug_message
 *  ---------------------------------------------------------------------------------------------
 *  xTalk has begun handling a message and is reporting the message to the debugger (if any).
 *
 *  The UI will eventually display this in the Message Watcher.
 */
static void _handle_debug_message(XTE *in_engine, StackMgrStack *in_stack, char const *in_message, int in_level, int in_handled)
{
    _acu_thread_mutex_lock(&(_g_acu.debug_information_m));
    if (in_stack != _g_acu.current_stack)
    {
        _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
        return; /* ignore debug information for stacks other than the current */
    }
    _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
    
    
    in_stack->xt_sic.param[0].value.string = in_message;
    in_stack->xt_sic.param[1].value.integer = in_level;
    in_stack->xt_sic.param[2].value.boolean = in_handled;
    
    void _acu_ut_debug_message();
    _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_debug_message);
}


/* both these need to be reimagined;
 possibly a queue for variable mutations outgoing notifications,
 with the smarts to clear the queue of local changes when the
 handler level decreases, or remove notifications that are
 no longer valid;
 lets limit the UI's debugger to showing only globals or whatever
 is in the currently executing handler, otherwise we'll have
 a massive task on our hands */

/*
 *  _handle_debug_variable
 *  ---------------------------------------------------------------------------------------------
 *  xTalk is reporting to the debugger (if any) that a variable has been mutated.
 *
 *  The UI will eventually display this in the Variable Watcher.
 *
 *  ! Cannot use SIC under any circumstances, as may have actually been invoked from the
 *    sleeping part of the SIC handler.
 */
static void _handle_debug_variable(XTE *in_engine, StackMgrStack *in_stack, char const *in_name, int is_global, char const *in_value)
{
    //printf("got var %s\n", in_name);
    _acu_thread_mutex_lock(&(_g_acu.debug_information_m));
    
    if (in_stack != _g_acu.current_stack)
    {
        _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
        return; /* ignore debug information for stacks other than the current */
    }
    
    
    /* find the variable */
    for (int i = 0; i < _g_acu.debug_variable_count; i++)
    {
        if (strcmp(_g_acu.debug_variables[i].name, in_name) == 0)
        {
            /* found it; update it's value */
            ACUDebugVariable *var = &(_g_acu.debug_variables[i]);
            
            if (var->value) free(var->value);
            var->value = _acu_clone_cstr(in_value);
            
            _g_acu.ut_sig_debug_info_changed = _ACU_T_SIG_NOTICE;
            
            _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
            return;
        }
    }
    
    /* didn't find it; create it */
    ACUDebugVariable *new_table = realloc(_g_acu.debug_variables, sizeof(ACUDebugVariable) * (_g_acu.debug_variable_count + 1));
    if (!new_table)
    {
        _acu_raise_error(STACKMGR_ERROR_MEMORY);
        return;
    }
    _g_acu.debug_variables = new_table;
    new_table[_g_acu.debug_variable_count].name = _acu_clone_cstr(in_name);
    new_table[_g_acu.debug_variable_count].value = _acu_clone_cstr(in_value);
    _g_acu.debug_variable_count++;
    
    _g_acu.ut_sig_debug_info_changed = _ACU_T_SIG_NOTICE;

    _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
    
    
    /* post DIR_IC message to UT */
    /*_acu_xt_comms_lock(in_stack);
    
    if (in_stack->ut_dir_ic.param[0].value.string) free((char*)in_stack->ut_dir_ic.param[0].value.string);
    if (in_stack->ut_dir_ic.param[2].value.string) free((char*)in_stack->ut_dir_ic.param[2].value.string);
    in_stack->ut_dir_ic.param[0].value.string = _acu_clone_cstr(in_name);
    in_stack->ut_dir_ic.param[1].value.integer = is_global;
    in_stack->ut_dir_ic.param[2].value.string = _acu_clone_cstr(in_value);
    
    void _acu_ut_debug_got_variable();
    in_stack->ut_dir_ic.implementor = (ACUImplementor)_acu_ut_debug_got_variable;
    in_stack->ut_sig_dir_ic = _ACU_T_SIG_REQ;
    
    _acu_xt_comms_unlock(in_stack);*/
}


/*
 *  _handle_debug_handler
 *  ---------------------------------------------------------------------------------------------
 *  xTalk is reporting to the debugger (if any) that the handler stack has changed.
 *
 *  The UI will eventually display this in the Variable Watcher.
 *
 *  ! Cannot use SIC under any circumstances, as may have actually been invoked from the
 *    sleeping part of the SIC handler.
 */
static void _handle_debug_handler(XTE *in_engine, StackMgrStack *in_stack, char const *in_handlers[], int in_handler_count)
{
    _acu_thread_mutex_lock(&(_g_acu.debug_information_m));
    
    if (in_stack != _g_acu.current_stack)
    {
        _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
        return; /* ignore debug information for stacks other than the current */
    }
    
    if (_g_acu.debug_handler_name) free(_g_acu.debug_handler_name);
    if (in_handler_count < 1)
        _g_acu.debug_handler_name = NULL;
    else
        _g_acu.debug_handler_name = _acu_clone_cstr(in_handlers[in_handler_count-1]);
    _g_acu.ut_sig_debug_info_changed = _ACU_T_SIG_NOTICE;
    
    _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
    
    void _acu_unregister_vars(void);
    _acu_unregister_vars();
    xte_debug_enumerate_variables(in_stack->xtalk, XTE_CONTEXT_CURRENT);
    
    
    /*in_stack->xt_sic.param[0].value.pointer = (void*)in_handlers;
    in_stack->xt_sic.param[1].value.integer = in_handler_count; 
    
    void _acu_ut_debug_got_handlers(StackMgrStack *in_stack);
    _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_debug_got_handlers);*/
}


/************/





int _retr_script(XTE *in_engine, StackMgrStack *in_register_entry, XTEVariant *in_object,
                        char const **out_script, int const *out_checkpoints[], int *out_checkpoint_count)
{
    int err = stackmgr_script_get(xte_variant_ref_ident(in_object), out_script, out_checkpoints, out_checkpoint_count, NULL);
    if (err == STACK_ERR_NONE) return XTE_TRUE;
    return XTE_FALSE;
}


int _next_responder(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_responder, XTEVariant **out_responder)
{
    HandleDef *responder = xte_variant_ref_ident(in_responder);
    HandleDef *next_responder = NULL;
    
    *out_responder = NULL;
    if (!_stackmgr_handle_is_stack(responder))
    {
        switch (responder->reference.type)
        {
            case STACKMGR_TYPE_BUTTON:
            case STACKMGR_TYPE_FIELD:
            {
                //long card_id, bkgnd_id;
                //stack_widget_owner(in_stack->stack, responder->reference.widget_id, &card_id, &bkgnd_id);
                
                next_responder = _stackmgr_handle_create(STACKMGR_FLAG_MAN_RELEASE);
                next_responder->reference.type = STACKMGR_TYPE_CARD;
                next_responder->reference.register_entry = in_stack;
                next_responder->reference.session_id = in_stack->session_id;
                //next_responder->reference.layer_id = (card_id != STACK_NO_OBJECT ? card_id : bkgnd_id);
                next_responder->reference.layer_id = _stackmgr_current_card_id(in_stack);
                next_responder->reference.layer_is_card = ACU_TRUE;//(card_id != STACK_NO_OBJECT);
                
                *out_responder = xte_object_ref(in_engine, "card", next_responder,
                                                (XTEObjRefDeallocator)&_acu_variant_handle_destructor);
                break;
            }
                
            case STACKMGR_TYPE_CARD:
            {
                long bkgnd_id;
                bkgnd_id = stack_card_bkgnd_id(in_stack->stack, responder->reference.layer_id);
                
                next_responder = _stackmgr_handle_create(STACKMGR_FLAG_MAN_RELEASE);
                next_responder->reference.type = STACKMGR_TYPE_BKGND;
                next_responder->reference.register_entry = in_stack;
                next_responder->reference.session_id = in_stack->session_id;
                next_responder->reference.layer_id = bkgnd_id;
                next_responder->reference.layer_is_card = ACU_FALSE;
                
                *out_responder = xte_object_ref(in_engine, "bkgnd", next_responder,
                                                (XTEObjRefDeallocator)&_acu_variant_handle_destructor);
                break;
            }
            case STACKMGR_TYPE_BKGND:
            {
                /*next_responder = _stackmgr_handle_create(STACKMGR_FLAG_MAN_RELEASE);
                next_responder->reference.type = STACKMGR_TYPE_STACK;
                next_responder->reference.register_entry = in_stack;
                next_responder->reference.session_id = in_stack->session_id;
                next_responder->reference.layer_id = STACK_NO_OBJECT;
                next_responder->reference.layer_is_card = ACU_FALSE;
                
                *out_responder = xte_object_ref(in_engine, "stack", next_responder,
                                                (XTEObjRefDeallocator)&_acu_variant_handle_destructor);*/
                break;
            }
            case STACKMGR_TYPE_STACK:
            default:
                break;
        }
    }

    return XTE_ERROR_NONE;
}







static char const* _get_localized_class_name(XTE *in_engine, StackMgrStack *in_stack, char const *in_class_name)
{
    if (in_stack->cb_rslt_localized_string) free(in_stack->cb_rslt_localized_string);
    _acu_begin_single_thread_callback();
    in_stack->cb_rslt_localized_string = _g_acu.callbacks.localized_class_name(in_class_name);
    _acu_end_single_thread_callback();
    return in_stack->cb_rslt_localized_string;
}



void _acu_xt_progress(XTE *in_engine, StackMgrStack *in_stack, int in_paused, XTEVariant *in_handler_object, long in_source_line);


static struct XTECallbacks _xtalk_callbacks = {
    (XTEMessageResultCB)&_handle_message_result,
    (XTEScriptErrorCB)&_handle_script_error,
    (XTEScriptProgressCB)&_acu_xt_progress,
    
    (XTEDebugMessageCB) &_handle_debug_message,
    (XTEDebugVariableCB) &_handle_debug_variable,
    (XTEDebugHandlerCB) &_handle_debug_handler,
    
    (XTELocalizedClassNameCB) &_get_localized_class_name,
    
    (XTEDebugCheckpointCB) &_handle_script_checkpoint,
};


extern struct XTECommandDef _acu_xt_commands[];
extern struct XTEElementDef _acu_xt_elements[];
extern struct XTEClassDef _acu_xt_classes[];
extern struct XTEConstantDef _acu_xt_constants[];
extern struct XTEPropertyDef _acu_xt_properties[];
extern struct XTESynonymDef _acu_xt_synonyms[];
extern struct XTEFunctionDef _acu_xt_functions[];



int _stackmgr_xtalk_init(StackMgrStack *in_stack)
{
    in_stack->xtalk = xte_create(in_stack);
    if (!in_stack->xtalk) return ACU_FALSE;
    
    xte_configure_callbacks(in_stack->xtalk, _xtalk_callbacks);
    xte_configure_environment(in_stack->xtalk,
                              _acu_xt_classes,
                              _acu_xt_constants,
                              _acu_xt_properties,
                              _acu_xt_elements,
                              _acu_xt_functions,
                              _acu_xt_commands,
                              _acu_xt_synonyms);
    
    return ACU_TRUE;
}





