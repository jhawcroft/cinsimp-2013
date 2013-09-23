/*
 
 xTalk Engine Reference Parsing Unit
 xtalk_refs.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Responsible for tagging and parsing object references, and object property expressions
 
 */

/* 
 HyperTalk 2.2, pg 103 includes an interesting dicussion of traditional HTs interpretation
 of object reference expressions, and I think we might want to change this behaviour,
 if we haven't already implicitly done so... 
 */

#include "xtalk_internal.h"



/*********
 Utility
 */

/* determine if the node is an ordinal or special numeric constant,
 eg. "first", "second", "any", "middle", "next", etc. */
static int _xte_node_is_ordinal(XTEAST *in_node, XTEOrdinal *out_ordinal)
{
    if (!in_node) return XTE_FALSE;
    if (in_node->type != XTE_AST_WORD) return XTE_FALSE;
    
    if (_xte_compare_cstr(in_node->value.string, "first") == 0)
    {
        if (out_ordinal) *out_ordinal = XTE_ORD_FIRST;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "middle") == 0)
    {
        if (out_ordinal) *out_ordinal = XTE_ORD_MIDDLE;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "last") == 0)
    {
        if (out_ordinal) *out_ordinal = XTE_ORD_LAST;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "any") == 0)
    {
        if (out_ordinal) *out_ordinal = XTE_ORD_RANDOM;
        return XTE_TRUE;
    }
   /* if (_xte_compare_cstr(in_node->value.string, "this") == 0)
    {
        if (out_ordinal) *out_ordinal = XTE_ORD_THIS;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "next") == 0)
    {
        if (out_ordinal) *out_ordinal = XTE_ORD_NEXT;
        return XTE_TRUE;
    }
    if ( (_xte_compare_cstr(in_node->value.string, "previous") == 0) ||
        (_xte_compare_cstr(in_node->value.string, "prev") == 0) )
    {
        if (out_ordinal) *out_ordinal = XTE_ORD_PREV;
        return XTE_TRUE;
    }*/
    
    if (_xte_compare_cstr(in_node->value.string, "second") == 0)
    {
        if (out_ordinal) *out_ordinal = 2;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "third") == 0)
    {
        if (out_ordinal) *out_ordinal = 3;
        return XTE_TRUE;
    }
    if ((_xte_compare_cstr(in_node->value.string, "forth") == 0) ||
        (_xte_compare_cstr(in_node->value.string, "fourth") == 0))
    {
        if (out_ordinal) *out_ordinal = 4;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "fifth") == 0)
    {
        if (out_ordinal) *out_ordinal = 5;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "sixth") == 0)
    {
        if (out_ordinal) *out_ordinal = 6;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "seventh") == 0)
    {
        if (out_ordinal) *out_ordinal = 7;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "eighth") == 0)
    {
        if (out_ordinal) *out_ordinal = 8;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "ninth") == 0)
    {
        if (out_ordinal) *out_ordinal = 9;
        return XTE_TRUE;
    }
    if (_xte_compare_cstr(in_node->value.string, "tenth") == 0)
    {
        if (out_ordinal) *out_ordinal = 10;
        return XTE_TRUE;
    }
    
    return XTE_FALSE;
}




/*********
 Parsing
 */


/** TODO **
 
 LOW PRIORITY
 
  should be possible to verify at runtime in interpreter whether the object
 which an element belongs (owner) has such elements, and pose an appropriate error if it doesn't
 instead of leaving this to the environment implementation functions
 
 shouldn't be putting function handlers in during parsing until we know what the type is,
 a bit like property handling...
 */

/* flag an object reference, lookup the appropriate function pointer(s) and begin translation */
static void _xte_convert_ref(XTE *in_engine, XTEAST *in_stmt, int in_offset, struct XTERefInt *in_ref)
{
    /* remove all keywords that form reference definition */
    for (int i = 0; i < in_ref->word_count; i++)
        _xte_ast_destroy( _xte_ast_list_remove(in_stmt, in_offset) );
    
    /* create reference node */
    XTEAST *ref = _xte_ast_create(in_engine, XTE_AST_REF);
    
    
    /* check for preceeding ordinal */
    XTEElementGetter uid_type = NULL;
    int uid_is_string = XTE_FALSE;
    if (!_xte_node_is_ordinal(_xte_ast_list_child(in_stmt, in_offset - 1), NULL))
    {
        /* look for UID in statement */
        XTEAST *uid_word = _xte_ast_list_child(in_stmt, in_offset);
        if ((uid_word) && (uid_word->type == XTE_AST_WORD))
        {
            for (int i = 0; i < in_ref->uid_alt_count; i++)
            {
                if (_xte_compare_cstr(uid_word->value.string, in_ref->uid_alt[i].uid_name) == 0)
                {
                    _xte_ast_destroy( _xte_ast_list_remove(in_stmt, in_offset) );

                    uid_type = in_ref->uid_alt[i].get;
                    if (strcmp(in_ref->uid_alt[i].uid_type, "string") == 0) uid_is_string = XTE_TRUE;
                    break;
                }
            }
        }
    }
    else
    {
        uid_type = in_ref->get_range;
        ref->value.ref.params_are_indicies = XTE_TRUE;
    }
    
    // was previously creating ref here  ******
    ref->value.ref.type = in_ref->type;
    ref->value.ref.get_count = in_ref->get_count;
    
    char *buffer = malloc(strlen(in_ref->name) + 20);
    if (!buffer) return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
    if (in_ref->singular)
        sprintf(buffer, "element(s) of \"%s\"", in_ref->name);
    else
        sprintf(buffer, "\"%s\" collection", in_ref->name);
    ref->note = _xte_clone_cstr(in_engine, buffer);
    free(buffer);
    
    // if there's not UID specified, then insert two pointers: one for strings and one for numbers
    // otherwise, insert the single matching UID pointer
    // decision will be made a runtime based on evaluation of the indentity expression and it's type **TODO**
    
    /* insert UID type into reference node */
    if (in_ref->singular)
    {
        if (!uid_type)
        {
            ref->value.ref.get_by_number = in_ref->get_range;
            ref->value.ref.get_by_string = in_ref->get_named;
        }
        else
        {
            if (uid_is_string) ref->value.ref.get_by_string = uid_type;
            else ref->value.ref.get_by_number = uid_type;
        }
    }
    else
    {
        ref->value.ref.get_by_number = in_ref->get_range;
        ref->value.ref.params_are_indicies = XTE_TRUE;
        ref->value.ref.is_collection = XTE_TRUE;
    }
    
    /* insert reference node */
    _xte_ast_list_insert(in_stmt, in_offset, ref);
}


/* find object references;
 all references whether global or class scope must be tagged and processed;
 actual validation of types is dynamically resolved at runtime as the language
 does not guarantee the type of any given object expression. */
void _xte_parse_refs(XTE *in_engine, XTEAST *in_stmt)
{
    /* iterate forwards through statement words */
    for (int i = 0; i < in_stmt->children_count; i++)
    {
        XTEAST *child = in_stmt->children[i];
        if (child->type == XTE_AST_WORD)
        {
            /* iterate through reference clauses */
            for (int j = in_engine->ref_count-1; j >= 0; j--)
            {
                /* compare reference clause against statement */
                struct XTERefInt *ref = in_engine->refs + j;
                if (i + ref->word_count < in_stmt->children_count + 1)
                {
                    int w;
                    for (w = 0; w < ref->word_count; w++)
                    {
                        XTEAST *word = in_stmt->children[i + w];
                        if ((word->type != XTE_AST_WORD) || (_xte_compare_cstr(ref->words[w], word->value.string) != 0))
                            break;
                    }
                    if (ref->word_count == w)
                    {
                        /* found matching reference clause; translate it */
                        _xte_convert_ref(in_engine, in_stmt, i, ref);
                        break;
                    }
                }
            }
        }
    }
}


/* coalesces ordinals, identifiers and range parameters into existing tagged reference nodes
 within the AST;
 run later in the parsing process, after additional parsing has been completed. */
static void _xte_coalesce_ref_param(XTE *in_engine, XTEAST *in_stmt, int i)
{
    XTEAST *ref = in_stmt->children[i];
    if (ref->type != XTE_AST_REF) return;
    
    /* flag the reference so we don't try to process it again */
    ref->flags |= XTE_AST_FLAG_FINALIZED;
    
    /* don't look for params for collection references */
    if (ref->value.ref.is_collection) return;
    
    /* look for preceeding ordinals */
    XTEOrdinal ordinal_value;
    if (_xte_node_is_ordinal(_xte_ast_list_child(in_stmt, i - 1), &ordinal_value))
    {
        /* append the ordinal value */
        //ref->value.integer = ordinal_value;
        _xte_ast_list_append(ref, _xte_ast_create_literal_integer(in_engine, ordinal_value));
        ref->flags |= XTE_AST_FLAG_ORDINAL;
        _xte_ast_destroy(_xte_ast_list_remove(in_stmt, i - 1));
        i--;
        
        /* look for optional preceeding "the" */
        XTEAST *the = _xte_ast_list_child(in_stmt, i - 1);
        if ((the) && (the->type == XTE_AST_WORD) && (_xte_compare_cstr(the->value.string, "the") == 0))
        {
            _xte_ast_destroy(_xte_ast_list_remove(in_stmt, i - 1));
            i--;
        }
    }
    else if (!(ref->flags & XTE_AST_FLAG_ORDINAL))
    {
        /* append following parameter */
        _xte_ast_list_append(ref, _xte_ast_list_remove(in_stmt, i+1));
        
        /* look for range */
        if (ref->children[0])// might be a bad check - probably supposed to be looking for at least one param
            // not including function pointers - write a subroutine? *******
        {
            XTEAST *to = _xte_ast_list_child(in_stmt, i+1);
            if ((to) && (to->type == XTE_AST_WORD) && (_xte_compare_cstr(to->value.string, "to") == 0))
            {
                _xte_ast_destroy(_xte_ast_list_remove(in_stmt, i+1));
                
                /* append the following parameter */
                ref->value.ref.is_range = XTE_TRUE;
                ref->value.ref.get_by_string = NULL;
                _xte_ast_list_append(ref, _xte_ast_list_remove(in_stmt, i+1));
            }
        }
    }
}



/* prototypes of functions in related modules: */
int _xte_is_unary_op(XTEAST *in_op);



/*
 *  _xte_coalesce_refs
 *  ---------------------------------------------------------------------------------------------
 *  Completes object references by coalescing relevant parameters from the token stream and
 *  combining references into a heirarchy when joined by "of" or "in".
 *
 *  ! Mutates the input token stream.
 *
 *  The result is XTE_TRUE if the parse was successful (no syntax error.)
 *  Syntax errors are handled as per protocol.
 */
int _xte_coalesce_refs(XTE *in_engine, XTEAST *in_stmt, long in_source_line)
{
    assert(IS_XTE(in_engine));
    assert(in_stmt != NULL);
    
    /* coalesce reference parameters */
    for (int i = in_stmt->children_count-1; i >= 0; i--)
    {
        XTEAST *keyword = in_stmt->children[i];
        if ( (keyword->type == XTE_AST_REF) && (!(keyword->flags & XTE_AST_FLAG_FINALIZED)) )
        {
            _xte_coalesce_ref_param(in_engine, in_stmt, i);
            i = in_stmt->children_count-1;
        }
    }
    //_xte_ast_debug(in_stmt);
    
    /* coalesce object references, properties, constants and functions with their owner/operands;
     ie. anything separated by "OF" / "IN" is grouped to form a heirarchical structure suitable
     for execution by the interpreter */
    for (int i = in_stmt->children_count-1; i >= 0; i--)
    {
        XTEAST *keyword = in_stmt->children[i];
        if (keyword && ((keyword->type == XTE_AST_OF) || (keyword->type == XTE_AST_IN)))
        {
            XTEAST *prev_node = _xte_ast_list_child(in_stmt, i-1);
            if ( prev_node && ((prev_node->type == XTE_AST_REF) ||
                              (prev_node->type == XTE_AST_PROPERTY) ||
                               (prev_node->type == XTE_AST_FUNCTION) ||
                               (prev_node->type == XTE_AST_CONSTANT)) && i+1 < in_stmt->children_count )
            {
                XTEAST *next, *next_plus_1;
                next = _xte_ast_list_child(in_stmt, i+1);
                next_plus_1 = _xte_ast_list_child(in_stmt, i+2);
                if (_xte_is_unary_op(next))
                {
                    _xte_ast_list_append(prev_node, _xte_ast_list_remove(in_stmt, i+1));
                    _xte_ast_list_append(next, _xte_ast_list_remove(in_stmt, i+1));
                }
                else
                {
                    _xte_ast_list_append(prev_node, _xte_ast_list_remove(in_stmt, i+1));
                }
                _xte_ast_destroy(_xte_ast_list_remove(in_stmt, i));
            }
            else
            {
                ERROR_SYNTAX(in_source_line, "Expected object, property or chunk expression.", NULL, NULL, NULL);
                return XTE_FALSE;
            }
        }
    }
    return XTE_TRUE;
}




/*********
 Dictionary
 */


/* references must be tabled in order of word count so that multiple word references match
 before single word references, eg.
 "card field" has a different meaning to "card";
 this function returns an appropriate index for the next reference to be inserted into the catalogue */
static int _xte_ref_insertion_index(XTE *in_engine, int in_word_count)
{
    for (int i = 0; i < in_engine->ref_count; i++)
    {
        if (in_engine->refs[i].word_count >= in_word_count)
            return i;
    }
    return in_engine->ref_count;
}



/* builds an internal list of unique identifiers for a specific collection of objects;
 invoked by _xte_build_refs_table() */
static void _xte_build_uids_list(XTE *in_engine, struct XTERefIntUID **out_uids, int *out_uid_count, char *in_class_name)
{
    /* lookup the class */
    struct XTEClassInt *class = _xte_class_for_name(in_engine, in_class_name);
    
    /* count the number of UID properties of the class */
    *out_uid_count = 0;
    for (int i = 0; i < class->property_count; i++)
        if (class->properties[i] && class->properties[i]->is_unique_id) (*out_uid_count)++;
    
    /* allocate list large enough to hold UIDs */
    struct XTERefIntUID *uids = *out_uids = calloc(sizeof(struct XTERefIntUID), *out_uid_count);
    if (!uids)
    {
        *out_uid_count = 0;
        return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
    }
    int uid_index = 0;
    for (int i = 0; i < class->property_count; i++)
    {
        if (class->properties[i] && class->properties[i]->is_unique_id)
        {
            struct XTERefIntUID *uid = &(uids[uid_index++]);
            uid->get = class->properties[i]->owner_inst_for_id;
            uid->uid_name = _xte_clone_cstr(in_engine, class->properties[i]->name);
            uid->uid_type = _xte_clone_cstr(in_engine, class->properties[i]->usual_type);
        }
    }
}


static struct XTERefInt* _xte_ref_add(XTE *in_engine, int in_word_count)
{
    /* increase the table size */
    struct XTERefInt *new_table = realloc(in_engine->refs, sizeof(struct XTERefInt) * (in_engine->ref_count + 1));
    if (!new_table) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    in_engine->refs = new_table;
    
    /* find the most appropriate offset to insert the reference */
    int insertion_index = _xte_ref_insertion_index(in_engine, in_word_count);
    
    /* insert and return an empty reference */
    memmove(in_engine->refs + insertion_index + 1, in_engine->refs + insertion_index,
            sizeof(struct XTERefInt) * (in_engine->ref_count - insertion_index));
    in_engine->ref_count++;
    struct XTERefInt *result = in_engine->refs + insertion_index;
    memset(result, 0, sizeof(struct XTERefInt));
    return result;
}


/* adds a single element to the internal reference catalogue;
 this function will be invoked from this module and from the class module */
void _xte_element_add(XTE *in_engine, struct XTEElementDef *in_def, struct XTEClassInt *in_class)
{
    /* compute the words for the singular form of the element */
    char **words;
    int word_count;
    _xte_itemize_cstr(in_engine, in_def->singular, " ", &(words), &(word_count));
    
    /* add a reference and configure */
    struct XTERefInt *ref = _xte_ref_add(in_engine, word_count);
    if (!ref) return;
    ref->singular = XTE_TRUE;
    ref->name = _xte_clone_cstr(in_engine, in_def->name);
    ref->type = _xte_clone_cstr(in_engine, in_def->type);
    ref->word_count = word_count;
    ref->words = words;
    ref->get_range = in_def->get_range;
    ref->get_named = in_def->get_named;
    ref->get_count = in_def->get_count;
    _xte_build_uids_list(in_engine, &(ref->uid_alt), &(ref->uid_alt_count), in_def->type);
    
    /* compute the words for the plural form of the element */
    _xte_itemize_cstr(in_engine, in_def->name, " ", &(words), &(word_count));

    /* add a reference and configure */
    ref = _xte_ref_add(in_engine, word_count);
    if (!ref) return;
    ref->singular = XTE_FALSE;
    ref->name = _xte_clone_cstr(in_engine, in_def->name);
    ref->word_count = word_count;
    ref->words = words;
    ref->get_range = in_def->get_range;
}


/* add one or more elements with one call */
void _xte_elements_add(XTE *in_engine, struct XTEElementDef *in_defs)
{
    if (!in_defs) return;
    struct XTEElementDef *the_elements = in_defs;
    while (the_elements->name)
    {
        _xte_element_add(in_engine, the_elements, NULL);
        the_elements++;
    }
}




/*********
 Built-ins
 */

extern struct XTEElementDef _xte_builtin_chunk_elements[];

/* add built-in language elements */
void _xte_elements_add_builtins(XTE *in_engine)
{
    _xte_elements_add(in_engine, _xte_builtin_chunk_elements);
}








