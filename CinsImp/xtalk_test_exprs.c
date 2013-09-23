/*
 
 xTalk Engine Tests: Expression Parsing
 xtalk_test_exprs.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Unit tests for expression parsing
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


#if XTALK_TESTS

static char const *_g_error_msg = NULL;


static void _handle_script_error(XTE *in_engine, void *in_context, XTEVariant *in_source_object, long in_source_line,
                                 char const *in_template, char const *in_arg1, char const *in_arg2, char const *in_arg3, int in_runtime)
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
        if (!_xte_parse_expression(engine, tree, 0))
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
        
        
        
        
        /* check the result */
        /*if ((engine->error_operational != XTE_ERROR_NONE) && (the_case->result))
        {
            printf("%s:%d: parser test #%d: failed!\n  Expression shouldn't have validated.\n", in_file, in_line, case_number);
        }
        else if (engine->error_operational == XTE_ERROR_NONE)
        {
            const char *result = _xte_ast_debug_text(tree, XTE_AST_DEBUG_NO_POINTERS);
            if ( (!the_case->result) || (strcmp(the_case->result, result) != 0) )
            {
                result = _xte_ast_debug_text(tree, XTE_AST_DEBUG_NO_POINTERS | XTE_AST_DEBUG_QUOTED_OUTPUT);
                printf("%s:%d: parser test #%d: failed!\n%s\n", in_file, in_line, case_number, result);
            }
        }*/
        
        
    }
}



static struct XTETestParserCase const TEST_CONSTANTS[] = {
    {
        "pi true newline one",
        "Expected operator but found something else."
    },
    {
        "pi",
        "EXPRESSION ()\n"
        "   CONSTANT fPTR=$0 (pi)\n"
    },
    NULL
};


static struct XTETestParserCase const TEST_PROPERTIES[] = {
    {
        "the itemDelimiter",
        "EXPRESSION ()\n"
        "   PROPERTY fPTR-map=0 rep=0 (itemDelimiter)\n"
    },
    {
        "itemDelimiter",
        "EXPRESSION ()\n"
        "   WORD itemDelimiter (FLAGS=$0)\n"
    },
    {
        "the version",
        "EXPRESSION ()\n"
        "   PROPERTY fPTR-map=0 rep=0 (version)\n"
    },
    {
        "the SyStEm",
        "EXPRESSION ()\n"
        "   PROPERTY fPTR-map=0 rep=0 (system)\n"
    },
    {
        "the _Test_pArser of the SystEm",
        "EXPRESSION ()\n"
        "   PROPERTY fPTR-map=0 rep=0 (_test_parser)\n"
        "      PROPERTY fPTR-map=0 rep=0 (system)\n"
    },
    {
        "_Test_pArser of the SystEm",
        "EXPRESSION ()\n"
        "   PROPERTY fPTR-map=0 rep=0 (_test_parser)\n"
        "      PROPERTY fPTR-map=0 rep=0 (system)\n"
    },
    {
        "number of characters of _Test_pArser of the SystEm",
        "EXPRESSION ()\n"
        "   PROPERTY fPTR-map=0 rep=0 (number)\n"
        "      REFERENCE  [COLLECTION] (\"characters\" collection)\n"
        "        INTEGER fPTR=$0\n"
        "         PROPERTY fPTR-map=0 rep=0 (_test_parser)\n"
        "            PROPERTY fPTR-map=0 rep=0 (system)\n"
    },
    NULL
};


static struct XTETestParserCase const TEST_SYNONYMS[] = {
    {
        "char",
        "Can't understand arguments of chunk expression."
    },
    {
        "char 1 of \"hello\"",
        "EXPRESSION ()\n"
        "   REFERENCE string (element(s) of \"characters\")\n"
        "     INTEGER fPTR=$0\n"
        "      INTEGER 1\n"
        "      STRING \"hello\"\n"
    },
    NULL
};


static struct XTETestParserCase const TEST_FUNCTIONS[] = {
    {
        "abs of",
        "Expected object, property or chunk expression."
    },
    {
        "abs",
        "EXPRESSION ()\n"
        "   WORD abs (FLAGS=$0)\n"
    },
    {
        "abs of -1",
        "EXPRESSION ()\n"
        "   FUNCTION fPTR=$0 (abs)\n"
        "      EXPRESSION ()\n"
        "         OPERATOR negate\n"
        "            INTEGER 1\n"
    },
    {
        "the abs",
        "Expected operator but found something else."
    },
    {
        "the abs of x + 2",
        "EXPRESSION ()\n"
        "   OPERATOR add\n"
        "      FUNCTION fPTR=$0 (abs)\n"
        "         EXPRESSION ()\n"
        "            WORD x (FLAGS=$0)\n"
        "      INTEGER 2\n"
    },
    {
        "min(3,7,1)",
        "EXPRESSION ()\n"
        "   FUNCTION fPTR=$0 (min)\n"
        "      EXPRESSION ()\n"
        "         INTEGER 3\n"
        "      EXPRESSION ()\n"
        "         INTEGER 7\n"
        "      EXPRESSION ()\n"
        "         INTEGER 1\n"
    },
    {
        "min()",
        "EXPRESSION ()\n"
        "   FUNCTION fPTR=$0 (min)\n"
    },
    {
        "user_function(3)",
        "EXPRESSION ()\n"
        "   FUNCTION fPTR=$0 (user_function)\n"
        "      EXPRESSION ()\n"
        "         INTEGER 3\n"
    },
    {
        "the unknown of (3)",
        "Expected object, property or chunk expression."
    },
    {
        "the unknown of 2",
        "Expected object, property or chunk expression."
    },
    NULL
};


static struct XTETestParserCase const TEST_OPERATORS[] = {
    {
        "3 * 2 + 7",
        "EXPRESSION ()\n"
        "   OPERATOR add\n"
        "      OPERATOR multiply\n"
        "         INTEGER 3\n"
        "         INTEGER 2\n"
        "      INTEGER 7\n"
    },
    {
        "3 * (2 + 7)",
        "EXPRESSION ()\n"
        "   OPERATOR multiply\n"
        "      INTEGER 3\n"
        "      EXPRESSION ()\n"
        "         OPERATOR add\n"
        "            INTEGER 2\n"
        "            INTEGER 7\n"
    },
    {
        "3 + 7 > 10",
        "EXPRESSION ()\n"
        "   OPERATOR greater than\n"
        "      OPERATOR add\n"
        "         INTEGER 3\n"
        "         INTEGER 7\n"
        "      INTEGER 10\n"
    },
    {
        "not (3 + 7 > -10)",
        "EXPRESSION ()\n"
        "   OPERATOR not\n"
        "      EXPRESSION ()\n"
        "         OPERATOR greater than\n"
        "            OPERATOR add\n"
        "               INTEGER 3\n"
        "               INTEGER 7\n"
        "            OPERATOR negate\n"
        "               INTEGER 10\n"
    },
    NULL
};


static struct XTETestParserCase const TEST_REFS[] = {
    {
        "any word of char 1 to 7 of the last line of item six of theText",
        "EXPRESSION ()\n"
        "   REFERENCE string (element(s) of \"words\")\n"
        "     INTEGER fPTR=$0\n"
        "      INTEGER -3\n"
        "      REFERENCE string [RANGE] (element(s) of \"characters\")\n"
        "        INTEGER fPTR=$0\n"
        "         INTEGER 1\n"
        "         INTEGER 7\n"
        "         REFERENCE string (element(s) of \"lines\")\n"
        "           INTEGER fPTR=$0\n"
        "            INTEGER -1\n"
        "            REFERENCE string (element(s) of \"items\")\n"
        "              INTEGER fPTR=$0\n"
        "               CONSTANT fPTR=$0 (six)\n"
        "               WORD theText (FLAGS=$0)\n"
    },
    {
        "middle word of char x to (y-3) of the last line of name of the system",
        "EXPRESSION ()\n"
        "   REFERENCE string (element(s) of \"words\")\n"
        "     INTEGER fPTR=$0\n"
        "      INTEGER -2\n"
        "      REFERENCE string [RANGE] (element(s) of \"characters\")\n"
        "        INTEGER fPTR=$0\n"
        "         WORD x (FLAGS=$0)\n"
        "         EXPRESSION ()\n"
        "            OPERATOR subtract\n"
        "               WORD y (FLAGS=$0)\n"
        "               INTEGER 3\n"
        "         REFERENCE string (element(s) of \"lines\")\n"
        "           INTEGER fPTR=$0\n"
        "            INTEGER -1\n"
        "            PROPERTY fPTR-map=0 rep=0 (name)\n"
        "               PROPERTY fPTR-map=0 rep=0 (system)\n"
    },
    {
        "middle word of char ",
        "Can't understand arguments of chunk expression."
    },
    {
        "word",
        "Can't understand arguments of chunk expression."
    },
    {
        "var_name",
        "EXPRESSION ()\n"
        "   WORD var_name (FLAGS=$0)\n"
    },
    {
        "the middle word of",
        "Expected object, property or chunk expression."
    },
    {
        "the middle word of bogus",
        "EXPRESSION ()\n"
        "   REFERENCE string (element(s) of \"words\")\n"
        "     INTEGER fPTR=$0\n"
        "      INTEGER -2\n"
        "      WORD bogus (FLAGS=$0)\n"
    },
    {
        "middle word of bogus",
        "EXPRESSION ()\n"
        "   REFERENCE string (element(s) of \"words\")\n"
        "     INTEGER fPTR=$0\n"
        "      INTEGER -2\n"
        "      WORD bogus (FLAGS=$0)\n"
    },
    NULL
};


static struct XTETestParserCase const TEST_COMPLEX[] = {
    {
        "sum (3,2.5, \"19,5,-3\", abs(-2))",
        "EXPRESSION ()\n"
        "   FUNCTION fPTR=$0 (sum)\n"
        "      EXPRESSION ()\n"
        "         INTEGER 3\n"
        "      EXPRESSION ()\n"
        "         REAL 2.500000\n"
        "      EXPRESSION ()\n"
        "         STRING \"19,5,-3\"\n"
        "      EXPRESSION ()\n"
        "         FUNCTION fPTR=$0 (abs)\n"
        "            OPERATOR negate\n"
        "               INTEGER 2\n"
    },
    {
        "the itemDelimiter & \" is used on: \" & item 2 to 4 of line 2 of name of the system",
        "EXPRESSION ()\n"
        "   OPERATOR concat\n"
        "      OPERATOR concat\n"
        "         PROPERTY fPTR-map=0 rep=0 (itemDelimiter)\n"
        "         STRING \" is used on: \"\n"
        "      REFERENCE string [RANGE] (element(s) of \"items\")\n"
        "        INTEGER fPTR=$0\n"
        "         INTEGER 2\n"
        "         INTEGER 4\n"
        "         REFERENCE string (element(s) of \"lines\")\n"
        "           INTEGER fPTR=$0\n"
        "            INTEGER 2\n"
        "            PROPERTY fPTR-map=0 rep=0 (name)\n"
        "               PROPERTY fPTR-map=0 rep=0 (system)\n"
    },
    NULL
};



static struct XTETestParserCase const EXPR_TEST_ERROR[] = {
    {
        "sum ) 5.0",
        "Can't understand \")\"."
    },
    {
        "sum (5.0",
        "Expected \")\"."
    },
    {
        "5 of the system",
        "Expected object, property or chunk expression."
    },
    {
        "end",
        "Can't understand this."
    },
    {
        "name of",
        "Expected object, property or chunk expression."
    },
    {
        "pickles bob",
        "Expected operator but found something else."
    },
    NULL
};


static struct XTETestParserCase const EXPR_TEST_OK[] = {
    {
        "sum (5.0)",
        "EXPRESSION ()\n"
        "   FUNCTION fPTR=$0 (sum)\n"
        "      EXPRESSION ()\n"
        "         REAL 5.000000\n"
    },
    {
        "sum (5.0 + (3.0))",
        "EXPRESSION ()\n"
        "   FUNCTION fPTR=$0 (sum)\n"
        "      EXPRESSION ()\n"
        "         OPERATOR add\n"
        "            REAL 5.000000\n"
        "            EXPRESSION ()\n"
        "               REAL 3.000000\n"
    },
    {
        "sum ((5.0))",
        "EXPRESSION ()\n"
        "   FUNCTION fPTR=$0 (sum)\n"
        "      EXPRESSION ()\n"
        "         EXPRESSION ()\n"
        "            REAL 5.000000\n"
    },
    {
        "name of the system",
        "EXPRESSION ()\n"
        "   PROPERTY fPTR-map=0 rep=0 (name)\n"
        "      PROPERTY fPTR-map=0 rep=0 (system)\n"
    },
    NULL
};



/* test runner */
void _xte_parse_expression_test(void)
{
    _xte_tests_parsing_run(__FILE__, __LINE__, TEST_SYNONYMS);
    _xte_tests_parsing_run(__FILE__, __LINE__, TEST_CONSTANTS);
    _xte_tests_parsing_run(__FILE__, __LINE__, TEST_PROPERTIES);
    _xte_tests_parsing_run(__FILE__, __LINE__, TEST_FUNCTIONS);
    _xte_tests_parsing_run(__FILE__, __LINE__, TEST_OPERATORS);
    _xte_tests_parsing_run(__FILE__, __LINE__, TEST_REFS);
    _xte_tests_parsing_run(__FILE__, __LINE__, TEST_COMPLEX);
    
    _xte_tests_parsing_run(__FILE__, __LINE__, EXPR_TEST_ERROR);
    _xte_tests_parsing_run(__FILE__, __LINE__, EXPR_TEST_OK);
}





#endif


