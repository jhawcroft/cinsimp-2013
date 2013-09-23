/*
 
 Stack File Format
 stack.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Implements the stack file format API.  Features include:
 -  Data and layout storage
 -  Find
 -  Undo management
 
 *************************************************************************************************
 Out-of-Memory Protocol
 -------------------------------------------------------------------------------------------------
 Stack will invoke an app_out_of_memory_xxx function from JHApp.m in the event it is unable to 
 allocate memory.  Whatever process is executing will abort, leaving the underlying stack file
 in a consistent state, and without leaking.
 
 ** TODO ** CHECK THAT THIS IS ACTUALLY HAPPENING **
    will need to better handle errors of sqlite - any error in a specific routine should result in
    any call to _save() failing and being translated into a _cancel() instead, since if one
    statement fails the change being committed could leave the DB in an inconsistent state.
 
 
 RESOLUTION: should replace direct calls to app_out_of-Memory() throughout this module 
 with calls via an internal function & callback hook.  Such that the stack will know what is 
 going on without having to further modify malloc() or introduce complexities in an otherwise
 simple return statement.
 
 
 app_out_of_memory_void() and similar
 
 ** TODO ** suggest we actually call these from within the stack allocator
 I generally don't want to try and recover at the moment, an error and application termination 
 would probably be acceptable, if on a slight delay to allow for the current call chain
 to complete and tidy up files, etc.  Consider further.
 
 
 SQLite
 -------------------------------------------------------------------------------------------------
 We use SQLite.  More research will be needed in case we load Xternals in future that also include
 a SQLite library to make sure we don't have any kind of symbol conflict ***
 */


#ifndef JH_CINSIMP_STACK_H
#define JH_CINSIMP_STACK_H


#include "xtalk_engine.h"



/*********
 Limits
 */

/*
 *  _STACK_CARD_LIMIT
 *  ---------------------------------------------------------------------------------------------
 *  The maximum number of cards a stack is allowed.
 *
 *  ** TODO ** further testing to see just how well the various components handle a stack with
 *  several million cards, eg. the speed of ID tables for lookups and mutations primarily.
 *
 *  This is limited for a few reasons,
 *  i)   the amount of memory required for the sequence tables of a stack with a large number of
 *  cards becomes an issue, even on modern systems with multiple gigabytes of RAM.
 *  ii)  we can (eventually) detect run-away scripts creating massive numbers of cards and halt
 *  them in their tracks.
 *  iii) we can publish a precise limit in technical specifications for CinsImp
 */

#define _STACK_CARD_LIMIT 10000000 /* 10 million */


/*
 *  _LAYER_WIDGET_LIMIT
 *  ---------------------------------------------------------------------------------------------
 *  The maximum number of widgets a card/background layer is allowed.
 *
 *  This is limited for a couple of reasons,
 *  i)   we can (eventually) detect run-away scripts creating massive numbers of cards and halt
 *  them in their tracks.
 *  iii) we can publish a precise limit in technical specifications for CinsImp
 */

#define _LAYER_WIDGET_LIMIT 30000 /* 30 thousand */








/******************
 constants
 */

#define STACK_YES 1
#define STACK_NO 0


#define STACK_NO_OBJECT -1


enum Widget
{
    WIDGET_BUTTON_PUSH = 0,
    WIDGET_BUTTON_TRANSPARENT,
    WIDGET_FIELD_TEXT,
    WIDGET_FIELD_CHECK,
    WIDGET_FIELD_PICKLIST,
    WIDGET_FIELD_GRID,
};

/* the integer value of these is used internally by our database
 so we can't rearrange/change willynilly... needs review */
enum Property
{
    PROPERTY_NAME,
    PROPERTY_STYLE,
    PROPERTY_SCRIPT,
    PROPERTY_BORDER,
    PROPERTY_CONTENT,
    PROPERTY_LOCKED,
    PROPERTY_DONTSEARCH,
    PROPERTY_SHARED,
    PROPERTY_TEXTFONT,
    PROPERTY_TEXTSIZE,
    PROPERTY_TEXTSTYLE,
    PROPERTY_CANTPEEK,
    PROPERTY_CANTDELETE,
    PROPERTY_CANTABORT,
    PROPERTY_PRIVATE,
    PROPERTY_PASSWORD,
    PROPERTY_USERLEVEL,
    PROPERTY_RICHTEXT,
    PROPERTY_SCRIPTEDITORRECT,
    PROPERTY_ICON,
};

typedef enum FindMode
{
    /* Token modes; matches multiple tokens using boolean AND, but tokens do not
     have to occur consecutively or even in the same field, only the first token is
     highlighted and reported in the found text/chunk properties */
    
    /* finds words that begin with the words of the text (HyperCard 'normal') */
    FIND_WORDS_BEGINNING,
    
    /* finds words that contain the words of the text anywhere (HyperCard 'chars') */
    FIND_WORDS_CONTAINING,
    
    /* finds words that exactly match words of the text (HyperCard 'words') */
    FIND_WHOLE_WORDS,
    
    /* Phrase modes; matches a consecutive range of tokens, all of which must occur within 
     the same place within the same field */
    
    /* finds any consecutive range of characters that matches the text exactly (HyperCard 'string') */
    FIND_CHAR_PHRASE,
    
    /* finds any consecutive range of whole words that matches the text exactly (HyperCard 'whole') */
    FIND_WORD_PHRASE,
    
} StackFindMode;


typedef enum StackSortMode
{
    STACK_SORT_ASCENDING,
    STACK_SORT_DESCENDING,
} StackSortMode;


/******************
 error and status codes
 */

/* this should be replaced by additional error codes as below and phased out */
typedef enum StackOpenStatus
{
    STACK_OK = 0,
    STACK_RECOVERED,
    STACK_UNKNOWN,
    STACK_PERMISSION,
    STACK_INVALID, /* ie. not a stack */
    STACK_CORRUPT,
    STACK_TOO_NEW,
    STACK_TOO_OLD,
} StackOpenStatus;


/*
 *  No Error
 *  ---------------------------------------------------------------------------------------------
 */
#define STACK_ERR_NONE 0

/*
 *  Recoverable Errors
 *  ---------------------------------------------------------------------------------------------
 *  These errors are minor or can be recovered from easily.
 */
#define STACK_ERR_UNKNOWN 1
#define STACK_ERR_TOO_MANY_CARDS 2
#define STACK_ERR_TOO_MANY_WIDGETS 3
#define STACK_ERR_NO_OBJECT 4

/*
 *  Fatal Errors
 *  ---------------------------------------------------------------------------------------------
 *  Recovery is not advised for these errors.
 */
#define STACK_ERR_MEMORY -2
#define STACK_ERR_IO -3


/*
 *  Object Types
 *  ---------------------------------------------------------------------------------------------
 *  STACK_SELF references the Stack itself and the ID parameter to functions using one of these
 *  types will be ignored in that case.
 */
#define STACK_SELF 0
#define STACK_BKGND 1
#define STACK_CARD 2
#define STACK_WIDGET 3


/******************
 types
 */

typedef struct Stack Stack;

typedef void (*StackFatalErrorHandler) (Stack *in_stack, void *in_context, int in_error);




/******************
 Stack
 */

/* creation, opening and closing */

Stack* stack_create(char const *in_path, long in_card_width, long in_card_height, StackFatalErrorHandler in_fatal_handler, void *in_error_context);
Stack* stack_open(char const *in_path, StackFatalErrorHandler in_fatal_handler, void *in_error_context, StackOpenStatus *out_status);
void stack_set_fatal_error_handler(Stack *in_stack, StackFatalErrorHandler in_fatal_handler, void *in_context);
void stack_close(Stack *in_stack);
int stack_file_is_locked(Stack *in_stack);
int stack_is_writable(Stack *in_stack);

char const* stack_name(Stack *in_stack);


/* stack file utilities */

void stack_compact(Stack *in_stack);


/* card and window sizes */

void stack_set_card_size(Stack *in_stack, long in_width, long in_height);
void stack_get_card_size(Stack *in_stack, long *out_width, long *out_height);

void stack_get_window_size(Stack *in_stack, long *out_width, long *out_height);
void stack_set_window_size(Stack *in_stack, long in_width, long in_height);


/* card sorting */
void stack_sort(Stack *in_stack, XTE *in_xtalk, long in_bkgnd_id, int in_marked,
                XTEVariant *in_sort_key, StackSortMode in_mode);

/* return the card ID currently being sorted, or STACK_NO_OBJECT if no sorting is occurring;
 can be used by glue code in the application to return an appropriate value to 
 the engine for 'this card' and references to objects where no card is specified */
long stack_sorting_card(Stack *in_stack);



/******************
 Scripts
 */

/*
 *  stack_script_get
 *  ---------------------------------------------------------------------------------------------
 *  Access the script and checkpoints for a specific object.
 *
 *  The object is identified by <in_type> and <in_id>, where <in_type> can be:
 *  STACK_SELF, STACK_CARD, STACK_BKGND or STACK_WIDGET.
 *
 *  The output is static and will remain valid between successive calls.
 *
 *  Returns either STACK_ERR_NONE if successful, or STACK_ERR_NO_OBJECT if the object no longer
 *  exists.  May also return STACK_ERR_MEMORY if an out of memory condition occurs, and in such
 *  a case, the condition will be reported via standard out of memory protocol (see notes.)
 */
int stack_script_get(Stack *in_stack, int in_type, long in_id, char const **out_script,
                     int const *out_checkpoints[], int *out_checkpoint_count, long *out_sel_offset);

/*
 *  stack_script_set
 *  ---------------------------------------------------------------------------------------------
 *  Mutate the script and checkpoints of a specific object.
 *
 *  The object is identified by <in_type> and <in_id>, where <in_type> can be:
 *  STACK_SELF, STACK_CARD, STACK_BKGND or STACK_WIDGET.
 *
 *  Inputs are assumed to be transient and are saved prior to call return.  Caller retains 
 *  ownership of inputs.
 *
 *  Returns either STACK_ERR_NONE if successful, or STACK_ERR_NO_OBJECT if the object no longer
 *  exists.
 */
int stack_script_set(Stack *in_stack, int in_type, long in_id, char const *in_script,
                     int const in_checkpoints[], int in_checkpoint_count, long in_sel_offset);



/******************
 Backgrounds
 */

long stack_bkgnd_create(Stack *in_stack, long in_after_card_id, int *out_error);

long stack_bkgnd_card_count(Stack *in_stack, long in_bkgnd_id);


/******************
 Cards
 */


long stack_card_create(Stack *in_stack, long in_after_card_id, int *out_error);
long stack_card_delete(Stack *in_stack, long in_card_id);

long stack_card_count(Stack *in_stack);

long stack_card_id_for_index(Stack *in_stack, long in_index);
long stack_card_index_for_id(Stack *in_stack, long in_id);

long stack_card_id_for_name(Stack *in_stack, char const *in_name);

long stack_card_bkgnd_id(Stack *in_stack, long in_card_id);




/******************
 Stack, Cards, Bkgnds
 */

const char* stack_prop_get_string(Stack *in_stack, long in_card_id, long in_bkgnd_id, enum Property in_prop);
long stack_prop_get_long(Stack *in_stack, long in_card_id, long in_bkgnd_id, enum Property in_prop);

void stack_prop_set_string(Stack *in_stack, long in_card_id, long in_bkgnd_id, enum Property in_prop, char* in_string);
void stack_prop_set_long(Stack *in_stack, long in_card_id, long in_bkgnd_id, enum Property in_prop, long in_long);



/******************
 Layer Pictures
 */

long stack_layer_picture_get(Stack *in_stack, long in_card_id, long in_bkgnd_id, void **out_data, int *out_visible);
void stack_layer_picture_set(Stack *in_stack, long in_card_id, long in_bkgnd_id, void *in_data, long in_size);


/******************
 Resources (General)
 */

int stack_res_type_count(Stack *in_stack);
char const* stack_res_type_n(Stack *in_stack, int in_number);
int stack_res_count(Stack *in_stack, char const *in_type);
int stack_res_n(Stack *in_stack, char const *in_type, int in_number);
int stack_res_get(Stack *in_stack, int in_id, char const *in_type, char **out_name, void **out_data, long *out_size);
int stack_res_create(Stack *in_stack, int in_id, char *in_type, char *in_name);
int stack_res_set(Stack *in_stack, int in_id, char const *in_type, int const *in_new_id, char const *in_new_name,
                  void const *in_data, long in_size);
int stack_res_delete(Stack *in_stack, int in_id, char const *in_type);


/******************
 Widgets
 */

/* widget creation, destruction and enumeration */

long stack_create_widget(Stack *in_stack, enum Widget in_type, long in_card_id, long in_bkgnd_id, int *out_error);
void stack_delete_widget(Stack *in_stack, long in_widget_id);

long stack_widget_count(Stack *in_stack, long in_card_id, long in_bkgnd_id);
long stack_widget_n(Stack *in_stack, long in_card_id, long in_bkgnd_id, long in_widget_index);
long stack_widget_named(Stack *in_stack, long in_card_id, long in_bkgnd_id, char const *in_name);
long stack_widget_id(Stack *in_stack, long in_card_id, long in_bkgnd_id, long in_id);
int stack_widget_basics(Stack *in_stack, long in_widget_id, long *out_x, long *out_y, long *out_width, long *out_height,
                         int *out_hidden, enum Widget *out_type);
int stack_widget_is_field(Stack *in_stack, long in_widget_id);
int stack_widget_is_card(Stack *in_stack, long in_widget_id);

void stack_widget_owner(Stack *in_stack, long in_widget_id, long *out_card_id, long *out_bkgnd_id);


/* tab sequence access */

long stack_widget_next(Stack *in_stack, long in_widget_id, long in_card_id, int in_bkgnd);
long stack_widget_previous(Stack *in_stack, long in_widget_id, long in_card_id, int in_bkgnd);


/* widget direct layout manipulation */

void stack_widget_set_rect(Stack *in_stack, long in_widget_id, long in_x, long in_y, long in_width, long in_height);
void stack_widgets_send_front(Stack *in_stack, long *in_widgets, int in_count);
void stack_widgets_send_back(Stack *in_stack, long *in_widgets, int in_count);
void stack_widgets_shuffle_forward(Stack *in_stack, long *in_widgets, int in_count);
void stack_widgets_shuffle_backward(Stack *in_stack, long *in_widgets, int in_count);


/* widget direct content manipulation */

void stack_widget_content_get(Stack *in_stack, long in_widget_id, long in_card_id, int in_bkgnd, char **out_searchable,
                              char **out_formatted, long *out_formatted_size, int *out_editable);
void stack_widget_content_set(Stack *in_stack, long in_widget_id, long in_card_id, char *in_searchable,
                              char *in_formatted, long in_formatted_size);


/* widget property access */

const char* stack_widget_prop_get_string(Stack *in_stack, long in_widget_id, long in_card_id, enum Property in_prop);
long stack_widget_prop_get_long(Stack *in_stack, long in_widget_id, long in_card_id, enum Property in_prop);

void stack_widget_prop_set_string(Stack *in_stack, long in_widget_id, long in_card_id, enum Property in_prop, char *in_string);
void stack_widget_prop_set_long(Stack *in_stack, long in_widget_id, long in_card_id, enum Property in_prop, long in_long);


/* widget cut, copy and paste;
 eventually clone */

long stack_widget_copy(Stack *in_stack, long *in_ids, int in_count, void **out_data);
long stack_widget_cut(Stack *in_stack, long *in_ids, int in_count, void **out_data);
int stack_widget_paste(Stack *in_stack, long in_card_id, long in_bkgnd_id, void *in_data, long in_size, long **out_ids);


/******************
 Undo
 */

const char* stack_can_undo(Stack *in_stack);
const char* stack_can_redo(Stack *in_stack);

void stack_undo(Stack *in_stack, long *out_card_id);
void stack_redo(Stack *in_stack, long *out_card_id);

void stack_undo_activity_begin(Stack *in_stack, const char *in_description, long in_card_id);
void stack_undo_activity_end(Stack *in_stack);
void stack_undo_flush(Stack *in_stack);


/******************
 Find
 */

void stack_reset_find(Stack *in_stack);
void stack_find(Stack *in_stack, long in_card_id, enum FindMode in_mode, char *in_search, long in_field_id, int in_marked);

int stack_find_step(Stack *in_stack);

int stack_find_result(Stack *in_stack, long *out_card_id, long *out_field_id, long *out_offset, long *out_length, char **out_text);
const char* stack_find_terms(Stack *in_stack);





#if DEBUG
#define STACK_TESTS 1
void stack_test(void);
#endif


#endif
