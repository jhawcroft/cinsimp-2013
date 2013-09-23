/*
 
 xTalk Engine: Memory Allocator
 xtalk_mem.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Custom memory allocation wrapper, primarily to provide debugging facilities:
 
 -  randomizes newly allocated memory and memory that has been free'd
 (this will make it more likely that access to such memory results in a failed assertion
 or obvious fault)
 
 -  uses 'sentinels' to help check for fence-post errors and reports as soon as an error is
 detected at the next call to the allocator, for a specific block
 
 -  tracks all allocated memory blocks so can provide a list of blocks that haven't yet been
 free'd and the amount of memory explicitly allocated (not including housekeeping) at any
 point in time
 
 -  tracks the source code file and line number where the latest allocationo of any block
 occurred, allowing easier identification of leaks
 
 *************************************************************************************************
 */

#define _XTE_ALLOCATOR_INT /* disable malloc, calloc, realloc, free macro replacements */
#include "xtalk_internal.h"

#include <libgen.h> /* required for basename() */


#if DEBUG


/******************
 Constants
 */

#define _SENTINEL_BYTES 10



/******************
 Types
 */

struct _AllocatorBlockHeader
{
    long bytes;
    char *file;
    long line;
    int flag;
    struct _AllocatorBlockHeader *next;
    struct _AllocatorBlockHeader *prev;
    unsigned char sentinel[_SENTINEL_BYTES];
};

struct _AllocatorBlockFooter
{
    unsigned char sentinel[_SENTINEL_BYTES];
};


/******************
 Globals
 */

static long _g_xte_allocated = 0;
static struct _AllocatorBlockHeader *_g_xte_allocated_blocks = NULL;



/******************
 Internal Utilities
 */

static void _xte_mem_check_sentinels(void *in_memory)
{
    in_memory -= sizeof(struct _AllocatorBlockHeader);
    struct _AllocatorBlockHeader *header = in_memory;
    for (int i = 0; i < _SENTINEL_BYTES; i++)
    {
        if (header->sentinel[i] != 0xF0)
        {
            printf("_xte_malloc: detected fencepost error!\n");
            abort();
            return;
        }
    }
    struct _AllocatorBlockFooter *footer = in_memory + sizeof(struct _AllocatorBlockHeader) + header->bytes;
    for (int i = 0; i < _SENTINEL_BYTES; i++)
    {
        if (footer->sentinel[i] != 0xF0)
        {
            printf("_xte_malloc: detected fencepost error!\n");
            abort();
            return;
        }
    }
}


static void _xte_mem_randomize(void *in_memory, long in_bytes)
{
    unsigned char *block = in_memory;
    for (int i = 0; i < in_bytes; i++)
    {
        block[i] = rand() % 255;
    }
}


static void _xte_mem_register(void *in_memory)
{
    in_memory -= sizeof(struct _AllocatorBlockHeader);
    struct _AllocatorBlockHeader *header = in_memory;
    
    //printf("Underlying acu REGIST  %p  %ld bytes\n", header, header->bytes);
    
    
    header->prev = NULL;
    header->next = _g_xte_allocated_blocks;
    
    if (header->next)
        header->next->prev = header;
    
    _g_xte_allocated_blocks = header;
}


static void _xte_mem_unregister(void *in_memory)
{
    in_memory -= sizeof(struct _AllocatorBlockHeader);
    struct _AllocatorBlockHeader *header = in_memory;
    
    //printf("Underlying acu FORGET %p  %ld bytes\n", header, header->bytes);
    
    
    if (header->next)
        header->next->prev = header->prev;
    
    if (header->prev)
        header->prev->next = header->next;
    else
        _g_xte_allocated_blocks = header->next;
}



/******************
 Debug Utilities
 */

/*
 *  _xte_mem_allocated
 *  ---------------------------------------------------------------------------------------------
 *  Returns the number of bytes explictly allocated by calls to this allocator.
 */
long _xte_mem_allocated(void)
{
    return _g_xte_allocated;
}


/*
 *  _xte_mem_baseline
 *  ---------------------------------------------------------------------------------------------
 *  Sets the flag on all currently allocated memory blocks so that we can determine which ones
 *  have been allocated most recently, at a later time.
 */
void _xte_mem_baseline(void)
{
    struct _AllocatorBlockHeader *block;
    block = _g_xte_allocated_blocks;
    while (block)
    {
        block->flag = XTE_TRUE;
        block = block->next;
    }
}


/*
 *  _xte_mem_print
 *  ---------------------------------------------------------------------------------------------
 *  Prints a list of all current memory blocks known to this allocator, including the name of the
 *  source file and line number where the most recent allocation of that block occurred.
 *
 *  Only prints those after the last call to _xte_mem_baseline().
 */
void _xte_mem_print(void)
{
    struct _AllocatorBlockHeader *block;
    printf("_xte_mem_print():\n");
    block = _g_xte_allocated_blocks;
    while (block)
    {
        if (!block->flag)
        {
            void *ptr = (void*)block + sizeof(struct _AllocatorBlockHeader);
            printf("  BLOCK: %p  % 7ld bytes  (%s: %ld)\n", ptr, block->bytes, block->file, block->line);
        }
        block = block->next;
    }
}


#endif



/******************
 Allocation API
 */

void* _xte_malloc(long in_bytes, int in_zone, char const *in_file, long in_line)
{
#ifndef DEBUG
    return malloc(in_bytes);
#else
    
    assert(in_bytes >= 0);
    if (in_file == NULL) in_file = "?";
    
    long bytes_required = in_bytes + sizeof(struct _AllocatorBlockHeader) + sizeof(struct _AllocatorBlockFooter);
    void *memory = malloc(bytes_required);
    if (!memory) return NULL;
    
    struct _AllocatorBlockHeader *header = memory;
    header->bytes = in_bytes;
    char *temp = strdup(in_file);
    header->file = strdup(basename(temp));
    free(temp);
    header->line = in_line;
    header->flag = XTE_FALSE;
    memset(header->sentinel, 0xF0, _SENTINEL_BYTES);
    
    struct _AllocatorBlockFooter *footer = memory + sizeof(struct _AllocatorBlockHeader) + in_bytes;
    memset(footer->sentinel, 0xF0, _SENTINEL_BYTES);
    
    void *result = memory + sizeof(struct _AllocatorBlockHeader);
    _xte_mem_randomize(result, in_bytes);
    
    _g_xte_allocated += in_bytes;
    _xte_mem_register(result);
    return result;
    
#endif
}


void* _xte_calloc(long in_number, long in_bytes, int in_zone, char const *in_file, long in_line)
{
#ifndef DEBUG
    return calloc(in_number, in_bytes);
#else
    void *result = _xte_malloc(in_number * in_bytes, in_zone, in_file, in_line);
    if (result) memset(result, 0, in_number * in_bytes);
    return result;
#endif
}


void* _xte_realloc(void *in_memory, long in_bytes, int in_zone, char const *in_file, long in_line)
{
#ifndef DEBUG
    void *memory = realloc(in_memory, in_bytes);
    return memory;
#else
    
    if (!in_memory)
    {
        return _xte_malloc(in_bytes, in_zone, in_file, in_line);
    }
    _xte_mem_check_sentinels(in_memory);
    
    in_memory -= sizeof(struct _AllocatorBlockHeader);
    struct _AllocatorBlockHeader *header = in_memory;
    long original_bytes = header->bytes;
    
    if (header->bytes == in_bytes)
    {
        void *result = in_memory + sizeof(struct _AllocatorBlockHeader);
        return result;
    }
    
    _xte_mem_unregister(in_memory + sizeof(struct _AllocatorBlockHeader));
    
    long bytes_required = in_bytes + sizeof(struct _AllocatorBlockHeader) + sizeof(struct _AllocatorBlockFooter);
    long bytes_added = in_bytes - header->bytes;
    void *new_memory = realloc(in_memory, bytes_required);
    if (!new_memory)
    {
        return NULL;
    }
    in_memory = new_memory;
    
    header = in_memory;
    header->bytes = in_bytes;
    
    struct _AllocatorBlockFooter *footer = in_memory + sizeof(struct _AllocatorBlockHeader) + in_bytes;
    memset(footer->sentinel, 0xF0, _SENTINEL_BYTES);
    
    void *result = in_memory + sizeof(struct _AllocatorBlockHeader);
    if (bytes_added > 0)
        _xte_mem_randomize(result + original_bytes, bytes_added);
    
    _g_xte_allocated += bytes_added;
    _xte_mem_register(result);
    return result;
    
#endif
}


void _xte_free(void *in_memory)
{
#ifndef DEBUG
    free(in_memory);
#else
    
    if (!in_memory)
    {
        printf("_xte_free: trying to free a NULL pointer!\n");
        abort();
        return;
    }
    
    _xte_mem_check_sentinels(in_memory);
    
    _xte_mem_unregister(in_memory);
    
    in_memory -= sizeof(struct _AllocatorBlockHeader);
    struct _AllocatorBlockHeader *header = in_memory;
    if (header->file) free(header->file);
    _g_xte_allocated -= header->bytes;
    
    _xte_mem_randomize(in_memory, sizeof(struct _AllocatorBlockHeader) + sizeof(struct _AllocatorBlockFooter) + header->bytes);
    
    free(in_memory);
    
#endif
}



#ifndef DEBUG
int _xte_allocator_debug = 0;
#else
int _xte_allocator_debug = 1;
#endif




void* _xte_malloc_static(long in_bytes)
{
    return malloc(in_bytes);
}


void _xte_free_static(void *in_mem)
{
    free(in_mem);
}





