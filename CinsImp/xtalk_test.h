/*
 
 xTalk Engine Test API
 xtalk_test.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Public API for testing the xTalk engine
 
 */

#ifndef XTALK_TEST_H
#define XTALK_TEST_H
#ifdef DEBUG
/* defined if tests are compiled */
#define XTALK_TESTS 1
#endif

#if XTALK_TESTS

/* run all tests; report results to stdout */
void xte_test(void);

#endif
#endif
