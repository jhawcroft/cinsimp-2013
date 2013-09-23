/*
 
 xTalk Engine Chunk Unit
 xtalk_chunk.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Implements string chunk handling; characters, lines, words and items expressions
 
 */

#include "xtalk_internal.h"



/*********
 Configuration
 */

#define MAX_NESTED_CHUNKS 10



/*********
 Types
 */

typedef enum
{
    XTE_CHUNK_CHAR,
    XTE_CHUNK_WORD,
    XTE_CHUNK_LINE,
    XTE_CHUNK_ITEM,
} XTEChunkType;


typedef struct XTEChunkData XTEChunkData;

struct XTEChunkData
{
    XTEChunkType type;
    XTETextRange range;
    
    XTEVariant *container_ref;
    XTEChunkData *parent_chunk;
    XTEVariant *parent_chunk_ref;
    
    char *substring;
    XTETextRange range_bytes;
    XTETextRange range_chars;
    
    long write_prefix_insert;
    
    /*
    int offset;
    int length;
     */
};


typedef struct 
{
    XTEVariant *container_ref;
    XTETextRange range_bytes;
    XTETextRange range_chars;
    char *text;
} XTEChunkWrite;



/*********
 Chunk Specific UTF-8 Utilities
 */

char const* _xte_utf8_end(char const *in_begin)
{
    assert(in_begin != NULL);
    while (*(in_begin++))
    {
        ; /* do nothing; just iterate until we find the end of the string */
    }
    return in_begin;
}

/* specific to chunk handling - not a general method probably... 
 
 
 */
char const* _xte_utf8_index_char(char const *in_string, long const in_char_offset)
{
    assert(in_string != NULL);
    assert(in_char_offset >= 0);
    
    char const *ptr = in_string;
    long char_index = 0;
    
    while (*ptr)
    {
        if ((ptr[0] & 0xC0) != 0x80)
        {
            if (char_index == in_char_offset) return ptr;
            char_index++;
        }
        ptr++;
    }
    
    return ptr;
}


long _xte_utf8_count_chars_between(char const *in_begin, char const *const in_end)
{
    assert(in_begin != NULL);
    assert(in_end != NULL);
    assert(in_end >= in_begin);
    long count = 0;
    while ((*in_begin) && (in_begin != in_end))
    {
        if ((in_begin[0] & 0xC0) != 0x80) count++;
        in_begin++;
    }
    return count;
}


long _xte_utf8_count_bytes_in_range(char const *in_string, long in_start, long in_length)
{
    assert(in_string != NULL);
    assert(in_start >= 0);
    assert(in_length >= 0);
    
    char const *begin = _xte_utf8_index_char(in_string, in_start);
    char const *end = _xte_utf8_index_char(begin, in_length);
    
    return end-begin;
}


char const* _xte_utf8_index_word(char const *const in_string, long const in_word_offset)
{
    assert(in_string != NULL);
    assert(in_word_offset >= 0);
    
    /* setup */
    char const *ptr = in_string;
    
    /* skip leading whitespace until we find the first word */
    while (ptr && (isspace(*ptr)) && (*ptr != 0))
        ptr = _xte_utf8_index_char(ptr, 1);
    
    /* enumerate words */
    long word_index = 0;
    while ((word_index < in_word_offset) && (ptr) && (*ptr != 0))
    {
        /* scan until we find the end of the current word */
        while (ptr && (!isspace(*ptr)) && (*ptr != 0))
            ptr = _xte_utf8_index_char(ptr, 1);
        
        /* scan until we find the start of the next word */
        while (ptr && (isspace(*ptr)) && (*ptr != 0))
            ptr = _xte_utf8_index_char(ptr, 1);
        
        word_index++;
    }
    
    return ptr;
}


char const* _xte_utf8_index_word_end(char const *const in_string)
{
    assert(in_string != NULL);
    
    /* setup */
    char const *ptr = in_string;
    
    /* scan until we find the end of the current word */
    while (ptr && (!isspace(*ptr)) && (*ptr != 0))
        ptr = _xte_utf8_index_char(ptr, 1);
    
    return ptr;
}



char const* _xte_utf8_index_line(char const *const in_string, long const in_line_offset)
{
    assert(in_string != NULL);
    assert(in_line_offset >= 0);
    
    /* setup */
    char const *ptr = in_string;
    
    /* enumerate lines */
    long line_index = 0;
    while ((line_index < in_line_offset) && (ptr) && (*ptr != 0))
    {
        /* scan until we find the end of the current line */
        while (ptr && (*ptr != 10) && (*ptr != 13) && (*ptr != 0))
            ptr = _xte_utf8_index_char(ptr, 1);
        
        /* identify the start of the next line (CR, CRLF, LF) */
        if ((ptr[0] == 13) && (ptr[1] == 10)) ptr += 2;
        else ptr++;
        
        line_index++;
    }
    
    return ptr;
}


char const* _xte_utf8_index_line_end(char const *const in_string)
{
    assert(in_string != NULL);
    
    /* setup */
    char const *ptr = in_string;
    
    /* scan until we find the end of the current line */
    while (ptr && (*ptr != 10) && (*ptr != 13) && (*ptr != 0))
        ptr = _xte_utf8_index_char(ptr, 1);
    
    return ptr;
}


/* item delimiters are currently case-sensitive;
 (HC was case sensitive and didn't support more than 1 character anyway!) */
char const* _xte_utf8_index_item(char const *const in_string, char const *const in_delimiter, long const in_item_offset)
{
    assert(in_string != NULL);
    assert(in_item_offset >= 0);
    assert(in_delimiter != NULL);
    assert(in_delimiter[0] != 0);
    
    /* setup */
    char const *ptr = in_string;
    char const *ptr_end = ptr + strlen(in_string);
    long delim_len = strlen(in_delimiter);
    
    /* shortcuts */
    if (ptr_end - ptr < delim_len) {
        if (in_item_offset == 0) return ptr;
        else return ptr_end;
    }
    
    /* enumerate items */
    char const *ptr_limit = ptr_end - delim_len;
    long item_index = 0;
    while ((*ptr) && (ptr <= ptr_limit) && (item_index < in_item_offset))
    {
        if (memcmp(ptr, in_delimiter, delim_len) == 0)
        {
            item_index++;
            ptr += delim_len;
        }
        else
            ptr++;
    }
    
    if (item_index == in_item_offset) return ptr;
    else return ptr_end;
}


char const* _xte_utf8_index_item_end(char const *const in_string, char const *const in_delimiter)
{
    assert(in_string != NULL);
    assert(in_delimiter != NULL);
    assert(in_delimiter[0] != 0);
    
    char const *end = _xte_utf8_index_item(in_string, in_delimiter, 1);
    if ((end > in_string) && (end[0] != 0)) end -= strlen(in_delimiter);
    
    return end;
}


long _xte_chunk_utf8_word_count(char const *in_string)
{
    assert(in_string != NULL);
    long word_count = 0;
    in_string = _xte_utf8_index_word(in_string, 0);
    while (*in_string)
    {
        if (in_string[0] == 0) return word_count;
        word_count++;
        in_string = _xte_utf8_index_word(in_string, 1);
    }
    return word_count;
}


long _xte_chunk_utf8_item_count(char const *in_string, char const *const in_item_delim)
{
    assert(in_string != NULL);
    assert(in_item_delim != NULL);
    
    /* setup */
    char const *ptr = in_string;
    char const *ptr_end = ptr + strlen(in_string);
    long delim_len = strlen(in_item_delim);
    
    /* shortcuts */
    if (ptr_end - ptr < delim_len) return 0;
    if (in_string[0] == 0) return 0;
    
    /* enumerate items */
    char const *ptr_limit = ptr_end - delim_len;
    long item_count = 1;
    while ((*ptr) && (ptr <= ptr_limit))
    {
        if (memcmp(ptr, in_item_delim, delim_len) == 0)
        {
            item_count++;
            ptr += delim_len;
        }
        else
            ptr++;
    }
    
    return item_count;
}


long _xte_chunk_utf8_line_count(char const *in_string)
{
    assert(in_string != NULL);
    
    /* setup */
    char const *ptr = in_string;
    if (ptr[0] == 0) return 0;
    
    /* enumerate lines */
    long line_count = 1;
    while (*ptr != 0)
    {
        if ((ptr[0] == 13) && (ptr[1] == 10))
        {
            line_count++;
            ptr++;
        }
        else if ((ptr[0] == 13) || (ptr[0] == 10))
            line_count++;
        
        ptr++;
    }
    
    return line_count;
}




/*********
 Chunk Computation
 */

static char* _xte_chunk_substring(char const *const in_begin, char const *const in_end)
{
    long len = in_end - in_begin;
    char *result = malloc(len + 1);
    if (!result) return NULL;
    memcpy(result, in_begin, len);
    result[len] = 0;
    return result;
}


static long _xte_chunk_max(long in_1, long in_2)
{
    if (in_1 > in_2) return in_1;
    else return in_2;
}


static void _xte_chunk_compute(XTEChunkData *io_chunk, char const *in_parent_string, char const *in_item_delim)
{
    if (io_chunk->parent_chunk && (io_chunk->parent_chunk->range_chars.length == 0))
    {
        /* specified element doesn't exist;
         must add to prefix characters to create it */
        char const *empty = "";
        io_chunk->substring = _xte_chunk_substring(empty, empty);
        
        io_chunk->range_bytes = io_chunk->parent_chunk->range_bytes;
        io_chunk->range_chars = io_chunk->parent_chunk->range_chars;
        
        io_chunk->write_prefix_insert = _xte_chunk_max(io_chunk->range.offset, 0);
        return;
    }
    
    /* setup some variables */
    char const *parent_string;
    long parent_bytes;
    long parent_chars;
    
    if (!in_parent_string) parent_string = io_chunk->parent_chunk->substring;
    else parent_string = in_parent_string;
    parent_bytes = strlen(parent_string);
    parent_chars = _xte_utf8_strlen(parent_string);
    
    /* locate the start and end of the chunk range */
    char const *start;
    char const *end;
    if ((io_chunk->range.offset < 0) && (io_chunk->range.length < 0))
    {
        /* return the entire container */
        start = parent_string;
        end = _xte_utf8_end(parent_string);
    }
    else
    {
        switch (io_chunk->type)
        {
            case XTE_CHUNK_CHAR:
                start = _xte_utf8_index_char(parent_string, io_chunk->range.offset);
                end = _xte_utf8_index_char(start,  io_chunk->range.length);
                break;
            case XTE_CHUNK_WORD:
                start = _xte_utf8_index_word(parent_string, io_chunk->range.offset);
                if (io_chunk->range.length > 0)
                {
                    end = _xte_utf8_index_word(start, io_chunk->range.length - 1);
                    end = _xte_utf8_index_word_end(end);
                }
                else
                    end = _xte_utf8_index_word_end(start);
                break;
            case XTE_CHUNK_ITEM:
                start = _xte_utf8_index_item(parent_string, in_item_delim, io_chunk->range.offset);
                if (io_chunk->range.length > 0)
                {
                    end = _xte_utf8_index_item(start, in_item_delim, io_chunk->range.length - 1);
                    end = _xte_utf8_index_item_end(end, in_item_delim);
                }
                else
                    end = _xte_utf8_index_item_end(start, in_item_delim);
                break;
            case XTE_CHUNK_LINE:
                start = _xte_utf8_index_line(parent_string, io_chunk->range.offset);
                if (io_chunk->range.length > 0)
                {
                    end = _xte_utf8_index_line(start, io_chunk->range.length - 1);
                    end = _xte_utf8_index_line_end(end);
                }
                else
                    end = _xte_utf8_index_line_end(start);
                break;
            default: break;
        }
    }
    
    /* compute the chunk data */
    io_chunk->substring = _xte_chunk_substring(start, end);
    io_chunk->range_bytes.offset = start - parent_string +
    (io_chunk->parent_chunk ? io_chunk->parent_chunk->range_bytes.offset : 0);
    io_chunk->range_bytes.length = end - start;
    io_chunk->range_chars.offset = _xte_utf8_count_chars_between(parent_string, start) + // need to count chars from start of substring to start
    (io_chunk->parent_chunk ? io_chunk->parent_chunk->range_chars.offset : 0);
    io_chunk->range_chars.length = _xte_utf8_strlen(io_chunk->substring);
    io_chunk->write_prefix_insert = 0;
    
    /* count the number of io_chunk->type(s) in parent_string,
    if range.offset > than that count.... */
    long count_elements;
    switch (io_chunk->type)
    {
        case XTE_CHUNK_CHAR: count_elements = _xte_utf8_strlen(parent_string); break;
        case XTE_CHUNK_ITEM: count_elements = _xte_chunk_utf8_item_count(parent_string, in_item_delim); break;
        case XTE_CHUNK_LINE:
            count_elements = _xte_chunk_utf8_line_count(parent_string);
            if (parent_string[0] == 0) count_elements = 1;
            break;
        case XTE_CHUNK_WORD: count_elements = _xte_chunk_utf8_word_count(parent_string); break;
    }
    if (io_chunk->range.offset + 1 > count_elements)
    {
        /* specified element doesn't exist;
         must add to prefix characters to create it */
        
        io_chunk->write_prefix_insert = (io_chunk->range.offset + 1) - count_elements;
        
        //io_chunk->write_prefix_insert = io_chunk->range_chars.offset; // save
        
        io_chunk->range_bytes.offset = (io_chunk->parent_chunk ?
                                        io_chunk->parent_chunk->range_bytes.offset +
                                        io_chunk->parent_chunk->range_bytes.length :
                                        parent_bytes);
        io_chunk->range_bytes.length = 0;
        
        io_chunk->range_chars.offset = (io_chunk->parent_chunk ?
                                        io_chunk->parent_chunk->range_chars.offset +
                                        io_chunk->parent_chunk->range_chars.length :
                                        parent_chars);
        io_chunk->range_chars.length = 0;
        
        //io_chunk->write_prefix_insert -= io_chunk->range_chars.offset;
        
    }
}


static void _xte_chunk_delete(XTEChunkData *in_chunk)
{
    if (in_chunk->substring) free(in_chunk->substring);
    free(in_chunk);
}


/* call this to get started with a non-chunk container */
XTEChunkData* _xte_chunk_new_with_string(void *in_container_ref, char const *in_container_string,
                                         XTEChunkType in_type, XTETextRange in_script_range, char const *in_item_delim)
{
    struct XTEChunkData *chunk = calloc(1, sizeof(struct XTEChunkData));
    if (!chunk) return NULL;
    
    chunk->container_ref = in_container_ref;
    chunk->parent_chunk = NULL;//in_parent_chunk;
    chunk->type = in_type;
    chunk->range = in_script_range;
    
    _xte_chunk_compute(chunk, in_container_string, in_item_delim);
    
    return chunk;
}


/* call this to get started with an existing chunk; this becomes a subchunk */
XTEChunkData* _xte_chunk_new_with_chunk(struct XTEChunkData *in_parent_chunk,
                                        XTEChunkType in_type, XTETextRange in_script_range, char const *in_item_delim)
{
    struct XTEChunkData *chunk = calloc(1, sizeof(struct XTEChunkData));
    if (!chunk) return NULL;
    
    chunk->container_ref = NULL;
    chunk->parent_chunk = in_parent_chunk;
    chunk->type = in_type;
    chunk->range = in_script_range;
    
    _xte_chunk_compute(chunk, NULL, in_item_delim);
    
    return chunk;
}


static long _xte_chunk_element_count(XTEChunkData *in_chunk, char const *in_item_delim)
{
    switch (in_chunk->type)
    {
        case XTE_CHUNK_CHAR: return _xte_utf8_strlen(in_chunk->substring);
        case XTE_CHUNK_WORD: return _xte_chunk_utf8_word_count(in_chunk->substring);
        case XTE_CHUNK_ITEM: return _xte_chunk_utf8_item_count(in_chunk->substring, in_item_delim);
        case XTE_CHUNK_LINE: return _xte_chunk_utf8_line_count(in_chunk->substring);
    }
}


int _xte_chunk_prepare_write(XTE *in_engine, XTEChunkData *in_chunk, char const *in_string, char const *in_item_delim, XTEChunkWrite *out_write)
{
    /* figure out how big our entire write prefix will be;
     and obtain the original container we will modify */
    long item_delim_bytes = strlen(in_item_delim);
    XTEChunkData *the_chunk = in_chunk;
    int chunk_count = 0;
    XTEChunkData *chunk_list[MAX_NESTED_CHUNKS];
    long insert_prefix_size = 0;
    void *container_ref = NULL;
    while (the_chunk)
    {
        chunk_list[chunk_count++] = the_chunk;
        
        if (the_chunk->container_ref)
        {
            container_ref = the_chunk->container_ref;
        }
        
        if (the_chunk->type == XTE_CHUNK_ITEM)
            insert_prefix_size += the_chunk->write_prefix_insert * item_delim_bytes;
        else
            insert_prefix_size += the_chunk->write_prefix_insert;
        
        the_chunk = the_chunk->parent_chunk;
    }
    if (chunk_count > MAX_NESTED_CHUNKS)
        return _xte_panic_int(in_engine, XTE_ERROR_INTERNAL, "Exceeded maximum of %d nested chunk expressions.", MAX_NESTED_CHUNKS);
    
    
    /* allocate a text buffer to hold our text to insert,
     including the write prefix (if any) */
    char *the_text = malloc(strlen(in_string) + insert_prefix_size + 1);
    if (!the_text) return _xte_panic_int(in_engine, XTE_ERROR_MEMORY, "Out of memory writing chunk expression.");
    
    
    /* compute an insert prefix */
    int insert_prefix_offset = 0;
    for (int c = chunk_count-1; c >= 0; c--)
    {
        /* does the chunk have a prefix? */
        XTEChunkData *the_chunk = chunk_list[c];
        if (the_chunk->write_prefix_insert == 0) continue;
        
        /* determine the string to use for this prefix */
        char const *prefix_str;
        long prefix_len;
        switch (the_chunk->type)
        {
            case XTE_CHUNK_CHAR: prefix_str = " "; prefix_len = 1; break;
            case XTE_CHUNK_ITEM: prefix_str = in_item_delim; prefix_len = item_delim_bytes; break;
            case XTE_CHUNK_WORD: prefix_str = " "; prefix_len = 1; break;
            case XTE_CHUNK_LINE: prefix_str = "\n"; prefix_len = 1; break;
            default: break;
        }
        
        /* append the prefix the required number of times */
        for (int i = 0; i < the_chunk->write_prefix_insert; i++)
        {
            strcpy(the_text + insert_prefix_offset, prefix_str);
            insert_prefix_offset += prefix_len;
        }
    }
    
    
    /* copy the text to the text buffer */
    strcpy(the_text + insert_prefix_offset, in_string);
    
    
    /* prepare the write details */
    out_write->container_ref = container_ref;
    out_write->range_bytes = in_chunk->range_bytes;
    out_write->range_chars = in_chunk->range_chars;
    out_write->text = the_text;
    
    return XTE_TRUE;
}




/*********
 Engine-Chunk Implementation Glue
 */

extern XTEVariant *track_variant;// debugging

static void _xte_chunk_ref_dealloc(XTEVariant *in_variant, const char *in_type, XTEChunkData *io_data)
{
    if (io_data->container_ref) track_variant = NULL;//debug
    
    xte_variant_release(io_data->parent_chunk_ref);
    xte_variant_release(io_data->container_ref);
    _xte_chunk_delete(io_data);
}




static XTEVariant* _xte_chunk_ref_build(XTE *in_engine, XTEVariant *in_owner, XTEVariant *in_params[], int in_param_count, XTEChunkType in_type)
{
    /* get value of owner as string
     (without resolving the owner if it's a reference) */
    XTEVariant *value = xte_variant_value(in_engine, in_owner);
    if (!xte_variant_convert(in_engine, value, XTE_TYPE_STRING))
    {
        xte_variant_release(value);
        xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
        return NULL;
    }
    
    /* decode chunk parameters */
    int the_offset, the_length;
    if (in_param_count == 0)
    {
        the_offset = -1;
        the_length = -1; /* get the entire container; count mechanism will work out the details later */
    }
    else if (in_param_count == 1)
    {
        the_offset = xte_variant_as_int(in_params[0]) - 1;
        the_length = 1;
    }
    else if (in_param_count == 2)
    {
        the_offset = xte_variant_as_int(in_params[0]) - 1;
        the_length = xte_variant_as_int(in_params[1]) - xte_variant_as_int(in_params[0]) + 1;
    }
    
    /* build a chunk with parameters around owner object */
    XTEChunkData *chunk;
    if (strcmp(xte_variant_ref_type(in_owner),"chunk") == 0)
    {
        
        chunk = _xte_chunk_new_with_chunk(xte_variant_ref_ident(in_owner),
                                          in_type,
                                          _xte_text_make_range(the_offset, the_length),
                                          in_engine->item_delimiter);
        chunk->parent_chunk_ref = in_owner;
        xte_variant_retain(in_owner);
    }
    else
    {
        
        track_variant = in_owner; // debugging
        xte_variant_retain(in_owner);
        chunk = _xte_chunk_new_with_string(in_owner,
                                           xte_variant_as_cstring(value),
                                           in_type,
                                           _xte_text_make_range(the_offset, the_length),
                                           in_engine->item_delimiter);
        chunk->parent_chunk_ref = NULL;
    }
    xte_variant_release(value);
    
    /* create the chunk object */
    return xte_object_ref(in_engine, "chunk", chunk, (XTEObjRefDeallocator)&_xte_chunk_ref_dealloc);
}


static XTEVariant* _xte_chunk_read(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    /* get chunk data */
    if (strcmp(xte_variant_ref_type(in_owner), "chunk") != 0)
    {
        xte_callback_error(in_engine, "Expected chunk expression.", NULL, NULL, NULL);
        return NULL;
    }
    XTEChunkData *data = xte_variant_ref_ident(in_owner);
    
    /* return the chunk value */
    return xte_string_create_with_cstring(in_engine, data->substring);
}


static void _xte_chunk_write(XTE *in_engine, void *in_context, XTEVariant *in_owner,
                             XTEVariant *in_value, XTETextRange in_text_range, XTEPutMode in_mode)
{
    /* check provided range is not specific;
     attempting to write to a specific character range of a chunk expression is not implemented
     and should be unnecessary; we spit out an internal error if it occurs. */
    if ((in_text_range.offset != -1) || (in_text_range.length != -1))
        return _xte_panic_void(in_engine, XTE_ERROR_UNIMPLEMENTED,
                               "Cannot write to a range of a chunk expression in this version of the xTalk engine.");
    
    /* get chunk */
    if (strcmp(xte_variant_ref_type(in_owner), "chunk") != 0)
    {
        xte_callback_error(in_engine, "Expected chunk expression.", NULL, NULL, NULL);
        return;
    }
    XTEChunkData *chunk = xte_variant_ref_ident(in_owner);
    
    /* convert input value to string */
    if (!xte_variant_convert(in_engine, in_value, XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
        return;
    }
    
    /* prepare to write */
    XTEChunkWrite write_details;
    _xte_chunk_prepare_write(in_engine, chunk, xte_variant_as_cstring(in_value), in_engine->item_delimiter, &write_details);
    
    /* write! */
    XTEVariant *str_value = xte_string_create_with_cstring(in_engine, write_details.text);
    xte_container_write(in_engine, write_details.container_ref, str_value, write_details.range_chars, in_mode);
    
    /* cleanup */
    xte_variant_release(str_value);
    free(write_details.text);
}


static XTEVariant* _xte_chunk_count(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    /* get chunk data */
    if (strcmp(xte_variant_ref_type(in_owner), "chunk") != 0)
    {
        xte_callback_error(in_engine, "Expected chunk expression.", NULL, NULL, NULL);
        return NULL;
    }
    XTEChunkData *chunk = xte_variant_ref_ident(in_owner);
    
    /* return the chunk length */
    return xte_integer_create(in_engine, (int)_xte_chunk_element_count(chunk, in_engine->item_delimiter));
}


static struct XTEPropertyDef _chunk_props[] = {
    {"number", 0, "integer", 0, &_xte_chunk_count, NULL},
    NULL,
};


struct XTEClassDef _xte_builtin_class_chunk = {
    "chunk",
    0,
    sizeof(_chunk_props) / sizeof(struct XTEPropertyDef),
    _chunk_props,
    &_xte_chunk_read,
    &_xte_chunk_write,
};


XTEVariant* _xte_chunk_chars(XTE *in_engine, void *in_context, XTEVariant *in_owner, XTEVariant *in_params[], int in_param_count)
{
    return _xte_chunk_ref_build(in_engine, in_owner, in_params, in_param_count, XTE_CHUNK_CHAR);
}


XTEVariant* _xte_chunk_words(XTE *in_engine, void *in_context, XTEVariant *in_owner, XTEVariant *in_params[], int in_param_count)
{
    return _xte_chunk_ref_build(in_engine, in_owner, in_params, in_param_count, XTE_CHUNK_WORD);
}


XTEVariant* _xte_chunk_lines(XTE *in_engine, void *in_context, XTEVariant *in_owner, XTEVariant *in_params[], int in_param_count)
{
    return _xte_chunk_ref_build(in_engine, in_owner, in_params, in_param_count, XTE_CHUNK_LINE);
}


XTEVariant* _xte_chunk_items(XTE *in_engine, void *in_context, XTEVariant *in_owner, XTEVariant *in_params[], int in_param_count)
{
    return _xte_chunk_ref_build(in_engine, in_owner, in_params, in_param_count, XTE_CHUNK_ITEM);
}


struct XTEElementDef _xte_builtin_chunk_elements[] = {
    { "characters", "character", "string", 0, &_xte_chunk_chars, NULL, &_xte_chunk_count },
    { "words", "word", "string", 0, &_xte_chunk_words, NULL, &_xte_chunk_count },
    { "lines", "line", "string", 0, &_xte_chunk_lines, NULL, &_xte_chunk_count },
    { "items", "item", "string", 0, &_xte_chunk_items, NULL, &_xte_chunk_count },
    NULL,
};


/*
 FOR debugging only:
 void check_chunks(XTEVariant *in_variant)
 {
 if (in_variant && (in_variant->type == XTE_TYPE_OBJECT) && (strcmp(in_variant->value.ref.type->name, "chunk")==0))
 {
 printf("CHUNK\n");
 struct XTEChunkData *data = xte_variant_ref_ident(in_variant);
 check_chunks(data->container_ref);
 }
 }
 */

