/*
 
 Application Control Unit - Utilities
 acu_util.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 General utilities
 
 *************************************************************************************************
 */

#include "acu_int.h"


char* __acu_clone_cstr(char const *in_string, char const *in_file, long in_line)
{
    if (!in_string) in_string = "";
    long len = strlen(in_string);
    char *result = _acu_malloc(len + 1, _ACU_MALLOC_ZONE_GENERAL, in_file, in_line);
    if (!result) return NULL;
    strcpy(result, in_string);
    return result;
}

