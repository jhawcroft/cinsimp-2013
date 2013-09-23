/*
 
 Stack File Format
 stack_utf8.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 UTF-8 string utilities
 
 (see header file for module description)
 
 */

#include "stack_int.h"

#include <string.h>
#include <ctype.h>

// UTF-8 case-insensitive string comparison

// will be used by find and eventually sort

// **TODO** needs to be fixed to work with UTF8, not just ASCII

// return 0 if same, or return a diff +1 or -1
int _stack_utf8_compare(const char *in_string_1, const char *in_string_2)
{
    long len1 = strlen(in_string_1), len2 = strlen(in_string_2);
    if (len1 != len2) return 1;
    for (long i = 0; i < len1; i++)
    {
        if (tolower(in_string_1[i]) != tolower(in_string_2[i])) return 1;
    }
    return 0;
}

