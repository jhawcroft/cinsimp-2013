/*
 
 xTalk Engine Utility
 xtalk_util.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Assorted internal utilities
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"



/*
 *  _xte_cstr_format_fill()
 *  ---------------------------------------------------------------------------------------------
 *  Fill a printf-style string format with values from a variable argument list.
 *
 *  Format specifiers must take the following form:
 *
 *      % <type>
 *
 *  Supported Types (<type>):
 *   Code   Type/output         Description
 *      %   %                   (a percent sign)
 *      s   char*               string
 *      d   int                 decimal
 *      f   double              floating-point
 *      x   int                 hexadecimal
 *
 *  Other features of printf, such as length, padding and capitalization are NOT
 *  supported by this implementation.
 *
 *  Used by: _engine.c (error and runtime exception handling)
 *           _ast.c (debugging output, test case validation)
 */

char* _xte_cstr_format_fill(const char *in_format, va_list in_list)
{
    /* processing variables */
    long length = strlen(in_format);
    char *output_buffer = NULL, *new_output_buffer;
    long output_size = 0;
    long normal_offset = 0;
    long normal_length;
    
    /* temporary variables for decoding of arguments */
    char *var_string;
    int var_int;
    double var_float;
    char var_buffer[1024];
    long var_buffer_size = 0;
    
    /* scan message format for format specifiers */
    for (long i = 0; i <= length; i++)
    {
        if ( ((in_format[i] == '%') && (in_format[i+1])) || (i == length) ||  (in_format[i] == '\n'))
        {
            /* found a format specifier or the end of the message format;
             output normal text up to this point */
            normal_length = i - normal_offset;
            if (normal_length > 0)
            {
                new_output_buffer = realloc(output_buffer, output_size + normal_length + 1);
                if (!new_output_buffer)
                {
                    if (output_buffer) free(output_buffer);
                    return NULL;
                }
                output_buffer = new_output_buffer;
                memcpy(output_buffer + output_size, in_format + normal_offset, normal_length);
                output_buffer[output_size + normal_length] = 0;
                output_size += normal_length;
            }
            if (i == length) break;
            
            /* output the next function argument formatted according to this format specifier */
            var_string = NULL;
            if (in_format[i] == '\n')
            {
                var_string = "\n";
            }
            else
            {
                switch (in_format[i+1])
                {
                    case 's':
                    {
                        var_string = va_arg(in_list, char*);
                        if (!var_string) var_string = "";
                        
                        //printf("%s", var_string);
                        break;
                    }
                    case 'd':
                        var_int = va_arg(in_list, int);
                        var_buffer_size = sprintf(var_buffer, "%d", var_int);
                        var_string = var_buffer;
                        //printf("%d", var_int);
                        break;
                    case 'f':
                        var_float = va_arg(in_list, double);
                        var_buffer_size = sprintf(var_buffer, "%f", var_float);
                        var_string = var_buffer;
                        //printf("%f", var_float);
                        break;
                    case 'x':
                        var_int = va_arg(in_list, int);
                        var_buffer_size = sprintf(var_buffer, "%X", var_int);
                        var_string = var_buffer;
                        break;
                    case '%':
                        var_string = "%";
                        //printf("%%");
                        break;
                }
            }
            
            /* copy the string version of the argument to the output buffer */
            if (var_string)
            {
                long var_string_length = strlen(var_string);
                new_output_buffer = realloc(output_buffer, output_size + var_string_length + 1);
                if (!new_output_buffer)
                {
                    if (output_buffer) free(output_buffer);
                    return NULL;
                }
                output_buffer = new_output_buffer;
                strcpy(output_buffer + output_size, var_string);
                output_size += var_string_length;
            }
            
            /* continue scanning */
            i++;
            normal_offset = i+1;
        }
    }

    return output_buffer;
}



/*
 *  _xte_first_word()
 *  ---------------------------------------------------------------------------------------------
 *  Return the first word of the supplied string up to and not including the first white space.
 *
 *  If there's a problem, the string starts with whitespace or the function is supplied with an
 *  empty input string, it returns an empty string.
 */

char const* _xte_first_word(XTE *in_engine, char const *in_string)
{
    assert(IS_XTE(in_engine));
    assert(in_string);
    
    /* identify the first whitespace offset */
    const char *c = in_string;
    int offset = 0;
    while (*c)
    {
        if (isspace(*c)) break;
        offset++;
        c++;
    }
    
    /* allocate a string large enough to hold the first word */
    if (in_engine->f_result_cstr) free(in_engine->f_result_cstr);
    char *result = in_engine->f_result_cstr = malloc(offset + 1);
    if (!result) return "";
    memcpy(result, in_string, offset);
    result[offset] = 0;
    return result;
}



/*
 *  _xte_clone_cstr()
 *  ---------------------------------------------------------------------------------------------
 *  Return a copy of the input string.
 *
 *  If a NULL pointer is supplied, an empty string is returned.
 *
 *  If there isn't enough memory to copy the string, returns NULL and follows XTE protocol.
 */

char* _xte_clone_cstr(XTE * const in_engine, char const *in_string)
{
    if (!in_string) in_string = "";
    long len = strlen(in_string);
    char *result = malloc(len + 1);
    if (!result) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    strcpy(result, in_string);
    return result;
}


/*
 *  _xte_compare_cstr()
 *  ---------------------------------------------------------------------------------------------
 *  Case-insensitive comparison of two strings for equality only.
 *
 *  Supports case-insensitive comparison of ONLY the ASCII-roman characters in the strings.
 *  Works with UTF-8 encoded string data.
 *
 *  Returns 0 for equality, or 1 otherwise (based on strcmp).
 */

int _xte_compare_cstr(char const *in_string_1, char const *in_string_2)
{
    assert(in_string_1 != NULL);
    assert(in_string_2 != NULL);
    
    long len1 = strlen(in_string_1), len2 = strlen(in_string_2);
    if (len1 != len2) return 1;
    for (long i = 0; i < len1; i++)
    {
        if (tolower(in_string_1[i]) != tolower(in_string_2[i])) return 1;
    }
    return 0;
}


/*
 *  _xte_itemize_cstr()
 *  ---------------------------------------------------------------------------------------------
 *  Splits a string into many substrings using the supplied delimiter string.
 *
 *  Performs a case-sensitive, binary search for the supplied delimiter.
 *  Works with UTF-8 encoded string data.
 *
 *  If you supply an empty (zero-length) delimiter, returns a single string for the entire source.
 *
 *  The results can be cleaned up by calling _xte_cstrs_free().
 */

void _xte_itemize_cstr(XTE * const in_engine, char const *in_string, char const *in_delimiter, char **out_items[], int *out_count)
{
    assert(in_engine != NULL);
    assert(in_string != NULL);
    assert(in_delimiter != NULL);
    assert(out_items != NULL);
    assert(out_count != NULL);
    
    /* get string lengths */
    long len = strlen(in_string);
    long del_len = strlen(in_delimiter);
    
    /* if the delimiter is empty, then just return a copy of the input string */
    if (del_len == 0)
    {
        char **items = *out_items = calloc(sizeof(char*), 1);
        if (!items) return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
        
        *out_count = 1;
        items[0] = _xte_clone_cstr(in_engine, in_string);
        return;
    }
    
    /* count the number of items in the input string */
    *out_count = 1;
    for (long i = 0; i + del_len <= len; i++)
    {
        /* find next item delimiter */
        if (memcmp(in_string + i, in_delimiter, del_len) == 0)
        {
            (*out_count)++;
            i = i + del_len - 1;
        }
    }
    
    /* allocate memory for each item ptr */
    char **items = *out_items = calloc(sizeof(char*), *out_count);
    if (!items) {
        *out_count = 0;
        return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
    }
    
    /* compute each item */
    long item_offset = 0, item_len = 0;
    int item_index = 0;
    for (long i = 0; i <= len; i++)
    {
        /* find next item delimiter */
        if ( (i == len) || ((i + del_len <= len) && (memcmp(in_string + i, in_delimiter, del_len) == 0)) )
        {
            /* get item length */
            item_len = i - item_offset;
            
            /* allocate memory for item */
            char *item = items[item_index++] = malloc(item_len + 1);
            if (!item)
            {
                *out_count = 0;
                return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
            }
            
            /* build item string */
            memcpy(item, in_string + item_offset, item_len);
            item[item_len] = 0;
            
            /* prepare for next item */
            item_offset = i + del_len;
            i = item_offset - 1;
            if (i == len) break;
        }
    }
}


/*
 *  _xte_cstrs_free()
 *  ---------------------------------------------------------------------------------------------
 *  Calls free() on each of the strings in an array of C-strings, and then on the array itself.
 */
void _xte_cstrs_free(char *in_strings[], int const in_count)
{
    for (int i = 0; i < in_count; i++)
        if (in_strings[i]) free(in_strings[i]);
    free(in_strings);
}



/*
 *  _xte_random()
 *  ---------------------------------------------------------------------------------------------
 *  Returns a pseudo-random number between 1 and <in_upper_limit>, or 2147483648, whichever is
 *  smaller.
 *
 *  Uses the standard library random generator.
 *
 *  Seeded by xte_create() in _engine.c.
 */

int _xte_random(int in_upper_limit)
{
    int const MAX_RANDOM_LIMIT = 2147483648;
    if (in_upper_limit == 0) in_upper_limit = MAX_RANDOM_LIMIT;
    return rand() % in_upper_limit + 1;
}



/*
 *  _xte_text_make_range()
 *  ---------------------------------------------------------------------------------------------
 *  Convenience function; creates an XTETextRange with the supplied arguments.
 *
 *  If the length is negative, it computes the range to have positive parameters.
 */

XTETextRange _xte_text_make_range(long in_offset, long in_length)
{
    assert(in_offset >= -1);
    XTETextRange result;
    if (in_offset < 0)
    {
        result.offset = -1;
        result.length = -1;
    }
    else if ((in_length < 0) && (in_offset >= 0))
    {
        result.offset = in_offset + in_length;
        result.length = in_length * -1;
    }
    else
    {
        result.offset = in_offset;
        result.length = in_length;
    }
    return result;
}


XTETextRange const XTE_TEXT_RANGE_ALL = {
    -1,
    -1
};





