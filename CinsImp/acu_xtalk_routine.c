
//
//  acu_xtalk_routine.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 19/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "acu_int.h"


void _acu_post_idle(StackMgrStack *in_stack);

/*
 *  _acu_xtalk_poll
 *  ---------------------------------------------------------------------------------------------
 *  Polls the xtalk engine routinely, to check if it's busy.  If it isn't, automatically posts
 *  the "idle" system message.
 *
 *  If the engine is busy, checks to see if it has made a request of the main application (UI)
 *  thread and handle any such request/notification.
 *
 *  Also drains the idle autorelease pool each time the xtalk engine idles.
 *
 *  <in_not_idle> disables idle processing for this call and is invoked when called in rapid
 *  succession while an xTalk engine is executing.
 */
void _acu_xtalk_poll(StackMgrStack *in_stack, int in_not_idle)
{
    assert(in_stack != NULL);
    
    /*
     A Note About This Function:
     One of the central reasons why we're checking flags the way we are,
     at the beginning and then performing associated actions at the end
     is to ensure that memory syncronisation occurs on all the relevant
     data.
     
     The other reasons are to allow the shortest possible time holding the
     lock, and to ensure we can acquire the lock again for some responses.
     */
    
    
    /*
     Gather Thread Communications/Runtime State
     */
    
    _acu_xt_comms_lock(in_stack);
    
    /* check a bunch of conditions and set flags
     so we can deal with those conditions after the 
     communications lock has been released */
    int got_error = ACU_FALSE, is_idle = ACU_FALSE, got_sic = ACU_FALSE, got_chkpt = ACU_FALSE;
    int got_dir_ic = ACU_FALSE;
    ACU_IC sic, dir_ic;
    
    /* check for a message on SIC */
    if ((in_stack->xt_sig_sic == _ACU_T_SIG_REQ) && (in_stack->xt_sic.implementor))
    {
        got_sic = ACU_TRUE;
        sic = in_stack->xt_sic;
        in_stack->xt_sic.implementor = NULL;
    }
    
    /* check for a message on DIR_IC;
     must be handled while locked, as the parameters
     may be overwritten otherwise */
    if ((in_stack->ut_sig_dir_ic == _ACU_T_SIG_REQ) && (in_stack->ut_dir_ic.implementor))
    {
        //// SHOULDNT BE A DIR IC - should just be a bunch of variables - we're not using it as it was intended ************* TODO ****
        got_dir_ic = ACU_TRUE;
        dir_ic = in_stack->ut_dir_ic;
        in_stack->ut_dir_ic.implementor = NULL;
        
        in_stack->ut_dir_ic.param[0].value.pointer = NULL;
        in_stack->ut_dir_ic.param[2].value.pointer = NULL;
    }
    
    /* check for script errors */
    if (in_stack->ut_sig_script_error == _ACU_T_SIG_NOTICE)
    {
        in_stack->ut_sig_script_error = _ACU_T_SIG_NONE;
        got_error = ACU_TRUE;
        in_stack->ut_has_error_halt_messages = ACU_TRUE;
    }
    
    /* check for checkpoint */
    if (in_stack->ut_sig_script_chkpt == _ACU_T_SIG_NOTICE)
    {
        in_stack->ut_sig_script_chkpt = _ACU_T_SIG_NONE;
        got_chkpt = ACU_TRUE;
    }
    
    /* check if the xTalk engine is idle */
    is_idle = ((!in_stack->xt_is_executing) && (!in_not_idle) &&
               (!in_stack->ut_has_error_halt_messages) &&
               (in_stack->ut_script_editor_count == 0));
    
    in_stack->xt_is_executing = in_stack->xt_is_executing;
    
    _acu_xt_comms_unlock(in_stack);
    
    
    
    /*
     Act on Thread Communications/Runtime State
     */
    
    /* handle SIC message */
    if (got_sic)
        sic.implementor(in_stack, sic.param, sic.param_count); /* note that access of pointers should POSSIBLY ??
                                                                be done within a mutex lock to ensure memory syncronisation */
    
    /* handle DIR IC message */
    if (got_dir_ic)
    {
        dir_ic.implementor(in_stack, dir_ic.param, dir_ic.param_count); /* note that access of pointers should POSSIBLY ??
                                                                         be done within a mutex lock to ensure memory syncronisation */
        if (dir_ic.param[0].value.pointer) free(dir_ic.param[0].value.pointer);
        if (dir_ic.param[2].value.pointer) free(dir_ic.param[2].value.pointer);
    }
    
    /* deal with conditions for which flags
     were previously set, now that the communications
     lock has been released */
    if (got_error)
    {
        // can fix this warning by providing our own script error callback **** TODO****
        in_stack->xt_halt_code = _ACU_HALT_ERROR;
        _g_acu.callbacks.script_error(in_stack->ui_context,
                                      in_stack->ut_script_error_object,
                                      in_stack->thread_xte_source_line,
                                      in_stack->thread_xte_error_template,
                                      in_stack->thread_xte_error_arg1,
                                      in_stack->thread_xte_error_arg2,
                                      in_stack->thread_xte_error_arg3,
                                      in_stack->thread_xte_error_is_runtime);
    }
    
    /* check if the xTalk engine is idle */
    if (is_idle)
    {
        /* exit the debugger */
        in_stack->xt_halt_code = _ACU_HALT_NONE;
        _acu_check_exit_debugger(in_stack);
        
        /* drain idle autorelease pool */
        _acu_autorelease_pool_drain(in_stack->arpool_idle);
        
        /* unlock the screen (if locked) */
        if (in_stack->effect_queue_playback == 0)
        {
            if (in_stack->lock_screen > 0)
            {
                in_stack->lock_screen = 1;
                _acu_unlock_screen(in_stack, ACU_FALSE);
            }
            
            in_stack->effect_queue_count = 0;
            
            if (in_stack->idle_repaint_required)
            {
                in_stack->idle_repaint_required = ACU_FALSE;
                _g_acu.callbacks.view_refresh(in_stack->ui_context);
            }
        }
        
        /* generate "idle" system message
         provided there is no mouse/touch going on */
        if (!_g_acu.ut_finger_tracking)
            _acu_post_idle(in_stack);
    }
    
    /* open script editor to display checkpoint */
    if (got_chkpt)
    {
        in_stack->xt_halt_code = _ACU_HALT_CHKPT;
        in_stack->thread_is_debuggable = ACU_TRUE;
        _acu_enter_debugger(in_stack);
    }

    /********** REFACTORING OLD CODE OUT:  **********/
    
/*
    has_halted = ((in_stack->thread_xte_halt_code != _THREAD_XTE_HALT_NONE));
    if (!has_halted)
    {
        if (in_stack->thread_is_debuggable)
        {
            in_stack->thread_is_debuggable = ACU_FALSE;
            _acu_exit_debugger(in_stack);
        }
    }
   */

}


void _acu_xtalk_callback_respond(StackMgrStack *in_stack)
{
    /* post acknowledgement and signal thread to wakeup */
    _acu_xt_comms_lock(in_stack);
    assert(in_stack->xt_sig_sic == _ACU_T_SIG_REQ);
    in_stack->xt_sig_sic = _ACU_T_SIG_ACK;
    _acu_thread_cond_signal(&(in_stack->xt_wakeup));
    _acu_xt_comms_unlock(in_stack);
}



/*
 *  stackmgr_script_running
 *  ---------------------------------------------------------------------------------------------
 *  Returns ACU_TRUE if the specified stack is currently executing a CinsTalk script.
 *
 *  All UI event handlers should check this before allowing the user to interact with the
 *  specified stack.  If it's true, the user must not be allowed to interact with the stack
 *  (the running xTalk engine has exclusive access to the stack at this time.)
 
 
 !!! DEPRECIATED BEHAVIOUR 1.0 shall allow the user to interact with the UI in favor of mutex
 locking of the underlying stack file and appropriate guards by enforcing a go through ACS policy.
 
 That or force UI to wait as was recently thinking
 
 Still need to ask if a script is running to display a status window, assuming I even have one?
 Maybe a badge?
 
 */
int stackmgr_script_running(StackHandle in_stack)
{
    StackMgrStack *register_entry = _stackmgr_stack(in_stack);
    assert(register_entry != NULL);
    
    return register_entry->xt_is_executing;
}






