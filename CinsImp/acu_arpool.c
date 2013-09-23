/*
 
 Application Control Unit - Autorelease Pool
 acu_arpool.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Provides auto-release pools to the ACU; these are collections of memory managed objects using a
 retain/release mechanism.  When the pool is disposed of or drained, all the objects within are
 automatically sent an appropriate release() call.
 
 *************************************************************************************************
 */

#include "acu_int.h"


/******************
 Assertions
 */

#define _ACU_ARPOOL_STRUCT_ID "ARpl" /* allows us to verify that objects are what we expect them to be */
#define IS_ARPOOL(x) ((x != 0) && (strcmp(x->_struct_ident, _ACU_ARPOOL_STRUCT_ID) == 0))


/******************
 Constants
 */

/* allocated space for items increases by this number each time the pool becomes full */
#define _ACU_ARPOOL_ALLOC_ITEMS 50


/******************
 Types
 */

struct _ACUAutoreleasePool
{
    char _struct_ident[sizeof(_ACU_ARPOOL_STRUCT_ID)]; /* allows us to verify that objects are what we expect them to be */
    
    int alloc;
    int size;
    void **items_and_releasors; /* array of void*s, in pairs: [object, releasor] */
};



/******************
 Implementation
 */

/*
 *  _acu_autorelease_pool_create
 *  ---------------------------------------------------------------------------------------------
 *  Creates a new autorelease pool.
 */
ACUAutoreleasePool* _acu_autorelease_pool_create(void)
{
    ACUAutoreleasePool *result = malloc(sizeof(ACUAutoreleasePool));
    if (!result) return NULL;
    memset(result, 0, sizeof(ACUAutoreleasePool));
    strcpy(result->_struct_ident, _ACU_ARPOOL_STRUCT_ID);
    assert(IS_ARPOOL(result));
    return result;
}


/*
 *  _acu_autorelease_pool_drain
 *  ---------------------------------------------------------------------------------------------
 *  'Drains' an autorelease pool by invoking the appropriate release() function on each object
 *  and removing the objects from the pool.
 */
void _acu_autorelease_pool_drain(ACUAutoreleasePool *in_pool)
{
    assert(IS_ARPOOL(in_pool));
    
    /* release all the objects */
    for (int i = 0, item_index = 0; i < in_pool->size; i++, item_index += 2)
    {
        void *object = in_pool->items_and_releasors[item_index];
        ACUAutoreleasePoolReleasor releasor = in_pool->items_and_releasors[item_index + 1];
        
        assert(object != NULL);
        assert(releasor != NULL);
        
        releasor(object);
    }
    
    /* empty the pool */
    in_pool->size = 0;
}


/*
 *  _acu_autorelease_pool_dispose
 *  ---------------------------------------------------------------------------------------------
 *  Disposes of an autorelease pool.  First automatically drains the pool.
 */
void _acu_autorelease_pool_dispose(ACUAutoreleasePool *in_pool)
{
    assert(IS_ARPOOL(in_pool));
    _acu_autorelease_pool_drain(in_pool);
    if (in_pool->items_and_releasors) free(in_pool->items_and_releasors);
    free(in_pool);
}


/*
 *  _acu_autorelease
 *  ---------------------------------------------------------------------------------------------
 *  Adds an object to the specified pool, to be released when the pool is later drained using 
 *  the supplied <in_releasor> callback.
 *
 *  Returns NULL if an error occurred, otherwise returns the object itself.  Supplied object
 *  pointer must not be NULL.
 */
void* _acu_autorelease(ACUAutoreleasePool *in_pool, void *in_object, ACUAutoreleasePoolReleasor in_releasor)
{
    assert(IS_ARPOOL(in_pool));
    assert(in_releasor != NULL);
    
    if (!in_object) return in_object;
    
    /* check the pool is large enough to add another object;
     make it larger if it isn't */
    if (in_pool->size == in_pool->alloc)
    {
        void **new_elements = realloc(in_pool->items_and_releasors, sizeof(void*) * (in_pool->alloc + _ACU_ARPOOL_ALLOC_ITEMS) * 2);
        if (!new_elements) return NULL;
        in_pool->items_and_releasors = new_elements;
        in_pool->alloc += _ACU_ARPOOL_ALLOC_ITEMS;
    }
    
    /* add the item to the pool */
    long item_index = in_pool->size * 2;
    in_pool->items_and_releasors[item_index] = in_object;
    in_pool->items_and_releasors[item_index + 1] = in_releasor;
    in_pool->size++;
    
    return in_object;
}




