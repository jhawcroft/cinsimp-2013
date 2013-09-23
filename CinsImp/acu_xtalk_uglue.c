/*
 
 Application Control Unit - xTalk UI-side Glue
 acu_xtalk_uglue.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Glue code that implements CinsImp environment specific functionality on behalf of the xTalk
 thread, on the application's UI thread.
 
 *************************************************************************************************
 */

#include "acu_int.h"


void _acu_ut_message_result(StackMgrStack *in_stack, ACUParam in_params[], int in_param_count)
{
    _g_acu.callbacks.message_set(in_params[0].value.string);
    _acu_xtalk_callback_respond(in_stack);
}


void _acu_ut_refresh_card(StackMgrStack *in_stack, ACUParam in_params[], int in_param_count)
{
    _acu_repaint_card(in_stack);
    //_g_acu.callbacks.view_refresh(in_stack->ui_context);
    _acu_xtalk_callback_respond(in_stack);
}


void _acu_ut_find(StackMgrStack *in_stack, ACUParam in_params[], int in_param_count)
{
    _g_acu.callbacks.do_find(in_stack->ui_context,
                             in_params[0].value.string,
                             (int)in_params[1].value.integer,
                             in_params[2].value.integer);
    _acu_xtalk_callback_respond(in_stack);
}


void _acu_ut_debug_message(StackMgrStack *in_stack, ACUParam in_params[], int in_param_count)
{
    _g_acu.callbacks.debug_message(in_stack->xtalk,
                                   in_stack->ui_context,
                                   in_params[0].value.string,
                                   (int)in_params[1].value.integer,
                                   in_params[2].value.boolean);
    _acu_xtalk_callback_respond(in_stack);
}

/*
void _acu_ut_debug_got_variable(StackMgrStack *in_stack, ACUParam in_params[], int in_param_count)
{
    void _acu_register_var_mutation(char const *in_name, int is_global, char const *in_value);
    _acu_register_var_mutation(in_params[0].value.string,
                               (int)in_params[1].value.integer,
                               in_params[2].value.string);
    
    _g_acu.callbacks.debug_vars_changed(in_stack->ui_context);
    
    
    _acu_xt_comms_lock(in_stack);
    assert(in_stack->ut_sig_dir_ic == _ACU_T_SIG_REQ);
    _acu_thread_cond_signal(&(in_stack->xt_wakeup));
    _acu_xt_comms_unlock(in_stack);
}
*/
/*
void _acu_ut_debug_got_handlers(StackMgrStack *in_stack, ACUParam in_params[], int in_param_count)
{
    void _acu_debug_got_all_handlers(char const *in_handlers[], int in_handler_count);
    _acu_debug_got_all_handlers(in_params[0].value.pointer,
                                (int)in_params[1].value.integer);
    
    _g_acu.callbacks.debug_vars_changed(in_stack->ui_context);
    _acu_xtalk_callback_respond(in_stack);
}
*/

void _acu_ut_visual(StackMgrStack *in_stack, ACUParam in_params[], int in_param_count)
{
    if (in_stack->effect_queue_count == ACU_LIMIT_MAX_VISUAL_EFFECTS)
    {
        xte_callback_error(in_stack->xtalk, "Too many visual effects.", NULL, NULL, NULL);
        _acu_xtalk_callback_respond(in_stack);
        return;
    }
    
    in_stack->effect_queue[in_stack->effect_queue_count].effect = (int)in_params[0].value.integer;
    in_stack->effect_queue[in_stack->effect_queue_count].speed = (int)in_params[1].value.integer;
    in_stack->effect_queue[in_stack->effect_queue_count].dest = (int)in_params[2].value.integer;
    in_stack->effect_queue_count++;
    
    _acu_xtalk_callback_respond(in_stack);
}


void _acu_ut_go_relative(StackMgrStack *in_stack, ACUParam in_params[], int in_param_count)
{
    _acu_lock_screen(in_stack);
    
    long card_index = stack_card_index_for_id(in_stack->stack, _stackmgr_current_card_id(in_stack));
    long card_count;
    switch (in_params[0].value.integer)
    {
        case -2: // next
            card_index++;
            if (card_index >= stack_card_count(in_stack->stack)) card_index = 0;
            _acu_go_card(in_stack, stack_card_id_for_index(in_stack->stack, card_index));
            break;
        case -1: // previous
            card_index--;
            if (card_index < 0) card_index = stack_card_count(in_stack->stack)-1;
            _acu_go_card(in_stack, stack_card_id_for_index(in_stack->stack, card_index));
            break;
            
        case -3: // middle
            card_count = stack_card_count(in_stack->stack);
            if (card_count == 1) card_index = 0;
            else card_index = card_count/2;
            _acu_go_card(in_stack, stack_card_id_for_index(in_stack->stack, card_index));
            break;
        case -5: // last
            card_count = stack_card_count(in_stack->stack);
            _acu_go_card(in_stack, stack_card_id_for_index(in_stack->stack, card_count-1));
            break;
            
        case 1: // regular ordinal
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            card_count = stack_card_count(in_stack->stack);
            if (in_params[0].value.integer > card_count)
            {
                in_stack->xt_sic.param[3].value.boolean = ACU_FALSE; // couldn't go there
                break;
            }
            _acu_go_card(in_stack, stack_card_id_for_index(in_stack->stack, in_params[0].value.integer - 1));
            break;
    }
    
    _acu_unlock_screen(in_stack, ACU_TRUE);
    //_acu_xtalk_callback_respond(in_stack);
}

/*
 {"go [to] {`which``-2`next|`-1`prev|`-1`previous|`1`first|`2`second|`3`third|`4`forth|`5`fifth|"
 "`6`sixth|`7`seventh|`8`eighth|`9`ninth|`10`tenth|`-3`middle|`-4`any|`-5`last}
 */

void _acu_ut_go_absolute(StackMgrStack *in_stack, ACUParam in_params[], int in_param_count)
{
    _acu_lock_screen(in_stack);
    StackHandle the_card = in_params[0].value.pointer;
    _acu_go_card(in_stack, HDEF(the_card).layer_id);
    
    _acu_unlock_screen(in_stack, ACU_TRUE);
    //_acu_xtalk_callback_respond(in_stack);
}



/**********/







/*



void _acu_xtalk_ui_debug_variables_changed(StackMgrStack *in_stack)
{
    _g_acu.callbacks.debug_vars_changed(in_stack->ui_context);
    _acu_xtalk_callback_respond(in_stack);
}

*/











void _acu_unlock_screen(StackMgrStack *in_stack, int in_respond_xtalk_after)
{
    if (in_stack->lock_screen > 0) in_stack->lock_screen--;
    if (in_stack->lock_screen > 0)
    {
        if (in_respond_xtalk_after) _acu_xtalk_callback_respond(in_stack);
        return;
    }
    
    /* run visual effects */
    if (in_stack->effect_queue_count > 0)
    {
        /* disable auto-status timer */
        in_stack->auto_status_timer = -1;
        _g_acu.callbacks.auto_status_control(in_stack->ui_context, ACU_FALSE, 0);
        
        in_stack->effect_queue_playback = 1;
        in_stack->respond_to_xtalk_after_effects = in_respond_xtalk_after;
        _g_acu.callbacks.effect_render(in_stack->ui_context,
                                       in_stack->effect_queue[0].effect,
                                       in_stack->effect_queue[0].speed,
                                       in_stack->effect_queue[0].dest
                                       );
    }
    else
    {
        if (in_respond_xtalk_after) _acu_xtalk_callback_respond(in_stack);
        else _g_acu.callbacks.screen_release(in_stack->ui_context);
    }
}


void acu_effect_rendered(StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    /* empty the visual effect queue
     at end of playback */
    if (the_stack->effect_queue_playback == the_stack->effect_queue_count)
    {
        the_stack->effect_queue_playback = 0;
        the_stack->effect_queue_count = 0;
        
        /* reenable the auto-status timer */
        the_stack->auto_status_timer = 0;
        
        /* check if we're supposed to let xTalk know */
        if (the_stack->respond_to_xtalk_after_effects)
        {
            the_stack->respond_to_xtalk_after_effects = ACU_FALSE;
            the_stack->idle_repaint_required = ACU_TRUE;
            _acu_xtalk_callback_respond(the_stack);
        }
        
        return;
    }
    
    /* continue playing visual effect queue */
    _g_acu.callbacks.effect_render(the_stack->ui_context,
                                   the_stack->effect_queue[the_stack->effect_queue_playback].effect,
                                   the_stack->effect_queue[the_stack->effect_queue_playback].speed,
                                   the_stack->effect_queue[the_stack->effect_queue_playback].dest
                                   );
    the_stack->effect_queue_playback++;
}


void _acu_ut_unlock_screen(StackMgrStack *in_stack)
{
    _acu_unlock_screen(in_stack, ACU_TRUE);
    //_acu_xtalk_callback_respond(in_stack);
}


void _acu_lock_screen(StackMgrStack *in_stack)
{
    in_stack->lock_screen++;
    in_stack->idle_repaint_required = ACU_TRUE;
    if (in_stack->lock_screen == 1)
    {
        /* grab a copy of whatever is on the screen currently */
        _g_acu.callbacks.screen_save(in_stack->ui_context);
    }
}


void _acu_ut_lock_screen(StackMgrStack *in_stack)
{
    _acu_lock_screen(in_stack);
    _acu_xtalk_callback_respond(in_stack);
}





void _acu_ut_answer_choice(StackMgrStack *in_stack)
{
    char const *prompt, *btn1 = NULL, *btn2 = NULL, *btn3 = NULL;
    
    prompt = xte_variant_as_cstring(in_stack->thread_xte_imp_param[0]);
    if (in_stack->thread_xte_imp_param[1])
        btn1 = xte_variant_as_cstring(in_stack->thread_xte_imp_param[1]);
    if (in_stack->thread_xte_imp_param[2])
        btn2 = xte_variant_as_cstring(in_stack->thread_xte_imp_param[2]);
    if (in_stack->thread_xte_imp_param[3])
        btn3 = xte_variant_as_cstring(in_stack->thread_xte_imp_param[3]);
    if (!btn1) btn1 = "OK";
    
    _g_acu.callbacks.answer_choice(in_stack->ui_context, prompt, btn1, btn2, btn3);
}


void acu_answer_choice_reply(StackHandle in_stack, int in_button_index, char const *in_reply)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    the_stack->thread_xte_imp_param[0] = xte_string_create_with_cstring(the_stack->xtalk, in_reply);
    
    _acu_xtalk_callback_respond(the_stack);
}


void _acu_ut_answer_file(StackMgrStack *in_stack)
{
    char const *prompt, *type1 = NULL, *type2 = NULL, *type3 = NULL;
    
    prompt = xte_variant_as_cstring(in_stack->thread_xte_imp_param[0]);
    if (in_stack->thread_xte_imp_param[1])
        type1 = xte_variant_as_cstring(in_stack->thread_xte_imp_param[1]);
    if (in_stack->thread_xte_imp_param[2])
        type2 = xte_variant_as_cstring(in_stack->thread_xte_imp_param[2]);
    if (in_stack->thread_xte_imp_param[3])
        type3 = xte_variant_as_cstring(in_stack->thread_xte_imp_param[3]);
    
    _g_acu.callbacks.answer_file(in_stack->ui_context, prompt, type1, type2, type3);
}


void acu_answer_file_reply(StackHandle in_stack, char const *in_pathname)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    the_stack->thread_xte_imp_param[0] = xte_string_create_with_cstring(the_stack->xtalk, in_pathname);
    
    _acu_xtalk_callback_respond(the_stack);
}


void _acu_ut_answer_folder(StackMgrStack *in_stack)
{
    char const *prompt;
    
    prompt = xte_variant_as_cstring(in_stack->thread_xte_imp_param[0]);
    
    _g_acu.callbacks.answer_folder(in_stack->ui_context, prompt);
}


void acu_answer_folder_reply(StackHandle in_stack, char const *in_pathname)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    the_stack->thread_xte_imp_param[0] = xte_string_create_with_cstring(the_stack->xtalk, in_pathname);
    
    _acu_xtalk_callback_respond(the_stack);
}



void _acu_ut_ask_text(StackMgrStack *in_stack)
{
    char const *prompt, *response = NULL;
    
    prompt = xte_variant_as_cstring(in_stack->thread_xte_imp_param[0]);
    if (in_stack->thread_xte_imp_param[1])
        response = xte_variant_as_cstring(in_stack->thread_xte_imp_param[1]);
    if (!response) response = "";
    
    _g_acu.callbacks.ask_text(in_stack->ui_context, prompt, response, (in_stack->thread_xte_imp_param[2] != NULL));
}


void acu_ask_choice_reply(StackHandle in_stack, char const *in_reply)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    the_stack->thread_xte_imp_param[0] = xte_string_create_with_cstring(the_stack->xtalk, in_reply);
    
    _acu_xtalk_callback_respond(the_stack);
}


void _acu_ut_ask_file(StackMgrStack *in_stack)
{
    char const *prompt, *defaultFilename = NULL;
    
    prompt = xte_variant_as_cstring(in_stack->thread_xte_imp_param[0]);
    if (in_stack->thread_xte_imp_param[1])
        defaultFilename = xte_variant_as_cstring(in_stack->thread_xte_imp_param[1]);
    if (!defaultFilename) defaultFilename = "";
    
    _g_acu.callbacks.ask_file(in_stack->ui_context, prompt, defaultFilename);
}


void acu_ask_file_reply(StackHandle in_stack, char const *in_pathname)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    the_stack->thread_xte_imp_param[0] = xte_string_create_with_cstring(the_stack->xtalk, in_pathname);
    
    _acu_xtalk_callback_respond(the_stack);

}



