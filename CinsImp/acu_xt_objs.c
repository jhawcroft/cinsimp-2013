/*
 
 Application Control Unit: xTalk Thread: Object Reference Construction
 acu_xt_objs.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Environmental glue code to allow referencing objects within the CinsImp environment; including
 buttons, fields, cards, backgrounds and stacks.
 
 *************************************************************************************************
 */

#include "acu_int.h"


/*********
 Notes to be reviewed & reformatted
 */

/* need to be careful that there isn't a write/refresh going on-via despatch **TODO**
 proper protection of stack AND handle via mutex 
 
 accesses to fields, etc. anytime we access the stack probably - since UI will not be locked out */


/***TODO *** global property called 'userModify'  LOW PRIORITY
 for locked/unwritable stacks - allows scripts and user to change the value of text in a field on a card,
 but the changes are not saved */
//(void **io_rtf, long *io_size, char **out_plain, char const *in_new_string, XTETextRange in_edit_range)



/*********
 Utility
 */

/*
 *  _acu_variant_int_destructor
 *  ---------------------------------------------------------------------------------------------
 *  Calls free() on an int previously assigned as data to an XTEVariant object; invoked by the
 *  xTalk engine when the variant is disposed.
 */
void _acu_variant_int_destructor(XTEVariant *in_variant, char *in_type, int *in_count)
{
    assert(in_count != NULL);
    free(in_count);
}


/*
 *  _owner_card
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a stack handle for the relevant owner card for a widget.  If owner is specified,
 *  a handle to that card is returned.  Otherwise, a handle to the current card is assumed.
 *
 *  If owner is specified but isn't a card, returns NULL.
 *
 *  ! Caller is responsible for releasing the returned handle when finished.
 */
static StackHandle _owner_card(StackMgrStack *in_stack, XTEVariant *in_owner)
{
    assert(in_stack != NULL);
    
    if (xte_variant_is_class(in_owner, "card"))
        return acu_retain((StackHandle)xte_variant_object_data_ptr(in_owner));
    
    else if (in_owner != NULL) return NULL;
    
    else return acu_handle_for_current_card(STACKMGR_FLAG_MAN_RELEASE, (StackHandle)in_stack);
}


/*
 *  _owner_bkgnd_or_card
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a stack handle for the relevant owner card/bkgnd for a widget.  If owner is specified,
 *  a handle to that is returned.  Otherwise, a handle to the current card is assumed.
 *
 *  If owner is specified but isn't a card or bkgnd, returns NULL.
 *
 *  ! Caller is responsible for releasing the returned handle when finished.
 */
static StackHandle _owner_bkgnd_or_card(StackMgrStack *in_stack, XTEVariant *in_owner)
{
    assert(in_stack != NULL);
    
    if (xte_variant_is_class(in_owner, "card"))
        return acu_retain((StackHandle)xte_variant_object_data_ptr(in_owner));
    
    else if (xte_variant_is_class(in_owner, "bkgnd"))
        return acu_retain((StackHandle)xte_variant_object_data_ptr(in_owner));
    
    else if (in_owner != NULL) return NULL;
    
    else return acu_handle_for_current_card(STACKMGR_FLAG_MAN_RELEASE, (StackHandle)in_stack);
}


/*
 *  _owner_bkgnd
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a stack handle for the relevant owner bkgnd for a widget.  If owner is specified,
 *  a handle to that bkgnd is returned.  Otherwise, a handle to the current card's bkgnd is assumed.
 *
 *  If owner is specified but isn't a bkgnd, returns NULL.
 *
 *  ! Caller is responsible for releasing the returned handle when finished.
 */
static StackHandle _owner_bkgnd(StackMgrStack *in_stack, XTEVariant *in_owner)
{
    assert(in_stack != NULL);
    
    if (xte_variant_is_class(in_owner, "bkgnd"))
        return acu_retain((StackHandle)xte_variant_object_data_ptr(in_owner));
    
    else if (in_owner != NULL) return NULL;
    
    else return acu_handle_for_current_bkgnd(STACKMGR_FLAG_MAN_RELEASE, (StackHandle)in_stack);
}



/*********
 Fields
 */


/*
 *  _obj_cd_fld_range
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card field by number, or the number of card fields.
 */
XTEVariant* _obj_cd_fld_range(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                                     XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* get current card or content owner card (if specified) */
    StackHandle the_card = _owner_card(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* are we trying to get a single field? */
    if (in_param_count == 1)
    {
        /* lookup the field by number */
        int number = xte_variant_as_int(in_params[0]);
        long field_id = STACK_NO_OBJECT;
        long widget_count = (int)stack_widget_count(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT);
        for (long i = 0, widget_num = 0; i < widget_count; i++)
        {
            long widget_id = stack_widget_n(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT, i);
            if (stack_widget_is_field(in_stack->stack, widget_id)) widget_num++;
            if ((widget_num == number) && (widget_num > 0)) field_id = widget_id;
        }
        if (field_id == STACK_NO_OBJECT)
        {
            acu_release(the_card);
            return NULL;
        }
        
        /* return a variant reference to the field */
        StackHandle the_field = acu_handle_for_field_id(STACKMGR_FLAG_MAN_RELEASE, the_card, field_id, ACU_INVALID_HANDLE);
        acu_release(the_card);
        return xte_object_create(in_engine, "cdfld", the_field, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
    }
    
    /* are we trying to get all the card fields? */
    else if (in_param_count == 0)
    {
        int *the_count = malloc(sizeof(int));
        assert(the_count != NULL);
        
        /* count the number of card fields */
        *the_count = 0;
        int widget_count = (int)stack_widget_count(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT);
        for (int i = 0; i < widget_count; i++)
        {
            long widget_id = stack_widget_n(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT, i);
            if (stack_widget_is_field(in_stack->stack, widget_id)) (*the_count)++;
        }
        
        /* return a count */
        acu_release(the_card);
        return xte_object_create(in_engine, "objcount", the_count, (XTEObjectDeallocator)&_acu_variant_int_destructor);
    }
    
    /* not allowed to get a range of card fields */
    acu_release(the_card);
    return NULL;
}


/*
 *  _obj_cd_fld_named
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card field by name.
 */
XTEVariant* _obj_cd_fld_named(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                                 XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* validate and convert the parameters */
    if (in_param_count != 1) return NULL;
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_STRING)) return NULL;
    
    /* get current card or content owner card (if specified) */
    StackHandle the_card = _owner_card(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* lookup the field by name */
    const char *name = xte_variant_as_cstring(in_params[0]);
    long field_id = stack_widget_named(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT, name);
    if ((field_id != STACK_NO_OBJECT) && (!stack_widget_is_field(in_stack->stack, field_id))) field_id = STACK_NO_OBJECT;
    if (field_id == STACK_NO_OBJECT)
    {
        acu_release(the_card);
        return NULL;
    }
    
    /* return a variant reference to the field */
    StackHandle the_field = acu_handle_for_field_id(STACKMGR_FLAG_MAN_RELEASE, the_card, field_id, ACU_INVALID_HANDLE);
    acu_release(the_card);
    return xte_object_create(in_engine, "cdfld", the_field, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
}


/*
 *  _obj_cd_fld_id
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card field by ID.
 */
XTEVariant* _obj_cd_fld_id(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* validate and convert the parameters */
    if (in_param_count != 1) return NULL;
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_INTEGER)) return NULL;
    
    /* get current card or content owner card (if specified) */
    StackHandle the_card = _owner_card(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* lookup the field by ID */
    long field_id = xte_variant_as_int(in_params[0]);
    field_id = stack_widget_id(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT, field_id);
    if ((field_id != STACK_NO_OBJECT) && (!stack_widget_is_field(in_stack->stack, field_id))) field_id = STACK_NO_OBJECT;
    if (field_id == STACK_NO_OBJECT)
    {
        acu_release(the_card);
        return NULL;
    }
    
    /* return a variant reference to the field */
    StackHandle the_field = acu_handle_for_field_id(STACKMGR_FLAG_MAN_RELEASE, the_card, field_id, ACU_INVALID_HANDLE);
    acu_release(the_card);
    return xte_object_create(in_engine, "cdfld", the_field, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
}


/*
 *  _obj_bg_fld_range
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a bkgnd field by number, or the number of bkgnd fields.
 */
XTEVariant* _obj_bg_fld_range(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                                  XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* get current card or content owner card/bkgnd (if specified) */
    StackHandle the_card_or_bkgnd = _owner_bkgnd_or_card(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* determine the background ID of the widget */
    long widget_bkgnd_id = STACK_NO_OBJECT;
    if (HDEF(the_card_or_bkgnd).type == STACKMGR_TYPE_CARD)
        widget_bkgnd_id = stack_card_bkgnd_id(in_stack->stack, HDEF(the_card_or_bkgnd).layer_id);
    else
        widget_bkgnd_id = HDEF(the_card_or_bkgnd).layer_id;
    
    /* are we trying to get a single field? */
    if (in_param_count == 1)
    {
        /* lookup the field by number */
        int number = xte_variant_as_int(in_params[0]);
        long field_id = STACK_NO_OBJECT;
        long widget_count = (int)stack_widget_count(in_stack->stack, STACK_NO_OBJECT, widget_bkgnd_id);
        for (long i = 0, widget_num = 0; i < widget_count; i++)
        {
            long widget_id = stack_widget_n(in_stack->stack, STACK_NO_OBJECT, widget_bkgnd_id, i);
            if (stack_widget_is_field(in_stack->stack, widget_id)) widget_num++;
            if ((widget_num == number) && (widget_num > 0)) field_id = widget_id;
        }
        if (field_id == STACK_NO_OBJECT)
        {
            acu_release(the_card_or_bkgnd);
            return NULL;
        }
        
        /* return a variant reference to the field */
        StackHandle the_field = acu_handle_for_field_id(STACKMGR_FLAG_MAN_RELEASE, the_card_or_bkgnd, field_id, ACU_INVALID_HANDLE);
        acu_release(the_card_or_bkgnd);
        return xte_object_create(in_engine, "bgfld", the_field, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
    }
    
    /* are we trying to get all the bkgnd fields? */
    else if (in_param_count == 0)
    {
        int *the_count = malloc(sizeof(int));
        assert(the_count != NULL);
        
        /* count the number of bkgnd fields */
        *the_count = 0;
        int widget_count = (int)stack_widget_count(in_stack->stack, STACK_NO_OBJECT, widget_bkgnd_id);
        for (int i = 0; i < widget_count; i++)
        {
            long widget_id = stack_widget_n(in_stack->stack, STACK_NO_OBJECT, widget_bkgnd_id, i);
            if (stack_widget_is_field(in_stack->stack, widget_id)) (*the_count)++;
        }
        
        /* return a count */
        acu_release(the_card_or_bkgnd);
        return xte_object_create(in_engine, "objcount", the_count, (XTEObjectDeallocator)&_acu_variant_int_destructor);
    }
    
    /* not allowed to get a range of bkgnd fields */
    acu_release(the_card_or_bkgnd);
    return NULL;
}



/*
 *  _obj_bg_fld_named
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a bkgnd field by name.
 */
XTEVariant* _obj_bg_fld_named(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                                  XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* validate and convert the parameters */
    if (in_param_count != 1) return NULL;
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_STRING)) return NULL;
    
    /* get current card or content owner card/bkgnd (if specified) */
    StackHandle the_card_or_bkgnd = _owner_bkgnd_or_card(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* determine the background ID of the widget */
    long widget_bkgnd_id = STACK_NO_OBJECT;
    if (HDEF(the_card_or_bkgnd).type == STACKMGR_TYPE_CARD)
        widget_bkgnd_id = stack_card_bkgnd_id(in_stack->stack, HDEF(the_card_or_bkgnd).layer_id);
    else
        widget_bkgnd_id = HDEF(the_card_or_bkgnd).layer_id;
    
    /* lookup the field by name */
    const char *name = xte_variant_as_cstring(in_params[0]);
    long field_id = stack_widget_named(in_stack->stack, STACK_NO_OBJECT, widget_bkgnd_id, name);
    if ((field_id != STACK_NO_OBJECT) && (!stack_widget_is_field(in_stack->stack, field_id))) field_id = STACK_NO_OBJECT;
    if (field_id == STACK_NO_OBJECT)
    {
        acu_release(the_card_or_bkgnd);
        return NULL;
    }
    
    /* return a variant reference to the field */
    StackHandle the_field = acu_handle_for_field_id(STACKMGR_FLAG_MAN_RELEASE, the_card_or_bkgnd, field_id, the_card_or_bkgnd);
    acu_release(the_card_or_bkgnd);
    return xte_object_create(in_engine, "bgfld", the_field, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
}


/*
 *  _obj_bg_fld_id
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a bkgnd field by ID.
 */
XTEVariant* _obj_bg_fld_id(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                               XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* validate and convert the parameters */
    if (in_param_count != 1) return NULL;
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_INTEGER)) return NULL;
    
    /* get current card or content owner card (if specified) */
    StackHandle the_card_or_bkgnd = _owner_card(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* determine the background ID of the widget */
    long widget_bkgnd_id = STACK_NO_OBJECT;
    if (HDEF(the_card_or_bkgnd).type == STACKMGR_TYPE_CARD)
        widget_bkgnd_id = stack_card_bkgnd_id(in_stack->stack, HDEF(the_card_or_bkgnd).layer_id);
    else
        widget_bkgnd_id = HDEF(the_card_or_bkgnd).layer_id;
    
    /* lookup the field by ID */
    long field_id = xte_variant_as_int(in_params[0]);
    field_id = stack_widget_id(in_stack->stack, STACK_NO_OBJECT, widget_bkgnd_id, field_id);
    if ((field_id != STACK_NO_OBJECT) && (!stack_widget_is_field(in_stack->stack, field_id))) field_id = STACK_NO_OBJECT;
    if (field_id == STACK_NO_OBJECT)
    {
        acu_release(the_card_or_bkgnd);
        return NULL;
    }
    
    /* return a variant reference to the field */
    StackHandle the_field = acu_handle_for_field_id(STACKMGR_FLAG_MAN_RELEASE, the_card_or_bkgnd, field_id, the_card_or_bkgnd);
    acu_release(the_card_or_bkgnd);
    return xte_object_create(in_engine, "cdfld", the_field, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
}



/*********
 Buttons
 */

/*
 *  _obj_cd_btn_range
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card button by number, or the number of card buttons.
 */
XTEVariant* _obj_cd_btn_range(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* get current card or owner card (if specified) */
    StackHandle the_card = _owner_card(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* are we trying to get a single button? */
    if (in_param_count == 1)
    {
        /* lookup the button by number */
        int number = xte_variant_as_int(in_params[0]);
        long button_id = STACK_NO_OBJECT;
        long widget_count = (int)stack_widget_count(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT);
        for (long i = 0, widget_num = 0; i < widget_count; i++)
        {
            long widget_id = stack_widget_n(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT, i);
            if (!stack_widget_is_field(in_stack->stack, widget_id)) widget_num++;
            if ((widget_num == number) && (widget_num > 0)) button_id = widget_id;
        }
        if (button_id == STACK_NO_OBJECT)
        {
            acu_release(the_card);
            return NULL;
        }
        
        /* return a variant reference to the field */
        StackHandle the_button = acu_handle_for_button_id(STACKMGR_FLAG_MAN_RELEASE, the_card, button_id);
        acu_release(the_card);
        return xte_object_create(in_engine, "cdbtn", the_button, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
    }
    
    /* are we trying to get all the card buttons? */
    else if (in_param_count == 0)
    {
        int *the_count = malloc(sizeof(int));
        assert(the_count != NULL);
        
        /* count the number of card buttons */
        *the_count = 0;
        int widget_count = (int)stack_widget_count(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT);
        for (int i = 0; i < widget_count; i++)
        {
            long widget_id = stack_widget_n(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT, i);
            if (!stack_widget_is_field(in_stack->stack, widget_id)) (*the_count)++;
        }
        
        /* return a count */
        acu_release(the_card);
        return xte_object_create(in_engine, "objcount", the_count, (XTEObjectDeallocator)&_acu_variant_int_destructor);
    }
    
    /* not allowed to get a range of card buttons */
    acu_release(the_card);
    return NULL;
}


/*
 *  _obj_cd_btn_named
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card button by name.
 */
XTEVariant* _obj_cd_btn_named(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* validate and convert the parameters */
    if (in_param_count != 1) return NULL;
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_STRING)) return NULL;
    
    /* get current card or owner card (if specified) */
    StackHandle the_card = _owner_card(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* lookup the button by name */
    const char *name = xte_variant_as_cstring(in_params[0]);
    long button_id = stack_widget_named(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT, name);
    if ((button_id != STACK_NO_OBJECT) && stack_widget_is_field(in_stack->stack, button_id)) button_id = STACK_NO_OBJECT;
    if (button_id == STACK_NO_OBJECT)
    {
        acu_release(the_card);
        return NULL;
    }
    
    /* return a variant reference to the button */
    StackHandle the_button = acu_handle_for_button_id(STACKMGR_FLAG_MAN_RELEASE, the_card, button_id);
    acu_release(the_card);
    return xte_object_create(in_engine, "cdbtn", the_button, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
}


/*
 *  _obj_cd_btn_id
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card button by ID.
 */
XTEVariant* _obj_cd_btn_id(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                           XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* validate and convert the parameters */
    if (in_param_count != 1) return NULL;
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_INTEGER)) return NULL;
    
    /* get current card or owner card (if specified) */
    StackHandle the_card = _owner_card(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* lookup the button by ID */
    long button_id = xte_variant_as_int(in_params[0]);
    button_id = stack_widget_id(in_stack->stack, HDEF(the_card).layer_id, STACK_NO_OBJECT, button_id);
    if ((button_id != STACK_NO_OBJECT) && stack_widget_is_field(in_stack->stack, button_id)) button_id = STACK_NO_OBJECT;
    if (button_id == STACK_NO_OBJECT)
    {
        acu_release(the_card);
        return NULL;
    }
    
    /* return a variant reference to the button */
    StackHandle the_button = acu_handle_for_button_id(STACKMGR_FLAG_MAN_RELEASE, the_card, button_id);
    acu_release(the_card);
    return xte_object_create(in_engine, "cdbtn", the_button, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
}


/*
 *  _obj_bg_btn_range
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a background button by number, or the number of background buttons.
 */
XTEVariant* _obj_bg_btn_range(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* get current background or owner background (if specified) */
    StackHandle the_bkgnd = _owner_bkgnd(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* are we trying to get a single button? */
    if (in_param_count == 1)
    {
        /* lookup the button by number */
        int number = xte_variant_as_int(in_params[0]);
        long button_id = STACK_NO_OBJECT;
        long widget_count = (int)stack_widget_count(in_stack->stack, STACK_NO_OBJECT, HDEF(the_bkgnd).layer_id);
        for (long i = 0, widget_num = 0; i < widget_count; i++)
        {
            long widget_id = stack_widget_n(in_stack->stack, STACK_NO_OBJECT, HDEF(the_bkgnd).layer_id, i);
            if (!stack_widget_is_field(in_stack->stack, widget_id)) widget_num++;
            if ((widget_num == number) && (widget_num > 0)) button_id = widget_id;
        }
        if (button_id == STACK_NO_OBJECT)
        {
            acu_release(the_bkgnd);
            return NULL;
        }
        
        /* return a variant reference to the field */
        StackHandle the_button = acu_handle_for_button_id(STACKMGR_FLAG_MAN_RELEASE, the_bkgnd, button_id);
        acu_release(the_bkgnd);
        return xte_object_create(in_engine, "bgbtn", the_button, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
    }
    
    /* are we trying to get all the card buttons? */
    else if (in_param_count == 0)
    {
        int *the_count = malloc(sizeof(int));
        assert(the_count != NULL);
        
        /* count the number of card buttons */
        *the_count = 0;
        int widget_count = (int)stack_widget_count(in_stack->stack, STACK_NO_OBJECT, HDEF(the_bkgnd).layer_id);
        for (int i = 0; i < widget_count; i++)
        {
            long widget_id = stack_widget_n(in_stack->stack, STACK_NO_OBJECT, HDEF(the_bkgnd).layer_id, i);
            if (!stack_widget_is_field(in_stack->stack, widget_id)) (*the_count)++;
        }
        
        /* return a count */
        acu_release(the_bkgnd);
        return xte_object_create(in_engine, "objcount", the_count, (XTEObjectDeallocator)&_acu_variant_int_destructor);
    }
    
    /* not allowed to get a range of card buttons */
    acu_release(the_bkgnd);
    return NULL;
}


/*
 *  _obj_bg_btn_named
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card button by name.
 */
XTEVariant* _obj_bg_btn_named(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* validate and convert the parameters */
    if (in_param_count != 1) return NULL;
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_STRING)) return NULL;
    
    /* get current background or owner background (if specified) */
    StackHandle the_bkgnd = _owner_bkgnd(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* lookup the button by name */
    const char *name = xte_variant_as_cstring(in_params[0]);
    long button_id = stack_widget_named(in_stack->stack, STACK_NO_OBJECT, HDEF(the_bkgnd).layer_id, name);
    if ((button_id != STACK_NO_OBJECT) && stack_widget_is_field(in_stack->stack, button_id)) button_id = STACK_NO_OBJECT;
    if (button_id == STACK_NO_OBJECT)
    {
        acu_release(the_bkgnd);
        return NULL;
    }
    
    /* return a variant reference to the button */
    StackHandle the_button = acu_handle_for_button_id(STACKMGR_FLAG_MAN_RELEASE, the_bkgnd, button_id);
    acu_release(the_bkgnd);
    return xte_object_create(in_engine, "bgbtn", the_button, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
}


/*
 *  _obj_bg_btn_id
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card button by ID.
 */
XTEVariant* _obj_bg_btn_id(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                           XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* validate and convert the parameters */
    if (in_param_count != 1) return NULL;
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_INTEGER)) return NULL;
    
    /* get current background or owner background (if specified) */
    StackHandle the_bkgnd = _owner_bkgnd(in_stack, in_owner);
    if (in_owner != NULL) return NULL;
    
    /* lookup the button by ID */
    long button_id = xte_variant_as_int(in_params[0]);
    button_id = stack_widget_id(in_stack->stack, STACK_NO_OBJECT, HDEF(the_bkgnd).layer_id, button_id);
    if ((button_id != STACK_NO_OBJECT) && stack_widget_is_field(in_stack->stack, button_id)) button_id = STACK_NO_OBJECT;
    if (button_id == STACK_NO_OBJECT)
    {
        acu_release(the_bkgnd);
        return NULL;
    }
    
    /* return a variant reference to the button */
    StackHandle the_button = acu_handle_for_button_id(STACKMGR_FLAG_MAN_RELEASE, the_bkgnd, button_id);
    acu_release(the_bkgnd);
    return xte_object_create(in_engine, "bgbtn", the_button, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
}



/*********
 Cards
 */

/*
 *  _obj_card_range
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card by number, or the number of cards.
 */
XTEVariant* _obj_card_range(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* if owner is specified, ensure it's either a bkgnd or stack */
    if ((in_owner) && (!xte_variant_is_class(in_owner, "bkgnd")) && (!xte_variant_is_class(in_owner, "stack")))
        return NULL;
    
    /* are we trying to get a single card? */
    if (in_param_count == 1)
    {
        /* lookup the card by number */
        int number = xte_variant_as_int(in_params[0]);
        long card_id = stack_card_id_for_index(in_stack->stack, number-1);
        if (card_id == STACK_NO_OBJECT) return NULL;
        
        /* return a variant reference to the card */
        StackHandle the_button = acu_handle_for_card_id(STACKMGR_FLAG_MAN_RELEASE, in_stack->stack, card_id);
        return xte_object_create(in_engine, "card", the_button, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
    }
    
    /* are we trying to get all the cards? */
    else if (in_param_count == 0)
    {
        int *the_count = malloc(sizeof(int));
        assert(the_count != NULL);
        
        /* count the number of cards */
        if (xte_variant_is_class(in_owner, "bkgnd"))
            *the_count = (int)stack_bkgnd_card_count(in_stack->stack, HDEF(xte_variant_object_data_ptr(in_owner)).layer_id);
        else
            *the_count = (int)stack_card_count(in_stack->stack);
        
        /* return a count */
        return xte_object_create(in_engine, "objcount", the_count, (XTEObjectDeallocator)&_acu_variant_int_destructor);
    }
    
    /* not allowed to get a range of cards */
    return NULL;
}


/*
 *  _obj_card_named
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card by name.
 */
XTEVariant* _obj_card_named(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* validate and convert the parameters */
    if (in_param_count != 1) return NULL;
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_STRING)) return NULL;
    
    /* lookup the card by name */
    const char *name = xte_variant_as_cstring(in_params[0]);
    long card_id = stack_card_id_for_name(in_stack->stack, name);
    if (card_id == STACK_NO_OBJECT) return NULL;
    
    /* return a variant reference to the card */
    StackHandle the_button = acu_handle_for_card_id(STACKMGR_FLAG_MAN_RELEASE, in_stack->stack, card_id);
    return xte_object_create(in_engine, "card", the_button, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
}


/*
 *  _obj_card_id
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a reference to a card by ID.
 */
XTEVariant* _obj_card_id(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                           XTEVariant *in_params[], int in_param_count)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    /* validate and convert the parameters */
    if (in_param_count != 1) return NULL;
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_INTEGER)) return NULL;
    
    /* lookup the card by ID */
    long card_id = xte_variant_as_int(in_params[0]);
    long card_index = stack_card_index_for_id(in_stack->stack, card_id);
    if (card_index == STACK_NO_OBJECT) return NULL;
    
    /* return a variant reference to the card */
    StackHandle the_button = acu_handle_for_card_id(STACKMGR_FLAG_MAN_RELEASE, in_stack->stack, card_id);
    return xte_object_create(in_engine, "card", the_button, (XTEObjectDeallocator)&_acu_variant_handle_destructor);
}







