/*
 
 Application Control Unit - View
 acu_view.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Services affecting the currently loaded card view; includes tracking the current card and 
 background, enabling various ACU services to function without interaction with the UI
 
 *************************************************************************************************
 */

#include "acu_int.h"


long _stackmgr_current_card_id(StackMgrStack *in_registry_entry)
{
    long card_id = stack_sorting_card(in_registry_entry->stack);
    if (card_id == STACK_NO_OBJECT) card_id = in_registry_entry->__current_card_id;
    return card_id;
}


long _stackmgr_current_bkgnd_id(StackMgrStack *in_registry_entry)
{
    long sorting_card_id = stack_sorting_card(in_registry_entry->stack);
    if (sorting_card_id == STACK_NO_OBJECT)
        return in_registry_entry->__current_bkgnd_id;
    else
        return stack_card_bkgnd_id(in_registry_entry->stack, sorting_card_id);
}


long stackmgr_current_card_id(StackHandle in_handle)
{
    StackMgrStack *register_entry = _stackmgr_stack(in_handle);
    assert(register_entry != NULL);
    return _stackmgr_current_card_id(register_entry);
}


long stackmgr_current_bkgnd_id(StackHandle in_handle)
{
    StackMgrStack *register_entry = _stackmgr_stack(in_handle);
    assert(register_entry != NULL);
    return _stackmgr_current_bkgnd_id(register_entry);
}


/* **TODO**: eventually we should probably handle the go to, but right now, the view notifies us when it changes card */
void stackmgr_set_current_card_id(StackHandle in_stack, long in_card_id)
{
    if (in_card_id < 1) return;
    StackMgrStack *register_entry = _stackmgr_stack(in_stack);
    assert(register_entry != NULL);
    
    register_entry->__current_card_id = in_card_id;
    register_entry->__current_bkgnd_id = stack_card_bkgnd_id(register_entry->stack, in_card_id);
}




