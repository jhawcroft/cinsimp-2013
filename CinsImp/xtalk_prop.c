/*
 
 xTalk Engine Property Parsing Unit
 xtalk_prop.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Responsible for parsing property identifiers
 
 */

#include "xtalk_internal.h"



/*********
 Utilities
 */

/* checks if the node is a legal property representation, ie. "short" | "abbreviated" | "long";
 if it is, it returns an enumeration constant with an appropriate value. 
 this constant will be passed to the callback function when requesting the property value. */
static int _xte_node_is_prop_rep(XTEAST *in_node, XTEPropRep *out_rep)
{
    if (!in_node) return XTE_FALSE;
    if (in_node->type != XTE_AST_WORD) return XTE_FALSE;
    
    if (_xte_compare_cstr(in_node->value.string, "short") == 0)
    {
        if (out_rep) *out_rep = XTE_PROPREP_SHORT;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "abbr") == 0)
    {
        if (out_rep) *out_rep = XTE_PROPREP_ABBREVIATED;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "abbrev") == 0)
    {
        if (out_rep) *out_rep = XTE_PROPREP_ABBREVIATED;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "abbreviated") == 0)
    {
        if (out_rep) *out_rep = XTE_PROPREP_ABBREVIATED;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "long") == 0)
    {
        if (out_rep) *out_rep = XTE_PROPREP_LONG;
        return XTE_TRUE;
    }
    
    return XTE_FALSE;
}


/* searches immediately prior to the current offset within the expression for a known property name */
static struct XTEPropertyInt* _xte_prop_lookup_backward(XTE *in_engine, XTEAST *in_stmt, int in_offset)
{
    if (in_offset < 0) return NULL;
    
    /* search global table first */
    for (int i = 0; i < in_engine->global_prop_count; i++)
    {
        /* ignore properties whose names are too long */
        if (in_offset + 1 - in_engine->global_props[i]->word_count < 1) continue;
        struct XTEPropertyInt *prop = in_engine->global_props[i];
        
        /* match words of property */
        int w = 0;
        for (int o = in_offset + 1 - prop->word_count; o <= in_offset; o++, w++)
        {
            if ((in_stmt->children[o]->type != XTE_AST_WORD) ||
                (_xte_compare_cstr(in_stmt->children[o]->value.string, prop->words[w]) != 0)) break;
        }
        if (w == prop->word_count)
        {
            /* found a matching property */
            return prop;
        }
    }
    
    /* search class properties table */
    for (int i = 0; i < in_engine->class_prop_count; i++)
    {
        /* ignore properties whose names are too long */
        if (in_offset + 1 - in_engine->class_props[i]->word_count < 0) continue;
        struct XTEPropertyInt *prop = in_engine->class_props[i];
        
        /* match words of property */
        int w = 0;
        for (int o = in_offset + 1 - prop->word_count; o <= in_offset; o++, w++)
        {
            if ((in_stmt->children[o]->type != XTE_AST_WORD) ||
                (_xte_compare_cstr(in_stmt->children[o]->value.string, prop->words[w]) != 0)) break;
        }
        if (w == prop->word_count)
        {
            /* found a matching property */
            return prop;
        }
    }
    
    
    return NULL;
}


/* searches immediately following the current offset within the expression for a known property name */
static struct XTEPropertyInt* _xte_prop_lookup_forward(XTE *in_engine, XTEAST *in_stmt, int in_offset)
{
    if (in_offset >= in_stmt->children_count) return NULL;
    
    /* search global table first */
    for (int i = 0; i < in_engine->global_prop_count; i++)
    {
        /* ignore properties whose names are too long */
        if (in_offset + in_engine->global_props[i]->word_count > in_stmt->children_count) continue;
        struct XTEPropertyInt *prop = in_engine->global_props[i];
        
        /* match words of property */
        int w = 0;
        for (; w < prop->word_count; w++)
        {
            if ((in_stmt->children[in_offset + w]->type != XTE_AST_WORD) ||
                (_xte_compare_cstr(in_stmt->children[in_offset + w]->value.string, prop->words[w]) != 0)) break;
        }
        if (w == prop->word_count)
        {
            /* found a matching property */
            return prop;
        }
    }
    
    /* search class properties table */
    for (int i = 0; i < in_engine->class_prop_count; i++)
    {
        /* ignore properties whose names are too long */
        if (in_offset + in_engine->class_props[i]->word_count > in_stmt->children_count) continue;
        struct XTEPropertyInt *prop = in_engine->class_props[i];
        
        /* match words of property */
        int w = 0;
        for (; w < prop->word_count; w++)
        {
            if ((in_stmt->children[in_offset + w]->type != XTE_AST_WORD) ||
                (_xte_compare_cstr(in_stmt->children[in_offset + w]->value.string, prop->words[w]) != 0)) break;
        }
        if (w == prop->word_count)
        {
            /* found a matching property */
            return prop;
        }
    }
    
    
    return NULL;
}



/*********
 Parsing
 */

/* search for "of" and then check backwards for a valid property name;
 search for "the" and then check forwards for a valid property name
 need to check for "short", "abbreviated", "long" property representations */

/*
 *  _xte_parse_properties
 *  ---------------------------------------------------------------------------------------------
 *  Looks for known property names either suffixed by "of" or prefixed by "the".
 *
 *  ! Mutates the input token stream.
 */
void _xte_parse_properties(XTE *in_engine, XTEAST *in_stmt)
{
    /* iterate through statement words */
    for (int i = 0; i < in_stmt->children_count; i++)
    {
        XTEAST *child = in_stmt->children[i];
        if (child->type == XTE_AST_OF)
        {
            /* check backwards for any property name */
            struct XTEPropertyInt *prop = _xte_prop_lookup_backward(in_engine, in_stmt, i-1);
            
            /* if there's a property name, process the property. 
             otherwise skip it and continue */
            if (prop)
            {
                /* check backwards for representation offset */
                XTEPropRep rep = XTE_PROPREP_NORMAL;
                int rep_offset = _xte_node_is_prop_rep(_xte_ast_list_child(in_stmt, i-prop->word_count-1), &rep);
                
                /* remove existing nodes */
                i -= prop->word_count + rep_offset;
                if (_xte_node_is_the(_xte_ast_list_child(in_stmt, i-1)))
                    _xte_ast_destroy(_xte_ast_list_remove(in_stmt, --i));
                for (int w = 0; w < prop->word_count; w++)
                    _xte_ast_destroy(_xte_ast_list_remove(in_stmt, i));
                if (rep_offset)
                    _xte_ast_destroy(_xte_ast_list_remove(in_stmt, i));
                
                /* insert property node */
                XTEAST *prop_node = _xte_ast_create(in_engine, XTE_AST_PROPERTY);
                prop_node->value.property.pmap_entry = prop->mapped_ptr;
                prop_node->value.property.representation = rep;
                prop_node->note = _xte_clone_cstr(in_engine, prop->name);
                _xte_ast_list_insert(in_stmt, i, prop_node);
            }
        }
        else if ((child->type == XTE_AST_WORD) && (_xte_compare_cstr(child->value.string, "the") == 0))
        {
            /* check forwards for property representation;
             if there is one, we're guaranteed a property */
            XTEPropRep rep = XTE_PROPREP_NORMAL;
            int rep_offset = _xte_node_is_prop_rep(_xte_ast_list_child(in_stmt, i+1), &rep);
            
            /* check forwards for any property name */
            struct XTEPropertyInt *prop = _xte_prop_lookup_forward(in_engine, in_stmt, i+1+rep_offset);
            
            /* if there's a property representation, or a property name,
             process the property. otherwise skip it and continue */
            if (prop)
            {
                /* remove existing nodes */
                _xte_ast_destroy(_xte_ast_list_remove(in_stmt, i));
                if (rep_offset)
                    _xte_ast_destroy(_xte_ast_list_remove(in_stmt, i));
                for (int w = 0; w < prop->word_count; w++)
                    _xte_ast_destroy(_xte_ast_list_remove(in_stmt, i));
                
                /* insert property node */
                XTEAST *prop_node = _xte_ast_create(in_engine, XTE_AST_PROPERTY);
                prop_node->value.property.pmap_entry = prop->mapped_ptr;
                prop_node->value.property.representation = rep;
                prop_node->note = _xte_clone_cstr(in_engine, prop->name);
                _xte_ast_list_insert(in_stmt, i, prop_node);
            }
        }
    }
}



/*********
 Dictionary
 */


/* returns a function pointer for the read implementation of the specified property
 for a specific object type,
 ie. the read function for the name property of a field might differ from the function
 for the name property of a card. */
XTEPropertyGetter _xte_property_getter(XTE *in_engine, int in_pmap_entry, struct XTEClassInt *in_class)
{
    struct XTEPropertyPtrs *entry = &(in_engine->prop_ptr_table[in_pmap_entry]);
    for (int i = 0; i < entry->ptr_count; i++)
    {
        if (entry->ptrs[i].owner == in_class)
            return entry->ptrs[i].func_read;
    }
    return NULL;
}


struct XTEPropertyPtr* _xte_property_ptrs(XTE *in_engine, int in_pmap_entry, struct XTEClassInt *in_class)
{
    struct XTEPropertyPtrs *entry = &(in_engine->prop_ptr_table[in_pmap_entry]);
    for (int i = 0; i < entry->ptr_count; i++)
    {
        if (entry->ptrs[i].owner == in_class)
            return &(entry->ptrs[i]);
    }
    return NULL;
}


/* used by the "set" command in _mutate.c to handle property names that are missing "the" prefix;
 special case for this built-in command */
struct XTEPropertyPtr* _xte_property_named(XTE *in_engine, const char *in_name)
{
    for (int i = 0; i < in_engine->prop_ptr_count; i++)
    {
        if (_xte_compare_cstr(in_engine->prop_ptr_table[i].name, in_name) == 0)
        {
            for (int e = 0; e < in_engine->prop_ptr_table[i].ptr_count; e++)
            {
                if (!in_engine->prop_ptr_table[i].ptrs[e].owner)
                    return &(in_engine->prop_ptr_table[i].ptrs[e]);
            }
            return NULL;
        }
    }
    return NULL;
}



/* records the function pointer for a property definition, for a specific class type (owner);
 returns an integer specific to the name of the property;
 if two properties of different class types share the same name, eg. "id" or "name",
 they will obtain the same map integer result.
 this integer can later (at runtime) be used by calling _xte_property_getter() to obtain 
 the correct function pointer for the specific type of object being accessed. */
static int _xte_map_property_pointer(XTE *in_engine, struct XTEPropertyDef *in_def, void *in_owner)
{
    int table_index = -1;
    
    /* see if we can find an existing property entry for the given property name */
    for (int ti = 0; ti < in_engine->prop_ptr_count; ti++)
    {
        struct XTEPropertyPtrs *ptrs = &(in_engine->prop_ptr_table[ti]);
        if (strcmp(ptrs->name, in_def->name) == 0)
        {
            /* found an existing entry, append this property pointers to that entry */
            table_index = ti;
            break;
        }
    }
    
    if (table_index < 0)
    {
        /* add a new property entry */
        struct XTEPropertyPtrs *new_table = realloc(in_engine->prop_ptr_table, sizeof(struct XTEPropertyPtrs) * (in_engine->prop_ptr_count + 1));
        if (!new_table) return _xte_panic_int(in_engine, XTE_ERROR_MEMORY, NULL);
        in_engine->prop_ptr_table = new_table;
        table_index = in_engine->prop_ptr_count++;
        
        struct XTEPropertyPtrs *new_entry = &(in_engine->prop_ptr_table[table_index]);
        new_entry->ptr_count = 0;
        new_entry->ptrs = NULL;
        new_entry->name = _xte_clone_cstr(in_engine, in_def->name);
    }
    
    /* add the property definition to the pointers list for the table entry */
    if (table_index < 0) return _xte_panic_int(in_engine, XTE_ERROR_INTERNAL, NULL);
    struct XTEPropertyPtrs *table_entry = &(in_engine->prop_ptr_table[table_index]);
    
    struct XTEPropertyPtr *new_ptrs = realloc(table_entry->ptrs, sizeof(struct XTEPropertyPtr) * (table_entry->ptr_count + 1));
    if (!new_ptrs) return _xte_panic_int(in_engine, XTE_ERROR_MEMORY, NULL);
    table_entry->ptrs = new_ptrs;
    
    struct XTEPropertyPtr *ptr_entry = &(new_ptrs[table_entry->ptr_count++]);
    
    ptr_entry->func_read = in_def->func_get;
    ptr_entry->func_write = in_def->func_set;
    ptr_entry->owner = in_owner;
    ptr_entry->env_id = in_def->env_id;
    
    return table_index;
}


/* adds a single property to the internal property catalogue;
 this function will be invoked from this module and from the class module */
struct XTEPropertyInt* _xte_property_add(XTE *in_engine, struct XTEPropertyDef *in_def, struct XTEClassInt *in_class)
{
    if (!in_def->name) return NULL;
    
    /* get pointers to the appropriate table
     (class or global) */
    struct XTEPropertyInt ***the_table;
    int *the_table_size;
    if (in_class)
    {
        the_table = &in_engine->class_props;
        the_table_size = &in_engine->class_prop_count;
    }
    else
    {
        the_table = &in_engine->global_props;
        the_table_size = &in_engine->global_prop_count;
    }
    
    /* increase the table size */
    struct XTEPropertyInt **new_table = realloc(*the_table, sizeof(struct XTEPropertyInt*) * (*the_table_size + 1));
    if (!new_table) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    *the_table = new_table;
    struct XTEPropertyInt *the_prop = new_table[(*the_table_size)++] = calloc(sizeof(struct XTEPropertyInt), 1);
    if (!the_prop) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    
    /* define the property */
    the_prop->name = _xte_clone_cstr(in_engine, in_def->name);
    _xte_itemize_cstr(in_engine, in_def->name, " ", &(the_prop->words), &(the_prop->word_count));
    the_prop->mapped_ptr = _xte_map_property_pointer(in_engine, in_def, in_class);
    the_prop->is_unique_id = in_def->flags & XTE_PROPERTY_UID;
    the_prop->usual_type = _xte_clone_cstr(in_engine, in_def->type);
    the_prop->owner_inst_for_id = in_def->func_element_by_prop;
    return the_prop;
}


/* add one or more properties with one call */
void _xte_properties_add(XTE *in_engine, struct XTEPropertyDef *in_defs)
{
    if (!in_defs) return;
    struct XTEPropertyDef *the_props = in_defs;
    while (the_props->name)
    {
        _xte_property_add(in_engine, the_props, NULL);
        the_props++;
    }
}



/*********
 Built-ins
 */

static XTEVariant* bi_system(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_object_ref(in_engine, "system", NULL, NULL);
}

/* returns the original receiver object of the current message;
 See also: "target" constant, which should provide the value of the target, not the target object,
 also, object references need to be able to be converted to a string possibly via
 a new callback which we can investigate late in testing */
static XTEVariant* bi_target(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    xte_variant_retain(in_engine->the_target);
    return in_engine->the_target;
}


XTEVariant* bi_the_result(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    if (!in_engine->the_result) _xte_set_result(in_engine, xte_string_create_with_cstring(in_engine, ""));
    //if ((in_engine->the_result && (in_engine->the_result->type == XTE_TYPE_PROP) &&
    //    (in_engine->the_result->value.prop.ptrs[0].func_read == &bi_the_result))
        
    return xte_variant_value(in_engine, in_engine->the_result);
}


static XTEVariant* bi_item_delimiter_get(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, in_engine->item_delimiter);
}

static void bi_item_delimiter_set(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEVariant *in_new_value)
{
    if (!xte_variant_convert(in_engine, in_new_value, XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
        return;
    }
    if (in_engine->item_delimiter) free(in_engine->item_delimiter);
    in_engine->item_delimiter = _xte_clone_cstr(in_engine, xte_variant_as_cstring(in_new_value));
}


static XTEVariant* bi_number_fmt_get(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, in_engine->number_format);
}

static void bi_number_fmt_set(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEVariant *in_new_value)
{
    if (!xte_variant_convert(in_engine, in_new_value, XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, "Expected string here.", NULL, NULL, NULL);
        return;
    }
    if (in_engine->number_format) free(in_engine->number_format);
    in_engine->number_format = _xte_clone_cstr(in_engine, xte_variant_as_cstring(in_new_value));
}


static struct XTEPropertyDef bi_props[] = {
    {"system", 0, "system", 0, &bi_system, NULL},
    {"target", 0, "object", 0, &bi_target, NULL},
    {"result", 0, "value", 0, &bi_the_result, NULL},
    {"itemDelimiter", 0, "string", 0, &bi_item_delimiter_get, NULL, &bi_item_delimiter_set},
    {"numberFormat", 0, "string", 0, &bi_number_fmt_get, NULL, &bi_number_fmt_set},
    NULL,
};


extern struct XTEPropertyDef _xte_builtin_date_properties[];


/* add built-in language constants */
void _xte_properties_add_builtins(XTE *in_engine)
{
    _xte_properties_add(in_engine, bi_props);
    _xte_properties_add(in_engine, _xte_builtin_date_properties);
}








