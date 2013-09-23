/*
 
 xTalk Engine Internal Test API
 xtalk_test_int.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Internal API for testing the xTalk engine
 
 */

#include "xtalk_test.h"

#ifndef XTALK_TEST_INT_H
#define XTALK_TEST_INT_H
#if XTALK_TESTS

#include <stdio.h>

void _xte_lexer_test(void);
void _xte_parse_expression_test(void);
void _xte_parse_command_test(void);
void _xte_message_proto_test(void);
void _xte_memory_test(void);
void _xte_srcfmat_test(void);
void _xte_parse_handler_test(void);

struct XTETestParserCase
{
    char const *syntax;
    char const *result;
};




#endif
#endif
