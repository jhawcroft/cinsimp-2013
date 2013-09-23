/*
 
 Stack Widgets
 stack_widg.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Widget (button / field) layout and content
 
 !!  This code unit is critical to the integrity of the Stack file format.
 
 *************************************************************************************************
 
 Content
 -------------------------------------------------------------------------------------------------
 
 Widgets have two types of content: searchable and formatted.
 
 Searchable content is always a plain, UTF-8 encoded string.
 
 Formatted content is arbitrary binary data and should be used by the application either in 
 concert with or instead of the searchable content for display content to support fonts, sizes,
 colours, styles and other formatting along with the text (eg. RTF).
 
 When content is updated, the application must at minimum write a searchable string.  This string
 is used by the stack_find() and stack_sort() commands.
 
 
 Widget Existance
 -------------------------------------------------------------------------------------------------
 
 Widget lookups and counts always go through a card/background widget sequence table.  This is to
 ensure that the widget actually belongs to a specific layer.
 
 Regardless of the state of the database, the sequence table is the one and only way to determine
 if a widget exists on a layer.
 
 Unless a widget exists in a sequence table, it doesn't exist at all!  In this way, if corruption
 occurs, at least the behaviour of the application should be consistent.  Lookup functions should
 reliably return STACK_NO_OBJECT in case an ID is invalid.
 
 IO Errors
 -------------------------------------------------------------------------------------------------
 
 Calling many of the accessor/mutator functions for widgets that don't exist may cause a Stack
 IO error, STACK_ERR_IO.
 
 */

#include "stack_int.h"


/*********
 Internal Widget Management
 */

/*
 *  _split_widgets_by_layer
 *  ---------------------------------------------------------------------------------------------
 *  Takes an array of widget IDs and splits into two separate arrays, one for card widgets and
 *  one for background widgets.
 *
 *  Returns STACK_YES if the split was successful, or STACK_NO if it fails.
 *
 *  The widgets must all belong to the same card and/or background layer - otherwise this call will
 *  abort the split and return STACK_NO.
 *
 *  The outputs are unspecified if the function fails.
 *
 *  Called by _rearrange_widgets(), below.
 */

static int _split_widgets_by_layer(Stack *in_stack, long *in_widgets, int in_count,
                                    long **out_cd, int *out_cd_count,
                                    long **out_bg, int *out_bg_count)
{
    assert(in_stack != NULL);
    assert(in_widgets != NULL);
    assert(out_cd != NULL);
    assert(out_cd_count != NULL);
    assert(out_bg != NULL);
    assert(out_bg_count != NULL);
    
    /* make the arrays (sloppy allocation) */
    *out_cd_count = 0;
    *out_bg_count = 0;
    *out_bg = _stack_malloc(sizeof(long) * in_count);
    if (!*out_bg) return _stack_panic_false(in_stack, STACK_ERR_MEMORY);
    *out_cd = _stack_malloc(sizeof(long) * in_count);
    if (!*out_cd)
    {
        _stack_free(*out_bg);
        return _stack_panic_false(in_stack, STACK_ERR_MEMORY);
    }
    
    /* populate the card widgets array */
    long layerid = STACK_NO_OBJECT;
    for (int i = 0; i < in_count; i++)
    {
        /* check if the widget is a card widget */
        if (stack_widget_is_card(in_stack, in_widgets[i]))
        {
            /* track which layers the widgets belong to;
             they must only belong to one card layer */
            if (layerid == STACK_NO_OBJECT) layerid = _widget_layer_id(in_stack, in_widgets[i]);
            else if (layerid != _widget_layer_id(in_stack, in_widgets[i]))
            {
                _stack_free(*out_cd);
                _stack_free(*out_bg);
                return STACK_NO;
            }
            
            /* record the card ID in the card list */
            (*out_cd)[*out_cd_count] = in_widgets[i];
            *out_cd_count += 1;
        }
    }
    
    /* populate the bkgnd widgets array */
    for (int i = 0; i < in_count; i++)
    {
        /* check if the widget is a bkgnd widget */
        if (!stack_widget_is_card(in_stack, in_widgets[i]))
        {
            /* track which layers the widgets belong to;
             they must only belong to one bkgnd layer */
            if (layerid == STACK_NO_OBJECT) layerid = _widget_layer_id(in_stack, in_widgets[i]);
            else if (layerid != _widget_layer_id(in_stack, in_widgets[i]))
            {
                _stack_free(*out_cd);
                _stack_free(*out_bg);
                return STACK_NO;
            }
            
            /* record the bkgnd ID in the bkgnd list */
            (*out_bg)[*out_bg_count] = in_widgets[i];
            *out_bg_count += 1;
        }
    }
    
    return STACK_YES;
}


/*
 *  _rearrange_widgets
 *  ---------------------------------------------------------------------------------------------
 *  Resequences the specified widgets within their respective layers, using the supplied function.
 *  This function effectively changes the tab-sequence of fields.
 *
 *  The supplied list of widgets must all be on a single card and or background layer, otherwise
 *  this function will have no effect.
 *
 *  The sequence function must be one of those provided by idtable.c:
 *      idtable_send_front
 *      idtable_send_back
 *      idtable_shuffle_forward
 *      idtable_shuffle_backward
 *
 *  Card and background widgets can be supplied in any order and are sorted into their respective
 *  layer prior to resequencing.
 *
 *  This function forms the underlying implementation for the Send to Front, Send to Back, 
 *  Send Forward and Send Back commands in the user interface.
 */

static void _rearrange_widgets(Stack *in_stack, long *in_widgets, int in_count, void (*in_sequence_function)(IDTable*, long*, int))
{
    assert(in_stack != NULL);
    assert(in_widgets != NULL);
    assert(in_sequence_function != NULL);
    
    /* prepare undo record */
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    if (!undo_data) return _stack_panic_void(in_stack, STACK_ERR_MEMORY);
    
    /* split the widgets into their respective card and background layers;
     check that all widgets belong to at most one unique card and one unique background layer */
    long *cd, *bg, layer;
    int cd_count, bg_count;
    if (!_split_widgets_by_layer(in_stack, in_widgets, in_count, &cd, &cd_count, &bg, &bg_count))
    {
        serbuff_destroy(undo_data, STACK_YES);
        return;
    }
    
    /* resequence all the card widgets */
    IDTable *table;
    serbuff_write_long(undo_data, cd_count); /* write the count to the undo data */
    if (cd_count > 0)
    {
        /* get the card ID of the first widget */
        layer = _widget_layer_id(in_stack, cd[0]);
        
        /* get the sequence of all widgets on that card */
        table = _stack_widget_seq_get(in_stack, layer, 0);
        
        /* write the card ID and the entire sequence table to the undo data */
        serbuff_write_long(undo_data, layer);
        serbuff_write_cstr(undo_data, idtable_to_ascii(table));
        
        /* run the supplied idtable resequencing function */
        in_sequence_function(table, cd, cd_count);
        
        /* save the new widget sequence to disk */
        _stack_widget_seq_set(in_stack, layer, 0, table);
    }
    
    /* resequence all the background widgets */
    serbuff_write_long(undo_data, bg_count); /* write the count to the undo data */
    if (bg_count > 0)
    {
        /* get the background ID of the first widget */
        layer = _widget_layer_id(in_stack, bg[0]);
        
        /* get the sequence of all widgets on that bkgnd */
        table = _stack_widget_seq_get(in_stack, 0, layer);
        
        /* write the bkgnd ID and the entire sequence table to the undo data */
        serbuff_write_long(undo_data, layer);
        serbuff_write_cstr(undo_data, idtable_to_ascii(table));
        
        /* run the supplied idtable resequencing function */
        in_sequence_function(table, bg, bg_count);
        
        /* save the new widget sequence to disk */
        _stack_widget_seq_set(in_stack, 0, layer, table);
    }
    
    /* cleanup */
    _stack_free(cd);
    _stack_free(bg);
    
    /* lodge the undo record */
    _undo_record_step(in_stack, UNDO_REARRANGE_WIDGETS, undo_data);
}


/*
 *  stack_widget_owner
 *  ---------------------------------------------------------------------------------------------
 *  Returns the ID of the card or background parent of the specified widget ID, or
 *  STACK_NO_OBJECT if there is no such widget.
 *
 *  (Does not verify that the widget actually exists.)
 */

void stack_widget_owner(Stack *in_stack, long in_widget_id, long *out_card_id, long *out_bkgnd_id)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    assert(out_card_id != NULL);
    assert(out_bkgnd_id != NULL);
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "SELECT cardid,bkgndid FROM widget WHERE widgetid=?1",
                       -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        *out_card_id = sqlite3_column_int(stmt, 0);
        *out_bkgnd_id = sqlite3_column_int(stmt, 1);
    }
    else
    {
        *out_card_id = STACK_NO_OBJECT;
        *out_bkgnd_id = STACK_NO_OBJECT;
    }
    sqlite3_finalize(stmt);
    
    if (*out_card_id < 1) *out_card_id = STACK_NO_OBJECT;
    if (*out_bkgnd_id < 1) *out_bkgnd_id = STACK_NO_OBJECT;
}


/*
 *  _widget_layer_id
 *  ---------------------------------------------------------------------------------------------
 *  Returns the ID of the card or background parent of the specified widget ID, or
 *  STACK_NO_OBJECT if there is no such widget.
 *
 *  (Does not verify that the widget actually exists.)
 */

long _widget_layer_id(Stack *in_stack, long in_widget_id)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    
    long card_id, bkgnd_id;
    stack_widget_owner(in_stack, in_widget_id, &card_id, &bkgnd_id);
    if (card_id > 0) return card_id;
    else if (bkgnd_id > 0) return bkgnd_id;
    else return STACK_NO_OBJECT;
}


/*
 *  _widget_tabbable
 *  ---------------------------------------------------------------------------------------------
 *  Returns STACK_YES if the specified widget is tabbable/user editable.
 *
 *  If in_bkgnd is STACK_YES, ie. if the user is editing the background, only returns STACK_YES
 *  if the widget has editable Shared Text.
 *
 *  (Does not verify that the widget actually exists or is a field.)
 *
 *  In all other cases, returns STACK_NO.
 */

static int _widget_tabbable(Stack *in_stack, long in_widget_id, int in_bkgnd)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    assert((in_bkgnd == !0) || (in_bkgnd ==!!0));
    
    sqlite3_stmt *stmt;
    int tabbable = STACK_YES;
    sqlite3_prepare_v2(in_stack->db,
                       "SELECT shared,hidden,locked,type FROM widget WHERE widgetid=?1",
                       -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (in_bkgnd && (!sqlite3_column_int(stmt, 0))) tabbable = STACK_NO;
        if ((!in_bkgnd) && sqlite3_column_int(stmt, 0)) tabbable = STACK_NO;
        if (sqlite3_column_int(stmt, 1)) tabbable = STACK_NO;
        if (sqlite3_column_int(stmt, 2)) tabbable = STACK_NO;
    }
    else tabbable = STACK_NO;
    switch (sqlite3_column_int(stmt, 3))
    {
        case WIDGET_FIELD_CHECK:
        case WIDGET_FIELD_GRID:
        case WIDGET_FIELD_PICKLIST:
        case WIDGET_FIELD_TEXT:
            break;
        default:
            tabbable = STACK_NO;
            break;
    }
    sqlite3_finalize(stmt);
    
    if (!stack_is_writable(in_stack)) tabbable = STACK_NO;
    
    return tabbable;
}


/*
 *  _stack_widget_content_owner
 *  ---------------------------------------------------------------------------------------------
 *  Determines what layer, card or background owns the content of a specific widget, taking
 *  into account the Shared setting of background owned widgets.
 *
 *  Results are as follows:
 *          Widget Owner        Shared?     Content Owner
 *          -----------------------------------------------
 *          Card                -           Card
 *          Background          NO          Card
 *          Background          YES         Background
 *
 *  Returns the appropriate layer ID in either out_card_id or out_bkgnd_id, and STACK_NO_OBJECT
 *  in the other.
 *
 *  (If called on a card field and supplied with the wrong card ID, both results will return
 *  STACK_NO_OBJECT.  Background owners are not verified.)
 */

void _stack_widget_content_owner(Stack *in_stack, long in_widget_id, long in_current_card_id,
                                 long *out_card_id, long *out_bkgnd_id)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    assert(in_current_card_id > 0);
    assert(out_card_id != NULL);
    assert(out_bkgnd_id != NULL);
    
    /* default to the widget content not being owned by anyone */
    *out_card_id = STACK_NO_OBJECT;
    *out_bkgnd_id = STACK_NO_OBJECT;
    
    /* lookup the card, background and shared of the widget */
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db,
                       "SELECT cardid,bkgndid,shared FROM widget WHERE widgetid=?1",
                       -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    int err = sqlite3_step(stmt);
    if (err == SQLITE_ROW)
    {
        /* is widget on a card? */
        if (sqlite3_column_int(stmt, 1) < 1)
        {
            if (sqlite3_column_int(stmt, 0) == in_current_card_id)
                *out_card_id = in_current_card_id;
        }
        
        /* is background widget with card content? */
        else if (sqlite3_column_int(stmt, 2) == 0)
            *out_card_id = in_current_card_id;
        
        /* background widget with background content */
        else
            *out_bkgnd_id = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    
    /* ignore database errors for this routine;
     used by get_content() which requires it to silently fail at the moment */
    //if (err != SQLITE_ROW) return;
    
    //assert(*out_bkgnd_id == STACK_NO_OBJECT || *out_card_id == STACK_NO_OBJECT);
}



/*********
 Public API
 */

/* accessors: */


/*
 *  stack_widget_count
 *  ---------------------------------------------------------------------------------------------
 *  Returns the number of widgets on the specified layer, card or background.
 *  (Not the combined number of widgets visible on a card)
 */

long stack_widget_count(Stack *in_stack, long in_card_id, long in_bkgnd_id)
{
    assert(in_stack != NULL);
    assert((in_card_id > 0) || (in_bkgnd_id > 0));
    assert((in_card_id < 1) || (in_bkgnd_id < 1));
    
    /* load the appropriate sequence table (cached usually.)
     we use this instead of doing a SELECT COUNT(*) in the unlikely event there is database
     corruption this will allow us to behave consistently across the unit */
    IDTable *table;
    if (in_card_id > 0) table = _stack_widget_seq_get(in_stack, in_card_id, STACK_NO_OBJECT);
    else table = _stack_widget_seq_get(in_stack, STACK_NO_OBJECT, in_bkgnd_id);
    
    /* return the number of items in the table (widget count) */
    return idtable_size(table);
}



/*
 *  stack_widget_n
 *  ---------------------------------------------------------------------------------------------
 *  Returns the ID of widget N (0-based) in the specified layer.  If the widget doesn't exist in
 *  the layer, returns STACK_NO_OBJECT.
 *
 *  (Can be relied upon even if the database has sustained corruption)
 */

long stack_widget_n(Stack *in_stack, long in_card_id, long in_bkgnd_id, long in_widget_index)
{
    assert(in_stack != NULL);
    assert((in_card_id > 0) || (in_bkgnd_id > 0));
    assert((in_card_id < 1) || (in_bkgnd_id < 1));
    
    /* load the appropriate sequence table */
    IDTable *table;
    if (in_card_id > 0) table = _stack_widget_seq_get(in_stack, in_card_id, STACK_NO_OBJECT);
    else table = _stack_widget_seq_get(in_stack, STACK_NO_OBJECT, in_bkgnd_id);
    
    /* lookup and return the widget ID */
    long widget_id = idtable_id_for_index(table, in_widget_index);
    return (widget_id == 0 ? STACK_NO_OBJECT : widget_id);
}


/*
 *  stack_widget_id
 *  ---------------------------------------------------------------------------------------------
 *  If a widget with the ID exists for the specified layer (card/background), returns the ID.
 *  Otherwise returns STACK_NO_OBJECT.
 *
 *  Basically just a quick way to verify that an ID is a valid widget ID for a layer.
 *
 *  (Can be relied upon even if the database has sustained corruption)
 */

long stack_widget_id(Stack *in_stack, long in_card_id, long in_bkgnd_id, long in_widget_id)
{
    assert(in_stack != NULL);
    assert((in_card_id > 0) || (in_bkgnd_id > 0));
    assert((in_card_id < 1) || (in_bkgnd_id < 1));
    
    /* load the appropriate sequence table */
    IDTable *table;
    if (in_card_id > 0) table = _stack_widget_seq_get(in_stack, in_card_id, STACK_NO_OBJECT);
    else table = _stack_widget_seq_get(in_stack, STACK_NO_OBJECT, in_bkgnd_id);
    
    /* lookup sequence and return the widget ID */
    long widget_seq = idtable_index_for_id(table, in_widget_id);
    return (widget_seq < 0 ? STACK_NO_OBJECT : in_widget_id);
    
    return STACK_NO_OBJECT;
}


/*
 *  stack_widget_named
 *  ---------------------------------------------------------------------------------------------
 *  Returns the ID of the widget with the specified name and parent layer (card/background.)  
 *  If the widget doesn't exist in the layer, returns STACK_NO_OBJECT.
 *
 *  (Can be relied upon even if the database has sustained corruption)
 */

long stack_widget_named(Stack *in_stack, long in_card_id, long in_bkgnd_id, char const *in_name)
{
    assert(in_stack != NULL);
    assert(in_name != NULL);
    assert((in_card_id > 0) || (in_bkgnd_id > 0));
    assert((in_card_id < 1) || (in_bkgnd_id < 1));
    
    /* lookup the widget ID */
    long widget_id;
    sqlite3_stmt *stmt;
    if (in_card_id != STACK_NO_OBJECT)
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT widgetid FROM widget WHERE name LIKE ?1 AND cardid=?2", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 2, (int)in_card_id);
    }
    else
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT widgetid FROM widget WHERE name LIKE ?1 AND bkgndid=?2", -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 2, (int)in_bkgnd_id);
    }
    sqlite3_bind_text(stmt, 1, (char*)in_name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) widget_id = sqlite3_column_int(stmt, 0);
    else widget_id = STACK_NO_OBJECT;
    sqlite3_finalize(stmt);
    if (widget_id == STACK_NO_OBJECT) return STACK_NO_OBJECT;
    
    /* verify the widget is on the specified layer */
    return stack_widget_id(in_stack, in_card_id, in_bkgnd_id, widget_id);
}


/*
 *  stack_widget_basics
 *  ---------------------------------------------------------------------------------------------
 *  Provides commonly used, basic layout information about the specified widget ID.
 *
 *  Only the results for which a non-NULL pointer is supplied will be retrieved, so you can
 *  lookup a specific value and ignore the others if convenient.
 *
 *  Returns STACK_YES if the request was successful, STACK_NO otherwise.  Returned values are 
 *  undefined if the request fails.
 *
 *  Intended for use by the user interface view to enumerate and draw widgets to the screen.
 *
 *  ! Should not be relied upon to determine if a widget exists, use stack_widget_id(),
 *  stack_widget_named() or stack_widget_n() for this purpose instead.
 */

int stack_widget_basics(Stack *in_stack, long in_widget_id,
                        long *out_x, long *out_y, long *out_width, long *out_height,
                        int *out_hidden, enum Widget *out_type)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    
    /* fetch information */
    sqlite3_stmt *stmt;
    int err;
    sqlite3_prepare_v2(in_stack->db,
                             "SELECT x,y,width,height,hidden,type FROM widget WHERE widgetid=?1",
                             -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    err = sqlite3_step(stmt);
    if (err != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return STACK_NO;
    }
    
    /* copy requested values */
    if (out_x) *out_x = sqlite3_column_int(stmt, 0);
    if (out_y) *out_y = sqlite3_column_int(stmt, 1);
    if (out_width) *out_width = sqlite3_column_int(stmt, 2);
    if (out_height) *out_height = sqlite3_column_int(stmt, 3);
    if (out_hidden) *out_hidden = sqlite3_column_int(stmt, 4);
    if (out_type) *out_type = sqlite3_column_int(stmt, 5);
    
    /* cleanup */
    sqlite3_finalize(stmt);
    return STACK_YES;
}


/*
 *  stack_widget_is_card
 *  ---------------------------------------------------------------------------------------------
 *  Convenience function; determines if the specified widget ID is part of a card or background.
 * 
 *  ! Should not be relied upon to determine if a widget exists, use stack_widget_id(),
 *  stack_widget_named() or stack_widget_n() for this purpose instead.
 *
 *  Returns STACK_YES or STACK_NO.
 */

int stack_widget_is_card(Stack *in_stack, long in_widget_id)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    
    sqlite3_stmt *stmt;
    long card_id = STACK_NO_OBJECT;
    sqlite3_prepare_v2(in_stack->db, "SELECT cardid FROM widget WHERE widgetid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    if (sqlite3_step(stmt) == SQLITE_ROW)
        card_id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    return (card_id > 0);
}


/*
 *  stack_widget_is_field
 *  ---------------------------------------------------------------------------------------------
 *  Convenience function; determines if the specified widget ID is a field.
 *
 *  ! Should not be relied upon to determine if a widget exists, use stack_widget_id(),
 *  stack_widget_named() or stack_widget_n() for this purpose instead.
 *
 *  Returns STACK_YES or STACK_NO.
 */

int stack_widget_is_field(Stack *in_stack, long in_widget_id)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    
    int is_field = 0;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "SELECT type FROM widget WHERE widgetid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        switch (sqlite3_column_int(stmt, 0))
        {
            case WIDGET_FIELD_CHECK:
            case WIDGET_FIELD_GRID:
            case WIDGET_FIELD_PICKLIST:
            case WIDGET_FIELD_TEXT:
                is_field = 1;
                break;
        }
    }
    sqlite3_finalize(stmt);
    
    return is_field;
}


/*
 *  stack_widget_next
 *  ---------------------------------------------------------------------------------------------
 *  Returns the widget ID of the next widget to that given, in the tab-sequence for the specified
 *  card, and given the user is/isn't editing the background (in_bkgnd.)
 *
 *  If supplied with an invalid widget ID, attempts to find the first widget that should get
 *  the focus.
 *
 *  If no widget should have focus, returns STACK_NO_OBJECT.  Focusability of a widget is checked
 *  by _widget_tabbable().  Generally focus goes around-and-around in a loop, so if one field can
 *  get the focus, this function will always return a valid widget ID.
 *
 *  Various factors are considered, including whether a widget is locked, hidden or has it's
 *  value shared between multiple cards of the same background.
 *
 *  Intended for use by the user interface view to determine the next field that should get the
 *  focus when the user presses the Tab-key.
 *
 *  See also it's opposite, below: stack_widget_previous().
 */

long stack_widget_next(Stack *in_stack, long in_widget_id, long in_card_id, int in_bkgnd)
{
    assert(in_stack != NULL);
    assert((in_bkgnd == !0) || (in_bkgnd ==!!0));
    
    /* lookup the bkgnd ID of the card */
    long bkgnd_id = _cards_bkgnd(in_stack, in_card_id);
    assert(bkgnd_id > 0);
    
    /* get the ID tables */
    IDTable *card_table = NULL;
    if (!in_bkgnd) card_table = _stack_widget_seq_get(in_stack, in_card_id, 0);
    IDTable *bkgnd_table = _stack_widget_seq_get(in_stack, 0, bkgnd_id);
    long card_widget_count = 0;
    if (card_table) card_widget_count = idtable_size(card_table);
    long bkgnd_widget_count = 0;
    if (bkgnd_table) bkgnd_widget_count = idtable_size(bkgnd_table);
    
    /* get the supplied widget index, or the last index if none was provided */
    long widget_index = -1;
    if (in_widget_id > 0)
    {
        if (card_table)
        {
            widget_index = idtable_index_for_id(card_table, in_widget_id);
            if (widget_index >= 0) widget_index += bkgnd_widget_count;
        }
        
        if (widget_index < 0)
        {
            widget_index = idtable_index_for_id(bkgnd_table, in_widget_id);
            //if (widget_index >= 0) widget_index += bkgnd_widget_count;
        }
    }
    if (widget_index < 0)
        widget_index = card_widget_count + bkgnd_widget_count - 1;
    if (widget_index < 0) return STACK_NO_OBJECT;
    
    /* increment the widget index and check the corresponding widget is tabbable;
     repeat until we find a widget or we return to the original widget index */
    long widget_id;
    for (int i = 0; i < card_widget_count + bkgnd_widget_count + 2; i++)
    {
        widget_index++;
        if (widget_index >= card_widget_count + bkgnd_widget_count)
            widget_index = 0;
        
        if ((widget_index < bkgnd_widget_count) && bkgnd_table)
            widget_id = idtable_id_for_index(bkgnd_table, widget_index);
        else
        {
            if (card_table)
                widget_id = idtable_id_for_index(card_table, widget_index - bkgnd_widget_count);
            else
                widget_id = 0;
        }
        if (_widget_tabbable(in_stack, widget_id, in_bkgnd))
            return widget_id;
    }
    
    return STACK_NO_OBJECT;
}


/*
 *  stack_widget_next
 *  ---------------------------------------------------------------------------------------------
 *  Returns the widget ID of the previous widget to that given, in the tab-sequence for the 
 *  specified card, and given the user is/isn't editing the background (in_bkgnd.)
 *
 *  If supplied with an invalid widget ID, attempts to find the last widget that should get
 *  the focus.
 *
 *  If no widget should have focus, returns STACK_NO_OBJECT.  See it's opposite, 
 *  stack_widget_next() for more information.
 *
 *  Intended for use by the user interface view to determine the next field that should get the
 *  focus when the user presses the Tab-key.
 */

long stack_widget_previous(Stack *in_stack, long in_widget_id, long in_card_id, int in_bkgnd)
{
    assert(in_stack != NULL);
    assert((in_bkgnd == !0) || (in_bkgnd ==!!0));
    
    /* lookup the bkgnd ID of the card */
    long bkgnd_id = _cards_bkgnd(in_stack, in_card_id);
    assert(bkgnd_id > 0);
    
    /* get the ID tables */
    IDTable *card_table = NULL;
    if (!in_bkgnd) card_table = _stack_widget_seq_get(in_stack, in_card_id, 0);
    IDTable *bkgnd_table = _stack_widget_seq_get(in_stack, 0, bkgnd_id);
    long card_widget_count = 0;
    if (card_table) card_widget_count = idtable_size(card_table);
    long bkgnd_widget_count = 0;
    if (bkgnd_table) bkgnd_widget_count = idtable_size(bkgnd_table);
    
    /* get the supplied widget index, or the first index if none was provided */
    long widget_index = -1;
    if (in_widget_id > 0)
    {
        if (card_table)
        {
            widget_index = idtable_index_for_id(card_table, in_widget_id);
            if (widget_index >= 0) widget_index += bkgnd_widget_count;
        }
        if (widget_index < 0)
        {
            widget_index = idtable_index_for_id(bkgnd_table, in_widget_id);
            //if (widget_index >= 0) widget_index += bkgnd_widget_count;
        }
    }
    if (widget_index < 0) widget_index = 0;
    if ((card_widget_count == 0) && (bkgnd_widget_count == 0)) return STACK_NO_OBJECT;
    
    /* increment the widget index and check the corresponding widget is tabbable;
     repeat until we find a widget or we return to the original widget index */
    long widget_id;
    for (int i = 0; i < card_widget_count + bkgnd_widget_count + 2; i++)
    {
        widget_index--;
        if (widget_index < 0)
            widget_index = card_widget_count + bkgnd_widget_count - 1;
        
        if ((widget_index < bkgnd_widget_count) && bkgnd_table)
            widget_id = idtable_id_for_index(bkgnd_table, widget_index);
        else
        {
            if (card_table)
                widget_id = idtable_id_for_index(card_table, widget_index - bkgnd_widget_count);
            else
                widget_id = 0;
        }
        if (_widget_tabbable(in_stack, widget_id, in_bkgnd))
            return widget_id;
    }
    
    return STACK_NO_OBJECT;
}



/* mutators: */

/*
 *  stack_widgets_send_front
 *  ---------------------------------------------------------------------------------------------
 *  Moves a selection of widgets to the front of the tab-sequence and view heirarchy.
 */

void stack_widgets_send_front(Stack *in_stack, long *in_widgets, int in_count)
{
    _rearrange_widgets(in_stack, in_widgets, in_count, idtable_send_front);
}


/*
 *  stack_widgets_send_back
 *  ---------------------------------------------------------------------------------------------
 *  Moves a selection of widgets to the back of the tab-sequence and view heirarchy.
 */

void stack_widgets_send_back(Stack *in_stack, long *in_widgets, int in_count)
{
    _rearrange_widgets(in_stack, in_widgets, in_count, idtable_send_back);
}


/*
 *  stack_widgets_shuffle_forward
 *  ---------------------------------------------------------------------------------------------
 *  Moves a selection of widgets forward 1-position in the tab-sequence and view heirarchy.
 */

void stack_widgets_shuffle_forward(Stack *in_stack, long *in_widgets, int in_count)
{
    _rearrange_widgets(in_stack, in_widgets, in_count, idtable_shuffle_forward);
}


/*
 *  stack_widgets_shuffle_backward
 *  ---------------------------------------------------------------------------------------------
 *  Moves a selection of widgets backward 1-position in the tab-sequence and view heirarchy.
 */

void stack_widgets_shuffle_backward(Stack *in_stack, long *in_widgets, int in_count)
{
    _rearrange_widgets(in_stack, in_widgets, in_count, idtable_shuffle_backward);
}


/*
 *  stack_create_widget
 *  ---------------------------------------------------------------------------------------------
 *  Creates a new widget of the specified type on a card/background layer.
 *
 *  Returns the ID of the newly created widget if successful, or STACK_NO_OBJECT otherwise.
 *  If there was an error, out_error is set to an error code:
 *
 *  - STACK_ERR_TOO_MANY_WIDGETS - if the stack has reached the maximum specified number of cards
 *  - STACK_ERR_UNKNOWN - everything else
 */

long stack_create_widget(Stack *in_stack, enum Widget in_type, long in_card_id, long in_bkgnd_id, int *out_error)
{
    assert(in_stack != NULL);
    assert((in_card_id > 0) || (in_bkgnd_id > 0));
    assert((in_card_id < 1) || (in_bkgnd_id < 1));
    assert(out_error);
    
    /* set the default error status */
    *out_error = STACK_ERR_UNKNOWN;
    
    /* create the widget */
    sqlite3_stmt *stmt;
    int err;
    sqlite3_prepare_v2(in_stack->db,
                                 "INSERT INTO widget VALUES (NULL, ?1, ?2, '', ?3, 0, 0, ?4, 0, 0, 0, 200, 60, 0, '', ?5, 0)",
                                 -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_id);
    sqlite3_bind_int(stmt, 2, (int)in_bkgnd_id);
    sqlite3_bind_int(stmt, 3, (int)in_type);
    if ((in_type == WIDGET_BUTTON_PUSH) || (in_type == WIDGET_BUTTON_TRANSPARENT))
    {
        sqlite3_bind_text(stmt, 4, "on mouseUp\n  \nend mouseUp\n", -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, 13);
    }
    else
    {
        sqlite3_bind_text(stmt, 4, "", -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, 0);
    }
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE) return STACK_NO_OBJECT;
    long widget_id = sqlite3_last_insert_rowid(in_stack->db);

    /* add the widget to the card/bkgnd widget sequence */
    IDTable *widget_seq = _stack_widget_seq_get(in_stack, in_card_id, in_bkgnd_id);
    if (!widget_seq) return STACK_NO_OBJECT; /* leaves an orphan record, probably not a big deal given how unlikely it is;
                                              we could clean up later in _compact() */
    idtable_append(widget_seq, widget_id);
    _stack_widget_seq_set(in_stack, in_card_id, in_bkgnd_id, widget_seq);
    
    
    
    
    
    /* record the undo step */
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    if (undo_data)
    {
        serbuff_write_long(undo_data, widget_id);
        serbuff_write_long(undo_data, idtable_size(widget_seq)-1);
        _undo_record_step(in_stack, UNDO_WIDGET_CREATE, undo_data);
    }
    
    /* return the new widget ID */
    return widget_id;
}


/*
 *  stack_delete_widget
 *  ---------------------------------------------------------------------------------------------
 *  Deletes the specified widget.
 */

void stack_delete_widget(Stack *in_stack, long in_widget_id)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    
    /* figure out who owns the widget */
    long card_id, bkgnd_id;
    stack_widget_owner(in_stack, in_widget_id, &card_id, &bkgnd_id);
    
    /* get the widget sequence table for the owner layer */
    IDTable *widget_seq = _stack_widget_seq_get(in_stack, card_id, bkgnd_id);
    assert(widget_seq != NULL);
    
    /* create the undo step */
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    if (undo_data)
    {
        void *widget_data;
        long widget_id_list[1];
        widget_id_list[0] = in_widget_id;
        long widget_data_size = _widgets_serialize(in_stack, widget_id_list, 1, &widget_data, STACK_YES);
        serbuff_write_data(undo_data, widget_data, widget_data_size);
        serbuff_write_long(undo_data, card_id);
        serbuff_write_long(undo_data, bkgnd_id);
        serbuff_write_long(undo_data, idtable_index_for_id(widget_seq, in_widget_id));
    }
    
    _stack_begin(in_stack, STACK_ENTRY_POINT);
    
    /* delete the widget */
    sqlite3_stmt *stmt;
    int err;
    sqlite3_prepare_v2(in_stack->db, "DELETE FROM widget_content WHERE widgetid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
    {
        if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        return _stack_panic_void(in_stack, STACK_ERR_IO);
    }
    
    sqlite3_prepare_v2(in_stack->db, "DELETE FROM widget_options WHERE widgetid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
    {
        if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        return _stack_panic_void(in_stack, STACK_ERR_IO);
    }
    
    sqlite3_prepare_v2(in_stack->db, "DELETE FROM widget WHERE widgetid=?1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
    {
        if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        return _stack_panic_void(in_stack, STACK_ERR_IO);
    }
    
    /* remove the widget from the sequence */
    idtable_remove(widget_seq, idtable_index_for_id(widget_seq, in_widget_id));
    _stack_widget_seq_set(in_stack, card_id, bkgnd_id, widget_seq);
    
    /* write changes to disk */
    if (_stack_commit(in_stack) != SQLITE_OK)
    {
        if (undo_data) serbuff_destroy(undo_data, 1);
        _stack_cancel(in_stack);
        _stack_widget_seq_cache_load(in_stack, card_id, bkgnd_id);
        return _stack_panic_void(in_stack, STACK_ERR_IO);
    }

    /* record undo step */
    if (undo_data)
        _undo_record_step(in_stack, UNDO_WIDGET_DELETE, undo_data);
}


/*
 *  stack_widget_set_rect
 *  ---------------------------------------------------------------------------------------------
 *  Sets the location and size of the specified widget within the card/background to which 
 *  it belongs.
 */

void stack_widget_set_rect(Stack *in_stack, long in_widget_id, long in_x, long in_y, long in_width, long in_height)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    
    /* prepare the undo step */
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    sqlite3_stmt *stmt;
    int err;
    if (undo_data)
    {
        serbuff_write_long(undo_data, in_widget_id);
        sqlite3_prepare_v2(in_stack->db,
                           "SELECT x,y,width,height FROM widget WHERE widgetid=?1",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        err = sqlite3_step(stmt);
        if (err == SQLITE_ROW)
        {
            serbuff_write_long(undo_data, sqlite3_column_int(stmt, 0));
            serbuff_write_long(undo_data, sqlite3_column_int(stmt, 1));
            serbuff_write_long(undo_data, sqlite3_column_int(stmt, 2));
            serbuff_write_long(undo_data, sqlite3_column_int(stmt, 3));
        }
        sqlite3_finalize(stmt);
        if (err != SQLITE_ROW)
            return _stack_panic_void(in_stack, STACK_ERR_IO);
    }
    
    /* change the widget bounds */
    sqlite3_prepare_v2(in_stack->db,
                       "UPDATE widget SET x=?1, y=?2, width=?3, height=?4 WHERE widgetid=?5",
                       -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_x);
    sqlite3_bind_int(stmt, 2, (int)in_y);
    sqlite3_bind_int(stmt, 3, (int)in_width);
    sqlite3_bind_int(stmt, 4, (int)in_height);
    sqlite3_bind_int(stmt, 5, (int)in_widget_id);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_DONE)
        return _stack_panic_void(in_stack, STACK_ERR_IO);
    
    /* record the undo step */
    if (undo_data)
        _undo_record_step(in_stack, UNDO_WIDGET_SIZE, undo_data);
}




/*********
 Pasteboard Support API
 */

/*
 *  stack_widget_copy
 *  ---------------------------------------------------------------------------------------------
 *  Obtains a block of data representative of the supplied widget IDs, suitable for storage on
 *  the host OS pasteboard for Cut, Copy and Paste operations.
 *
 *  Function retains ownership of the data.  The data remains valid until the next mutating 
 *  operation on the stack.
 */

long stack_widget_copy(Stack *in_stack, long *in_ids, int in_count, void **out_data)
{
    return _widgets_serialize(in_stack, in_ids, in_count, out_data, STACK_NO);
}


/*
 *  stack_widget_cut
 *  ---------------------------------------------------------------------------------------------
 *  Obtains a block of data representative of the supplied widget IDs, suitable for storage on
 *  the host OS pasteboard for Cut, Copy and Paste operations.
 *
 *  Removes the specified widget IDs from their respective card/background layers.
 *
 *  Function retains ownership of the data.  The data remains valid until the next mutating
 *  operation on the stack.
 */

long stack_widget_cut(Stack *in_stack, long *in_ids, int in_count, void **out_data)
{
    long size;
    size = _widgets_serialize(in_stack, in_ids, in_count, out_data, STACK_NO);
    for (int i = 0; i < in_count; i++)
    {
        stack_delete_widget(in_stack, in_ids[i]);
    }
    return size;
}


/*
 *  stack_widget_paste
 *  ---------------------------------------------------------------------------------------------
 *  Creates widgets using the provided serialized data (as previously obtained from 
 *  stack_widget_cut() or stack_widget_copy().
 *
 *  The widgets are 'pasted' into the specified card or background layer.
 *
 *  Widgets are pasted without their original content and their IDs may change if an ID conflict
 *  would otherwise result.
 *
 *  Intended for use by host OS paste operation.
 */

int stack_widget_paste(Stack *in_stack, long in_card_id, long in_bkgnd_id, void *in_data, long in_size, long **out_ids)
{
    return _widgets_unserialize(in_stack, in_data, in_size, in_card_id, in_bkgnd_id, (long const **)out_ids, STACK_NO, STACK_YES, STACK_NO);
}


/* clone note:  should be implemented such that widget list is separated into card & background widgets, with each handled
 separately (use an above function for that purpose), then the clones can be maintained within their respective layers */



/*********
 Content API
 */


/*
 *  stack_widget_content_get
 *  ---------------------------------------------------------------------------------------------
 *  Accesses the content of the specified widget, for a specific card and given the user is/isn't
 *  in edit background mode.
 *
 *  Only returns what is requested - thus you can leave any of the outputs: searchable, formatted
 *  or editable NULL.
 *
 *  Editable is a boolean STACK_YES/STACK_NO indicating if the widget should be editable under
 *  current circumstances, taking into account the setting of the locked and shared properties.
 *
 *  Ownership of the resultant text and/or data remains with the function, so callers do not have
 *  to worry about deallocating anything, but may need to take a copy.
 *  The results will remain valid until the next call to stack_widget_content_get() or the stack
 *  is closed.
 *
 *  If the supplied widget ID is invalid, will likely return an empty string (no error raised),
 *  however, if the stack has somehow become corrupt, there is no guarantee of the return content.
 */

void stack_widget_content_get(Stack *in_stack, long in_widget_id, long in_card_id, int in_bkgnd, char **out_searchable,
                              char **out_formatted, long *out_formatted_size, int *out_editable)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    
    /* cleanup old static data from previous call */
    if (in_stack->searchable) _stack_free(in_stack->searchable);
    in_stack->searchable = NULL;
    if (in_stack->formatted) _stack_free(in_stack->formatted);
    in_stack->formatted = NULL;
    long formatted_size = 0;
    
    /* find out who owns the Content: card or background */
    long card_id, bkgnd_id;
    _stack_widget_content_owner(in_stack, in_widget_id, in_card_id, &card_id, &bkgnd_id);
    
    /* shortcut: return empty if in background mode and the content is owned by the card */
    if ( (!in_bkgnd) || (in_bkgnd && (bkgnd_id != STACK_NO_OBJECT)) )
    {
        /* access the content for the appropriate content owner */
        sqlite3_stmt *stmt;
        int err;
        sqlite3_prepare_v2(in_stack->db,
                           "SELECT searchable,formatted FROM widget_content WHERE widgetid=?1 AND cardid=?2 AND bkgndid=?3",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_bind_int(stmt, 2, (int)card_id);
        sqlite3_bind_int(stmt, 3, (int)bkgnd_id);
        err = sqlite3_step(stmt);
        
        /* there is content */
        if (err == SQLITE_ROW)
        {
            //printf("stack_widget_content_get: SQLite OK: \"%s\"\n", (char*)sqlite3_column_text(stmt, 0));
            in_stack->searchable = _stack_clone_cstr( (char*)sqlite3_column_text(stmt, 0) );
            //printf("stack_widget_content_get: output   : \"%s\"\n", in_stack->searchable);
            formatted_size = sqlite3_column_bytes(stmt, 1);
            in_stack->formatted = _stack_malloc(formatted_size);
            if (!in_stack->formatted) {
                formatted_size = 0;
                _stack_panic_void(in_stack, STACK_ERR_MEMORY);
            }
            else memcpy(in_stack->formatted, sqlite3_column_blob(stmt, 1), formatted_size);
        }
        //else printf("stack_widget_content_get: SQLite error: %d\n", err);
        
        sqlite3_finalize(stmt);
    }
    
    /* return the requested content */
    if (!in_stack->searchable) in_stack->searchable = _stack_clone_cstr("");
    if (out_searchable) *out_searchable = in_stack->searchable;
    if (out_formatted) *out_formatted = in_stack->formatted;
    if (out_formatted_size) *out_formatted_size = formatted_size;
    if (out_editable) *out_editable = _widget_tabbable(in_stack, in_widget_id, in_bkgnd);
}



/*
 *  stack_widget_content_set
 *  ---------------------------------------------------------------------------------------------
 *  Writes new content for the specified widget, for a specific card and taking into account
 *  the widget's content owner (card/background).  Note that a widget's content owner may be 
 *  different to the widget's owner, eg. when a background field's content is not shared.
 *
 *  Searchable content is required.  Formatted content is optional.  The format is determined by
 *  the application.
 *
 *  Does not assume ownership of the supplied content and takes a copy to write to disk.
 */

void stack_widget_content_set(Stack *in_stack, long in_widget_id, long in_card_id, char *in_searchable,
                              char *in_formatted, long in_formatted_size)
{
    assert(in_stack != NULL);
    assert(in_widget_id > 0);
    assert(in_card_id > 0);
    assert(in_searchable != NULL);
    
    //printf("Setting searchable: \"%s\" (%ld)\n", in_searchable, in_widget_id);
    
    
    /* find out who owns the content, card or background */
    long card_id, bkgnd_id;
    _stack_widget_content_owner(in_stack, in_widget_id, in_card_id, &card_id, &bkgnd_id);
    //printf("Owner: \"%s\" (%ld, %ld, %ld)\n", in_searchable, in_widget_id, card_id, bkgnd_id);
    
    /* check if there is any existing content;
     and prepare the undo step */
    SerBuff *undo_data = serbuff_create(in_stack, NULL, 0, 0);
    //printf("create: \"%s\" (%ld, %ld, %ld)\n", in_searchable, in_widget_id, card_id, bkgnd_id);
    if (undo_data)
    {
        serbuff_write_long(undo_data, in_widget_id);
        serbuff_write_long(undo_data, in_card_id);
    }
    //printf("ids: \"%s\" (%ld, %ld, %ld)\n", in_searchable, in_widget_id, card_id, bkgnd_id);
    sqlite3_stmt *stmt;
    int existing;
    sqlite3_prepare_v2(in_stack->db,
                       "SELECT searchable,formatted FROM widget_content WHERE widgetid=?1 AND cardid=?2 AND bkgndid=?3",
                       -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_widget_id);
    sqlite3_bind_int(stmt, 2, (int)card_id);
    sqlite3_bind_int(stmt, 3, (int)bkgnd_id);
    //printf("bind: \"%s\" (%ld, %ld, %ld)\n", in_searchable, in_widget_id, card_id, bkgnd_id);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        existing = STACK_YES;
        
        /* save the existing content */
        if (undo_data)
        {
            //printf("step: \"%s\" (%ld, %ld, %ld)\n", in_searchable, in_widget_id, card_id, bkgnd_id);
            serbuff_write_cstr(undo_data, (char*)sqlite3_column_text(stmt, 0));
            //printf("txt: \"%s\" (%ld, %ld, %ld)\n", in_searchable, in_widget_id, card_id, bkgnd_id);
            serbuff_write_data(undo_data, (void*)sqlite3_column_blob(stmt, 1), sqlite3_column_bytes(stmt, 1));
            //printf("blob: \"%s\" (%ld, %ld, %ld)\n", in_searchable, in_widget_id, card_id, bkgnd_id);
        }
    }
    else
    {
        existing = STACK_NO;
        if (undo_data)
        {
            serbuff_write_cstr(undo_data, "");
            serbuff_write_data(undo_data, "", 0);
        }
    }
    sqlite3_finalize(stmt);
    
    /* update/insert new content */
    int err;
    if (existing)
    {
        //printf("updating existing \"%s\"\n", in_searchable);
        sqlite3_prepare_v2(in_stack->db,
                           "UPDATE widget_content SET searchable=?4,formatted=?5 WHERE widgetid=?1 AND cardid=?2 AND bkgndid=?3",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_bind_int(stmt, 2, (int)card_id);
        sqlite3_bind_int(stmt, 3, (int)bkgnd_id);
        sqlite3_bind_text(stmt, 4, in_searchable, -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(stmt, 5, in_formatted, (int)in_formatted_size, SQLITE_TRANSIENT);
        err = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    else
    {
        //printf("inserting new \"%s\"\n", in_searchable);
        sqlite3_prepare_v2(in_stack->db,
                           "INSERT INTO widget_content VALUES (?1, ?2, ?3, ?4, ?5)",
                           -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, (int)in_widget_id);
        sqlite3_bind_int(stmt, 2, (int)card_id);
        sqlite3_bind_int(stmt, 3, (int)bkgnd_id);
        sqlite3_bind_text(stmt, 4, in_searchable, -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(stmt, 5, in_formatted, (int)in_formatted_size, SQLITE_TRANSIENT);
        err = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    if (err != SQLITE_DONE)
    {
        if (undo_data) serbuff_destroy(undo_data, STACK_YES);
        return _stack_panic_void(in_stack, STACK_ERR_IO);
    }
    
    /* record the undo step */
    if (undo_data)
        _undo_record_step(in_stack, UNDO_WIDGET_CONTENT, undo_data);
}


