/*
 
 xTalk Engine Tests: Lexical Analysis
 xtalk_test_exprs.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Unit tests for lexical analysis 
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"



#if XTALK_TESTS

/* successful test results;
 generated using _xte_ast_debug_text() with the XTE_AST_DEBUG_QUOTED_OUTPUT flag */

static const char *TEST_RESULT_1 = "LIST ()\n";

static const char *TEST_RESULT_2 = "LIST ()\n"
"   WORD x (FLAGS=$0)\n"
"   NEWLINE\n"
"   WORD y (FLAGS=$0)\n"
"   NEWLINE\n"
"   WORD z (FLAGS=$0)\n"
"   NEWLINE\n"
"   WORD long (FLAGS=$0)\n"
"   WORD line (FLAGS=$0)\n"
"   NEWLINE\n"
"   WORD long (FLAGS=$0)\n"
"   WORD line (FLAGS=$0)\n"
"   NEWLINE\n"
"   WORD long (FLAGS=$0)\n"
"   WORD line (FLAGS=$0)\n"
"   NEWLINE\n"
"   WORD long (FLAGS=$0)\n"
"   WORD line (FLAGS=$0)\n"
"   NEWLINE\n"
"   WORD new (FLAGS=$0)\n"
"   WORD line (FLAGS=$0)\n"
"   NEWLINE\n"
"   WORD . (FLAGS=$0)\n"
"   WORD nine (FLAGS=$0)\n"
"   REAL 0.900000\n"
"   WORD text0 (FLAGS=$0)\n"
"   REAL 0.100000\n"
"   WORD text (FLAGS=$0)\n"
"   REAL 0.100000\n"
"   REAL 3.141590\n"
"   OPERATOR subtract\n"
"   INTEGER 7\n"
"   OPERATOR add\n"
"   OPERATOR negate\n"
"   REAL 8.100000\n"
"   OPERATOR multiply\n"
"   PAREN-OPEN\n"
"   OPERATOR negate\n"
"   REAL 1.100000\n"
"   PAREN-CLOSE\n"
"   OPERATOR equal\n"
"   OPERATOR equal\n"
"   OPERATOR not equal\n"
"   OPERATOR not equal\n"
"   OPERATOR not equal\n"
"   OPERATOR less or equal\n"
"   OPERATOR less or equal\n"
"   OPERATOR greater or equal\n"
"   OPERATOR greater or equal\n"
"   OPERATOR is in\n"
"   OPERATOR contains\n"
"   OPERATOR is not in\n"
"   OPERATOR is within\n"
"   OPERATOR is not within\n"
"   OPERATOR there is a\n"
"   OPERATOR there is no\n"
"   WORD there (FLAGS=$0)\n"
"   OPERATOR equal\n"
"   WORD apple (FLAGS=$0)\n"
"   OPERATOR exponent\n"
"   OPERATOR multiply\n"
"   OPERATOR divide fp\n"
"   OPERATOR divide int\n"
"   OPERATOR modulus\n"
"   OPERATOR add\n"
"   OPERATOR concat\n"
"   OPERATOR concat space\n"
"   STRING \"Quoted string literal\"\n"
"   OPERATOR and\n"
"   OPERATOR or\n"
"   OPERATOR not\n"
"   OF\n"
"   IN\n"
"   COMMA\n"
"   WORD functionaire (FLAGS=$0)\n"
"   COMMA\n"
"   KEYWORD function\n"
"   KEYWORD end\n"
"   KEYWORD exit\n"
"   KEYWORD global\n"
"   KEYWORD if\n"
"   KEYWORD then\n"
"   KEYWORD else\n"
"   KEYWORD next\n"
"   KEYWORD on\n"
"   KEYWORD pass\n"
"   KEYWORD repeat\n"
"   WORD passes (FLAGS=$0)\n"
"   PAREN-OPEN\n"
"   KEYWORD pass\n"
"   PAREN-CLOSE\n"
"   WORD zpass (FLAGS=$0)\n"
"   COMMA\n"
"   WORD passz (FLAGS=$0)\n"
"   COMMA\n"
"   WORD end5_3id (FLAGS=$0)\n"
"   WORD _1name (FLAGS=$0)\n"
"   KEYWORD end\n"
"   REAL 5.200000\n"
"   KEYWORD pass\n"
"   REAL 0.200000\n"
"   WORD pass3 (FLAGS=$0)\n"
"   REAL 0.140000\n"
"   WORD $ (FLAGS=$0)\n"
"   WORD % (FLAGS=$0)\n"
"   WORD pete (FLAGS=$0)\n";



/* test runner */
void _xte_lexer_test(void)
{
    
    /* create an engine */
    XTE *engine = xte_create(NULL);
    XTEAST *tree;
    const char *result;
    
    /* run test 1 */
    tree = _xte_lex(engine, "");
    result = _xte_ast_debug_text(tree, 0);
    /*printf("%s", result);*/
    if (strcmp(TEST_RESULT_1, result) != 0)
        printf("Failed xte_lexer_test(): 1\n");
    
    /* run test 2 */
    tree = _xte_lex(engine, "x\ry\nz\r\nlong\xC2\xAC\nline\nlong\xC2\xAC\rline\nlong\xC2\xAC\r\nline"
                    "\nlong \t\xC2\xAC\t \thidden\nline --comment here\nnew line\n"
                    ".nine.9text0.1text 0.1 3.14159 -7 + -8.1 * (-1.1) is = is not <> \xE2\x89\xA0"
                    "\xE2\x89\xA4 <= \xE2\x89\xA5 >= is in contains is not in"
                    " is within is not within there is a there is no there is apple "
                    "^*/div mod+& &&\"Quoted string literal\" and or not of in,functionaire,function"
                    " end exit global if then else next on pass repeat passes(pass)zpass,passz,"
                    "end5_3id _1name end 5.2pass.2pass3.14$%pete");
    result = _xte_ast_debug_text(tree, 0);
    /*printf("%s", result);*/
    if (strcmp(TEST_RESULT_2, result) != 0)
        printf("Failed xte_lexer_test(): 2\n");
}


#endif




