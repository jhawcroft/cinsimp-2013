//
//  sorter.h
//  sorter
//
//  Created by Joshua Hawcroft on 14/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#ifndef sorter_sorter_h
#define sorter_sorter_h


typedef enum {
    SORTER_ASCENDING,
    SORTER_DESCENDING,
} SorterDirection;

typedef struct Sorter Sorter;
typedef void* Sortable;

typedef int (*SorterComparator) (Sorter *in_sorter, void *in_user, Sortable in_item1, Sortable in_item2);

Sorter* sorter_create(SorterComparator in_comparator, void *in_user);
void sorter_destroy(Sorter *in_sorter);
int sorter_add(Sorter *in_sorter, Sortable in_item);
void sorter_sort(Sorter *in_sorter, SorterDirection in_direction);
long sorter_count(Sorter *in_sorter);
Sortable sorter_item(Sorter *in_sorter, long in_index);


#endif
