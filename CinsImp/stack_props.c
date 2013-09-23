/*
 
 Stack Object Properties
 stack_props.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Standardised accessors and mutators for object properties; stack, background, card and widget, 
 along with a handful of ad-hoc mechanisms for stack window and card sizes.
 
 *************************************************************************************************
 */

#include "stack_int.h"




void stack_get_window_size(Stack *in_stack, long *out_width, long *out_height)
{
    sqlite3_stmt *stmt;
    
    int err = sqlite3_prepare_v2(in_stack->db,
                                 "SELECT windowwidth, windowheight FROM stack",
                                 -1, &stmt, NULL);
    if (err != SQLITE_OK) return;
    
    err = sqlite3_step(stmt);
    if (err != SQLITE_ROW) return;
    
    *out_width = sqlite3_column_int(stmt, 0);
    *out_height = sqlite3_column_int(stmt, 1);
    
    sqlite3_finalize(stmt);
}


void stack_set_window_size(Stack *in_stack, long in_width, long in_height)
{
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(in_stack->db, "UPDATE stack SET windowwidth=?1, windowheight=?2", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_width);
    sqlite3_bind_int(stmt, 2, (int)in_height);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}


void stack_get_card_size(Stack *in_stack, long *out_width, long *out_height)
{
    sqlite3_stmt *stmt;
    
    int err = sqlite3_prepare_v2(in_stack->db,
                                 "SELECT cardwidth, cardheight FROM stack",
                                 -1, &stmt, NULL);
    if (err != SQLITE_OK)
    {
        *out_width = 512;
        *out_height = 342;
        return;
    }
    
    err = sqlite3_step(stmt);
    if (err != SQLITE_ROW) return;
    
    *out_width = sqlite3_column_int(stmt, 0);
    *out_height = sqlite3_column_int(stmt, 1);
    
    sqlite3_finalize(stmt);
    
}


void stack_set_card_size(Stack *in_stack, long in_width, long in_height)
{
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(in_stack->db, "UPDATE stack SET cardwidth=?1, cardheight=?2", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_width);
    sqlite3_bind_int(stmt, 2, (int)in_height);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}






const char* stack_widget_prop_get_string(Stack *in_stack, long in_widget_id, long in_card_id, enum Property in_prop)
{
    sqlite3_stmt *stmt;
    
    if (in_stack->widget_result) _stack_free(in_stack->widget_result);
    in_stack->widget_result = NULL;
    
    if (in_prop == PROPERTY_NAME)
    {
        sqlite3_prepare_v2(in_stack->db,
                           "SELECT name FROM widget WHERE widgetid=?1",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_step(stmt);
        in_stack->widget_result = _stack_clone_cstr( (char*)sqlite3_column_text(stmt, 0) );
        sqlite3_finalize(stmt);
    }
    else if (in_prop == PROPERTY_STYLE)
    {
        sqlite3_prepare_v2(in_stack->db,
                           "SELECT type FROM widget WHERE widgetid=?1",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_step(stmt);
        switch (sqlite3_column_int(stmt, 0))
        {
            case WIDGET_BUTTON_PUSH:
                in_stack->widget_result = _stack_clone_cstr("push");
                break;
            case WIDGET_BUTTON_TRANSPARENT:
                in_stack->widget_result = _stack_clone_cstr("transparent");
                break;
            case WIDGET_FIELD_TEXT:
                in_stack->widget_result = _stack_clone_cstr("text");
                break;
            case WIDGET_FIELD_CHECK:
                in_stack->widget_result = _stack_clone_cstr("checkbox");
                break;
            case WIDGET_FIELD_PICKLIST:
                in_stack->widget_result = _stack_clone_cstr("picklist");
                break;
            case WIDGET_FIELD_GRID:
                in_stack->widget_result = _stack_clone_cstr("grid");
                break;
        }
        sqlite3_finalize(stmt);
    }
    else if (in_prop == PROPERTY_CONTENT)
    {
        char *searchable;
        stack_widget_content_get(in_stack, in_widget_id, in_card_id, 0, &searchable, NULL, NULL, NULL);
        return searchable;
    }
    else
    {
        sqlite3_prepare_v2(in_stack->db,
                           "SELECT value FROM widget_options WHERE widgetid=?1 AND optionid=?2",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_bind_int(stmt, 2, (int)in_prop);
        sqlite3_step(stmt);
        in_stack->widget_result = _stack_clone_cstr( (char*)sqlite3_column_text(stmt, 0) );
        sqlite3_finalize(stmt);
    }
    
    if (!in_stack->widget_result) in_stack->widget_result = _stack_clone_cstr("");
    return in_stack->widget_result;
}

/*  bools
 
 case PROPERTY_LOCKED:
 
 break;
 case PROPERTY_DONTSEARCH:
 
 break;
 case PROPERTY_SHARED:
 
 break;
 
 */


void stack_widget_prop_set_string(Stack *in_stack, long in_widget_id, long in_card_id, enum Property in_prop, char *in_string)
{
    sqlite3_stmt *stmt;
    
    /* record the undo step */
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    serbuff_write_long(undo_data, in_widget_id);
    serbuff_write_long(undo_data, in_card_id);
    serbuff_write_long(undo_data, in_prop);
    serbuff_write_cstr(undo_data, stack_widget_prop_get_string(in_stack, in_widget_id, in_card_id, in_prop));
    _undo_record_step(in_stack, UNDO_WIDGET_PROPERTY_STRING, undo_data);
    
    if (in_prop == PROPERTY_NAME)
    {
        sqlite3_prepare_v2(in_stack->db,
                           "UPDATE widget SET name=?2 WHERE widgetid=?1",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_bind_text(stmt, 2, in_string, -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    else if (in_prop == PROPERTY_STYLE)
    {
        sqlite3_prepare_v2(in_stack->db,
                           "UPDATE widget SET type=?2 WHERE widgetid=?1",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        if (_stack_str_same(in_string, "push"))
            sqlite3_bind_int(stmt, 2, (int)WIDGET_BUTTON_PUSH);
        else if (_stack_str_same(in_string, "transparent"))
            sqlite3_bind_int(stmt, 2, (int)WIDGET_BUTTON_TRANSPARENT);
        else if (_stack_str_same(in_string, "text"))
            sqlite3_bind_int(stmt, 2, (int)WIDGET_FIELD_TEXT);
        else if (_stack_str_same(in_string, "checkbox") || _stack_str_same(in_string, "check box"))
            sqlite3_bind_int(stmt, 2, (int)WIDGET_FIELD_CHECK);
        else if (_stack_str_same(in_string, "picklist"))
            sqlite3_bind_int(stmt, 2, (int)WIDGET_FIELD_PICKLIST);
        else if (_stack_str_same(in_string, "grid"))
            sqlite3_bind_int(stmt, 2, (int)WIDGET_FIELD_GRID);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    else if (in_prop == PROPERTY_CONTENT)
    {
        stack_widget_content_set(in_stack, in_widget_id, in_card_id, in_string, NULL, 0);
    }
    else
    {
        sqlite3_prepare_v2(in_stack->db,
                           "UPDATE widget_options SET value=?3 WHERE widgetid=?1 AND optionid=?2",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_bind_int(stmt, 2, (int)in_prop);
        sqlite3_bind_text(stmt, 3, in_string, -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (sqlite3_changes(in_stack->db) == 0)
        {
            sqlite3_prepare_v2(in_stack->db,
                               "INSERT INTO widget_options VALUES (?1, ?2, ?3)",
                               -1, &stmt, NULL);
            sqlite3_bind_int(stmt, 1, (int)in_widget_id);
            sqlite3_bind_int(stmt, 2, (int)in_prop);
            sqlite3_bind_text(stmt, 3, in_string, -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
}


long stack_widget_prop_get_long(Stack *in_stack, long in_widget_id, long in_card_id, enum Property in_prop)
{
    long result;
    sqlite3_stmt *stmt = NULL;
    
    if ((in_prop == PROPERTY_LOCKED) || (in_prop == PROPERTY_DONTSEARCH) || (in_prop == PROPERTY_SHARED)
         || (in_prop == PROPERTY_ICON))
    {
        switch (in_prop)
        {
            case PROPERTY_LOCKED:
                sqlite3_prepare_v2(in_stack->db,
                                   "SELECT locked FROM widget WHERE widgetid=?1",
                                   -1, &stmt, NULL);
                break;
            case PROPERTY_DONTSEARCH:
                sqlite3_prepare_v2(in_stack->db,
                                   "SELECT dontsearch FROM widget WHERE widgetid=?1",
                                   -1, &stmt, NULL);
                break;
            case PROPERTY_SHARED:
                sqlite3_prepare_v2(in_stack->db,
                                   "SELECT shared FROM widget WHERE widgetid=?1",
                                   -1, &stmt, NULL);
                break;
            case PROPERTY_ICON:
                sqlite3_prepare_v2(in_stack->db,
                                   "SELECT iconid FROM widget WHERE widgetid=?1",
                                   -1, &stmt, NULL);
                break;
            default:
                break;
        }
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_step(stmt);
        result = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return result;
    }
    
    sqlite3_prepare_v2(in_stack->db,
                       "SELECT value FROM widget_options WHERE widgetid=?1 AND optionid=?2",
                       -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    sqlite3_bind_int(stmt, 2, (int)in_prop);
    sqlite3_step(stmt);
    result = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return result;
}


void stack_widget_prop_set_long(Stack *in_stack, long in_widget_id, long in_card_id, enum Property in_prop, long in_long)
{
    sqlite3_stmt *stmt = NULL;
    
    /* record the undo step */
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    serbuff_write_long(undo_data, in_widget_id);
    serbuff_write_long(undo_data, in_card_id);
    serbuff_write_long(undo_data, in_prop);
    serbuff_write_long(undo_data, stack_widget_prop_get_long(in_stack, in_widget_id, in_card_id, in_prop));
    _undo_record_step(in_stack, UNDO_WIDGET_PROPERTY_LONG, undo_data);
    
    if ((in_prop == PROPERTY_LOCKED) || (in_prop == PROPERTY_DONTSEARCH) || (in_prop == PROPERTY_SHARED) ||
         (in_prop == PROPERTY_ICON))
    {
        switch (in_prop)
        {
            case PROPERTY_LOCKED:
                sqlite3_prepare_v2(in_stack->db,
                                   "UPDATE widget SET locked=?2 WHERE widgetid=?1",
                                   -1, &stmt, NULL);
                break;
            case PROPERTY_DONTSEARCH:
                sqlite3_prepare_v2(in_stack->db,
                                   "UPDATE widget SET dontsearch=?2 WHERE widgetid=?1",
                                   -1, &stmt, NULL);
                break;
            case PROPERTY_SHARED:
                sqlite3_prepare_v2(in_stack->db,
                                   "UPDATE widget SET shared=?2 WHERE widgetid=?1",
                                   -1, &stmt, NULL);
                break;
            case PROPERTY_ICON:
                sqlite3_prepare_v2(in_stack->db,
                                   "UPDATE widget SET iconid=?2 WHERE widgetid=?1",
                                   -1, &stmt, NULL);
                break;
            default:
                break;
        }
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_bind_int(stmt, 2, (int)in_long);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return;
    }
    
    sqlite3_prepare_v2(in_stack->db,
                       "UPDATE widget_options SET value=?3 WHERE widgetid=?1 AND optionid=?2",
                       -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    sqlite3_bind_int(stmt, 2, (int)in_prop);
    sqlite3_bind_int(stmt, 3, (int)in_long);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (sqlite3_changes(in_stack->db) == 0)
    {
        sqlite3_prepare_v2(in_stack->db,
                           "INSERT INTO widget_options VALUES (?1, ?2, ?3)",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_bind_int(stmt, 2, (int)in_prop);
        sqlite3_bind_int(stmt, 3, (int)in_long);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

//PROPERTY_PASSWORD,

long stack_prop_get_long(Stack *in_stack, long in_card_id, long in_bkgnd_id, enum Property in_prop)
{
    char sql[1024];
    const char* fld = "";
    sqlite3_stmt *stmt;
    long result = 0;
    
    switch (in_prop)
    {
        case PROPERTY_LOCKED:
            fld = "locked";
            if ((in_card_id == 0) && (in_bkgnd_id == 0) && (in_stack->readonly)) return 1;
            break;
        case PROPERTY_CANTPEEK: fld = "cantpeek"; break;
        case PROPERTY_CANTDELETE: fld = "cantdelete"; break;
        case PROPERTY_CANTABORT: fld = "cantabort"; break;
        case PROPERTY_PRIVATE: fld = "private"; break;
        case PROPERTY_USERLEVEL: fld = "userlevellimit"; break;
        case PROPERTY_DONTSEARCH: fld = "dontsearch"; break;
        default: break;
    }
    
    if ((in_card_id == 0) && (in_bkgnd_id == 0))
        sprintf(sql, "SELECT %s FROM stack", fld);
    else if (in_card_id > 0)
        sprintf(sql, "SELECT %s FROM card WHERE cardid=%ld", fld, in_card_id);
    else
        sprintf(sql, "SELECT %s FROM bkgnd WHERE bkgndid=%ld", fld, in_bkgnd_id);
    
    sqlite3_prepare_v2(in_stack->db, sql, -1, &stmt, NULL);
    sqlite3_step(stmt);
    result = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    return result;
}

void stack_prop_set_long(Stack *in_stack, long in_card_id, long in_bkgnd_id, enum Property in_prop, long in_long)
{
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    serbuff_write_long(undo_data, in_card_id);
    serbuff_write_long(undo_data, in_bkgnd_id);
    serbuff_write_long(undo_data, in_prop);
    serbuff_write_long(undo_data, stack_prop_get_long(in_stack, in_card_id, in_bkgnd_id, in_prop));
    _undo_record_step(in_stack, UNDO_PROPERTY_LONG, undo_data);
    
    char sql[1024];
    const char* fld = "";
    sqlite3_stmt *stmt;
    
    switch (in_prop)
    {
        case PROPERTY_LOCKED: fld = "locked"; break;
        case PROPERTY_CANTPEEK: fld = "cantpeek"; break;
        case PROPERTY_CANTDELETE: fld = "cantdelete"; break;
        case PROPERTY_CANTABORT: fld = "cantabort"; break;
        case PROPERTY_PRIVATE: fld = "private"; break;
        case PROPERTY_USERLEVEL: fld = "userlevellimit"; break;
        case PROPERTY_DONTSEARCH: fld = "dontsearch"; break;
        default: break;
    }
    
    if ((in_card_id == 0) && (in_bkgnd_id == 0))
        sprintf(sql, "UPDATE stack SET %s=%ld", fld, in_long);
    else if (in_card_id > 0)
        sprintf(sql, "UPDATE card SET %s=%ld WHERE cardid=%ld", fld, in_long, in_card_id);
    else
        sprintf(sql, "UPDATE bkgnd SET %s=%ld WHERE bkgndid=%ld", fld, in_long, in_bkgnd_id);
    
    sqlite3_prepare_v2(in_stack->db, sql, -1, &stmt, NULL);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    /* cache Can't Modify */
    if (in_prop == PROPERTY_LOCKED)
        in_stack->soft_lock = (int)in_long;
}


const char* stack_prop_get_string(Stack *in_stack, long in_card_id, long in_bkgnd_id, enum Property in_prop)
{
    if (!in_stack) return "";
    
    char sql[1024];
    const char* fld = "";
    sqlite3_stmt *stmt;
    
    if (in_stack->result) _stack_free(in_stack->result);
    in_stack->result = NULL;
    
    switch (in_prop)
    {
        case PROPERTY_PASSWORD: fld = "passwordhash"; break;
        case PROPERTY_NAME: fld = "name"; break;
        default: break;
    }
    
    if ((in_card_id == 0) && (in_bkgnd_id == 0))
        sprintf(sql, "SELECT %s FROM stack", fld);
    else if (in_card_id > 0)
        sprintf(sql, "SELECT %s FROM card WHERE cardid=%ld", fld, in_card_id);
    else
        sprintf(sql, "SELECT %s FROM bkgnd WHERE bkgndid=%ld", fld, in_bkgnd_id);
    
    sqlite3_prepare_v2(in_stack->db, sql, -1, &stmt, NULL);
    sqlite3_step(stmt);
    in_stack->result = _stack_clone_cstr((char*)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    
    return in_stack->result;
}


void stack_prop_set_string(Stack *in_stack, long in_card_id, long in_bkgnd_id, enum Property in_prop, char* in_string)
{
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    serbuff_write_long(undo_data, in_card_id);
    serbuff_write_long(undo_data, in_bkgnd_id);
    serbuff_write_long(undo_data, in_prop);
    serbuff_write_cstr(undo_data, stack_prop_get_string(in_stack, in_card_id, in_bkgnd_id, in_prop));
    _undo_record_step(in_stack, UNDO_PROPERTY_STRING, undo_data);
    
    char sql[1024];
    const char* fld = "";
    sqlite3_stmt *stmt;
    
    switch (in_prop)
    {
        case PROPERTY_PASSWORD: fld = "passwordhash"; break;
        case PROPERTY_NAME: fld = "name"; break;
        default: break;
    }
    
    if ((in_card_id == 0) && (in_bkgnd_id == 0))
        sprintf(sql, "UPDATE stack SET %s=?1", fld);
    else if (in_card_id > 0)
        sprintf(sql, "UPDATE card SET %s=?1 WHERE cardid=%ld", fld, in_card_id);
    else
        sprintf(sql, "UPDATE bkgnd SET %s=?1 WHERE bkgndid=%ld", fld, in_bkgnd_id);
    
    sqlite3_prepare_v2(in_stack->db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, in_string, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}





