/*
 
 Serialisation Buffer
 serbuff.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 !!  This code unit is critical to the integrity of the Stack file format.
 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack_int.h"



struct SerBuff
{
    long alloc;
    long size;
    char *data;
    long read_offset;
    int readonly;
    Stack *stack;
};


SerBuff* serbuff_create(Stack *in_stack, void *in_data, long in_size, int in_nocopy)
{
    assert(IS_STACK(in_stack));
    assert(IS_BOOL(in_nocopy));
    
    SerBuff *buff = _stack_malloc(sizeof(struct SerBuff));
    if (!buff) return _stack_panic_null(in_stack, STACK_ERR_MEMORY);
    buff->stack = in_stack;
    buff->readonly = 0;
    if (in_nocopy)
    {
        buff->data = in_data;
        buff->size = in_size;
        buff->alloc = in_size;
        buff->readonly = 1;
    }
    else
    {
        buff->data = _stack_malloc(in_size);
        if (!buff->data)
        {
            _stack_free(buff);
            return _stack_panic_null(in_stack, STACK_ERR_MEMORY);
        }
        buff->size = in_size;
        buff->alloc = in_size;
        memcpy(buff->data, in_data, in_size);
    }
    buff->read_offset = 0;
    return buff;
}


long serbuff_data(SerBuff *in_buff, void **out_data)
{
    *out_data = in_buff->data;
    return in_buff->size;
}


void serbuff_destroy(SerBuff *in_buff, int in_cleanup_data)
{
    if (in_buff->data && in_cleanup_data) _stack_free(in_buff->data);
    _stack_free(in_buff);
}


static int _begin_write(SerBuff *in_buff, long in_required)
{
    if (in_required + in_buff->size > in_buff->alloc)
    {
        long new_size = in_required + in_buff->size;
        char *new_data = _stack_realloc(in_buff->data, new_size);
        if (!new_data) return _stack_panic_false(in_buff->stack, STACK_ERR_MEMORY);
        
        in_buff->alloc = new_size;
        in_buff->data = new_data;
    }
    return STACK_YES;
}


void serbuff_write_long(SerBuff *in_buff, long in_long)
{
    if (!_begin_write(in_buff, sizeof(long))) return _stack_panic_void(in_buff->stack, STACK_ERR_MEMORY);
    memcpy(&(in_buff->data[in_buff->size]), &in_long, sizeof(long));
    in_buff->size += sizeof(long);
}


void serbuff_write_cstr(SerBuff *in_buff, const char *in_cstr)
{
    long len = strlen(in_cstr) + 1;
    if (!_begin_write(in_buff, len)) return _stack_panic_void(in_buff->stack, STACK_ERR_MEMORY);
    memcpy(&(in_buff->data[in_buff->size]), in_cstr, len);
    in_buff->size += len;
}


void serbuff_write_data(SerBuff *in_buff, void const *in_data, long in_size)
{
    if (!_begin_write(in_buff, sizeof(long) + in_size)) return _stack_panic_void(in_buff->stack, STACK_ERR_MEMORY);
    serbuff_write_long(in_buff, in_size);
    memcpy(&(in_buff->data[in_buff->size]), in_data, in_size);
    in_buff->size += in_size;
}


void serbuff_reset(SerBuff *in_buff)
{
    in_buff->read_offset = 0;
}


long serbuff_read_long(SerBuff *in_buff)
{
    long result = *((long*) (in_buff->data + in_buff->read_offset));
    in_buff->read_offset += sizeof(long);
    return result;
}


const char* serbuff_read_cstr(SerBuff *in_buff)
{
    const char *result = in_buff->data + in_buff->read_offset;
    in_buff->read_offset += strlen(result) + 1;
    return result;
}


long serbuff_read_data(SerBuff *in_buff, void **out_data)
{
    long bytes = serbuff_read_long(in_buff);
    *out_data = in_buff->data + in_buff->read_offset;
    in_buff->read_offset += bytes;
    return bytes;
}



