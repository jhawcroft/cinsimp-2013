/*
 
 Stack File Format
 stack_util.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Utility functions
 
 (see header file for module description)
 
 */

#include "stack_int.h"
#include "xtalk_engine.h"


char* _stack_clone_cstr(char const *in_string)
{
    if (!in_string) in_string = "";
    long len = strlen(in_string);
    char *result;
    result = _stack_malloc(len + 1);
    if (!result) return app_out_of_memory_null();
    strcpy(result, in_string);
    return result;
}


/* where is this called from anyway?  do we use it anywhere other than Find() ? */

/* use the string capabilities of the xTalk engine instead here; it becomes a provider of string services
 for other parts of the application ***TODO *** */

#define WORDIZE_ALLOC_SIZE 50

void _stack_wordize(char *in_string, char **out_items[], long **out_offsets, int *out_count)
{
    assert(out_items != NULL);
    assert(out_count != NULL);
    
    if (!in_string) in_string = "";
    
    *out_items = NULL;
    *out_count = 0;
    if (out_offsets) *out_offsets = NULL;
    int alloc_words = 0;
    
    char const *word_begin = xte_cstring_index_word(in_string, 0);
    while (word_begin)
    {
        char const *word_end = xte_cstring_index_word_end(word_begin);
        
        if (*out_count + 1 > alloc_words)
        {
            char **new_words = _stack_realloc(*out_items, sizeof(char*) * (alloc_words + WORDIZE_ALLOC_SIZE));
            if (!new_words) return app_out_of_memory_void();
            *out_items = new_words;
            
            if (out_offsets)
            {
                long *new_offsets = _stack_realloc(*out_offsets, sizeof(long) * (alloc_words + WORDIZE_ALLOC_SIZE));
                if (!new_offsets) return app_out_of_memory_void();
                *out_offsets = new_offsets;
            }
            
            alloc_words += WORDIZE_ALLOC_SIZE;
        }
        
        char *the_word = _stack_malloc(word_end - word_begin + 1);
        if (!the_word) return app_out_of_memory_void();
        memcpy(the_word, word_begin, word_end - word_begin);
        the_word[word_end - word_begin] = 0;
        (*out_items)[*out_count] = the_word;
        
        if (out_offsets)
            (*out_offsets)[*out_count] = xte_cstring_chars_between(in_string, word_begin);
        
        (*out_count)++;
        
        word_begin = xte_cstring_index_word(word_begin, 1);
    }
}


void _stack_items_free(char **in_string, int in_count)
{
    if (!in_string) return;
    for (int i = 0; i < in_count; i++)
        if (in_string[i]) _stack_free(in_string[i]);
    _stack_free(in_string);
}


int _stack_str_same(char *in_string_1, char *in_string_2)
{
    long len1 = strlen(in_string_1);
    long len2 = strlen(in_string_2);
    if (len1 != len2) return 0;
    for (long i = 0; i < len1; i++)
    {
        if (tolower(in_string_1[i]) != tolower(in_string_2[i])) return 0;
    }
    return 1;
}


