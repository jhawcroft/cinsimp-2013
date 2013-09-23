/*
 
 Stack Layer Pictures
 stack_lpic.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Pictures for card and background layers
 
 *************************************************************************************************
 */

#include "stack_int.h"


/**********
 Public API
 */

/*
 *  stack_layer_picture_get
 *  ---------------------------------------------------------------------------------------------
 *  
 */

long stack_layer_picture_get(Stack *in_stack, long in_card_id, long in_bkgnd_id, void **out_data, int *out_visible)
{
    assert(in_stack != NULL);
    assert((in_card_id > 0) || (in_bkgnd_id > 0));
    assert((in_card_id < 1) || (in_bkgnd_id < 1));
    assert(out_data != NULL);
    assert(out_visible != NULL);
    
    /* assume the worst */
    *out_data = NULL;
    *out_visible = STACK_NO;
    
    /* cleanup old static data from previous call */
    if (in_stack->picture_data) _stack_free(in_stack->picture_data);
    in_stack->picture_data = NULL;
    long data_size = 0;
    
    /* query the appropriate table */
    sqlite3_stmt *stmt;
    if (in_card_id > 0)
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT visible, data FROM picture_card WHERE cardid=?1", -1,
                           &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_card_id);
    }
    else
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT visible, data FROM picture_bkgnd WHERE bkgndid=?1", -1,
                           &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_bkgnd_id);
    }
    int err;
    err = sqlite3_step(stmt);
    
    /* copy the result (if any) */
    int visible = STACK_NO;
    if (err == SQLITE_ROW)
    {
        data_size = sqlite3_column_bytes(stmt, 1);
        in_stack->picture_data = _stack_malloc(data_size);
        if (!in_stack->picture_data)
        {
            data_size = 0;
            _stack_panic_void(in_stack, STACK_ERR_MEMORY);
        }
        else
        {
            memcpy(in_stack->picture_data, sqlite3_column_blob(stmt, 1), data_size);
            visible = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    if (err != SQLITE_ROW) return 0;
    
    /* return the requested picture */
    *out_visible = visible;
    *out_data = in_stack->picture_data;
    return data_size;
}


/*
 *  stack_layer_picture_set
 *  ---------------------------------------------------------------------------------------------
 *  
 */

void stack_layer_picture_set(Stack *in_stack, long in_card_id, long in_bkgnd_id, void *in_data, long in_size)
{
    assert(in_stack != NULL);
    assert((in_card_id > 0) || (in_bkgnd_id > 0));
    assert((in_card_id < 1) || (in_bkgnd_id < 1));
    
    /* check if there is any existing content;
     and prepare the undo step */
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    if (undo_data)
    {
        serbuff_write_long(undo_data, in_card_id);
        serbuff_write_long(undo_data, in_bkgnd_id);
    }
    sqlite3_stmt *stmt;
    int existing;
    if (in_card_id > 0)
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT data FROM picture_card WHERE cardid=?1", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_card_id);
    }
    else
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT data FROM picture_bkgnd WHERE bkgndid=?1", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_bkgnd_id);
    }
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        /* save the existing content */
        existing = STACK_YES;
        if (undo_data)
            serbuff_write_data(undo_data, (void*)sqlite3_column_blob(stmt, 0), sqlite3_column_bytes(stmt, 0));
    }
    else
    {
        existing = STACK_NO;
        if (undo_data)
            serbuff_write_data(undo_data, "", 0);
    }
    sqlite3_finalize(stmt);
    
    /* remove content if the graphic is empty */
    int err = SQLITE_DONE;
    if ((in_size == 0) || (in_data == NULL))
    {
        if (existing)
        {
            if (in_card_id > 0)
            {
                sqlite3_prepare_v2(in_stack->db, "DELETE FROM picture_card WHERE cardid=?1", -1, &stmt, NULL);
                sqlite3_bind_int(stmt, 1, (int)in_card_id);
            }
            else
            {
                sqlite3_prepare_v2(in_stack->db, "DELETE FROM picture_bkgnd WHERE bkgndid=?1", -1, &stmt, NULL);
                sqlite3_bind_int(stmt, 1, (int)in_bkgnd_id);
            }
            err = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        else
        {
            /* there is no content; do nothing! */
        }
    }
    
    /* update/insert new content */
    else
    {        
        if (existing)
        {
            if (in_card_id > 0)
            {
                sqlite3_prepare_v2(in_stack->db, "UPDATE picture_card SET visible=1,data=?2 WHERE cardid=?1", -1, &stmt, NULL);
                sqlite3_bind_int(stmt, 1, (int)in_card_id);
            }
            else
            {
                sqlite3_prepare_v2(in_stack->db, "UPDATE picture_bkgnd SET visible=1,data=?2 WHERE bkgndid=?1", -1, &stmt, NULL);
                sqlite3_bind_int(stmt, 1, (int)in_bkgnd_id);
            }
            sqlite3_bind_blob(stmt, 2, in_data, (int)in_size, SQLITE_STATIC);
            err = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        else
        {
            if (in_card_id > 0)
            {
                sqlite3_prepare_v2(in_stack->db, "INSERT INTO picture_card VALUES (?1, 1, ?2)", -1, &stmt, NULL);
                sqlite3_bind_int(stmt, 1, (int)in_card_id);
            }
            else
            {
                sqlite3_prepare_v2(in_stack->db, "INSERT INTO picture_bkgnd VALUES (?1, 1, ?2)", -1, &stmt, NULL);
                sqlite3_bind_int(stmt, 1, (int)in_bkgnd_id);
            }
            sqlite3_bind_blob(stmt, 2, in_data, (int)in_size, SQLITE_STATIC);
            err = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    if (err != SQLITE_DONE)
    {
        if (undo_data) serbuff_destroy(undo_data, STACK_YES);
        return _stack_panic_void(in_stack, STACK_ERR_IO);
    }
    
    /* record the undo step */
    if (undo_data) serbuff_destroy(undo_data, STACK_YES);
    /*if (undo_data)
        _undo_record_step(in_stack, UNDO_WIDGET_CONTENT, undo_data);*/
}








