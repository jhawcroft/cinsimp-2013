/*
 
 Application Control Unit: xTalk Thread: Object Property Access/Mutation
 acu_xt_props.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Environmental glue code to allow access/mutation of object properties and content within the
 CinsImp environment, including the content of fields.
 
 *************************************************************************************************
 */

#include "acu_int.h"


/*********
 Utility
 */

static int _widget_is_displayed(StackMgrStack *in_stack, long in_widget_id)
{
    return ACU_TRUE; // **TODO** later
}



/*********
 Widgets (General)
 */

XTEVariant* _acu_xt_widg_get_str(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation)
{
    StackHandle object = xte_variant_object_data_ptr(in_object);
    return xte_string_create_with_cstring(in_engine, stack_widget_prop_get_string(in_stack->stack,
                                                                                  HDEF(object).widget_id,
                                                                                  STACK_NO_OBJECT,
                                                                                  in_prop));
}


void _acu_xt_widg_set_str(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEVariant *in_new_value)
{
    if (!xte_variant_convert(in_engine, in_new_value, XTE_TYPE_STRING))
        return xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
    StackHandle object = xte_variant_object_data_ptr(in_object);
    stack_widget_prop_set_string(in_stack->stack,
                                 HDEF(object).widget_id,
                                 STACK_NO_OBJECT,
                                 in_prop,
                                 (char*)xte_variant_as_cstring(in_new_value));
    if (_widget_is_displayed(in_stack, HDEF(object).widget_id))
        _acu_xt_card_needs_repaint(in_stack, ACU_TRUE);
}


XTEVariant* _acu_xt_widg_get_bool(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation)
{
    StackHandle object = xte_variant_object_data_ptr(in_object);
    return xte_boolean_create(in_engine, (int)stack_widget_prop_get_long(in_stack->stack,
                                                                         HDEF(object).widget_id,
                                                                         STACK_NO_OBJECT,
                                                                         in_prop));
}


void _acu_xt_widg_set_bool(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEVariant *in_new_value)
{
    if (!xte_variant_convert(in_engine, in_new_value, XTE_TYPE_BOOLEAN))
        return xte_callback_error(in_engine, "Expected true or false here.", NULL, NULL, NULL);
    StackHandle object = xte_variant_object_data_ptr(in_object);
    stack_widget_prop_set_long(in_stack->stack,
                               HDEF(object).widget_id,
                               STACK_NO_OBJECT,
                               in_prop,
                               xte_variant_as_int(in_new_value));
    if (_widget_is_displayed(in_stack, HDEF(object).widget_id))
        _acu_xt_card_needs_repaint(in_stack, ACU_TRUE);
}


XTEVariant* _acu_xt_widg_get_int(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation)
{
    StackHandle object = xte_variant_object_data_ptr(in_object);
    return xte_integer_create(in_engine, (int)stack_widget_prop_get_long(in_stack->stack,
                                                                         HDEF(object).widget_id,
                                                                         STACK_NO_OBJECT,
                                                                         in_prop));
}


void _acu_xt_widg_set_int(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEVariant *in_new_value)
{
    if (!xte_variant_convert(in_engine, in_new_value, XTE_TYPE_INTEGER))
        return xte_callback_error(in_engine, "Expected integer here.", NULL, NULL, NULL);
    StackHandle object = xte_variant_object_data_ptr(in_object);
    stack_widget_prop_set_long(in_stack->stack,
                               HDEF(object).widget_id,
                               STACK_NO_OBJECT,
                               in_prop,
                               xte_variant_as_int(in_new_value));
    if (_widget_is_displayed(in_stack, HDEF(object).widget_id))
        _acu_xt_card_needs_repaint(in_stack, ACU_TRUE);
}



/*********
 Field Containers
 */

/*
 *  _obj_fld_read
 *  ---------------------------------------------------------------------------------------------
 *  Read and return the content of a field.
 */
XTEVariant* _obj_fld_read(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_field, XTEPropRep in_representation)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    if ( (!xte_variant_is_class(in_field, "cdfld")) && (!xte_variant_is_class(in_field, "bgfld")) ) return NULL;
    StackHandle field = xte_variant_object_data_ptr(in_field);
    char *searchable;
    long content_card_id = (HDEF(field).layer_is_card ? HDEF(field).layer_id : _stackmgr_current_card_id(in_stack));
    stack_widget_content_get(in_stack->stack,
                             HDEF(field).widget_id,
                             content_card_id,
                             STACK_NO,
                             &searchable,
                             NULL,
                             NULL,
                             NULL);
    return xte_string_create_with_cstring(in_engine, searchable);
}


/*
 *  _obj_fld_write
 *  ---------------------------------------------------------------------------------------------
 *  Write to a field.  May replace the content, or only a specific portion depending on the mode
 *  and text range parameters.
 *
 *  Only the specified range of text should be modified.  Fonts and styles must be preserved on
 *  text content that is not mutated.
 */
void _obj_fld_write(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_field,
                    XTEVariant *in_value, XTETextRange in_text_range, XTEPutMode in_mode)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    if ( (!xte_variant_is_class(in_field, "cdfld")) && (!xte_variant_is_class(in_field, "bgfld")) ) return;
    StackHandle field = xte_variant_object_data_ptr(in_field);
    
    /* read the existing content of the field */
    char *formatted;
    long formatted_size;
    char *plain;
    long content_card_id = (HDEF(field).layer_is_card ? HDEF(field).layer_id : _stackmgr_current_card_id(in_stack));
    stack_widget_content_get(in_stack->stack,
                             HDEF(field).widget_id,
                             content_card_id,
                             STACK_NO,
                             &plain,
                             &formatted,
                             &formatted_size,
                             NULL);
    
    /* determine the replacement text and range */
    char const *new_text = xte_variant_as_cstring(in_value);
    //printf("acu: _obj_fld_write: existing content: \"%s\"\n", plain);
    //printf("acu: _obj_fld_write: new content     : \"%s\"\n", new_text);
    //printf("acu: _obj_fld_write: mode            : %d\n", in_mode);
    //printf("acu: _obj_fld_write: text range      : %ld,%ld\n", in_text_range.offset, in_text_range.length);
    
    XTETextRange edit_range;
    if (in_text_range.offset < 0)
    {
        switch (in_mode) {
            case XTE_PUT_INTO:
                edit_range.offset = 0;
                edit_range.length = xte_cstring_length(plain);
                break;
            case XTE_PUT_BEFORE:
                edit_range.offset = 0;
                edit_range.length = 0;
                break;
            case XTE_PUT_AFTER:
                edit_range.offset = xte_cstring_length(plain);
                edit_range.length = 0;
                break;
            default:
                break;
        }
    }
    else
    {
        switch (in_mode)
        {
            case XTE_PUT_INTO:
                edit_range = in_text_range;
                break;
            case XTE_PUT_BEFORE:
                edit_range.offset = in_text_range.offset;
                edit_range.length = 0;
                break;
            case XTE_PUT_AFTER:
                edit_range.offset = in_text_range.offset + in_text_range.length;
                edit_range.length = 0;
                break;
                
            default:
                break;
        }
    }
    
    /* mutate the RTF appropriately */
    _acu_begin_single_thread_callback();
    _g_acu.callbacks.mutate_rtf((void**)&formatted, &formatted_size, &plain, new_text, edit_range);
    
    /* replace the field content with the modified attributed string data */
    stack_widget_content_set(in_stack->stack,
                             HDEF(field).widget_id,
                             content_card_id,
                             plain,
                             formatted,
                             formatted_size);
    
    /* cleanup after RTF mutation */
    //_g_acu.callbacks.mutate_rtf(NULL, NULL, NULL, NULL, edit_range);
    _acu_end_single_thread_callback();
    
    free(plain);
    free(formatted);
    
    /* if the current card is also the card of the widget being changed,
     refresh the screen */
    if (content_card_id == _stackmgr_current_card_id(in_stack))
    {
        _acu_xt_card_needs_repaint(in_stack, ACU_TRUE);
    }
}



/*********
 Cards, Backgrounds, Stacks (General)
 */

XTEVariant* _acu_xt_stak_get_str(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation)
{
    StackHandle object = xte_variant_object_data_ptr(in_object);
    return xte_string_create_with_cstring(in_engine, stack_prop_get_string(in_stack->stack,
                                                                           (HDEF(object).layer_is_card ? HDEF(object).layer_id : STACK_NO_OBJECT),
                                                                           (HDEF(object).layer_is_card ? STACK_NO_OBJECT : HDEF(object).layer_id),
                                                                           in_prop));
}


void _acu_xt_stak_set_str(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEVariant *in_new_value)
{
    if (!xte_variant_convert(in_engine, in_new_value, XTE_TYPE_STRING))
        return xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
    StackHandle object = xte_variant_object_data_ptr(in_object);
    stack_prop_set_string(in_stack->stack,
                          (HDEF(object).layer_is_card ? HDEF(object).layer_id : STACK_NO_OBJECT),
                          (HDEF(object).layer_is_card ? STACK_NO_OBJECT : HDEF(object).layer_id),
                          in_prop,
                          (char*)xte_variant_as_cstring(in_new_value));
    _acu_xt_card_needs_repaint(in_stack, ACU_TRUE);
}


XTEVariant* _acu_xt_stak_get_bool(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation)
{
    StackHandle object = xte_variant_object_data_ptr(in_object);
    return xte_boolean_create(in_engine, (int)stack_prop_get_long(in_stack->stack,
                                                                  (HDEF(object).layer_is_card ? HDEF(object).layer_id : STACK_NO_OBJECT),
                                                                  (HDEF(object).layer_is_card ? STACK_NO_OBJECT : HDEF(object).layer_id),
                                                                  in_prop));
}


void _acu_xt_stak_set_bool(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEVariant *in_new_value)
{
    if (!xte_variant_convert(in_engine, in_new_value, XTE_TYPE_BOOLEAN))
        return xte_callback_error(in_engine, "Expected true or false here.", NULL, NULL, NULL);
    StackHandle object = xte_variant_object_data_ptr(in_object);
    stack_prop_set_long(in_stack->stack,
                          (HDEF(object).layer_is_card ? HDEF(object).layer_id : STACK_NO_OBJECT),
                          (HDEF(object).layer_is_card ? STACK_NO_OBJECT : HDEF(object).layer_id),
                          in_prop,
                          xte_variant_as_int(in_new_value));
    _acu_xt_card_needs_repaint(in_stack, ACU_TRUE);
}







/*
 static struct XTEPropertyDef bkgnd_props[] = {
 {"id", "integer", XTE_PROPERTY_UID, 0},
 {"name", "string", XTE_PROPERTY_UID, 0},
 NULL,
 };
 
 
 static struct XTEPropertyDef stack_props[] = {
 {"name", "string", 0},
 {"path", "string", 0},
 {"cantpeek", "boolean", 0},
 NULL,
 };*/


/*
 int stackmgr_script_get(StackHandle in_handle, char const **out_script, int const *out_checkpoints[], int *out_checkpoint_count,
 long *out_sel_offset);
 */





