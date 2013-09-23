/*
 
 xTalk Engine Mutate Unit
 xtalk_cmds.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Implementation of built-in mutatation commands; put and set
 
 */

#include "xtalk_internal.h"


static void _xte_cmd_put(XTE *in_engine, void *in_context, XTEVariant *in_params[], int in_param_count)
{
    //printf("PUT A: %ld\n", _xte_mem_allocated());
    
    const char* mode = xte_variant_as_cstring(in_params[1]);
    
    if ((strcmp(mode, "into") == 0) && (in_params[2]) && xte_variant_is_variable(in_params[2]))
    {
        if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_VALUE))
        {
            xte_callback_error(in_engine, "Expected value here.", NULL, NULL, NULL);
            return;
        }
    }
    else
    {
        if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_STRING))
        {
            xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
            return;
        }
    }
    //printf("PUT B: %ld\n", _xte_mem_allocated());
    
    
    if (!in_params[2])
    {
        /* print the value using the message result handler */
        in_engine->callback.message_result(in_engine, in_engine->context, xte_variant_as_cstring(in_params[0]));
        return;
    }
    
    if (!xte_variant_is_container(in_params[2]))
    {
        xte_callback_error(in_engine, "Expected container here.", NULL, NULL, NULL);
        return;
    }
    
    
    int success = XTE_FALSE;
    
    
    
    if (strcmp(mode, "after") == 0)
        success = xte_container_write(in_engine, in_params[2], in_params[0], XTE_TEXT_RANGE_ALL, XTE_PUT_AFTER);
    else if (strcmp(mode, "before") == 0)
        success = xte_container_write(in_engine, in_params[2], in_params[0], XTE_TEXT_RANGE_ALL, XTE_PUT_BEFORE);
    else
        success = xte_container_write(in_engine, in_params[2], in_params[0], XTE_TEXT_RANGE_ALL, XTE_PUT_INTO);
    
    
    //printf("PUT C: %ld\n", _xte_mem_allocated());
    
    if (!success)
    {
        xte_callback_error(in_engine, "That container can't be modified.", NULL, NULL, NULL);
        return;
    }
}


static void _xte_cmd_get(XTE *in_engine, void *in_context, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_VALUE))
    {
        xte_callback_error(in_engine, "Expected value here.", NULL, NULL, NULL);
        return;
    }
    /*XTEVariant* _xte_resolve_value(XTE *in_engine, XTEVariant *in_variant);
    
    _xte_resolve_value(in_engine, in_params[0]);
    */
    xte_set_global(in_engine, "it", in_params[0]);
}



struct XTEPropertyPtr* _xte_property_named(XTE *in_engine, const char *in_name);

static void _xte_cmd_set(XTE *in_engine, void *in_context, XTEVariant *in_params[], int in_param_count)
{
    int is_valid_prop = XTE_FALSE;
    
    /* check if property specifier is valid */
    if (xte_variant_type(in_params[0]) == XTE_TYPE_PROP)
        is_valid_prop = XTE_TRUE;
    
    /* special case: we can allow late detection of a global property name (without preceeding "the")
     in this command. */
    XTEVariant *alt_prop_ref = NULL;
    if ((!is_valid_prop) && (xte_variant_type(in_params[0]) == XTE_TYPE_GLOBAL))
    {
        struct XTEPropertyPtr *ptrs = _xte_property_named(in_engine, in_params[0]->value.utf8_string);
        if (ptrs)
        {
            alt_prop_ref = xte_property_ref(in_engine, ptrs, NULL, XTE_PROPREP_NORMAL);
            is_valid_prop = XTE_TRUE;
        }
        //printf("Looking up non-the prefixed property\n");
    }
    
    /* raise error if no property identified */
    if (!is_valid_prop)
    {
        xte_callback_error(in_engine, "Expected property name.", NULL, NULL, NULL);
        return;
    }
    
    /* attempt to write the property */
    if (!xte_property_write(in_engine, (alt_prop_ref?alt_prop_ref:in_params[0]), in_params[1]))
    {
        xte_callback_error(in_engine, "That property is read-only.", NULL, NULL, NULL);
        return;
    }
    
    /* cleanup */
    if (alt_prop_ref) xte_variant_release(alt_prop_ref);
}


struct XTECommandDef _xte_builtin_mutator_cmds[] = {
#if XTALK_TESTS
    {
        "testcmd [password [`clearText``true`clear]] <question> [with <defaultReply>]",
        "question,clearText,defaultReply",
        NULL,
    },
#endif
    {
        "put <source> [{`mode``into`into|`after`after|`before`before} <dest>]",
        "source,mode,dest",
        &_xte_cmd_put,
    },
    {
        "set <prop> to <value>",
        "prop,value",
        &_xte_cmd_set,
    },
    {
        "get <expr>",
        "expr",
        &_xte_cmd_get,
    },
    NULL,
};

