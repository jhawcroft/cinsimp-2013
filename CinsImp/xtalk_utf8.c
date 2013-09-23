/*
 
 xTalk Engine UTF-8 Support
 xtalk_utf8.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Functions that are specifically related to intelligent handling of UTF-8 strings
 

 */

#include "xtalk_internal.h"


/** TODO ** review whether this functionality can be soft-linked via callback (not too many callbacks)
 or whether we continue to implement as a platform-specific file,ie. xtalk_utf8_mac.m */

int utf8_compare(const char *in_string1, const char *in_string2);
int utf8_contains(const char *in_string1, const char *in_string2);

int _xte_utf8_compare(const char *in_string1, const char *in_string2)
{
    return utf8_compare(in_string1, in_string2);
}


int _xte_utf8_contains(const char *in_string1, const char *in_string2)
{
    return utf8_contains(in_string1, in_string2);
}



/*
 *  _xte_utf8_strlen()
 *  ---------------------------------------------------------------------------------------------
 *  Counts the number of characters in a null-terminated UTF-8 string.
 */

long _xte_utf8_strlen(char const *in_string)
{
    if (!in_string) in_string = "";
    long count = 0;
    while (*(in_string++))
    {
        if ((in_string[0] & 0xC0) != 0x80) count++;
    }
    return count;
}


long xte_cstring_length(char const *in_string)
{
    return _xte_utf8_strlen(in_string);
}


/* these are implmemented in chunk.c : */
char const* _xte_utf8_index_word(char const *const in_string, long const in_word_offset);
char const* _xte_utf8_index_word_end(char const *const in_string);
long _xte_utf8_count_chars_between(char const *in_begin, char const *const in_end);


char const* xte_cstring_index_word(char const *in_string, long in_word_index)
{
    assert(in_string != NULL);
    assert(in_word_index >= 0);
    char const *result = _xte_utf8_index_word(in_string, in_word_index);
    assert(result != NULL);
    return (*result ? result : NULL);
}


char const* xte_cstring_index_word_end(char const *in_string)
{
    assert(in_string != NULL);
    return _xte_utf8_index_word_end(in_string);
}


long xte_cstring_chars_between(char const *in_begin, char const *const in_end)
{
    return _xte_utf8_count_chars_between(in_begin, in_end);
}


