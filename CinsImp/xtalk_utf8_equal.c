//
//  xtalk_utf8_equality.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 15/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "xtalk_internal.h"


/********
 UTF-8
 */

extern char const _XTE_UTF8_CHAR_TAIL_SIZES[256];
extern unsigned long const _XTE_UTF8_CP_OFFSETS_FOR_TAIL_SIZE[6];



static int _xte_utf8_to_ucs(unsigned int *io_dest, int in_dest_size, char const *in_source, long in_source_size)
{
    assert(in_source != NULL);
    assert(io_dest != NULL);
    assert(in_dest_size > 0);
    
    /* prepare to convert */
    unsigned int codepoint;
    char const *source_end = in_source + in_source_size;
    int tail_size;
    int dest_offset = 0;
    
    /* iterate over source bytes */
    while (dest_offset < in_dest_size) {
        /* get UTF-8 character tail size */
        tail_size = _XTE_UTF8_CHAR_TAIL_SIZES[(unsigned char)*in_source];
        
        /* check if we've reached the limit of valid characters in the input string */
        if ( ((in_source_size < 0) && (*in_source == 0)) ||
            ((in_source_size >= 0) && (in_source + tail_size >= source_end)) ) break;
        
        /* convert UTF-8 character to UCS-4 codepoint */
        codepoint = 0;
        switch (tail_size) {
                /* these fall through deliberately */
            case 3: codepoint += (unsigned char)*in_source++; codepoint <<= 6;
            case 2: codepoint += (unsigned char)*in_source++; codepoint <<= 6;
            case 1: codepoint += (unsigned char)*in_source++; codepoint <<= 6;
            case 0: codepoint += (unsigned char)*in_source++;
        }
        codepoint -= _XTE_UTF8_CP_OFFSETS_FOR_TAIL_SIZE[tail_size];
        
        /* write the codepoint to the output */
        io_dest[dest_offset++] = codepoint;
    }
    
    /* return the number of codepoints output */
    return dest_offset;
}



static char const* _xte_utf8_equal_next_char(char const *in_string)
{
    if ((!in_string) || (!*in_string)) return NULL;
    while (*(++in_string))
    {
        if ((*in_string & 0xC0) != 0x80) return in_string;
    }
    return NULL;
}




/********
 Case Folding
 */


#define MAX_CODE_HEX_BYTES 6
#define MAX_CODE_POINTS_MAPPING 3

extern char const *const _XTE_UTF8_CASE_DATA;


struct UTF8CaseFoldingRule
{
    int pattern_points;
    unsigned int pattern[MAX_CODE_POINTS_MAPPING];
    unsigned int uppercase_point;
};


struct UTF8CaseFoldingTable
{
    int rules_count;
    struct UTF8CaseFoldingRule *rules;
};


static struct UTF8CaseFoldingTable _g_xte_utf8_case_table = { 0, NULL };


static char const* _xte_utf8_next_line(char const *in_string)
{
    if ((!in_string) || (!*in_string)) return NULL;
    while ( *in_string && ((*in_string != 10) && (*in_string != 13)) )
        in_string++;
    while ( *in_string && ((*in_string == 10) || (*in_string == 13)) )
        in_string++;
    if (!(*in_string)) return NULL;
    return in_string;
}


static int _xte_utf8_read_hex_to_unicode(char const **io_hex)
{
    assert(io_hex);
    if (!(*io_hex)) return 0;
    
    const char *hex = *io_hex;
    int value = 0;
    while (*hex)
    {
        int half_value;
        switch (*hex)
        {
            case '0':
                half_value = 0; break;
            case '1':
                half_value = 1; break;
            case '2':
                half_value = 2; break;
            case '3':
                half_value = 3; break;
            case '4':
                half_value = 4; break;
            case '5':
                half_value = 5; break;
            case '6':
                half_value = 6; break;
            case '7':
                half_value = 7; break;
            case '8':
                half_value = 8; break;
            case '9':
                half_value = 9; break;
            case 'A':
            case 'a':
                half_value = 0xA; break;
            case 'B':
            case 'b':
                half_value = 0xB; break;
            case 'C':
            case 'c':
                half_value = 0xC; break;
            case 'D':
            case 'd':
                half_value = 0xD; break;
            case 'E':
            case 'e':
                half_value = 0xE; break;
            case 'F':
            case 'f':
                half_value = 0xF; break;
            default:
                *io_hex = hex;
                return value;
        }
        value <<= 4;
        value += half_value;
        hex++;
    }
    return value;
}


static int _xte_utf8_init_case_folding(void)
{
    /* don't init case folding if we've already done it */
    if (_g_xte_utf8_case_table.rules_count > 0) return 1;
    
    /* get the case folding Unicode data */
    char const *data = _XTE_UTF8_CASE_DATA;
    struct UTF8CaseFoldingRule rule;
    
    /* count the number of rules */
    _g_xte_utf8_case_table.rules_count = 0;
    while (data && (*data))
    {
        /* skip an uppercase code point */
        _xte_utf8_read_hex_to_unicode(&data);
        
        /* skip semi-colon, space */
        if (*data) data++;
        if (*data) data++;
        
        /* check status is Common or Full */
        if ((*data == 'C') || (*data == 'F'))
            _g_xte_utf8_case_table.rules_count++;
        
        /* find next line */
        data = _xte_utf8_next_line(data);
    }
    
    /* load the rules */
    data = _XTE_UTF8_CASE_DATA;
    _g_xte_utf8_case_table.rules = _xte_malloc_static(_g_xte_utf8_case_table.rules_count * sizeof(struct UTF8CaseFoldingRule));
    if (!_g_xte_utf8_case_table.rules) return 0;
    memset(_g_xte_utf8_case_table.rules, 0, _g_xte_utf8_case_table.rules_count * sizeof(struct UTF8CaseFoldingRule));
    int rule_index = 0;
    while (data && (*data))
    {
        /* read a uppercase code point */
        rule.uppercase_point = _xte_utf8_read_hex_to_unicode(&data);
        
        /* skip semi-colon, space */
        if (*data) data++;
        if (*data) data++;
        
        /* check status is Common or Full */
        if ((*data == 'C') || (*data == 'F'))
        {
            /* skip the status, semicolon and space */
            data += 3;
            
            /* read mapping code point(s) */
            rule.pattern_points = 0;
            while (rule.pattern_points < MAX_CODE_POINTS_MAPPING)
            {
                /* read mapping code point */
                rule.pattern[rule.pattern_points++] = _xte_utf8_read_hex_to_unicode(&data);
                
                /* look for space / semicolon */
                if ( (!(*data)) || (*data == ';') ) break;
                
                /* skip space */
                data++;
            }
            
            /* register rule */
            if (rule.pattern_points > 0)
                _g_xte_utf8_case_table.rules[rule_index++] = rule;
        }
        
        /* find next line */
        data = _xte_utf8_next_line(data);
    }
    
    /* return success */
    return 1;
}



/*
 returns the number of codepoints consumed during the transformation
 */
static int _xte_utf8_toupper(unsigned int *io_ucs)
{
    /* ASCII case transform (shortcut) */
    /*if (*io_ucs < 0x80)
     {
     *io_ucs = toupper(*io_ucs);
     return 1;
     }*/
    
    /* Unicode case transform */
    for (int rule_index = 0; rule_index < _g_xte_utf8_case_table.rules_count; rule_index++)
    {
        struct UTF8CaseFoldingRule *rule = _g_xte_utf8_case_table.rules + rule_index;
        if (memcmp(rule->pattern, io_ucs, sizeof(unsigned int) * rule->pattern_points) == 0)
        {
            memset(io_ucs, 0, MAX_CODE_POINTS_MAPPING * sizeof(unsigned int));
            io_ucs[0] = rule->uppercase_point;
            return rule->pattern_points;
        }
    }
    
    return 1;
}



/*
 this may be incorrectly (not) handling multi-codepoints - must check with one from UTF8 table data...
 */
static int _xte_utf8_streq(char const *const in_string1, long in_bytes1, char const *const in_string2, long in_bytes2)
{
    assert(in_string1 != NULL);
    assert(in_string2 != NULL);
    
    if (!_xte_utf8_init_case_folding()) return 0;
    
    char const *ptr1 = in_string1;
    char const *ptr2 = in_string2;
    char const *string1_end = (in_bytes1 >= 0 ? in_string1 + in_bytes1 : NULL);
    char const *string2_end = (in_bytes2 >= 0 ? in_string2 + in_bytes2 : NULL);
    
    while (ptr1 && ptr2 && (ptr1 != string1_end) && (*ptr1 != 0) && (ptr2 != string2_end) && (*ptr2 != 0))
    {
        unsigned int conversion_buffer_1[MAX_CODE_POINTS_MAPPING];
        unsigned int conversion_buffer_2[MAX_CODE_POINTS_MAPPING];
        
        int converted_chars_1 = _xte_utf8_to_ucs(conversion_buffer_1,
                                                 MAX_CODE_POINTS_MAPPING,
                                                 ptr1,
                                                 (string1_end ? string1_end-ptr1 : -1));
        int converted_chars_2 = _xte_utf8_to_ucs(conversion_buffer_2,
                                                 MAX_CODE_POINTS_MAPPING,
                                                 ptr2,
                                                 (string2_end ? string2_end-ptr2 : -1));
        if (converted_chars_1 != converted_chars_2) return 0;
        
        converted_chars_1 = _xte_utf8_toupper(conversion_buffer_1);
        converted_chars_2 = _xte_utf8_toupper(conversion_buffer_2);
        
        if (conversion_buffer_1[0] != conversion_buffer_2[0]) return 0;
        
        for (int i = 0; i < converted_chars_1; i++)
            ptr1 = _xte_utf8_equal_next_char(ptr1);
        for (int i = 0; i < converted_chars_2; i++)
            ptr2 = _xte_utf8_equal_next_char(ptr2);
    }
    
    if (string1_end && (*string1_end == 0)) string1_end = NULL;
    if (string2_end && (*string2_end == 0)) string2_end = NULL;
    
    return ( ((ptr1 == NULL) && (ptr2 == NULL)) ||
            ((ptr1 == string1_end) && (ptr2 == string2_end)) );
}



int xte_cstrings_equal(char const *in_string1, char const *in_string2)
{
    return _xte_utf8_streq(in_string1, strlen(in_string1), in_string2, strlen(in_string2));
}


int xte_cstrings_szd_equal(char const *in_string1, long in_bytes1, char const *in_string2, long in_bytes2)
{
    return _xte_utf8_streq(in_string1, in_bytes1, in_string2, in_bytes2);
}





