/*
 
 xTalk Engine API and Configuration
 xtalk_engine.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Exposes an API for configuration and integration of the xTalk engine
 
 */

#include "xtalk_internal.h"


#define DEFAULT_NUMBER_FORMAT "0.######"
#define DEFAULT_ITEM_DELIMITER ","


/*************
 Initalization
 */

void _xte_synonyms_add_builtins(XTE *in_engine);
void _xte_classes_add_builtins(XTE *in_engine);
void _xte_constants_add_builtins(XTE *in_engine);
void _xte_properties_add_builtins(XTE *in_engine);
void _xte_elements_add_builtins(XTE *in_engine);
void _xte_functions_add_builtins(XTE *in_engine);
void _xte_commands_add_builtins(XTE *in_engine);

XTE* xte_create(void *in_context)
{
    /* create an engine */
    XTE *engine = calloc(1, sizeof(struct XTE));
    if (!engine) return NULL;
    
    strcpy(engine->struct_id, _XTE_STRUCT_ID);
    
    engine->handler_stack_ptr = -1;
    
    engine->context = in_context;
    
#if DEBUG
    /* save how many bytes have been allocated at this point */
    engine->init_alloc_bytes = _xte_mem_allocated();
    _xte_mem_baseline();
#endif
    
    /* initalize the engine's built-in language terminology */
    _xte_synonyms_add_builtins(engine);
    _xte_classes_add_builtins(engine);
    _xte_properties_add_builtins(engine);
    _xte_constants_add_builtins(engine);
    _xte_elements_add_builtins(engine);
    _xte_functions_add_builtins(engine);
    _xte_commands_add_builtins(engine);
    
    /* initalize random number generator */
    srand((unsigned int)time(NULL));
    
    /* initalize item delimiter */
    engine->item_delimiter = _xte_clone_cstr(engine, DEFAULT_ITEM_DELIMITER);
    
    /* initalize the number format */
    engine->number_format = _xte_clone_cstr(engine, DEFAULT_NUMBER_FORMAT);
    
    /* define special global "it" */
    _xte_define_global(engine, "it");
    
    /* initalize date/time */
    engine->os_datetime_context = _xte_os_date_init();
    
    /* set an empty result */
    _xte_set_result(engine, xte_string_create_with_cstring(engine, ""));
    
    return engine;
}



void xte_dispose(XTE *in_engine)
{
    /* date/time */
    _xte_os_date_deinit(in_engine->os_datetime_context);
    
    /* dispose terminology dictionary */
    for (int i = 0; i < in_engine->func_count; i++)
    {
        struct XTEFunctionInt *func = in_engine->funcs[i];
        if (func->name) free(func->name);
        free(func);
    }
    if (in_engine->funcs) free(in_engine->funcs);
    
    for (int i = 0; i < in_engine->syns_count; i++)
    {
        struct XTESynonymInt *syn = in_engine->syns + i;
        if (syn->name) free(syn->name);
        for (int w = 0; w < syn->word_count; w++)
        {
            if (syn->words[w]) free(syn->words[w]);
        }
        if (syn->words) free(syn->words);
        for (int w = 0; w < syn->rep_word_count; w++)
        {
            if (syn->rep_words[w]) free(syn->rep_words[w]);
        }
        if (syn->rep_words) free(syn->rep_words);
    }
    if (in_engine->syns) free(in_engine->syns);
    
    for (int i = 0; i < in_engine->cons_count; i++)
    {
        struct XTEConstantInt *cons = in_engine->cons + i;
        if (cons->name) free(cons->name);
        for (int w = 0; w < cons->word_count; w++)
        {
            if (cons->words[w]) free(cons->words[w]);
        }
        if (cons->words) free(cons->words);
    }
    if (in_engine->cons) free(in_engine->cons);
    
    for (int i = 0; i < in_engine->ref_count; i++)
    {
        struct XTERefInt *ref = in_engine->refs + i;
        if (ref->name) free(ref->name);
        if (ref->type) free(ref->type);
        for (int w = 0; w < ref->word_count; w++)
        {
            if (ref->words[w]) free(ref->words[w]);
        }
        if (ref->words) free(ref->words);
        for (int u = 0; u < ref->uid_alt_count; u++)
        {
            if (ref->uid_alt[u].uid_name) free(ref->uid_alt[u].uid_name);
            if (ref->uid_alt[u].uid_type) free(ref->uid_alt[u].uid_type);
        }
        if (ref->uid_alt) free(ref->uid_alt);
    }
    if (in_engine->refs) free(in_engine->refs);
    
    for (int i = 0; i < in_engine->class_prop_count; i++)
    {
        struct XTEPropertyInt *prop = in_engine->class_props[i];
        if (prop->name) free(prop->name);
        if (prop->usual_type) free(prop->usual_type);
        for (int w = 0; w < prop->word_count; w++)
        {
            if (prop->words[w]) free(prop->words[w]);
        }
        if (prop->words) free(prop->words);
        if (prop) free(prop);
    }
    if (in_engine->class_props) free(in_engine->class_props);
    
    for (int i = 0; i < in_engine->global_prop_count; i++)
    {
        struct XTEPropertyInt *prop = in_engine->global_props[i];
        if (prop->name) free(prop->name);
        if (prop->usual_type) free(prop->usual_type);
        for (int w = 0; w < prop->word_count; w++)
        {
            if (prop->words[w]) free(prop->words[w]);
        }
        if (prop->words) free(prop->words);
        if (prop) free(prop);
    }
    if (in_engine->global_props) free(in_engine->global_props);
    
    for (int i = 0; i < in_engine->prop_ptr_count; i++)
    {
        struct XTEPropertyPtrs *entry = in_engine->prop_ptr_table + i;
        if (entry->name) free(entry->name);
        if (entry->ptrs) free(entry->ptrs);
    }
    if (in_engine->prop_ptr_table) free(in_engine->prop_ptr_table);
    
    for (int i = 0; i < in_engine->class_count; i++)
    {
        struct XTEClassInt *a_class = in_engine->classes[i];
        if (a_class->name) free(a_class->name);
        /* this table is full of pointers to records free'd earlier */
        if (a_class->properties) free(a_class->properties);
        if (a_class) free(a_class);
    }
    if (in_engine->classes) free(in_engine->classes);
    
    for (int i = 0; i < in_engine->cmd_prefix_count; i++)
    {
        struct XTECmdPrefix *prefix = in_engine->cmd_prefix_table + i;
        if (prefix->prefix_word) free(prefix->prefix_word);
        for (int s = 0; s < prefix->command_count; s++)
        {
            struct XTECmdInt *syntax = prefix->commands + s;
            if (syntax->pattern) _xte_bnf_destroy(syntax->pattern);
            for (int p = 0; p < syntax->param_count; p++)
            {
                if (syntax->params[p]) free(syntax->params[p]);
            }
            if (syntax->params) free(syntax->params);
            if (syntax->param_is_delayed) free(syntax->param_is_delayed);
        }
        if (prefix->commands) free(prefix->commands);
    }
    if (in_engine->cmd_prefix_table) free(in_engine->cmd_prefix_table);



    /* dispose of internal settings and operations variables */
    if (in_engine->item_delimiter) free(in_engine->item_delimiter);
    if (in_engine->number_format) free(in_engine->number_format);
    
    for (int i = 0; i < in_engine->global_count; i++)
    {
        if (in_engine->globals[i]->name) free(in_engine->globals[i]->name);
        xte_variant_release(in_engine->globals[i]->value);
        free(in_engine->globals[i]);
    }
    if (in_engine->globals) free(in_engine->globals);
    
    xte_variant_release(in_engine->the_result);
    xte_variant_release(in_engine->the_target);
    
    if (in_engine->error_message) free(in_engine->error_message);
    
    if (in_engine->temp_debug_var_value) free(in_engine->temp_debug_var_value);
    for (int i = 0; i < in_engine->temp_debug_handler_count; i++)
    {
        if (in_engine->temp_debug_handlers[i]) free(in_engine->temp_debug_handlers[i]);
    }
    if (in_engine->temp_debug_handlers) free(in_engine->temp_debug_handlers);
    
    _xte_reset_errors(in_engine);
    
    if (in_engine->f_result_cstr) free(in_engine->f_result_cstr);
    
    
    
#if DEBUG
    
    /* check that we haven't leaked at all, during operation,
     and that dispose() was successful. */
    if (_xte_mem_allocated() != in_engine->init_alloc_bytes)
    {
        printf("WARNING! xTalk: leak detected on _dispose(): %ld bytes\n", _xte_mem_allocated() - in_engine->init_alloc_bytes);
        _xte_mem_print();
        //abort();
    }
    else
        printf("xTalk: engine disposed() without leaks.\n");
    
    
    free(in_engine);
#endif
}




/*************
 Configuration
 */

void _xte_synonyms_add(XTE *in_engine, struct XTESynonymDef *in_defs);
void _xte_properties_add(XTE *in_engine, struct XTEPropertyDef *in_defs);
void _xte_constants_add(XTE *in_engine, struct XTEConstantDef *in_defs);
void _xte_classes_add(XTE *in_engine, struct XTEClassDef *in_defs);
void _xte_elements_add(XTE *in_engine, struct XTEElementDef *in_defs);
void _xte_functions_add(XTE *in_engine, struct XTEFunctionDef *in_defs);
void _xte_commands_add(XTE *in_engine, struct XTECommandDef *in_defs);


void xte_configure_environment(XTE *in_engine, struct XTEClassDef *in_classes, struct XTEConstantDef *in_consts,
                            struct XTEPropertyDef *in_props, struct XTEElementDef *in_elements,
                            struct XTEFunctionDef *in_funcs, struct XTECommandDef *in_commands,
                            struct XTESynonymDef *in_syns)
{
    /* initalize the environment's language terminology */
    _xte_synonyms_add(in_engine, in_syns);
    _xte_classes_add(in_engine, in_classes);
    _xte_properties_add(in_engine, in_props);
    _xte_constants_add(in_engine, in_consts);
    _xte_elements_add(in_engine, in_elements);
    _xte_functions_add(in_engine, in_funcs);
    _xte_commands_add(in_engine, in_commands);
}


void xte_configure_callbacks(XTE *in_engine, struct XTECallbacks in_callbacks)
{
    in_engine->callback = in_callbacks;
}



/*************
 Error Handling
 */

static void _xte_die(void)
{
    fprintf(stderr, "_xte_die() invoked during panic.");
    abort();
}


static void _xte_vpanic(XTE *in_engine, XTEError in_error, const char *in_msg_format, va_list in_list)
{
    /* don't overwrite one error with another */
    if (in_engine->error_fatal) return;
    
    /* clear existing error message */
    if (in_engine->error_message) free(in_engine->error_message);
    in_engine->error_message = NULL;
    
    /* set new error */
    if (!in_msg_format) in_msg_format = "";
    in_engine->error_message = _xte_cstr_format_fill(in_msg_format, in_list);
    in_engine->error_fatal = in_error;
    
    /* output to stdout */
    fprintf(stderr, "xtalk panic!\n");
    fprintf(stderr, "    %s\n", in_engine->error_message);
    
#ifdef XTALK_TESTS
    abort();
#endif
}


static void _xte_vraise(XTE *in_engine, XTEError in_error, const char *in_msg_format, va_list in_list)
{

    
    /* don't overwrite one error with another */
    if (in_engine->error_operational || in_engine->error_fatal) return;
    
    /* clear existing error message */
    if (in_engine->error_message) free(in_engine->error_message);
    in_engine->error_message = NULL;
    
    /// UNTIL TOTALLY PATCHED *****
    /*if (in_error == XTE_ERROR_SYNTAX)
    {
        // redirect to appropraite error handler for syntax level errors
        _xte_error_syntax(in_engine, INVALID_LINE_NUMBER, in_msg_format, NULL, NULL, NULL);
        return;
    }*/
    
    /* set new error */
    if (!in_msg_format) in_msg_format = "";
    in_engine->error_message = _xte_cstr_format_fill(in_msg_format, in_list);
    in_engine->error_operational = in_error;
    
    
    
    // this is as per protocol, but we need a callback **TODO**
    if (in_error == XTE_ERROR_MEMORY)
    {
        /* if the error is an out of memory error, terminate the application,
         invoke the out of memory callback and terminate the application */
        fprintf(stderr, "CinsImp xTalk engine: Out of memory!\n");
        abort();
    }
}


void* _xte_panic_null(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...)
{
    if (!in_engine) _xte_die();
    va_list args;
    va_start(args, in_msg_format);
    _xte_vpanic(in_engine, in_error, in_msg_format, args);
    va_end(args);
    return NULL;
}


void _xte_panic_void(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...)
{
    if (!in_engine) _xte_die();
    va_list args;
    va_start(args, in_msg_format);
    _xte_vpanic(in_engine, in_error, in_msg_format, args);
    va_end(args);
    return;
}


int _xte_panic_int(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...)
{
    if (!in_engine) _xte_die();
    va_list args;
    va_start(args, in_msg_format);
    _xte_vpanic(in_engine, in_error, in_msg_format, args);
    va_end(args);
    return 0;
}


void _xte_raise_void(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...)
{
    va_list args;
    va_start(args, in_msg_format);
    _xte_vraise(in_engine, in_error, in_msg_format, args);
    va_end(args);
}


void* _xte_raise_null(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...)
{
    va_list args;
    va_start(args, in_msg_format);
    _xte_vraise(in_engine, in_error, in_msg_format, args);
    va_end(args);
    return NULL;
}


int _xte_raise_int(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...)
{
    va_list args;
    va_start(args, in_msg_format);
    _xte_vraise(in_engine, in_error, in_msg_format, args);
    va_end(args);
    return 0;
}



static void _xte_trim_last_newline(XTEAST *io_stream)
{
    if (io_stream == NULL) return;
    if ((io_stream->children_count == 0) || (!io_stream->children[io_stream->children_count-1])) return;
    if (io_stream->children[io_stream->children_count-1]->type != XTE_AST_NEWLINE) return;
    _xte_ast_destroy(_xte_ast_list_remove(io_stream, io_stream->children_count-1));
}


static void _xte_message(XTE *in_engine, const char *in_message)
{
    XTEAST *ast;
    
    /* reset errors */
    _xte_reset_errors(in_engine);
    
    /* attempt to lex the message */
    ast = _xte_lex(in_engine, in_message);
    _xte_trim_last_newline(ast);
    
    /* if the result of the lex was empty, or there was an error... */
    if ((!ast) || (ast->children_count == 0) || _xte_has_error(in_engine))
    {
        _xte_error_post(in_engine);
        return;
    }
    
    /* attempt to parse and interpret the message as an expression */
    int dontTryExpression = XTE_FALSE;
    _xte_parse_expression(in_engine, ast, INVALID_LINE_NUMBER);
    
    /* special case: single word global variable name / command name;
     prevent a single word that isn't a global variable from coming back
     as itself, which is by default what undefined names do */
    if (ast && (ast->children_count == 1) && (ast->children[0]) && (ast->children[0]->type == XTE_AST_WORD))
    {
        if (!_xte_global_exists(in_engine, ast->children[0]->value.string)) dontTryExpression = XTE_TRUE;
    }
    
    /* interpret, if not prohibited by previous checks or parse failed */
    if ( (!_xte_has_error(in_engine)) && (!dontTryExpression) )
    {
        
        
        
        _xte_interpret(in_engine, ast);
        
        /* check for success */
        if (!_xte_has_error(in_engine))
        {
            /* inform environment of the results */
            if (in_engine->the_result)
            {
                if (xte_variant_convert(in_engine, in_engine->the_result, XTE_TYPE_STRING))
                    in_engine->callback.message_result(in_engine, in_engine->context, xte_variant_as_cstring(in_engine->the_result));
                else
                {
                    _xte_set_result(in_engine, NULL);
                }
            }
            
            /* cleanup and exit */
            _xte_ast_destroy(ast);
            if (xte_has_error(in_engine)) _xte_error_post(in_engine);
            return;
        }
    }
    
    /* cleanup */
    _xte_ast_destroy(ast);
    
    /* set error destination */
    in_engine->which_error = ERROR_2;
    
    /* attempt to lex the message */
    ast = _xte_lex(in_engine, in_message);
    _xte_trim_last_newline(ast);
    
    /* if the result of the lex was empty, or there was an error... */
    if ((!ast) || (ast->children_count == 0) || _xte_has_error(in_engine))
    {
        _xte_error_post(in_engine);
        return;
    }
    
    /* attempt to parse and interpret the message as a command */
    _xte_parse_command(in_engine, ast, INVALID_LINE_NUMBER);

    
    /* interpret, if parse was successful */
    if (!_xte_has_error(in_engine))
    {
        
        
        
        _xte_interpret(in_engine, ast);
        
        /* check for success */
        if (!_xte_has_error(in_engine))
        {
            /* make results available */
            if (in_engine->the_result)
            {
                //if (!xte_variant_convert(in_engine, in_engine->the_result, XTE_TYPE_STRING))
                //    _xte_set_result(in_engine, NULL);
            }
            
            /* cleanup and exit */
            _xte_ast_destroy(ast);
            return;
        }
    }
    
    /* cleanup */
    _xte_ast_destroy(ast);
    _xte_set_result(in_engine, NULL);
    
    /* raise any error that occurred */
    _xte_error_post(in_engine);
}


/*************
 Entry Points
 */

void xte_message(XTE *in_engine, const char *in_message, XTEVariant *in_first_responder)
{
    in_engine->invoked_via_message = XTE_TRUE;
    in_engine->me = in_first_responder;
    
    in_engine->run_state = XTE_RUNSTATE_RUN;
    _xte_message(in_engine, in_message);
    
    //xte_variant_release(in_engine->me);
    in_engine->me = NULL;
}



XTEVariant* xte_evaluate_delayed_param(XTE *in_engine, XTEVariant *in_param)
{
    /* reset errors */
    _xte_reset_errors(in_engine);
    
    /* interpret the AST */
    _xte_interpret(in_engine, in_param->value.ast);
    
    /* return 'the result' */
    return in_engine->the_result;
}




void xte_post_system_event(XTE *in_engine, XTEVariant *in_target, char const *in_event, XTEVariant *in_params[], int in_param_count)
{
    /* invoke internal command message send to target; this function is a wrapper;
     ie. call _xte_send_message() */
    in_engine->invoked_via_message = XTE_FALSE;
    _xte_reset_errors(in_engine);
    
    if (in_engine->handler_stack_ptr < 0)
        in_engine->me = NULL;

    in_engine->run_state = XTE_RUNSTATE_RUN;
    _xte_send_message(in_engine, in_target, in_target, XTE_FALSE, in_event, in_params, in_param_count, NULL, NULL);
    /* we ignored the inability to find a message handler,
     as this is a system event,
     in future, eg. for keydown we may need to detect that code returned
     from _send_message() here and respond appropriately - 
     or else allow setting a built-in handler for the message for outsiders - 
     some kind of callback will be needed anyway */
    
    _xte_error_post(in_engine);
}


void xte_abort(XTE *in_engine)
{
    in_engine->run_state = XTE_RUNSTATE_ABORT;
}



/*
 *  xte_set_result
 *  ---------------------------------------------------------------------------------------------
 *  Sets "the result" to the supplied value.
 *
 *  !  Assumes ownership of the supplied value.
 */
void xte_set_result(XTE *in_engine, XTEVariant *in_result)
{
    _xte_set_result(in_engine, in_result);
}



/*void _xte_f_result_cstr(XTE *in_engine, char const *in_result)
{
    if (in_engine->f_result_cstr) free(in_engine->f_result_cstr);
    in_engine->f_result_cstr = in_result;
}*/
