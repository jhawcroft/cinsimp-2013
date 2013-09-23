/*
 
 Stack File Format
 stack_cards.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Card enumeration & manipulation

 !!  This code unit is critical to the integrity of the Stack file format.
 
 *************************************************************************************************
 
 Card Sequence Management
 -------------------------------------------------------------------------------------------------
 
 Cards are maintained in a specific sequence within the stack.  This is achieved through two 
 mechanisms:
 
 1.     a table of card IDs with entries position within the table corresponding to their sequence
        within the stack.
        (currently implemented by the "cards" column of the underlying "stack" SQLite table)
 
 2.     a list of recently added/deleted card IDs and their associated sequence stored on disk.
        (currently implemented by the underlying "stack_card_changes" SQLlite table)
 
 These mechanisms work as follows:
 
 -      When the stack is opened:
        -   the table (1) is loaded,
        -   and the list (2) is replayed, inserting recently created cards into the correct sequence
            in the sequence table and displacing the cards following, and removing deleted cards
            from the in memory sequence (1)
 
 -      When a card is created:
        -   it is added to the sequence table (1) in memory,
        -   and an entry added to the list (2) on disk
 
 -      When a card is deleted:
        -   it is deleted from the sequence table (1) in memory,
        -   and an entry added to the list (2) on disk
 
 -      When any of the following conditions/actions occur:
        -   the cards are sorted, or
        -   when the number of items in the list (2) is beyond a threshold (at stack open), or
        -   when the stack is "compacted"
 
        The following action will occur:
        -   the up-to-date sequence table (1) is purged to disk,
        -   and the list (2) is purged
 
 In this way, a stack can contain (in theory) many millions of cards, in sequence.  Operating time
 of the stack during card inserts and deletes should be fairly consistent and insignificant.
 Massive unnecessary disk I/O during card creation/deletion is also minimised.
 
 */

#include "stack_int.h"


/*********
 Configuration
 */

/*
 *  _STACK_CARD_SEQUENCE_FLUSH_THRESHOLD
 *  ---------------------------------------------------------------------------------------------
 *  When the number of new/deleted cards since the last flush to disk of the sequence table
 *  is >= this threshold on stack_open(), the complete sequence table will be flushed to disk
 *  and the change list purged.  See also _stack_card_table_reload().
 *
 *  This is part of Card Sequence Management - discussed in the header of this unit.
 *
 *  Discussion:
 *
 *  In theory, a larger setting is better.  For small stacks, the setting really doesn't matter.
 *  But for large stacks, if the setting is too small, then the user may be forced at stack open
 *  to wait for a flush more frequently.  Also, if it's too big the user may notice a lag on open.
 *  Ultimately if speed is always a compromise, we should avoid unnecessary file I/O, which means
 *  a larger number.
 */

#define _STACK_CARD_SEQUENCE_FLUSH_THRESHOLD 500



/*********
 Internal Card Management
 */


/*
 *  _stack_card_table_flush
 *  ---------------------------------------------------------------------------------------------
 *  Writes the card sequence table to disk and purges the change list.
 *  Invoked by _sort(), _compact() and from _stack_card_table_load().
 *
 *  This is part of Card Sequence Management - discussed in the header of this unit.
 */

void _stack_card_table_flush(Stack *in_stack)
{
    assert(in_stack != NULL);
    
    if (!stack_is_writable(in_stack)) return;
    
    _stack_begin(in_stack, STACK_NO_FLAGS);
    
    sqlite3_stmt *stmt;
    int err;
    err = sqlite3_prepare_v2(in_stack->db, "UPDATE stack SET cards=?1", -1, &stmt, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_text(stmt, 1, idtable_to_ascii(in_stack->stack_card_table), -1, SQLITE_STATIC);
    assert(err == SQLITE_OK);
    err = sqlite3_step(stmt);
    assert(err == SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    err = sqlite3_exec(in_stack->db, "DELETE FROM stack_card_changes", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    
    _stack_commit(in_stack);
}


/*
 *  _stack_card_table_load
 *  ---------------------------------------------------------------------------------------------
 *  Loads the card sequence table from disk at stack_open().
 *
 *  This is part of Card Sequence Management - discussed in the header of this unit.
 */

void _stack_card_table_load(Stack *in_stack)
{
    assert(in_stack != NULL);
    assert(in_stack->stack_card_table == NULL);
    
    /* load the stack file's card table */
    sqlite3_stmt *stmt;
    int err;
    err = sqlite3_prepare_v2(in_stack->db, "SELECT cards FROM stack", -1, &stmt, NULL);
    err = sqlite3_step(stmt);
    assert(err == SQLITE_ROW);
    in_stack->stack_card_table = idtable_create_with_ascii(in_stack, (char*)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    assert(in_stack->stack_card_table != NULL);

    /* play the stack file's card changes list */
    err = sqlite3_prepare_v2(in_stack->db, "SELECT cardid,deleted,sequence FROM stack_card_changes ORDER BY entryid", -1, &stmt, NULL);
    assert(err == SQLITE_OK);
    int change_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        change_count++;
        if (sqlite3_column_int(stmt, 1) == 0)
            idtable_insert(in_stack->stack_card_table, sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 2));
        else
            idtable_remove(in_stack->stack_card_table, idtable_index_for_id(in_stack->stack_card_table, sqlite3_column_int(stmt, 0)));
    }
    sqlite3_finalize(stmt);
   
    /* if there have been a lot of changes since we flushed the ID table,
     then flush the ID table! */
    if (change_count >= _STACK_CARD_SEQUENCE_FLUSH_THRESHOLD)
        _stack_card_table_flush(in_stack);
}



/*
 *  _cards_bkgnd
 *  ---------------------------------------------------------------------------------------------
 *  Returns the background ID for the specified card.
 *
 *  Supplied card ID must be valid or this function may report a stack IO error.
 */

long _cards_bkgnd(Stack *in_stack, long in_card_id)
{
    assert(in_stack != NULL);
    assert(in_card_id > 0);
    
    long bkgnd_id;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db,
                       "SELECT bkgndid FROM card WHERE cardid=?1",
                       -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_id);
    if (sqlite3_step(stmt) == SQLITE_ROW)
        bkgnd_id = sqlite3_column_int(stmt, 0);
    else
        bkgnd_id = STACK_NO_OBJECT;
    sqlite3_finalize(stmt);
    
    if (bkgnd_id == STACK_NO_OBJECT)
        _stack_panic_void(in_stack, STACK_ERR_IO);
    return bkgnd_id;
}


/*
 *  _stack_card_create
 *  ---------------------------------------------------------------------------------------------
 *  Creates a new card, immediately after the specified card ID in the stack.
 *
 *  If bkgnd ID isn't STACK_NO_OBJECT, then creates in that background.  Otherwise uses the 
 *  background of the card it's created after.
 *
 *  Returns the ID of the new card if successful, or STACK_NO_OBJECT if there was a problem.
 *  If there was an error, out_error is set to an error code:
 *
 *  - STACK_ERR_TOO_MANY_CARDS - if the stack has reached the maximum specified number of cards
 *  - STACK_ERR_UNKNOWN - everything else
 */
long _stack_card_create(Stack *in_stack, long in_bkgnd_id, long in_after_card_id, int *out_error)
{
    assert(IS_STACK(in_stack));
    
    long new_card_id = 0;
    _stack_begin(in_stack, STACK_NO_FLAGS);
    
    /* get the card's background */
    long bkgnd_id;
    if (in_bkgnd_id != STACK_NO_OBJECT)
        bkgnd_id = in_bkgnd_id;
    else
        bkgnd_id = _cards_bkgnd(in_stack, in_after_card_id);
    
    /* create the card */
    sqlite3_stmt *stmt;
    int err;
    sqlite3_prepare_v2(in_stack->db, "INSERT INTO card (bkgndid, widgets, name, cantdelete, dontsearch, script)"
                       " VALUES (?1, '', '', 0, 0, '')", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)bkgnd_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE) {
        _stack_cancel(in_stack);
        return STACK_NO_OBJECT;
    }
    new_card_id = sqlite3_last_insert_rowid(in_stack->db);
    
    /* add to stack's card changes list */
    long next_card_index = idtable_index_for_id(in_stack->stack_card_table, in_after_card_id) + 1;
    sqlite3_prepare_v2(in_stack->db, "INSERT INTO stack_card_changes (cardid, deleted, sequence)"
                       " VALUES (?1, 0, ?2)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)new_card_id);
    sqlite3_bind_int(stmt, 2, (int)next_card_index);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE) {
        _stack_cancel(in_stack);
        return STACK_NO_OBJECT;
    }
    
    /* commit changes to disk */
    if (_stack_commit(in_stack) != SQLITE_OK)
    {
        _stack_cancel(in_stack);
        return STACK_NO_OBJECT;
    }
    
    /* add to stack's card table */
    assert(in_stack->stack_card_table != NULL);
    if (next_card_index >= idtable_size(in_stack->stack_card_table))
        idtable_append(in_stack->stack_card_table, new_card_id);
    else
        idtable_insert(in_stack->stack_card_table, new_card_id, next_card_index);
    
    /* record the undo step */
    /*SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    if (undo_data)
    {
        serbuff_write_long(undo_data, new_card_id);
        _undo_record_step(in_stack, UNDO_CARD_CREATE, undo_data);
    }*/
    
    /* invalidate the widget cache */
    _stack_widget_cache_invalidate(in_stack);
    
    /* return the new card ID */
    *out_error = STACK_OK;
    stack_undo_flush(in_stack);
    return new_card_id;
}




/*********
 Public API
 */

/* accessors: */

/*
 *  stack_card_id_for_index
 *  ---------------------------------------------------------------------------------------------
 *  Obtain the ID for a card within the stack, with the specified index (0-based).
 *
 *  If the supplied index is invalid, returns STACK_NO_OBJECT. 
 */

long stack_card_id_for_index(Stack *in_stack, long in_index)
{
    assert(in_stack != NULL);
    assert(in_stack->stack_card_table != NULL);
    
    if (in_index < 0) return STACK_NO_OBJECT;
    
    long card_id = idtable_id_for_index(in_stack->stack_card_table, in_index);
    if (card_id < 1) return STACK_NO_OBJECT;
    
    return card_id;
}


/* other lookup function, ie. by name should go here */



/*
 *  stack_card_count
 *  ---------------------------------------------------------------------------------------------
 *  Obtains the number of cards in the stack.
 *
 *  Will always return a positive integer > 0 (stacks cannot be empty).
 */

long stack_card_count(Stack *in_stack)
{
    assert(in_stack != NULL);
    assert(in_stack->stack_card_table != NULL);
    long count = idtable_size(in_stack->stack_card_table);
    assert(count > 0);
    return count;
}


/*
 *  stack_bkgnd_card_count
 *  ---------------------------------------------------------------------------------------------
 *  Obtains the number of cards in the background specified.
 *
 *  Will always return a positive integer > 0 (backgrounds cannot be empty).
 */
long stack_bkgnd_card_count(Stack *in_stack, long in_bkgnd_id)
{
    assert(in_stack != NULL);
    assert(in_bkgnd_id > 0);
    assert(in_stack->stack_card_table != NULL);
    
    long count = 0;
    sqlite3_stmt *stmt = NULL;
    
    /* query the list of cards in a particular background */
    sqlite3_prepare_v2(in_stack->db, "SELECT cardid FROM card WHERE bkgndid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_bkgnd_id);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int cardid = sqlite3_column_int(stmt, 0);
        
        /* check if the card exists */
        if (idtable_index_for_id(in_stack->stack_card_table, cardid) >= 0) count++;
    }
    sqlite3_finalize(stmt);
    
    assert(count > 0);
    return count;
}


/*
 *  stack_card_index_for_id
 *  ---------------------------------------------------------------------------------------------
 *  Obtain the index for a card with the specified ID.
 *
 *  If there's no such card, returns STACK_NO_OBJECT.
 */
long stack_card_index_for_id(Stack *in_stack, long in_id)
{
    assert(in_stack != NULL);
    assert(in_stack->stack_card_table != NULL);
    return idtable_index_for_id(in_stack->stack_card_table, in_id);
}


/*
 *  stack_card_id_for_name
 *  ---------------------------------------------------------------------------------------------
 *  Obtain the ID for a card with the specified name.
 *
 *  If there's no such card, returns STACK_NO_OBJECT.
 */
long stack_card_id_for_name(Stack *in_stack, char const *in_name)
{
    assert(in_stack != NULL);
    assert(in_stack->stack_card_table != NULL);
    
    long result = STACK_NO_OBJECT;
    sqlite3_stmt *stmt;
    sqlite3_prepare(in_stack->db, "SELECT cardid FROM card WHERE name=?1", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, in_name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    if ((result != STACK_NO_OBJECT) && (stack_card_index_for_id(in_stack, result) >= 0))
        return result;
    else
        return STACK_NO_OBJECT;
}


/*
 *  stack_card_bkgnd_id
 *  ---------------------------------------------------------------------------------------------
 *  Returns the background ID for the specified card.
 *
 *  ! The card ID must exist, or this function may report an IO error causing stack closure.
 */

long stack_card_bkgnd_id(Stack *in_stack, long in_card_id)
{
    return _cards_bkgnd(in_stack, in_card_id);
}



/* mutators: */

/*
 *  stack_card_create
 *  ---------------------------------------------------------------------------------------------
 *  Creates a new card, immediately after the specified card ID in the stack.
 *
 *  Returns the ID of the new card if successful, or STACK_NO_OBJECT if there was a problem.
 *  If there was an error, out_error is set to an error code:
 *
 *  - STACK_ERR_TOO_MANY_CARDS - if the stack has reached the maximum specified number of cards
 *  - STACK_ERR_UNKNOWN - everything else
 */
long stack_card_create(Stack *in_stack, long in_after_card_id, int *out_error)
{
    assert(in_stack != NULL);
    assert(in_after_card_id > 0);
    
    *out_error = STACK_ERR_UNKNOWN;
    
    /* check if the stack is writable */
    if (!stack_is_writable(in_stack)) return STACK_NO_OBJECT;
    
    /* check there aren't too many cards */
    if (stack_card_count(in_stack) >= _STACK_CARD_LIMIT)
    {
        *out_error = STACK_ERR_TOO_MANY_CARDS;
        return STACK_NO_OBJECT;
    }
    
    return _stack_card_create(in_stack, STACK_NO_OBJECT, in_after_card_id, out_error);
}


/*
 *  stack_bkgnd_create
 *  ---------------------------------------------------------------------------------------------
 *  Creates a new background, immediately after the specified card ID in the stack.
 *
 *  Returns the ID of the first card of the new background, if successful, or STACK_NO_OBJECT
 *  if there was a problem.  If there was an error, out_error is set to an error code:
 *
 *  - STACK_ERR_TOO_MANY_CARDS - if the stack has reached the maximum specified number of cards
 *  - STACK_ERR_UNKNOWN - everything else
 */
long stack_bkgnd_create(Stack *in_stack, long in_after_card_id, int *out_error)
{
    assert(in_stack != NULL);
    assert(in_after_card_id > 0);
    
    *out_error = STACK_ERR_UNKNOWN;
    
    /* check if the stack is writable */
    if (!stack_is_writable(in_stack)) return STACK_NO_OBJECT;
    
    /* check there aren't too many cards */
    if (stack_card_count(in_stack) >= _STACK_CARD_LIMIT)
    {
        *out_error = STACK_ERR_TOO_MANY_CARDS;
        return STACK_NO_OBJECT;
    }
    
    long new_card_id = 0;
    long bkgnd_id = 0;
    _stack_begin(in_stack, STACK_ENTRY_POINT);
    
    /* create the background */
    sqlite3_stmt *stmt;
    int err;
    sqlite3_prepare_v2(in_stack->db, "INSERT INTO bkgnd (widgets, name, cantdelete, dontsearch, script)"
                       " VALUES ('', '', 0, 0, '')", -1, &stmt, NULL);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE) {
        _stack_cancel(in_stack);
        return STACK_NO_OBJECT;
    }
    bkgnd_id = sqlite3_last_insert_rowid(in_stack->db);
    
    /* create the card */
    new_card_id = _stack_card_create(in_stack, bkgnd_id, in_after_card_id, out_error);
    
    /* commit changes to disk */
    if (_stack_commit(in_stack) != SQLITE_OK)
    {
        _stack_cancel(in_stack);
        return STACK_NO_OBJECT;
    }
    
    /* invalidate the widget cache */
    _stack_widget_cache_invalidate(in_stack);
    
    /* return the ID of the first card of the new background */
    *out_error = STACK_OK;
    stack_undo_flush(in_stack);
    return new_card_id;
}


/*
 *  stack_card_delete
 *  ---------------------------------------------------------------------------------------------
 *  Deletes the specified card.
 *
 *  Returns the ID of the next card if successful, or the ID of the card to delete if there is
 *  a problem.
 */
long stack_card_delete(Stack *in_stack, long in_card_id)
{
    assert(in_stack != NULL);
    assert(in_card_id > 0);
    
    /* check if the stack is writable */
    if (!stack_is_writable(in_stack)) return in_card_id;
    
    sqlite3_stmt *stmt;
    int err;
    long sequence, card_count;
    
    /* check if we're allowed to delete the card */
    if (stack_prop_get_long(in_stack, in_card_id, 0, PROPERTY_CANTDELETE)) return in_card_id;
    
    /* check there isn't only 1 card left in the stack */
    card_count = stack_card_count(in_stack);
    if (card_count == 1) return in_card_id;
    
    /* get the background of the card */
    long bkgnd_id = _cards_bkgnd(in_stack, in_card_id);
    assert(bkgnd_id > 0);
    
    /* prepare the undo step */
    //SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    //if (undo_data)
    //{
   //     void const *card_data;
    //    long card_data_size = _card_serialize(in_stack, in_card_id, &card_data);
    //    serbuff_write_data(undo_data, card_data, card_data_size);
    //}
    
    _stack_begin(in_stack, STACK_ENTRY_POINT);
    
    /* perform the deletion */
    sqlite3_prepare_v2(in_stack->db, "DELETE FROM card WHERE cardid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
    {
        //if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        return in_card_id;
    }
    
    sqlite3_prepare_v2(in_stack->db, "DELETE FROM widget_content WHERE widgetid IN "
                       "(SELECT widgetid FROM widget WHERE cardid=?1)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
    {
        //if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        return in_card_id;
    }
    
    sqlite3_prepare_v2(in_stack->db, "DELETE FROM widget_content WHERE cardid=?2 AND widgetid IN "
                       "(SELECT widgetid FROM widget WHERE bkgndid=?1)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)bkgnd_id);
    sqlite3_bind_int(stmt, 2, (int)in_card_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
    {
        //if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        return in_card_id;
    }
    
    sqlite3_prepare_v2(in_stack->db, "DELETE FROM widget_options WHERE widgetid IN "
                       "(SELECT widgetid FROM widget WHERE cardid=?1)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
    {
        //if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        return in_card_id;
    }
    
    sqlite3_prepare_v2(in_stack->db, "DELETE FROM widget WHERE cardid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
    {
        //if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        return in_card_id;
    }
    
    /* add to stack's card changes list */
    sqlite3_prepare_v2(in_stack->db, "INSERT INTO stack_card_changes (cardid, deleted, sequence)"
                             " VALUES (?1, 1, -1)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
    {
        //if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        return in_card_id;
    }
    
    /* write changes to disk */
    if (_stack_commit(in_stack) != SQLITE_OK)
    {
        //if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        return in_card_id;
    }
    
    /* remove from stack's card table */
    assert(in_stack->stack_card_table != NULL);
    sequence = idtable_index_for_id(in_stack->stack_card_table, in_card_id);
    idtable_remove(in_stack->stack_card_table, sequence);
    if (sequence >= idtable_size(in_stack->stack_card_table)) sequence = 0;
    
    /* record the undo step */
    //if (undo_data)
    //    _undo_record_step(in_stack, UNDO_CARD_DELETE, undo_data);
    
    /* invalidate the widget cache */
    _stack_widget_cache_invalidate(in_stack);
    
    /* return the next card ID */
    long next_card_id = idtable_id_for_index(in_stack->stack_card_table, sequence);
    assert(next_card_id > 0);
    stack_undo_flush(in_stack);
    return next_card_id;
}





