/*
 
 xTalk Engine Tests: Handler Parsing: Loops
 xtalk_test_exprs.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Unit tests for handler parsing of loops
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


#if XTALK_TESTS



struct XTETestParserCase const _TEST_LOOPS_OK[] = {
    {
        "on mouseUp\n"
        "  repeat\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP forever\n"
        "      COMMAND fPTR=$0 (beep)\n"
        "      PARAMS:\n"
    },
    {
        "on mouseUp\n"
        "  repeat\n"
        "  end repeat\n"
        "  beep\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP forever\n"
        "   COMMAND fPTR=$0 (beep)\n"
        "   PARAMS:\n"
    },
    {
        "on mouseUp\n"
        "  repeat forever\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP forever\n"
        "      COMMAND fPTR=$0 (beep)\n"
        "      PARAMS:\n"
    },
    {
        "on mouseUp\n"
        "  repeat 2\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP number\n"
        "      EXPRESSION ()\n"
        "         INTEGER 2\n"
        "      COMMAND fPTR=$0 (beep)\n"
        "      PARAMS:\n"
    },
    {
        "on mouseUp\n"
        "  repeat until x\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP until\n"
        "      EXPRESSION ()\n"
        "         WORD x (FLAGS=$0)\n"
        "      COMMAND fPTR=$0 (beep)\n"
        "      PARAMS:\n"
    },
    {
        "on mouseUp\n"
        "  repeat while x -- comment\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP while\n"
        "      EXPRESSION ()\n"
        "         WORD x (FLAGS=$0)\n"
        "      COMMAND fPTR=$0 (beep)\n"
        "      PARAMS:\n"
    },
    {
        "on mouseUp\n"
        "  repeat for 3\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP number\n"
        "      EXPRESSION ()\n"
        "         INTEGER 3\n"
        "      COMMAND fPTR=$0 (beep)\n"
        "      PARAMS:\n"
    },
    {
        "on mouseUp\n"
        "  repeat for 42 times\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP number\n"
        "      EXPRESSION ()\n"
        "         INTEGER 42\n"
        "      COMMAND fPTR=$0 (beep)\n"
        "      PARAMS:\n"
    },
    {
        "on mouseUp\n"
        "  repeat with x = 1 to 10\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP count up\n"
        "      WORD x (FLAGS=$0)\n"
        "      EXPRESSION ()\n"
        "         INTEGER 1\n"
        "      EXPRESSION ()\n"
        "         INTEGER 10\n"
        "      COMMAND fPTR=$0 (beep)\n"
        "      PARAMS:\n"
    },
    {
        "on mouseUp\n"
        "  repeat with x = 5 down to x - 1\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP count down\n"
        "      WORD x (FLAGS=$0)\n"
        "      EXPRESSION ()\n"
        "         INTEGER 5\n"
        "      EXPRESSION ()\n"
        "         OPERATOR subtract\n"
        "            WORD x (FLAGS=$0)\n"
        "            INTEGER 1\n"
        "      COMMAND fPTR=$0 (beep)\n"
        "      PARAMS:\n"
    },
    {
        "on mouseUp\n"
        "  repeat 3 times\n"
        "    beep\n"
        "  end repeat -- this is a comment\n"
        "end mouseUp"
        ,
        "HANDLER C \"mouseUp\"\n"
        "   PARAM-NAMES\n"
        "   LOOP number\n"
        "      EXPRESSION ()\n"
        "         INTEGER 3\n"
        "      COMMAND fPTR=$0 (beep)\n"
        "      PARAMS:\n"
    },
    NULL
};


struct XTETestParserCase const _TEST_LOOPS_ERROR[] = {
    {
        "on mouseUp\n"
        "  repeat for\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected integer here."
    },
    {
        "on mouseUp\n"
        "  repeat forever with sausages\n"
        "  end repeat\n"
        "  beep\n"
        "end mouseUp"
        ,
        "Expected end of line after \"repeat forever\"."
    },
    {
        "on mouseUp\n"
        "  repeat until\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected true or false expression here."
    },
    {
        "on mouseUp\n"
        "  repeat while\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected true or false expression here."
    },
    {
        "on mouseUp\n"
        "  repeat with\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected counter variable after \"repeat with\"."
    },
    {
        "on mouseUp\n"
        "  repeat with ~x\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "\"~\" is not a valid variable name."
    },
    {
        "on mouseUp\n"
        "  repeat with end\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected counter variable here but found \"end\"."
    },
    {
        "on mouseUp\n"
        "  repeat with x\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected \"=\" after \"repeat with x\"."
    },
    {
        "on mouseUp\n"
        "  repeat with x !\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected \"=\" after \"repeat with x\"."
    },
    {
        "on mouseUp\n"
        "  repeat with x = \n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected integer but found end of line."
    },
    {
        "on mouseUp\n"
        "  repeat with x = 1\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected \"to\" or \"down to\" but found end of line."
    },
    {
        "on mouseUp\n"
        "  repeat with x = 1 down\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected \"down to\" but found end of line."
    },
    {
        "on mouseUp\n"
        "  repeat with x = 1 to\n"
        "    beep\n"
        "  end repeat\n"
        "end mouseUp"
        ,
        "Expected integer but found end of line."
    },
    {
        "on mouseUp\n"
        "  repeat 3 times\n"
        "    beep\n"
        "  end\n"
        "end mouseUp"
        ,
        "Expected \"end repeat\" but found end of line."
    },
    {
        "on mouseUp\n"
        "  repeat 3 times\n"
        "    beep\n"
        "  end robert\n"
        "end mouseUp"
        ,
        "Expected \"end repeat\" but found \"end robert\"."
    },
    {
        "on mouseUp\n"
        "  repeat 3 times\n"
        "    beep\n"
        "  end repeat end\n"
        "end mouseUp"
        ,
        "Expected end of line after \"end repeat\"."
    },
    
    NULL
};





#endif


