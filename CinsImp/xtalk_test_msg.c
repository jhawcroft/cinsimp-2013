/*
 
 xTalk Engine Tests: Message Protocol
 xtalk_test_msg.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Unit tests for the Message Protocol; commands and expression interpretation
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


#if XTALK_TESTS


static XTE *g_test_engine;
static char* g_test_script_error = NULL;
static char* g_test_result = NULL;


static void _handle_message_result(XTE *in_engine, void *incontext, const char *in_result)
{
    if (g_test_result) free(g_test_result);
    g_test_result = _xte_clone_cstr(in_engine, in_result);
}


static void _handle_script_error(XTE *in_engine, void *incontext, XTEVariant *in_source_object, long in_source_line,
                                 const char *in_message, char const *in_arg1, char const *in_arg2, char const *in_arg3, int in_runtime)
{
    if (g_test_script_error) free(g_test_script_error);
    g_test_script_error = _xte_clone_cstr(in_engine, in_message);
}


static struct XTECallbacks callbacks = {
    &_handle_message_result,
    &_handle_script_error,
};


void _xte_message_proto_test(void)
{
    /* setup test environment */
    g_test_engine = xte_create(NULL);
    xte_configure_callbacks(g_test_engine, callbacks);
    g_test_script_error = _xte_clone_cstr(g_test_engine, "");
    g_test_result = _xte_clone_cstr(g_test_engine, "");
    
    /* perform tests... */
    xte_message(g_test_engine, "put the itemDelimiter && \"is the item delimiter.\" into theMessage", NULL);
    if (strlen(g_test_script_error) != 0)
        printf("%s:%d: Message Protocol test: failed!\n%s\n", __FILE__, __LINE__, g_test_script_error);
    
    xte_message(g_test_engine, "three + seven * 2", NULL);
    if (strlen(g_test_script_error) != 0)
        printf("%s:%d: Message Protocol test: failed!\n%s\n", __FILE__, __LINE__, g_test_script_error);
    if (strcmp(g_test_result, "17") != 0)
        printf("%s:%d: Message Protocol test: failed!\n%s\n", __FILE__, __LINE__, g_test_result);
    
    xte_message(g_test_engine, "three + seven * 2 div 0", NULL);
    if (strcmp(g_test_script_error, "Can't divide by zero.") != 0)
        printf("%s:%d: Message Protocol test: failed!\n%s\n", __FILE__, __LINE__, g_test_script_error);
    if (strcmp(g_test_result, "17") != 0)
        printf("%s:%d: Message Protocol test: failed!\n%s\n", __FILE__, __LINE__, g_test_result);
}



#endif

