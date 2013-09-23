/*
 
 xTalk Engine Tests: Handler Parsing
 xtalk_test_exprs.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Unit tests for handler parsing, and harness for loop and conditional tests: _loop.c, _cond.c
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"



#if XTALK_TESTS


int _xte_parse_handler(XTE *in_engine, XTEAST *in_stream, int const in_checkpoints[], int in_checkpoint_count, int in_checkpoint_offset);


char const *_g_error_msg = NULL;


static void _handle_script_error(XTE *in_engine, void *in_context, XTEVariant *in_object, long in_source_line, char const *in_template,
             char const *in_arg1, char const *in_arg2, char const *in_arg3, int in_runtime)
{
    static char buffer[4096];
    if ((in_arg1) && (in_arg2) && (in_arg3))
        sprintf(buffer, in_template, in_arg1, in_arg2, in_arg3);
    else if ((in_arg1) && (in_arg2))
        sprintf(buffer, in_template, in_arg1, in_arg2);
    else if ((in_arg1))
        sprintf(buffer, in_template, in_arg1);
    else
        sprintf(buffer, "%s", in_template);
    _g_error_msg = buffer;
}


static void _xte_tests_parsing_run(char const *in_file, int in_line, struct XTETestParserCase const in_cases[])
{
    int dummy_checkpoints[1];
    
    /* create an engine */
    XTE *engine = xte_create(NULL);
    struct XTECallbacks callbacks = {
        NULL,
        (XTEScriptErrorCB)&_handle_script_error,
        NULL,
    };
    xte_configure_callbacks(engine, callbacks);
    
    /* iterate through test cases */
    struct XTETestParserCase const *the_case = in_cases;
    int case_number = 1;
    while (the_case->syntax)
    {
        /* reset errors */
        _xte_reset_errors(engine);
        
        /* run the test */
        _g_error_msg = NULL;
        XTEAST *tree = _xte_lex(engine, the_case->syntax);
        if (!_xte_parse_handler(engine, tree, dummy_checkpoints, 0, 0))
        {
            _xte_ast_destroy(tree);
            tree = NULL;
        }
        _xte_error_post(engine);
        
        /* check the result */
        const char *result = NULL;
        if (tree) result = _xte_ast_debug_text(tree, XTE_AST_DEBUG_NO_POINTERS);
        if (_g_error_msg != NULL) result = _g_error_msg;
        
        if ( (!result) || (strcmp(the_case->result, result) != 0) )
        {
            if (!_g_error_msg)
                result = _xte_ast_debug_text(tree, XTE_AST_DEBUG_NO_POINTERS | XTE_AST_DEBUG_QUOTED_OUTPUT);
            printf("%s:%d: parser test #%d: failed!\n%s\n", in_file, in_line, case_number, result);
        }
        
        /* cleanup */
        _xte_ast_destroy(tree);
        
        /* go next test case */
        case_number++;
        the_case++;
    }
}



static struct XTETestParserCase const TEST_HANDLERS[] = {
    {
        "on mouseUp\n"
        "  beep 2\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   COMMAND fPTR=$0 (beep)\n"
        "   PARAMS:\n"
        "      EXPRESSION ()\n"
        "         INTEGER 2\n"
    },
    {
        "on mouseUp\n"
        "  beep 2\n"
        "end mouseUpz"
        ,
        "Expected \"end mouseUp\"."
    },
    {
        "on mouseUp x,\n"
        "  beep 2\n"
        "end mouseUpz"
        ,
        "Expected parameter name after \",\"."
    },
    {
        "on mouseUp x,z blob\n"
        "  beep 2\n"
        "end mouseUpz"
        ,
        "Expected end of line after parameters."
    },
    {
        "on mouseUp 7\n"
        "  beep 2\n"
        "end mouseUpz"
        ,
        "Expected parameters or end of line after \"mouseUp\"."
    },
    {
        "on mouseUp\n"
        "  beep 2\n"
        "endz mouseUp"
        ,
        "Expected \"end\" after \"on\"."
    },
    {
        "on mouseUp\n"
        "  beep 2\n"
        "bob end mouseUp"
        ,
        "\"end mouseUp\" should be at the beginning of the line."
    },
    {
        "function pickles x,y\n"
        "  return x * y\n"
        "end pickles"
        ,
        "HANDLER F \"pickles\"\n"
        "   PARAM-NAMES\n"
        "      WORD x (FLAGS=$0)\n"
        "      WORD y (FLAGS=$0)\n"
        "   EXIT handler (return)\n"
        "      EXPRESSION ()\n"
        "         OPERATOR multiply\n"
        "            WORD x (FLAGS=$0)\n"
        "            WORD y (FLAGS=$0)\n"
    },
    {
        "on mouseUp\n"
        "  beep 2\n"
        "  end robert\n"
        "end mouseUp"
        ,
        "Expected \"end mouseUp\" here."
    },
    NULL
};


extern struct XTETestParserCase const _TEST_LOOPS_OK[];
extern struct XTETestParserCase const _TEST_LOOPS_ERROR[];

extern struct XTETestParserCase const _TEST_IFS_ERROR[];
extern struct XTETestParserCase const _TEST_IFS_OK[];

extern struct XTETestParserCase const _TEST_STMTS_ERROR[];
extern struct XTETestParserCase const _TEST_STMTS_OK[];


static struct XTETestParserCase const TEST_CHECK[] = {
    {
        "on mouseUp\n"
        "  repeat\n"
        "    if cookies = 2 then put \"2 cookies\"\n"
        "    else\n"
        "    beep 8\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected \"end if\" but found \"end repeat\"."
    },
    {
        "on mouseUp\n"
        "  repeat\n"
        "    if cookies = 2 then put \"2 cookies\"\n"
        "    else\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected \"end if\" but found \"end repeat\"."
    },
    NULL
};


void _xte_parse_handler_test(void)
{
    _xte_tests_parsing_run(__FILE__, __LINE__, TEST_HANDLERS);
    
    _xte_tests_parsing_run(__FILE__, __LINE__, _TEST_LOOPS_OK);
    _xte_tests_parsing_run(__FILE__, __LINE__, _TEST_LOOPS_ERROR);
    
    _xte_tests_parsing_run(__FILE__, __LINE__, _TEST_IFS_ERROR);
    _xte_tests_parsing_run(__FILE__, __LINE__, _TEST_IFS_OK);
    
    _xte_tests_parsing_run(__FILE__, __LINE__, _TEST_STMTS_ERROR);
    _xte_tests_parsing_run(__FILE__, __LINE__, _TEST_STMTS_OK);
    
    //_xte_tests_parsing_run(__FILE__, __LINE__, TEST_CHECK);
}



#endif
