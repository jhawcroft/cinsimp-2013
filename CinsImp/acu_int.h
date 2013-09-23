/*
 
 Application Control Unit
 acu_int.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Internal ACU header
 
 *************************************************************************************************
 
 Dual Threads
 -------------------------------------------------------------------------------------------------
 The ACU operates on two threads:
 -  The main thread ('UT'), on which the application UI is running and the application starts
 -  A secondary thread ('XT'), on which the xTalk engine and all script execution occurs
 
 This model allows the UI to remain responsive to basic manipulation, the application to report
 as 'live', the implementation of a user-script-abort mechanism and for the scripts of separate
 stacks to run concurrently.
 
 *************************************************************************************************
 
 Inter-Thread Communications
 -------------------------------------------------------------------------------------------------
 There are several mechanisms used for communication between UT and XT, depending on the type of
 information being exchanged.
 
 All communications mechanisms currently share a single mutex - xt_mutex_comms.  This must be
 locked during communications, using _acu_xt_comms_lock() and _acu_xt_comms_unlock().
 
 Communications mechanisms should signal - xt_wakeup - anytime, as XT may be sleeping awaiting 
 a response, additional work or a signal condition.
 
 It shouldn't matter to UT, but if XT is asleep, xt_is_sleeping is set to ACU_TRUE (this is for
 debugging purposes only and wakeups should always be triggered regardless via _acu_xt_wakeup).
 
 
 Using the following abbreviations in documentation relating to the threading of the ACU:
 
 -  UT      User-interface Thread - the main thread of the application
 
 -  XT      Xtalk Thread - the secondary thread on which xTalk scripts are executed
 
 -  TERM    A 'signal' sent by UT to XT to terminate the secondary thread, as soon as possible
            delivered when a stack is closed
            (see xt_sig_demand_terminate variable)
 
 -  EVENT   One or more events are waiting in the queue for processing by the xTalk engine
            (see xtalk_sys_event_queue variable)
 
 -  DCR     Debug Control Request - UT is requesting the XT script to:
 
     -   _ACU_T_SIG_NONE     Default (No Current Request)
     -   _ACU_T_SIG_ABORT    Abort current system event execution and empty system event queue
     -   _ACU_T_SIG_RUN      Continue script where it is currently paused
     -   _ACU_T_SIG_STEP     Execute the next script line and pause
     -   _ACU_T_SIG_INTO     Execute until we enter a handler
     -   _ACU_T_SIG_OUT      Execute until we have left a handler
 
            (see xt_sig_dbg_ctrl variable)
 
 -  DIR     Debug Information Request - UT is requesting information about the variables or a
            single variable? in the xTalk runtime
            (see xt_sig_dbg_ireq variable and associates)
 
 -  DMR     Debug Mutate Request - UT is requesting a specific variable be mutated to a new value
            (see xt_sig_dbg_imut variable and associates)
 
 -  SIC     Syncronous Implementation Channel - the XT uses this to syncronously request services
            of the UT, almost always for part of a command implementation, but occasionally for
            other purposes
            
            While the XT is blocked and sleeping, it continues to service DIRs and DMRs and
            recognise TERM
 
 -  CHKPT   A signal from the XT to the UT that a checkpoint has been encountered and the xTalk
            engine is now paused
            (see ut_sig_script_chkpt variable)
 
 -  ERROR   A signal from the XT to the UT that a script error has been encountered and the xTalk
            engine has either aborted completely or paused to allow variable inspection
            (see ut_sig_script_error variable and associates)
 
 
 Signal variables generally use the following flags:
 
 -   _ACU_T_SIG_NONE        No signal present currently
 -   _ACU_T_SIG_REQ         Signal the thread
 -   _ACU_T_SIG_ACK         Acknowledge the signal
 
 
 !  DIRs and DMRs are currently broken while I figure out an appropriate implementation
 

 *************************************************************************************************
 */

#ifndef CINSIMP_ACU_INT_H
#define CINSIMP_ACU_INT_H

/******************
 Standard Library
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/******************
 ACU Headers
 */
#include "acu.h"
#include "acu_limits.h"
#include "acu_thread.h"
#include "stack.h"
#include "stack_int.h" /* temporary until we stop using Stack* all over the place */
#include "xtalk_engine.h"


/******************
 Constants
 */

/* inter-thread communications signals: */
#define _ACU_T_SIG_NONE     0
#define _ACU_T_SIG_REQ      1
#define _ACU_T_SIG_ACK      2
#define _ACU_T_SIG_NOTICE   3

/* debug specific signals: */
#define _ACU_T_SIG_ABORT    4
#define _ACU_T_SIG_RUN      6
#define _ACU_T_SIG_STEP     7
#define _ACU_T_SIG_INTO     8
#define _ACU_T_SIG_OUT      9

/* reason for halting execution */
#define _ACU_HALT_NONE      0
#define _ACU_HALT_ERROR     1
#define _ACU_HALT_CHKPT     2


/* internal code used to sanity check that a handle is actually a handle */
#define _STACKMGR_HANDLE_STRUCT_ID "STKH"

#define _SCRIPT_ENTRY_INVALID 0
#define _SCRIPT_ENTRY_SYSTEM_MESSAGE 1
#define _SCRIPT_ENTRY_MESSAGE 2

#define _INVALID_SESSION_ID 0


/*
#define _THREAD_XTE_CTRL_CONTINUE 0 
#define _THREAD_XTE_CTRL_SHUTDOWN 1
#define _THREAD_XTE_CTRL_TERMINATED 2 

#define _THREAD_XTE_CTRL_ABORT 3 
#define _THREAD_XTE_CTRL_PAUSE 4 
#define _THREAD_XTE_CTRL_STEP_OVER 5 
#define _THREAD_XTE_CTRL_STEP_INTO 6
#define _THREAD_XTE_CTRL_STEP_OUT 7


#define _THREAD_XTE_HALT_NONE 0
#define _THREAD_XTE_HALT_CHECKPOINT 1
#define _THREAD_XTE_HALT_ERROR 2
#define _THREAD_XTE_HALT_COMPLETE 3


#define _THREAD_XTE_DEBUG_NONE 0
#define _THREAD_XTE_DEBUG_SETV 1
#define _THREAD_XTE_DEBUG_ALLV 2


#define _ACU_OWNS_PIPE 0
#define _XTE_OWNS_PIPE 1

*/



/******************
 Macros
 */

#define IS_STACKHANDLE(x) ((x != NULL) && (strcmp(x->struct_id, _STACKMGR_HANDLE_STRUCT_ID) == 0))


/******************
 Types
 */

typedef struct _ACUAutoreleasePool ACUAutoreleasePool;



struct StackScriptingError
{
    long source_line;
    char *template_msg;
    char *arg1;
    char *arg2;
    char *arg3;
    int runtime;
};


#define _ACU_EVENT_SYSTEM 0  /* normal system event */
#define _ACU_EVENT_MESSAGE 1 /* message box */


typedef struct _ACUEvent
{
    int         type;
    char        *message;
    XTEVariant  *target;
    XTEVariant  *params[ACU_LIMIT_MAX_SYS_EVENT_PARAMS];
    int         param_count;
    
} ACUEvent;

typedef struct _ACUEventQueue
{
    int         size;
    int         count;
    ACUEvent    *events;
    int         front;
    int         back;
    ACUEvent    last_event;
    
} ACUEventQueue;



typedef struct _ACUQueuedEffect
{
    int effect;
    int speed;
    int dest;
    
} ACUQueuedEffect;



typedef struct _ACUxTalkOpenFile
{
    char *pathname; /* NULL if the file handle isn't open */
    FILE *fp;
    
} ACUxTalkOpenFile;




typedef struct _StackMgrStack StackMgrStack;


typedef struct
{
    union
    {
        void *pointer;
        struct HandleDef *hdef;
        long integer;
        char const *string;
        double real;
        int boolean;
    } value;
    
} ACUParam;


typedef void (*ACUImplementor) (StackMgrStack *in_stack, ACUParam in_params[], int in_param_count);


typedef struct
{
    ACUImplementor implementor;
    ACUParam param[ACU_MAX_IC_PARAMS];
    int param_count;
    
} ACU_IC;



typedef struct
{
    int the_id;
    char *the_name;
    void *the_data;
    long the_size;
    Stack *the_stack;
    
} ACUCachedIcon;



struct _StackMgrStack
{
    /* session ID allows us to a) determine if this slot is available for registration or is in use
     by a current stack, and b) issue pointers to slots and still verify a handle's stack is open
     after stacks have been successively closed/opened and slots reused */
    int session_id;
    
    /* cache of some useful identifying information */
    char *name_short;
    char *name_long;
    
    /* stack file wrapper */
    Stack *stack;
    
    /* each stack has it's own xtalk thread and state */
    XTE *xtalk;
    
    /* thread management: */
    ACUThread thread_xte;
    ACUThreadMutex xt_mutex_comms;
    
    /* on OS X, the NSDocument subclass handling this stack (if any) */
    void *ui_context;
    
    /* current view state */
    long __current_card_id; /* must be accessed using _stackmgr_current_card_id();
                             an alternative current card (eg. during sorting) may
                             be returned */
    long __current_bkgnd_id;
    
    /* screen refresh locking; zero = unlocked, > 0 locked a set # of times;
     automatically unlocked at idle */
    int lock_screen;
    int idle_repaint_required;
    
    /* visual effect queue */
    ACUQueuedEffect effect_queue[ACU_LIMIT_MAX_VISUAL_EFFECTS];
    int effect_queue_count;
    int effect_queue_playback;
    int respond_to_xtalk_after_effects;
    
    /* files opened by xTalk */
    ACUxTalkOpenFile open_files[ACU_LIMIT_MAX_OPEN_FILES];

    /* auto-release pool for cleanup of handles, variants, etc. */
    ACUAutoreleasePool *arpool_idle; /* drained when the idle system message is generated;
                                      ie. when the xtalk engine is idle */
    
    /* system event queue for the xtalk engine;
     memory maintained by _evtq.c */
    ACUEventQueue *xtalk_sys_event_queue;
    
    /* does the UI have the stack? should event processing be halted?
     prevents the XT from picking up another event from the event queue,
     it is the responsibility of UT to ensure nothing is executing on XT
     by checking the xt_is_executing flag prior to setting this */
    int xt_disable_processing;
    
    /* should the system event queue be disabled? eg. during layout/painting */
    int ut_disable_queue;
    
    /* wakeup sleeping XT */
    ACUThreadCond xt_wakeup;
    
    /* unambiguous xtalk shutdown - allows complete termination of XT
     at stack closure, regardless of other control signals */
    int xt_sig_demand_terminate;
    
    /* if XT is sleeping */
    int xt_is_sleeping; /* for debugging purposes only; may be removed at a later date */
    
    /* if xTalk engine has a script processing
     or any backlog of system events to process,
     may still be true if the thread is sleeping */
    int xt_is_executing;
    
    /* syncronous implementation channel (SIC);
     memory maintained by XT */
    int xt_sig_sic;
    ACU_IC xt_sic;
    
    /* debug control signal */
    int xt_sig_dbg_ctrl;
    
    /* debug information request signal */
    int xt_sig_dbg_ireq;
    int xt_ireq_handler_index;
    
    /* response to debug information request;
     asyncronous channel;
     memory allocated and freed by ACU */
    int ut_sig_dir_ic;
    ACU_IC ut_dir_ic;
    
    /* debug information mutate signal;
     memory maintained by caller to ACU */
    int xt_sig_dbg_imut;
    char *xt_imut_var_name;
    int xt_imut_handler_index;
    char *xt_imut_var_value;
    
    /* last script error notice;
     memory maintained by XTE provided it isn't 
     reentered, thereby resetting errors */
    int ut_sig_script_error;
    int thread_xte_source_line;
    char *thread_xte_error_template;
    char *thread_xte_error_arg1;
    char *thread_xte_error_arg2;
    char *thread_xte_error_arg3;
    int thread_xte_error_is_runtime;
    StackHandle ut_script_error_object;
    //XTEVariant *thread_xte_error_object;
    
    /* has UI posted an error message and still awaiting a response?
     (until a response is received, no idle messages should be lodged
     and no other UI actions should be available) */
    int ut_has_error_halt_messages;
    
    /* count open script editors;
     if there's more than 1, we don't allow sending messages
     (well at least not idle) */
    int ut_script_editor_count;
    int ut_debug_suspended_closure;
    
    /* checkpoint signal */
    int ut_sig_script_chkpt;
    
    /* halt code - reason for halting;
     misleading name - is set and controlled on the UT side;
     isnt used for comms between threads */
    int xt_halt_code;
    
    /* is debugging currently occurring?  ie. have we entered debug mode? */
    int thread_is_debuggable;
    
    /* auto-status display timer */
    int         auto_status_timer;
    
    
    /*** TO REVISE/ REMOVE: ****/
    // still used by a handful of things in _cmds
    // and possibly elsewhere
    void *thread_xte_imp;
    int thread_xte_imp_param_count;
    void *thread_xte_imp_param[ACU_MAX_THREAD_IMP_ARGS];

    /* pool for callback results */
    //ACUAutoreleasePool *ar_callback_results;
    
       
    
    /* specific callback result temporaries;
     to be cleaned up */
    char *cb_rslt_localized_string;
    
    /* icon manager cache for the UI */
    ACUCachedIcon *iconmgr_cache;
    int iconmgr_cache_size;
};


/* internal reference representation */
struct HandleRef
{
    int type;
    
    StackMgrStack *register_entry;
    int session_id;
    
    //Stack* stack;
    long layer_id;
    int layer_is_card;
    long widget_id;
};


/* internal handle representation */
typedef struct HandleDef
{
    char struct_id[sizeof(_STACK_STRUCT_ID)];
    
    int ref_count;
    
    struct HandleRef reference;
    
    ACUThreadMutex mutex;
    
} HandleDef;


typedef struct _ACUDebugVariable
{
    char *name;
    char *value;
    
} ACUDebugVariable;


/* stack manager internal state structure */
struct ACU
{
    /* basic state */
    int             inited;
    int             last_error;
    
    /* configured callbacks */
    ACUCallbacks callbacks;
    
    /* temporary results */
    char            result_handle_desc[_MAX_DESC_LENGTH + 1];
    
    /* handle auto-release pool;
     allows us to automatically release unused handles */
    ACUAutoreleasePool *ar_pool;
    ACUThreadMutex ar_pool_mutex;
    
    /* callback result release pool */
    ACUAutoreleasePool *ar_callback_results;
    ACUThreadMutex ar_callback_result_mutex;
    
    /* open stack registry */
    StackMgrStack   *open_stacks;
    int             open_stacks_alloc;
    int             open_stacks_count;
    
    /* current stack (if any) */
    StackMgrStack   *current_stack;
    
    /* register of debugging globals for the
     current stack */
    ACUThreadMutex debug_information_m;
    char *debug_handler_name;
    ACUDebugVariable *debug_variables;
    int debug_variable_count;
    int ut_sig_debug_info_changed;
    
    ACUDebugVariable *debug_vars_copy;
    int debug_vars_copy_count;
    
    /* below debug stuff is DEPRECATED- we're removing from 1.0
     and replacing with simplified above ^ */
    ACUDebugVariable *debug_globals;
    int debug_global_count;
    ACUDebugVariable *debug_locals;
    int debug_local_count;
    char **debug_handlers;
    int debug_handler_count;
    int debug_handler_index;
    
    /* mutex for the ACU allocator */
    ACUThreadMutex mem_mutex;
    
    /* # active script threads */
    int     active_script_count;
    int     xtalk_is_active;
    
    /* idle generating timer */
    int         idle_gen_timer;
    
    int ut_finger_tracking;
    
    ACUThreadMutex non_reentrant_callback_m;
    
    /* built-in resources */
    Stack *builtin_resources;

};





/******************
 Globals
 */

/* the stack manager internal state */
extern struct ACU _g_acu;



void _acu_check_exit_debugger(StackMgrStack *in_stack);



void _stackmgr_handle_stack_error(Stack *in_stack, StackMgrStack *in_register_entry, int in_error);


/******************
 Errors
 */

/* error handling */
int _stackmgr_has_fatal(void);
void _stackmgr_raise_error(int in_error_code);

void _acu_raise_error(int in_error_code);


/******************
 Handles
 */

/* check if a handle is actually a Stack* */
int _stackmgr_handle_is_stack(HandleDef *in_handle);

/* obtain the registry entry for a given stack handle;
 currently works with handles to stacks and Stack pointers */
StackMgrStack* _stackmgr_stack(StackHandle in_handle);

/* reverse of above, creates a manually released handle for the given registry entry */
StackHandle _stackmgr_handle(StackMgrStack *in_registry_entry);

/* create an untyped handle, ready to be populated
 with reference details */
HandleDef* _stackmgr_handle_create(int in_flags);


#define HDEF(x) (((HandleDef*)x)->reference)



void _acu_xt_card_needs_repaint(StackMgrStack *in_stack, int in_now);



/******************
 Stack Manager
 */
void _stackmgr_clear_scripting_error(StackMgrStack *in_stack);


void _acu_update_xtalk_timer_interval(void);

/******************
 Current Card
 */

long _stackmgr_current_card_id(StackMgrStack *in_registry_entry);
long _stackmgr_current_bkgnd_id(StackMgrStack *in_registry_entry);


/******************
 Threading
 */

void _stackmgr_thread_scripting_start(StackMgrStack *in_register_entry);
void _stackmgr_scripting_lock(StackMgrStack *in_register_entry);
void _stackmgr_scripting_unlock(StackMgrStack *in_register_entry);



void _acu_xt_comms_lock(StackMgrStack *in_stack);
void _acu_xt_comms_unlock(StackMgrStack *in_stack);
void _acu_xt_wakeup(StackMgrStack *in_stack);

void _acu_begin_single_thread_callback(void);
void _acu_end_single_thread_callback(void);


/******************
 Utility
 */

char* _stackmgr_clone_cstr(char const *in_string);


/******************
 Threading
 */

int _acu_thread_create(ACUThread *out_thread, void *in_routine, void *in_context, long in_stack_size);
int _acu_thread_mutex_create(ACUThreadMutex *out_mutex);
int _acu_thread_cond_create(ACUThreadCond *out_cond);

void _acu_thread_mutex_dispose(ACUThreadMutex *in_mutex);
void _acu_thread_cond_dispose(ACUThreadCond *in_cond);

void _acu_thread_mutex_lock(ACUThreadMutex *in_mutex);
void _acu_thread_mutex_unlock(ACUThreadMutex *in_mutex);

void _acu_thread_cond_signal(ACUThreadCond *in_cond);
void _acu_thread_cond_wait(ACUThreadCond *in_cond, ACUThreadMutex *in_mutex);

void _acu_thread_usleep(int in_usecs);

void _acu_thread_exit(void);


/******************
 Memory Management
 */

#define _ACU_MALLOC_ZONE_GENERAL 0  /* general memory; no specific requirements */
#define _ACU_MALLOC_ZONE_STANDARD -1 /* use standard library directly, without debugging capabilities */

void* _acu_malloc(long in_bytes, int in_zone, char const *in_file, long in_line);
void* _acu_calloc(long in_number, long in_bytes, int in_zone, char const *in_file, long in_line);
void* _acu_realloc(void *in_memory, long in_bytes, int in_zone, char const *in_file, long in_line);
void _acu_free(void *in_memory);

#ifndef _ACU_ALLOCATOR_INT
#define __func__ __FUNCTION__
#define malloc(bytes) _acu_malloc(bytes, _ACU_MALLOC_ZONE_GENERAL, __func__, __LINE__)
#define calloc(num, bytes) _acu_calloc(num, bytes, _ACU_MALLOC_ZONE_GENERAL, __func__, __LINE__)
#define realloc(mem, bytes) _acu_realloc(mem, bytes, _ACU_MALLOC_ZONE_GENERAL, __func__, __LINE__)
#define free(mem) _acu_free(mem)
#endif

#if DEBUG
long _acu_allocated(void);
void _acu_mem_print(void);
void _acu_mem_baseline(void);
#endif






ACUAutoreleasePool* _acu_autorelease_pool_create(void);
void _acu_autorelease_pool_dispose(ACUAutoreleasePool *in_pool);
void _acu_autorelease_pool_drain(ACUAutoreleasePool *in_pool);

typedef void (*ACUAutoreleasePoolReleasor)(void *in_object);
void* _acu_autorelease(ACUAutoreleasePool *in_pool, void *in_object, ACUAutoreleasePoolReleasor in_releasor);

char* _acu_clone_cstr(char const *in_string);

void _acu_xt_sic(StackMgrStack *in_stack, ACUImplementor in_implementor);


void _acu_xtalk_callback_respond(StackMgrStack *in_stack);

void _acu_xtalk_thread_handle_debug_requests(StackMgrStack *in_stack);




void _acu_enter_debugger(StackMgrStack *in_stack);
void _acu_exit_debugger(StackMgrStack *in_stack);


void _acu_variant_handle_destructor(XTEVariant *in_variant, char *in_type, StackHandle in_handle);


void _acu_debug_get_all_variables(int in_context);



void _acu_unlock_screen(StackMgrStack *in_stack, int in_respond_xtalk_after);
void _acu_lock_screen(StackMgrStack *in_stack);

void _acu_repaint_card(StackMgrStack *in_stack);
void _acu_go_card(StackMgrStack *in_stack, long in_card_id);



int _load_builtin_resources(void);


/******************
 Event Queues
 */

ACUEventQueue* _acu_event_queue_create(int in_queue_size);
void _acu_event_queue_dispose(ACUEventQueue *in_queue);
int _acu_event_queue_post(ACUEventQueue *in_queue, ACUEvent *in_event);
int _acu_event_queue_count(ACUEventQueue *in_queue);
ACUEvent* _acu_event_queue_next(ACUEventQueue *in_queue);


char* __acu_clone_cstr(char const *in_string, char const *in_file, long in_line);
#define _acu_clone_cstr(in_string) __acu_clone_cstr(in_string, __FUNCTION__, __LINE__)
#define _stackmgr_clone_cstr(in_string) __acu_clone_cstr(in_string, __FUNCTION__, __LINE__)


/******************
 xTalk File I/O
 */

void _acu_xt_fio_open(StackMgrStack *in_stack, char const *in_filepath);
void _acu_xt_fio_close(StackMgrStack *in_stack, char const *in_filepath);
void _acu_xt_fio_read(StackMgrStack *in_stack, char const *in_filepath, long *in_char_begin, char *in_char_end, long *in_char_count);
void _acu_xt_fio_read_line(StackMgrStack *in_stack, char const *in_filepath);
void _acu_xt_fio_write(StackMgrStack *in_stack, char const *in_filepath, char const *in_text, long *in_char_begin);




void _acu_dispose_callback_results(void);


void _acu_iconmgr_recache(StackMgrStack *in_stack);
void _acu_iconmgr_dispose(StackMgrStack *in_stack);


#endif
