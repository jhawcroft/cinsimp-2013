/*
 
 Stack File Format
 stack_int.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Internal API for this module.  See the stack.h file for a description of the module.
 
 */

#ifndef JH_CINSIMP_STACK_INT_H
#define JH_CINSIMP_STACK_INT_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "sqlite3.h"

#include "stack.h"

#include "jh_c_int.h"






/* ID table:
 */

typedef struct IDTable IDTable;

#define IDTABLE_OK              0
#define IDTABLE_ERROR_MEMORY    1


IDTable* idtable_create(Stack *in_stack);
IDTable* idtable_create_with_ascii(Stack *in_stack, const char *in_ascii);
void idtable_destroy(IDTable *in_table);

const char* idtable_to_ascii(IDTable *in_table);

void idtable_insert(IDTable *in_table, long in_id, long in_at_index);
void idtable_append(IDTable *in_table, long in_id);
void idtable_remove(IDTable *in_table, long in_index);

long idtable_size(IDTable *in_table);

long idtable_index_for_id(IDTable *in_table, long in_id);
long idtable_id_for_index(IDTable *in_table, long in_index);

void idtable_mutate_slot(IDTable *in_table, long in_index, long in_new_id);

void idtable_send_front(IDTable *in_table, long *in_ids, int in_count);
void idtable_send_back(IDTable *in_table, long *in_ids, int in_count);
void idtable_shuffle_forward(IDTable *in_table, long *in_ids, int in_count);
void idtable_shuffle_backward(IDTable *in_table, long *in_ids, int in_count);


void idtable_clear(IDTable *in_table);

#define STACK_NO_COPY 1
#define STACK_COPY 0


/* serialisation buffer */

typedef struct SerBuff SerBuff;

SerBuff* serbuff_create(Stack *in_stack, void *in_data, long in_size, int in_nocopy);
long serbuff_data(SerBuff *in_buff, void **out_data);
void serbuff_destroy(SerBuff *in_buff, int in_cleanup_data);

void serbuff_write_long(SerBuff *in_buff, long in_long);
void serbuff_write_cstr(SerBuff *in_buff, const char *in_cstr);
void serbuff_write_data(SerBuff *in_buff, void const *in_data, long in_size);

void serbuff_reset(SerBuff *in_buff);

long serbuff_read_long(SerBuff *in_buff);
const char* serbuff_read_cstr(SerBuff *in_buff);
long serbuff_read_data(SerBuff *in_buff, void **out_data);


/* undo management */

enum UndoAction
{
    UNDO_WIDGET_CREATE,
    UNDO_WIDGET_DELETE,
    UNDO_WIDGET_SIZE,
    UNDO_WIDGET_CONTENT,
    UNDO_WIDGET_PROPERTY_STRING,
    UNDO_WIDGET_PROPERTY_LONG,
    UNDO_PROPERTY_LONG,
    UNDO_PROPERTY_STRING,
    UNDO_REARRANGE_WIDGETS,
    UNDO_CARD_CREATE,
    UNDO_CARD_DELETE,
};


struct UndoStep
{
    enum UndoAction action;
    SerBuff *data;
};


struct UndoFrame
{
    
    char *description;
    struct UndoStep *steps;
    int step_count;
    long card_id;
};



/* stack instance */

#define _STACK_STRUCT_ID "STAK"

#define IS_STACK(x) ((x != NULL) && (strcmp(x->struct_id, _STACK_STRUCT_ID) == 0))
#define BLESS_STAK(x) strcpy(x->struct_id, _STACK_STRUCT_ID);

#define IS_BOOL(x) ((x == !0) || (x == !!0))


struct Stack
{
    char struct_id[sizeof(_STACK_STRUCT_ID)+1];
    
    char *pathname;
    
#if STACK_TESTS
    /* memory checks */
    long allocated_bytes;
#endif
    
    /* managed return values;
     these are temporary strings that are allocated when accessing a property
     */
    char *result;
    char *widget_result;
    char *searchable;
    char *formatted;
    SerBuff *serializer_card;
    SerBuff *serializer_widgets;
    long *ids_result;
    void *picture_data;
    char *name;
    char *_returned_script;
    int *_returned_checkpoints;
    char *_returned_res_str;
    void *_returned_res_data;
    
    
    /* file I/O */
    
    StackFatalErrorHandler fatal_handler;
    void *fatal_context;
    int has_fatal_error;
    
    int has_begun;
    sqlite3 *db;
    int readonly; /* is the file itself locked from outside CinsImp? */
    int soft_lock; /* user has set Can't Modify to true, this is a cache of the property */
    
    
    /* caches of frequently used data structures */
    
    long cached_card_id;
    IDTable *card_widget_table;
    long cached_bkgnd_id;
    IDTable *bkgnd_widget_table;
    IDTable *stack_card_table;
    
    
    /* undo management */
    
    struct UndoFrame *undo_stack;
    int undo_stack_size;
    int undo_stack_top;
    int undo_redo_ptr;
    int record_undo_steps;
    
    
    /* unserialisation and widget creation */
    
    long override_widget_id;
    
    
    /* sort */
    
    long sort_card_id;
    
    
    /* find */
    
    long found_card_id;
    long found_field_id;
    long found_offset;
    long found_length;
    char *found_text;
    
    int found_match_this_search;
    
    int matches_this_search;
    int trace;
    
    enum {
        _FIND_GET_NEXT_CARD,
        _FIND_CARD_SETUP,
        _FIND_GET_CARD_FIELDS,
        
        _FIND_WORDS_FIRST, /* if ((in_stack->find_mode == FIND_WHOLE_WORDS) ||
                            (in_stack->find_mode == FIND_WORDS_BEGINNING) ||
                            (in_stack->find_mode == FIND_WORDS_CONTAINING))*/
        _FIND_WORDS_OTHER,
        
        _FIND_WORD_PHRASE,
        _FIND_CHAR_PHRASE,
        
        _FIND_CHECK_ANY_RESULT,
        _FIND_FINISH,
        
    }   find_state;
    
    long find_card_id;
    enum FindMode find_mode;
    char *find_search;
    long find_search_bytes;
    char **find_words;
    int find_word_count;
    long find_field_id;
    int find_marked;
    
    int find_stop_card;
    long find_restrict_bkgnd;
    
    long find_state_card_id;
    long *find_state_fields;
    int find_state_field_count;
    long find_state_field_index;
    long find_state_field_offset;
    long find_state_field_length;
    int find_state_field_word_index;
    
    char *find_state_field_content;
    char **find_state_field_words;
    long *find_state_field_offsets;
    int find_state_field_word_count;
};


/* fatal error conditions */

void _stack_file_error_void(Stack *in_stack);
void* _stack_file_error_null(Stack *in_stack);



void _stack_panic_void(Stack *in_stack, int in_error_code);
int _stack_panic_err(Stack *in_stack, int in_error_code);
int _stack_panic_false(Stack *in_stack, int in_error_code);
void* _stack_panic_null(Stack *in_stack, int in_error_code);



/* utilities */

char* _stack_clone_cstr(char const *in_string);
int _stack_str_same(char *in_string_1, char *in_string_2);

void _stack_wordize(char *in_string, char **out_items[], long **out_offsets, int *out_count);
void _stack_items_free(char **in_string, int in_count);

int _stack_utf8_compare(const char *in_string_1, const char *in_string_2);


/* caches */

void _stack_widget_seq_cache_load(Stack *in_stack, long in_card_id, long in_bkgnd_id);
IDTable* _stack_widget_seq_get(Stack *in_stack, long in_card_id, long in_bkgnd_id);
void _stack_widget_seq_set(Stack *in_stack, long in_card_id, long in_bkgnd_id, IDTable *in_list);
void _stack_widget_cache_invalidate(Stack *in_stack);


/* serialization */

long _widgets_serialize(Stack *in_stack, long *in_ids, int in_count, void **out_data, int in_incl_content);
int _widgets_unserialize(Stack *in_stack, void *in_data, long in_data_size, long in_card_id, long in_bkgnd_id,
                         long const **out_ids, int in_incl_content, int in_undoable, int in_only_content);

long _card_serialize(Stack *in_stack, long in_card_id, void const **out_data);
long _card_unserialize(Stack *in_stack, void const *in_data, long in_size, long in_after_card_id);


/* widgets */

long _widget_layer_id(Stack *in_stack, long in_widget_id);


/* cards */

long _cards_bkgnd(Stack *in_stack, long in_card_id);


/* undo */

void _undo_stack_destroy(Stack *in_stack);
void _undo_stack_create(Stack *in_stack);
void _undo_record_step(Stack *in_stack, enum UndoAction in_action, SerBuff *in_data);


/* find */

void stack_reset_find(Stack *in_stack);



/* underlying file I/O */
#define STACK_ENTRY_POINT 1
#define STACK_NO_FLAGS 0
void _stack_begin(Stack *in_stack, int in_is_entry_point);
int _stack_commit(Stack *in_stack);
void _stack_cancel(Stack *in_stack);



/* custom memory allocator */
void* _stack_malloc(long in_bytes);
void* _stack_calloc(long in_number, long in_size);
void* _stack_realloc(void *in_memory, long in_size);
void _stack_free(void *in_memory);

#if STACK_TESTS
long _stack_mem_allocated(void);
#endif


#endif
