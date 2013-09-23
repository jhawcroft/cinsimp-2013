/*
 
 Stack File Format
 stack.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Basic setup and file I/O
 
 !!  This code unit is critical to the integrity of the Stack file format.
 
 */
/*
 
 TODO
 
 -  Write a schema checking function so that SQLite function calls needn't be checked as routinely
 (_stack_check_schema_is_ok())
 
 */


/* figuring out a specific card within a specific bkgnd by number
 will require loading the stack's cards table, and flagging only
 those cards that exist in the bkgnd (parallel array),
 then iterating and counting to identify specific cards of a
 particular bkgnd.  thus cards are sorted always within the stack.
 the actual sorting can be done via indirection - building a list
 of the cards in the stack as a series of indexes into the stack's
 card table? this would potentially screw-up a stack sort but
 that shouldn't matter.  ?? */

#include "stack_int.h"


#define _STACK_FILE_FORMAT_VERSION 1

/*
static void _handle_sql_error(void *pArg, int iErrCode, const char *zMsg){
    fprintf(stderr, "(%d) %s\n", iErrCode, zMsg);
}
*/

static void _stack_unit_init(void)
{
    static int g_stack_unit_inited = 0;
    if (g_stack_unit_inited) return;
    g_stack_unit_inited = 1;
    
    //sqlite3_config(SQLITE_CONFIG_LOG, errLogCallback, pData);
}



/*
 Notes: apparently SQLite only supports a single transaction via the BEGIN and COMMIT statements,
 ie. they cannot be nested.
 
 What this is probably going to mean, for simplicity, is that anytime we have to issue a rollback,
 we should be flushing the undo stack, as it may well be mutilated and the error was presumably
 something fairly serious anyway.
 */

void _stack_begin(Stack *in_stack, int in_is_entry_point)
{
    assert(in_stack != NULL);

    if (in_is_entry_point)
    {
        assert(in_stack->has_begun == 0);
        in_stack->has_begun = 1;
    }
    else
        in_stack->has_begun++;
    
    if (in_stack->has_begun > 1) return;
    
    int err = sqlite3_exec(in_stack->db, "BEGIN", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
}


int _stack_commit(Stack *in_stack)
{
    assert(in_stack != NULL);
    assert(in_stack->has_begun > 0);
    
    in_stack->has_begun--;
    if (in_stack->has_begun > 0) return SQLITE_OK;
    
    int err = sqlite3_exec(in_stack->db, "COMMIT", NULL, NULL, NULL);    
    return err;
}


/*** TODO *** needs further review: it is likely that if we ever have to rollback, that something
 majorly wrong has happened with either the file format, memory corruption or file I/O.
 
 this should probably be grounds for a catastrophic error message - at least for the open stack.
 review invoking modules also to ensure they take this approach.
 */

void _stack_cancel(Stack *in_stack)
{
    assert(in_stack != NULL);
    assert(in_stack->has_begun > 0);
    
    in_stack->has_begun--;
    if (in_stack->has_begun > 0) return;
    
    int err = sqlite3_exec(in_stack->db, "ROLLBACK", NULL, NULL, NULL);
    assert(err == SQLITE_OK);
    
    stack_undo_flush(in_stack); /* necessary since with the nesting of complex serialisaton routines
                                 things could be in a very screwed up state if anything major has
                                 gone wrong while accessing the disk,
                                 which frankly is of concern anyway... */
}


static void _stack_init(Stack *io_stack)
{
    /* error handling */
    io_stack->fatal_handler = NULL;
    io_stack->has_fatal_error = STACK_NO;
    
    /* database */
    io_stack->db = NULL;
    
    /* serialization */
    io_stack->override_widget_id = 0;
    
    /* caches */
    io_stack->card_widget_table = NULL;
    io_stack->bkgnd_widget_table = NULL;
    io_stack->stack_card_table = NULL;
    
    /* undo */
    io_stack->undo_stack = NULL;
    _undo_stack_create(io_stack);
    
    /* find */
    io_stack->find_search = NULL;
    io_stack->find_words = NULL;
    io_stack->find_state_fields = NULL;
    io_stack->find_state_field_content = NULL;
    io_stack->find_state_field_words = NULL;
    io_stack->find_state_field_offsets = NULL;
    io_stack->found_text = NULL;
    stack_reset_find(io_stack);
}


static int _sql_table_exists(sqlite3 *in_db, char const *in_table)
{
    assert(in_db != NULL);
    assert(in_table != NULL);
    int exists;
    sqlite3_stmt *stmt;
    int err;
    err = sqlite3_prepare_v2(in_db, "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?1", -1, &stmt, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_bind_text(stmt, 1, in_table, -1, SQLITE_STATIC);
    assert(err == SQLITE_OK);
    err = sqlite3_step(stmt);
    assert(err == SQLITE_ROW);
    exists = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return exists;
}


static int _stack_check_schema_is_ok(sqlite3 *in_db, StackOpenStatus *out_status)
{
    assert(in_db != NULL);
    assert(out_status != NULL);
    
    /* default error */
    *out_status = STACK_INVALID;
    
    /* check core tables exist */
    if (!_sql_table_exists(in_db, "stack")) return 0;
    if (!_sql_table_exists(in_db, "bkgnd")) return 0;
    if (!_sql_table_exists(in_db, "stack_card_changes")) return 0;
    if (!_sql_table_exists(in_db, "picture_card")) return 0;
    if (!_sql_table_exists(in_db, "picture_bkgnd")) return 0;
    if (!_sql_table_exists(in_db, "card")) return 0;
    if (!_sql_table_exists(in_db, "widget")) return 0;
    if (!_sql_table_exists(in_db, "widget_options")) return 0;
    if (!_sql_table_exists(in_db, "widget_content")) return 0;
    if (!_sql_table_exists(in_db, "resource")) return 0;
    
    /* read and check stack file format version */
    int version_ok = 0;
    sqlite3_stmt *stmt;
    int err;
    err = sqlite3_prepare_v2(in_db, "SELECT version FROM stack", -1, &stmt, NULL);
    assert(err == SQLITE_OK);
    err = sqlite3_step(stmt);
    if (err != SQLITE_ROW)
        *out_status = STACK_INVALID;
    else
    {
        if (sqlite3_column_int(stmt, 0) > _STACK_FILE_FORMAT_VERSION)
            *out_status = STACK_TOO_NEW;
        else
            version_ok = 1;
    }
    sqlite3_finalize(stmt);
    if (!version_ok) return 0;
    
    /* return a successful validation status */
    *out_status = STACK_OK;
    return 1;
}


void _stack_card_table_load(Stack *in_stack);


Stack* stack_create(char const *in_path, long in_card_width, long in_card_height, StackFatalErrorHandler in_fatal_handler, void *in_error_context)
{
    Stack *stack;
    
    /* allocate and initalize the data structure */
    stack = _stack_calloc(1, sizeof(struct Stack));
    if (!stack) return app_out_of_memory_null();
    BLESS_STAK(stack);
#if STACK_TESTS
    stack->allocated_bytes = _stack_mem_allocated();
#endif
    _stack_init(stack);
    stack->pathname = _stack_clone_cstr(in_path);
    if (stack->has_fatal_error)
    {
        /* there was a problem during initalization */
        stack_close(stack);
        return NULL;
    }
    
    /* create the underlying SQLite database */
    int err = sqlite3_open_v2(in_path, &(stack->db), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (err != SQLITE_OK)
    {
        /* couldn't create, free structures and exit */
        stack_close(stack);
        return NULL;
    }
    
    /* create database schema */
    sqlite3_stmt *stmt;
    err = sqlite3_exec(stack->db,
                 "CREATE TABLE stack (version INTEGER, bkgnds TEXT, cards TEXT, locked INTEGER, cantdelete INTEGER, "
                 "userlevellimit INTEGER, private INTEGER, passwordhash TEXT, script TEXT, "
                 "windowwidth INTEGER, windowheight INTEGER, scrollx INTEGER, scrolly INTEGER, "
                 "showscrollx INTEGER, showscrolly INTEGER, cantpeek INTEGER, cantabort INTEGER, "
                 "cardwidth INTEGER, cardheight INTEGER, script_cp TEXT, script_sel INTEGER)",
                 NULL, NULL, NULL);
    sqlite3_exec(stack->db,
                 "CREATE TABLE bkgnd (bkgndid INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "widgets TEXT, name TEXT, cantdelete INTEGER, "
                 "dontsearch INTEGER, script TEXT, script_cp TEXT, script_sel INTEGER)",
                 NULL, NULL, NULL);
    sqlite3_exec(stack->db,
                 "CREATE TABLE stack_card_changes (entryid INTEGER PRIMARY KEY, cardid INTEGER, deleted INTEGER, sequence INTEGER)",
                 NULL, NULL, NULL);
    sqlite3_exec(stack->db,
                 "CREATE TABLE card (cardid INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "bkgndid INTEGER, widgets TEXT, name TEXT, cantdelete INTEGER, "
                 "dontsearch INTEGER, script TEXT, script_cp TEXT, script_sel INTEGER)",
                 NULL, NULL, NULL);
    sqlite3_exec(stack->db,
                 "CREATE TABLE picture_card (cardid INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "visible INTEGER, data BLOB)",
                 NULL, NULL, NULL);
    sqlite3_exec(stack->db,
                 "CREATE TABLE picture_bkgnd (bkgndid INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "visible INTEGER, data BLOB)",
                 NULL, NULL, NULL);
    sqlite3_exec(stack->db,
                 "CREATE TABLE widget (widgetid INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "cardid INTEGER, bkgndid INTEGER, name TEXT, type INTEGER, shared INTEGER, "
                 "dontsearch INTEGER, script TEXT, hidden INTEGER, x INTEGER, y INTEGER, "
                 "width INTEGER, height INTEGER, locked INTEGER, script_cp TEXT, script_sel INTEGER, "
                 "iconid INTEGER)",
                 NULL, NULL, NULL);
    sqlite3_exec(stack->db,
                 "CREATE TABLE widget_options (widgetid INTEGER, "
                 "optionid INTEGER, value TEXT)",
                 NULL, NULL, NULL);
    sqlite3_exec(stack->db,
                 "CREATE TABLE widget_content (widgetid INTEGER, "
                 "cardid INTEGER, bkgndid INTEGER, searchable TEXT, formatted BLOB)",
                 NULL, NULL, NULL);
    sqlite3_exec(stack->db,
                 "CREATE TABLE resource (resourceid INTEGER, resourcetype TEXT, resourcename TEXT, data BLOB, "
                 "PRIMARY KEY (resourceid,resourcetype))",
                 NULL, NULL, NULL);
    
    sqlite3_prepare_v2(stack->db, "INSERT INTO stack VALUES (?3, '0000000001', '0000000001', 0, 0, 5, 0, '', '', ?1, ?2, "
                       "0, 0, 0, 0, 0, 0, ?1, ?2, '', 0)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_width);
    sqlite3_bind_int(stmt, 2, (int)in_card_height);
    sqlite3_bind_int(stmt, 3, _STACK_FILE_FORMAT_VERSION);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_exec(stack->db,
                 "INSERT INTO bkgnd VALUES (1, '', '', 0, 0, '', '', 0)",
                 NULL, NULL, NULL);
    sqlite3_exec(stack->db,
                 "INSERT INTO card VALUES (1, 1, '', '', 0, 0, '', '', 0)",
                 NULL, NULL, NULL);

    /* check database schema */
    StackOpenStatus status;
    if (!_stack_check_schema_is_ok(stack->db, &status))
    {
        /* schema not created properly, free structures and exit */
        stack_close(stack);
        return NULL;
    }
    
    /* load the stack's card table */
    _stack_card_table_load(stack);
    
    /* set the fatal error handler */
    stack->fatal_handler = in_fatal_handler;
    stack->fatal_context = in_error_context;
    return stack;
}


static void _sql_trace(void* in_context, const char*in_sql)
{
    printf("Stack: SQL: %s\n", in_sql);
}


Stack* stack_open(char const *in_path, StackFatalErrorHandler in_fatal_handler, void *in_error_context, StackOpenStatus *out_status)
{
    Stack *stack;
    *out_status = STACK_UNKNOWN;
    
#if DEBUG
    printf("SQLite Thread Safe: %d\n", sqlite3_threadsafe());
#endif
    
    /* allocate and initalize the data structure */
    stack = _stack_calloc(1, sizeof(struct Stack));
    if (!stack) return app_out_of_memory_null();
    BLESS_STAK(stack);
#if STACK_TESTS
    stack->allocated_bytes = _stack_mem_allocated();
#endif
    _stack_init(stack);
    stack->pathname = _stack_clone_cstr(in_path);
    if (stack->has_fatal_error)
    {
        /* there was a problem during initalization */
        stack_close(stack);
        return NULL;
    }
    
    /* open the underlying SQLite database */
    int err = sqlite3_open_v2(in_path, &(stack->db), SQLITE_OPEN_READWRITE, NULL);
    if (err != SQLITE_OK)
    {
        /* couldn't open, free structures and exit */
        *out_status = STACK_PERMISSION;
        stack_close(stack);
        return NULL;
    }
    
    /* check if the stack is read-only */
    err = sqlite3_exec(stack->db, "UPDATE stack SET version=version", NULL, NULL, NULL);
    if ((err != SQLITE_OK) && (err != SQLITE_READONLY))
    {
        /* schema problem, free structures and exit */
        *out_status = STACK_INVALID;
        stack_close(stack);
        return NULL;
    }
    stack->readonly = (err == SQLITE_READONLY);
    
    /* check database schema */
    if (!_stack_check_schema_is_ok(stack->db, out_status))
    {
        /* schema not created properly, free structures and exit */
        stack_close(stack);
        return NULL;
    }
    
    /* set the fatal error handler */
    stack->fatal_handler = in_fatal_handler;
    stack->fatal_context = in_error_context;
    
    /* install a SQL trace routine for profiling */
    /*sqlite3_trace(stack->db, &_sql_trace, NULL);*/
    
    /* cache Can't Modify */
    stack->soft_lock = (int)stack_prop_get_long(stack, 0, 0, PROPERTY_LOCKED);
    
    /* load the stack's card table */
    _stack_card_table_load(stack);
    
    /* return the newly opened stack */
    return stack;
}


void stack_reset_find(Stack *in_stack);

void stack_close(Stack *in_stack)
{
    /* database */
    if (in_stack->db) sqlite3_close_v2(in_stack->db);
    
    /* caches */
    if (in_stack->card_widget_table) idtable_destroy(in_stack->card_widget_table);
    if (in_stack->bkgnd_widget_table) idtable_destroy(in_stack->bkgnd_widget_table);
    if (in_stack->stack_card_table) idtable_destroy(in_stack->stack_card_table);
    
    /* undo */
    _undo_stack_destroy(in_stack);
    
    /* find */
    stack_reset_find(in_stack);
    
    /* temporary results */
    if (in_stack->result) _stack_free(in_stack->result);
    if (in_stack->widget_result) _stack_free(in_stack->widget_result);
    if (in_stack->searchable) _stack_free(in_stack->searchable);
    if (in_stack->formatted) _stack_free(in_stack->formatted);
    if (in_stack->serializer_card) serbuff_destroy(in_stack->serializer_card, 1);
    if (in_stack->serializer_widgets) serbuff_destroy(in_stack->serializer_widgets, 1);
    if (in_stack->ids_result) _stack_free(in_stack->ids_result);
    if (in_stack->picture_data) _stack_free(in_stack->picture_data);
    if (in_stack->name) _stack_free(in_stack->name);
    if (in_stack->_returned_script) _stack_free(in_stack->_returned_script);
    if (in_stack->_returned_checkpoints) _stack_free(in_stack->_returned_checkpoints);
    
    if (in_stack->_returned_res_str) _stack_free(in_stack->_returned_res_str);
    if (in_stack->_returned_res_data) _stack_free(in_stack->_returned_res_data);
    
    /* pathname */
    if (in_stack->pathname) _stack_free(in_stack->pathname);
    
#if STACK_TESTS
    /* check that we haven't leaked at all, during operation,
     and that dispose() was successful. */
    if (_stack_mem_allocated() != in_stack->allocated_bytes)
    {
        printf("WARNING! Stack: leak detected on _close(): %ld bytes\n", _stack_mem_allocated() - in_stack->allocated_bytes);
        printf("Note this test only works reliably when there is only 1 document at present.\n"
               "Disregard if you've opened multiple windows.\n");
        //abort();
    }
    else
        printf("Stack close()'d without leaks.\n");
#endif
    
    /* data structure itself */
    _stack_free(in_stack);
}


int stack_file_is_locked(Stack *in_stack)
{
    return in_stack->readonly;
}


int stack_is_writable(Stack *in_stack)
{
    return ((in_stack->readonly == 0) && (in_stack->soft_lock == 0));
}


void _stack_card_table_flush(Stack *in_stack);


void stack_compact(Stack *in_stack)
{
    if (in_stack->readonly) return;
    
    _stack_card_table_flush(in_stack);
    
    sqlite3_exec(in_stack->db, "VACUUM", NULL, NULL, NULL);
}













