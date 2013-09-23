/*
 
 xTalk Engine String
 xtalk_string.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 UTF-8 string manipulation operations and inbuilt functions
 
 */

#include "xtalk_internal.h"



XTEVariant* _xte_op_string_concat(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_STRING)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_STRING)) )
    {
        xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
        return NULL;
    }
    
    long left_length = strlen(in_left->value.utf8_string);
    long new_length = left_length + strlen(in_right->value.utf8_string);
    in_left->value.utf8_string = realloc(in_left->value.utf8_string, new_length + 1);
    if (!in_left->value.utf8_string) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    strcpy(in_left->value.utf8_string + left_length, in_right->value.utf8_string);
    return in_left;
}


XTEVariant* _xte_op_string_concat_sp(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_STRING)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_STRING)) )
    {
        xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
        return NULL;
    }
    
    long left_length = strlen(in_left->value.utf8_string);
    long new_length = left_length + strlen(in_right->value.utf8_string);
    in_left->value.utf8_string = realloc(in_left->value.utf8_string, new_length + 2);
    if (!in_left->value.utf8_string) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    strcpy(in_left->value.utf8_string + left_length + 1, in_right->value.utf8_string);
    in_left->value.utf8_string[left_length] = ' ';
    return in_left;
}


/* is left within right? */
XTEVariant* _xte_op_string_is_in(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_STRING)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_STRING)) )
    {
        xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
        return NULL;
    }
    
    XTEVariant *result = xte_boolean_create(in_engine, _xte_utf8_contains(in_right->value.utf8_string, in_left->value.utf8_string));
    return result;
}


XTEVariant* _xte_op_string_is_not_in(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_STRING)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_STRING)) )
    {
        xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
        return NULL;
    }
    
    XTEVariant *result = xte_boolean_create(in_engine, !_xte_utf8_contains(in_right->value.utf8_string, in_left->value.utf8_string));
    return result;
}






