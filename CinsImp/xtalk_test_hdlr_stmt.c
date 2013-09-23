/*
 
 xTalk Engine Tests: Handler Parsing: Statements
 xtalk_test_exprs.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Unit tests for handler parsing of statements
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


#if XTALK_TESTS



struct XTETestParserCase const _TEST_STMTS_ERROR[] = {
    {
        "on mouseUp\n"
        "  global -- missing variable list\n"
        "end mouseUp"
        ,
        "Expected global variable name after \"global\"."
    },
    {
        "on mouseUp\n"
        "  global 5\n"
        "end mouseUp"
        ,
        "Expected global variable here but found \"5\"."
    },
    {
        "on mouseUp\n"
        "  global ~\n"
        "end mouseUp"
        ,
        "\"~\" is not a valid variable name."
    },
    {
        "on mouseUp\n"
        "  global a b\n"
        "end mouseUp"
        ,
        "Expected end of line but found \"b\"."
    },
    {
        "on mouseUp\n"
        "  global a, \n"
        "end mouseUp"
        ,
        "Expected another global variable name after \",\"."
    },
    {
        "on mouseUp\n"
        "  exit\n"
        "end mouseUp"
        ,
        "Expected \"mouseUp\" after \"exit\"."
    },
    {
        "on mouseUp\n"
        "  repeat 3 times\n"
        "    if name = \"Josh\" then\n"
        "      exit\n"
        "    end if\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected \"repeat\" after \"exit\"."
    },
    {
        "on mouseUp\n"
        "  repeat 3 times\n"
        "    if name = \"Josh\" then\n"
        "      exit repeat i\n"
        "    end if\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected end of line but found i."
    },
    {
        "on mouseUp\n"
        "  exit repeat\n"
        "end mouseUp"
        ,
        "Found \"exit repeat\" outside a repeat loop."
    },
    {
        "on mouseUp\n"
        "  exit to\n"
        "end mouseUp"
        ,
        "Expected \"user\" after \"exit to\"."
    },
    {
        "on mouseUp\n"
        "  exit to HyperCard\n"
        "end mouseUp"
        ,
        "Expected \"exit to user\" but found HyperCard."
    },
    {
        "on mouseUp\n"
        "  exit mouseDown\n"
        "end mouseUp"
        ,
        "Expected \"exit mouseUp\" but found mouseDown."
    },
    {
        "on mouseUp\n"
        "  pass\n"
        "end mouseUp"
        ,
        "Expected \"mouseUp\" after \"pass\"."
    },
    {
        "on mouseUp\n"
        "  pass mouseDown\n"
        "end mouseUp"
        ,
        "Expected \"pass mouseUp\" here."
    },
    {
        "on mouseUp\n"
        "  pass mouseUp to CinsImp\n"
        "end mouseUp"
        ,
        "Expected end of line after \"pass mouseUp\"."
    },
    {
        "on mouseUp\n"
        "  next\n"
        "end mouseUp"
        ,
        "Expected \"repeat\" after \"next\"."
    },
    {
        "on mouseUp\n"
        "  next iteration\n"
        "end mouseUp"
        ,
        "Expected \"next repeat\" here."
    },
    {
        "on mouseUp\n"
        "  repeat forever\n"
        "    next repeat i\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected end of line after \"next repeat\"."
    },
    {
        "on mouseUp\n"
        "  repeat forever\n"
        "    beep\n"
        "  end repeat\n"
        "  next repeat\n"
        "end mouseUp"
        ,
        "Found \"next repeat\" outside a repeat loop."
    },
    NULL
};


struct XTETestParserCase const _TEST_STMTS_OK[] = {
    {
        "on mouseUp\n"
        "  global a, b\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   GLOBAL-DELC\n"
        "      WORD a (FLAGS=$0)\n"
        "      WORD b (FLAGS=$0)\n"
    },
    {
        "on mouseUp\n"
        "  global a\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   GLOBAL-DELC\n"
        "      WORD a (FLAGS=$0)\n"
    },
    {
        "on mouseUp\n"
        "  exit mouseUp\n"
        "  exit to user\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   EXIT handler (return)\n"
        "   EXIT event\n"
    },
    {
        "on mouseUp\n"
        "  repeat forever\n"
        "    exit repeat\n"
        "    if true then exit repeat\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP forever\n"
        "      EXIT loop\n"
        "      CONDITION\n"
        "         EXPRESSION ()\n"
        "            CONSTANT fPTR=$0 (true)\n"
        "         LIST ()\n"
        "            EXIT loop\n"
    },
    {
        "on mouseUp\n"
        "  repeat forever\n"
        "    next repeat\n"
        "    if true then next repeat\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP forever\n"
        "      EXIT iteration\n"
        "      CONDITION\n"
        "         EXPRESSION ()\n"
        "            CONSTANT fPTR=$0 (true)\n"
        "         LIST ()\n"
        "            EXIT iteration\n"
    },
    {
        "on mouseUp\n"
        "  pass mouseUp -- with a comment\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   EXIT handler (pass)\n"
    },
    {
        "on mouseUp\n"
        "  return\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   EXIT handler (return)\n"
    },
    {
        "on mouseUp\n"
        "  return x * 2\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   EXIT handler (return)\n"
        "      EXPRESSION ()\n"
        "         OPERATOR multiply\n"
        "            WORD x (FLAGS=$0)\n"
        "            INTEGER 2\n"
    },
    NULL
};



#endif

