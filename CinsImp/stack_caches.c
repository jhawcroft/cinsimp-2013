/*
 
 Stack Caches
 stack_caches.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Caches of commonly used data structures - serves two purposes:
 1) has the potential to increase application performance
 2) reduces the need for memory management elsewhere in the module
 
 !!  This code unit is critical to the integrity of the Stack file format.
 
 *************************************************************************************************
 */

#include "stack_int.h"


/**********
 Widget Sequence Tables
 */


/*
 *  _stack_widget_cache_invalidate
 *  ---------------------------------------------------------------------------------------------
 *  Purges both widget sequence caches for card & background.
 *
 *  Should be called anytime whole cards/backgrounds are created/destroyed, to ensure they are
 *  not reused without a reload.
 */

void _stack_widget_cache_invalidate(Stack *in_stack)
{
    if (in_stack->card_widget_table) idtable_destroy(in_stack->card_widget_table);
    in_stack->card_widget_table = NULL;
    in_stack->cached_card_id = STACK_NO_OBJECT;
    if (in_stack->bkgnd_widget_table) idtable_destroy(in_stack->bkgnd_widget_table);
    in_stack->bkgnd_widget_table = NULL;
    in_stack->cached_bkgnd_id = STACK_NO_OBJECT;
}


/*
 *  _stack_widget_seq_cache_load
 *  ---------------------------------------------------------------------------------------------
 *  Loads the widget sequence cache for a specific layer (card/background.)  Anything in the
 *  cache currently is purged.
 *
 *  Should be called by any widget function that doesn't operate directly on the ID table returned
 *  by _stack_widget_seq_get().
 */

void _stack_widget_seq_cache_load(Stack *in_stack, long in_card_id, long in_bkgnd_id)
{
    assert(in_stack != NULL);
    assert( (in_card_id > 0) || (in_bkgnd_id > 0) );
    assert( (in_card_id < 1) || (in_bkgnd_id < 1) );
    
    /* load the sequence table data from disk */
    sqlite3_stmt *stmt;
    if (in_card_id > 0)
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT widgets FROM card WHERE cardid=?1", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_card_id);
    }
    else
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT widgets FROM bkgnd WHERE bkgndid=?1", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_bkgnd_id);
    }
    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return _stack_file_error_void(in_stack);
    }
    
    /* create a new sequence table;
     and replace the old one (if any) */
    IDTable *new_table;
    if (in_card_id > 0)
    {
        if (in_stack->card_widget_table) idtable_destroy(in_stack->card_widget_table);
        in_stack->cached_card_id = in_card_id;
        new_table = in_stack->card_widget_table = idtable_create_with_ascii(in_stack, (char*)sqlite3_column_text(stmt, 0));
    }
    else
    {
        if (in_stack->bkgnd_widget_table) idtable_destroy(in_stack->bkgnd_widget_table);
        in_stack->cached_bkgnd_id = in_bkgnd_id;
        new_table = in_stack->bkgnd_widget_table = idtable_create_with_ascii(in_stack, (char*)sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    if (!new_table) return _stack_file_error_void(in_stack);
}



/*
 *  _stack_widget_seq_get
 *  ---------------------------------------------------------------------------------------------
 *  Returns the widget sequence table for the specified layer (card/background).  If that layer's
 *  table has recently been accessed, returns a cached copy.
 *
 *  The table remains valid until the next call to _stack_widget_seq_get() or 
 *  _stack_widget_seq_cache_load(), or the stack is closed.
 *  You must not destroy the table yourself!
 *
 *  The table may be mutated, provided the caller also invokes _stack_widget_seq_set() or
 *  _stack_widget_seq_cache_load() post any change to bring the disk and memory into alignment.
 */

IDTable* _stack_widget_seq_get(Stack *in_stack, long in_card_id, long in_bkgnd_id)
{
    assert(in_stack != NULL);
    assert( (in_card_id > 0) || (in_bkgnd_id > 0) );
    assert( (in_card_id < 1) || (in_bkgnd_id < 1) );
    
    /* check if the cache has the correct data;
     if it does, just return the table */
    if ((in_card_id > 0) && (in_stack->cached_card_id == in_card_id))
        return in_stack->card_widget_table;
    if ((in_bkgnd_id > 0) && (in_stack->cached_bkgnd_id == in_bkgnd_id))
        return in_stack->bkgnd_widget_table;
    
    /* request the cache be updated from disk */
    _stack_widget_seq_cache_load(in_stack, in_card_id, in_bkgnd_id);
    
    /* return the cache */
    if (in_card_id > 0) return in_stack->card_widget_table;
    else return in_stack->bkgnd_widget_table;
}


/*
 *  _stack_widget_seq_set
 *  ---------------------------------------------------------------------------------------------
 *  Writes the specified widget sequence table for the specified layer (card/background) to disk.
 *
 *  Does nothing with the table.  If the table was previously returned by _stack_widget_seq_get(),
 *  you do not need to do anything with it.
 *
 *  If however you allocated the IDtable yourself, you must remember to _destroy() it when done.
 */

void _stack_widget_seq_set(Stack *in_stack, long in_card_id, long in_bkgnd_id, IDTable *in_seq)
{
    assert(in_stack != NULL);
    assert( (in_card_id > 0) || (in_bkgnd_id > 0) );
    assert( (in_card_id < 1) || (in_bkgnd_id < 1) );
    
    /* write the specified sequence table to disk */
    sqlite3_stmt *stmt;
    int err;
    if (in_card_id > 0)
    {
        sqlite3_prepare_v2(in_stack->db, "UPDATE card SET widgets=?2 WHERE cardid=?1", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_card_id);
    }
    else
    {
        sqlite3_prepare_v2(in_stack->db, "UPDATE bkgnd SET widgets=?2 WHERE bkgndid=?1", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_bkgnd_id);
    }
    sqlite3_bind_text(stmt, 2, idtable_to_ascii(in_seq), -1, SQLITE_STATIC);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE) return _stack_file_error_void(in_stack);
    
    /* reload the cache, if necessary */
    if ((in_card_id > 0) && (in_stack->cached_card_id == in_card_id))
        _stack_widget_seq_cache_load(in_stack, in_card_id, in_bkgnd_id);
    if ((in_bkgnd_id > 0) && (in_stack->cached_bkgnd_id == in_bkgnd_id))
        _stack_widget_seq_cache_load(in_stack, in_card_id, in_bkgnd_id);
}




