//
//  stackmgr.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 6/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "acu_int.h"


/******************
 Internal State
 */

struct ACU _g_acu = { 0 };



/******************
 Initialization
 */

int acu_init(ACUCallbacks *in_callbacks)
{
    assert(_g_acu.inited == 0);
    
    memset(&_g_acu, 0, sizeof(struct ACU));
    _g_acu.inited = ACU_TRUE;
    _g_acu.callbacks = *in_callbacks;
    if (!_acu_thread_mutex_create(&(_g_acu.mem_mutex))) return ACU_FALSE;
    
    _g_acu.ar_pool = _acu_autorelease_pool_create();
    if (!_g_acu.ar_pool) return ACU_FALSE;
    if (!_acu_thread_mutex_create(&(_g_acu.ar_pool_mutex))) return ACU_FALSE;
    
    if (!_acu_thread_mutex_create(&(_g_acu.non_reentrant_callback_m))) return ACU_FALSE;
    
    if (!_acu_thread_mutex_create(&(_g_acu.debug_information_m))) return ACU_FALSE;
    
    _g_acu.ar_callback_results = _acu_autorelease_pool_create();
    if (!_g_acu.ar_callback_results) return ACU_FALSE;
    if (!_acu_thread_mutex_create(&(_g_acu.ar_callback_result_mutex))) return ACU_FALSE;
    
    if (!_load_builtin_resources()) return ACU_FALSE;
    
    return ACU_TRUE;
}


/******************
 Application Termination
 */

void acu_quit(void)
{
    // opportunity to dispose of some stuff and check for memory leaks, etc.
    
    printf("Terminating ACU\n");
    
    if (_g_acu.builtin_resources)
        stack_close(_g_acu.builtin_resources);
    
    //_acu_thread_exit();
}


/******************
 Timers
 */

static void _acu_idle(void)
{
   void _stackmgr_idle(int in_xtalk_only);
    _stackmgr_idle(ACU_FALSE);
    
    
    void stackmgr_arp_drain(void);
    //void stackmgr_scripting_idle(void);
    //stackmgr_scripting_idle();
    stackmgr_arp_drain();
    
    _acu_thread_mutex_lock(&(_g_acu.debug_information_m));
    if (_g_acu.ut_sig_debug_info_changed)
    {
        _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
        _g_acu.callbacks.debug_vars_changed();
    }
    else
        _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
    
}


void acu_xtalk_timer(void)
{
    void _stackmgr_idle(int in_xtalk_only);
    _stackmgr_idle(ACU_TRUE);
}


void _acu_update_xtalk_timer_interval(void)
{
    if ((_g_acu.active_script_count > 0) && (!_g_acu.xtalk_is_active))
    {
        _g_acu.xtalk_is_active = ACU_TRUE;
        //printf("xTalk is active\n");
        _g_acu.callbacks.adjust_timers(ACU_TRUE);
    }
    else if ((_g_acu.active_script_count <= 0) && (_g_acu.xtalk_is_active))
    {
        _g_acu.xtalk_is_active = ACU_FALSE;
         //printf("xTalk is INactive\n");
        _g_acu.callbacks.adjust_timers(ACU_FALSE);
    }
}


void acu_source_indent(char const *in_source, long *io_selection_start, long *io_selection_length,
                       XTEIndentingHandler in_indenting_handler, void *in_context)
{
    xte_source_indent(_g_acu.current_stack->xtalk, in_source, io_selection_start, io_selection_length, in_indenting_handler, in_context);
}


#define _ACU_IDLE_TIMER_THRESHOLD 1


/* primary ACU timer, going forward */
void acu_timer(void)
{
    _g_acu.idle_gen_timer++;
    if (_g_acu.idle_gen_timer >= _ACU_IDLE_TIMER_THRESHOLD)
    {
        _g_acu.idle_gen_timer = 0;
        _acu_idle();
    }
    
    
    
    
}



/*******
 Thread Safety
 */


/*
 Some callbacks are only designed to work when called by a single thread at a time,
 ie. the functionality is non-reentrant. Such as mutate_rtf.
 These can guard those instances and protect against weirdness.
 */

void _acu_begin_single_thread_callback(void)
{
    _acu_thread_mutex_lock(&(_g_acu.non_reentrant_callback_m));
}


void _acu_end_single_thread_callback(void)
{
    _acu_dispose_callback_results();
    _acu_thread_mutex_unlock(&(_g_acu.non_reentrant_callback_m));
}



/*******
 Mouse/Touch Interaction General Tracking
 (impacts the sending of "idle" events and handling of UI unlocking)
 */

void acu_finger_start(void)
{
    _g_acu.ut_finger_tracking = ACU_TRUE;
}


void acu_finger_end(void)
{
    _g_acu.ut_finger_tracking = ACU_FALSE;
}



