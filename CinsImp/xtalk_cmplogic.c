/*
 
 xTalk Engine Comparative
 xtalk_math.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Comparison operations and functions
 
 */

#include "xtalk_internal.h"


XTEVariant* _xte_op_logic_not(XTE *in_engine, XTEVariant *in_right)
{
    if (!xte_variant_convert(in_engine, in_right, XTE_TYPE_BOOLEAN))
    {
        xte_callback_error(in_engine, "Expected true or false here.", NULL, NULL, NULL);
        return NULL;
    }
    
    in_right->value.boolean = !in_right->value.boolean;
    return in_right;
}


XTEVariant* _xte_op_logic_and(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_BOOLEAN)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_BOOLEAN)) )
    {
        xte_callback_error(in_engine, "Expected true or false here.", NULL, NULL, NULL);
        return NULL;
    }
    
    in_left->value.boolean = in_left->value.boolean && in_right->value.boolean;
    return in_left;
}


XTEVariant* _xte_op_logic_or(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_BOOLEAN)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_BOOLEAN)) )
    {
        xte_callback_error(in_engine, "Expected true or false here.", NULL, NULL, NULL);
        return NULL;
    }
    
    in_left->value.boolean = in_left->value.boolean || in_right->value.boolean;
    return in_left;
}


XTEVariant* _xte_op_comp_eq(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    int result = _xte_compare_variants(in_engine, in_left, in_right);
    return xte_boolean_create(in_engine, (result == 0));
}


XTEVariant* _xte_op_comp_neq(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    int result = _xte_compare_variants(in_engine, in_left, in_right);
    return xte_boolean_create(in_engine, (result != 0));
}


XTEVariant* _xte_op_comp_less(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    int result = _xte_compare_variants(in_engine, in_left, in_right);
    return xte_boolean_create(in_engine, (result < 0));
}


XTEVariant* _xte_op_comp_less_eq(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    int result = _xte_compare_variants(in_engine, in_left, in_right);
    return xte_boolean_create(in_engine, (result <= 0));
}


XTEVariant* _xte_op_comp_more(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    int result = _xte_compare_variants(in_engine, in_left, in_right);
    return xte_boolean_create(in_engine, (result > 0));
}


XTEVariant* _xte_op_comp_more_eq(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    int result = _xte_compare_variants(in_engine, in_left, in_right);
    return xte_boolean_create(in_engine, (result >= 0));
}




