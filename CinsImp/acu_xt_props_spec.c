/*
 
 Application Control Unit: xTalk Thread: Special Property Access/Mutation
 acu_xt_props.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Environmental glue code to allow access/mutation of special properties.
 
 *************************************************************************************************
 */

#include "acu_int.h"


/*********
 Object Counts
 */

/*
 *  _acu_xt_objcount_get_number
 *  ---------------------------------------------------------------------------------------------
 *  Return the number of objects previously counted by an object reference.
 */
XTEVariant* _acu_xt_objcount_get_number(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    //assert(IS_XTE(in_engine));
    assert(in_stack != NULL);
    
    if (!xte_variant_is_class(in_owner, "objcount")) return NULL;
    int *the_count = (int*)xte_variant_object_data_ptr(in_owner);
    return xte_integer_create(in_engine, *the_count);
}


/*********
 All Objects
 */

XTEVariant* _acu_xt_obj_get_id(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation)
{
    StackHandle object = xte_variant_object_data_ptr(in_object);
    switch (HDEF(object).type)
    {
        case STACKMGR_TYPE_BUTTON:
        case STACKMGR_TYPE_FIELD:
            return xte_integer_create(in_engine, (int)HDEF(object).widget_id);
        case STACKMGR_TYPE_CARD:
        case STACKMGR_TYPE_BKGND:
            return xte_integer_create(in_engine, (int)HDEF(object).layer_id);
    }
    return NULL;
}

