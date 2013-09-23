/*
 
 xTalk Engine (XTE), Errors
 xtalk_limits.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Error handling for various types of errors:
 
 -  Script errors; runtime and syntax (recoverable)
 -  Internal, usage and memory errors (generally advise application termination, see _engine.h)
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


/* new error unit - will allow errors to be localised.  will move raise and fatal error handlers here */

void _xte_error_syntax(XTE *in_engine, long in_source_line, char const *in_template,
                       char const *in_param1, char const *in_param2, char const *in_param3)
{
    
    if (in_engine->error_detail[in_engine->which_error].has_error) return;
    in_engine->error_detail[in_engine->which_error].has_error = XTE_TRUE;
    
    in_engine->error_detail[in_engine->which_error].posted = XTE_FALSE;
    in_engine->error_detail[in_engine->which_error].source_object = in_engine->me;
    xte_variant_retain(in_engine->me);
    
    in_engine->error_detail[in_engine->which_error].source_line = in_source_line;
    in_engine->error_detail[in_engine->which_error].msg_template = _xte_clone_cstr(in_engine, in_template);
    in_engine->error_detail[in_engine->which_error].param1 = _xte_clone_cstr(in_engine, in_param1);
    in_engine->error_detail[in_engine->which_error].param2 = _xte_clone_cstr(in_engine, in_param2);
    in_engine->error_detail[in_engine->which_error].param3 = _xte_clone_cstr(in_engine, in_param3);
    in_engine->error_detail[in_engine->which_error].is_runtime = XTE_FALSE;
}


void _xte_error_runtime(XTE *in_engine, long in_source_line, char const *in_template,
                        char const *in_param1, char const *in_param2, char const *in_param3)
{
    
    if (in_engine->error_detail[in_engine->which_error].has_error) return;
    in_engine->error_detail[in_engine->which_error].has_error = XTE_TRUE;
 
    in_engine->error_detail[in_engine->which_error].posted = XTE_FALSE;
    in_engine->error_detail[in_engine->which_error].source_object = in_engine->me;
    xte_variant_retain(in_engine->me);
    
    in_engine->error_detail[in_engine->which_error].source_line = in_source_line;
    in_engine->error_detail[in_engine->which_error].msg_template = _xte_clone_cstr(in_engine, in_template);
    in_engine->error_detail[in_engine->which_error].param1 = _xte_clone_cstr(in_engine, in_param1);
    in_engine->error_detail[in_engine->which_error].param2 = _xte_clone_cstr(in_engine, in_param2);
   in_engine->error_detail[in_engine->which_error].param3 = _xte_clone_cstr(in_engine, in_param3);
    in_engine->error_detail[in_engine->which_error].is_runtime = XTE_TRUE;
    
    if (in_engine->handler_stack_ptr < 0)
    //if (in_engine->invoked_via_message)
        in_engine->run_state = XTE_RUNSTATE_ABORT;
    else
        in_engine->run_state = XTE_RUNSTATE_DEBUG_ERROR;
    
    if (in_engine->handler_stack_ptr >= 0)
        _xte_error_post(in_engine);
   
}

/* post an error previously raised with error_runtime or error_syntax;
 posts the first one */


static void _xte_error_post_n(XTE *in_engine, int n)
{
    if (!in_engine->callback.script_error) return;
    if (in_engine->error_detail[n].posted) return;
    in_engine->error_detail[n].posted = XTE_TRUE;
    in_engine->callback.script_error(in_engine,
                                     in_engine->context,
                                     in_engine->error_detail[n].source_object,
                                     in_engine->error_detail[n].source_line,
                                     in_engine->error_detail[n].msg_template,
                                     in_engine->error_detail[n].param1,
                                     in_engine->error_detail[n].param2,
                                     in_engine->error_detail[n].param3,
                                     in_engine->error_detail[n].is_runtime);
}

/* post the error with the highest priority of the two;
 runtime errors have a higher priority */
void _xte_error_post(XTE *in_engine)
{
    if (!in_engine->callback.script_error) return;
    
    if (in_engine->error_detail[0].has_error)
    {
        if ( (in_engine->error_detail[1].has_error) && (in_engine->error_detail[1].is_runtime)
                 && (!(in_engine->error_detail[0].is_runtime)) )
            _xte_error_post_n(in_engine, 1);
        else
            _xte_error_post_n(in_engine, 0);
    }
    else if (in_engine->error_detail[1].has_error)
        _xte_error_post_n(in_engine, 1);
}


int _xte_has_postable_error(XTE *in_engine)
{
    return in_engine->error_detail[in_engine->which_error].has_error;
}

int _xte_has_error(XTE *in_engine)
{
    return (in_engine->error_fatal || in_engine->error_operational || (in_engine->error_detail[in_engine->which_error].has_error));
}

int xte_has_error(XTE *in_engine)
{
    return _xte_has_error(in_engine);
}


void _xte_reset_errors(XTE *in_engine)
{
    if (in_engine->error_operational)
    {
        in_engine->error_operational = XTE_ERROR_NONE;
        if (in_engine->error_message) free(in_engine->error_message);
        in_engine->error_message = NULL;
    }
    
    for (int i = 0; i < TWO_ERRORS; i++)
    {
        if (in_engine->error_detail[i].has_error)
        {
            in_engine->error_detail[i].has_error = XTE_FALSE;
            if (in_engine->error_detail[i].msg_template) free(in_engine->error_detail[i].msg_template);
            if (in_engine->error_detail[i].param1) free(in_engine->error_detail[i].param1);
            if (in_engine->error_detail[i].param2) free(in_engine->error_detail[i].param2);
            if (in_engine->error_detail[i].param3) free(in_engine->error_detail[i].param3);
            
            xte_variant_release(in_engine->error_detail[i].source_object);
            
            
            in_engine->error_detail[i].msg_template = NULL;
            in_engine->error_detail[i].param1 = NULL;
            in_engine->error_detail[i].param2 = NULL;
            in_engine->error_detail[i].param3 = NULL;
            
            in_engine->error_detail[i].source_object = NULL;
        }
    }
    in_engine->which_error = ERROR_1;
}


void _xte_out_of_memory(XTE *in_engine)
{
    _xte_panic_void(in_engine, XTE_ERROR_MEMORY, "Out of memory.");
    return;
}


void xte_callback_error(XTE *in_engine, char const *in_template, char const *in_param1, char const *in_param2, char const *in_param3)
{
    if (!in_template) in_template = "Can't understand arguments to command.";
    
    _xte_error_runtime(in_engine, in_engine->run_error_line, in_template, in_param1, in_param2, in_param3);
    
}


void* xte_callback_error_null(XTE *in_engine, char const *in_template, char const *in_param1, char const *in_param2, char const *in_param3)
{
    xte_callback_error(in_engine, in_template, in_param1, in_param2, in_param3);
    return NULL;
}


void xte_callback_failed(XTE *in_engine)
{
    xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
}


void* xte_callback_failed_null(XTE *in_engine)
{
    xte_callback_failed(in_engine);
    return NULL;
}

