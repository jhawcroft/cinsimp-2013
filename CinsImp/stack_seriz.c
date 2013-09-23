/*
 
 Stack Serialization
 stack_seriz.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Serialization routines; used by undo/redo and clipboard routines to decompose/recompose objects
 as/from byte streams
 
 !!  This code unit is critical to the integrity of the Stack file format.
 
 *************************************************************************************************
 */

#include "stack_int.h"



/**********
 Widgets
 */


/*
 *  _widgets_serialize
 *  ---------------------------------------------------------------------------------------------
 *  Creates a block of data representative of the specified widget IDs.
 *
 *  Returns the size of the data block if successful, or zero (0) on error.
 *  out_data is set to NULL if there's a problem.
 *
 *  Function retains ownership of the data.  Caller must copy if they wish to make changes.
 *  Data will remain available until the next call or the stack is closed.
 */

long _widgets_serialize(Stack *in_stack, long *in_ids, int in_count, void **out_data, int in_incl_content)
{
    assert(in_stack != NULL);
    assert(in_ids != NULL);
    assert(out_data != NULL);
    
    /* in case of error, adhere to our contract with the caller */
    *out_data = NULL;
    
    /* create a serialization buffer to help build the result;
     and cleanup the last one (if any) */
    if (in_stack->serializer_widgets) serbuff_destroy(in_stack->serializer_widgets, 1);
    in_stack->serializer_widgets = NULL;
    in_stack->serializer_widgets = serbuff_create(in_stack, NULL, 0, 0);
    if (!in_stack->serializer_widgets) return 0;
    
    /* serialize each widget */
    long card_id = STACK_NO_OBJECT, bkgnd_id = STACK_NO_OBJECT;
    int err;
    serbuff_write_long(in_stack->serializer_widgets, in_count);
    for (int i = 0; i < in_count; i++)
    {
        /* serialize the widget particulars */
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(in_stack->db, "SELECT name,type,shared,dontsearch,x,y,width,height,locked,cardid,bkgndid "
                           "FROM widget WHERE widgetid=?1", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_ids[i]);
        err = sqlite3_step(stmt);
        
#if DEBUG
        //printf("Serializing widget ID %ld\n", in_ids[i]);
#endif
        
        serbuff_write_long(in_stack->serializer_widgets, in_ids[i]);
        serbuff_write_cstr(in_stack->serializer_widgets, (const char*)sqlite3_column_text(stmt, 0));
        serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 1));
        serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 2));
        serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 3));
        serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 4));
        serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 5));
        serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 6));
        serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 7));
        serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 8));
        
        serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 9));
        serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 10));
        
        /* ...and grab layer owners while we're at it */
        if ((card_id == STACK_NO_OBJECT) && (sqlite3_column_int(stmt, 9) > 0))
            card_id = sqlite3_column_int(stmt, 9);
        if ((bkgnd_id == STACK_NO_OBJECT) && (sqlite3_column_int(stmt, 10) > 0))
            bkgnd_id = sqlite3_column_int(stmt, 10);
        
        assert( (card_id == STACK_NO_OBJECT) || (sqlite3_column_int(stmt, 9) <= 0) ||
               (sqlite3_column_int(stmt, 9) == card_id) );
        assert( (bkgnd_id == STACK_NO_OBJECT) || (sqlite3_column_int(stmt, 10) <= 0) ||
               (sqlite3_column_int(stmt, 10) == bkgnd_id) );

        sqlite3_finalize(stmt);
        if (err != SQLITE_ROW) return 0;
        
        sqlite3_prepare_v2(in_stack->db, "SELECT COUNT(*) "
                           "FROM widget_options WHERE widgetid=?1", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_ids[i]);
        err = sqlite3_step(stmt);
        int option_count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (err != SQLITE_ROW) return 0;
        serbuff_write_long(in_stack->serializer_widgets, option_count);
        
        err = sqlite3_prepare_v2(in_stack->db, "SELECT optionid,value "
                           "FROM widget_options WHERE widgetid=?1", -1, &stmt, NULL);
        if (err != SQLITE_OK) return 0;
        sqlite3_bind_int(stmt, 1, (int)in_ids[i]);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 0));
            serbuff_write_cstr(in_stack->serializer_widgets, (const char*)sqlite3_column_text(stmt, 1));
        }
        sqlite3_finalize(stmt);
        
        if (in_incl_content)
        {
            sqlite3_prepare_v2(in_stack->db, "SELECT COUNT(*) "
                               "FROM widget_content WHERE widgetid=?1", -1, &stmt, NULL);
            sqlite3_bind_int(stmt, 1, (int)in_ids[i]);
            err = sqlite3_step(stmt);
            option_count = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            if (err != SQLITE_ROW) return 0;
            serbuff_write_long(in_stack->serializer_widgets, option_count);
            
            err = sqlite3_prepare_v2(in_stack->db, "SELECT cardid,bkgndid,searchable,formatted "
                                     "FROM widget_content WHERE widgetid=?1", -1, &stmt, NULL);
            if (err != SQLITE_OK) return 0;
            sqlite3_bind_int(stmt, 1, (int)in_ids[i]);
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 0));
                serbuff_write_long(in_stack->serializer_widgets, sqlite3_column_int(stmt, 1));
                serbuff_write_cstr(in_stack->serializer_widgets, (void*)sqlite3_column_text(stmt, 2));
                serbuff_write_data(in_stack->serializer_widgets, (void*)sqlite3_column_blob(stmt, 3), sqlite3_column_bytes(stmt, 3));
            }
            sqlite3_finalize(stmt);
        }
        else
            serbuff_write_long(in_stack->serializer_widgets, 0); /* no content */
    }
    
    /* serialize the widget sequence */
    if (in_incl_content)
    {
        if (card_id != STACK_NO_OBJECT)
        {
            IDTable *widget_seq = _stack_widget_seq_get(in_stack, card_id, 0);
            serbuff_write_cstr(in_stack->serializer_widgets, idtable_to_ascii(widget_seq));
        }
        else serbuff_write_cstr(in_stack->serializer_widgets, "");
        if (bkgnd_id != STACK_NO_OBJECT)
        {
            IDTable *widget_seq = _stack_widget_seq_get(in_stack, 0, bkgnd_id);
            serbuff_write_cstr(in_stack->serializer_widgets, idtable_to_ascii(widget_seq));
        }
        else serbuff_write_cstr(in_stack->serializer_widgets, "");
    }
    else
    {
        serbuff_write_cstr(in_stack->serializer_widgets, "");
        serbuff_write_cstr(in_stack->serializer_widgets, "");
    }
    
    /* return the size of the buffer, and the buffer content */
    return serbuff_data(in_stack->serializer_widgets, out_data);
}


/*
 *  _widgets_unserialize
 *  ---------------------------------------------------------------------------------------------
 *  Creates widget(s) on the specified card, using the data previously generated by 
 *  _widgets_serialize().
 *
 *  Returns the number of widgets created if successful, or STACK_NO_OBJECT on error.
 *
 *  If out_ids is specified, on success, *out_ids will be pointed to an array of the created 
 *  widget IDs.  On failure, it will be pointed to NULL.  This array will remain available until
 *  the next call or the stack is closed.
 *
 *  Attempts to reuse the widget ID of the widget when it was serialized.  However, if the 
 *  widget ID is no longer available (as may be the case during a pasteboard operation) a completely 
 *  new widget ID will be generated.
 *
 *  Operates in two different modes:
 *
 *  1)  If in_incl_content is STACK_YES, then content and widget sequences will be preserved (undo).
 *      And widgets will be restored to their respective original layers (pre-serialization.)
 *
 *  2)  Otherwise, if in_incl_content is STACK_NO, conntent will not be recreated and the widgets 
 *      will be appended to the end of the widget sequence (paste).
 *      Widgets will all be placed into the specified layer (card or background).
 *
 *  In addition, in mode 1), when including content, if in_only_content is STACK_YES,
 *  ONLY the content of the widgets is unserialized and the widgets themselves are assumed to 
 *  already exist (such is the case for text in background fields when a card is restored.)
 *
 *  Caller retains ownership of the data.
 */

int _widgets_unserialize(Stack *in_stack, void *in_data, long in_data_size, long in_card_id, long in_bkgnd_id,
                                long const **out_ids, int in_incl_content, int in_undoable, int in_only_content)
{
    assert(in_stack != NULL);
    assert(in_data != NULL);
    assert((in_card_id > 0) || (in_bkgnd_id > 0));
    
    /* in case of error, adhere to our contract with the caller */
    if (out_ids) *out_ids = NULL;
    
    /* create a serialization buffer to help unserialize the widget(s) */
    SerBuff *buff = serbuff_create(in_stack, in_data, in_data_size, STACK_NO_COPY);
    if (!buff) return STACK_NO_OBJECT;
    
    /* count the number of widgets to recreate */
    int count = (int)serbuff_read_long(buff);
    
    /* check the number of cards isn't too much */
    IDTable *widget_table = _stack_widget_seq_get(in_stack, in_card_id, in_bkgnd_id);
    if (idtable_size(widget_table) + count >= _LAYER_WIDGET_LIMIT)
    {
        serbuff_destroy(buff, 0);
        return STACK_NO_OBJECT;
    }
    
    /* create a list of IDs for the created widget(s) */
    int i;
    long *created_ids = _stack_malloc(sizeof(long) * count);
    if (!created_ids) {
        serbuff_destroy(buff, 0);
        app_out_of_memory_void();
        return STACK_NO_OBJECT;
    }
    
    /* unserialize the widgets */
    _stack_begin(in_stack, STACK_NO_FLAGS);
    int err = SQLITE_OK;
    sqlite3_stmt *stmt;
    for (i = 0; i < count; i++)
    {
        long widget_id;
        if (! in_only_content)
        {
            /* determine new widget ID */
            long proposed_widget_id = serbuff_read_long(buff);
            err = sqlite3_prepare_v2(in_stack->db, "SELECT widgetid FROM widget WHERE widgetid=?1", -1, &stmt, NULL);
            if (err != SQLITE_OK) break;
            sqlite3_bind_int(stmt, 1, (int)proposed_widget_id);
            if (sqlite3_step(stmt) == SQLITE_ROW)
                proposed_widget_id = STACK_NO_OBJECT;
            sqlite3_finalize(stmt);
            
#if DEBUG
            //printf("UNSerializing proposed widget ID %ld\n", proposed_widget_id);
#endif
        
            /* create the widget */
            sqlite3_prepare_v2(in_stack->db, "INSERT INTO widget (widgetid,name,type,shared,dontsearch,x,y,"
                               "width,height,locked, cardid,bkgndid) VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10, ?11,?12)", -1, &stmt, NULL);
            if (proposed_widget_id == STACK_NO_OBJECT) sqlite3_bind_null(stmt, 1);
            else sqlite3_bind_int(stmt, 1, (int)proposed_widget_id);
            sqlite3_bind_text(stmt, 2, serbuff_read_cstr(buff), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, (int)serbuff_read_long(buff));
            sqlite3_bind_int(stmt, 4, (int)serbuff_read_long(buff));
            sqlite3_bind_int(stmt, 5, (int)serbuff_read_long(buff));
            sqlite3_bind_int(stmt, 6, (int)serbuff_read_long(buff));//x
            sqlite3_bind_int(stmt, 7, (int)serbuff_read_long(buff));//y
            sqlite3_bind_int(stmt, 8, (int)serbuff_read_long(buff));//w
            sqlite3_bind_int(stmt, 9, (int)serbuff_read_long(buff));//h
            sqlite3_bind_int(stmt, 10, (int)serbuff_read_long(buff));
            if (in_incl_content)
            {
                /* put back on the respective layers they were originally */
                sqlite3_bind_int(stmt, 11, (int)serbuff_read_long(buff));
                sqlite3_bind_int(stmt, 12, (int)serbuff_read_long(buff));
            }
            else
            {
                /* put all on the specified layer */
                serbuff_read_long(buff);
                serbuff_read_long(buff);
                sqlite3_bind_int(stmt, 11, (int)in_card_id);
                sqlite3_bind_int(stmt, 12, (int)in_bkgnd_id);
            }
            err = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (err != SQLITE_DONE) break;
            
            /* grab the new widget id */
            widget_id = sqlite3_last_insert_rowid(in_stack->db);
        }
        else
        {
            /* skip definition;
             we're just interested in the content */
            widget_id = serbuff_read_long(buff);
            
            serbuff_read_cstr(buff);// name
            serbuff_read_long(buff);
            serbuff_read_long(buff);
            serbuff_read_long(buff);
            serbuff_read_long(buff);//x
            serbuff_read_long(buff);//y
            serbuff_read_long(buff);//w
            serbuff_read_long(buff);//h
            serbuff_read_long(buff);
            serbuff_read_long(buff);
            serbuff_read_long(buff);
        }
        
        /* record the widget */
        created_ids[i] = widget_id;
        
        if (!in_only_content)
        {
            /* configure the widget */
            int o, option_count = (int)serbuff_read_long(buff);
            err = SQLITE_DONE;
            for (o = 0; o < option_count; o++)
            {
                sqlite3_prepare_v2(in_stack->db, "INSERT INTO widget_options (widgetid,optionid,value) "
                                   "VALUES (?1,?2,?3)", -1, &stmt, NULL);
                sqlite3_bind_int(stmt, 1, (int)widget_id);
                sqlite3_bind_int(stmt, 2, (int)serbuff_read_long(buff));
                sqlite3_bind_text(stmt, 3, serbuff_read_cstr(buff), -1, SQLITE_STATIC);
                err = sqlite3_step(stmt);
                sqlite3_finalize(stmt);
                if (err != SQLITE_DONE) break;
            }
            if (err != SQLITE_DONE) break;
        }
        else
        {
            /* skip configuration */
            int o, option_count = (int)serbuff_read_long(buff);
            for (o = 0; o < option_count; o++)
            {
                serbuff_read_long(buff);
                serbuff_read_cstr(buff);
            }
        }
        
        /* restore the widget content, if required */
        if (in_incl_content)
        {
            int content_count = (int)serbuff_read_long(buff);
            err = SQLITE_DONE;
            for (int co = 0; co < content_count; co++)
            {
                sqlite3_prepare_v2(in_stack->db, "INSERT INTO widget_content (widgetid,cardid,bkgndid,searchable,formatted) "
                                   "VALUES (?1,?2,?3,?4,?5)", -1, &stmt, NULL);
                sqlite3_bind_int(stmt, 1, (int)widget_id);
                sqlite3_bind_int(stmt, 2, (int)serbuff_read_long(buff));
                sqlite3_bind_int(stmt, 3, (int)serbuff_read_long(buff));
                sqlite3_bind_text(stmt, 4, serbuff_read_cstr(buff), -1, SQLITE_STATIC);
                void *data;
                long size;
                size = serbuff_read_data(buff, &data);
                sqlite3_bind_blob(stmt, 5, data, (int)size, SQLITE_TRANSIENT);
                err = sqlite3_step(stmt);
                sqlite3_finalize(stmt);
                if (err != SQLITE_DONE) break;
            }
            if (err != SQLITE_DONE) break;
        } /* if (in_incl_content) */
        else serbuff_read_long(buff);
        
        if (!in_only_content)
        {
            /* add the widget to the card/bkgnd widget sequence */
            if (!in_incl_content)
            {
                IDTable *widget_seq = _stack_widget_seq_get(in_stack, in_card_id, in_bkgnd_id);
                idtable_append(widget_seq, widget_id);
                _stack_widget_seq_set(in_stack, in_card_id, in_bkgnd_id, widget_seq);
            }
        }
    }
    
    /* check for errors in the creation of any widget;
     if one occurs, rollback the file and abort */
    if ((err != SQLITE_OK) && (err != SQLITE_DONE))
    {
        _stack_widget_seq_cache_load(in_stack, in_card_id, in_bkgnd_id);
        serbuff_destroy(buff, 0);
        _stack_free(created_ids);
        _stack_cancel(in_stack);
        return STACK_NO_OBJECT;
    }
    
    /* update the card/bkgnd widget sequence */
    if (in_incl_content)
    {
        char const *seq_data = serbuff_read_cstr(buff);
        if (seq_data && (seq_data[0]))
        {
            IDTable *widget_seq = idtable_create_with_ascii(in_stack, seq_data);
            if (widget_seq)
            {
                _stack_widget_seq_set(in_stack, in_card_id, 0, widget_seq);
                idtable_destroy(widget_seq);
            }
            else
            {
                _stack_widget_seq_cache_load(in_stack, in_card_id, in_bkgnd_id);
                serbuff_destroy(buff, 0);
                _stack_free(created_ids);
                _stack_cancel(in_stack);
                return STACK_NO_OBJECT;
            }
        }
        
        if (!in_only_content)
        {
            seq_data = serbuff_read_cstr(buff);
            if (seq_data && (seq_data[0]))
            {
                IDTable *widget_seq = idtable_create_with_ascii(in_stack, seq_data);
                if (widget_seq)
                {
                    _stack_widget_seq_set(in_stack, 0, in_bkgnd_id, widget_seq);
                    idtable_destroy(widget_seq);
                }
                else
                {
                    _stack_widget_seq_cache_load(in_stack, in_card_id, in_bkgnd_id);
                    serbuff_destroy(buff, 0);
                    _stack_free(created_ids);
                    _stack_cancel(in_stack);
                    return STACK_NO_OBJECT;
                }
            }
        } /* if (!in_only_content) */
    }
    
    /* save changes to disk */
    err = _stack_commit(in_stack);
    if (err != SQLITE_OK)
    {
        _stack_widget_seq_cache_load(in_stack, in_card_id, in_bkgnd_id);
        serbuff_destroy(buff, 0);
        _stack_free(created_ids);
        _stack_cancel(in_stack);
        return STACK_NO_OBJECT;
    }
    
    if (in_undoable)
    {
        /* record the undo steps */
        for (int i = 0; i < count; i++)
        {
            SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
            if (undo_data)
            {
                serbuff_write_long(undo_data, created_ids[i]);
                //serbuff_write_long(undo_data, idtable_size(widget_list));
                _undo_record_step(in_stack, UNDO_WIDGET_CREATE, undo_data);
            }
            else
            {
                /* if we fail to record the undo steps for any reason,
                 ensure the undo stack is flushed */
                stack_undo_flush(in_stack);
                break;
            }
        }
    }
    
    /* cleanup and return the new widget IDs */
    if (out_ids) {
        if (in_stack->ids_result) _stack_free(in_stack->ids_result);
        *out_ids = in_stack->ids_result = created_ids;
    }
    else _stack_free(created_ids);
    serbuff_destroy(buff, 0);
    return count;
}



/**********
 Cards
 */

/*
 *  _card_serialize
 *  ---------------------------------------------------------------------------------------------
 *  Creates a block of data representative of the specified card ID.
 *
 *  Returns the size of the data block if successful, or zero (0) on error.
 *  out_data is set to NULL if there's a problem.
 *
 *  Function retains ownership of the data.  Caller must copy if they wish to make changes.
 *  Data will remain available until the next call or the stack is closed.
 *
 *  Relies upon _widgets_serialize().
 */

long _card_serialize(Stack *in_stack, long in_card_id, void const **out_data)
{
    assert(in_stack != NULL);
    assert(in_card_id > 0);
    assert(out_data != NULL);
    
    /* in case of error, adhere to our contract with the caller */
    *out_data = NULL;
    
    /* create a serialization buffer to help build the result;
     and cleanup the last one (if any) */
    if (in_stack->serializer_card) serbuff_destroy(in_stack->serializer_card, 1);
    in_stack->serializer_card = serbuff_create(in_stack, NULL, 0, 0);
    if (!in_stack->serializer_card) return 0;
    
    /* serialize the card particulars */
    sqlite3_stmt *stmt;
    int err;
    sqlite3_prepare_v2(in_stack->db, "SELECT bkgndid,widgets,name,cantdelete,dontsearch "
                       "FROM card WHERE cardid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_id);
    err = sqlite3_step(stmt);
    
#if DEBUG
    //printf("Serializing card ID %ld\n", in_card_id);
#endif
    
    serbuff_write_long(in_stack->serializer_card, in_card_id);
    serbuff_write_long(in_stack->serializer_card, sqlite3_column_int(stmt, 0));
    serbuff_write_cstr(in_stack->serializer_card, (char*)sqlite3_column_text(stmt, 1));
    serbuff_write_cstr(in_stack->serializer_card, (char*)sqlite3_column_text(stmt, 2));
    serbuff_write_long(in_stack->serializer_card, sqlite3_column_int(stmt, 3));
    serbuff_write_long(in_stack->serializer_card, sqlite3_column_int(stmt, 4));
    
    sqlite3_finalize(stmt);
    if (err != SQLITE_ROW) return 0;
    
    assert(in_stack->stack_card_table != NULL);
    serbuff_write_long(in_stack->serializer_card, idtable_index_for_id(in_stack->stack_card_table, in_card_id));
    
    /* get the background */
    long bkgnd_id = _cards_bkgnd(in_stack, in_card_id);
    assert(bkgnd_id > 0);
    
    /* serialize the card widgets */
    sqlite3_prepare_v2(in_stack->db, "SELECT COUNT(*) FROM widget WHERE cardid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_id);
    err = sqlite3_step(stmt);
    int widget_count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (err != SQLITE_ROW) return 0;
    
    long *widget_ids = _stack_malloc(sizeof(long) * widget_count);
    if (widget_count && (!widget_ids)) app_out_of_memory_void();
    if (widget_count && widget_ids)
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT widgetid FROM widget WHERE cardid=?1", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_card_id);
        int i = 0;
        while ((err = sqlite3_step(stmt)) == SQLITE_ROW)
            widget_ids[i++] = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (err != SQLITE_DONE)
        {
            if (widget_ids) _stack_free(widget_ids);
            return 0;
        }
        
        long widget_data_size;
        void *widget_data;
        widget_data_size = _widgets_serialize(in_stack, widget_ids, widget_count, &widget_data, STACK_YES);
        serbuff_write_data(in_stack->serializer_card, widget_data, widget_data_size);
    }
    else
        serbuff_write_data(in_stack->serializer_card, "", 0);
    if (widget_ids) _stack_free(widget_ids);
    
    
    /* serialize background GUID;
     which is used to allow smart pasting of cards between stacks into backgrounds that are equivalent;
     GUID is rolled over each time a background's layout changes */
    /* to do in a later version */
    
    
    /* serialize the background widgets */
    sqlite3_prepare_v2(in_stack->db, "SELECT COUNT(*) FROM widget WHERE bkgndid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)bkgnd_id);
    err = sqlite3_step(stmt);
    widget_count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (err != SQLITE_ROW) return 0;
    
    widget_ids = _stack_malloc(sizeof(long) * widget_count);
    if (widget_count && (!widget_ids)) app_out_of_memory_void();
    if (widget_count && widget_ids)
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT widgetid FROM widget WHERE bkgndid=?1", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)bkgnd_id);
        int i = 0;
        while ((err = sqlite3_step(stmt)) == SQLITE_ROW)
            widget_ids[i++] = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (err != SQLITE_DONE)
        {
            if (widget_ids) _stack_free(widget_ids);
            return 0;
        }
        
        long widget_data_size;
        void *widget_data;
        widget_data_size = _widgets_serialize(in_stack, widget_ids, widget_count, &widget_data, STACK_YES);
        serbuff_write_data(in_stack->serializer_card, widget_data, widget_data_size);
    }
    else
        serbuff_write_data(in_stack->serializer_card, "", 0);
    if (widget_ids) _stack_free(widget_ids);
    
    /* return the size of the buffer, and the buffer content */
    return serbuff_data(in_stack->serializer_card, (void**)out_data);
}


/*
 *  _card_unserialize
 *  ---------------------------------------------------------------------------------------------
 *  Creates a card in the stack, using the data previously generated by _card_serialize().
 *
 *  Returns the ID of the new card if successful, or STACK_NO_OBJECT on error.
 *
 *  If <in_after_card_id> is not STACK_NO_OBJECT, the card is inserted immediately after 
 *  the specified card ID.
 *
 *  Attempts to reuse the card ID of the card when it was serialized.  However, if the card ID is
 *  no longer available (as may be the case during a pasteboard operation) a completely new card
 *  ID will be generated.
 *
 *  Caller retains ownership of the data.
 *
 *  Relies upon _widgets_unserialize().
 */

long _card_unserialize(Stack *in_stack, void const *in_data, long in_size, long in_after_card_id)
{
    assert(in_stack != NULL);
    
    /* check we don't have too many cards in the stack */
    if (stack_card_count(in_stack) >= _STACK_CARD_LIMIT) return STACK_NO_OBJECT;
    
    /* create a serialization buffer to help unserialize the card */
    SerBuff *buff = serbuff_create(in_stack, (void*)in_data, in_size, STACK_NO_COPY);
    if (!buff) return STACK_NO_OBJECT;
    
    /* check if the serialized card ID is currently in use */
    sqlite3_stmt *stmt;
    int err;
    err = sqlite3_prepare_v2(in_stack->db, "SELECT cardid FROM card WHERE cardid=?1", -1, &stmt, NULL);
    assert(err == SQLITE_OK);
    long proposed_card_id = serbuff_read_long(buff);
    sqlite3_bind_int(stmt, 1, (int)proposed_card_id);
    if (sqlite3_step(stmt) == SQLITE_ROW)
        proposed_card_id = 0; /* we don't care about errors as we're just checking to see it exists */
    sqlite3_finalize(stmt);
    
    _stack_begin(in_stack, STACK_NO_FLAGS);
    
#if DEBUG
    //printf("UNSerializing proposed card ID %ld\n", proposed_card_id);
#endif
    
    /* unserialize the card and particulars */
    sqlite3_prepare_v2(in_stack->db, "INSERT INTO card (cardid,bkgndid,widgets,name,cantdelete,dontsearch)"
                       " VALUES (?1,?2,?3,?4,?5,?6)", -1, &stmt, NULL);
    if (proposed_card_id == 0)
        sqlite3_bind_null(stmt, 1); /* generate a completely new card ID */
    else
        sqlite3_bind_int(stmt, 1, (int)proposed_card_id); /* use the existing card ID */
    sqlite3_bind_int(stmt, 2, (int)serbuff_read_long(buff));
    sqlite3_bind_text(stmt, 3, serbuff_read_cstr(buff), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, serbuff_read_cstr(buff), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, (int)serbuff_read_long(buff));
    sqlite3_bind_int(stmt, 6, (int)serbuff_read_long(buff));
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE) {
        serbuff_destroy(buff, 0);
        _stack_cancel(in_stack);
        return STACK_NO_OBJECT;
    }
    long card_id = sqlite3_last_insert_rowid(in_stack->db);
    
    /* determine sequence for new card */
    assert(in_stack->stack_card_table != NULL);
    long proposed_sequence = serbuff_read_long(buff);
    if (in_after_card_id != STACK_NO_OBJECT)
    {
        proposed_sequence = idtable_index_for_id(in_stack->stack_card_table, in_after_card_id);
        if (proposed_sequence < 0)
        {
            serbuff_destroy(buff, 0);
            _stack_cancel(in_stack);
            return STACK_NO_OBJECT;
        }
        proposed_sequence++;
    }
    
    /* add to stack's card changes list */
    sqlite3_prepare_v2(in_stack->db, "INSERT INTO stack_card_changes (cardid, deleted, sequence)"
                       " VALUES (?1, 0, ?2)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)card_id);
    sqlite3_bind_int(stmt, 2, (int)proposed_sequence);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
    {
        serbuff_destroy(buff, 0);
        _stack_cancel(in_stack);
        return STACK_NO_OBJECT;
    }
    
    /* unserialize the card widgets */
    long widget_data_size;
    void *widget_data;
    widget_data_size = serbuff_read_data(buff, &widget_data);
    if (widget_data_size)
    {
        err = _widgets_unserialize(in_stack, widget_data, widget_data_size, card_id, 0, NULL, 1, STACK_NO, STACK_NO);
        if (err < 0)
        {
            serbuff_destroy(buff, 0);
            _stack_cancel(in_stack);
            return STACK_NO_OBJECT;
        }
    }

    
    /* unserialize background GUID;
     which is used to allow smart pasting of cards between stacks into backgrounds that are equivalent;
     GUID is rolled over each time a background's layout changes */
    /* to do in a later version */
    
    /* is there a background in this stack with an identical GUID?
     if there is, we should just unserialize the background widget's content,
     otherwise, we should unserialize complete background widgets
     into a new background with an identical GUID to the one supplied in the data */
    
    
    /* unserialize the bkgnd widgets;
     JUST the content (in this version) */
    widget_data_size = serbuff_read_data(buff, &widget_data);
    if (widget_data_size)
    {
        err = _widgets_unserialize(in_stack, widget_data, widget_data_size, card_id, 0, NULL, 1, STACK_NO, STACK_YES);
        if (err < 0)
        {
            serbuff_destroy(buff, 0);
            _stack_cancel(in_stack);
            return STACK_NO_OBJECT;
        }
    }
    
    
    /* save changes to disk */
    err = _stack_commit(in_stack);
    if (err != SQLITE_OK)
    {
        serbuff_destroy(buff, 0);
        _stack_cancel(in_stack);
        return STACK_NO_OBJECT;
    }
    
    /* insert new card into stack */
    idtable_insert(in_stack->stack_card_table, card_id, proposed_sequence);
    
    /* record the undo step */
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    if (undo_data)
    {
        serbuff_write_long(undo_data, card_id);
        _undo_record_step(in_stack, UNDO_CARD_CREATE, undo_data);
    }
    
    /* cleanup and return the new card ID */
    serbuff_destroy(buff, 0);
    return card_id;
}


