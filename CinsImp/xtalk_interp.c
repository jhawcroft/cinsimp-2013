/*
 
 xTalk Engine Interpreter
 xtalk_interp.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Interprets the abstract syntax trees produced by the parser
 
 Basic operators, variables and language constructs are implemented within the engine; everything 
 else is implemented via callbacks to functions configured by the engine's environment.
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


/*********
 Constants
 */

/*
 *  Loop abort codes
 *  ---------------------------------------------------------------------------------------------
 *  Determines if and how a loop iteration should terminate early.
 */
#define _LOOP_ABORT_NONE 0
#define _LOOP_ABORT_NEXT 1 /* jump to the beginning of the next loop iteration */
#define _LOOP_ABORT_EXIT 2 /* exit the loop entirely */


#define _THREAD_PAUSE_USECS 100


/*********
 Utilities
 */

#ifdef ERROR_RUNTIME
#undef ERROR_RUNTIME
#endif
#define ERROR_RUNTIME(in_template, in_param1, in_param2) \
{ _xte_error_runtime(in_engine, in_engine->run_error_line, in_template, in_param1, in_param2, NULL);  }


/*
 *  _xte_resolve_ordinal
 *  ---------------------------------------------------------------------------------------------
 *  Attempts to resolve the specified ordinal to an actual element number of a collection with
 *  <in_total_items> number of elements.
 *
 *  XTE_ORD_FIRST, XTE_ORD_MIDDLE, XTE_ORD_RANDOM and XTE_ORD_LAST resolve.
 *
 *  Any other ordinal will return itself - that is an invalid number.
 */
static int _xte_resolve_ordinal(int in_total_items, int in_ordinal)
{
    assert(IS_COUNT(in_total_items));
    assert(IS_ORDINAL(in_ordinal));
    
    switch (in_ordinal)
    {
        case XTE_ORD_LAST:
            return in_total_items;
        case XTE_ORD_MIDDLE:
            if (in_total_items < 2) return in_total_items;
            else return in_total_items / 2;
        case XTE_ORD_RANDOM:
            return _xte_random(in_total_items);
    }
    
    return in_ordinal;
}


/*
 *  _xte_routine_callback
 *  ---------------------------------------------------------------------------------------------
 *  Provides an opportunity for the engine's thread to update the user interface,
 *  change debugging displays and exchange information with the user while a script is running.
 *
 *  If the engine is invoked in a separate thread, will enable control of the engine by a debug
 *  interface while a script is running.
 */
static void _xte_routine_callback(XTE *in_engine, long in_source_line)
{
    /* notify environment of pause */
    if (in_engine->run_state == XTE_RUNSTATE_PAUSE)
    {
        if ((in_engine->callback.debug_checkpoint) && (in_source_line != INVALID_LINE_NUMBER))
            in_engine->callback.debug_checkpoint(in_engine, in_engine->context, in_engine->me, in_source_line);
    }
    
    /* notify environment of progress */
    if (!in_engine->callback.script_progress) return;
    in_engine->callback.script_progress(in_engine, in_engine->context, (in_engine->run_state == XTE_RUNSTATE_PAUSE),
                                        in_engine->me, in_source_line);
    
    // eventually we may wish to short-list changes to variables, etc.  **TODO**
    // if we know that the caller is interested in tracking changes,
    // eg. if a variablewatcher window is open
}


/*
 *  _xte_should_abort
 *  ---------------------------------------------------------------------------------------------
 *  Checks the run state of the engine; should it abort or pause?  If it should pause, this
 *  function blocks until the engine is manually resumed using () .  Designed to be used with
 *  the invoking process via the script_progress callback.
 */
static int _xte_should_abort(XTE *in_engine, long in_source_line)
{
    assert(IS_XTE(in_engine));
    
    /* first priority, has the script been aborted for any reason? */
    if (in_engine->run_state == XTE_RUNSTATE_ABORT) return XTE_TRUE;
    
    /* give the engine's invoker an opportunity to do something */
    _xte_routine_callback(in_engine, in_source_line);
    in_source_line = INVALID_LINE_NUMBER;
    
    /* check the run state of the engine */
    switch (in_engine->run_state)
    {
        case XTE_RUNSTATE_RUN:
            break; /* continue script */
            
        case XTE_RUNSTATE_ABORT:
            /* first priority, has the script been aborted for any reason? */
            return XTE_TRUE;
            
        case XTE_RUNSTATE_PAUSE:
        case XTE_RUNSTATE_DEBUG_ERROR:
        {
            while ((in_engine->run_state == XTE_RUNSTATE_PAUSE) ||
                   (in_engine->run_state == XTE_RUNSTATE_DEBUG_ERROR))
            {
                /* pause for a while */
                _xte_platform_suspend_thread(_THREAD_PAUSE_USECS);
                
                /* give the engine's invoker an opportunity to do something */
                _xte_routine_callback(in_engine, in_source_line);
            }
            
            /* first priority, has the script been aborted for any reason? */
            if (in_engine->run_state == XTE_RUNSTATE_ABORT) return XTE_TRUE;
        }
    }
    
    /* give the signal to continue */
    return XTE_FALSE;
}



/*********
 Implementation
 */

/*
 *  Implementation Forward Declarations
 *  ---------------------------------------------------------------------------------------------
 *  Permit the implementation to be recursive and factored into a number of separate functions.
 */
XTEVariant* _xte_interpret_subtree(XTE *in_engine, XTEAST *in_ast);


void xte_debug_enumerate_handlers(XTE *in_engine)
{
    //return; // ** DEBUG REMOVE
    /* notify the observer */
    if (in_engine->callback.debug_handler)
        in_engine->callback.debug_handler(in_engine,
                                          in_engine->context,
                                          (char const **)in_engine->temp_debug_handlers,
                                          in_engine->temp_debug_handler_count);
}


static void _handler_stack_changed(XTE *in_engine)
{
    /* cleanup old debug list */
    for (int i = 0; i < in_engine->temp_debug_handler_count; i++)
    {
        if (in_engine->temp_debug_handlers[i]) free(in_engine->temp_debug_handlers[i]);
    }
    if (in_engine->temp_debug_handlers) free(in_engine->temp_debug_handlers);
    in_engine->temp_debug_handlers = NULL;
    in_engine->temp_debug_handler_count = 0;
    
    /* build new debug list */
    in_engine->temp_debug_handlers = calloc(in_engine->handler_stack_ptr + 2, sizeof(char*));
    if (!in_engine->temp_debug_handlers) return _xte_raise_void(in_engine, XTE_ERROR_MEMORY, NULL);
    for (int i = 0; i <= in_engine->handler_stack_ptr; i++)
    {
        in_engine->temp_debug_handlers[i] = _xte_clone_cstr(in_engine,in_engine->handler_stack[i].handler->value.handler.name);
        if (!in_engine->temp_debug_handlers[i]) return _xte_raise_void(in_engine, XTE_ERROR_MEMORY, NULL);
    }
    in_engine->temp_debug_handler_count = in_engine->handler_stack_ptr + 1;
    
    xte_debug_enumerate_handlers(in_engine);
}


/*
 *  _xte_push_handler
 *  ---------------------------------------------------------------------------------------------
 *  A user's message handler is beginning to execute.
 
 *  Track the handlers currently running on a stack to check for errors, keep track of variables 
 *  and runtime state, and allow debugging.
 */
static int _xte_push_handler(XTE *in_engine, XTEAST *in_handler, XTEVariant *in_params[], int in_param_count)
{
    assert(in_engine != NULL);
    assert(in_handler != NULL);
    
    if (in_engine->handler_stack_ptr + 1 == XTALK_LIMIT_NESTED_HANDLERS)
    {
        ERROR_RUNTIME("Too much recursion.", NULL, NULL);
        return XTE_FALSE;
    }
    
    in_engine->handler_stack_ptr++;
    struct XTEHandlerFrame *frame = in_engine->handler_stack + in_engine->handler_stack_ptr;
    
    frame->handler = in_handler;
    frame->local_count = 0;
    frame->locals = NULL;
    frame->imported_global_count = 0;
    frame->imported_globals = NULL;
    frame->param_count = in_param_count;
    frame->params = in_params;
    frame->nested_loops = 0;
    frame->loop_abort_code = _LOOP_ABORT_NONE;
    frame->return_value = NULL;
    
    frame->saved_run_error_line = (int)in_engine->run_error_line;
    in_engine->run_error_line = in_handler->source_line;
    
    _handler_stack_changed(in_engine);
    
    _xte_global_import(in_engine, "it");

    return XTE_TRUE;
}


/*
 *  _xte_push_handler
 *  ---------------------------------------------------------------------------------------------
 *  A user's message handler is finished executing.  See notes above.
 */
static void _xte_pop_handler(XTE *in_engine)
{
    assert(in_engine != NULL);
    assert(in_engine->handler_stack_ptr >= 0);
    
    //in_engine->run_error_line = INVALID_LINE_NUMBER;
    
    struct XTEHandlerFrame *frame = in_engine->handler_stack + in_engine->handler_stack_ptr;
    
    frame->handler = NULL;
    for (int i = 0; i < frame->local_count; i++)
    {
        if (frame->locals[i].name) free(frame->locals[i].name);
        if (frame->locals[i].value) xte_variant_release(frame->locals[i].value);
    }
    if (frame->locals) free(frame->locals);
    if (frame->imported_globals) free(frame->imported_globals);
    frame->local_count = 0;
    frame->param_count = 0;
    frame->params = NULL;
    frame->imported_globals = NULL;
    frame->imported_global_count = 0;
    frame->return_value = NULL;
    
    in_engine->run_error_line = frame->saved_run_error_line;
    
    in_engine->handler_stack_ptr--;
    
    _handler_stack_changed(in_engine);
}


/*
 *  _xte_interpret_ast_block
 *  ---------------------------------------------------------------------------------------------
 *  Interpret a compiled block, ie. list of statements in a handler, condition or loop.
 *
 *  Invoked indirectly from any of the following node types: XTE_AST_LIST, XTE_AST_LOOP,
 *  XTE_AST_CONDITION, XTE_AST_HANDLER.
 */
static void _xte_interpret_ast_block(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert((in_ast->type == XTE_AST_LIST) || (in_ast->type == XTE_AST_HANDLER));
    
    /* iterate over handler statements/blocks/constructs */
    for (int i = 0; i < in_ast->children_count; i++)
    {
        /* execute single statement */
        XTEAST *stmt = in_ast->children[i];
        if (stmt)
        {
            in_engine->run_error_line = in_ast->children[i]->source_line + 1;
            XTEVariant *stmt_result = _xte_interpret_subtree(in_engine, in_ast->children[i]);
            assert(stmt_result == NULL);
        }
        else in_engine->run_error_line = in_ast->source_line + 1;
        
        /* check debugging state */
        if (in_engine->run_state == XTE_RUNSTATE_DEBUG_OVER)
            in_engine->run_state = XTE_RUNSTATE_PAUSE;
        if (_xte_should_abort(in_engine, in_engine->run_error_line)) return;
        if (in_engine->exit_handler) return;
    }
}


/*
 *  _xte_interpret_ast_handler
 *  ---------------------------------------------------------------------------------------------
 *  Interpret a compiled handler.
 *
 *  The result (if not NULL) is the caller's responsibility.
 */
static XTEVariant* _xte_interpret_ast_handler(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_HANDLER);
    
    /* check debugging state */
    if ((in_engine->run_state == XTE_RUNSTATE_DEBUG_INTO) && (in_engine->handler_stack_ptr > in_engine->debug_handler_ref_point))
    {
        in_engine->run_state = XTE_RUNSTATE_PAUSE;
        if (_xte_should_abort(in_engine, in_ast->source_line)) return NULL;
    }
    
    /* reset exit flag */
    in_engine->exit_handler = XTE_FALSE;
    
    /* iterate over handler statements/blocks/constructs */
    _xte_interpret_ast_block(in_engine, in_ast);
    
    /* reset exit flag */
    in_engine->exit_handler = XTE_FALSE;
    
    /* return result */
    return in_engine->handler_stack[in_engine->handler_stack_ptr].return_value;
}


/*
 *  _xte_interpret_ast_param_names
 *  ---------------------------------------------------------------------------------------------
 *  Copies a handler's parameters to local variables with specified names.
 *
 *  Invoked indirectly from any of the following node types: XTE_AST_HANDLER.
 */
static void _xte_interpret_ast_param_names(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_PARAM_NAMES);
    assert(in_engine->handler_stack_ptr >= 0);
    
    struct XTEHandlerFrame *frame = in_engine->handler_stack + in_engine->handler_stack_ptr;
    for (int i = 0; i < in_ast->children_count; i++)
    {
        XTEAST *param_name = in_ast->children[i];
        assert(param_name != NULL);
        assert(param_name->type == XTE_AST_WORD);
        assert(param_name->value.string != NULL);
        
        XTEVariant *param_value = NULL;
        if (i < frame->param_count)
            param_value = xte_variant_retain(frame->params[i]);
        else
            param_value = xte_string_create_with_cstring(in_engine, "");
        
        struct XTEVariable *new_locals = realloc(frame->locals, sizeof(struct XTEVariable) * (frame->local_count + 1));
        if (!new_locals) OUT_OF_MEMORY_RETURN_VOID;
        frame->locals = new_locals;
        int var_index = frame->local_count++;
        
        struct XTEVariable *var = &(new_locals[var_index]);
        var->name = _xte_clone_cstr(in_engine, param_name->value.string);
        var->value = param_value;
    }
}


/*
 *  _xte_interpret_ast_loop
 *  ---------------------------------------------------------------------------------------------
 *  Interprets a loop.
 *
 *  Invoked indirectly from any of the following node types: XTE_AST_LIST, XTE_AST_LOOP,
 *  XTE_AST_CONDITION, XTE_AST_HANDLER.
 */
static void _xte_interpret_ast_loop(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_LOOP);
    
    /* setup; prepare counters, variables, etc. if any */
    int loop_count = 0;
    int loop_limit = 0;
    
    if (in_ast->engine->handler_stack_ptr >= 0)
    {
        in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].nested_loops++;
        in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].loop_abort_code = _LOOP_ABORT_NONE;
    }
    
    if ((in_ast->value.loop == XTE_AST_LOOP_COUNT_UP) ||
        (in_ast->value.loop == XTE_AST_LOOP_COUNT_DOWN))
    {
        /* set the loop count variable and loop_count */
        XTEVariant *initer = _xte_interpret_subtree(in_engine, in_ast->children[1]);
        if (!xte_variant_convert(in_engine, initer, XTE_TYPE_INTEGER))
        {
            xte_variant_release(initer);
            ERROR_RUNTIME("Expected integer start value for loop.", NULL, NULL);
            return;
        }
        loop_count = xte_variant_as_int(initer);
        xte_variant_release(initer);
        
        XTEVariant *start = xte_integer_create(in_engine, loop_count);
        _xte_variable_write(in_engine, in_ast->children[0]->value.string, start, _xte_text_make_range(-1, -1), XTE_PUT_INTO);
        xte_variant_release(start);
    }
    
    if (in_ast->value.loop == XTE_AST_LOOP_NUMBER)
    {
        XTEVariant *initer = _xte_interpret_subtree(in_engine, in_ast->children[0]);
        if (!xte_variant_convert(in_engine, initer, XTE_TYPE_INTEGER))
        {
            xte_variant_release(initer);
            ERROR_RUNTIME("Expected integer count for loop.", NULL, NULL);
            return;
        }
        loop_limit = xte_variant_as_int(initer);
        xte_variant_release(initer);
    }
    
    int begin_loop_stmts = 0;
    switch (in_ast->value.loop)
    {
        case XTE_AST_LOOP_NUMBER:
        case XTE_AST_LOOP_UNTIL:
        case XTE_AST_LOOP_WHILE:
            begin_loop_stmts = 1;
            break;
        case XTE_AST_LOOP_COUNT_DOWN:
        case XTE_AST_LOOP_COUNT_UP:
            begin_loop_stmts = 3;
            break;
    }
    
    /* actually run the loop */
    for (;;)
    {
        /* check loop counter */
        if (in_ast->value.loop == XTE_AST_LOOP_NUMBER)
        {
            if (loop_count == loop_limit) break;
        }
        else if ((in_ast->value.loop == XTE_AST_LOOP_COUNT_UP) ||
                 (in_ast->value.loop == XTE_AST_LOOP_COUNT_DOWN))
        {
            XTEVariant *initer = _xte_interpret_subtree(in_engine, in_ast->children[2]);
            if (!xte_variant_convert(in_engine, initer, XTE_TYPE_INTEGER))
            {
                xte_variant_release(initer);
                ERROR_RUNTIME("Expected integer end value for loop.", NULL, NULL);
                return;
            }
            loop_limit = xte_variant_as_int(initer);
            xte_variant_release(initer);
            
            if (loop_count == loop_limit) break;
        }
        
        /* check loop condition */
        if ((in_ast->value.loop == XTE_AST_LOOP_WHILE) ||
            (in_ast->value.loop == XTE_AST_LOOP_UNTIL))
        {
            XTEVariant *value = _xte_interpret_subtree(in_engine, in_ast->children[0]);
            if (!xte_variant_convert(in_engine, value, XTE_TYPE_BOOLEAN))
            {
                xte_variant_release(value);
                ERROR_RUNTIME("Expected true or false for loop condition.", NULL, NULL);
                return;
            }
            int should_exit = XTE_FALSE;
            if ((in_ast->value.loop == XTE_AST_LOOP_UNTIL) &&
                (value->value.boolean)) should_exit = XTE_TRUE;
            else if ((in_ast->value.loop == XTE_AST_LOOP_WHILE) &&
                     (!value->value.boolean)) should_exit = XTE_TRUE;
            xte_variant_release(value);
            if (should_exit) break;
        }
        
        /* run loop iteration */
        for (int i = begin_loop_stmts; i < in_ast->children_count; i++)
        {
            in_engine->run_error_line = in_ast->children[i]->source_line;
            _xte_interpret_subtree(in_engine, in_ast->children[i]);
            
            /* check debugging state */
            if (in_engine->run_state == XTE_RUNSTATE_DEBUG_OVER)
                in_engine->run_state = XTE_RUNSTATE_PAUSE;
            //if (_xte_should_abort(in_engine, in_ast->source_line)) return;
            if (i + 1 < in_ast->children_count)
                _xte_routine_callback(in_engine, in_ast->children[i+1]->source_line);
            else
                _xte_routine_callback(in_engine, in_ast->source_line);
            if (in_ast->engine->run_state == XTE_RUNSTATE_ABORT) break;
            if (in_ast->engine->handler_stack_ptr >= 0)
            {
                if (in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].loop_abort_code !=
                    _LOOP_ABORT_NONE) break;
            }
        }
        
        _xte_routine_callback(in_engine, in_ast->source_line);
        if (in_ast->engine->run_state == XTE_RUNSTATE_ABORT) break;
        if (in_ast->engine->handler_stack_ptr >= 0)
        {
            if (in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].loop_abort_code ==
                _LOOP_ABORT_EXIT) break;
        }
        
        /* increment / decrement loop count variable */
        if ((in_ast->value.loop == XTE_AST_LOOP_COUNT_UP) ||
            (in_ast->value.loop == XTE_AST_LOOP_NUMBER))
            loop_count++;
        else if (in_ast->value.loop == XTE_AST_LOOP_COUNT_DOWN)
            loop_count--;
        
        /* set the loop count variable to loop_count */
        if ((in_ast->value.loop == XTE_AST_LOOP_COUNT_UP) ||
            (in_ast->value.loop == XTE_AST_LOOP_COUNT_DOWN))
        {
            XTEVariant *count = xte_integer_create(in_engine, loop_count);
            _xte_variable_write(in_engine, in_ast->children[0]->value.string, count, _xte_text_make_range(-1, -1), XTE_PUT_INTO);
            xte_variant_release(count);
        }
    }
    
    /* cleanup */
    if (in_ast->engine->handler_stack_ptr >= 0)
    {
        in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].nested_loops--;
    }
    
    /* check debugging state */
    if (in_engine->run_state == XTE_RUNSTATE_DEBUG_OVER)
        in_engine->run_state = XTE_RUNSTATE_PAUSE;
    _xte_should_abort(in_engine, in_ast->source_line);
}


/*
 *  _xte_interpret_ast_condition
 *  ---------------------------------------------------------------------------------------------
 *  Interprets an if..then..else clause.
 *
 *  Invoked indirectly from any of the following node types: XTE_AST_LIST, XTE_AST_LOOP,
 *  XTE_AST_CONDITION, XTE_AST_HANDLER.
 */
static void _xte_interpret_ast_condition(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_CONDITION);
    
    for (int i = 0; i < in_ast->children_count; i += 2)
    {
        if (i + 1 == in_ast->children_count)
        {
            /* else */
            _xte_interpret_subtree(in_engine, in_ast->children[i]); // leaking returned
        }
        else
        {
            /* if */
            XTEVariant* result = _xte_interpret_subtree(in_engine, in_ast->children[i]);
            if (xte_variant_convert(in_ast->engine, result, XTE_TYPE_BOOLEAN))
            {
                /* check debugging state */
                if (in_engine->run_state == XTE_RUNSTATE_DEBUG_OVER)
                    in_engine->run_state = XTE_RUNSTATE_PAUSE;
                if (_xte_should_abort(in_engine, in_ast->source_line)) return;
                
                if (result->value.boolean)
                {
                    xte_variant_release(result);
                    _xte_interpret_subtree(in_engine, in_ast->children[i+1]);// leaking returned ** TODO ** check elsewehre too
                    break;
                }
            }
            else
            {
                xte_variant_release(result);
                ERROR_RUNTIME("Expected true or false here.", NULL, NULL);
                break;
            }
            xte_variant_release(result);
        }
    }
    
    /* check debugging state */
    if (in_engine->run_state == XTE_RUNSTATE_DEBUG_OVER)
        in_engine->run_state = XTE_RUNSTATE_PAUSE;
    _xte_should_abort(in_engine, in_ast->source_line);
}


/*
 *  _xte_interpret_ast_exit
 *  ---------------------------------------------------------------------------------------------
 *  Interpret a return, exit, pass or next statement.
 */
static void _xte_interpret_ast_exit(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_EXIT);
    
    switch (in_ast->value.exit)
    {
        case XTE_AST_EXIT_EVENT:
            /* abort the current script */
            in_ast->engine->run_state = XTE_RUNSTATE_ABORT;
            return;
            
        case XTE_AST_EXIT_PASSING:
            /* pass the current message on */
            in_ast->engine->exited_passing = XTE_TRUE;
            in_engine->exit_handler = XTE_TRUE;
            /* falls through deliberately, to exit handler */
        case XTE_AST_EXIT_HANDLER:
            /* if there's an expression in the ast, it's a return value;
             interpret it and put it into the result */
            if (in_ast->engine->handler_stack_ptr < 0)
            {
                ERROR_RUNTIME("Can't \"exit\" or \"return\" outside a message handler.", NULL, NULL);
            }
            in_engine->exit_handler = XTE_TRUE;
            if (in_ast->children_count == 1)
            {
                in_engine->handler_stack[in_engine->handler_stack_ptr].return_value =
                _xte_interpret_subtree(in_engine, in_ast->children[0]);
                return;
            }
            else
                return;
            
        case XTE_AST_EXIT_ITERATION:
            /* go to the next iteration of the loop */
            if ((in_ast->engine->handler_stack_ptr >= 0) &&
                (in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].nested_loops > 0))
            {
                in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].loop_abort_code = _LOOP_ABORT_NEXT;
            }
            else
            {
                ERROR_RUNTIME("\"next repeat\" must occur within a loop.", NULL, NULL);
            }
            break;
            
        case XTE_AST_EXIT_LOOP:
            /* exit the loop */
            if ((in_ast->engine->handler_stack_ptr >= 0) &&
                (in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].nested_loops > 0))
            {
                in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].loop_abort_code = _LOOP_ABORT_EXIT;
            }
            else
            {
                ERROR_RUNTIME("\"exit repeat\" must occur within a loop.", NULL, NULL);
            }
            break;
    }
}


/*
 *  _xte_interpret_ast_global_decl
 *  ---------------------------------------------------------------------------------------------
 *  Interpret a global variable import declaration.  Import specified global variables into the
 *  current handler scope.
 */
static void _xte_interpret_ast_global_decl(XTE *in_engine, XTEAST *in_ast)
{
    // just make sure that any global declared in here doesn't have a conflicting name
    // with an existing local, otherwise it's a runtime error
    // add these to a list of imported globals in the current stack frame
    // also ensure they're created and initalized with a default value, probably?
    // check HC implementation for guide on this
    
    // actually - important when we use _xte_variable_access to ensure that it gets created here,
    // otherwise it'll introduce bugs later... DECIDED *** ensures conflicting local and global
    // names are correctly resolved when setting a variable value - ie. goes to correct scope
    
    for (int i = 0; i < in_ast->children_count; i++)
    {
        _xte_global_import(in_ast->engine, in_ast->children[i]->value.string);
    }
}


/*
 *  _xte_interpret_ast_expression
 *  ---------------------------------------------------------------------------------------------
 *  Evaluates and returns the value of an expression.
 */
static XTEVariant* _xte_interpret_ast_expression(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_EXPRESSION);
    assert(in_ast->children_count < 2);
    
    if (in_ast->children_count == 1)
        return _xte_interpret_subtree(in_engine, in_ast->children[0]);
    else
        return NULL;
}


/*
 *  _xte_interpret_ast_constant
 *  ---------------------------------------------------------------------------------------------
 *  Returns a constant value.
 */
static XTEVariant* _xte_interpret_ast_constant(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_CONSTANT);
    
    /* get the environment provided implementation of the constant */
    XTEPropertyGetter imp = in_ast->value.ptr;
    assert(imp != NULL);
    
    /* if the constant is "of" some other object,
     evaluate what that object is, it's 'owner' */
    XTEVariant *owner = NULL;
    if (in_ast->children_count == 1)
        owner = _xte_interpret_subtree(in_engine, in_ast->children[0]);
    if (_xte_has_error(in_engine))
    {
        xte_variant_release(owner);
        return NULL;
    }
    
    /* ask the environment for the value of the constant */
    XTEVariant *result = imp(in_ast->engine, in_ast->engine->context, 0, owner, XTE_PROPREP_NORMAL);
    xte_variant_release(owner);
    if (!result)
    {
        ERROR_RUNTIME("Can't understand arguments of \"%s\".", in_ast->note, NULL);
        return NULL;
    }
    
    return result;
}


/*
 *  _xte_interpret_ast_operator
 *  ---------------------------------------------------------------------------------------------
 *  Interpret an operator and it's operands, returning the result.
 *
 *  Ownership of the operands is transferred to the relevant implementation function to permit
 *  an optmisation for some operators where one of the operands is returned, mutated, rather
 *  than allocating a new XTEVariant for the result.
 *
 *  Implementors do not have to worry about cleaning up as this function automatically releases
 *  unused operands at exit.
 */
static XTEVariant* _xte_interpret_ast_operator(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_OPERATOR);
    assert(IS_OPERATOR(in_ast->value.op));
    
    /* get the number of operands applicable to this particular operator */
    int operand_count = _xte_operator_operand_count(in_ast->value.op);
    assert(in_ast->children_count == operand_count);
    
    /* maximum number of operands is currently 2;
     evaluate the operands individually */
    XTEVariant *operands[2] = {NULL, NULL};
    if (operand_count >= 1) operands[0] = _xte_interpret_subtree(in_engine, in_ast->children[0]);
    if (operand_count >= 2) operands[1] = _xte_interpret_subtree(in_engine, in_ast->children[1]);
    
    /* prepare for the result */
    XTEVariant *result = NULL;
    
    switch (in_ast->value.op)
    {
            /* unary */
        case XTE_AST_OP_NEGATE://- (preceeded by any operator, left parenthesis, beginning of stream, newline or OF)
            result = _xte_op_math_negate(in_ast->engine, operands[0]);
            break;
            
        case XTE_AST_OP_NOT://not
            result = _xte_op_logic_not(in_ast->engine, operands[0]);
            break;
            
        case XTE_AST_OP_THERE_IS_A:// add a ptr to the class to determine if something exists
        case XTE_AST_OP_THERE_IS_NO:
            result = _xte_panic_null(in_ast->engine, XTE_ERROR_UNIMPLEMENTED, NULL);
            break;
            
            /* binary */
        case XTE_AST_OP_EQUAL://is, = equality of floats defined by book too
            result = _xte_op_comp_eq(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_NOT_EQUAL:// is not, <>
            result = _xte_op_comp_neq(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_GREATER://>
            result = _xte_op_comp_more(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_LESSER://<
            result = _xte_op_comp_less(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_LESS_EQ://<=
            result = _xte_op_comp_less_eq(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_GREATER_EQ://>=
            result = _xte_op_comp_more_eq(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_CONCAT://&
            result = _xte_op_string_concat(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_CONCAT_SP://&&
            result = _xte_op_string_concat_sp(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_IS_IN: // reverse operands for "contains"
            result = _xte_op_string_is_in(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_CONTAINS:
            result = _xte_op_string_is_in(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_IS_NOT_IN:
            result = _xte_op_string_is_not_in(in_ast->engine, operands[0], operands[1]);
            break;
            
        case XTE_AST_OP_EXPONENT://^
            result = _xte_op_math_exponent(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_MULTIPLY://*
            result = _xte_op_math_multiply(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_DIVIDE_FP:///
            result = _xte_op_math_divide_fp(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_ADD://+
            //result = xte_integer_create(in_ast->engine, 1);
            result = _xte_op_math_add(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_SUBTRACT://-
            result = _xte_op_math_subtract(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_DIVIDE_INT://div
            result = _xte_op_math_divide_int(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_MODULUS://mod
            result = _xte_op_math_modulus(in_ast->engine, operands[0], operands[1]);
            break;
            
        case XTE_AST_OP_IS_WITHIN://is within (geometric)
        case XTE_AST_OP_IS_NOT_WITHIN://is not within (geometric)
            result = _xte_panic_null(in_ast->engine, XTE_ERROR_UNIMPLEMENTED, NULL);
            break;
            
        case XTE_AST_OP_AND://and
            result = _xte_op_logic_and(in_ast->engine, operands[0], operands[1]);
            break;
        case XTE_AST_OP_OR://or
            result = _xte_op_logic_or(in_ast->engine, operands[0], operands[1]);
            break;
            
    }
    
    /* cleanup; release any operands whose address differs from the result */
    if (operands[0] && (operands[0] != result)) xte_variant_release(operands[0]);
    if (operands[1] && (operands[1] != result)) xte_variant_release(operands[1]);
    
    return result;
}


/*
 *  _xte_interpret_ast_function_call
 *  ---------------------------------------------------------------------------------------------
 *  Interpret a function call, returning the result.  The result is also available in "the result".
 */
static XTEVariant* _xte_interpret_ast_function_call(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_FUNCTION);
    
    /* build param list */
    XTEVariant **param_list = calloc(sizeof(XTEVariant*), in_ast->children_count + 1);
    if (!param_list) return _xte_panic_null(in_ast->engine, XTE_ERROR_MEMORY, NULL);
    for (int i = 0; i < in_ast->children_count; i++)
    {
        param_list[i] = _xte_interpret_subtree(in_engine, in_ast->children[i]);
        
        _xte_routine_callback(in_engine, INVALID_LINE_NUMBER);
        if (in_ast->engine->run_state == XTE_RUNSTATE_ABORT) break;
    }
    
    _xte_set_result(in_engine, NULL);
    
    /* send the message to obtain the function value */
    int err = XTE_ERROR_NO_HANDLER;
    if (in_ast->engine->run_state != XTE_RUNSTATE_ABORT)
    {
        if (in_ast->value.function.named)
        {
            /* execute a command message send */
            err = _xte_send_message(in_ast->engine, in_engine->me, in_engine->me, XTE_TRUE, in_ast->value.function.named,
                                    param_list, in_ast->children_count, in_ast->value.function.ptr, NULL);
        }
    }
    
    /* cleanup */
    for (int i = 0; i < in_ast->children_count; i++)
    {
        xte_variant_release(param_list[i]);
    }
    free(param_list);
    
    /* check the result and issue an error if necessary */
    if (err != XTE_ERROR_NONE)
    {
        if (err == XTE_ERROR_NO_OBJECT)
        {
            ERROR_RUNTIME("No such object.", in_ast->value.function.named, NULL);
        }
        else
        {
            ERROR_RUNTIME("Can't understand \"%s\".", in_ast->value.function.named, NULL);
        }
        return NULL;
    }
    if (in_engine->the_result == NULL)
    {
        ERROR_RUNTIME("Can't understand arguments to \"%s\".", in_ast->value.function.named, NULL);
        return NULL;
    }
    
    /* return "the result" as if it were our own;
     remember the parent expression will assume ownership of it */
    return xte_variant_retain(in_engine->the_result);
}



/*
 *  _xte_interpret_ast_command_call
 *  ---------------------------------------------------------------------------------------------
 *  Interpret a command call.  If there is a result, it's available only in "the result".
 */
static void _xte_interpret_ast_command_call(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_COMMAND);
    
    /* clear the result */
    _xte_set_result(in_engine, xte_string_create_with_cstring(in_engine, ""));
    
    /* build param list */
    XTEVariant **param_list = calloc(sizeof(XTEVariant*), in_ast->children_count + 1);
    if (!param_list) return _xte_panic_void(in_ast->engine, XTE_ERROR_MEMORY, NULL);
    for (int i = 0; i < in_ast->children_count; i++)
    {
        if (in_ast->children[i] && (in_ast->children[i]->flags & XTE_AST_FLAG_DELAYED_EVAL))
            param_list[i] = _xte_ast_wrap(in_ast->engine, in_ast->children[i]);
        else
            param_list[i] = _xte_interpret_subtree(in_engine, in_ast->children[i]);
        
        _xte_routine_callback(in_engine, INVALID_LINE_NUMBER);
        if (in_ast->engine->run_state == XTE_RUNSTATE_ABORT) break;
    }
    
    _xte_set_result(in_engine, NULL);
    
    /* send the message to obtain the function value */
    int err = XTE_ERROR_NO_HANDLER;
    if (in_ast->engine->run_state != XTE_RUNSTATE_ABORT)
    {
        if (in_ast->value.command.named)
        {
            /* execute a command message send */
            err = _xte_send_message(in_ast->engine, in_engine->me, in_engine->me, XTE_FALSE, in_ast->value.command.named,
                                    param_list, in_ast->children_count, in_ast->value.command.ptr, NULL);
        }
    }
    
    /* cleanup */
    for (int i = 0; i < in_ast->children_count; i++)
    {
        xte_variant_release(param_list[i]);
    }
    free(param_list);
    
    /* check the result and issue an error if necessary */
    if (err != XTE_ERROR_NONE)
    {
        //return;
        if (err == XTE_ERROR_NO_OBJECT)
        {
            ERROR_RUNTIME("No such object.", in_ast->value.command.named, NULL);
        }
        else
        {
            ERROR_RUNTIME("Can't understand \"%s\".", in_ast->value.command.named, NULL);
        }
        return;
    }
}


/*
 *  _xte_interpret_ast_variable
 *  ---------------------------------------------------------------------------------------------
 *  Creates a reference to a variable, by name.  If the variable cannot be found at evaluation,
 *  the interpreter will return the name itself as a string literal.
 */
static XTEVariant* _xte_interpret_ast_variable(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_WORD);
    
    return xte_global_ref(in_ast->engine, in_ast->value.string);
}


/*
 *  _xte_interpret_ast_property
 *  ---------------------------------------------------------------------------------------------
 *  Returns a reference to the property and it's owning object.  The property can then either be
 *  read or written by the recipient command/function.
 */
static XTEVariant* _xte_interpret_ast_property(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_PROPERTY);
    
    /* get the owner (if any) */
    XTEVariant *owner = NULL;
    if (in_ast->children_count == 1)
        owner = _xte_interpret_subtree(in_engine, in_ast->children[0]);
    
    /* resolve the owner if it's a property reference */
    if (owner && (owner->type == XTE_TYPE_PROP))
    {
        if (!owner->value.prop.ptrs->func_read)
        {
            ERROR_RUNTIME("Can't get that property.", NULL, NULL);
            xte_variant_release(owner);
            return NULL;
        }
        XTEVariant *owner_value = owner->value.prop.ptrs->func_read(in_ast->engine,
                                                                    in_ast->engine->context,
                                                                    owner->value.prop.ptrs->env_id,
                                                                    owner,
                                                                    owner->value.prop.rep);
        xte_variant_release(owner);
        owner = owner_value;
    }
    
    /* get environment provided pointers for the property,
     based on the type of the owner object */
    struct XTEClassInt *owner_class = _xte_variant_class(in_engine, owner);
    struct XTEPropertyPtr *ptrs = _xte_property_ptrs(in_ast->engine,
                                                     in_ast->value.property.pmap_entry,
                                                     owner_class);//(owner ? owner->value.ref.type : NULL)
    if (!ptrs)
    {
        ERROR_RUNTIME("Can't get that property.", NULL, NULL);
        xte_variant_release(owner);
        return NULL;
    }
    
    /* create a property reference */
    return xte_property_ref(in_ast->engine, ptrs, owner, in_ast->value.property.representation);
}


/*
 *  _xte_interpret_ast_reference
 *  ---------------------------------------------------------------------------------------------
 *  Evaluates an object reference, returning the result.
 *
 *  An object reference can be:
 *  -   a reference to a single object of a specific class,
 *  -   a range of objects of a class, or
 *  -   an entire collection of objects of a specific class
 *
 *  The object(s) may be referenced via a specific owner context/collection or globally.
 */
static XTEVariant* _xte_interpret_ast_reference(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));
    assert(in_ast != NULL);
    assert(in_ast->type == XTE_AST_REF);
    
    /* 
     evaluate parameters;
     forms:
     -  class <param2> of <param1>              eg. card id 31 of this background
     -  class <param2> to <param3> of <param1>  eg. word 1 to 2 of "Hello World."
     -  class plural of <param1>                eg. the cards of this stack
     
     param_array has 3 elements, the last of which is always NULL to enable this array to be 
     delivered directly to environment callbacks and always be NULL terminated.
     */
    XTEVariant *param_array[3] = {NULL, NULL, NULL};
    XTEVariant *owner = NULL;
    int param_count = 0;
    
    /* is this a reference to an entire collection of objects? */
    if (in_ast->value.ref.is_collection)
    {
        /* look for an owner context */
        if (in_ast->children_count > 0)
            owner = _xte_interpret_subtree(in_engine, in_ast->children[0]);
    }
    /* is this a reference to a range of objects? */
    else if (in_ast->value.ref.is_range)
    {
        /* evaluate the beginning and end of the range */
        param_array[0] = _xte_interpret_subtree(in_engine, in_ast->children[0]);
        if (!param_array[0])
        {
            ERROR_RUNTIME("Can't understand arguments to \"%s\".", in_ast->value.ref.type, NULL);
            return NULL;
        }
        param_array[1] = _xte_interpret_subtree(in_engine, in_ast->children[1]);
        if (!param_array[1])
        {
            ERROR_RUNTIME("Can't understand arguments to \"%s\".", in_ast->value.ref.type, NULL);
            xte_variant_release(param_array[0]);
            return NULL;
        }
        param_count = 2;
        
        /* look for an owner context */
        if (in_ast->children_count == 3)
            owner = _xte_interpret_subtree(in_engine, in_ast->children[2]);
    }
    /* this is a reference to a specific object */
    else
    {
        /* evaluate the object identifier */
        param_array[0] = _xte_interpret_subtree(in_engine, in_ast->children[0]);
        if (!param_array[0])
        {
            ERROR_RUNTIME("Can't understand arguments to \"%s\".", in_ast->value.ref.type, NULL);
            return NULL;
        }
        param_count = 1;
        
        /* look for an owner context */
        if (in_ast->children_count == 2)
            owner = _xte_interpret_subtree(in_engine, in_ast->children[1]);
    }
    
    /* resolve the value of any parameters */
    if (param_array[0]) _xte_resolve_value(in_ast->engine, param_array[0]);
    if (param_array[1]) _xte_resolve_value(in_ast->engine, param_array[1]);
    
    /* error_cleanup may be used to cleanup after runtime errors beyond this point: */
    
    /* 
     NOTE TO BE REVIEWED:
     if we're a reference to a property, must be able to resolve owner (ie. property) to a primitive too;
     if it wont resolve, it's an error   
     *** REALLY?  Sounds like crap.  The chunk handling routines should
     attempt a resolution of the owner to a primitive, ie. STRING type.  and if it fails, they should be
     the ones to output an error.  Our job is just to do the basics, and enable things to work smoothly.
     May require work in our chunk handling module to check for these things...
     **TODO**
     */
    
    /* translate special ordinal values automatically, eg. LAST, MIDDLE, ANY;
     we must be able to obtain a count of the elements in the relevant collection.
     only perform this translation if 
     i)   the params are indicies (not IDs or something else),  (previously: there is a .get_count callback[in_ast->value.ref.get_count])
     ii)  one of the parameters is a negative integer (ie. special ordinal) */
    if ((in_ast->value.ref.params_are_indicies) &&
        (
         (param_array[0] && (param_array[0]->type == XTE_TYPE_INTEGER) && (param_array[0]->value.integer < 0)) ||
         (param_array[1] && (param_array[1]->type == XTE_TYPE_INTEGER) && (param_array[1]->value.integer < 0))
         ))
    {
        /* request the entire collection or an object representing it;
         it is up to the environment implementation to decide what to return as it will ultimately
         be the one asked to make sense of it, not us */
        if (!in_ast->value.ref.get_by_number)
        {
            ERROR_RUNTIME("Can't get \"%s\" by number.", in_ast->value.ref.type, NULL);
            goto error_cleanup;
        }
        XTEVariant *entire_collection = in_ast->value.ref.get_by_number(in_ast->engine, in_ast->engine->context, owner, NULL, 0);
        if (!entire_collection)
        {
            ERROR_RUNTIME("Can't get \"%s\" by number.", in_ast->value.ref.type, NULL);
            goto error_cleanup;
        }
        
        /* request the number of items in the collection */
        if (!in_ast->value.ref.get_count)
        {
            xte_variant_release(entire_collection);
            ERROR_RUNTIME("Can't get \"%s\" by number.", in_ast->value.ref.type, NULL);
            goto error_cleanup;
        }
        XTEVariant *count = in_ast->value.ref.get_count(in_ast->engine, in_ast->engine->context, 0, entire_collection, XTE_PROPREP_NORMAL);
        if (!count)
        {
            xte_variant_release(entire_collection);
            ERROR_RUNTIME("Can't get \"%s\" by number.", in_ast->value.ref.type, NULL);
            goto error_cleanup;
        }
        int element_count = (count ? xte_variant_as_int(count) : 0);
        xte_variant_release(count);
        xte_variant_release(entire_collection);
        
        /* resolve the special ordinal values */
        if (param_array[0] && (param_array[0]->type == XTE_TYPE_INTEGER) && (param_array[0]->value.integer < 0))
            param_array[0]->value.integer = _xte_resolve_ordinal(element_count, param_array[0]->value.integer);
        if (param_array[1] && (param_array[1]->type == XTE_TYPE_INTEGER) && (param_array[1]->value.integer < 0))
            param_array[1]->value.integer = _xte_resolve_ordinal(element_count, param_array[1]->value.integer);
    }
    
    /* determine the appropriate environment callback */
    XTEElementGetter element_getter = NULL;
    if (in_ast->value.ref.is_collection || in_ast->value.ref.params_are_indicies)
        element_getter = in_ast->value.ref.get_by_number;
    else
    {
        /* determine which callback to use based on 
         the type of the parameters: string or number;
         prefer number whenever possible */
        if ( (param_array[0] && (param_array[0]->type == XTE_TYPE_INTEGER)) ||
            (param_array[1] && (param_array[1]->type == XTE_TYPE_INTEGER)) )
            element_getter = in_ast->value.ref.get_by_number;
        else
            element_getter = in_ast->value.ref.get_by_string;
    }
    if (element_getter == NULL)
    {
        ERROR_RUNTIME("Can't understand arguments to \"%s\".", in_ast->value.ref.type, NULL);
        goto error_cleanup;
    }
    
    /* invoke the environment callback with the parameters and owner;
     obtain the elements referenced by this reference node */
    XTEVariant *result = element_getter(in_engine, in_engine->context, owner, param_array, param_count);
    
    /* check we actually got element(s) */
    if (result == NULL)
    {
        ERROR_RUNTIME("No such %s.",
                      in_engine->callback.localized_class_name(in_engine, in_engine->context, in_ast->value.ref.type),
                      NULL);
        goto error_cleanup;
    }
    
    /* cleanup */
    for (int i = 0; i < param_count; i++)
        if (param_array[i]) xte_variant_release(param_array[i]);
    if (owner) xte_variant_release(owner);
    
    /* return the successful result */
    return result;
    
    /* cleanup and return unsuccessful after runtime error */
error_cleanup:
    for (int i = 0; i < param_count; i++)
        if (param_array[i]) xte_variant_release(param_array[i]);
    if (owner) xte_variant_release(owner);
    return NULL;
}



/*
 *  _xte_interpret_subtree
 *  ---------------------------------------------------------------------------------------------
 *  Invokes the appropriate interpreter routines depending on the type of node to execute.
 *
 *  The result (if not NULL) is the caller's responsibility.
 */
XTEVariant* _xte_interpret_subtree(XTE *in_engine, XTEAST *in_ast)
{
    assert(IS_XTE(in_engine));

    /* we allow NULL AST execution to support missing command parameters, etc. */
    if (!in_ast) return NULL;
    
    /* has the script been aborted? */
    if (_xte_should_abort(in_engine, INVALID_LINE_NUMBER)) return NULL;
    
    /* has the loop iteration been aborted? */
    if ((in_ast->engine->handler_stack_ptr >= 0) && (in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].nested_loops > 0))
    {
        if (in_ast->engine->handler_stack[in_ast->engine->handler_stack_ptr].loop_abort_code != _LOOP_ABORT_NONE) return NULL;
    }
    
    /* check debugging state */
    if ((in_engine->run_state == XTE_RUNSTATE_DEBUG_OUT) && (in_engine->handler_stack_ptr < in_engine->debug_handler_ref_point))
    {
        in_engine->run_state = XTE_RUNSTATE_PAUSE;
        if (_xte_should_abort(in_engine, in_ast->source_line)) return NULL;
    }
    
    /* is there a checkpoint set for the current source line? */
    if (in_ast->flags & XTE_AST_FLAG_IS_CHECKPOINT)
    {
        in_engine->run_state = XTE_RUNSTATE_PAUSE;
        if (_xte_should_abort(in_engine, in_ast->source_line)) return NULL;
    }
    
    /* branch according to type of node being executed */
    switch (in_ast->type)
    {
            /* script specific control nodes: */
        case XTE_AST_HANDLER:
            /* runtime errors: has source_line */
            return _xte_interpret_ast_handler(in_engine, in_ast);
            
        case XTE_AST_LIST:
            _xte_interpret_ast_block(in_engine, in_ast);
            return NULL;
        
        case XTE_AST_PARAM_NAMES:
            _xte_interpret_ast_param_names(in_engine, in_ast);
            return NULL;
            
        case XTE_AST_LOOP:
            /* runtime errors: has source_line */
            _xte_interpret_ast_loop(in_engine, in_ast);
            return NULL;
            
        case XTE_AST_CONDITION:
            /* runtime errors: has source_line */
            _xte_interpret_ast_condition(in_engine, in_ast);
            return NULL;

        case XTE_AST_EXIT:
            /* runtime errors: has source_line */
            _xte_interpret_ast_exit(in_engine, in_ast);
            return NULL;
            
        case XTE_AST_GLOBAL_DECL:
            /* runtime errors: has source_line */
            _xte_interpret_ast_global_decl(in_engine, in_ast);
            // look at introducing a runtime error for conflicting names, local same as global **TODO**
            return NULL;
            
        case XTE_AST_COMMAND:
            _xte_interpret_ast_command_call(in_engine, in_ast);
            return NULL;
            
        case XTE_AST_FUNCTION:
            return _xte_interpret_ast_function_call(in_engine, in_ast);
            
            /* expression general nodes: */
        case XTE_AST_LITERAL_STRING:
            return xte_string_create_with_cstring(in_ast->engine, in_ast->value.string);
            
        case XTE_AST_LITERAL_BOOLEAN:
            return xte_boolean_create(in_ast->engine, in_ast->value.boolean);
            
        case XTE_AST_LITERAL_REAL:
            return xte_real_create(in_ast->engine, in_ast->value.real);
            
        case XTE_AST_LITERAL_INTEGER:
            return xte_integer_create(in_ast->engine, in_ast->value.integer);
            
        case XTE_AST_EXPRESSION:
            return _xte_interpret_ast_expression(in_engine, in_ast);
            
        case XTE_AST_CONSTANT:
            return _xte_interpret_ast_constant(in_engine, in_ast);
            
        case XTE_AST_OPERATOR:
            return _xte_interpret_ast_operator(in_engine, in_ast);
            
        case XTE_AST_WORD:
            return _xte_interpret_ast_variable(in_engine, in_ast);
            
        case XTE_AST_PROPERTY:
            return _xte_interpret_ast_property(in_engine, in_ast);
            
        case XTE_AST_REF:
            return _xte_interpret_ast_reference(in_engine, in_ast);
        
        default:
            break;
    }
    
    /* shouldn't get here - node type should have been handled;
     suggest we raise an internal error in production */
    assert(0);
    return NULL;
}


/*
 *  _xte_prepare_to_interpret
 *  ---------------------------------------------------------------------------------------------
 *  Prepare the engine for this session.  Sets variables that need to be given a fixed setup
 *  regardless of how the interpreter was entered.
 */
static void _xte_prepare_to_interpret(XTE *in_engine)
{
    assert(in_engine != NULL);
    
    //in_engine->run_state = XTE_RUNSTATE_RUN;// probably should only be doing this ***TODO*** at actual entry points
    // and not delayed evaluation, etc.
}



/*********
 Internal API
 */

/*
 *  xte_callback_still_busy
 *  ---------------------------------------------------------------------------------------------
 *  May be invoked by an environment callback at regular intervals for a long running process.
 *  Provides us an opportunity to terminate that process, as well as to provide our own periodic
 *  updates to the engine's owner.
 */
int xte_callback_still_busy(XTE *in_engine)
{
    _xte_routine_callback(in_engine, INVALID_LINE_NUMBER);
    
    if (in_engine->run_state == XTE_RUNSTATE_ABORT) return XTE_ABORT;
    
    return XTE_OK;
}


/*
 *  _xte_set_result
 *  ---------------------------------------------------------------------------------------------
 *  Safely set "the result" of a function/command/expression evaluation.  Do not allow setting
 *  a reference to "the result" property itself here (cyclic reference.)
 *
 *  !  Assumes ownership of the supplied value.
 */
void _xte_set_result(XTE *in_engine, XTEVariant *in_value)
{
    if (!in_value) return;
    
    XTEVariant* bi_the_result(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation);
    if (in_value && (in_value->type == XTE_TYPE_PROP) && (in_value->value.prop.ptrs[0].func_read == &bi_the_result))
        return;
    
    if (in_engine->the_result) xte_variant_release(in_engine->the_result);
    in_engine->the_result = in_value;
}


/*
 *  _xte_interpret
 *  ---------------------------------------------------------------------------------------------
 *  Entry point for ad-hoc command call and expression evaluation.
 */
void _xte_interpret(XTE *in_engine, XTEAST *in_ast)
{
    assert(in_engine != NULL);
    assert(in_ast != NULL);
    assert(in_ast->type != XTE_AST_HANDLER);
    
    //_xte_ast_debug(in_ast);
    
    _xte_prepare_to_interpret(in_engine);
    
    _xte_set_result(in_engine, _xte_interpret_subtree(in_engine, in_ast));
}


/*
 *  _xte_interpret_handler
 *  ---------------------------------------------------------------------------------------------
 *  Entry point for handler execution.
 */
void _xte_interpret_handler(XTE *in_engine, XTEAST *in_handler, XTEVariant *in_params[], int in_param_count)
{
    assert(in_engine != NULL);
    assert(in_handler != NULL);
    assert(in_handler->type == XTE_AST_HANDLER);
    
    //_xte_ast_debug(in_handler);
    
    if (!_xte_push_handler(in_engine, in_handler, in_params, in_param_count)) return;
    
    in_engine->exited_passing = XTE_FALSE;
    _xte_prepare_to_interpret(in_engine);
    _xte_interpret_subtree(in_engine, in_handler);
    
    _xte_pop_handler(in_engine);
}


/*
 *  xte_debug_step_over
 *  ---------------------------------------------------------------------------------------------
 *  Debug function.  Causes the interpreter to effectively step over the next executable 
 *  source line (probably a single statement or condition.)  The interpreter pauses at the next
 *  executable statement.
 */
void xte_debug_step_over(XTE *in_engine)
{
    if (in_engine->run_state == XTE_RUNSTATE_ABORT) return;
    in_engine->run_state = XTE_RUNSTATE_DEBUG_OVER;
}


/*
 *  xte_debug_step_into
 *  ---------------------------------------------------------------------------------------------
 *  Debug function.  Causes the interpreter to step into the next handler invocation.  The
 *  interpreter pauses at the first statement.
 */
void xte_debug_step_into(XTE *in_engine)
{
    if (in_engine->run_state == XTE_RUNSTATE_ABORT) return;
    in_engine->debug_handler_ref_point = in_engine->handler_stack_ptr;
    in_engine->run_state = XTE_RUNSTATE_DEBUG_INTO;
}


/*
 *  xte_debug_step_out
 *  ---------------------------------------------------------------------------------------------
 *  Debug function.  Causes the interpreter to step out of the current handler invocation.  The
 *  interpreter pauses at the statement immediately following the one that caused a message
 *  send to the current handler.
 */
void xte_debug_step_out(XTE *in_engine)
{
    if (in_engine->run_state == XTE_RUNSTATE_ABORT) return;
    in_engine->debug_handler_ref_point = in_engine->handler_stack_ptr;
    in_engine->run_state = XTE_RUNSTATE_DEBUG_OUT;
}


/*
 *  xte_continue
 *  ---------------------------------------------------------------------------------------------
 *  Debug function.  Causes the interpreter to continue executing, if was previously paused for
 *  a checkpoint or by an external debugger.
 */
void xte_continue(XTE *in_engine)
{
    if (in_engine->run_state == XTE_RUNSTATE_ABORT) return;
    in_engine->run_state = XTE_RUNSTATE_RUN;
}




