/*
 
 xTalk Engine Test Unit
 xtalk_test.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Internal unit tests for the xTalk engine
 
 */

#include "xtalk_test_int.h"
#include "xtalk_internal.h"

#if XTALK_TESTS


/* run all tests; report results to stdout */
void xte_test(void)
{
    printf("xTalk: Running tests...\n");
    
    printf("Testing lexer...\n");
    _xte_lexer_test();
    
    printf("Testing expression parser...\n");
    _xte_parse_expression_test();
    
    printf("Testing command parser...\n");
    _xte_parse_command_test();
    
    printf("Testing The Message Protocol API...\n");
    //_xte_message_proto_test();
    
    printf("Testing memory allocation...\n");
    printf("MEMORY ALLOCATION TESTS DISABLED UNTIL FIXED\n");
    //_xte_memory_test(); // revisit, after expression parsing fixed.
    
    printf("Testing source formatting...\n");
    _xte_srcfmat_test();
    
    printf("Testing handler parser...\n");
    _xte_parse_handler_test();
    
    printf("xTalk: Tests completed.\n");
}


#endif




