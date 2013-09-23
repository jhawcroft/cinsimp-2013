//
//  acu_xtalk_entry.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 17/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "acu_int.h"




/* referenced from both threads;
 handle management and auto-release pools need protection, ie. a mutex on specific handles;
 allocator is already protected */
void _acu_variant_handle_destructor(XTEVariant *in_variant, char *in_type, StackHandle in_handle)
{
    //printf("releasing stackhandle w/in variant\n");
    acu_release(in_handle); /// *******TODO ****** protect
}


XTEVariant* _acu_handle_to_variant(StackHandle in_handle)
{
    HandleDef *target_handle = (HandleDef*)in_handle;
    StackMgrStack *the_stack = target_handle->reference.register_entry;
    
    acu_retain(in_handle);
    
    switch (target_handle->reference.type)
    {
        case STACKMGR_TYPE_BUTTON:
            return xte_object_ref(the_stack->xtalk,
                                  (stack_widget_is_card(the_stack->stack, target_handle->reference.widget_id) ?
                                   "cdbtn" : "bgbtn"),
                                  in_handle,
                                  (XTEObjRefDeallocator)&_acu_variant_handle_destructor);
            
        case STACKMGR_TYPE_CARD:
            return xte_object_ref(the_stack->xtalk,
                                    "card",
                                    in_handle,
                                    (XTEObjRefDeallocator)&_acu_variant_handle_destructor);
    }
    
    return NULL;
}


/*
 *  _acu_post_system_event
 *  ---------------------------------------------------------------------------------------------
 *  Asyncronously posts a CinsTalk system event message, optionally with a single parameter.
 *  If the interpreter is currently busy, this call is ignored.
 *  Possibly in future, some events, specifically keyDown may elect to wait a short period to see
 *  if the event is handled quickly, and allow typing to be processed sensibly by the main event
 *  loop.  Otherwise, keyDown events will get lost.  Basically the wait should be only as long as
 *  it takes to display the script in progress window.  We should probably determine that
 *  delay within the ACU, since the ACU is supposed to be the responsible party not the UI ****
 *  (see acu.h for usage notes)
 *
 *  !  We retain target, copy name and take ownership of the parameter.
 *     Note the queue retains and copies everything when we post to it.
 */
static void _acu_post_system_event(StackMgrStack *in_stack, StackHandle in_target, char const *in_name, XTEVariant *in_param1)
{
    assert(in_target != NULL);
    assert(in_name != NULL);
    assert(*in_name != 0);
    
    /* are we not in browse mode? */
    if (in_stack->ut_disable_queue)
    {
        xte_variant_release(in_param1);
        return;
    }
    
    /* setup an event record */
    ACUEvent the_event;
    the_event.type = _ACU_EVENT_SYSTEM;
    the_event.message = (char*)in_name;
    the_event.target = _acu_handle_to_variant(in_target);
    assert(the_event.target != NULL);
    if (in_param1 != NULL)
    {
        the_event.param_count = 1;
        the_event.params[0] = in_param1;
    }
    else
        the_event.param_count = 0;
    
    /* lock the mutex */
    _acu_xt_comms_lock(in_stack);
    
    /* track execution state */
    _g_acu.active_script_count++;
    in_stack->xt_is_executing = ACU_TRUE;
    
    /* reset signals */
    in_stack->xt_sig_dbg_ctrl = _ACU_T_SIG_NONE;
    in_stack->ut_sig_script_error = _ACU_T_SIG_NONE;
    in_stack->ut_sig_script_chkpt = _ACU_T_SIG_NONE;
    
    /* enable auto-status */
    in_stack->auto_status_timer = 0;
    
    /* post the event */
    _acu_event_queue_post(in_stack->xtalk_sys_event_queue, &the_event);
    
    /* cleanup */
    xte_variant_release(the_event.target);
    xte_variant_release(in_param1);
    
    /* wakeup the xtalk thread if it's sleeping */
    _acu_xt_comms_unlock(in_stack);
    _acu_xt_wakeup(in_stack);
    
    /* increase frequency of main thread monitoring
     while xtalk is active */
    _acu_update_xtalk_timer_interval();
}


void _acu_post_idle(StackMgrStack *in_stack)
{
    //_g_acu.callbacks.stack_closed(in_stack->ui_context, 1); // for testing cAN be removed
    //return;
    
    
    /* disable auto-status */
    in_stack->auto_status_timer = -1;
    _g_acu.callbacks.auto_status_control(in_stack->ui_context, ACU_FALSE, 0);
    
    /* lodge "idle" event */
    StackHandle the_stack = _stackmgr_handle(in_stack);
    StackHandle the_card = acu_handle_for_card_id(STACKMGR_FLAG_MAN_RELEASE, the_stack, _stackmgr_current_card_id(in_stack));
    
    _acu_post_system_event(in_stack, the_card, "idle", NULL);

    acu_release(the_card);
    acu_release(the_stack);
}


/*
 *  acu_post_system_event
 *  ---------------------------------------------------------------------------------------------
 *  !  We retain target, copy name and take ownership of the parameter.
 */
void acu_post_system_event(StackHandle in_target, char const *in_name, XTEVariant *in_param1)
{
    _acu_post_system_event(_stackmgr_stack(in_target), in_target, in_name, in_param1);
}



void acu_message(StackHandle in_stack, char const *in_message)
{
    assert(in_stack != NULL);
    assert(in_message != NULL);
    
    /* are we not in browse mode? */
    //if (in_stack->ut_disable_queue) return;
    
    /* check we have a valid handle */
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    /* setup an event record */
    ACUEvent the_event;
    the_event.type = _ACU_EVENT_MESSAGE;
    the_event.message = (char*)in_message;
    StackHandle the_card = acu_handle_for_card_id(STACKMGR_FLAG_MAN_RELEASE, in_stack, stackmgr_current_card_id(in_stack));
    the_event.target = _acu_handle_to_variant(the_card);
    acu_release(the_card);
    assert(the_event.target != NULL);
    the_event.param_count = 0;
    
    /* lock the mutex */
    _acu_xt_comms_lock(the_stack);
    
    /* track execution state */
    _g_acu.active_script_count++;
    the_stack->xt_is_executing = ACU_TRUE;
    
    /* reset signals */
    the_stack->xt_sig_dbg_ctrl = _ACU_T_SIG_NONE;
    the_stack->ut_sig_script_error = _ACU_T_SIG_NONE;
    the_stack->ut_sig_script_chkpt = _ACU_T_SIG_NONE;
    
    /* enable auto-status */
    the_stack->auto_status_timer = 0;
    
    /* post the event */
    _acu_event_queue_post(the_stack->xtalk_sys_event_queue, &the_event);
    
    /* cleanup */
    xte_variant_release(the_event.target);
    
    /* wakeup the xtalk thread if it's sleeping */
    _acu_xt_comms_unlock(the_stack);
    _acu_xt_wakeup(the_stack);
    
    /* increase frequency of main thread monitoring
     while xtalk is active */
    _acu_update_xtalk_timer_interval();    
}


#define _ACU_UI_STACK_ACQUISITION_TIMEOUT_MSEC 100
#define _ACU_UI_STACK_ACQUISITION_WAIT_INTERVAL_MSEC 5

/*
 *  acu_ui_acquire_stack
 *  ---------------------------------------------------------------------------------------------
 *  Attempts to give the UI exclusive access to the stack within a short period of time
 *  (0.5 - 1.0 seconds).  If access can be obtained, returns ACU_TRUE, otherwise ACU_FALSE.
 *
 *  If the xtalk engine is accessing the stack, the UI cannot, as Stack is not protected by a 
 *  mutex, and it's against the design of CinsImp to allow the user to 'use' the stack while a
 *  script is executing.
 */
int acu_ui_acquire_stack(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    /* first, check if there's anything in the system event queue
     for the xTalk engine to execute; or if it's currently flagged as 
     executing */
    int acquired = ACU_FALSE;
    _acu_xt_comms_lock(the_stack);
    if ((the_stack->xt_is_executing) || (_acu_event_queue_count(the_stack->xtalk_sys_event_queue) > 0))
    {
        /* wait a short time and recheck if xTalk is still busy */
        _acu_xt_comms_unlock(the_stack);
        for (int i = 0; i < _ACU_UI_STACK_ACQUISITION_TIMEOUT_MSEC / _ACU_UI_STACK_ACQUISITION_WAIT_INTERVAL_MSEC; i++)
        {
            _acu_thread_usleep(_ACU_UI_STACK_ACQUISITION_WAIT_INTERVAL_MSEC * 1000);
            
            _acu_xt_comms_lock(the_stack);
            if ((the_stack->xt_is_executing) || (_acu_event_queue_count(the_stack->xtalk_sys_event_queue) > 0))
            {
                _acu_xt_comms_unlock(the_stack);
                continue;
            }
            else
            {
                _acu_xt_comms_unlock(the_stack);
                acquired = ACU_TRUE;
                break;
            }
        }
    }
    else
    {
        acquired = ACU_TRUE;
        _acu_xt_comms_unlock(the_stack);
    }
    
    /* check if we acquired exclusive access to the stack,
     ie. have all scripts finished executing and there's nothing
     waiting in the system event queue? */
    if (!acquired) return ACU_FALSE;
    
    /* disable system event processing while the UI is busy;
     but still allow events to be queued? *** CHECK *** */
    _acu_xt_comms_lock(the_stack);
    the_stack->xt_disable_processing = ACU_TRUE;
    _acu_xt_comms_unlock(the_stack);
    
    return ACU_TRUE;
}


/*
 *  acu_ui_relinquish_stack
 *  ---------------------------------------------------------------------------------------------
 *  Relinquishes control of the stack, returning control to the xTalk engine and ACU.
 *  (partnered with an initial call to acu_ui_acquire_stack).
 *
 *  Also automatically called at application idle.
 */
void acu_ui_relinquish_stack(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    /* re-enable queue processing */
    int has_events = ACU_FALSE;
    _acu_xt_comms_lock(the_stack);
    the_stack->xt_disable_processing = ACU_FALSE;
    has_events = (_acu_event_queue_count(the_stack->xtalk_sys_event_queue) > 0);
    if (has_events)
    {
        _g_acu.active_script_count++;
        the_stack->xt_is_executing = ACU_TRUE;
    }
    _acu_xt_comms_unlock(the_stack);
    
    /* restart queue processing if necessary */
    if (has_events)
    {
        _acu_xt_wakeup(the_stack);
        
        /* increase frequency of main thread monitoring
         while xtalk is active */
        _acu_update_xtalk_timer_interval();
    }
}




