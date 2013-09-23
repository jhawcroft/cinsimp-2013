/*
 
 Application Control Unit - xTalk Thread
 acu_xtalk_thread.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Separate thread for xTalk execution.
 
 Permits the user interface to remain under user-control while scripts are in progress, and 
 for the user to continue browsing/mutating other open stacks while a long script runs in another
 open stack.
 
 *************************************************************************************************
 */

#include "acu_int.h"


#define _THREAD_MONITORING_INTERVAL_USECS 500000
#define _THREAD_TERMINATION_TIMEOUT_USECS 5000000



/** XT **********************************************************************************/


/******************
 Utilities
 */

/*
 *  _acu_xt_card_needs_repaint
 *  ---------------------------------------------------------------------------------------------
 *  Signal from within XT that the current card displayed by the UI should be refreshed/repainted
 *  optionally, immediately if <in_now> is ACU_TRUE.
 *
 *  If the refresh is requested immediately, blocks until it is completed.
 *
 *  If the refresh is requested delayed, it occurs at completion of script execution when the
 *  interpreter next becomes idle.
 */
void _acu_xt_card_needs_repaint(StackMgrStack *in_stack, int in_now)
{
    if (in_now)
    {
        void _acu_ut_refresh_card();
        _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_refresh_card);
    }
    else in_stack->idle_repaint_required = ACU_TRUE;
}


/*
 *  _acu_xt_post_sys_event
 *  ---------------------------------------------------------------------------------------------
 *  Post a system event message to the running xTalk engine from within the XT thread.
 *
 *  Invoked by XT in response to messages posted to the queue by UT, and may be invoked by XT
 *  glue code in response to particular commands, such as go, which post system messages such as
 *  close card, open card, etc.
 */
void _acu_xt_post_sys_event(StackMgrStack *in_stack, ACUEvent *in_event)
{
    /* invoke the xtalk engine */
    xte_post_system_event(in_stack->xtalk, in_event->target, in_event->message, in_event->params, in_event->param_count);
}




/******************
 Event Loop
 */

/*
 *  _acu_xt_can_sleep
 *  ---------------------------------------------------------------------------------------------
 *  Checks for TERM, EVENT, DCR, DIR and DMR; if anything is outstanding returns ACU_FALSE,
 *  otherwise ACU_TRUE.
 *
 *  Allows XT to avoid going to sleep if there are still things to do.
 *
 *  ! Assumes a lock has already been acquired on the communications mutex.
 */
int _acu_xt_can_sleep(StackMgrStack *in_stack)
{
    return ((in_stack->xt_sig_demand_terminate == _ACU_T_SIG_NONE) &&
            (_acu_event_queue_count(in_stack->xtalk_sys_event_queue) == 0) &&
            (in_stack->xt_sig_sic != _ACU_T_SIG_ACK) &&
            (in_stack->xt_sig_dbg_ctrl == _ACU_T_SIG_NONE));
}


/*
 *  _acu_xt_sleep
 *  ---------------------------------------------------------------------------------------------
 *  Puts the thread to sleep if there isn't anything to do.
 *
 *  ! Assumes a lock has already been acquired on the communications mutex.
 */
void _acu_xt_sleep(StackMgrStack *in_stack)
{
    if (!_acu_xt_can_sleep(in_stack)) return;
    in_stack->xt_is_sleeping = ACU_TRUE;
    _acu_thread_cond_wait(&(in_stack->xt_wakeup), &(in_stack->xt_mutex_comms));
    in_stack->xt_is_sleeping = ACU_FALSE;
}



/*
 *  _acu_xt_should_terminate
 *  ---------------------------------------------------------------------------------------------
 *  Checks for TERM and returns ACU_TRUE if set, ACU_FALSE otherwise.
 *
 *  ! Assumes a lock has already been acquired on the communications mutex.
 */
int _acu_xt_should_terminate(StackMgrStack *in_stack)
{
    if (in_stack->xt_sig_demand_terminate == _ACU_T_SIG_REQ)
    {
        /* XT terminating; signal engine should abort */
        xte_abort(in_stack->xtalk);
        return ACU_TRUE;
    }
    return ACU_FALSE;
}


/*
 *  _acu_xt_handle_dirs_and_dmrs
 *  ---------------------------------------------------------------------------------------------
 *  Checks for and handles queued DIRs (Debug Information Requests) and DMRs (Debug Mutation
 *  Requests).
 *
 *  Keeps an eye-out for TERM and terminates if it occurs.
 *
 *  ! Assumes a lock has already been acquired on the communications mutex.
 */
void _acu_xt_handle_dirs_and_dmrs(StackMgrStack *in_stack)
{
    if (in_stack->xt_sig_dbg_ireq == _ACU_T_SIG_REQ)
    {
        in_stack->xt_sig_dbg_ireq = _ACU_T_SIG_NONE;
        xte_debug_enumerate_handlers(in_stack->xtalk);
        //xte_debug_enumerate_variables(in_stack->xtalk, XTE_CONTEXT_CURRENT); // done by the handler enumeration anyway
    }
    if (in_stack->xt_sig_dbg_imut == _ACU_T_SIG_REQ)
    {
        in_stack->xt_sig_dbg_imut = _ACU_T_SIG_NONE;
        xte_debug_mutate_variable(in_stack->xtalk, in_stack->xt_imut_var_name, XTE_CONTEXT_CURRENT, in_stack->xt_imut_var_value);
    }
}



/*
 _handle_var_mutate_req(in_stack);
 if (in_stack->xt_sig_dbg_ireq == _ACU_T_SIG_REQ)
 {
 asyncronous debug information request;
 send response via SIC 
_acu_xt_comms_unlock(in_stack);
//xte_debug_enumerate_handlers(in_stack->xtalk);
//xte_debug_enumerate_variables(in_stack->xtalk, in_stack->xt_ireq_handler_index);

in_stack->xt_sig_dbg_ireq = _ACU_T_SIG_NONE;  nothing can change during responding,
                                              so it's safe to cancel the request 
_acu_xt_comms_lock(in_stack);
}
*/

/* superceeded by above */
static void _handle_var_mutate_req(StackMgrStack *in_stack)
{
    if (in_stack->xt_sig_dbg_imut == _ACU_T_SIG_REQ)
    {
        /* syncronous debug information mutate request;
         must respond outside of lock as callback will use SIC */
        
        char *name, *value;
        int index;
        index = in_stack->xt_imut_handler_index;
        name = _acu_clone_cstr(in_stack->xt_imut_var_name);
        value = _acu_clone_cstr(in_stack->xt_imut_var_value);
        
        in_stack->xt_sig_dbg_imut = _ACU_T_SIG_ACK; /* acknowledge the syncronous request */
        _acu_xt_comms_unlock(in_stack);
        
        xte_debug_mutate_variable(in_stack->xtalk, name, index, value);
        
        free(name);
        free(value);
        
        _acu_xt_comms_lock(in_stack);
    }
}





/*
 *  _acu_xt_progress
 *  ---------------------------------------------------------------------------------------------
 *  XT progress routine - callback invoked by the xTalk engine while a script is running.
 *
 *  Looks for high priority signals:
 *  -   xt_sig_demand_terminate
 *
 *  Looks for messages in the system event queue:
 *  -
 *
 */
void _acu_xt_progress(XTE *in_engine, StackMgrStack *in_stack, int in_paused, XTEVariant *in_handler_object, long in_source_line)
{
    _acu_xt_comms_lock(in_stack);
    
    
    /*
     check for high-priority signals
     */
    
    if (_acu_xt_should_terminate(in_stack))
    {
        _acu_xt_comms_unlock(in_stack);
        return;
    }
    
    /* asyncronous debug control signal */
    if (in_stack->xt_sig_dbg_ctrl != _ACU_T_SIG_NONE)
    {
        switch (in_stack->xt_sig_dbg_ctrl)
        {
            case _ACU_T_SIG_ABORT:
                xte_abort(in_engine);
                break;
            case _ACU_T_SIG_RUN:
                if (in_paused)
                    xte_continue(in_engine);
                break;
            case _ACU_T_SIG_STEP:
                if (in_paused)
                    xte_debug_step_over(in_engine);
                break;
            case _ACU_T_SIG_INTO:
                if (in_paused)
                    xte_debug_step_into(in_engine);
                break;
            case _ACU_T_SIG_OUT:
                if (in_paused)
                    xte_debug_step_out(in_engine);
                break;
            default:
                assert(0);
        }
        in_stack->xt_sig_dbg_ctrl = _ACU_T_SIG_NONE;
        in_paused = ACU_FALSE;
    }
    
    _acu_xt_handle_dirs_and_dmrs(in_stack);
    
    
    /* if there's nothing to do and we're paused deliberately, go to sleep
     and reduce CPU cycles */
    if (in_paused)
        _acu_xt_sleep(in_stack);
    
    _acu_xt_comms_unlock(in_stack);
}


/*
 *  _acu_xtalk_thread_loop
 *  ---------------------------------------------------------------------------------------------
 *  Event loop for XT.
 *
 *  Looks for high priority signals:
 *  -   xt_sig_demand_terminate
 *
 *  Looks for messages in the system event queue:
 *  -   
 *
 *  If there are no signals, and no communications of any kind with XT, goes to sleep.  Thread is
 *  woken up by UT with a call to _acu_xt_wakeup().
 */
static void* _acu_xtalk_thread_loop(StackMgrStack *in_stack)
{
#if DEBUG
    printf("XT started.\n");
#endif
    for (;;)
    {
        _acu_xt_comms_lock(in_stack);
        
        /*
         consider going to sleep
         */
        
        /* check if there is no communications/signalling of any kind */
        if (_acu_xt_can_sleep(in_stack))
        {
            /* sleep until woken up */
            in_stack->xt_is_executing = ACU_FALSE;
            _acu_xt_sleep(in_stack);
        }
        
        
        /*
         check for high-priority signals
         */
        
        if (in_stack->xt_sig_demand_terminate == _ACU_T_SIG_REQ)
        {
            /* XT terminate */
            in_stack->xt_sig_demand_terminate = _ACU_T_SIG_ACK;
            _acu_xt_comms_unlock(in_stack);
#if DEBUG
            printf("XT terminated.\n");
#endif
            _acu_thread_exit();
            return NULL;
        }
        
        _acu_xt_handle_dirs_and_dmrs(in_stack);
        
        
        /*
         check for work in the system event queue
         (note: nested system events are posted directly by the XT-side glue code routine
         and not via the external event queue, whose purpose is to accumulate events while
         processing is occurring)
         */
        
        if (!in_stack->xt_disable_processing)
        {
            ACUEvent *the_event = _acu_event_queue_next(in_stack->xtalk_sys_event_queue);
            if (the_event)
            {
                _acu_xt_comms_unlock(in_stack); /* starting new work has side-effects which may
                                                 require the lock */
                switch (the_event->type)
                {
                    case _ACU_EVENT_SYSTEM:
                        _acu_xt_post_sys_event(in_stack, the_event);
                        break;
                    case _ACU_EVENT_MESSAGE:
                        xte_message(in_stack->xtalk, the_event->message, the_event->target);
                        break;
                        
                    default: assert(0);
                }
                continue;
            }
        }
        
        
        _acu_xt_comms_unlock(in_stack);
    }
    
    /* abnormal thread termination */
    assert(0);
}




/******************
 Inter-thread Communications
 */

/*
 *  _acu_xt_sic
 *  ---------------------------------------------------------------------------------------------
 *  A syncronous operational message to UT has been composed.  Send the message and wait for a 
 *  result.
 *
 *  ! Note that the wakeup may not be related to the message, eg. high priority signals such as
 *    xt_sig_demand_terminate must always be checked on exit.
 */
void _acu_xt_sic(StackMgrStack *in_stack, ACUImplementor in_implementor)
{
    /* don't do anything if we've been ordered to terminate */
    _acu_xt_comms_lock(in_stack);
    if (in_stack->xt_sig_demand_terminate == _ACU_T_SIG_REQ)
    {
        _acu_xt_comms_unlock(in_stack);
        return;
    }
    assert(in_stack->xt_sig_sic == _ACU_T_SIG_NONE);
    
    /* post SIC message to UT */
    in_stack->xt_sic.implementor = in_implementor;
    in_stack->xt_sig_sic = _ACU_T_SIG_REQ;
    
    /* go to sleep while we wait for an acknowledgement or response */
    while (in_stack->xt_sig_sic != _ACU_T_SIG_ACK)
    {
        _acu_thread_cond_wait(&(in_stack->xt_wakeup), &(in_stack->xt_mutex_comms));
        
        /* look for signals other than our response;
         we can be preempted successfully by: TERM, DIR, DMR
         we can't be preempted by: EVENT, DCR */
        if (_acu_xt_should_terminate(in_stack))
        {
            _acu_xt_comms_unlock(in_stack);
            return;
        }
    
        _acu_xt_handle_dirs_and_dmrs(in_stack);
    }
    
    /* clear the request */
    in_stack->xt_sig_sic = _ACU_T_SIG_NONE;
    _acu_xt_comms_unlock(in_stack);
}



/** ACU Thread **********************************************************************************/


/******************
 Startup and Shutdown
 */

/*
 *  _acu_xtalk_thread_startup
 *  ---------------------------------------------------------------------------------------------
 *  Creates and starts an xtalk thread.
 */
int _acu_xtalk_thread_startup(StackMgrStack *in_stack)
{
    /* create locking and control mechanisms */
    if (!_acu_thread_mutex_create(&(in_stack->xt_mutex_comms)))
        return ACU_FALSE;
    if (!_acu_thread_cond_create(&(in_stack->xt_wakeup)))
        return ACU_FALSE;
    
    /* create and start the thread itself */
    if (!_acu_thread_create(&(in_stack->thread_xte), &_acu_xtalk_thread_loop, in_stack, ACU_STACK_SIZE_XTE))
        return ACU_FALSE;
    
    /* return success */
    return ACU_TRUE;
}


/*
 *  _acu_xtalk_thread_shutdown
 *  ---------------------------------------------------------------------------------------------
 *  Requests shutdown of an xtalk thread, waits for completion and disposes of the thread.
 */
void _acu_xtalk_thread_shutdown(StackMgrStack *in_stack)
{
    if (!in_stack->thread_xte) return;
    
    /* signal the thread to exit */
    _acu_xt_comms_lock(in_stack);
    in_stack->xt_sig_demand_terminate = _ACU_T_SIG_REQ;
    _acu_xt_comms_unlock(in_stack);
    _acu_xt_wakeup(in_stack);
    
    /* wait for the thread to exit */
    int terminated = ACU_FALSE;
    for (int i = 0; i < _THREAD_TERMINATION_TIMEOUT_USECS; i += _THREAD_MONITORING_INTERVAL_USECS)
    {
        _acu_thread_usleep(_THREAD_MONITORING_INTERVAL_USECS);
        _acu_xt_wakeup(in_stack);
        
        _acu_xt_comms_lock(in_stack);
        if (in_stack->xt_sig_demand_terminate == _ACU_T_SIG_ACK)
        {
            /* thread exited successfully */
            _acu_xt_comms_unlock(in_stack);
            terminated = ACU_TRUE;
            break;
        }
        _acu_xt_comms_unlock(in_stack);
    }
    
    /* if the thread didn't terminate,
     raise a fatal internal error */
    if (!terminated)
    {
        _acu_raise_error(ACU_ERROR_INTERNAL);
        return;
    }
    
    /* dispose of thread, mutex and condition */
    _acu_thread_mutex_dispose(&(in_stack->xt_mutex_comms));
    _acu_thread_cond_dispose(&(in_stack->xt_wakeup));
}




/** Shared ***************************************************************************************/


/******************
 Syncronisation and Signalling
 */

/*
 *  _acu_xt_comms_lock
 *  ---------------------------------------------------------------------------------------------
 *  Acquire a lock on the Communications channels between UT and XT.
 *
 *  May raise an internal error in the unlikely event there is a problem.
 */
void _acu_xt_comms_lock(StackMgrStack *in_stack)
{
    _acu_thread_mutex_lock(&(in_stack->xt_mutex_comms));
}


/*
 *  _acu_xt_comms_unlock
 *  ---------------------------------------------------------------------------------------------
 *  Relinquish a lock on the Communications channels between UT and XT.
 */
void _acu_xt_comms_unlock(StackMgrStack *in_stack)
{
    _acu_thread_mutex_unlock(&(in_stack->xt_mutex_comms));
}


/*
 *  _acu_xt_wakeup
 *  ---------------------------------------------------------------------------------------------
 *  Wake up XT if it is sleeping.  Otherwise, does nothing.
 *
 *  ! Ensure communications channels are not locked, otherwise this will cause a deadlock.
 */
void _acu_xt_wakeup(StackMgrStack *in_stack)
{
    _acu_thread_mutex_lock(&(in_stack->xt_mutex_comms));
    _acu_thread_cond_signal(&(in_stack->xt_wakeup));
    _acu_thread_mutex_unlock(&(in_stack->xt_mutex_comms));
}






