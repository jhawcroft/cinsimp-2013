/*
 
 xTalk Engine Source Formatting Utilities
 xtalk_srcfmat.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 xTalk source code formatting utilities; supports syntax colouring/styling and automatic indent
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"




#define _SPACES_PER_INDENT 2


struct Frame
{
    char *handler_name;
    enum {
        TYPE_SCRIPT,
        TYPE_HANDLER,
        TYPE_LOOP,
        TYPE_CONDITION,
    } type;
    enum {
        STATE_IN_BLOCK,
        STATE_COND_AWAIT_THEN,
        STATE_IN_BLOCK_MAY_ELSE,
        
        STATE_COND_EXP_THEN,
    } state;
};


struct Context {
    int indent;
    int error_indent;
    
    char line_text[XTALK_LIMIT_MAX_LINE_LENGTH_BYTES];
    XTEAST *line_keywords;
    
    int line_offset;
    
    struct Frame stack[XTALK_LIMIT_MAX_NESTED_BLOCKS];
    int stack_ptr;
};


static void _push(struct Context *in_context, int in_type)
{
    if (in_context->stack_ptr + 1 >= XTALK_LIMIT_MAX_NESTED_BLOCKS)
    {
        in_context->error_indent = in_context->indent;
        return;
    }
    in_context->stack_ptr++;
    in_context->stack[in_context->stack_ptr].type = in_type;
    in_context->stack[in_context->stack_ptr].handler_name = NULL;
    in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
}


static void _pop(struct Context *in_context)
{
    if (in_context->stack_ptr - 1 < 0)
    {
        in_context->error_indent = in_context->indent;
        return;
    }
    if (in_context->stack[in_context->stack_ptr].handler_name)
        free(in_context->stack[in_context->stack_ptr].handler_name);
    in_context->stack_ptr--;
}


static void _scan_for_then(struct Context *in_context);


static void _scan_for_else(struct Context *in_context, int no_else_if)
{
    XTEAST *keyword;
    
    in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK_MAY_ELSE;
    
    for (;;)
    {
        keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset++);
        if (!keyword) return;
        if ((keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_ELSE))
        {
            in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
            
            keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset++);
            if (!keyword)
            {
                _push(in_context, TYPE_CONDITION);
                in_context->indent++;
            }
            else if ((keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_IF))
            {
                if (no_else_if)
                {
                    in_context->error_indent = in_context->indent;
                    return;
                }
                _scan_for_then(in_context);
            }
            
            return;
        }
    }
}


static void _scan_for_then(struct Context *in_context)
{
    XTEAST *keyword;
    
    in_context->stack[in_context->stack_ptr].state = STATE_COND_AWAIT_THEN;
    
    for (;;)
    {
        keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset++);
        if (!keyword) return;
        if ((keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_THEN))
        {
            keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset);
            if (!keyword)
            {
                in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
                _push(in_context, TYPE_CONDITION);
                in_context->indent++;
                return;
            }
            else
            {
                _scan_for_else(in_context, XTE_TRUE);
                return;
            }
        }
    }
}


static void _prechecks(struct Context *in_context)
{
    XTEAST *keyword, *word;
    
    if ((in_context->stack[in_context->stack_ptr].state != STATE_IN_BLOCK) &&
        (in_context->stack[in_context->stack_ptr].state != STATE_IN_BLOCK_MAY_ELSE)) return;
    if (in_context->line_keywords->children_count == 0) return;
    switch (in_context->stack[in_context->stack_ptr].type)
    {
        case TYPE_HANDLER:
            keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset);
            if (keyword && (keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_END))
            {
                in_context->line_offset++;
                word = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset++);
                if ( (!word) || (word->type != XTE_AST_WORD) || (!word->value.string)
                    || (!xte_cstrings_equal(word->value.string, in_context->stack[in_context->stack_ptr].handler_name)) )
                {
                    in_context->error_indent = in_context->indent;
                    return;
                }
                
                _pop(in_context);
                in_context->indent--;
            }
            break;
            
        case TYPE_LOOP:
            keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset);
            if (keyword && (keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_END))
            {
                in_context->line_offset++;
                keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset++);
                if ((!keyword) || (keyword->type != XTE_AST_KEYWORD) || (keyword->value.keyword != XTE_AST_KW_REPEAT))
                {
                    in_context->error_indent = in_context->indent;
                    return;
                }
                
                _pop(in_context);
                in_context->indent--;
            }
            break;
            
        case TYPE_CONDITION:
            keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset);
            if (keyword && (keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_END))
            {
                in_context->line_offset++;
                keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset++);
                if ((!keyword) || (keyword->type != XTE_AST_KEYWORD) || (keyword->value.keyword != XTE_AST_KW_IF))
                {
                    in_context->error_indent = in_context->indent;
                    return;
                }
                
                _pop(in_context);
                in_context->indent--;
            }
            else if (keyword && (keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_ELSE)
                     && (in_context->stack[in_context->stack_ptr].state != STATE_IN_BLOCK_MAY_ELSE))
            {
                in_context->indent--;
            }
            break;
            
        case TYPE_SCRIPT:
            break;
    }
}


static void _postchecks(struct Context *in_context)
{
    XTEAST *keyword, *word;
    
    if (in_context->line_keywords->children_count != 0)
    {
        if (in_context->stack[in_context->stack_ptr].state == STATE_COND_AWAIT_THEN)
        {
            _scan_for_then(in_context);
            return;
        }
    }
    
    if (in_context->stack[in_context->stack_ptr].state == STATE_IN_BLOCK_MAY_ELSE)
    {
        in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
        
        keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset);
        if (keyword && (keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_ELSE))
        {
            _scan_for_else(in_context, XTE_FALSE);
        }
    }
    
    if (in_context->stack[in_context->stack_ptr].state != STATE_IN_BLOCK) return;
    if (in_context->line_keywords->children_count == 0) return;
    switch (in_context->stack[in_context->stack_ptr].type)
    {
        case TYPE_SCRIPT:
            keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset++);
            if (keyword && (keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_ON))
            {
                word = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset++);
                if ((!word) || (word->type != XTE_AST_WORD) || (!word->value.string))
                {
                    in_context->error_indent = 1;
                    return;
                }
                
                _push(in_context, TYPE_HANDLER);
                in_context->stack[in_context->stack_ptr].handler_name = word->value.string;
                word->value.string = NULL;
                
                in_context->indent++;
            }
            break;
            
        case TYPE_HANDLER:
        case TYPE_LOOP:
        case TYPE_CONDITION:
            keyword = _xte_ast_list_child(in_context->line_keywords, in_context->line_offset++);
            if (keyword && (keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_REPEAT))
            {
                _push(in_context, TYPE_LOOP);
                in_context->indent++;
            }
            else if (keyword && (keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_IF))
            {
                _scan_for_then(in_context);
            }
            else if (keyword && (keyword->type == XTE_AST_KEYWORD) && (keyword->value.keyword == XTE_AST_KW_ELSE))
            {
                in_context->line_offset--;
                _pop(in_context);
                _scan_for_else(in_context, XTE_FALSE);
            }
            
            break;
    }
}


static char const *_whitespace(int in_length)
{
#define WHITESPACE_SIZE 500
    static char const *const WHITESPACE =
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    ";
    assert(in_length >= 0);
    assert(in_length < WHITESPACE_SIZE);
    static char r[WHITESPACE_SIZE + 1];
    memcpy(r, WHITESPACE, in_length);
    r[in_length] = 0;
    return r;
}


static int _count_leading_whitespace_chars(char const *in_line_start, char const *in_line_end)
{
    char const *ptr = in_line_start;
    do
    {
        if (!isspace(*ptr)) break;
    } while (ptr++ != in_line_end);
    return (int)(ptr - in_line_start);
}


/* ! the source provided should be constant, and certainly must not change during the execution of this handler */
void xte_source_indent(XTE *in_engine, char const *in_source, long *io_selection_start, long *io_selection_length,
                       XTEIndentingHandler in_indenting_handler, void *in_context)
{
    /* setup processing context */
    struct Context context;
    context.indent = 0;
    context.error_indent = -1;
    
    context.stack_ptr = 0;
    context.stack[context.stack_ptr].handler_name = NULL;
    context.stack[context.stack_ptr].state = STATE_IN_BLOCK;
    context.stack[context.stack_ptr].type = TYPE_SCRIPT;
    
    long offset_adjustment = 0;
    long selection_start = *io_selection_start;
    long selection_end = *io_selection_start + *io_selection_length;
    
    /* enumerate the lines */
    char const *line_start = in_source;
    char const *line_ptr = in_source;
    long line_char_offset = 0;
    do
    {
        if ((*line_ptr == 0) || (*line_ptr == 10) || (*line_ptr == 13))
        {
            /* get line range */
            long line_length = line_ptr - line_start;
            char const *line_end;
            line_end = line_ptr - 1;
            
            /* count the number of leading whitespace characters */
            int existing_whitespace = (line_length == 0 ? 0 : _count_leading_whitespace_chars(line_start, line_end));
            
            /* get line text as string */
            memcpy(context.line_text, line_start, line_length);
            context.line_text[line_length] = 0;
            
            /* extract keywords on line */
            if (line_length == 0)
                context.line_keywords = _xte_ast_create(in_engine, XTE_AST_LIST);
            else
            {
                context.line_keywords = _xte_lex(in_engine, context.line_text);
                if (context.line_keywords && (context.line_keywords->children_count > 0) &&
                    (context.line_keywords->children[context.line_keywords->children_count-1]) &&
                    (context.line_keywords->children[context.line_keywords->children_count-1]->type == XTE_AST_NEWLINE))
                    _xte_ast_destroy(_xte_ast_list_remove(context.line_keywords, context.line_keywords->children_count-1));
            }
            context.line_offset = 0;
            
            /* pre-checks */
            if (context.line_keywords && (context.error_indent < 0))
                _prechecks(&context);
            
            /* do the indentation */
            int required_whitespace = _SPACES_PER_INDENT * (context.error_indent >= 0 ? context.error_indent : context.indent);
            int added_bytes = required_whitespace - existing_whitespace;
            if (added_bytes != 0)
            {
                in_indenting_handler(in_engine,
                                     in_context,
                                     line_char_offset,
                                     existing_whitespace,
                                     _whitespace(required_whitespace),
                                     required_whitespace);
            }
            
            /* adjust the selection */
            if (selection_start > line_char_offset)
                selection_start += added_bytes;
            if (selection_end > line_char_offset)
                selection_end += added_bytes;
            
            offset_adjustment += added_bytes;
            
            line_char_offset += xte_cstring_length(context.line_text) + added_bytes + 1;
            
            
            
            
            /* post-checks */
            if (context.line_keywords && (context.error_indent < 0))
                _postchecks(&context);
            
            /* cleanup */
            _xte_ast_destroy(context.line_keywords);
            
            /* continue to next line */
            line_start = line_ptr + 1;
        }
    } while (*(line_ptr++) != 0);
    
    /* cleanup */
    while (context.stack_ptr > 0)
        _pop(&context);
    
    /* output the adjusted selection */
    *io_selection_start = selection_start;
    *io_selection_length = selection_end - selection_start;
}



void xte_source_colourise(XTE *in_engine, char const *in_source, XTEColourisationHandler in_colorisation_handler, void *in_context)
{
    
}



