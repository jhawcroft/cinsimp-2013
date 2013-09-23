/*
 
 ID Table
 idtable.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 !!  This code unit is critical to the integrity of the Stack file format.
 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack_int.h"



/* how much to increase the ID Table size each time it runs out of space;
 increasing the size requires a realloc() call.
 1024 results in allocating a page-size of 4KB, which as a minimum requirement given the prevalence
 of ID tables is probably a nice figure.
 */
#define IDTABLE_ALLOC_INCREMENT 1024

/* how many characters to use for each ID in the ASCII representation of
 the ID table */
#define IDTABLE_ASCII_CHARS 10



struct IDTable
{
    long alloc;
    long count;
    unsigned int *ids; /* 32-bit number, 4 billion ids */
    Stack *stack;
    char *ascii;
};



IDTable* idtable_create(Stack *in_stack)
{
    assert(IS_STACK(in_stack));
    
    IDTable *table;
    table = _stack_malloc(sizeof(struct IDTable));
    if (!table) return _stack_panic_null(in_stack, STACK_ERR_MEMORY);
    table->alloc = 0;
    table->count = 0;
    table->ids = NULL;
    table->stack = in_stack;
    table->ascii = NULL;
    return table;
}


void idtable_destroy(IDTable *in_table)
{
    if (in_table->ascii) _stack_free(in_table->ascii);
    if (in_table->ids) _stack_free(in_table->ids);
    _stack_free(in_table);
}


void idtable_insert(IDTable *in_table, long in_id, long in_at_index)
{
    unsigned int *new_ids;
    
    /* fix the supplied index to within bounds */
    if (in_at_index < 0) in_at_index = 0;
    if (in_at_index > in_table->count) in_at_index = in_table->count;
    
    /* make the list bigger if there's not enough room */
    if (in_table->alloc == in_table->count)
    {
        new_ids = _stack_realloc(in_table->ids, sizeof(unsigned int) * (in_table->alloc + IDTABLE_ALLOC_INCREMENT));
        if (!new_ids) return _stack_panic_void(in_table->stack, STACK_ERR_MEMORY);
        in_table->ids = new_ids;
        in_table->alloc += IDTABLE_ALLOC_INCREMENT;
    }
    
    /* shuffle the existing items out the way */
    memmove(in_table->ids + in_at_index + 1, in_table->ids + in_at_index, sizeof(unsigned int) * (in_table->count - in_at_index));
    
    /* insert the new item */
    in_table->ids[in_at_index] = (int)in_id;
    in_table->count++;
}


void idtable_append(IDTable *in_table, long in_id)
{
    idtable_insert(in_table, in_id, in_table->count);
}


void idtable_remove(IDTable *in_table, long in_index)
{
    /* check the index is within bounds */
    if ((in_index < 0) || (in_index >= in_table->count)) return;
    
    /* shuffle the other items over the top */
    memmove(in_table->ids + in_index, in_table->ids + in_index + 1, sizeof(unsigned int) * (in_table->count - in_index - 1));

    /* decrement the item count */
    in_table->count--;
}


long idtable_size(IDTable *in_table)
{
    return in_table->count;
}


long idtable_index_for_id(IDTable *in_table, long in_id)
{
    for (long i = 0; i < in_table->count; i++)
    {
        if (in_table->ids[i] == in_id) return i;
    }
    return -1;
}


long idtable_id_for_index(IDTable *in_table, long in_index)
{
    if ((in_index < 0) || (in_index >= in_table->count)) return 0;
    return in_table->ids[in_index];
}


void idtable_mutate_slot(IDTable *in_table, long in_index, long in_new_id)
{
    if ((in_index < 0) || (in_index >= in_table->count)) return;
    in_table->ids[in_index] = (int)in_new_id;
}



IDTable* idtable_create_with_ascii(Stack *in_stack, const char *in_ascii)
{
    IDTable *table;
    char buffer[IDTABLE_ASCII_CHARS + 1];
    buffer[IDTABLE_ASCII_CHARS] = 0;
    
    table = idtable_create(in_stack);
    if (!table) return NULL;
    
    long count = strlen(in_ascii) / IDTABLE_ASCII_CHARS;
    for (long index = 0; index < count; index++)
    {
        memcpy(buffer, in_ascii + (IDTABLE_ASCII_CHARS * index), IDTABLE_ASCII_CHARS);
        idtable_append(table, atol(buffer));
    }
    
    return table;
}


const char* idtable_to_ascii(IDTable *in_table)
{
    char conversionSpecifier[20];
    
    sprintf(conversionSpecifier, "%%0%dld", IDTABLE_ASCII_CHARS);
    
    if (in_table->ascii) _stack_free(in_table->ascii);
    in_table->ascii = NULL;
    
    in_table->ascii = _stack_malloc( in_table->count * IDTABLE_ASCII_CHARS + 1 );
    if (!in_table->ascii) return _stack_panic_null(in_table->stack, STACK_ERR_MEMORY);
    in_table->ascii[in_table->count * IDTABLE_ASCII_CHARS] = 0;
    
    for (long index = 0; index < in_table->count; index++)
    {
        sprintf(in_table->ascii + (IDTABLE_ASCII_CHARS * index), conversionSpecifier, in_table->ids[index]);
        
    }
    
    return in_table->ascii;
}



static int _in_array(long *in_array, int in_count, long in_id)
{
    if (in_id == 0) return 0;
    for (int i = 0; i < in_count; i++)
    {
        if (in_array[i] == in_id) return i;
    }
    return -1;
}



void idtable_send_front(IDTable *in_table, long *in_ids, int in_count)
{
    long index, tempId, insertPos, ati;
    
    /* precheck */
    for (int i = in_count-1; i >= 0; i--)
    {
        index = idtable_index_for_id(in_table, in_ids[i]);
        if (index < 0) return;
    }
    
    /* do the actual move */
    insertPos = 0;
    for (long i = 0; i < in_table->count; i++)
    {
        ati = _in_array(in_ids, in_count, in_table->ids[i]);
        if (ati >= 0)
        {
            tempId = in_table->ids[i];
            idtable_remove(in_table, i);
            idtable_insert(in_table, tempId, insertPos++);
            in_ids[ati] = 0;
        }
    }
}


void idtable_send_back(IDTable *in_table, long *in_ids, int in_count)
{
    long index, tempId, insertPos, ati;
    
    /* precheck */
    for (int i = in_count-1; i >= 0; i--)
    {
        index = idtable_index_for_id(in_table, in_ids[i]);
        if (index < 0) return;
    }
    
    /* do the actual move */
    insertPos = in_table->count;
    for (long i = in_table->count-1; i >= 0; i--)
    {
        ati = _in_array(in_ids, in_count, in_table->ids[i]);
        if (ati >= 0)
        {
            tempId = in_table->ids[i];
            idtable_remove(in_table, i);
            idtable_insert(in_table, tempId, (insertPos--)-1);
            in_ids[ati] = 0;
        }
    }
}


void idtable_shuffle_forward(IDTable *in_table, long *in_ids, int in_count)
{
    long index, tempId, insertPos, ati;
    
    /* precheck */
    for (int i = in_count-1; i >= 0; i--)
    {
        index = idtable_index_for_id(in_table, in_ids[i]);
        if (index < 1) return;
    }
    
    /* do the actual move */
    insertPos = 0;
    for (long i = 0; i < in_table->count; i++)
    {
        ati = _in_array(in_ids, in_count, in_table->ids[i]);
        if (ati >= 0)
        {
            tempId = in_table->ids[i];
            idtable_remove(in_table, i);
            idtable_insert(in_table, tempId, i-1);
            in_ids[ati] = 0;
        }
    }
}


void idtable_shuffle_backward(IDTable *in_table, long *in_ids, int in_count)
{
    long index, tempId, insertPos, ati;
    
    /* precheck */
    for (int i = in_count-1; i >= 0; i--)
    {
        index = idtable_index_for_id(in_table, in_ids[i]);
        if ((index < 0) || (index == in_table->count-1)) return;
    }
    
    /* do the actual move */
    insertPos = 0;
    for (long i = in_table->count-1; i >= 0; i--)
    {
        ati = _in_array(in_ids, in_count, in_table->ids[i]);
        if (ati >= 0)
        {
            tempId = in_table->ids[i];
            idtable_remove(in_table, i);
            idtable_insert(in_table, tempId, i+1);
            in_ids[ati] = 0;
        }
    }
}


void idtable_clear(IDTable *in_table)
{
    in_table->count = 0;
}





// probably have to remove from incoming list to prevent recursion in some cases...
// Huh??

/*
 
TEST ROUTINES
----------------
 
 
 IDTable *table;
 long index;
 
 
 table = idtable_create_with_ascii("00000102410000000037000000000700000000980000003809");
 if (!table)
 {
 printf("Failed to create table!\n");
 return 0;
 }
 printf("Created ID table.\n");
 printf("Size %ld\n", idtable_size(table));
 
 
 printf("printing:\n");
 for (index = 0; index < idtable_size(table); index++)
 {
 printf("Index %ld: %ld\n", index, idtable_id_for_index(table, index));
 
 }
 
 
 printf("Table to ascii: \"%s\"\n", idtable_to_ascii(table));
 printf("Table to ascii: \"%s\"\n", idtable_to_ascii(table));
 
 
 idtable_destroy(table);
 printf("Cleaned up.\n");
 
 
 return 0;
 
 
 
 
 IDTable *table;
 
 table = idtable_create();
 if (!table)
 {
 printf("Failed to create table!\n");
 return 0;
 }
 printf("Created ID table.\n");
 
 
 printf("Size %ld\n", idtable_size(table));
 
 
 idtable_insert(table, 5, 0);
 idtable_insert(table, 17, 0);
 idtable_insert(table, 3, 0);
 idtable_insert(table, 4, 1);
 idtable_insert(table, 42, 4);
 
 long index;
 
 printf("printing:\n");
 for (index = 0; index < idtable_size(table); index++)
 {
 printf("Index %ld: %ld\n", index, idtable_id_for_index(table, index));
 
 }
 
 idtable_remove(table, 1);
 printf("printing:\n");
 for (index = 0; index < idtable_size(table); index++)
 {
 printf("Index %ld: %ld\n", index, idtable_id_for_index(table, index));
 
 }
 
 
 printf("Index of 5: %ld\n", idtable_index_for_id(table, 5));
 printf("Index of 92: %ld\n", idtable_index_for_id(table, 92));
 
 
 idtable_destroy(table);
 printf("Cleaned up.\n");
 
 */


