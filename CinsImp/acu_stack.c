/*
 
 Application Control Unit - Stack
 acu_stack.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Wrapper around Stack unit; provides safe access to a stack using StackHandles issued by the 
 Handle Manager
 
 *************************************************************************************************
 */

#include "acu_int.h"



/******************
 Stack Accessors/Mutators
 */



int stackmgr_script_get(StackHandle in_handle, char const **out_script, int const *out_checkpoints[], int *out_checkpoint_count,
                        long *out_sel_offset)
{
    int result;
    if (_stackmgr_handle_is_stack((HandleDef*)in_handle))
        result = stack_script_get((Stack*)in_handle, STACK_SELF, STACK_NO_OBJECT, out_script, out_checkpoints, out_checkpoint_count,
                                  out_sel_offset);
    else
    {
        HandleDef *the_handle = (HandleDef*)in_handle;
        StackMgrStack *register_entry = _stackmgr_stack(in_handle);
        if (!register_entry) return STACKMGR_ERROR_NO_OBJECT;
        switch (the_handle->reference.type)
        {
            case STACKMGR_TYPE_BKGND:
                result = stack_script_get(register_entry->stack, STACK_BKGND, the_handle->reference.layer_id,
                                          out_script, out_checkpoints, out_checkpoint_count, out_sel_offset);
                break;
            case STACKMGR_TYPE_CARD:
                result = stack_script_get(register_entry->stack, STACK_CARD, the_handle->reference.layer_id,
                                          out_script, out_checkpoints, out_checkpoint_count, out_sel_offset);
                break;
            case STACKMGR_TYPE_BUTTON:
            case STACKMGR_TYPE_FIELD:
                result = stack_script_get(register_entry->stack, STACK_WIDGET, the_handle->reference.widget_id,
                                          out_script, out_checkpoints, out_checkpoint_count, out_sel_offset);
                break;
        }
    }
    if (result == STACK_ERR_NONE) return STACKMGR_ERROR_NONE;
    return STACKMGR_ERROR_NO_OBJECT;
}


int stackmgr_script_set(StackHandle in_handle, char const *in_script, int const in_checkpoints[], int in_checkpoint_count,
                        long in_sel_offset)
{
    int result;
    if (_stackmgr_handle_is_stack((HandleDef*)in_handle))
        result = stack_script_set((Stack*)in_handle, STACK_SELF, STACK_NO_OBJECT, in_script, in_checkpoints, in_checkpoint_count,
                                  in_sel_offset);
    else
    {
        HandleDef *the_handle = (HandleDef*)in_handle;
        StackMgrStack *register_entry = _stackmgr_stack(in_handle);
        if (!register_entry) return STACKMGR_ERROR_NO_OBJECT;
        switch (the_handle->reference.type)
        {
            case STACKMGR_TYPE_BKGND:
                result = stack_script_set(register_entry->stack, STACK_BKGND, the_handle->reference.layer_id,
                                          in_script, in_checkpoints, in_checkpoint_count, in_sel_offset);
                break;
            case STACKMGR_TYPE_CARD:
                result = stack_script_set(register_entry->stack, STACK_CARD, the_handle->reference.layer_id,
                                          in_script, in_checkpoints, in_checkpoint_count, in_sel_offset);
                break;
            case STACKMGR_TYPE_BUTTON:
            case STACKMGR_TYPE_FIELD:
                result = stack_script_set(register_entry->stack, STACK_WIDGET, the_handle->reference.widget_id,
                                          in_script, in_checkpoints, in_checkpoint_count, in_sel_offset);
                break;
        }
    }
    if (result == STACK_ERR_NONE) return STACKMGR_ERROR_NONE;
    return STACKMGR_ERROR_NO_OBJECT;
}



void stackmgr_script_rect_get(StackHandle in_handle, int *out_x, int *out_y, int *out_w, int *out_h)
{
    char const *rect_str;
    if (_stackmgr_handle_is_stack((HandleDef*)in_handle))
    {
        rect_str = stack_prop_get_string((Stack*)in_handle, STACK_NO_OBJECT, STACK_NO_OBJECT, PROPERTY_SCRIPTEDITORRECT);
    }
    else
    {
        HandleDef *the_handle = (HandleDef*)in_handle;
        StackMgrStack *the_stack = _stackmgr_stack(in_handle);
        if (!the_stack) return;
        switch (the_handle->reference.type)
        {
            case STACKMGR_TYPE_BKGND:
                rect_str = stack_prop_get_string(the_stack->stack, STACK_NO_OBJECT, the_handle->reference.layer_id, PROPERTY_SCRIPTEDITORRECT);
                break;
            case STACKMGR_TYPE_CARD:
                rect_str = stack_prop_get_string(the_stack->stack, the_handle->reference.layer_id, STACK_NO_OBJECT, PROPERTY_SCRIPTEDITORRECT);
                break;
            case STACKMGR_TYPE_BUTTON:
            case STACKMGR_TYPE_FIELD:
                rect_str = stack_widget_prop_get_string(the_stack->stack, the_handle->reference.widget_id,
                                                        STACK_NO_OBJECT, PROPERTY_SCRIPTEDITORRECT);
                break;
        }
    }
    
    if (!rect_str) rect_str = "0,0,0,0";
    
    XTERect rect = xte_string_to_rect(rect_str);
    *out_x = (int)rect.origin.x;
    *out_y = (int)rect.origin.y;
    *out_w = (int)rect.size.width;
    *out_h = (int)rect.size.height;
}


void stackmgr_script_rect_set(StackHandle in_handle, int in_x, int in_y, int in_w, int in_h)
{
    XTERect rect = {in_x, in_y, in_w, in_h};
    char buffer[XTE_GEO_CONV_BUFF_SIZE];
    xte_rect_to_string(rect, buffer, XTE_GEO_CONV_BUFF_SIZE);
    
    if (_stackmgr_handle_is_stack((HandleDef*)in_handle))
    {
        stack_prop_set_string((Stack*)in_handle, STACK_NO_OBJECT, STACK_NO_OBJECT, PROPERTY_SCRIPTEDITORRECT, buffer);
    }
    else
    {
        HandleDef *the_handle = (HandleDef*)in_handle;
        StackMgrStack *the_stack = _stackmgr_stack(in_handle);
        if (!the_stack) return;
        switch (the_handle->reference.type)
        {
            case STACKMGR_TYPE_BKGND:
                stack_prop_set_string(the_stack->stack, STACK_NO_OBJECT, the_handle->reference.layer_id, PROPERTY_SCRIPTEDITORRECT, buffer);
                break;
            case STACKMGR_TYPE_CARD:
                stack_prop_set_string(the_stack->stack, the_handle->reference.layer_id, STACK_NO_OBJECT, PROPERTY_SCRIPTEDITORRECT, buffer);
                break;
            case STACKMGR_TYPE_BUTTON:
            case STACKMGR_TYPE_FIELD:
                stack_widget_prop_set_string(the_stack->stack, the_handle->reference.widget_id,
                                                        STACK_NO_OBJECT, PROPERTY_SCRIPTEDITORRECT, buffer);
                break;
        }
    }
}





/******************
 Actions
 */


/// USE THIS INSTEAD OF CALLING VIEW REFRESH DIRECTLY...
void _acu_repaint_card(StackMgrStack *in_stack)
{
    if (in_stack->lock_screen == 0)
        _g_acu.callbacks.view_refresh(in_stack->ui_context);
    else
        in_stack->idle_repaint_required = ACU_TRUE;
}



void _acu_go_card(StackMgrStack *in_stack, long in_card_id)
{
    if (in_card_id < 1) return;
    
    _g_acu.callbacks.view_layout_will_change(in_stack->ui_context);
    
    _acu_lock_screen(in_stack);
    
    in_stack->__current_card_id = in_card_id;
    in_stack->__current_bkgnd_id = stack_card_bkgnd_id(in_stack->stack, in_card_id);
    
    _acu_unlock_screen(in_stack, ACU_FALSE);
    
    _g_acu.callbacks.view_layout_did_change(in_stack->ui_context);
}










