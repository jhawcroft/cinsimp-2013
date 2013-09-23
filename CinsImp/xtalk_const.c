/*
 
 xTalk Engine Constant Parsing Unit
 xtalk_const.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Responsible for definition and parsing of constants
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"



/*********
 Parsing
 */


static void _xte_convert_constant(XTE *in_engine, XTEAST *in_stmt, int in_offset, struct XTEConstantInt *in_con)
{
    /* remove the existing terminology */
    for (int i = 0; i < in_con->word_count; i++)
        _xte_ast_destroy(_xte_ast_list_remove(in_stmt, in_offset));
    
    /* create a new constant node */
    XTEAST *const_node = _xte_ast_create(in_engine, XTE_AST_CONSTANT);
    const_node->value.ptr = in_con->func_get;
    const_node->note = _xte_clone_cstr(in_engine, in_con->name);
    
    /* insert the node into the statement */
    _xte_ast_list_insert(in_stmt, in_offset, const_node);
}


void _xte_parse_consts(XTE *in_engine, XTEAST *in_stmt)
{
    /* iterate backwards through statement words */
    for (int i = in_stmt->children_count - 1; i >= 0; i--)
    {
        XTEAST *child = in_stmt->children[i];
        if (child->type == XTE_AST_WORD)
        {
            /* iterate through constants */
            for (int j = in_engine->cons_count-1; j >= 0; j--)
            {
                /* compare constant against statement */
                struct XTEConstantInt *cons = &(in_engine->cons[j]);
                if (i + cons->word_count >= in_stmt->children_count + 1) continue;
                
                int w;
                for (w = 0; w < cons->word_count; w++)
                {
                    XTEAST *word = in_stmt->children[i + w];
                    if ((word->type != XTE_AST_WORD) ||
                        (_xte_compare_cstr(cons->words[w], word->value.string) != 0))
                        break;
                }
                if (cons->word_count == w)
                {
                    /* check preceeding word isn't the */
                    if (!_xte_node_is_the(_xte_ast_list_child(in_stmt, i-1)))
                    {
                        /* found matching constant; convert it */
                        _xte_convert_constant(in_engine, in_stmt, i, cons);
                        break;
                    }
                }
            }
        }
    }
}



/*********
 Dictionary
 */


/* adds a single constant to the internal constant catalogue */
static void _xte_constant_add(XTE *in_engine, struct XTEConstantDef *in_def)
{
    /* add an entry to the constant table */
    struct XTEConstantInt *new_table = realloc(in_engine->cons, sizeof(struct XTEConstantInt) * (in_engine->cons_count + 1));
    if (!new_table) return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
    in_engine->cons = new_table;
    struct XTEConstantInt *new_constant = &(new_table[in_engine->cons_count++]);
    
    /* configure the constant */
    new_constant->name = _xte_clone_cstr(in_engine, in_def->name);
    new_constant->func_get = in_def->func_get;
    _xte_itemize_cstr(in_engine, in_def->name, " ", &(new_constant->words), &(new_constant->word_count));
}


/* add one or more constants with one call */
void _xte_constants_add(XTE *in_engine, struct XTEConstantDef *in_defs)
{
    if (!in_defs) return;
    struct XTEConstantDef *the_cons = in_defs;
    while (the_cons->name)
    {
        _xte_constant_add(in_engine, the_cons);
        the_cons++;
    }
}



/*********
 Built-ins
 */

/* "me" is the object containing the currently executing handler, eg. a button, a stack or even the message box */
static XTEVariant* _bi_me(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    printf("get handler object\n");
    return NULL;// **TODO**
}

/* "target" is the resolved text contents of the container object to whom the message was originally sent;
 if the message was not originally sent to a button/field (first responder), an 
 "Expected button or field" runtime error should occur.
 See also: "the target" global property - the original message target object. */
static XTEVariant* _bi_target(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    printf("get target content\n");
    return NULL;// **TODO**
}

static XTEVariant* _bi_pi(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    if (in_owner) return NULL;
    return xte_real_create(in_engine, 3.14159265358979323846);
}

static XTEVariant* _bi_zero(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 0);
}

static XTEVariant* _bi_one(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 1);
}

static XTEVariant* _bi_two(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 2);
}

static XTEVariant* _bi_three(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 3);
}

static XTEVariant* _bi_four(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 4);
}

static XTEVariant* _bi_five(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 5);
}

static XTEVariant* _bi_six(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 6);
}

static XTEVariant* _bi_seven(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 7);
}

static XTEVariant* _bi_eight(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 8);
}

static XTEVariant* _bi_nine(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 9);
}

static XTEVariant* _bi_ten(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_integer_create(in_engine, 10);
}

static XTEVariant* _bi_true(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_boolean_create(in_engine, XTE_TRUE);
}

static XTEVariant* _bi_false(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_boolean_create(in_engine, XTE_FALSE);
}


static XTEVariant* _bi_empty(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, "");
}

static XTEVariant* _bi_space(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, " ");
}

static XTEVariant* _bi_colon(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, ":");
}

static XTEVariant* _bi_comma(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, ",");
}

static XTEVariant* _bi_tab(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, "\t");
}

static XTEVariant* _bi_quote(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, "\"");
}

static XTEVariant* _bi_newline(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, "\n");
}


static struct XTEConstantDef bi_consts[] = {
    {"me", _bi_me},
    {"target", _bi_target},
    
    {"pi", &_bi_pi},
    {"zero", &_bi_zero},
    {"one", &_bi_one},
    {"two", &_bi_two},
    {"three", &_bi_three},
    {"four", &_bi_four},
    {"five", &_bi_five},
    {"six", &_bi_six},
    {"seven", &_bi_seven},
    {"eight", &_bi_eight},
    {"nine", &_bi_nine},
    {"ten", &_bi_ten},
    
    {"true", &_bi_true},
    {"false", &_bi_false},
    
    {"empty", &_bi_empty},
    {"space", &_bi_space},
    {"colon", &_bi_colon},
    {"comma", &_bi_comma},
    {"tab", &_bi_tab},
    {"quote", &_bi_quote},
    {"newline", &_bi_newline},
    {"return", &_bi_newline},
    
    NULL,
};


/* add built-in language constants */
void _xte_constants_add_builtins(XTE *in_engine)
{
    _xte_constants_add(in_engine, bi_consts);
}




