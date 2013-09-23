/*
 
 xTalk Engine Tests: Memory
 xtalk_test_mem.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Unit tests for memory allocation
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


#if XTALK_TESTS



void _xte_mem_breakpoint_set(int in_bp);


static XTE *g_test_engine;
static char* g_test_script_error = NULL;
static char* g_test_result = NULL;


static void _handle_message_result(XTE *in_engine, void *in_context, const char *in_result)
{
    //printf("RESULT: \"%s\"\n", in_result);
    if (g_test_result) free(g_test_result);
    g_test_result = _xte_clone_cstr(in_engine, in_result);
}


static void _handle_script_error(XTE *in_engine, void *in_context, XTEVariant *in_source_object, long in_source_line,
                                 const char *in_message, char const *in_arg1, char const *in_arg2, char const *in_arg3, int in_runtime)
{
    //printf("ERROR: \"%s\"\n", in_message);
    if (g_test_script_error) free(g_test_script_error);
    g_test_script_error = _xte_clone_cstr(in_engine, in_message);
}


static struct XTECallbacks callbacks = {
    &_handle_message_result,
    &_handle_script_error,
};




/*
 
 !! IMPORTANT NOTE
 
 We will need to reset errors, prior to taking a memory reading, and also be aware that
 the lexer when it encounters a line comment automatically outputs a newline, even when one
 wasn't present - thus you need to chop it off using _xte_trim_last_newline (_engine.c)
 prior to any kind of parsing lest raise an error 
 
 Errors already existing prior to beginning a particular test will often result in inconsistent
 numbers and consequently a failed test - due to the effects of replacing one error of a specific
 length with another error of a different length.
 
 */




void _xte_memory_test(void)
{
    /* setup the test environment */
    long before, after;
    g_test_engine = xte_create(NULL);
    xte_configure_callbacks(g_test_engine, callbacks);
    g_test_script_error = _xte_clone_cstr(g_test_engine, "");
    g_test_result = _xte_clone_cstr(g_test_engine, "");
    
    /* run the tests... */
    XTEAST *tree;
    
    /* lexer */
    before = _xte_mem_allocated();
    tree = _xte_lex(g_test_engine,
                           "x\ry\nz\r\nlong\xC2\xAC\nline\nlong\xC2\xAC\rline\nlong\xC2\xAC\r\nline"
                           "\nlong \t\xC2\xAC\t \thidden\nline --comment here\nnew line\n"
                           ".nine.9text0.1text 0.1 3.14159 -7 + -8.1 * (-1.1) is = is not <> \xE2\x89\xA0"
                           "\xE2\x89\xA4 <= \xE2\x89\xA5 >= is in contains is not in"
                           " is within is not within there is a there is no there is apple "
                           "^*/div mod+& &&\"Quoted string literal\" and or not of in,functionaire,function"
                           " end exit global if then else next on pass repeat passes(pass)zpass,passz,"
                           "end5_3id _1name end 5.2pass.2pass3.14$%pete");
    _xte_ast_destroy(tree);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
    /* expression parser */
    _xte_reset_errors(g_test_engine);
    before = _xte_mem_allocated();
    tree = _xte_lex(g_test_engine, "3 * 2");
    _xte_parse_expression(g_test_engine, tree, 0);
    _xte_ast_destroy(tree);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
    _xte_reset_errors(g_test_engine);
    before = _xte_mem_allocated();
    tree = _xte_lex(g_test_engine, "5 - sqrt(-9) && any char of word 1 to 1 of the name of the system is not PI");
    _xte_parse_expression(g_test_engine, tree, 0);
    _xte_ast_destroy(tree);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
    /* command parser */
    _xte_reset_errors(g_test_engine);
    before = _xte_mem_allocated();
    tree = _xte_lex(g_test_engine, "set the itemdelimiter to \":\" -- commented words");
    _xte_parse_command(g_test_engine, tree, 0);
    _xte_ast_destroy(tree);
    _xte_reset_errors(g_test_engine);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
    _xte_reset_errors(g_test_engine);
    before = _xte_mem_allocated();
    tree = _xte_lex(g_test_engine, "put \"Hello World!\" into theVar");
    _xte_parse_command(g_test_engine, tree, 0);
    _xte_ast_destroy(tree);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
    /* the Message Protocol */
    _xte_reset_errors(g_test_engine);
    before = _xte_mem_allocated();
    xte_message(g_test_engine, "3 * 2", NULL);
    _handle_message_result(g_test_engine, NULL, "");
    _xte_set_result(g_test_engine, NULL);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
    _xte_reset_errors(g_test_engine);
    _handle_script_error(g_test_engine, NULL, NULL, 0, "", "", "", "", 0);
    before = _xte_mem_allocated();
    xte_message(g_test_engine, "set the itemdelimiter to \":\" -- commented words", NULL);
    _xte_set_result(g_test_engine, NULL);
    _xte_reset_errors(g_test_engine);
    _handle_script_error(g_test_engine, NULL, NULL, 0, "", "", "", "", 0);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
    _xte_reset_errors(g_test_engine);
    before = _xte_mem_allocated();
    xte_message(g_test_engine, "put \"Tests successful!!!\"", NULL);
    _handle_message_result(g_test_engine, NULL, "");
    _xte_set_result(g_test_engine, NULL);
    _xte_reset_errors(g_test_engine);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
    _xte_reset_errors(g_test_engine);
    before = _xte_mem_allocated();
    xte_message(g_test_engine, "sum(5,19,3) - 1", NULL);
    _handle_message_result(g_test_engine, NULL, "");
    _xte_set_result(g_test_engine, NULL);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
   
    _xte_reset_errors(g_test_engine);
    before = _xte_mem_allocated();
    xte_message(g_test_engine, "any item of line 2 of \"hello there\"", NULL);
    _handle_message_result(g_test_engine, NULL, "");
    _xte_set_result(g_test_engine, NULL);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
    _xte_reset_errors(g_test_engine);
    before = _xte_mem_allocated();
    xte_message(g_test_engine, "last char of abc", NULL);
    _handle_message_result(g_test_engine, NULL, "");
    _xte_set_result(g_test_engine, NULL);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
    /* stability */
    xte_message(g_test_engine, "put sqrt(9) into x", NULL); /* store the variable prior to mem check */
    before = _xte_mem_allocated();
    for (int i = 0; i < 1; i++)
    {
        xte_message(g_test_engine, "any item of line (95 - 94 ^ 1) of \"hello there\"", NULL);
        xte_message(g_test_engine, "set the itemdelimiter to \",\"", NULL);
        xte_message(g_test_engine, "put sqrt(9) into x", NULL);
    }
    _handle_message_result(g_test_engine, NULL, "");
    _xte_set_result(g_test_engine, NULL);
    _handle_script_error(g_test_engine, NULL, NULL, -1, "", NULL, NULL, NULL, 0);
    after = _xte_mem_allocated();
    if (after-before != 0)
        printf("%s:%d: memory test failed: leaked: %ld bytes\n", __FILE__, __LINE__, after-before);
    
}



#endif

