/*
 
 Stack Memory Allocator
 stack_mem.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Memory allocator
 
 Currently provides a means for the memory tests to check for leaks and fence-post errors.
 
 !!  This code unit is critical to the integrity of the Stack file format.
 
 *************************************************************************************************
 */

#include "stack_int.h"



#if STACK_TESTS

/* debugging implementations */

static long g_stack_mem_alloc_bytes = 0;
static int g_stack_mem_breakpoint = XTE_FALSE;

#define STACK_MEM_DEBUG_FENCE_POST_PADDING 10

long _stack_mem_allocated(void)
{
    return g_stack_mem_alloc_bytes;
}


void _stack_mem_breakpoint_set(int in_bp)
{
    g_stack_mem_breakpoint = in_bp;
}


static void _stack_mem_check_fenceposts(void *in_memory)
{
    long *mem_size = in_memory;
    mem_size--;
    in_memory += *mem_size;
    
    unsigned char *fenceposts = in_memory;
    for (int i = 0; i < STACK_MEM_DEBUG_FENCE_POST_PADDING; i++)
    {
        if (fenceposts[i] != 0xF0)
        {
            printf("_stack_malloc: detected fencepost error!\n");
            abort();
            return;
        }
    }
}


void* _stack_malloc(long in_size)
{
    if (g_stack_mem_breakpoint)
    {
        //int a = 0;
    }
    
    long *memory = malloc(in_size + sizeof(long) + STACK_MEM_DEBUG_FENCE_POST_PADDING);
    if (!memory) return NULL;
    
    *memory = in_size;
    g_stack_mem_alloc_bytes += in_size;
    
    memset((void*)memory + sizeof(long) + in_size, 0xF0, STACK_MEM_DEBUG_FENCE_POST_PADDING);
    
    //_xte_mem_check_fenceposts(memory + 1);
    
    return memory + 1;
}


void* _stack_calloc(long in_number, long in_size)
{
    void *memory = _stack_malloc(in_number * in_size);
    if (!memory) return NULL;
    
    memset(memory, 0, in_number * in_size);
    
    return memory;
}


void _stack_free(void *in_memory)
{
    if (g_stack_mem_breakpoint)
    {
        //int a = 0;
    }
    
    if (in_memory)
        _stack_mem_check_fenceposts(in_memory);
    
    long *memory = in_memory;
    memory--;
    
    g_stack_mem_alloc_bytes -= *memory;
    free(memory);
}


void* _stack_realloc(void *in_memory, long in_size)
{
    if (g_stack_mem_breakpoint)
    {
        //int a = 0;
    }
    
    if (in_memory)
        _stack_mem_check_fenceposts(in_memory);
    
    if ((!in_memory) && (in_size != 0))
        return _stack_malloc(in_size);
    
    if (!in_memory) return NULL;
    
    if (in_size == 0)
    {
        _stack_free(in_memory);
        return NULL;
    }
    
    long *memory = in_memory;
    memory--;
    long *new_memory = realloc(memory, in_size + sizeof(long) + STACK_MEM_DEBUG_FENCE_POST_PADDING);
    if (!new_memory) return NULL;
    
    g_stack_mem_alloc_bytes -= *new_memory;
    *new_memory = in_size;
    g_stack_mem_alloc_bytes += in_size;
    
    memset((void*)new_memory + sizeof(long) + in_size, 0xF0, STACK_MEM_DEBUG_FENCE_POST_PADDING);
    
    return new_memory + 1;
}







#else


void* _stack_malloc(long in_bytes)
{
    void *memory = malloc(in_bytes);
    return memory;
}


void* _stack_calloc(long in_number, long in_size)
{
    void *memory = calloc(in_number, in_size);
    return memory;
}


void* _stack_realloc(void *in_memory, long in_size)
{
    void *memory = realloc(in_memory, in_size);
    return memory;
}


void _stack_free(void *in_memory)
{
    free(in_memory);
}


#endif



void* _stack_malloc_static(long in_bytes)
{
#if STACK_TESTS
    //g_mem_alloc_bytes -= in_bytes;
#endif
    return malloc(in_bytes);
}


void _stack_free_static(void *in_mem)
{
#if STACK_TESTS
    //g_mem_alloc_bytes += in_bytes;
#endif
    free(in_mem);
}







