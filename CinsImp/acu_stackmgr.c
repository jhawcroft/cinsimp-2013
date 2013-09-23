/*
 
 Application Control Unit - Stack Manager
 acu_stack.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Manages all the open/loaded stacks
 
 *************************************************************************************************
 */

#include "acu_int.h"
#include "tools.h"


#define _ACU_AUTO_STATUS_THRESHOLD 2


static int _g_acu_session_gen = 0;


/******************
 Callback Handlers
 */

/*
 *  _stackmgr_handle_stack_error
 *  ---------------------------------------------------------------------------------------------
 *  Handles any error reported by the Stack unit by invoking the fatal_error handler for the ACU.
 *  Effectively recommends stack closure.
 */
void _stackmgr_handle_stack_error(Stack *in_stack, StackMgrStack *in_register_entry, int in_error)
{
    _g_acu.callbacks.stack_closed(in_register_entry->ui_context, in_error);
}


/******************
 Internal Stack Registry
 */

/*
 *  _stackmgr_stack_register
 *  ---------------------------------------------------------------------------------------------
 *  Registers a stack with the ACU Stack Manager in preparation for opening/loading the stack.
 */
static StackMgrStack* _stackmgr_stack_register(char const *in_path)
{
    /* check there are not too many stacks already registered */
    if (_g_acu.open_stacks_count >= _LIMIT_OPEN_STACKS)
    {
        _stackmgr_raise_error(STACKMGR_ERROR_TOO_MANY_STACKS);
        return NULL;
    }
    
    /* see if we've enough space in the registry;
     if we don't, create more space */
    if (_g_acu.open_stacks_alloc == _g_acu.open_stacks_count)
    {
        StackMgrStack *new_open_stacks = realloc(_g_acu.open_stacks,
                                                 sizeof(StackMgrStack) * (_g_acu.open_stacks_alloc + _OPEN_STACKS_ALLOC));
        if (!new_open_stacks)
        {
            _stackmgr_raise_error(STACKMGR_ERROR_MEMORY);
            return NULL;
        }
        _g_acu.open_stacks = new_open_stacks;
        memset(_g_acu.open_stacks + _g_acu.open_stacks_alloc, 0, sizeof(StackMgrStack) * _OPEN_STACKS_ALLOC);
        _g_acu.open_stacks_alloc += _OPEN_STACKS_ALLOC;
    }
    
    /* find the next available space in the registry;
     configure it for this stack */
    for (int i = 0; i < _g_acu.open_stacks_alloc; i++)
    {
        /* empty 'slots' in the registry have an invalid session ID,
         so we look for those */
        if (_g_acu.open_stacks[i].session_id == _INVALID_SESSION_ID)
        {
            StackMgrStack *the_stack = _g_acu.open_stacks + i;
            
            /* initalize the stack entry */
            memset(the_stack, 0, sizeof(StackMgrStack));
            
            /* we give each stack a session unique ID
             and reserve this 'slot',
             having a unique ID also allows handles to verify
             that the stack to which they point is still loaded
             and thus gracefully stop working without having
             to perform handle invalidation at closure. */
            the_stack->session_id = ++_g_acu_session_gen;
            _g_acu.open_stacks_count++;
            
            
            
            /* return the newly allocated registry entry */
            return the_stack;
        }
    }
    
    /* if no registry entry could be allocated,
     we raise an internal error */
    _stackmgr_raise_error(STACKMGR_ERROR_INTERNAL);
    return NULL;
}


/*
 *  _stackmgr_clear_scripting_error
 *  ---------------------------------------------------------------------------------------------
 *  Cleans up the last scripting error (if any).
 */
void _stackmgr_clear_scripting_error(StackMgrStack *in_stack)
{
    if (in_stack->thread_xte_error_template) free(in_stack->thread_xte_error_template);
    if (in_stack->thread_xte_error_arg1) free(in_stack->thread_xte_error_arg1);
    if (in_stack->thread_xte_error_arg2) free(in_stack->thread_xte_error_arg2);
    if (in_stack->thread_xte_error_arg3) free(in_stack->thread_xte_error_arg3);
    acu_release(in_stack->ut_script_error_object);
    
    in_stack->thread_xte_error_template = NULL;
    in_stack->thread_xte_error_arg1 = NULL;
    in_stack->thread_xte_error_arg2 = NULL;
    in_stack->thread_xte_error_arg3 = NULL;
    in_stack->ut_script_error_object = NULL;
}


/*
 *  _stackmgr_stack_unregister
 *  ---------------------------------------------------------------------------------------------
 *  Invalidates the registration of a stack with the ACU and releases the memory it occupied
 *  back into the pool for future loading/opening of stacks.
 */
static void _stackmgr_stack_unregister(StackMgrStack *in_stack)
{
    /* shutdown the xtalk thread */
    void _acu_xtalk_thread_shutdown(StackMgrStack *in_stack);
    _acu_xtalk_thread_shutdown(in_stack);
    
    /* close open xtalk files */
    void _acu_close_all_xtalk_files(StackMgrStack *in_stack);
    _acu_close_all_xtalk_files(in_stack);
    
    /* invalidate the entry */
    in_stack->session_id = _INVALID_SESSION_ID;
    _g_acu.open_stacks_count--;
    
    /* free allocated data */
    if (in_stack->arpool_idle) _acu_autorelease_pool_dispose(in_stack->arpool_idle);
    if (in_stack->xtalk_sys_event_queue) _acu_event_queue_dispose(in_stack->xtalk_sys_event_queue);
    if (in_stack->xtalk) xte_dispose(in_stack->xtalk);

    if (in_stack->stack) stack_close(in_stack->stack);
    if (in_stack->name_short) free(in_stack->name_short);
    if (in_stack->name_long) free(in_stack->name_long);
    
    if (in_stack->ut_dir_ic.param[0].value.string) free((char*)in_stack->ut_dir_ic.param[0].value.string);
    if (in_stack->ut_dir_ic.param[2].value.string) free((char*)in_stack->ut_dir_ic.param[2].value.string);
    
    
    if (in_stack->cb_rslt_localized_string) free(in_stack->cb_rslt_localized_string);
    
    //if (in_stack->set_var_name) free(in_stack->set_var_name);
    //if (in_stack->set_var_value) free(in_stack->set_var_value);
    
    _stackmgr_clear_scripting_error(in_stack);
    
    _acu_iconmgr_dispose(in_stack);
    
    //stackmgr_handle_release(in_stack->script_target);
}


/*
 *  _stackmgr_stack_initalize
 *  ---------------------------------------------------------------------------------------------
 *  Readies the specified Stack registry entry for use.  Fully loads the stack into the ACU and
 *  starts necessary subprocesses and units, such as xtalk.
 */
static int _stackmgr_stack_initalize(StackMgrStack *in_stack)
{
    assert(in_stack != NULL);
    
    /* disable the auto-status timer */
    in_stack->auto_status_timer = -1;

    /* get the first card and background */
    in_stack->__current_card_id = stack_card_id_for_index(in_stack->stack, 0);
    in_stack->__current_bkgnd_id = stack_card_bkgnd_id(in_stack->stack, in_stack->__current_card_id);
    
    /* create an auto-release pool */
    in_stack->arpool_idle = _acu_autorelease_pool_create();
    if (!in_stack->arpool_idle) return ACU_FALSE;
    
    /* create an instance of the xtalk engine */
    int _stackmgr_xtalk_init(StackMgrStack *in_stack);
    if (!_stackmgr_xtalk_init(in_stack)) return ACU_FALSE;
    
    /* create a system event queue */
    in_stack->xtalk_sys_event_queue = _acu_event_queue_create(ACU_SYS_EVENT_QUEUE_SIZE);
    if (!in_stack->xtalk_sys_event_queue) return ACU_FALSE;
    
    /* create a separate thread for xtalk execution */
    int _acu_xtalk_thread_startup(StackMgrStack *in_stack);
    if (!_acu_xtalk_thread_startup(in_stack)) return ACU_FALSE;
    
    /* return success */
    return ACU_TRUE;
}



/******************
 Stack <==> Handle
 */

/*
 *  _stackmgr_handle
 *  ---------------------------------------------------------------------------------------------
 *  Creates a handle for the specified stack.
 */
StackHandle _stackmgr_handle(StackMgrStack *in_registry_entry)
{
    HandleDef *handle = _stackmgr_handle_create(STACKMGR_FLAG_MAN_RELEASE);
    handle->reference.type = STACK_SELF;
    handle->reference.register_entry = in_registry_entry;
    handle->reference.session_id = in_registry_entry->session_id;
    return (StackHandle)handle;
}


/*
 *  _stackmgr_stack
 *  ---------------------------------------------------------------------------------------------
 *  Returns the stack registry entry for the stack to which a handle refers, regardless of
 *  whether the handle is a reference to the stack itself, or an object within that stack,
 *  or already a registry entry pointer.
 */
StackMgrStack* _stackmgr_stack(StackHandle in_handle)
{
    HandleDef *handle = (HandleDef*)in_handle;
    
    /* old compatibility mode Stack; isn't really a handle */
    if (_stackmgr_handle_is_stack(handle))
    {
        for (int i = 0; i < _g_acu.open_stacks_alloc; i++)
        {
            if ((_g_acu.open_stacks[i].session_id != _INVALID_SESSION_ID) &&
                (_g_acu.open_stacks[i].stack == (Stack*)in_handle))
                return _g_acu.open_stacks + i;
        }
        return NULL;
    }
    
    if (in_handle == STACKMGR_INVALID_HANDLE) return NULL;
    
    /* shortcut for internal functions to use StackMgrStack*
     anywhere a handle is accepted for external API */
    if (!IS_STACKHANDLE(in_handle))
    {
        return (StackMgrStack*)in_handle;
    }
    
    assert(IS_STACKHANDLE(in_handle));
    
    StackMgrStack *register_entry = handle->reference.register_entry;
    if (register_entry->session_id != handle->reference.session_id) return NULL;
    
    return register_entry;
}


/******************
 Stack Idle
 */


/*
 *  _stackmgr_idle
 *  ---------------------------------------------------------------------------------------------
 *  Invokes _stackmgr_poll_xtalk() on each open stack at each iteration of the main application
 *  event loop (technically in response to acu_idle).
 */
void _stackmgr_idle(int in_xtalk_only)
{
    void _acu_xtalk_poll(StackMgrStack *in_stack, int in_not_idle);
    _g_acu.active_script_count = 0;
    for (int i = 0; i < _g_acu.open_stacks_alloc; i++)
    {
        if (_g_acu.open_stacks[i].session_id != _INVALID_SESSION_ID)
        {
            acu_ui_relinquish_stack((StackHandle)&(_g_acu.open_stacks[i]));
            
    
            if (_g_acu.open_stacks[i].xt_is_executing)
            {
                _g_acu.active_script_count ++;
                
                
                
                if ((!in_xtalk_only) && (_g_acu.open_stacks[i].auto_status_timer >= 0))
                {
                    _g_acu.open_stacks[i].auto_status_timer++;
                    if (_g_acu.open_stacks[i].auto_status_timer >= _ACU_AUTO_STATUS_THRESHOLD)
                    {
                        _g_acu.open_stacks[i].auto_status_timer = -1;
                        _g_acu.callbacks.auto_status_control(_g_acu.open_stacks[i].ui_context, ACU_TRUE, ACU_TRUE);
                    }
                }
                
            }
            
            
            _acu_xtalk_poll(&(_g_acu.open_stacks[i]), in_xtalk_only);
            
            
            /*if ((!in_xtalk_only) && (_g_acu.open_stacks[i].effect_queue_playback == 0))
            {
                if (_g_acu.open_stacks[i].lock_screen > 0)
                {
                    _g_acu.open_stacks[i].lock_screen = 1;
                    _acu_unlock_screen(&(_g_acu.open_stacks[i]), ACU_FALSE);
                }
                
                _g_acu.open_stacks[i].effect_queue_count = 0;
                
                if (_g_acu.open_stacks[i].idle_repaint_required)
                {
                    _g_acu.open_stacks[i].idle_repaint_required = ACU_FALSE;
                    _g_acu.callbacks.view_refresh(_g_acu.open_stacks[i].ui_context);
                }
            }*/
        }
    }
    
    _acu_update_xtalk_timer_interval();
}






/******************
 Public API
 */

/*
 *  stackmgr_stack_open
 *  ---------------------------------------------------------------------------------------------
 *  Requests a specific stack be opened at the specified path.  <in_ui_context> should be a 
 *  pointer to the relevant UI document or controller (if there's a UI associated with the
 *  opening of this stack.)
 *
 *  On success, returns a valid StackHandle.  Otherwise returns STACKMGR_INVALID_HANDLE.
 */
StackHandle stackmgr_stack_open(char const *in_path, void *in_ui_context, int *out_error)
{
#if DEBUG
    printf("stackmgr_stack_open(): prior ACU memory usage: %ld\n", _acu_allocated());
#endif
    
    //_acu_mem_baseline();
    //_g_acu.callbacks.fatal_error(1);// debug testing only
    
    /* create a register entry */
    StackMgrStack *the_stack = _stackmgr_stack_register(in_path);
    if (!the_stack) return STACKMGR_INVALID_HANDLE;
    
    /* attempt to open the stack file */
    StackOpenStatus status;
    the_stack->stack = stack_open(in_path, (StackFatalErrorHandler)&_stackmgr_handle_stack_error, the_stack, &status);
    
    /* check for errors */
    if ((status != STACK_OK) && (status != STACK_RECOVERED))
    {
        /* invalidate and return the registry entry to the pool */
        _stackmgr_stack_unregister(the_stack);
        
        /* return an invalid handle and the error code
         provided by the Stack Unit */
        *out_error = status;
        return STACKMGR_INVALID_HANDLE;
    }
    
    /* save the short and long names of the stack */
    the_stack->name_long = _stackmgr_clone_cstr(in_path);
    if (!the_stack->name_long)
        goto stack_open_error;
    the_stack->name_short = _stackmgr_clone_cstr(stack_name(the_stack->stack));
    if (!the_stack->name_short)
        goto stack_open_error;
    
    /* initalize the stack registry entry */
    the_stack->ui_context = in_ui_context;
    if (!_stackmgr_stack_initalize(the_stack))
        goto stack_open_error;
    
    
    
    
    /* successfully opened; return a handle */
    return _stackmgr_handle(the_stack);
    
stack_open_error:
    /* invalidate and return the registry entry to the pool */
    _stackmgr_stack_unregister(the_stack);
    
    /* return an invalid handle and an error code */
    *out_error = STACKMGR_ERROR_MEMORY;
    return STACKMGR_INVALID_HANDLE;
}


/*
 *  stackmgr_stack_create
 *  ---------------------------------------------------------------------------------------------
 *  Creates a new stack at the specified path, using the supplied definition attributes.
 *  If successful, returns STACKMGR_ERROR_NONE.  Otherwise STACK_ERR_IO;
 */
int stackmgr_stack_create(char const *in_path, StackMgrStackCreateDef *in_def)
{
   
    
    
    /* attempt to create the stack file */
    Stack *stack = stack_create(in_path, in_def->card_width, in_def->card_height,
                                (StackFatalErrorHandler)&_stackmgr_handle_stack_error, NULL);
    if (!stack) return STACK_ERR_IO;
    stack_close(stack);
    return STACKMGR_ERROR_NONE;
}


/*
 *  stackmgr_stack_close
 *  ---------------------------------------------------------------------------------------------
 *  Closes the specified stack.
 */
void stackmgr_stack_close(StackHandle in_handle)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_handle);
    if (!the_stack) return;
    
    the_stack->ui_context = NULL; /* if this function ever becomes more complicated, eg.
                                        with stacks in use support, then make sure our reference
                                        to the UI is removed so we don't try and call it when
                                        the document window is closed */
    
    _stackmgr_stack_unregister(the_stack);
    
    
 #if DEBUG   
    printf("ACU Allocated Objects:\n");

    _acu_mem_print();
#endif
    
#if DEBUG
    printf("stackmgr_stack_close(): post ACU memory usage: %ld\n", _acu_allocated());
    printf("                        (before auto-release)\n");
#endif
}


/*
 *  stackmgr_stack_open_count
 *  ---------------------------------------------------------------------------------------------
 *  Returns the number of loaded/open stacks.
 */
int stackmgr_stack_open_count(void)
{
    return _g_acu.open_stacks_count;
}


/*
 *  stackmgr_stack_named
 *  ---------------------------------------------------------------------------------------------
 *  Returns a StackHandle for the stack with the specified name.  Returns STACKMGR_INVALID_HANDLE
 *  if no such stack is loaded/open.
 */
StackHandle stackmgr_stack_named(char const *in_name)
{
    for (int i = 0; i < _g_acu.open_stacks_alloc; i++)
    {
        if ((_g_acu.open_stacks[i].session_id != _INVALID_SESSION_ID) &&
            (_g_acu.open_stacks[i].name_short))
        {
            if (strcmp(in_name, _g_acu.open_stacks[i].name_short) == 0)
                return stackmgr_autorelease(_stackmgr_handle(_g_acu.open_stacks + i));
        }
    }
    return STACKMGR_INVALID_HANDLE;
}


/*
 *  stackmgr_stack_set_active
 *  ---------------------------------------------------------------------------------------------
 *  UI tells us which stack's document window currently has focus (if any).
 */
void stackmgr_stack_set_active(StackHandle in_handle)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_handle);
    
    _acu_thread_mutex_lock(&(_g_acu.debug_information_m));
    if (_g_acu.current_stack == the_stack)
    {
        _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
        return;
    }
    _g_acu.current_stack = the_stack;
    _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
    
    void _acu_unregister_vars(void);
    _acu_unregister_vars();
    
    if (_g_acu.current_stack)
    {
        /* ask xtalk thread for complete variable list */
        _acu_debug_get_all_variables(-1);
    }
}


/*
 *  stackmgr_stack_set_inactive
 *  ---------------------------------------------------------------------------------------------
 *  UI tells us which stack's document window has just lost focus.  If it's the window we 
 *  currently consider active, then set no active stack.  Otherwise, leave alone, since the one
 *  we believe is active probably still is.
 */
void stackmgr_stack_set_inactive(StackHandle in_handle)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_handle);
    
    _acu_thread_mutex_lock(&(_g_acu.debug_information_m));
    if (_g_acu.current_stack == the_stack)
    {
        _g_acu.current_stack = NULL;
        _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
        return;
    }
    _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
}


int stackmgr_stack_is_active(StackHandle in_handle)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_handle);
    assert(the_stack != NULL);
    
    int is_active;
    
    _acu_thread_mutex_lock(&(_g_acu.debug_information_m));
    is_active = (_g_acu.current_stack == the_stack);
    _acu_thread_mutex_unlock(&(_g_acu.debug_information_m));
    
    return is_active;
}


/*
 *  stackmgr_set_tool
 *  ---------------------------------------------------------------------------------------------
 *  Set the current tool for a stack.
 */
void stackmgr_set_tool(StackHandle in_handle, int in_tool)
{
    //printf("Got tool: %d\n", in_tool);
    StackMgrStack *stack = _stackmgr_stack(in_handle);
    assert(stack != NULL);
    stack->ut_disable_queue = (in_tool != AUTH_TOOL_BROWSE);
}






/******************
 Deprecated Public API
 */

/*
 *  stackmgr_stack_ptr
 *  ---------------------------------------------------------------------------------------------
 *  Convenience for older external functions that are still directly accessing the Stack Unit.
 *  Probably should be phased out prior to 1.0 release.  ***TODO***
 */
Stack* stackmgr_stack_ptr(StackHandle in_handle)
{
    /* old compatibility mode Stack; isn't really a handle */
    if (_stackmgr_handle_is_stack((HandleDef*)in_handle))
        return (Stack*)in_handle; 
    
    StackMgrStack *register_entry = _stackmgr_stack(in_handle);
    if (!register_entry) return NULL;
    return register_entry->stack;
}







