/*
 
 xTalk Engine - Message Heirarchy
 xtalk_msgheir.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Implements a message passing mechanism and the CinsScript 'message heirarchy' via callbacks
 to the environment
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


/* eventually lots of messages will get a next responder representative of cinsimp itself;
 this target should handle messages like keyDown and transfer them via callback to the appropriate
 UI widgets for normal processing, we can record an external void* context for whom sent a 
 system message and then callback with that, to ensure it's relatively easy to get messages
 back to the appropriate UI object */

/* use the engine to store error state information when unwinding */

/* use the C stack for this, ie. we just keep nesting calls as we go deeper.  we'll use an OS thread to track
 execution and allow debugging later, aborting now (v1.0), etc. */

/* return value to this indicates if we were successful, or if something bad has happened and execution has terminated probably */

/* ! SCRIPT CACHE can be ignored for now - there are a multitude of possibilities for this, some simpler
 than others, including perhaps keeping track of handler names for a given invokation of _xte_send_message and their
 compiled responders down the chain, thus avoiding identity issues, etc.
 
 realistically do need eventually a script cache, even so we can identify what message handler names are along the current
 responder chain, which might be necessary during messagebox determination too... */
/* probably use a map for this. need a good UTF-8 hashing function.  periodic rebuilding shouldn't be an issue. */




char* _xte_find_handler(XTE *in_engine, char const *in_script, int is_func, char const *in_name_utf8, int *out_handler_line_begin);
int _xte_parse_handler(XTE *in_engine, XTEAST *in_stream, int const in_checkpoint[], int checkpoint_count, int in_checkpoint_offset);


/* if it does respond to the message, return a compiled AST ready to execute for the handler;
 otherwise return NULL */

static XTEAST* _xte_target_handler(XTE *in_engine, XTEVariant *in_target, int is_func, char const *in_message)
{
    char const *script;
    int const *checkpoints;
    int checkpoint_count;
    
    if ((!in_target->value.ref.type) || (!in_target->value.ref.type->script_retr)) return NULL;
    
    int err = in_target->value.ref.type->script_retr(in_engine, in_engine->context, in_target,
                                                     &script, &checkpoints, &checkpoint_count);
    if (err != XTE_TRUE) return NULL;
    //printf("Checkpoints: %d\n", checkpoint_count);
    //printf("Script: \n%s\n", script);
    
    XTEVariant *save_me = in_engine->me;
    in_engine->me = in_target;
    
    int handler_line_begin;
    char *handler_script = _xte_find_handler(in_engine, script, is_func, in_message, &handler_line_begin);
    
    XTEAST *result = NULL;
    if (handler_script)
    {
        result = _xte_lex(in_engine, handler_script);
        free(handler_script);
    }
    
    if (result) 
    {
        _xte_parse_handler(in_engine, result, checkpoints, checkpoint_count, -(handler_line_begin - 1));
    }
    
    if (result && (result->type != XTE_AST_HANDLER))
    {
        _xte_ast_destroy(result);
        result = NULL;
    }
    
    in_engine->me = save_me;
    
    return result;
}




/* !! out_result is probably not necessary, since the result is actually tracked by engine->result
 and interpreters set_result() function 
 should return XTE_ERROR_NONE.  if it returns something else, we got a can't understand situation.
 */
int _xte_send_message(XTE *in_engine, XTEVariant *in_target, XTEVariant *in_responder, int is_func, char const *in_message,
                      XTEVariant *in_params[], int in_param_count, void* in_builtin_hint, int *io_handled)
{

    
    //printf("xTalk message: %s\n", in_message);
    int err = XTE_ERROR_NONE;
    
    /* setup handled flag if we don't have one */
    int _handled = XTE_FALSE;
    if (!io_handled) io_handled = &_handled;
    
    /* save the target if this is the first invokation of this call since the last recursive invokation chain ended */
    XTEVariant *saved_target = in_engine->the_target;
    in_engine->the_target = in_target;
    
    /* check if there is a target */
    XTEAST *handler = NULL;
    if (in_responder)
    {
        
        
        /* look for a handler in the target object */
        /* this subroutine is the one that should identify function ptrs for built-in system event handlers */
        handler = _xte_target_handler(in_engine, in_responder, is_func, in_message);
        
   
        
        if (handler != NULL)
        {
            if (!(*io_handled))
            {
                *io_handled = XTE_TRUE;
                
                //printf("XTE: Handled: %s\n", in_message);
                if (in_engine->callback.debug_message)
                    in_engine->callback.debug_message(in_engine, in_engine->context, in_message, in_engine->handler_stack_ptr + 1, XTE_TRUE);
            }
            
            
            
            /* set "me" */
            XTEVariant *save_me = in_engine->me;
            in_engine->me = in_responder;
            
            /* run handler */
            //_xte_ast_debug(handler);
            in_engine->exited_passing = XTE_FALSE;
            _xte_interpret_handler(in_engine, handler, in_params, in_param_count);
            
            /* clear "me" */
            in_engine->me = save_me;
            
            /* if handler ate the message, then return back up the call chain */
            if (!in_engine->exited_passing)
                goto _send_message_cleanup;
            
            /* if it passed the message, look for the next responder of the target */
            _xte_ast_destroy(handler);
            handler = NULL;
        }
        
        /* check for error */
        if (xte_has_error(in_engine)) goto _send_message_cleanup;
    }
    
    /* identify the next responder */
    XTEVariant *next_responder = NULL;
    if (in_responder && (in_responder->value.ref.type) && (in_responder->value.ref.type->next_responder))
        err = in_responder->value.ref.type->next_responder(in_engine, in_engine->context, in_responder, &next_responder);
    if ((!next_responder) && ( (err == XTE_ERROR_NO_OBJECT) || (err == XTE_ERROR_NONE) ))
    {
        /* if we find there is no next responder, we should examine global functions/commands
         to see if one matches and process it, if it does. */
        // this includes built-in system messages, for which we should have command handlers
        // configured in the environment to send these back to the target UI component
        // looks like StackManager is going to get lumbered with that
        // also, there's keyDown char, which if it returns, should stop a keypress.
        // thus all character typing should be dependent upon keyDown char command being
        // invoked (globally) and resulting in that char being inserted into the current
        // selection of the current field/message box
        
        err = XTE_ERROR_NONE;
        
        if (in_builtin_hint)
        {
            if (in_engine->callback.debug_message)
                in_engine->callback.debug_message(in_engine, in_engine->context, in_message, in_engine->handler_stack_ptr + 1, XTE_TRUE);
            
            if (!is_func)
            {
                XTECommandImp imp = (XTECommandImp)in_builtin_hint;
                imp(in_engine, in_engine->context, in_params, in_param_count);
            }
            else
            {
                XTEFunctionImp imp = (XTEFunctionImp)in_builtin_hint;
                _xte_set_result(in_engine, imp(in_engine, in_params, in_param_count));
            }
            
        }
        else
        {
            /* there is no handler either in CinsImp or in the stack;
             thus we don't understand the message and must output an error */
            err = XTE_ERROR_NO_HANDLER;
            
            if (!(*io_handled))
            {
                //printf("XTE: Unhandled: %s\n", in_message);
                if (in_engine->callback.debug_message)
                    in_engine->callback.debug_message(in_engine, in_engine->context, in_message, in_engine->handler_stack_ptr + 1, XTE_FALSE);
            }
            
        }
        
    }
    else if ((err == XTE_ERROR_NONE) && next_responder)
    {
        /* invoke ourselves on the next responder */
        err = _xte_send_message(in_engine, in_target, next_responder, is_func, in_message, in_params, in_param_count, in_builtin_hint, NULL);
        xte_variant_release(next_responder);
    }
    
    
_send_message_cleanup:
    
    /* cleanup and exit */
    if (handler) _xte_ast_destroy(handler);
    handler = NULL;
    
    in_engine->the_target = saved_target;
    
    return err;
}


