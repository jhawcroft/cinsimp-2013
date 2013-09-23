/*
 
 CinsImp <http://www.cinsimp.net>
 Copyright (C) 2009-2013 Joshua Hawcroft
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 *************************************************************************************************
 JHCinsImp: Stack Unit: Resource Management
 
 Provides the ability to index, store and retrieve 'resources' in a scheme inspired by the classic
 MacOS Resource Manager, stored within the data fork of the stack.
 
 Resources can be anything; version 1.0 will use this functionality primarily (exclusively?)
 for icons.
 
 */

#include "stack_int.h"


/*
 *  stack_res_type_count
 *  ---------------------------------------------------------------------------------------------
 *  Returns the number of unique resource types within the stack.  Use with stack_res_type_n()
 *  to enumerate the various types of resources saved within the stack.
 */
int stack_res_type_count(Stack *in_stack)
{
    assert(IS_STACK(in_stack));
    
    int result = 0;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "SELECT COUNT(DISTINCT resourcetype) FROM resource ORDER BY resourcetype", -1, &stmt, NULL);
    int err = sqlite3_step(stmt);
    if (err == SQLITE_ROW) result = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    return result;
}


/*
 *  stack_res_type_n
 *  ---------------------------------------------------------------------------------------------
 *  Returns the name of the resource type with the specified number.  Use with
 *  stack_res_type_count() to enumerate the various types of resources saved within the stack.
 *
 *  The result string remains valid until the next call, the stack is closed or stack_res_get()
 *  is invoked.
 *
 *  Returns NULL if no such type exists.
 */
char const* stack_res_type_n(Stack *in_stack, int in_number)
{
    assert(IS_STACK(in_stack));
    assert(in_number >= 1);
    
    if (in_stack->_returned_res_str) _stack_free(in_stack->_returned_res_str);
    in_stack->_returned_res_str = NULL;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "SELECT DISTINCT resourcetype FROM resource ORDER BY resourcetype LIMIT ?1,1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, in_number-1);
    int err = sqlite3_step(stmt);
    if (err == SQLITE_ROW)
    {
        in_stack->_returned_res_str = _stack_clone_cstr((char*)sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    
    return in_stack->_returned_res_str;
}


/*
 *  stack_res_count
 *  ---------------------------------------------------------------------------------------------
 *  Returns the number of resources with the specified type. Use with stack_res_count() to 
 *  enumerate all resources of a specified type.
 */
int stack_res_count(Stack *in_stack, char const *in_type)
{
    assert(IS_STACK(in_stack));
    assert(in_type != NULL);
    
    int result = 0;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "SELECT COUNT(*) FROM resource WHERE resourcetype=?1", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, in_type, -1, SQLITE_STATIC);
    int err = sqlite3_step(stmt);
    if (err == SQLITE_ROW) result = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    return result;
}


/*
 *  stack_res_n
 *  ---------------------------------------------------------------------------------------------
 *  Returns the ID of the resource with the specified type and number.  Use with 
 *  stack_res_count() to enumerate all resources of a specified type.
 *
 *  Returns STACK_NO_OBJECT if there is no resource with the supplied number and/or type pairing,
 *  or if there is an I/O error.
 */
int stack_res_n(Stack *in_stack, char const *in_type, int in_number)
{
    assert(IS_STACK(in_stack));
    assert(in_type != NULL);
    assert(in_number >= 1);
    
    int result = STACK_NO_OBJECT;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "SELECT resourceid FROM resource WHERE resourcetype=?1 ORDER BY resourceid LIMIT ?2,1", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, in_type, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, in_number-1);
    int err = sqlite3_step(stmt);
    if (err == SQLITE_ROW) result = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    return result;
}


/*
 *  stack_res_get
 *  ---------------------------------------------------------------------------------------------
 *  Accesses the details and/or data of the resource with the specified ID and type.
 *  Returns STACK_YES on success, STACK_NO otherwise.
 *
 *  Only returns the arguments for which a non-NULL pointer is supplied.
 *
 *  The result string and data remain valid until the next call, the stack is closed or 
 *  stack_res_type_n() is invoked.
 *
 *  An error can occur because the resource cannot be found or because of an I/O error.
 */
int stack_res_get(Stack *in_stack, int in_id, char const *in_type, char **out_name, void **out_data, long *out_size)
{
    assert(IS_STACK(in_stack));
    assert(in_id >= 1);
    
    if (in_stack->_returned_res_str) _stack_free(in_stack->_returned_res_str);
    in_stack->_returned_res_str = NULL;
    if (in_stack->_returned_res_data) _stack_free(in_stack->_returned_res_data);
    in_stack->_returned_res_data = NULL;
    long data_size = 0;
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "SELECT resourcename,data FROM resource WHERE resourceid=?1 AND resourcetype=?2", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, in_id);
    sqlite3_bind_text(stmt, 2, in_type, -1, SQLITE_STATIC);
    int err = sqlite3_step(stmt);
    if (err == SQLITE_ROW) {
        in_stack->_returned_res_str = _stack_clone_cstr((char*)sqlite3_column_text(stmt, 0));
        data_size = sqlite3_column_bytes(stmt, 1);
        in_stack->_returned_res_data = _stack_malloc(data_size);
        if (!in_stack->_returned_res_data)
        {
            _stack_panic_void(in_stack, STACK_ERR_MEMORY);
            sqlite3_finalize(stmt);
            return STACK_NO;
        }
        else
            memcpy(in_stack->_returned_res_data, sqlite3_column_blob(stmt, 1), data_size);
    }
    sqlite3_finalize(stmt);
    
    if (err == SQLITE_ROW)
    {
        if (out_name) *out_name = in_stack->_returned_res_str;
        if (out_data) *out_data = in_stack->_returned_res_data;
        if (out_size) *out_size = data_size;
        return STACK_YES;
    }
    else
        return STACK_NO;
}


/*
 *  stack_res_set
 *  ---------------------------------------------------------------------------------------------
 *  Mutates an existing resource with the specified ID and type.
 *  Returns STACK_YES on success, STACK_NO otherwise.
 *
 *  Only mutates the arguments for which a non-NULL pointer is supplied.  For example, to mutate
 *  only the name, supply a NULL new ID and data pointer.
 *
 *  An error can occur if there's a file I/O error, or if you try to change the type and ID to
 *  that already used by an existing resource.
 */
int stack_res_set(Stack *in_stack, int in_id, char const *in_type, int const *in_new_id, char const *in_new_name,
                  void const *in_data, long in_size)
{
    assert(IS_STACK(in_stack));
    assert(in_id >= 1);
    
    _stack_begin(in_stack, STACK_ENTRY_POINT);
    
    int existing = STACK_NO;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "SELECT resourceid FROM resource WHERE resourceid=?1 AND resourcetype=?2", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, in_id);
    sqlite3_bind_text(stmt, 2, in_type, -1, SQLITE_STATIC);
    int err = sqlite3_step(stmt);
    if (err == SQLITE_ROW) existing = STACK_YES;
    sqlite3_finalize(stmt);
    
    if (existing)
    {
        err = SQLITE_DONE;
        
        if (in_data)
        {
            sqlite3_prepare_v2(in_stack->db, "UPDATE resource SET data=?3 WHERE resourceid=?1 AND resourcetype=?2", -1, &stmt, NULL);
            sqlite3_bind_int(stmt, 1, in_id);
            sqlite3_bind_text(stmt, 2, in_type, -1, SQLITE_STATIC);
            sqlite3_bind_blob(stmt, 3, in_data, (int)in_size, SQLITE_STATIC);
            if (err == SQLITE_DONE) err = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        if (in_new_name)
        {
            sqlite3_prepare_v2(in_stack->db, "UPDATE resource SET resourcename=?3 WHERE resourceid=?1 AND resourcetype=?2", -1, &stmt, NULL);
            sqlite3_bind_int(stmt, 1, in_id);
            sqlite3_bind_text(stmt, 2, in_type, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, in_new_name, -1, SQLITE_STATIC);
            if (err == SQLITE_DONE) err = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        if (in_new_id)
        {
            sqlite3_prepare_v2(in_stack->db, "UPDATE resource SET resourceid=?3 WHERE resourceid=?1 AND resourcetype=?2", -1, &stmt, NULL);
            sqlite3_bind_int(stmt, 1, in_id);
            sqlite3_bind_text(stmt, 2, in_type, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, *in_new_id);
            if (err == SQLITE_DONE) err = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    else
        err = SQLITE_ERROR;
    
    _stack_commit(in_stack);
    
    return (err == SQLITE_DONE);
}


/*
 *  stack_res_delete
 *  ---------------------------------------------------------------------------------------------
 *  Deletes an existing resource with the specified ID and type.
 *  Returns STACK_YES on success, STACK_NO otherwise.
 *
 *  An error can occur because the resource doesn't exist or there's an I/O error.
 */
int stack_res_delete(Stack *in_stack, int in_id, char const *in_type)
{
    assert(IS_STACK(in_stack));
    assert(in_id >= 1);
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "DELETE FROM resource WHERE resourceid=?1 AND resourcetype=?2", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, in_id);
    sqlite3_bind_text(stmt, 2, in_type, -1, SQLITE_STATIC);
    int err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (err != SQLITE_DONE) return STACK_NO;
    else return STACK_YES;
}


/*
 *  stack_res_create
 *  ---------------------------------------------------------------------------------------------
 *  Creates a new resource with the specified ID, type and name.
 *  Returns the ID if creation was successful, STACK_NO_OBJECT otherwise.
 *
 *  You must specify all arguments.  Type must be an ASCII string with 1+ characters.  If name is
 *  to be empty, you must supply an empty string.
 *
 *  If you specify id as 0 or STACK_NO_OBJECT, the next available ID will be assigned 
 *  automatically.
 *
 *  If unsuccessful, either your arguments are wrong or a resource with the specified type and ID
 *  already exists, or there was an I/O error.
 */
int stack_res_create(Stack *in_stack, int in_id, char *in_type, char *in_name)
{
    assert(IS_STACK(in_stack));
    assert(in_type != NULL);
    assert(in_name != NULL);
    
    if (strlen(in_type) < 1) return STACK_NO;
    
    sqlite3_stmt *stmt;
    int err;
    if (in_id < 1)
    {
        sqlite3_prepare_v2(in_stack->db, "SELECT MAX(resourceid) FROM resource WHERE resourcetype=?1", -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, in_type, -1, SQLITE_STATIC);
        err = sqlite3_step(stmt);
        if (err == SQLITE_ROW) in_id = sqlite3_column_int(stmt, 0) + 1;
        sqlite3_finalize(stmt);
    }
    
    if (in_id < 1) return STACK_NO_OBJECT;
    
    sqlite3_prepare_v2(in_stack->db, "INSERT INTO resource (resourceid,resourcetype,resourcename,data) VALUES "
                       "(?1,?2,?3,NULL)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, in_id);
    sqlite3_bind_text(stmt, 2, in_type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, in_name, -1, SQLITE_STATIC);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (err != SQLITE_DONE) return STACK_NO_OBJECT;
    return in_id;
}


