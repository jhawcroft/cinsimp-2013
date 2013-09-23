/*
 
 C-String Items
 cstritems.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


static void _extend(char **io_items[], int *io_count)
{
    char **new_items = realloc(*io_items, sizeof(char*) * (*io_count + 1));
    if (!new_items) return;
    *io_items = new_items;
    new_items[*io_count] = NULL;
    *io_count += 1;
}


static void _itemize(char *in_string, char **out_items[], int *out_count)
{
    if (!in_string) in_string = "";
    long len = strlen(in_string);
    
    *out_items = NULL;
    *out_count = 0;
    long from = 0;
    char *item;
    long itemLen;
    
    for (long i = 0; i <= len; i++)
    {
        if ((in_string[i] == ',') || (in_string[i] == 0))
        {
            char **new_items = realloc(*out_items, sizeof(char*) * (*out_count + 1));
            if (!new_items) return;
            *out_items = new_items;
            
            itemLen = i - from;
            item = malloc(itemLen + 1);
            if (!item) return;
            
            item[itemLen] = 0;
            memcpy(item, in_string + from, itemLen);
            
            new_items[*out_count] = item;
            *out_count += 1;
            from = i+1;
        }
    }
    
    if ((*out_count == 1) && ((*out_items)[0][0] == 0))
    {
        free((*out_items)[0]);
        (*out_items)[0] = NULL;
        *out_count = 0;
    }
}


static char* _inplace_trim(char *in_string)
{
    long len = strlen(in_string);
    for (long i = 0; i < len; i++)
    {
        if (!isspace(in_string[i]))
        {
            memmove(in_string, in_string + i, len - i + 1);
            break;
        }
    }
    len = strlen(in_string);
    for (long i = len-1; i >= 0; i--)
    {
        if (!isspace(in_string[i]))
        {
            in_string[i+1] = 0;
            break;
        }
    }
    return in_string;
}


static int _streq(char *in_string1, char *in_string2)
{
    long len1 = strlen(in_string1), len2 = strlen(in_string2);
    if (len1 != len2) return 0;
    for (long i = 0; i < len1; i++)
    {
        if (tolower(in_string1[i]) != tolower(in_string2[i])) return 0;
    }
    return 1;
}


static void _items_cleanup(char **in_string, int in_count)
{
    for (int i = 0; i < in_count; i++)
        if (in_string[i]) free(in_string[i]);
    free(in_string);
}


static char* _cstr_clone(char *in_string)
{
    if (!in_string) in_string = NULL;
    long len = strlen(in_string);
    char *result = malloc(len+1);
    if (!result) return NULL;
    strcpy(result, in_string);
    return result;
}


static char* _items_join(char **in_items, int in_count)
{
    char *result = NULL, *temp;
    long result_len = 0;
    for (int i = 0; i < in_count; i++)
    {
        if (!in_items[i]) continue;
        _inplace_trim(in_items[i]);
        long item_len = strlen(in_items[i]);
        temp = realloc(result, result_len + item_len + 1 + 2);
        if (!temp) return result;
        result = temp;
        strcpy(result + result_len, in_items[i]);
        result_len += item_len;
        //if (i+1 == in_count) break;
        strcpy(result + result_len, ", ");
        result_len += 2;
    }
    if ((result) && (strcmp(result + result_len - 2, ", ") == 0))
        result[result_len-2] = 0;
    if (!result) result = _cstr_clone("");
    return result;
}





int cstr_has_item(const char *in_string, const char *in_item)
{
    char **items;
    int count, result = 0;
    _itemize((char*)in_string, &items, &count);
    for (int i = 0; i < count; i++)
    {
        if (_streq(_inplace_trim(items[i]), (char*)in_item))
        {
            result = 1;
            break;
        }
    }
    _items_cleanup(items, count);
    return result;
}


const char* cstr_item_add(const char *in_string, const char *in_item)
{
    char **items;
    int count, exists = 0;
    _itemize((char*)in_string, &items, &count);
    for (int i = 0; i < count; i++)
    {
        if (_streq(_inplace_trim(items[i]), (char*)in_item))
        {
            exists = 1;
            break;
        }
    }
    if (!exists)
    {
        _extend(&items, &count);
        items[count-1] = _cstr_clone((char*)in_item);
    }
    static char *result = NULL;
    if (result) free(result);
    result = _items_join(items, count);
    _items_cleanup(items, count);
    return result;
}


const char* cstr_item_remove(const char *in_string, const char *in_item)
{
    char **items;
    int count;
    _itemize((char*)in_string, &items, &count);
    for (int i = 0; i < count; i++)
    {
        if (_streq(_inplace_trim(items[i]), (char*)in_item))
        {
            if (items[i]) free(items[i]);
            items[i] = NULL;
            break;
        }
    }
    static char *result = NULL;
    if (result) free(result);
    result = _items_join(items, count);
    _items_cleanup(items, count);
    return result;
}


