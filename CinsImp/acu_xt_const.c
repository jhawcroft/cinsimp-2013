/*
 
 Application Control Unit: xTalk Thread: Constant Access
 acu_xt_const.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Environmental glue code to access 'constant' values relevant to the environment, including:
 
 -  this card
 -  this background
 -  this stack
 
 *************************************************************************************************
 */

#include "acu_int.h"



/*********
 Constants
 */

/*
 *  _acu_xt_const_this_card
 *  ---------------------------------------------------------------------------------------------
 *  Returns an XTEVariant object referencing the current card.
 */
XTEVariant* _acu_xt_const_this_card(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    StackHandle handle = acu_handle_for_current_card(STACKMGR_FLAG_MAN_RELEASE, (StackHandle)in_stack);
    return xte_object_create(in_engine, "card", handle, (XTEObjectDeallocator)_acu_variant_handle_destructor);
}



XTEVariant* _acu_xt_const_this_bkgnd(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    StackHandle handle = acu_handle_for_current_bkgnd(STACKMGR_FLAG_MAN_RELEASE, (StackHandle)in_stack);
    return xte_object_create(in_engine, "bkgnd", handle, (XTEObjectDeallocator)_acu_variant_handle_destructor);
}



XTEVariant* _acu_xt_const_this_stack(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    StackHandle handle = _stackmgr_handle(in_stack);
    return xte_object_create(in_engine, "stack", handle, (XTEObjectDeallocator)_acu_variant_handle_destructor);
    return NULL;
}




