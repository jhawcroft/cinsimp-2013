/*
 
 xTalk Engine - Conversions
 xtalk_conv.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Utilities for data and type conversion
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


/*********
 Configuration
 */


/*
 *  MAX_FORMATTED_NUMBER_LENGTH
 *  ---------------------------------------------------------------------------------------------
 *  Maximum length of formatted number (number -> string conversion.)
 *
 *  ! As long as this remains relatively high, ie. above that required for the largest numbers
 *    representable within an XTEVariant, there should be no buffer overflow possibility with
 *    the sprintf() based implementation.
 */
#define MAX_FORMATTED_NUMBER_LENGTH 127


/*********
 Implementation
 */

/*
 *  _xte_format_number_r
 *  ---------------------------------------------------------------------------------------------
 *  Converts a real number to a string and returns the statically allocated result.
 *  Uses the supplied format string to perform the conversion.
 *
 *  The format string consists of a number of zeros and hashes (0, #) and the decimal point (.)
 *  Valid format strings include:
 *
 *    0
 *    0.######
 *    #.000
 *    0.00
 *    0.000##
 *    000
 *    00.00
 *
 *  If the format string is unaccepable or otherwise invalid, the conversion fails and the call
 *  returns NULL.
 */
char const* _xte_format_number_r(XTE *in_engine, double in_real, char const *in_format_string)
{
    char buffer[MAX_FORMATTED_NUMBER_LENGTH];
    
    /* verify and validate the format string */
    int whole_scan = 1;
    int whole_req = 0, frac_req = 0, frac_opt = 0;
    for (char const *ptr = in_format_string; *ptr != 0; ptr++)
    {
        if (whole_scan)
        {
            if ((*ptr == '#') && (whole_req == 0))
            {
                if ((*(ptr+1) != '.') && (*(ptr+1) != 0)) return NULL;
            }
            else if (*ptr == '0') whole_req++;
            else if ((*ptr == '.') && (*(ptr+1) != 0)) whole_scan = 0;
            else return NULL;
        }
        else
        {
            if ((*ptr == '0') && (frac_opt == 0)) frac_req++;
            else if (*ptr == '#') frac_opt++;
            else return NULL;
        }
    }
    if (frac_req + frac_opt + whole_req + 1 > MAX_FORMATTED_NUMBER_LENGTH) return NULL;
    
    /* convert the number to a string */
    char printf_fmt[MAX_FORMATTED_NUMBER_LENGTH];
    sprintf(printf_fmt, "%%0%d.%df", whole_req + (whole_scan ? 0 : 1) + frac_req + frac_opt, frac_req + frac_opt);
    int bytes = sprintf(buffer, printf_fmt, in_real);
    
    /* find the decimal point in the string */
    char *dec_pt = strchr(buffer, '.');
    
    /* scan backwards to strip consecutive zeros from the end,
     according to format string */
    int count = 0;
    for (char *end_of_buffer = buffer + bytes - 1;
         ((end_of_buffer > dec_pt) && (count < frac_opt));
         end_of_buffer--, count++)
    {
        if (*end_of_buffer == '0')
            *end_of_buffer = 0;
        else
            break;
    }
    
    /* check if the last character is the decimal point;
     if it is, remove it */
    long buffer_len = strlen(buffer);
    if ((buffer_len > 0) && (buffer[buffer_len-1] == '.'))
        buffer[buffer_len-1] = 0;
    
    /* copy the output to the result buffer */
    if (in_engine->f_result_cstr) free(in_engine->f_result_cstr);
    in_engine->f_result_cstr = _xte_clone_cstr(in_engine, buffer);
    return in_engine->f_result_cstr;
}




