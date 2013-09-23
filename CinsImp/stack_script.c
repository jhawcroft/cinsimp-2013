/*
 
 Stack File Format
 stack_script.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Script access/mutation
 
 *************************************************************************************************
 
*/

#include "stack_int.h"


//static int test[1] = {2};

int stack_script_get(Stack *in_stack, int in_type, long in_id, char const **out_script,
                     int const *out_checkpoints[], int *out_checkpoint_count, long *out_sel_offset)
{
    //printf("stack_script_get()\n");
    
    sqlite3_stmt *stmt;
    switch (in_type)
    {
        case STACK_SELF:
            sqlite3_prepare_v2(in_stack->db, "SELECT script,script_cp,script_sel FROM stack", -1, &stmt, NULL);
            break;
        case STACK_BKGND:
            sqlite3_prepare_v2(in_stack->db, "SELECT script,script_cp,script_sel FROM bkgnd WHERE bkgndid=?1", -1, &stmt, NULL);
            break;
        case STACK_CARD:
            sqlite3_prepare_v2(in_stack->db, "SELECT script,script_cp,script_sel FROM card WHERE cardid=?1", -1, &stmt, NULL);
            break;
        case STACK_WIDGET:
            sqlite3_prepare_v2(in_stack->db, "SELECT script,script_cp,script_sel FROM widget WHERE widgetid=?1", -1, &stmt, NULL);
            break;
    }
    if (in_type != STACK_SELF) sqlite3_bind_int(stmt, 1, (int)in_id);
    int err = sqlite3_step(stmt);
    if (err == SQLITE_ROW)
    {
        if (in_stack->_returned_script) _stack_free(in_stack->_returned_script);
        in_stack->_returned_script = NULL;
        if (in_stack->_returned_checkpoints) _stack_free(in_stack->_returned_checkpoints);
        in_stack->_returned_checkpoints = NULL;
        
        in_stack->_returned_script = _stack_clone_cstr( (char*)sqlite3_column_text(stmt, 0) );
        
        char const *temp = (char const*)sqlite3_column_text(stmt, 1);
        if (!temp) temp = "";
        long temp_len = strlen(temp);
        int checkpoint_count = (int)(temp_len / 10);
        in_stack->_returned_checkpoints = _stack_malloc(sizeof(int) * checkpoint_count);
        if (in_stack->_returned_checkpoints)
        {
            for (int i = 0; i < checkpoint_count; i++)
            {
                char buffer[11];
                if (i * 10 + 10 > temp_len) break;
                memcpy(buffer, temp + (i * 10), 10);
                buffer[10] = 0;
                in_stack->_returned_checkpoints[i] = atoi(buffer);
            }
        }
        
        *out_script = in_stack->_returned_script;
        *out_checkpoints = in_stack->_returned_checkpoints;
        *out_checkpoint_count = (in_stack->_returned_checkpoints ? checkpoint_count : 0);
        if (out_sel_offset) *out_sel_offset = sqlite3_column_int(stmt, 2);
    }
    sqlite3_finalize(stmt);
    
    if (err == SQLITE_ROW) return STACK_ERR_NONE;
    return STACK_ERR_NO_OBJECT;
}


int stack_script_set(Stack *in_stack, int in_type, long in_id, char const *in_script,
                     int const in_checkpoints[], int in_checkpoint_count, long in_sel_offset)
{
    //printf("stack_script_set()\n");
    
    sqlite3_stmt *stmt;
    switch (in_type)
    {
        case STACK_SELF:
            sqlite3_prepare_v2(in_stack->db, "UPDATE stack SET script=?1,script_cp=?2,script_sel=?3", -1, &stmt, NULL);
            break;
        case STACK_BKGND:
            sqlite3_prepare_v2(in_stack->db, "UPDATE bkgnd SET script=?1,script_cp=?2,script_sel=?3 WHERE bkgndid=?4", -1, &stmt, NULL);
            break;
        case STACK_CARD:
            sqlite3_prepare_v2(in_stack->db, "UPDATE card SET script=?1,script_cp=?2,script_sel=?3 WHERE cardid=?4", -1, &stmt, NULL);
            break;
        case STACK_WIDGET:
            sqlite3_prepare_v2(in_stack->db, "UPDATE widget SET script=?1,script_cp=?2,script_sel=?3 WHERE widgetid=?4", -1, &stmt, NULL);
            break;
    }
    if (in_type != STACK_SELF) sqlite3_bind_int(stmt, 4, (int)in_id);
    sqlite3_bind_text(stmt, 1, in_script, -1, SQLITE_STATIC);
    
    char *temp = _stack_malloc((10 * in_checkpoint_count) + 1);
    if (temp)
    {
        temp[0] = 0;
        for (int i = 0; i < in_checkpoint_count; i++)
        {
            sprintf(temp + (i * 10), "%010d", in_checkpoints[i]);
        }
    }
    sqlite3_bind_text(stmt, 2, (temp ? temp : ""), -1, SQLITE_STATIC);
    
    sqlite3_bind_int(stmt, 3, (int)in_sel_offset);
    int err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (temp) _stack_free(temp);
    
    if (err == SQLITE_DONE) return STACK_ERR_NONE;
    return STACK_ERR_NO_OBJECT;
}



