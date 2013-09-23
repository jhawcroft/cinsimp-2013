/*
 
 Application Control Unit
 acu.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 C-based implementation of the CinsImp application, with hooks for a platform specific 
 user interface and minimal dependencies on platform-specific APIs
 
 *************************************************************************************************
 */

#ifndef CINSIMP_ACU_H
#define CINSIMP_ACU_H

/******************
 Dependencies
 */
#include "stack.h"
#include "xtalk_engine.h" /* despite this, there should be no calls to XTE from the Application */


/******************
 Constants
 */

/*
 *  STACKMGR_INVALID_HANDLE
 *  ---------------------------------------------------------------------------------------------
 *  Invalid handle.  See specific function documentation for use cases and scenarios.
 */
#define STACKMGR_INVALID_HANDLE 0

#define ACU_INVALID_HANDLE 0

#define INVALID_LINE_NUMBER 0


/*
 *  C Boolean Constants
 *  ---------------------------------------------------------------------------------------------
 */
#define STACKMGR_FALSE !!0
#define STACKMGR_TRUE !0

#define ACU_FALSE !!0
#define ACU_TRUE !0


/*
 *  Error/Status Codes
 *  ---------------------------------------------------------------------------------------------
 */

/*
 *  STACKMGR_ERROR_NONE
 *  ---------------------------------------------------------------------------------------------
 *  There is no error.
 */
#define STACKMGR_ERROR_NONE 0

#define ACU_ERROR_NONE 0
#define ACU_ERROR_MEMORY 1

/*
 *  STACKMGR_ERROR_MEMORY
 *  ---------------------------------------------------------------------------------------------
 *  The application is out of memory.
 *  ! This condition is fatal and the application should be terminated after presenting the user
 *  an appropriate error message.
 */
#define STACKMGR_ERROR_MEMORY 1
#define STACKMGR_ERROR_NO_OBJECT 2
#define STACKMGR_ERROR_TOO_MANY_STACKS 3 /* too many open stacks */
#define STACKMGR_ERROR_INTERNAL 4
#define STACKMGR_ERROR_THREADING 5

#define ACU_ERROR_INTERNAL 4


/*
 *  Handle Reference Types
 *  ---------------------------------------------------------------------------------------------
 *  Handles can be obtained referring to these various object types:
 */
#define STACKMGR_TYPE_STACK     1
#define STACKMGR_TYPE_CARD      2
#define STACKMGR_TYPE_BKGND     3
#define STACKMGR_TYPE_FIELD     4
#define STACKMGR_TYPE_BUTTON    5




#define ACU_EFFECT_CUT 0
#define ACU_EFFECT_DISSOLVE 1
#define ACU_EFFECT_WIPE_LEFT 2
#define ACU_EFFECT_WIPE_RIGHT 3
#define ACU_EFFECT_WIPE_UP 4
#define ACU_EFFECT_WIPE_DOWN 5

#define ACU_SPEED_NORMAL 0
#define ACU_SPEED_VERY_SLOW -2
#define ACU_SPEED_SLOW -1
#define ACU_SPEED_FAST 1
#define ACU_SPEED_VERY_FAST 2

#define ACU_DESTINATION_CARD 0
#define ACU_DESTINATION_BLACK 1
#define ACU_DESTINATION_WHITE 2
#define ACU_DESTINATION_GREY 3


/*
 *  Assorted Flags
 *  ---------------------------------------------------------------------------------------------
 */

/*
 *  STACKMGR_FLAG_AUTO_RELEASE
 *  ---------------------------------------------------------------------------------------------
 *  When you request a handle using a stackmgr_handle_for_xxx() function, the first argument is
 *  a flags field.  This flag indicates the returned StackHandle should be auto-released.
 *
 *  Auto-released handles are garbage collected at the next iteration of the event loop  
 *  (whenever stackmgr_arp_drain() is invoked.)
 */
#define STACKMGR_FLAG_AUTO_RELEASE   0x1

/*
 *  STACKMGR_FLAG_MAN_RELEASE
 *  ---------------------------------------------------------------------------------------------
 *  When you request a handle using a stackmgr_handle_for_xxx() function, the first argument is
 *  a flags field.  This flag indicates the returned StackHandle should be manually-released.
 *
 *  Manually-released handles are garbage collected when their reference count reaches zero.
 *  The newly acquired handle will have a reference count of one (1) and so must be released
 *  when the calling code is finished with it.
 */
#define STACKMGR_FLAG_MAN_RELEASE    0x2


/*
 *  STACKMGR_FLAG_LONG
 *  ---------------------------------------------------------------------------------------------
 *  Provide a long description when used with stackmgr_handle_description().
 */
#define STACKMGR_FLAG_LONG          0x1



/******************
 Opaque Types
 */

/*
 *  StackHandle
 *  ---------------------------------------------------------------------------------------------
 *  Opaque handle to a Stack or stack object, ie. card, background, field or button.
 *
 *  Currently backwards compatible with Stack* so that a Stack* can be treated as a handle to a 
 *  stack and visa-versa.  In the far future, may become a void* when the stack unit is no longer
 *  accessed directly and all accesses are via the stack manager (since Stack is ultimately 
 *  concerned with file format and not behaviour.)
 */
typedef Stack* StackHandle;



/******************
 Types
 */

typedef void (*ACUFatalErrorCB) (int in_error_code);
typedef void (*ACUStackClosedCB) (void *in_ui_context, int in_error_code);

typedef void (*ACUMessageSetCB) (char const *in_message);
typedef char* (*ACUMessageGetCB) (void);

typedef void (*ACUNoOpenStackErrorCB) (void);
typedef void (*ACUScriptErrorCB) (void *in_context, StackHandle in_handler_object, long in_source_line,
char const *in_template, char const *in_arg1, char const *in_arg2, char const *in_arg3, int in_runtime);

typedef void (*ACUCardRepaintCB) (void *in_context);

typedef void (*StackMgrCBFind) (void *in_ui_context, char const *in_terms, StackFindMode in_mode, long in_field_id);
typedef void (*StackMgrCBSystemBeep) (void);
typedef void (*StackMgrCBMutateRTF) (void **io_rtf, long *io_size, char **out_plain, char const *in_new_string, XTETextRange in_edit_range);
typedef void (*StackMgrCBViewRefresh) (void *in_ui_context);
//typedef void (*StackMgrCBScriptStatus) (void *in_ui_context, int in_running, char const *in_message, char const *in_abort, int in_abortable);
typedef void (*StackMgrCBDebug) (void *in_ui_context, StackHandle in_handler_object, long in_source_line);

typedef void (*ACUIsDebuggingCB) (void *in_ui_context, int is_debugging, StackHandle in_handler_object, long in_source_line);

typedef void (*ACUDebugVarsChanged) (void);


typedef void (*ACUTimerAdjustmentCB) (int in_xtalk_active);


typedef void (*ACUSaveScreen) (void *in_ui_context);
typedef void (*ACUReleaseScreen) (void *in_ui_context);

typedef void (*ACURenderEffect) (void *in_ui_context, int in_effect, int in_speed, int in_dest);


typedef void (*ACULayoutWillChange) (void *in_ui_context);
typedef void (*ACULayoutDidChange) (void *in_ui_context);


typedef void (*ACUAnswerChoice) (void *in_ui_context, char const *in_message, char const *in_btn1, char const *in_btn2, char const *in_btn3);
typedef void (*ACUAnswerFile) (void *in_ui_context, char const *in_message, char const *in_type1, char const *in_type2, char const *in_type3);
typedef void (*ACUAnswerFolder) (void *in_ui_context, char const *in_message);
typedef void (*ACUAskText) (void *in_ui_context, char const *in_message, char const *in_response, int in_password_mode);
typedef void (*ACUAskFile) (void *in_ui_context, char const *in_message, char const *in_default_filename);

void acu_answer_choice_reply(StackHandle in_stack, int in_button_index, char const *in_reply);
void acu_answer_file_reply(StackHandle in_stack, char const *in_pathname);
void acu_ask_choice_reply(StackHandle in_stack, char const *in_reply);
void acu_ask_file_reply(StackHandle in_stack, char const *in_pathname);
void acu_answer_folder_reply(StackHandle in_stack, char const *in_pathname);

/* part of the requirements for this is that it uses acu_callback_result_string() to produce a string result */
typedef char* (*ACULocalizedClassName) (char const *in_class_name);


typedef void (*ACUAutoStatusControl) (void *in_ui_context, int in_visible, int in_can_abort);


typedef char const* (*ACUBuiltinResourcesPathCB) (void);


/*
 *  ACUCallbacks
 *  ---------------------------------------------------------------------------------------------
 *  Callbacks from the ACU to the platform specific user-interface.  See the individual callback
 *  types for documentation.  These are provided once at application startup via
 *  acu_init().
 */
typedef struct
{
    ACUFatalErrorCB fatal_error;
    ACUNoOpenStackErrorCB no_open_stack;
    ACUScriptErrorCB script_error;
    
    ACUStackClosedCB stack_closed; /* fatal error, specific to stack, eg. stack cannot be written, etc.
                                    may eventually be used to indicate that the stack UI should be closed
                                    even under normal conditions */
    
    ACUMessageSetCB message_set;
    ACUMessageGetCB message_get;
    
    ACUCardRepaintCB view_refresh;

    StackMgrCBSystemBeep do_beep;
    StackMgrCBMutateRTF mutate_rtf;
    
    StackMgrCBFind do_find;
    
    //StackMgrCBScriptStatus script_status;
    StackMgrCBDebug debug;
    XTEDebugMessageCB debug_message;
    ACUIsDebuggingCB is_debugging;
    ACUDebugVarsChanged debug_vars_changed;
    ACUTimerAdjustmentCB adjust_timers;
    
    ACUSaveScreen screen_save;
    ACUReleaseScreen screen_release;
    
    ACURenderEffect effect_render;
    
    ACULayoutWillChange view_layout_will_change;
    ACULayoutDidChange view_layout_did_change;
    
    ACUAnswerChoice answer_choice;
    ACUAnswerFile answer_file;
    ACUAnswerFolder answer_folder;
    ACUAskText ask_text;
    ACUAskFile ask_file;
    
    ACULocalizedClassName localized_class_name;
    
    ACUAutoStatusControl auto_status_control;
    
    ACUBuiltinResourcesPathCB builtin_resources_path;
    
} ACUCallbacks;


/*
 *  StackMgrStackCreateDef
 *  ---------------------------------------------------------------------------------------------
 *  Information required to create a new stack.  See stackmgr_stack_create().
 */
typedef struct
{
    int card_width;
    int card_height;
    
} StackMgrStackCreateDef;



/******************
 Setup
 */

/*
 *  acu_init
 *  ---------------------------------------------------------------------------------------------
 *  
 */
int acu_init(ACUCallbacks *in_callbacks);


void acu_quit(void);


/******************
 Timers
 */

/*
 *  acu_idle
 *  ---------------------------------------------------------------------------------------------
 *  
 */
void acu_timer(void);

void acu_xtalk_timer();


void acu_finger_start(void);
void acu_finger_end(void);




void acu_source_indent(char const *in_source, long *io_selection_start, long *io_selection_length,
                       XTEIndentingHandler in_indenting_handler, void *in_context);



/******************
 Stack Management
 */

int stackmgr_stack_create(char const *in_path, StackMgrStackCreateDef *in_def);

/* returns a manually released handle; ie. caller owns it */
StackHandle stackmgr_stack_open(char const *in_path, void *in_ui_context, int *out_error);

void stackmgr_stack_close(StackHandle in_handle);

/* for backwards compatibility with existing code base; app should gradually be refactored to
 no longer use this directly, since there's no management of it ?? */
Stack* stackmgr_stack_ptr(StackHandle in_handle);

/* return the Stack's xtalk engine instance - also see comments above.
 actually mightn't be bad at all to provide these accessors; just that they need to have a
 documented policy around their direct usage outside the stack manager */
XTE* stackmgr_stack_xtalk(StackHandle in_handle);

int stackmgr_stack_open_count(void);

/* returns an auto-released handle; ie. caller must call retain() to keep it alive */
StackHandle stackmgr_stack_named(char const *in_name);


void stackmgr_stack_set_active(StackHandle in_handle);
void stackmgr_stack_set_inactive(StackHandle in_handle);
int stackmgr_stack_is_active(StackHandle in_handle);



/******************
 View
 */

void stackmgr_set_current_card_id(StackHandle in_stack, long in_card_id);

long stackmgr_current_card_id(StackHandle in_handle);
long stackmgr_current_bkgnd_id(StackHandle in_handle);



void acu_script_editor_opened(StackHandle in_object);
void acu_script_editor_closed(StackHandle in_object);

void acu_icon_data(StackHandle in_stack, int in_id, void **out_data, long *out_size);


/******************
 Icon Management UI
 */

int acu_iconmgr_count(StackHandle in_stack);
void acu_iconmgr_icon_n(StackHandle in_stack, int in_index, int *out_id, char **out_name, void **out_data, long *out_size);
void acu_iconmgr_preview_range(StackHandle in_stack, int in_from_number, int in_to_number);

int acu_iconmgr_create(StackHandle in_stack);
int acu_iconmgr_mutate(StackHandle in_stack, int in_id, int const *in_new_id, char const *in_new_name, void const *in_data, long in_size);
int acu_iconmgr_delete(StackHandle in_stack, int in_id);


/******************
 Handles
 */

StackHandle acu_retain(StackHandle in_handle);
StackHandle acu_release(StackHandle in_handle);



/******************
 Obtaining Handles
 */

/*
 *  stackmgr_handle_for_card_id
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a StackHandle for a card, within the owner stack.
 *
 *  Flags must specify the desired memory management: STACKMGR_FLAG_AUTO_RELEASE or
 *  STACKMGR_FLAG_MAN_RELEASE.
 */
StackHandle acu_handle_for_card_id(int in_flags, StackHandle in_stack, long in_card_id);


StackHandle acu_handle_for_current_card(int in_flags, StackHandle in_stack);
StackHandle acu_handle_for_current_bkgnd(int in_flags, StackHandle in_stack);


/*
 *  stackmgr_handle_for_bkgnd_id
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a StackHandle for a background, within the owner stack.
 *
 *  Flags must specify the desired memory management: STACKMGR_FLAG_AUTO_RELEASE or
 *  STACKMGR_FLAG_MAN_RELEASE.
 */
StackHandle stackmgr_handle_for_bkgnd_id(int in_flags, StackHandle in_stack, long in_bkgnd_id);


/*
 *  stackmgr_handle_for_field_id
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a StackHandle for a field, within the owner layer (card or background.)
 *  Optional <in_card> specifies the card when referring to a background field with unshared 
 *  text content (this is not required.)
 *
 *  Flags must specify the desired memory management: STACKMGR_FLAG_AUTO_RELEASE or
 *  STACKMGR_FLAG_MAN_RELEASE.
 */
StackHandle acu_handle_for_field_id(int in_flags, StackHandle in_owner, long in_field_id, StackHandle in_card);


/*
 *  acu_handle_for_button_id
 *  ---------------------------------------------------------------------------------------------
 *  Obtain a StackHandle for a button, within the owner layer (card or background.)
 *
 *  Flags must specify the desired memory management: STACKMGR_FLAG_AUTO_RELEASE or
 *  STACKMGR_FLAG_MAN_RELEASE.
 */
StackHandle acu_handle_for_button_id(int in_flags, StackHandle in_owner, long in_button_id);


/*
 *  stackmgr_handles_equivalent
 *  ---------------------------------------------------------------------------------------------
 *  Returns STACKMGR_TRUE if the two handles refer to the same resource.
 *  Returns STACK_FALSE otherwise.
 */
int stackmgr_handles_equivalent(StackHandle in_handle_1, StackHandle in_handle_2);





void acu_effect_rendered(StackHandle in_stack);




/******************
 Managing Handles
 */

/*
 *  stackmgr_handle_retain
 *  ---------------------------------------------------------------------------------------------
 *  Increments the reference count on the specified handle.  Should be called if a handle 
 *  is stored between iterations of the event loop so that the handle remains valid and isn't
 *  disposed of by the Stack Manager.
 */
StackHandle stackmgr_handle_retain(StackHandle in_handle);


/*
 *  stackmgr_handle_release
 *  ---------------------------------------------------------------------------------------------
 *  Decrements the reference count on the specified handle.  Should be called when the calling
 *  code is finished with a handle, except when that handle was obtained with the 
 *  STACKMGR_FLAG_AUTO_RELEASE flag and hasn't since been retained.
 */
StackHandle stackmgr_handle_release(StackHandle in_handle);


/*
 *  stackmgr_autorelease
 *  ---------------------------------------------------------------------------------------------
 *  Adds the handle to the Stack Manager's auto-release pool for release at the next iteration
 *  of the main event loop, or the next call to stackmgr_arp_drain().
 */
StackHandle stackmgr_autorelease(StackHandle in_handle);


/*
 *  stackmgr_handle_description
 *  ---------------------------------------------------------------------------------------------
 *  Returns a string description of the specified handle.
 *
 *  In flags can include any of the following length flags:
 *  -   STACKMGR_FLAG_LONG      Provide a long description, eg. card button id 1 of card "Demo"
 */
char const* stackmgr_handle_description(StackHandle in_handle, int in_flags);



StackHandle acu_release(StackHandle in_handle);
StackHandle acu_retain(StackHandle in_handle);
StackHandle acu_autorelease(StackHandle in_handle);


/******************
 Stack Accessor/Mutators
 */

int stackmgr_script_get(StackHandle in_handle, char const **out_script, int const *out_checkpoints[], int *out_checkpoint_count,
                        long *out_sel_offset);
int stackmgr_script_set(StackHandle in_handle, char const *out_script, int const in_checkpoints[], int in_checkpoint_count,
                        long in_sel_offset);

void stackmgr_script_rect_get(StackHandle in_handle, int *out_x, int *out_y, int *out_w, int *out_h);
void stackmgr_script_rect_set(StackHandle in_handle, int in_x, int in_y, int in_w, int in_h);




/******************
 Scripting Entry
 */

/*
 *  acu_post_system_event
 *  ---------------------------------------------------------------------------------------------
 *  Posts a CinsTalk system event message, optionally with a single parameter.  If the message
 *  is not handled, no error results.
 *
 *  Intended to be invoked by the user interface, for buttons, cards and the like.
 *
 *  This mechanism continues to work even while a script is in progress:
 *  eg. openCard, etc. A glue action pre-handling and post-handling must be able to generate 
 *  'knock-on' effects, ie. additional system messages.
 *
 *  The "idle" system message is generated automatically by the ACU.
 *
 *  ! Caller retains responsibility for target and event name.
 *  ! Caller RELINQUISHES ownership of the parameter and MUST NOT make any further reference
 *    to it, including calls to retain/release due to multithreading (variants are not
 *    thread safe!)
 *
 *  IMPORTANT:  The caller must not place the variant parameter(s) if any, on any kind of 
 *  autorelease mechanism or do anything with them after - total relinquish ownership!
 */
void acu_post_system_event(StackHandle in_target, char const *in_name, XTEVariant *in_param1);

/*
 *  acu_message
 *  ---------------------------------------------------------------------------------------------
 *  Accepts a CinsTalk command or expression and evaluates.  If the input is an expression, the
 *  result is delivered via the message_result callback.
 *
 *  If the xTalk engine is currently busy with another message/script, this function does nothing.
 *
 *  Intended to be invoked by the message box in response to the user hitting the Return key.
 *
 *  Caller retains all responsibility for arguments.
 */
void acu_message(StackHandle in_target, char const *in_message);


/******************
 Scripting Control
 */

/* used together with various script entry points and stack_set_current() to prevent access to UI
 during script execution within a stack, regardless of whether it's active or not! */

int stackmgr_script_running(StackHandle in_stack);





// need a callback to allow entry/exit of lockdown for a given stack - display sheet and disable tools after short delay
// lockdown sheet shouldn't be necessary immediately - thus we can trigger this callback twice - once when it happens
// and again 1/10th of a second later?

// we're receiving an idle message from the application anyway.  although we possibly should be explcit about entries
// as each entry may require slightly different timing, eg. i've not decided on the timing for idle system messages yet,
//  although I suspect 1/10 of a second would be ideal

/*
 aquire and relininquish, will serve to allow the UI as it's currently implemented in v1.0
 to safely obtain ownership of the stack to make changes, stopping the lodgement of further xTalk
 events during that time.
 */

int acu_ui_acquire_stack(StackHandle in_stack);
void acu_ui_relinquish_stack(StackHandle in_stack);

void stackmgr_set_tool(StackHandle in_handle, int in_tool);



StackHandle stackmgr_script_error_target(StackHandle in_stack);
long stackmgr_script_error_source_line(StackHandle in_stack);


void acu_script_abort(StackHandle in_stack);
void acu_script_debug(StackHandle in_stack);

int acu_script_is_resumable(StackHandle in_stack);
int acu_script_is_active(StackHandle in_stack);

void acu_script_continue(StackHandle in_stack);
void acu_debug_step_over(StackHandle in_stack);
void acu_debug_step_into(StackHandle in_stack);
void acu_debug_step_out(StackHandle in_stack);


char const* const* stackmgr_debug_var_contexts(void);

/*
 *  stackmgr_debug_vars
 *  ---------------------------------------------------------------------------------------------
 *  Returns the total number of variables in the current context (changed or not), and
 *  a NULL-terminated array of variables.  The returned array is limited to only those that have
 *  been mutated since the last call if <in_only_changed> is SM_TRUE.
 */
//void stackmgr_debug_vars(int in_only_changed, int *out_total, SMDebugVariable **out_variables);


void acu_debug_begin_inspection(void);
void acu_debug_end_inspection(void);

char const* acu_debug_context(void);

int acu_debug_variable_count(void);
char const* acu_debug_variable_name(int in_index);
char const* acu_debug_variable_value(int in_index);

void acu_debug_set_variable(int in_index, char const *in_name, char const *in_value);



int stackmgr_debug_var_count(int in_context);
char const* stackmgr_debug_var_name(int in_context, int in_index);
char const* stackmgr_debug_var_value(int in_context, int in_index);

/* we take some extra parameters to validate and ensure that we set the correct variable */
void stackmgr_debug_var_set(int in_context, int in_index, char const *in_name, char const *in_value);


/* convenience facilities to copy strings and data that must be available
 after the callback has returned, but only until the next callback;
 must be used with internally protected non rentrant callbacks*/
char* acu_callback_result_string(char const *in_string);
void* acu_callback_result_data(void const *in_data, int in_size);


#endif
