//
//  sorter.c
//  sorter
//
//  Created by Joshua Hawcroft on 14/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sorter.h"





#define SORTER_ALLOC_SIZE 4096


struct Sorter
{
    long item_alloc;
    long item_count;
    Sortable *items;
    void* user;
    SorterComparator comp;
};


Sorter* sorter_create(SorterComparator in_comparator, void *in_user)
{
    assert(in_comparator != NULL);
    Sorter *sorter = calloc(1, sizeof(Sorter));
    if (!sorter) return NULL;
    sorter->comp = in_comparator;
    sorter->user = in_user;
    return sorter;
}


void sorter_destroy(Sorter *in_sorter)
{
    assert(in_sorter != NULL);
    if (in_sorter->items) free(in_sorter->items);
    free(in_sorter);
}


long sorter_count(Sorter *in_sorter)
{
    assert(in_sorter != NULL);
    return in_sorter->item_count;
}


Sortable sorter_item(Sorter *in_sorter, long in_index)
{
    assert(in_sorter != NULL);
    assert(in_index >= 0);
    assert(in_index < in_sorter->item_count);
    return in_sorter->items[in_index];
}


int sorter_add(Sorter *in_sorter, Sortable in_item)
{
    assert(sizeof(Sortable) < SORTER_ALLOC_SIZE);
    assert(in_sorter != NULL);
    
    /* increase the number of allocated slots (if necessary) */
    if ((in_sorter->item_count + 1) * sizeof(Sortable) > in_sorter->item_alloc)
    {
        Sortable *new_items = realloc(in_sorter->items, sizeof(Sortable) * (in_sorter->item_alloc + SORTER_ALLOC_SIZE));
        if (!new_items) return 0;
        in_sorter->items = new_items;
        in_sorter->item_alloc += SORTER_ALLOC_SIZE;
    }
    
    /* append the item */
    in_sorter->items[in_sorter->item_count++] = in_item;
    return 1;
}


void sorter_sort(Sorter *in_sorter, SorterDirection in_direction)
{
    /* simple bubble sort */
    int continue_sort = 1;
    long item_limit = in_sorter->item_count - 1;
    int direction = ((in_direction == SORTER_ASCENDING) ? 1 : -1);
    while (continue_sort)
    {
        continue_sort = 0;
        for (long item = 0; item < item_limit; item++)
        {
            int diff = in_sorter->comp(in_sorter, in_sorter->user, in_sorter->items[item], in_sorter->items[item+1]);
            if (diff == direction)
            {
                /* swap */
                Sortable temp = in_sorter->items[item];
                in_sorter->items[item] = in_sorter->items[item+1];
                in_sorter->items[item+1] = temp;
                
                continue_sort = 1;
            }
        }
    }
}




