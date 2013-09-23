/*
 
 xTalk Engine Function Unit
 xtalk_func.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Responsible for definition of built-in and parsing of all function calls
 
 */

#include "xtalk_internal.h"



/*********
 Utilities
 */

static int _xte_node_is_function(XTEAST *in_node, XTEFunctionImp *out_func)
{
    if (!in_node) return XTE_FALSE;
    if (in_node->type != XTE_AST_WORD) return XTE_FALSE;
    
    /* look for function in global function name list */
    for (int i = 0; i < in_node->engine->func_count; i++)
    {
        struct XTEFunctionInt *func = in_node->engine->funcs[i];
        if (_xte_compare_cstr(func->name, in_node->value.string) == 0)
        {
            if (out_func) *out_func = func->imp;
            return XTE_TRUE;
        }
    }
    
    return XTE_FALSE;
}



/*********
 Parsing
 */

void _xte_parse_functions(XTE *in_engine, XTEAST *in_stmt)
{
    /* iterate through statement words */
    for (int i = 0; i < in_stmt->children_count; i++)
    {
        XTEFunctionImp func;
        XTEAST *node;
        XTEAST *child = in_stmt->children[i];
        if (child->type == XTE_AST_OF)
        {
            /* check if the preceeding word is a registered function name */
            if (_xte_node_is_function(node =_xte_ast_list_child(in_stmt, i-1), &func))
            {
                /* convert the node to a function node */
                i--;
                node->type = XTE_AST_FUNCTION;
                node->note = _xte_clone_cstr(in_engine, node->value.string);
                if (node->value.string) free(node->value.string);
                //node->value.ptr = func;
                node->value.function.ptr = func;
                node->value.function.named = _xte_clone_cstr(in_engine, node->note);
                
                /* check if the preceeding word is "the" */
                node =_xte_ast_list_child(in_stmt, i-1);
                if (_xte_node_is_the(node))
                {
                    /* remove "the" */
                    _xte_ast_destroy(_xte_ast_list_remove(in_stmt, i-1));
                    i--;
                }
                
                /* don't process further until later, because we need everything that can complete
                 parsing to finish before we parse 'of' relationships */
                
                /* append the operands list */
                //_xte_ast_destroy(_xte_ast_list_remove(in_stmt, i+1));
                //_xte_ast_list_append(node, _xte_ast_list_remove(in_stmt, i+1));
            }
        }
        else if (child->type == XTE_AST_EXPRESSION)
        {
           // _xte_parse_functions(in_engine, child);
            
            /* check if the preceeding node is a word (function name) */
            if (_xte_node_is_identifier(node = _xte_ast_list_child(in_stmt, i-1)))
            {
                /* convert the node to a function node;
                 check for built-in function */
                node->note = _xte_clone_cstr(in_engine, node->value.string);
                if (_xte_node_is_function(node, &func))
                {
                    if (node->value.string) free(node->value.string);
                    node->value.function.ptr = func;
                    node->value.function.named = _xte_clone_cstr(in_engine, node->note);
                }
                else
                {
                    if (node->value.string) free(node->value.string);
                    node->value.function.ptr = NULL;
                    node->value.function.named = _xte_clone_cstr(in_engine, node->note);
                }
                node->type = XTE_AST_FUNCTION;
                i--;
                
                /* append the operands list */
                XTEAST *operands = _xte_ast_list_remove(in_stmt, i+1);
                while (operands->children_count > 0)
                {
                    _xte_ast_list_append(node, _xte_ast_list_remove(operands, 0));
                }
                _xte_ast_destroy(operands);
            }
        }
    }
}


/*
 *  _post_process_func
 *  ---------------------------------------------------------------------------------------------
 *  Rearrange comma-delimited function arguments into separate groups.
 *
 *  ! Modifies the input token stream, replacing it with the resulting abstract syntax tree.
 */
static void _post_process_func(XTE *in_engine, XTEAST *in_func)
{
    assert(IS_XTE(in_engine));
    assert(in_func != NULL);
    
    XTEAST *list = _xte_ast_create(in_engine, XTE_AST_LIST);
    assert(list != NULL);
    XTEAST *current_param = _xte_ast_create(in_engine, XTE_AST_EXPRESSION);
    assert(current_param != NULL);
    _xte_ast_list_append(list, current_param);
    int count = in_func->children_count;
    for (int i = 0; i < count; i++)
    {
        XTEAST *child = in_func->children[0];
        if (child && (child->type == XTE_AST_COMMA))
        {
            current_param = _xte_ast_create(in_engine, XTE_AST_EXPRESSION);
            _xte_ast_list_append(list, current_param);
            _xte_ast_destroy(_xte_ast_list_remove(in_func, 0));
        }
        else
            _xte_ast_list_append(current_param, _xte_ast_list_remove(in_func, 0));
    }
    while (list->children_count > 0)
    {
        _xte_ast_list_append(in_func, _xte_ast_list_remove(list, 0));
    }
    _xte_ast_destroy(list);
    if ((in_func->children_count == 1) && (in_func->children[0]) &&
        (in_func->children[0]->type == XTE_AST_EXPRESSION) &&
        (in_func->children[0]->children_count == 0))
        _xte_ast_destroy(_xte_ast_list_remove(in_func, 0));
}


/*
 *  _xte_post_process_funcs
 *  ---------------------------------------------------------------------------------------------
 *  Descend through the subexpressions and operands of an expression looking for function calls.
 *  Invoke _post_process_func() on each function call, to complete parsing of the parameters.
 *
 *  ! Modifies the input token stream, replacing it with the resulting abstract syntax tree.
 */
void _xte_post_process_funcs(XTE *in_engine, XTEAST *in_expr)
{
    assert(IS_XTE(in_engine));
    assert(in_expr != NULL);
    
    for (int i = 0; i < in_expr->children_count; i++)
    {
        XTEAST *func = in_expr->children[i];
        if (!func) continue;
        switch (func->type)
        {
            case XTE_AST_EXPRESSION:
            case XTE_AST_OPERATOR:
                _xte_post_process_funcs(in_engine, func);
                break;
            case XTE_AST_FUNCTION:
                _post_process_func(in_engine, func);
                break;
            default: break;
        }
        //if (func && func->type == XTE_AST_FUNCTION)
         //   _post_process_func(in_engine, func);
    }
}



/*********
 Dictionary
 */

/* adds a single function to the internal function table */
static void _xte_function_add(XTE *in_engine, struct XTEFunctionDef *in_def)
{
    /* add an entry to the function table */
    struct XTEFunctionInt **new_table = realloc(in_engine->funcs, sizeof(struct XTEFunctionInt*) * (in_engine->func_count + 1));
    if (!new_table) return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
    in_engine->funcs = new_table;
    struct XTEFunctionInt *new_function = new_table[in_engine->func_count++] = calloc(1, sizeof(struct XTEFunctionInt));
    if (!new_table) return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
    
    /* configure the function */
    new_function->name = _xte_clone_cstr(in_engine, in_def->name);
    new_function->arg_count = in_def->arg_count;
    new_function->imp = in_def->ptr;
}


/* add one or more functions with one call */
void _xte_functions_add(XTE *in_engine, struct XTEFunctionDef *in_defs)
{
    if (!in_defs) return;
    struct XTEFunctionDef *the_func = in_defs;
    while (the_func->name)
    {
        _xte_function_add(in_engine, the_func);
        the_func++;
    }
}



/*********
 Built-ins
 */

extern struct XTEFunctionDef _xte_builtin_math_functions[];
extern struct XTEFunctionDef _xte_builtin_date_functions[];

/* add built-in language functions */
void _xte_functions_add_builtins(XTE *in_engine)
{
    _xte_functions_add(in_engine, _xte_builtin_math_functions);
    _xte_functions_add(in_engine, _xte_builtin_date_functions);
}



