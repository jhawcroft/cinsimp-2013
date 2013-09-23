/*
 
 Application Control Unit - xTalk File I/O
 acu_xtalk_fileio.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Text file I/O capability of the CinsTalk language; implements the underlying mechanism behind
 open file, read, write and close file commands.
 
 *************************************************************************************************
 */

#include "acu_int.h"


#define RW_BUFFER_SIZE 4096



/******************
 Utilities
 */

static ACUxTalkOpenFile* _lookup_xtalk_file(StackMgrStack *in_stack, char const *in_filepath)
{
    for (int i = 0; i < ACU_LIMIT_MAX_OPEN_FILES; i++)
    {
        if (in_stack->open_files[i].pathname && (strcmp(in_stack->open_files[i].pathname, in_filepath) == 0))
            return &(in_stack->open_files[i]);
    }
    return NULL;
}



/******************
 Internal API
 */

void _acu_xt_fio_open(StackMgrStack *in_stack, char const *in_filepath)
{
    ACUxTalkOpenFile *existing = _lookup_xtalk_file(in_stack, in_filepath);
    if (existing)
    {
        xte_set_result(in_stack->xtalk, xte_string_create_with_cstring(in_stack->xtalk, "File is already open."));
        return;
    }
    
    existing = NULL;
    for (int i = 0; i < ACU_LIMIT_MAX_OPEN_FILES; i++)
    {
        if (!in_stack->open_files[i].pathname)
        {
            existing = &(in_stack->open_files[i]);
            break;
        }
    }
    if (!existing)
    {
        xte_set_result(in_stack->xtalk, xte_string_create_with_cstring(in_stack->xtalk, "Can't open any more files."));
        return;
    }
    
    existing->pathname = _acu_clone_cstr(in_filepath);
    if (!existing->pathname)
    {
        _acu_raise_error(ACU_ERROR_MEMORY);
        return;
    }
    
    // ***TODO*** if the file cannot be written, we need to handle by putting something in "the result"
    // on any attempt to write, but make sure we can still open existing files
    
    /* try to open an existing file to check if it can be opened */
    FILE *fp = fopen(in_filepath, "r+"); /* open for reading/writing; but don't truncate */
    if (!fp) fp = fopen(in_filepath, "w+"); /* create if it doesn't exist */
    
    /* check if we opened the file */
    if (!fp)
    {
        if (existing->pathname) free(existing->pathname);
        existing->pathname = NULL;
        xte_set_result(in_stack->xtalk, xte_string_create_with_cstring(in_stack->xtalk, "Can't create that file."));
        return;
    }
    
    /* store the file pointer */
    existing->fp = fp;
}


void _acu_xt_fio_close(StackMgrStack *in_stack, char const *in_filepath)
{
    ACUxTalkOpenFile *existing = _lookup_xtalk_file(in_stack, in_filepath);
    if (!existing)
    {
        xte_set_result(in_stack->xtalk, xte_string_create_with_cstring(in_stack->xtalk, "File is not open."));
        return;
    }
    
    /* close and reset the file handle */
    if (existing->fp) fclose(existing->fp);
    existing->fp = NULL;
    if (existing->pathname) free(existing->pathname);
    existing->pathname = NULL;
}


void _acu_close_all_xtalk_files(StackMgrStack *in_stack)
{
    for (int i = 0; i < ACU_LIMIT_MAX_OPEN_FILES; i++)
    {
        if (in_stack->open_files[i].pathname)
            _acu_xt_fio_close(in_stack, in_stack->open_files[i].pathname);
    }
}


static void _scan_to_char(FILE *in_fp, long in_char_offset)
{
    char buffer[RW_BUFFER_SIZE];
    long bytes;
    
    long char_offset = -1;
    while (!feof(in_fp))
    {
        if (char_offset == in_char_offset) return;
        
        long byte_offset = ftell(in_fp);
        bytes = fread(buffer, 1, RW_BUFFER_SIZE, in_fp);
        
        for (long i = 0; i < bytes; i++)
        {
            if ((buffer[i] & 0xC0) != 0x80)
            {
                char_offset++;
                if (char_offset == in_char_offset)
                {
                    fseek(in_fp, byte_offset + i, SEEK_SET);
                    return;
                }
            }
        }
    }
    
    fseek(in_fp, 0, SEEK_END);
}


static const int _ACU_OFFSETS_FROM_UTF8[6] = {
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

#define isutf(c) (((c)&0xC0)!=0x80)

unsigned int _recompose_unichar(char *s)
{
    unsigned int ch = 0;
    int sz = 0;
    int i = 0;
    
    do {
        ch <<= 6;
        ch += (unsigned char)s[i++];
        sz++;
    } while (s[i] && !isutf(s[i]));
    ch -= _ACU_OFFSETS_FROM_UTF8[sz-1];
    
    return ch;
}


static void _scan_to_unichar(FILE *in_fp, int in_unichar)
{
    char buffer[RW_BUFFER_SIZE];
    long bytes, limit;
    
    while (!feof(in_fp))
    {
        long byte_offset = ftell(in_fp);
        limit = fread(buffer, 1, RW_BUFFER_SIZE, in_fp);
        
        /* set the limit so we always see whole characters */
        if (limit == RW_BUFFER_SIZE)
        {
            fseek(in_fp, -8, SEEK_CUR);
            bytes = limit - 8;
        }
        else bytes = limit;
        
        for (long i = 0; i < bytes; i++)
        {
            if ((buffer[i] & 0xC0) != 0x80)
            {
                /* recompose the unicode character at this offset */
                unsigned int ch = _recompose_unichar(buffer + i);
                
                /* compare to in_unichar */
                if (ch == in_unichar)
                {
                    fseek(in_fp, byte_offset + i, SEEK_SET);
                    return;
                }
            }
        }
    }
    
    fseek(in_fp, 0, SEEK_END);
}


void _acu_xt_fio_read(StackMgrStack *in_stack, char const *in_filepath, long *in_char_begin, char *in_char_end, long *in_char_count)
{
    ACUxTalkOpenFile *existing = _lookup_xtalk_file(in_stack, in_filepath);
    if ((!existing) || (!existing->fp))
    {
        xte_callback_error(in_stack->xtalk, "File not open.", NULL, NULL, NULL);
        return;
    }
    
    /* scan to the specified location (if provided) */
    if (in_char_begin)
    {
        fseek(existing->fp, 0, SEEK_SET);
        _scan_to_char(existing->fp, *in_char_begin);
    }
    
    /* save the starting position */
    long saved_offset = ftell(existing->fp);
    
    /* find the end of the next read */
    long end_of_read = saved_offset;
    if (in_char_count)
        _scan_to_char(existing->fp, *in_char_count);
    else if (in_char_end)
    {
        _scan_to_unichar(existing->fp, _recompose_unichar(in_char_end));
    }
    else
        fseek(existing->fp, 0, SEEK_END);
    end_of_read = ftell(existing->fp);
    long bytes_to_read = end_of_read - saved_offset;
    
    /* shortcut if there's nothing to read */
    if (bytes_to_read < 1)
    {
        XTEVariant *it_value = xte_string_create_with_cstring(in_stack->xtalk, "");
        xte_set_global(in_stack->xtalk, "it", it_value);
        xte_variant_release(it_value);
        return;
    }
    
    /* read from the file and copy what's read
     to our dynamic buffer */
    char buffer[RW_BUFFER_SIZE];
    long bytes;
    
    char *dynamic_buffer = NULL;
    long dynamic_buffer_size = 0;
    
    fseek(existing->fp, saved_offset, SEEK_SET);
    while (!feof(existing->fp))
    {
        bytes = fread(buffer, 1, RW_BUFFER_SIZE, existing->fp);
        if (bytes == 0) break;
        
        long bytes_to_copy;
        if (bytes_to_read > bytes) bytes_to_copy = bytes;
        else bytes_to_copy = bytes_to_read;
        
        if (bytes_to_copy < 1) break;
        
        char *new_buffer = realloc(dynamic_buffer, dynamic_buffer_size + bytes_to_copy + 1);
        if (!new_buffer)
        {
            if (dynamic_buffer) free(dynamic_buffer);
            xte_set_result(in_stack->xtalk, xte_string_create_with_cstring(in_stack->xtalk, "End of file."));
            return;
        }
        dynamic_buffer = new_buffer;
        memcpy(dynamic_buffer + dynamic_buffer_size, buffer, bytes_to_copy);
        dynamic_buffer_size += bytes_to_copy;
        dynamic_buffer[dynamic_buffer_size] = 0;
    }
    
    /* restore the file pointer */
    fseek(existing->fp, saved_offset + bytes_to_read, SEEK_SET);
    
    /* set the "it" variable;
     cleanup */
    XTEVariant *it_value = xte_string_create_with_cstring(in_stack->xtalk, dynamic_buffer);
    free(dynamic_buffer);
    xte_set_global(in_stack->xtalk, "it", it_value);
    xte_variant_release(it_value);
}


void _acu_xt_fio_read_line(StackMgrStack *in_stack, char const *in_filepath)
{
    ACUxTalkOpenFile *existing = _lookup_xtalk_file(in_stack, in_filepath);
    if ((!existing) || (!existing->fp))
    {
        xte_callback_error(in_stack->xtalk, "File not open.", NULL, NULL, NULL);
        return;
    }
    
    char buffer[RW_BUFFER_SIZE];
    long bytes;
    
    /* explicitly tell the user when we reach the end of file
     for this command */
    if (feof(existing->fp))
    {
        xte_set_result(in_stack->xtalk, xte_string_create_with_cstring(in_stack->xtalk, "End of file."));
        return;
    }
    
    /* save our current position */
    long saved_offset = ftell(existing->fp);
    
    /* find the length of the line */
    long line_bytes = -1;
    while ((!feof(existing->fp)) && (line_bytes < 0))
    {
        long byte_offset = ftell(existing->fp);
        bytes = fread(buffer, 1, RW_BUFFER_SIZE, existing->fp);
        if (bytes == 0)
        {
            /* explicitly tell the user when we reach the end of file
             for this command */
            xte_set_result(in_stack->xtalk, xte_string_create_with_cstring(in_stack->xtalk, "End of file."));
        }
        
        for (long i = 0; i < bytes; i++)
        {
            if ((buffer[i] == 10) || (buffer[i] == 13))
            {
                line_bytes = byte_offset + i - saved_offset;
                break;
            }
        }
    }
    if (line_bytes == -1) line_bytes = ftell(existing->fp) - saved_offset;
    
    /* check the line length is not absurd */
    if ((line_bytes < 0) || (line_bytes > ACU_LIMIT_MAX_READ_LINE_BYTES))
    {
        xte_set_result(in_stack->xtalk, xte_string_create_with_cstring(in_stack->xtalk, "Not enough memory to read from file."));
        return;
    }
    
    /* read the line */
    fseek(existing->fp, saved_offset, SEEK_SET);
    char *line = malloc(line_bytes + 1);
    if (!line)
    {
        xte_set_result(in_stack->xtalk, xte_string_create_with_cstring(in_stack->xtalk, "Not enough memory to read from file."));
        return;
    }
    fread(line, 1, line_bytes, existing->fp);
    line[line_bytes] = 0;
    
    /* seek past the end of the line; ready to read the next */
    fseek(existing->fp, saved_offset + line_bytes + 1, SEEK_SET);
    
    /* store the line in "it" */
    XTEVariant *it_value = xte_string_create_with_cstring(in_stack->xtalk, line);
    free(line);
    xte_set_global(in_stack->xtalk, "it", it_value);
    xte_variant_release(it_value);
}


void _acu_xt_fio_write(StackMgrStack *in_stack, char const *in_filepath, char const *in_text, long *in_char_begin)
{
    ACUxTalkOpenFile *existing = _lookup_xtalk_file(in_stack, in_filepath);
    if ((!existing) || (!existing->fp))
    {
        xte_callback_error(in_stack->xtalk, "File not open.", NULL, NULL, NULL);
        return;
    }
    
    if (in_char_begin)
    {
        fseek(existing->fp, 0, SEEK_SET);
        _scan_to_char(existing->fp, *in_char_begin);
    }
    
    // **TODO** if the first action you perform on the file is to write,
    // the file should be truncated prior to writing
    
    /* write to the file */
    fwrite(in_text, 1, strlen(in_text), existing->fp);
}


