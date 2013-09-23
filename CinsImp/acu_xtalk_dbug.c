//
//  acu_xtalk_dbug.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 20/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "acu_int.h"


/*
 *  _acu_enter_debugger
 *  ---------------------------------------------------------------------------------------------
 *  Notifies the application we have entered/resumed debug mode; providing a currently running
 *  script object and source line number.
 */
void _acu_enter_debugger(StackMgrStack *in_stack)
{
    //printf("DEBUGGER ENTER/RESUME\n");
    in_stack->ut_debug_suspended_closure = ACU_FALSE;
    _g_acu.callbacks.is_debugging(in_stack->ui_context, ACU_TRUE,
                                  in_stack->ut_script_error_object,
                                  in_stack->thread_xte_source_line);
}


/*
 *  _acu_exit_debugger
 *  ---------------------------------------------------------------------------------------------
 *  Notifies the application we have exited debug mode.
 */
void _acu_exit_debugger(StackMgrStack *in_stack)
{
    //printf("DEBUGGER LEAVE\n");
    in_stack->ut_debug_suspended_closure = ACU_FALSE;
    _g_acu.callbacks.is_debugging(in_stack->ui_context, ACU_FALSE, ACU_INVALID_HANDLE, 0);
}


/*
 *  _acu_suspend_debugger
 *  ---------------------------------------------------------------------------------------------
 *  Notifies the application we have suspended debug mode; this means the debug menu should still
 *  be available, but the currently executing line is unknown (in version 1.0 anyway).
 */
void _acu_suspend_debugger(StackMgrStack *in_stack)
{
    //printf("DEBUGGER SUSPEND\n");
    in_stack->ut_debug_suspended_closure = ACU_TRUE;
    _g_acu.callbacks.is_debugging(in_stack->ui_context, ACU_TRUE, ACU_INVALID_HANDLE, 0);
}




void _acu_check_exit_debugger(StackMgrStack *in_stack)
{
    if (in_stack->thread_is_debuggable)
    {
        in_stack->thread_is_debuggable = ACU_FALSE;
        in_stack->ut_has_error_halt_messages = ACU_FALSE;
        _acu_exit_debugger(in_stack);
    }
}



StackHandle stackmgr_script_error_target(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    return the_stack->ut_script_error_object;
}


long stackmgr_script_error_source_line(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    return the_stack->thread_xte_source_line;
}



void acu_script_editor_opened(StackHandle in_object)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_object);
    assert(the_stack != NULL);
    
    the_stack->ut_script_editor_count++;
}


void acu_script_editor_closed(StackHandle in_object)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_object);
    assert(the_stack != NULL);
    
    the_stack->ut_script_editor_count--;
    
    /* care must obviously be taken around Windows and ARC;
     the below code was a nice idea but caused issues.
     instead, we get the UI to decide if the window closure
     was initiated by the user.  if it is, we call the ACU
     with the normal acu_script_abort().
     and cancel the user's window closure, allowing the ACU
     to initiate the final automatic closure */
    
    /*if ((the_stack->ut_script_editor_count == 0) && acu_script_is_active(in_object)
        && (!the_stack->ut_debug_suspended_closure))
        acu_script_abort(in_object); // seems to be causing runtime errors for the objective-c code */
}




void acu_script_abort(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    the_stack->xt_halt_code = _ACU_HALT_NONE;
    the_stack->ut_has_error_halt_messages = ACU_FALSE;
    _acu_check_exit_debugger(the_stack);
    
    _acu_thread_mutex_lock(&(the_stack->xt_mutex_comms));
    the_stack->xt_sig_dbg_ctrl = _ACU_T_SIG_ABORT;
    _acu_thread_mutex_unlock(&(the_stack->xt_mutex_comms));
    _acu_xt_wakeup(the_stack);
}




void acu_script_debug(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    _acu_thread_mutex_lock(&(the_stack->xt_mutex_comms));
    int has_halted = (the_stack->xt_halt_code != _ACU_HALT_NONE);
    _acu_thread_mutex_unlock(&(the_stack->xt_mutex_comms));
    
    if ((has_halted) && (!the_stack->thread_is_debuggable))
    {
        the_stack->thread_is_debuggable = ACU_TRUE;
        _acu_enter_debugger(the_stack);
    }
}


int acu_script_is_resumable(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    return (the_stack->xt_halt_code != _ACU_HALT_ERROR);
}


int acu_script_is_active(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    return the_stack->xt_is_executing;
}


void acu_script_continue(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    _acu_suspend_debugger(the_stack);
    
    _acu_thread_mutex_lock(&(the_stack->xt_mutex_comms));
    the_stack->xt_halt_code = _ACU_HALT_NONE;
    the_stack->xt_sig_dbg_ctrl = _ACU_T_SIG_RUN;
    _acu_thread_mutex_unlock(&(the_stack->xt_mutex_comms));
    _acu_xt_wakeup(the_stack);
}


void acu_debug_step_over(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    _acu_suspend_debugger(the_stack);
    
    _acu_thread_mutex_lock(&(the_stack->xt_mutex_comms));
    the_stack->xt_halt_code = _ACU_HALT_NONE;
    the_stack->xt_sig_dbg_ctrl = _ACU_T_SIG_STEP;
    _acu_thread_mutex_unlock(&(the_stack->xt_mutex_comms));
    _acu_xt_wakeup(the_stack);
}


void acu_debug_step_into(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    _acu_suspend_debugger(the_stack);
    
    _acu_thread_mutex_lock(&(the_stack->xt_mutex_comms));
    the_stack->xt_halt_code = _ACU_HALT_NONE;
    the_stack->xt_sig_dbg_ctrl = _ACU_T_SIG_INTO;
    _acu_thread_mutex_unlock(&(the_stack->xt_mutex_comms));
    _acu_xt_wakeup(the_stack);
}


void acu_debug_step_out(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    _acu_suspend_debugger(the_stack);
    
    _acu_thread_mutex_lock(&(the_stack->xt_mutex_comms));
    the_stack->xt_halt_code = _ACU_HALT_NONE;
    the_stack->xt_sig_dbg_ctrl = _ACU_T_SIG_OUT;
    _acu_thread_mutex_unlock(&(the_stack->xt_mutex_comms));
    _acu_xt_wakeup(the_stack);
}


