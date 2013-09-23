/*
 
 xTalk Engine Variant Type
 xtalk_variant.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Variant data type used internally by the engine and externally by the language implementation;
 supports booleans, integers, reals and strings
 
 */

#include "xtalk_internal.h"


/*
 
 
 
 The functions in this file that are exposed to enivronment must never crash hard;
 always return an appropriate value, etc.
 
 **TODO** make sure of that during docs and refactoring
 
 
 */





XTEVariant* xte_variant_create(XTE *in_engine)
{
    XTEVariant *result = calloc(sizeof(struct XTEVariant), 1);
    if (!result) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    result->type = XTE_TYPE_NULL;
    result->ref_count = 1;
    return result;
}

/*
XTEVariant* xte_chunk_create(XTE *in_engine, XTEChunkType in_type, int is_count, XTEVariant *in_container, int in_offset, int in_length)
{
    XTEVariant *result = xte_variant_create(in_engine);
    if (!result) return NULL;
    result->type = XTE_TYPE_CHUNK;
    result->value.chunk.type = in_type;
    result->value.chunk.container_ref = in_container;
    if (is_count)
    {
        result->value.chunk.offset = -1;
        result->value.chunk.length = in_length;
    }
    else
    {
        result->value.chunk.offset = in_offset;
        result->value.chunk.length = in_length;
    }
    return result;
}
*/

/*
XTEVariant* xte_variant_clone(XTE *in_engine, XTEVariant *in_variant)
{
    if (!in_variant) return NULL;
    switch (in_variant->type)
    {
        case XTE_TYPE_BOOLEAN:
            return xte_boolean_create(in_engine, in_variant->value.boolean);
        case XTE_TYPE_STRING:
            return xte_string_create_with_cstring(in_engine, in_variant->value.utf8_string);
        case XTE_TYPE_INTEGER:
            return xte_integer_create(in_engine, in_variant->value.integer);
        case XTE_TYPE_REAL:
            return xte_real_create(in_engine, in_variant->value.real);
        case XTE_TYPE_GLOBAL:
            return xte_global_ref(in_engine, in_variant->value.utf8_string);
        case XTE_TYPE_OBJECT:
            
            return xte_object_ref(in_engine, in_variant->value.ref.type->name, in_variant->value.ref.ident);
        default: break;
    }
    return NULL;
}*/
/* like the deallocation problem, here we have a possible struct copying problem:  ********* TODO **
 could count clones? */


/* it is necessary to clone the AST and wrap the copy;
 the original will not stick around til when it's used again...
 OR NOT!!
 */
XTEVariant* _xte_ast_wrap(XTE *in_engine, XTEAST *in_ast)
{
    //printf("Wrapping an AST!\n");
    XTEVariant *result = xte_variant_create(in_engine);
    result->type = XTE_TYPE_AST;
    result->value.ast = in_ast;
    /* could put an expiry in here to prevent AST from being used beyond the outermost invokation of the interpreter */
    return result;
}


/*
 *  xte_variant_object_data_ptr
 *  ---------------------------------------------------------------------------------------------
 *  Returns the custom object data pointer that was used on object construction.
 */
void* xte_variant_object_data_ptr(XTEVariant *in_variant)
{
    assert(in_variant != NULL);
    assert(in_variant->type == XTE_TYPE_OBJECT);
    return in_variant->value.ref.ident;
}


/*
 *  xte_variant_is_class
 *  ---------------------------------------------------------------------------------------------
 *  Returns XTE_TRUE if the variant is of the specified class, XTE_FALSE otherwise.
 */
int xte_variant_is_class(XTEVariant *in_variant, char const *in_class_name)
{
    if (!in_variant) return XTE_FALSE;
    switch (in_variant->type)
    {
        case XTE_TYPE_BOOLEAN:
            return (strcmp(in_class_name, "boolean") == 0);
        case XTE_TYPE_INTEGER:
            return (strcmp(in_class_name, "integer") == 0);
        case XTE_TYPE_REAL:
            return (strcmp(in_class_name, "real") == 0);
        case XTE_TYPE_STRING:
            return (strcmp(in_class_name, "string") == 0);
        case XTE_TYPE_OBJECT:
            return ((in_variant->value.ref.type) && (strcmp(in_variant->value.ref.type->name, in_class_name) == 0));
        default:
            break;
    }
    return XTE_FALSE;
}


XTEVariant* xte_string_create_with_cstring(XTE *in_engine, const char *in_utf8_string)
{
    XTEVariant *result = xte_variant_create(in_engine);
    if (!result) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    result->type = XTE_TYPE_STRING;
    result->value.utf8_string = _xte_clone_cstr(in_engine, in_utf8_string);
    return result;
}


XTEVariant* xte_boolean_create(XTE *in_engine, int in_boolean)
{
    XTEVariant *result = xte_variant_create(in_engine);
    if (!result) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    result->type = XTE_TYPE_BOOLEAN;
    result->value.boolean = in_boolean;
    return result;
}


XTEVariant* xte_integer_create(XTE *in_engine, int in_integer)
{
    XTEVariant *result = xte_variant_create(in_engine);
    if (!result) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    result->type = XTE_TYPE_INTEGER;
    result->value.integer = in_integer;
    return result;
}


XTEVariant* xte_real_create(XTE *in_engine, double in_real)
{
    XTEVariant *result = xte_variant_create(in_engine);
    if (!result) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    result->type = XTE_TYPE_REAL;
    result->value.real = in_real;
    return result;
}


/*  assumes ownership of the owner (if any) */
XTEVariant* xte_property_ref(XTE *in_engine, struct XTEPropertyPtr *in_ptrs, XTEVariant *in_owner, XTEPropRep in_rep)
{
    XTEVariant *result = xte_variant_create(in_engine);
    if (!result) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    result->type = XTE_TYPE_PROP;
    result->value.prop.owner = in_owner;
    result->value.prop.ptrs = in_ptrs;
    result->value.prop.rep = in_rep;
    return result;
}


XTEVariant* xte_object_ref(XTE *in_engine, char *in_class, void *in_ident, XTEObjRefDeallocator in_dealloc)
{
    /* lookup the class */
    struct XTEClassInt *class = _xte_class_for_name(in_engine, in_class);
    if (!class)
    {
        printf("xte_object_ref(): invoked with unknown class \"%s\"\n", in_class);
        return NULL;
    }
    
    /* create the reference */
    XTEVariant *result = xte_variant_create(in_engine);
    if (!result) return NULL;
    result->type = XTE_TYPE_OBJECT;
    result->value.ref.type = class;
    result->value.ref.ident = in_ident;
    result->value.ref.deallocator = in_dealloc;
    return result;
}


XTEVariant* xte_object_create(XTE *in_engine, char const *in_class_name, void *in_data_ptr, XTEObjectDeallocator in_dealloc)
{
    return xte_object_ref(in_engine, (char*)in_class_name, in_data_ptr, in_dealloc);
}


XTEVariant* xte_global_ref(XTE *in_engine, char *in_var_name)
{
    
    /* create the reference variant */
    XTEVariant *result = xte_variant_create(in_engine);
    if (!result) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    result->type = XTE_TYPE_GLOBAL;
    result->value.utf8_string = _xte_clone_cstr(in_engine, in_var_name);
    return result;
}


/* destroys the variant, without deallocating the memory it consumes;
 allowing the same pointer to be reused for a different variant */
static void _xte_variant_zap(XTEVariant *in_variant, int in_only_external)
{
    if (!in_variant) return;
    switch (in_variant->type)
    {
        case XTE_TYPE_STRING:
        case XTE_TYPE_GLOBAL:
        case XTE_TYPE_LOCAL:
            if (!in_only_external)
            {
                if (in_variant->value.utf8_string) free(in_variant->value.utf8_string);
            }
            break;
        case XTE_TYPE_OBJECT:
            if ((in_variant->value.ref.deallocator) && (in_variant->value.ref.ident))
                in_variant->value.ref.deallocator(in_variant, in_variant->value.ref.type->name, in_variant->value.ref.ident);
            in_variant->value.ref.ident = NULL;
            break;
            
        case XTE_TYPE_PROP:
            if (in_variant->value.prop.owner)
                xte_variant_release(in_variant->value.prop.owner);
            in_variant->value.prop.owner = NULL;
            break;
            
        default: break;
    }
}

XTEVariant *track_variant = NULL;


/* direct refeences to this shall be replaced with xte_variant_release()
 and this method will have it's name changed to _xte_variant_free(). */
void xte_variant_free(XTEVariant *in_variant)
{
    if (!in_variant) return;
    if (in_variant->ref_count - 1 > 0)
    {
        in_variant->ref_count--;
        return;
    }
    
    if (track_variant && (in_variant == track_variant))
        abort();
    
    
    _xte_variant_zap(in_variant, XTE_FALSE);
    free(in_variant);
}



void xte_variant_release(XTEVariant *in_variant)
{
    xte_variant_free(in_variant);
}


XTEVariant* xte_variant_retain(XTEVariant *in_variant)
{
    if (!in_variant) return in_variant;
    in_variant->ref_count++;
    return in_variant;
}


/* these two format functions should be using a global (built-in to the engine) property, numberFormat
 as specified on page 411 */
static const char* _xte_format_integer(XTE *in_engine, int in_integer)
{
    char buffer[1024];
    char const *result = _xte_format_number_r(in_engine, in_integer, in_engine->number_format);
    if (!result)
    {
        sprintf(buffer, "%d", in_integer);
        if (in_engine->f_result_cstr) free(in_engine->f_result_cstr);
        in_engine->f_result_cstr = _xte_clone_cstr(in_engine, buffer);
        return in_engine->f_result_cstr;
    }
    return result;
}


static const char* _xte_format_real(XTE *in_engine, double in_real)
{
    char buffer[1024];
    char const *result = _xte_format_number_r(in_engine, in_real, in_engine->number_format);
    if (!result)
    {
        sprintf(buffer, "%f", in_real);
        if (in_engine->f_result_cstr) free(in_engine->f_result_cstr);
        in_engine->f_result_cstr = _xte_clone_cstr(in_engine, buffer);
        return in_engine->f_result_cstr;
    }
    return result;
}


XTEVariant* xte_variant_copy(XTE *in_engine, XTEVariant *in_variant)
{
    XTEVariant *result = xte_variant_create(in_engine);
    if (!result) return NULL;
    *result = *in_variant;
    if (in_variant->type == XTE_TYPE_STRING)
        result->value.utf8_string = _xte_clone_cstr(in_engine, in_variant->value.utf8_string);
    result->ref_count = 1;
    return result;
}


int _xte_global_exists(XTE *in_engine, const char *in_var_name)
{
    /* lookup the global variable by name */
    int var_index;
    for (var_index = 0; var_index < in_engine->global_count; var_index++)
    {
        if (_xte_compare_cstr(in_engine->globals[var_index]->name, in_var_name) == 0)
            return XTE_TRUE;
    }
    return XTE_FALSE;
}


static struct XTEVariable* _xte_global_lookup(XTE *in_engine, char const *in_var_name)
{
    int var_index;
    for (var_index = 0; var_index < in_engine->global_count; var_index++)
    {
        if (_xte_compare_cstr(in_engine->globals[var_index]->name, in_var_name) == 0)
            return in_engine->globals[var_index];
    }
    return NULL;
}


struct XTEVariable* _xte_define_global(XTE *in_engine, char const *in_var_name)
{
    struct XTEVariable **new_globals = realloc(in_engine->globals, sizeof(struct XTEVariable*) * (in_engine->global_count + 1));
    if (!new_globals) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    in_engine->globals = new_globals;
    
    struct XTEVariable *var = malloc(sizeof(struct XTEVariable));
    if (!var) return NULL;
    int var_index = in_engine->global_count++;
    new_globals[var_index] = var;
    
    var->name = _xte_clone_cstr(in_engine, in_var_name);
    var->value = xte_string_create_with_cstring(in_engine, "");
    var->is_global = XTE_TRUE;
    
    if (in_engine->callback.debug_variable)
        in_engine->callback.debug_variable(in_engine, in_engine->context, var->name, XTE_TRUE, "");
    
    return var;
}


static struct XTEVariable* _xte_define_local(XTE *in_engine, char const *in_var_name)
{
    struct XTEHandlerFrame *frame = in_engine->handler_stack + in_engine->handler_stack_ptr;
    
    struct XTEVariable *new_locals = realloc(frame->locals, sizeof(struct XTEVariable) * (frame->local_count + 1));
    if (!new_locals) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    frame->locals = new_locals;
    int var_index = frame->local_count++;
    
    struct XTEVariable *var = &(new_locals[var_index]);
    var->name = _xte_clone_cstr(in_engine, in_var_name);
    var->value = xte_string_create_with_cstring(in_engine, "");
    var->is_global = XTE_FALSE;
    
    if (in_engine->callback.debug_variable)
        in_engine->callback.debug_variable(in_engine, in_engine->context, var->name, XTE_FALSE, "");
    
    return var;
}


/* attempts to access the variable record with the given name,
 and if it doesn't exist, creates it with an appropriate scope */
static struct XTEVariable* _xte_variable_access(XTE *in_engine, char const *in_var_name, int in_create)
{
    /* if there's a handler executing; lookup the variable
     within the handler's scope,
     otherwise, lookup as a global variable */
    if (in_engine->handler_stack_ptr >= 0)
    {
        struct XTEHandlerFrame *frame = in_engine->handler_stack + in_engine->handler_stack_ptr;
        
        /* have handler; read local and imported global scope */
        for (int var_index = 0; var_index < frame->local_count; var_index++)
        {
            if (_xte_compare_cstr(frame->locals[var_index].name, in_var_name) == 0)
                return &(frame->locals[var_index]);
        }
        for (int var_index = 0; var_index < frame->imported_global_count; var_index++)
        {
            if (_xte_compare_cstr(frame->imported_globals[var_index]->name, in_var_name) == 0)
                return frame->imported_globals[var_index];
        }
        
        /* variable doesn't exist; create local */
        if (in_create)
            return _xte_define_local(in_engine, in_var_name);
    }
    else
    {
        /* no handler; read globle scope only */
        for (int var_index = 0; var_index < in_engine->global_count; var_index++)
        {
            if (_xte_compare_cstr(in_engine->globals[var_index]->name, in_var_name) == 0)
                return in_engine->globals[var_index];
        }
        
        /* variable doesn't exist; create global */
        if (in_create)
            return _xte_define_global(in_engine, in_var_name);
    }
    
    return NULL;
}



static void _xte_report_variable_mutation(XTE *in_engine, struct XTEVariable *in_var)
{
    assert(IS_XTE(in_engine));
    assert(in_var != NULL);
    
    
    
    /* grab the value of the variable as a string for reporting */
    /// ***TODO**** should be able to reuse/combine parts of the variant conversion mechanism hopefully in future
    if (in_engine->temp_debug_var_value) free(in_engine->temp_debug_var_value);
    in_engine->temp_debug_var_value = NULL;
    if (in_var->value)
    {
        switch (in_var->value->type)
        {
            case XTE_TYPE_STRING:
                in_engine->temp_debug_var_value = _xte_clone_cstr(in_engine, in_var->value->value.utf8_string);
                break;
            case XTE_TYPE_INTEGER:
                in_engine->temp_debug_var_value = _xte_clone_cstr(in_engine, _xte_format_integer(in_engine, in_var->value->value.integer));
                break;
            case XTE_TYPE_REAL:
                in_engine->temp_debug_var_value = _xte_clone_cstr(in_engine, _xte_format_real(in_engine, in_var->value->value.real));
                break;
            case XTE_TYPE_BOOLEAN:
                if (in_var->value->value.boolean)
                    in_engine->temp_debug_var_value = _xte_clone_cstr(in_engine, "true");
                else
                    in_engine->temp_debug_var_value = _xte_clone_cstr(in_engine, "false");
                break;
            default: break;
        }
    }
    if (!in_engine->temp_debug_var_value)
        in_engine->temp_debug_var_value = _xte_clone_cstr(in_engine, "");
    
    /* report the mutation */
    if (in_engine->callback.debug_variable)
        in_engine->callback.debug_variable(in_engine, in_engine->context, in_var->name, in_var->is_global, in_engine->temp_debug_var_value);
}



void _xte_global_import(XTE *in_engine, char const *in_var_name)
{
    struct XTEVariable *var = NULL;
    
    for (int var_index = 0; var_index < in_engine->global_count; var_index++)
    {
        if (_xte_compare_cstr(in_engine->globals[var_index]->name, in_var_name) == 0)
        {
            var = in_engine->globals[var_index];
            break;
        }
    }
    if (!var) var = _xte_define_global(in_engine, in_var_name);
    
    struct XTEHandlerFrame *frame = in_engine->handler_stack + in_engine->handler_stack_ptr;
    
    struct XTEVariable **new_imported_globals = realloc(frame->imported_globals, sizeof(struct XTEVariable*) *
                                                             (frame->imported_global_count + 1));
    if (!new_imported_globals) return _xte_raise_void(in_engine, XTE_ERROR_MEMORY, NULL);
    frame->imported_globals = new_imported_globals;
    new_imported_globals[frame->imported_global_count++] = var;
    
    _xte_report_variable_mutation(in_engine, var);
}


static XTEVariant* _xte_variable_read(XTE *in_engine, char const *in_var_name)
{
    /* lookup variable */
    struct XTEVariable *the_var = _xte_variable_access(in_engine, in_var_name, XTE_FALSE);
    
    /* if it doesn't exist, return the variable name itself */
    if (the_var == NULL)
        return xte_string_create_with_cstring(in_engine, in_var_name);
    
    /* return the value of the variable */
    return xte_variant_copy(in_engine, the_var->value);
}





long _xte_utf8_count_bytes_in_range(char const *in_string, long in_start, long in_end);
char const* _xte_utf8_index_char(char const *in_string, long const in_char_offset);

void _xte_variable_write(XTE *in_engine, char const *in_var_name, XTEVariant *in_value, XTETextRange in_range, XTEPutMode in_mode)
{
    /* lookup the variable;
     create it if necessary */
    struct XTEVariable *var = _xte_variable_access(in_engine, in_var_name, XTE_TRUE);
    
    /* mutate the variable */
    if (in_value == var->value) return; /* don't try to mutate to itself */
    if ((in_range.offset < 0) && (in_mode == XTE_PUT_INTO))
    {
        /* simple mutation; replace entire variable with new value */
        xte_variant_release(var->value);
        var->value = xte_variant_copy(in_engine, in_value);
    }
    else
    {
        /* convert the new value to a string */
        if (!xte_variant_convert(in_engine, in_value, XTE_TYPE_STRING))
        {
            xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
            return;
        }
        
        /* complex mutation; replace specific range of variable with new value;
         first we must convert the variable to a string
         (because strings are the only type of variable we support using ranges & modes on
         as variables, at this time - in future this could change!) */
        if (!xte_variant_convert(in_engine, var->value, XTE_TYPE_STRING))
        {
            /* if we can't convert it, just overwrite it completely */
            xte_variant_release(var->value);
            var->value = xte_string_create_with_cstring(in_engine, "");
        }
        char const *existing_string = xte_variant_as_cstring(var->value);
        
        /* if the range is unspecified, compute the length now */
        if (in_range.offset < 0)
        {
            in_range.offset = 0;
            in_range.length = _xte_utf8_strlen(existing_string);
        }
        
        /* transform replace range based on mode */
        if (in_mode == XTE_PUT_BEFORE)
            in_range.length = 0;
        else if (in_mode == XTE_PUT_AFTER)
        {
            in_range.offset += in_range.length;
            in_range.length = 0;
        }
        
        /* if there's a range, grab the length of the substring in bytes */
        long substring_bytes = 0;
        if (in_range.length > 0)
            substring_bytes = _xte_utf8_count_bytes_in_range(existing_string, in_range.offset, in_range.length);
        
        /* grab the part before and after the range being replaced */
        char const *first_part = existing_string;
        long first_part_size = _xte_utf8_count_bytes_in_range(existing_string, 0, in_range.offset);
        char const *last_part = _xte_utf8_index_char(existing_string, in_range.offset + in_range.length);
        long last_part_size = strlen(last_part);
        
        /* grab the new string value */
        char const *new_part = xte_variant_as_cstring(in_value);
        long new_part_size = strlen(new_part);
        
        /* build the new string */
        long new_string_len = first_part_size + new_part_size + last_part_size + 1;
        char *new_string = malloc(new_string_len);
        if (!new_string) return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
        memcpy(new_string, first_part, first_part_size);
        memcpy(new_string + first_part_size, new_part, new_part_size);
        memcpy(new_string + first_part_size + new_part_size, last_part, last_part_size + 1);
        
        /* replace the variable value with the new string */
        xte_variant_release(var->value);
        var->value = xte_variant_create(in_engine);
        if (!var->value) return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
        var->value->type = XTE_TYPE_STRING;
        var->value->value.utf8_string = new_string;
    }
    
    /* report the variable mutation to the debugger as required */
    if (var)
        _xte_report_variable_mutation(in_engine, var);
   
}






/* returns a variant for the primitive value of the supplied variant;
 resolving the value if it's a reference to a variable, object or property;
 will always return a new variant object and doesn't change the input variant */
XTEVariant* xte_variant_value(XTE *in_engine, XTEVariant *in_variant)
{
    if (!in_variant) return NULL;
    switch (in_variant->type)
    {
        case XTE_TYPE_PROP:
            if (in_variant->value.prop.ptrs->func_read)
                return in_variant->value.prop.ptrs->func_read(in_engine,
                                                              in_engine->context,
                                                              in_variant->value.prop.ptrs->env_id,
                                                              in_variant->value.prop.owner,
                                                              in_variant->value.prop.rep);
            return NULL;
        case XTE_TYPE_GLOBAL:
            return _xte_variable_read(in_engine, in_variant->value.utf8_string);
        case XTE_TYPE_OBJECT:
            if (in_variant->value.ref.type->container_read)
                return in_variant->value.ref.type->container_read(in_engine, in_engine->context, 0, in_variant, XTE_PROPREP_NORMAL);
            return NULL;
        case XTE_TYPE_BOOLEAN:
            return xte_boolean_create(in_engine, in_variant->value.boolean);
        case XTE_TYPE_STRING:
            return xte_string_create_with_cstring(in_engine, in_variant->value.utf8_string);
        case XTE_TYPE_INTEGER:
            return xte_integer_create(in_engine, in_variant->value.integer);
        case XTE_TYPE_REAL:
            return xte_real_create(in_engine, in_variant->value.real);
        default: break;
    }
    return NULL;
}


/** TODO ** ensure that offset and length of the container is modified, only,
 and if length is -1, then use the entire length of the container 
 
 should this actually be part of PUT
 
 value must be a variant, so we don't loose precision
 
 */


/*
 
 if (in_mode == XTE_PUT_AFTER)
 {
 out_write->range_bytes.offset += out_write->range_bytes.length;
 out_write->range_bytes.length = 0;
 out_write->range_chars.offset += out_write->range_chars.length;
 out_write->range_chars.length = 0;
 }
 else if (in_mode == XTE_PUT_BEFORE)
 {
 out_write->range_bytes.length = 0;
 out_write->range_chars.length = 0;
 }
 */

int xte_container_write(XTE *in_engine, XTEVariant *in_container, XTEVariant *in_value, XTETextRange in_range, XTEPutMode in_mode)
{
    if (!in_container) return _xte_panic_int(in_engine, XTE_ERROR_INTERNAL, NULL);
    switch (in_container->type)
    {
        case XTE_TYPE_GLOBAL:
            _xte_variable_write(in_engine, in_container->value.utf8_string, in_value, in_range, in_mode);
            return XTE_TRUE;
        case XTE_TYPE_OBJECT:
            if (!in_container->value.ref.type->container_write) return XTE_FALSE;
            in_container->value.ref.type->container_write(in_engine, in_engine->context, in_container, in_value, in_range, in_mode);
            return XTE_TRUE;
        default: break;
    }
    return _xte_panic_int(in_engine, XTE_ERROR_INTERNAL, NULL);
}


int xte_property_write(XTE *in_engine, XTEVariant *in_property, XTEVariant *in_value)
{
    if (!in_property->value.prop.ptrs->func_write)
        return XTE_FALSE;
    in_property->value.prop.ptrs->func_write(in_engine, in_engine->context, in_property->value.prop.ptrs->env_id, in_property->value.prop.owner, in_value);
    return XTE_TRUE;
}


static void _xte_variants_swap_ptrs(XTEVariant *in_var1, XTEVariant *in_var2)
{
    XTEVariant temp_var = *in_var1;
    *in_var1 = *in_var2;
    *in_var2 = temp_var;
}


XTEVariant* _xte_resolve_value(XTE *in_engine, XTEVariant *in_variant)
{
    if (!in_variant) return NULL;
    switch (in_variant->type)
    {
        case XTE_TYPE_PROP:
        {
            /* get the value */
            XTEVariant *prop_value = xte_variant_value(in_engine, in_variant);
            if (!prop_value)
            {
                xte_callback_error(in_engine, "Can't get that property.", NULL, NULL, NULL);
                return in_variant;// don't leak
            }
            
            /* cleanup */
            xte_variant_release(in_variant->value.prop.owner);
            
            /* return the property value */
            *in_variant = *prop_value;
            free(prop_value);
            break;
        }
        case XTE_TYPE_GLOBAL:
        {
            /* get the value */
            XTEVariant *global_value = xte_variant_value(in_engine, in_variant);//_xte_global_read(in_engine, in_variant->value.utf8_string);
            
            /* replace the input variant with the value */
            if (in_variant->value.utf8_string) free(in_variant->value.utf8_string);
            *in_variant = *global_value;
            
            /* cleanup */
            global_value->value.utf8_string = NULL;
            xte_variant_release(global_value);
            break;
        }
        case XTE_TYPE_OBJECT:
        {
            /* if the object has a content accessor; ie. is a container,
             then get the text of the object,
             otherwise fail with a can't understand */
            XTEVariant *container_value = xte_variant_value(in_engine, in_variant);
            
            if (container_value)
            {
                _xte_variants_swap_ptrs(container_value, in_variant);
                xte_variant_release(container_value); /* actually releasing the object reference;
                                                       as the pointers were swapped above */
            }
            else
            {
                xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
                return in_variant;// don't leak
            }
            
            break;
        }
        default: break;
    }
    return in_variant;
}


XTEVariantType xte_variant_type(XTEVariant *in_variant)
{
    if (!in_variant) return XTE_TYPE_NULL;
    return in_variant->type;
}


struct XTEClassInt* _xte_variant_class(XTE *in_engine, XTEVariant *in_variant)
{
    if (!in_variant) return NULL;
    if (in_variant->type == XTE_TYPE_OBJECT)
        return in_variant->value.ref.type;
    if (in_variant->type == XTE_TYPE_INTEGER)
        return _xte_class_for_name(in_engine, "integer");
    else if (in_variant->type == XTE_TYPE_REAL)
        return _xte_class_for_name(in_engine, "real");
    else if (in_variant->type == XTE_TYPE_BOOLEAN)
        return _xte_class_for_name(in_engine, "boolean");
    else if (in_variant->type == XTE_TYPE_STRING)
        return _xte_class_for_name(in_engine, "string");
    return NULL;
}


int xte_variant_is_variable(XTEVariant *in_variant)
{
    if (!in_variant) return XTE_FALSE;
    return ((in_variant->type == XTE_TYPE_GLOBAL) || (in_variant->type == XTE_TYPE_LOCAL));
}


static int _xte_cstr_is_numeric(char const *in_string)
{
    if (!in_string) in_string = "";
    int has_dec_pt = XTE_FALSE;
    if (in_string[0] == '-') in_string++;
    for (char const *ptr = in_string; *ptr != 0; ptr++)
    {
        if (isdigit(*ptr)) continue;
        else if (*ptr == '.')
        {
            if (!has_dec_pt) has_dec_pt = XTE_TRUE;
            else return XTE_FALSE;
        }
        else return XTE_FALSE;
    }
    return XTE_TRUE;
}


int xte_variant_convert(XTE *in_engine, XTEVariant *in_variant, XTEVariantType in_new_type)
{
    if (!in_variant) return XTE_FALSE;

    /* translate special types */
    if (in_new_type == XTE_TYPE_NUMBER)
    {
        if ((in_variant->type == XTE_TYPE_INTEGER) ||
            (in_variant->type == XTE_TYPE_REAL)) return XTE_TRUE;
        in_new_type = XTE_TYPE_REAL;
    }
    else if (in_new_type == XTE_TYPE_VALUE)
    {
        if ((in_variant->type == XTE_TYPE_INTEGER) ||
            (in_variant->type == XTE_TYPE_REAL) ||
            (in_variant->type == XTE_TYPE_BOOLEAN) ||
            (in_variant->type == XTE_TYPE_STRING)) return XTE_TRUE;
        
        /* resolve reference types */
        in_variant = _xte_resolve_value(in_engine, in_variant);
        
        if ((in_variant->type == XTE_TYPE_INTEGER) ||
            (in_variant->type == XTE_TYPE_REAL) ||
            (in_variant->type == XTE_TYPE_BOOLEAN) ||
            (in_variant->type == XTE_TYPE_STRING)) return XTE_TRUE;
        
        return XTE_FALSE;
    }
    
    /* resolve reference types */
    in_variant = _xte_resolve_value(in_engine, in_variant);
    if (!in_variant) return XTE_FALSE;
    
    /* check if conversion is necessary */
    if (in_new_type == in_variant->type) return XTE_TRUE;
    
    /* variant is about to be converted;
     release any externally allocated memory belonging to the variant */
    _xte_variant_zap(in_variant, XTE_TRUE);
    
    /* branch to appropriate conversion routine */
    switch (in_new_type)
    {
        case XTE_TYPE_STRING:
        {
            switch (in_variant->type)
            {
                case XTE_TYPE_BOOLEAN:
                    if (in_variant->value.boolean) in_variant->value.utf8_string = _xte_clone_cstr(in_engine, "true");
                    else in_variant->value.utf8_string = _xte_clone_cstr(in_engine, "false");
                    in_variant->type = XTE_TYPE_STRING;
                    return XTE_TRUE;
                case XTE_TYPE_INTEGER:
                    in_variant->value.utf8_string = _xte_clone_cstr(in_engine, _xte_format_integer(in_engine, in_variant->value.integer));
                    in_variant->type = XTE_TYPE_STRING;
                    return XTE_TRUE;
                case XTE_TYPE_REAL:
                    in_variant->value.utf8_string = _xte_clone_cstr(in_engine, _xte_format_real(in_engine, in_variant->value.real));
                    in_variant->type = XTE_TYPE_STRING;
                    return XTE_TRUE;
                case XTE_TYPE_NULL:
                    in_variant->value.utf8_string = _xte_clone_cstr(in_engine, "NULL");
                    in_variant->type = XTE_TYPE_STRING;
                    return XTE_TRUE;
                default: break;
            }
            break;
        }
        case XTE_TYPE_BOOLEAN:
        {
            switch (in_variant->type)
            {
                case XTE_TYPE_STRING:
                {
                    int bool_val = -1;
                    if (_xte_compare_cstr(in_variant->value.utf8_string, "false") == 0) bool_val = 0;
                    else if (_xte_compare_cstr(in_variant->value.utf8_string, "true") == 0) bool_val = 1;
                    if (bool_val < 0) return XTE_FALSE;
                    if (in_variant->value.utf8_string) free(in_variant->value.utf8_string);
                    in_variant->value.boolean = bool_val;
                    in_variant->type = XTE_TYPE_BOOLEAN;
                    return XTE_TRUE;
                }
                default: break;
            }
            break;
        }
        case XTE_TYPE_INTEGER:
        {
            switch (in_variant->type)
            {
                case XTE_TYPE_STRING:
                {
                    if (!_xte_cstr_is_numeric(in_variant->value.utf8_string))
                        return XTE_FALSE;
                    int integer = atoi(in_variant->value.utf8_string);
                    if (in_variant->value.utf8_string) free(in_variant->value.utf8_string);
                    in_variant->value.integer = integer;
                    in_variant->type = XTE_TYPE_INTEGER;
                    return XTE_TRUE;
                }
                case XTE_TYPE_REAL:
                {
                    in_variant->value.integer = in_variant->value.real;
                    in_variant->type = XTE_TYPE_INTEGER;
                    return XTE_TRUE;
                }
                default: break;
            }
            break;
        }
        case XTE_TYPE_REAL:
        {
            switch (in_variant->type)
            {
                case XTE_TYPE_STRING:
                {
                    if (!_xte_cstr_is_numeric(in_variant->value.utf8_string))
                        return XTE_FALSE;
                    int real = atof(in_variant->value.utf8_string);
                    if (in_variant->value.utf8_string) free(in_variant->value.utf8_string);
                    in_variant->value.real = real;
                    in_variant->type = XTE_TYPE_REAL;
                    return XTE_TRUE;
                }
                case XTE_TYPE_INTEGER:
                {
                    in_variant->value.real = in_variant->value.integer;
                    in_variant->type = XTE_TYPE_REAL;
                    return XTE_TRUE;
                }
                default: break;
            }
            break;
        }
        default:
            break;
    }
    return XTE_FALSE;
}


/* takes two variants and tries to make them both the same type;
 automatically selecting the best type for the most accurate comparison */
int _xte_make_variants_comparable(XTE *in_engine, XTEVariant *in_variant1, XTEVariant *in_variant2)
{
    _xte_resolve_value(in_engine, in_variant1);
    _xte_resolve_value(in_engine, in_variant2);
    
    if (in_variant1->type == in_variant2->type) return XTE_TRUE;
    switch (in_variant1->type)
    {
        case XTE_TYPE_STRING:
        {
            switch (in_variant2->type)
            {
                case XTE_TYPE_INTEGER:
                    return xte_variant_convert(in_engine, in_variant1, XTE_TYPE_INTEGER);
                case XTE_TYPE_REAL:
                    return xte_variant_convert(in_engine, in_variant1, XTE_TYPE_REAL);
                case XTE_TYPE_BOOLEAN:
                    return xte_variant_convert(in_engine, in_variant2, XTE_TYPE_STRING);
                default: break;
            }
        }
        case XTE_TYPE_BOOLEAN:
        {
            switch (in_variant2->type)
            {
                case XTE_TYPE_STRING:
                    return xte_variant_convert(in_engine, in_variant1, XTE_TYPE_STRING);
                default: break;
            }
        }
        case XTE_TYPE_INTEGER:
        {
            switch (in_variant2->type)
            {
                case XTE_TYPE_STRING:
                    return xte_variant_convert(in_engine, in_variant2, XTE_TYPE_INTEGER);
                case XTE_TYPE_REAL:
                    return xte_variant_convert(in_engine, in_variant1, XTE_TYPE_REAL);
                default: break;
            }
        }
        case XTE_TYPE_REAL:
        {
            switch (in_variant2->type)
            {
                case XTE_TYPE_INTEGER:
                    return xte_variant_convert(in_engine, in_variant2, XTE_TYPE_REAL);
                case XTE_TYPE_STRING:
                    return xte_variant_convert(in_engine, in_variant2, XTE_TYPE_REAL);
                default: break;
            }
        }
        default: break;
    }
    return XTE_FALSE;
}


int _xte_compare_variants(XTE *in_engine, XTEVariant *in_variant1, XTEVariant *in_variant2)
{
    if (!_xte_make_variants_comparable(in_engine, in_variant1, in_variant2)) return -1;
    switch (in_variant1->type)
    {
        case XTE_TYPE_BOOLEAN:
            if (in_variant1->value.boolean == in_variant2->value.boolean) return 0;
            return -1;
        case XTE_TYPE_INTEGER:
        {
            int diff = in_variant1->value.integer - in_variant2->value.integer;
            if (diff == 0) return 0;
            else if (diff > 0) return 1;
            else return -1;
        }
        case XTE_TYPE_REAL:
        {
            double diff = in_variant1->value.real - in_variant2->value.real;
            if (abs(diff) < REAL_THRESHOLD) return 0;
            else if (diff > 0) return 1;
            else return -1;
        }
        case XTE_TYPE_STRING:
            return _xte_utf8_compare(in_variant1->value.utf8_string, in_variant2->value.utf8_string);
        default: break;
    }
    return -1;
}


/* public API must copy variants before comparison can be made;
 the internal function coerces types and we don't wish to mutate the types involved *****TODO****/
int xte_compare_variants(XTE *in_engine, XTEVariant *in_variant1, XTEVariant *in_variant2)
{
    XTEVariant *comp1, *comp2;
    comp1 = xte_variant_copy(in_engine, in_variant1);
    comp2 = xte_variant_copy(in_engine, in_variant2);
    int result = _xte_compare_variants(in_engine, comp1, comp2);
    xte_variant_release(comp1);
    xte_variant_release(comp2);
    return result;
}


// ! must continue to work even for things like GLOBAL - ie. but only return the actual name;
// not a deep resolution of the actual value - this function operates on whatever the variant is
// currently, without smarts
const char* xte_variant_as_cstring(XTEVariant *in_variant)
{
    if (!in_variant) return "";
    if (!in_variant->value.utf8_string) return "";
    return in_variant->value.utf8_string;
}


double xte_variant_as_double(XTEVariant *in_variant)
{
    if (!in_variant) return 0.0;
    else if (in_variant->type == XTE_TYPE_INTEGER) return in_variant->value.integer;
    else return in_variant->value.real;
}


int xte_variant_as_int(XTEVariant *in_variant)
{
    if (!in_variant) return 0;
    else if (in_variant->type == XTE_TYPE_INTEGER) return in_variant->value.integer;
    else if (in_variant->type == XTE_TYPE_REAL) return in_variant->value.real;
    else if (in_variant->type == XTE_TYPE_STRING) return (int)atol(in_variant->value.utf8_string);
    else if (in_variant->type == XTE_TYPE_GLOBAL) return (int)atol(in_variant->value.utf8_string);
    else if (in_variant->type == XTE_TYPE_BOOLEAN) return in_variant->value.boolean;
    else return 0;
}


void* xte_variant_ref_ident(XTEVariant *in_variant)
{
    return in_variant->value.ref.ident;
}


const char* xte_variant_ref_type(XTEVariant *in_variant)
{
    if (!in_variant) return "";
    if (in_variant->type == XTE_TYPE_OBJECT)
        return in_variant->value.ref.type->name;
    return "";
}


int xte_variant_is_container(XTEVariant *in_variant)
{
    if (!in_variant) return XTE_FALSE;
    if (in_variant->type == XTE_TYPE_GLOBAL) return XTE_TRUE;
    if (in_variant->type == XTE_TYPE_LOCAL) return XTE_TRUE;
    if (in_variant->type == XTE_TYPE_OBJECT)
    {
        if (in_variant->value.ref.type->container_read)
            return XTE_TRUE;
    }
    return XTE_FALSE;
}


void xte_debug_mutate_variable(XTE *in_engine, char const *in_name, int in_context, char *in_value)
{
    assert(IS_XTE(in_engine));
    assert(in_name != NULL);
    assert(in_value != NULL);
    
    struct XTEVariable *var = _xte_variable_access(in_engine, in_name, XTE_FALSE);
    
    if (!var) return;
    
    xte_variant_release(var->value);
    var->value = xte_string_create_with_cstring(in_engine, in_value);
    
    _xte_report_variable_mutation(in_engine, var);
}


void xte_debug_enumerate_variables(XTE *in_engine, int in_context)
{
    assert(IS_XTE(in_engine));
    
    if (in_context == XTE_CONTEXT_CURRENT)
    {
        in_context = in_engine->handler_stack_ptr;
    }
    if (in_context < 0)
    {
        /* enumerate all globals */
        for (int v = 0; v < in_engine->global_count; v++)
        {
            struct XTEVariable *var = in_engine->globals[v];
            if (!var) continue;
            _xte_report_variable_mutation(in_engine, var);
        }
    }
    else
    {
        /* enumerate locals for the specified handler */
        if (in_context > in_engine->handler_stack_ptr) return;
        struct XTEHandlerFrame *frame = in_engine->handler_stack + in_context;
        for (int v = 0; v < frame->imported_global_count; v++)
        {
            struct XTEVariable *var = frame->imported_globals[v];
            if (!var) continue;
            _xte_report_variable_mutation(in_engine, var);
        }
        for (int v = 0; v < frame->local_count; v++)
        {
            struct XTEVariable *var = frame->locals + v;
            if (!var) continue;
            _xte_report_variable_mutation(in_engine, var);
        }
    }
}


void xte_set_global(XTE *in_engine, char const *in_name, XTEVariant *in_value)
{
    struct XTEVariable *global = _xte_global_lookup(in_engine, in_name);
    if (!global) global = _xte_define_global(in_engine, in_name);
    
    assert(global != NULL);
    
    xte_variant_release(global->value);
    global->value = xte_variant_copy(in_engine, in_value);
    
    _xte_report_variable_mutation(in_engine, global);
}


