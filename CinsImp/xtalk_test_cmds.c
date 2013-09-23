/*
 
 xTalk Engine Tests: Command Parsing
 xtalk_test_cmds.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Unit tests for command parsing
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


#if XTALK_TESTS



static void _xte_tests_parsing_run(char const *in_file, int in_line, struct XTETestParserCase const in_cases[])
{
    /* create an engine */
    XTE *engine = xte_create(NULL);
    
    /* iterate through test cases */
    struct XTETestParserCase const *the_case = in_cases;
    int case_number = 1;
    while (the_case->syntax)
    {
        /* reset errors */
        _xte_reset_errors(engine);
        
        /* run the test */
        XTEAST *tree = _xte_lex(engine, the_case->syntax);
        if (!_xte_parse_command(engine, tree, 0))
        {
            _xte_ast_destroy(tree);
            tree = NULL;
        }
        
        /* check the result */
        if (!the_case->result)
        {
            /* not expecting success */
            if (tree && (engine->error_operational == XTE_ERROR_NONE))
                printf("%s:%d: parser test #%d: failed!\n  Command shouldn't have parsed.\n", in_file, in_line, case_number);
        }
        else
        {
            /* expecting success */
            const char *result = _xte_ast_debug_text(tree, XTE_AST_DEBUG_NO_POINTERS);
            if ( (!the_case->result) || (!result) || (strcmp(the_case->result, result) != 0) )
            {
                result = _xte_ast_debug_text(tree, XTE_AST_DEBUG_NO_POINTERS | XTE_AST_DEBUG_QUOTED_OUTPUT);
                printf("%s:%d: parser test #%d: failed!\n%s\n", in_file, in_line, case_number, result);
            }
        }
        
        /* cleanup */
        _xte_ast_destroy(tree);
        
        /* go next test case */
        case_number++;
        the_case++;
    }
}



static struct XTETestParserCase const TEST_BASIC[] = {
    {
        "put zero into theVariable",
        "COMMAND fPTR=$0 (put)\n"
        "PARAMS:\n"
        "   EXPRESSION (source)\n"
        "      CONSTANT fPTR=$0 (zero)\n"
        "   EXPRESSION (mode)\n"
        "      WORD into (FLAGS=$0)\n"
        "   EXPRESSION (dest)\n"
        "      WORD theVariable (FLAGS=$0)\n"
    },
    {
        "put theVariable",
        "COMMAND fPTR=$0 (put)\n"
        "PARAMS:\n"
        "   EXPRESSION (source)\n"
        "      WORD theVariable (FLAGS=$0)\n"
        "   NULL\n"
        "   NULL\n"
    },
    {
        "set itemDelimiTER to \"~\"",
        "COMMAND fPTR=$0 (set)\n"
        "PARAMS:\n"
        "   EXPRESSION (prop)\n"
        "      WORD itemDelimiTER (FLAGS=$0)\n"
        "   EXPRESSION (value)\n"
        "      STRING \"~\"\n"
    },
    {/*4*/
        "set the itemDelimiTER to \"~\"",
        "COMMAND fPTR=$0 (set)\n"
        "PARAMS:\n"
        "   EXPRESSION (prop)\n"
        "      PROPERTY fPTR-map=0 rep=0 (itemDelimiter)\n"
        "   EXPRESSION (value)\n"
        "      STRING \"~\"\n"
    },
    {
        "testcmd password \"What's the password?\"",
        "COMMAND fPTR=$0 (testcmd)\n"
        "PARAMS:\n"
        "   EXPRESSION (question)\n"
        "      STRING \"What's the password?\"\n"
        "   NULL\n"
        "   NULL\n"
    },
    {
        "testcmd password clear \"What's the password?\"",
        "COMMAND fPTR=$0 (testcmd)\n"
        "PARAMS:\n"
        "   EXPRESSION (question)\n"
        "      STRING \"What's the password?\"\n"
        "   EXPRESSION (clearText)\n"
        "      CONSTANT fPTR=$0 (true)\n"
        "   NULL\n"
    },
    {
        "testcmd password clear \"What's the password?\" with \"Not a good password\"",
        "COMMAND fPTR=$0 (testcmd)\n"
        "PARAMS:\n"
        "   EXPRESSION (question)\n"
        "      STRING \"What's the password?\"\n"
        "   EXPRESSION (clearText)\n"
        "      CONSTANT fPTR=$0 (true)\n"
        "   EXPRESSION (defaultReply)\n"
        "      STRING \"Not a good password\"\n"
    },
    {
        "testcmd password \"What's the password?\" with \"ap4sswd\"",
        "COMMAND fPTR=$0 (testcmd)\n"
        "PARAMS:\n"
        "   EXPRESSION (question)\n"
        "      STRING \"What's the password?\"\n"
        "   NULL\n"
        "   EXPRESSION (defaultReply)\n"
        "      STRING \"ap4sswd\"\n"
    },
    {
        "testcmd passwor \"What's the password?\" with \"ap4sswd\"",
        NULL
    },
    {
        "testcmd password clear clear \"What's the password?\" with \"ap4sswd\"",
        NULL
    },
    {/* 11 */
        "testcmd password clear with \"ap4sswd\"",
        NULL
    },
    {
        "testcmd password with \"ap4sswd\"",
        NULL
    },
    NULL
};


void _xte_parse_command_test(void)
{
    _xte_tests_parsing_run(__FILE__, __LINE__, TEST_BASIC);
}



#endif

