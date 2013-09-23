/*
 
 xTalk Engine Handler Parsing Unit
 xtalk_hdlr.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Responsible for parsing handlers, control structures and statements.  Specifically:
 -  command and function handlers
 -  repeat loops
 -  if...then...else structures
 -  pass, exit, next, global, return statements
 
 Commands are passed down to _cmds.c.  Expressions are passed down to _exprs.c.
 The results are coalesced into the AST for the handler.
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


/*********
 Types
 */

/*
 *  struct ParseContext
 *  ---------------------------------------------------------------------------------------------
 *  Context for the handler parser; avoids a whole heap of function parameters.
 */
struct ParseContext
{
    /* the relevant engine */
    XTE *engine;
    
    /* the entire token stream of the handler being parsed */
    XTEAST *stream;
    
    /* the AST that will be returned upon successful parse of the handler */
    XTEAST *handler;
    
    /* the destination AST node for successfully parsed statements */
    XTEAST *stmt_container;
    
    /* the offset of the token in stream where the currently parsing line begins (inclusive) */
    int line_start;
    
    /* the offset of the token in stream where the currently parsing line ends (inclusive) */
    int line_end;
    
    /* the number of tokens in the currently parsing line begins (inclusive) */
    int line_length;
    
    /* a boolean, XTE_TRUE if the currently parsing line has a checkpoint set in the script editor */
    int line_checkpoint;
    
    /* the offset of the token in the stream of wherever the relevant parsing function is up-to,
     within the currently parsing line */
    int line_offset;
    
    /* the actual, hard source line number of the currently parsing line */
    int line_source;
    
    /* the logical line number of the currently parsing line (doesn't see line continuations ¬) */
    int line_number;
    
    /* a stack to keep track of various nested block types */
    struct ParseContextFrame {
        /* types of block allowed within a handler */
        enum {
            BLOCK_HANDLER,
            BLOCK_LOOP,
            BLOCK_CONDITION,
        } type;
        
        /* state to help track parsing complex structures like if..then..else */
        enum {
            STATE_IN_BLOCK,
            STATE_IN_BLOCK_MAY_ELSE,
            STATE_COND_EXP_THEN,
            
        } state;
        
        /* the AST node for the current block */
        XTEAST *block;
        
        /* used by complex structures like if..then..else to track the actual 
         AST node for the latest parsed structure */
        XTEAST *last_ctrl;
        
        /* boolean, XTE_TRUE when an else clause has been parsed;
         allows the parser to prevent the script from legally containing 
         more than one else clause per condition block */
        int got_default;
        
    } stack[XTALK_LIMIT_MAX_NESTED_BLOCKS];
    
    /* a pointer for the above stack;
     points to the current top of the stack */
    int stack_ptr;
};


/*
 *  ERROR_SYNTAX
 *  ---------------------------------------------------------------------------------------------
 *  Short macro wrapper for the engine's syntax error reporting function.
 */
#ifdef ERROR_SYNTAX
#undef ERROR_SYNTAX
#endif
#define ERROR_SYNTAX(in_template, in_arg1, in_arg2) _xte_error_syntax(in_context->engine, in_context->line_source, \
in_template, in_arg1, in_arg2, NULL)



/*********
 Implementation
 */

/*
 *  _register_chkpt
 *  ---------------------------------------------------------------------------------------------
 *  Registers the <in_node> as having a checkpoint set in the script editor; enables the 
 *  interpreter to break on AST nodes when a checkpoint has been set by the user.
 *
 *  Copies the appropriate actual source line number to the node so that the correct line can
 *  be highlighted by the debugging user interface.
 */
static void _register_chkpt(struct ParseContext *in_context, XTEAST *in_node, int is_checkpoint)
{
    assert(in_context != NULL);
    assert(in_node != NULL);
    
    if (is_checkpoint > 0)
    {
        in_node->flags |= XTE_AST_FLAG_IS_CHECKPOINT;
        in_node->source_line = in_context->line_source;//in_context->line_checkpoint;
    }
    else
        in_node->source_line = in_context->line_source;
}


/*
 *  Notes about Conditions
 *  ---------------------------------------------------------------------------------------------
 *  Conditions are quite complicated and have many variations in form:
 *
 *  -   if ... then ... else ...
 *
 *  -   if ...
 *      then ...
 *      else
 *        ...
 *      end if
 *  -   if ... then
 *        ...
 *      else ...
 *  etc.
 *
 *  As such, the parsing of conditions is spread across 4-separate functions:
 *  _xte_parse_block_if, _xte_parse_block_then, _xte_parse_block_else and _xte_parse_hdlr_line.
 *
 *  The resulting abstract syntax node is an XTE_AST_CONDITION.  The nodes children are organised
 *  consistently in the following form:
 *
 *      [ 1st condition expression ]
 *      [ 1st statement block ]
 *      ...
 *      [ Nth condition expression ]
 *      [ Nth statement block ]
 *      [ default statement block ]   (else)
 */


/*
 *  Condition Parsing Forward Declarations
 *  ---------------------------------------------------------------------------------------------
 *  Permits limited recursion within the parser.
 */
static int _xte_parse_block_if(struct ParseContext *in_context, int is_else_if, int is_checkpoint);
static int _xte_parse_block_then(struct ParseContext *in_context, int is_else_if, int is_checkpoint);
static int _xte_parse_block_else(struct ParseContext *in_context, int is_after_then, int is_checkpoint);
static int _xte_parse_stmt(struct ParseContext *in_context);

 
/*
 *  _xte_parse_block_if
 *  ---------------------------------------------------------------------------------------------
 *  Invoked by _xte_parse_hdlr_line() for lines beginning with "if".
 *  Also invoked by _xte_parse_block_else() for lines beginning with "else if".
 *  Attempts to parse the condition expression only.
 *
 *  If there is an error during parsing of this line, returns XTE_FALSE.  If parsing the line
 *  raises no error, returns XTE_TRUE.  Normal reporting of syntax errors applies.
 */
static int _xte_parse_block_if(struct ParseContext *in_context, int is_else_if, int is_checkpoint)
{
    assert(in_context != NULL);
    assert(IS_BOOL(is_else_if));
    assert(IS_BOOL(is_checkpoint));
    
    /* we expect the keyword preceeding this call to be "if" */
    assert(in_context->line_offset - 1 >= 0);
    assert(in_context->stream != NULL);
    assert(in_context->stream->children_count >= in_context->line_offset);
    assert(in_context->stream->children[in_context->line_offset-1]);
    assert(in_context->stream->children[in_context->line_offset-1]->type == XTE_AST_KEYWORD);
    assert(in_context->stream->children[in_context->line_offset-1]->value.keyword == XTE_AST_KW_IF);
    
    XTEAST *cond = in_context->stack[in_context->stack_ptr].last_ctrl;
    assert(cond != NULL);
    
    /* build a condition expression */
    XTEAST *cond_expr = _xte_ast_create(in_context->engine, XTE_AST_LIST);
    assert(cond_expr != NULL);
    _register_chkpt(in_context, cond_expr, is_checkpoint);
    _xte_ast_list_append(cond, cond_expr);
    
    for (; in_context->line_offset <= in_context->line_end; in_context->line_offset++)
    {
        XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
        if (token && (token->type == XTE_AST_KEYWORD)
            && (token->value.keyword == XTE_AST_KW_THEN)) break;
        _xte_ast_list_append(cond_expr, token);
        in_context->stream->children[in_context->line_offset] = NULL;
    }
    if (cond_expr->children_count == 0)
    {
        ERROR_SYNTAX("Expected true or false expression here.", NULL, NULL);
        return XTE_FALSE;
    }
    if (!_xte_parse_expression(in_context->engine, cond_expr, in_context->line_source)) return XTE_FALSE;
    
    if (in_context->line_offset <= in_context->line_end)
    {
        /* skip "then"; continue */
        XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
        if (token && (token->type == XTE_AST_KEYWORD) && (token->value.keyword == XTE_AST_KW_THEN))
            in_context->line_offset++;
        else
        {
            /* this shouldn't happen; condition can only end either by end of line or "then" */
            assert(0);
            return XTE_FALSE;
        }
    }
    else
    {
        /* got end of line; end of processing until next */
        in_context->stack[in_context->stack_ptr].state = STATE_COND_EXP_THEN;
        return XTE_TRUE;
    }
    
    if (!_xte_parse_block_then(in_context, is_else_if, XTE_FALSE)) return XTE_FALSE;
    
    return XTE_TRUE;
}


/*
 *  _xte_parse_block_then
 *  ---------------------------------------------------------------------------------------------
 *  Invoked by _xte_parse_block_if() following "then".
 *  Invoked by _xte_parse_hdlr_line() following "then".
 *  Attempts to parse single-line statement blocks and begin multi-line statement blocks.
 *
 *  If there is an error during parsing of this line, returns XTE_FALSE.  If parsing the line
 *  raises no error, returns XTE_TRUE.  Normal reporting of syntax errors applies.
 */
static int _xte_parse_block_then(struct ParseContext *in_context, int is_else_if, int is_checkpoint)
{
    assert(in_context != NULL);
    assert(IS_BOOL(is_else_if));
    assert(IS_BOOL(is_checkpoint));
    
    /* we do not assert that the previous keyword was "then" because
     there is no easy way to do so at present;
     "then" may follow a line break */
    
    XTEAST *cond = in_context->stack[in_context->stack_ptr].last_ctrl;
    assert(cond != NULL);
    
    /* if there's still stuff on this line, it's a single line block,
     possibly terminated by an else clause; build block */
    if (in_context->line_offset <= in_context->line_end)
    {
        XTEAST *cond_block = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(cond_block != NULL);
        _register_chkpt(in_context, cond_block, is_checkpoint);
        _xte_ast_list_append(cond, cond_block);
        
        in_context->stmt_container = cond_block;
        int stmt_end_offset;
        for (stmt_end_offset = in_context->line_offset; stmt_end_offset <= in_context->line_end; stmt_end_offset++)
        {
            XTEAST *token = _xte_ast_list_child(in_context->stream, stmt_end_offset);
            if (token && (token->type == XTE_AST_KEYWORD) && (token->value.keyword == XTE_AST_KW_ELSE))
            {
                stmt_end_offset--;
                if (stmt_end_offset < in_context->line_offset)
                {
                    ERROR_SYNTAX("Expected some statement but found \"else\".", NULL, NULL);
                    return XTE_FALSE;
                }
                break;
            }
        }
        if (stmt_end_offset > in_context->line_end) stmt_end_offset = in_context->line_end;
        int save_line_end = in_context->line_end;
        in_context->line_end = stmt_end_offset;
        if (!_xte_parse_stmt(in_context)) return XTE_FALSE;
        in_context->line_offset = stmt_end_offset + 1;
        in_context->line_end = save_line_end;
        
        /*XTEAST *stmt = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(stmt != NULL);
        _xte_ast_list_append(cond_block, stmt);
        
        for (; in_context->line_offset <= in_context->line_end; in_context->line_offset++)
        {
            XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
            if (token && (token->type == XTE_AST_KEYWORD) && (token->value.keyword == XTE_AST_KW_ELSE)) break;
            _xte_ast_list_append(stmt, token);
            in_context->stream->children[in_context->line_offset] = NULL;
        }
        // **TODO** parse statement*/
    }
    else
    {
        /* nothing on this line following "then";
         start a multi-line block */
        in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
        
        if (in_context->stack_ptr + 1 >= XTALK_LIMIT_MAX_NESTED_BLOCKS)
        {
            ERROR_SYNTAX("Too many nested blocks.", NULL, NULL);
            return XTE_FALSE;
        }
        in_context->stack_ptr++;
        
        XTEAST *cond_block = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(cond_block != NULL);
        _xte_ast_list_append(cond, cond_block);
        in_context->stack[in_context->stack_ptr].block = cond_block;
        in_context->stack[in_context->stack_ptr].type = BLOCK_CONDITION;
        in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
        in_context->stack[in_context->stack_ptr].got_default = XTE_FALSE;
        return XTE_TRUE;
    }
    
    /* is there "else" ? */
    if (in_context->line_offset <= in_context->line_end)
    {
        /* skip "else"; continue */
        XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
        if (token && (token->type == XTE_AST_KEYWORD) && (token->value.keyword == XTE_AST_KW_ELSE))
        {
            /* style: don't allow "else" to follow "else if" on the same line;
             quickly ends up looking awful and a really long line */
            if (is_else_if)
            {
                ERROR_SYNTAX("\"else\" cannot follow \"else if\" on the same line.", NULL, NULL);
                return XTE_FALSE;
            }
            
            in_context->line_offset++;
        }
        else
        {
            /* this shouldn't happen; block can only end either by end of line or "else" */
            assert(0);
            return XTE_FALSE;
        }
    }
    else
    {
        /* end of line; end of processing until next */
        in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK_MAY_ELSE;
        return XTE_TRUE;
    }
    
    if (!_xte_parse_block_else(in_context, XTE_TRUE, XTE_FALSE)) return XTE_FALSE;
    
    return XTE_TRUE;
}


/*
 *  _xte_parse_block_else
 *  ---------------------------------------------------------------------------------------------
 *  Invoked by _xte_parse_block_then() following "else".
 *  Invoked by _xte_parse_hdlr_line() following "else" (following a single-line block,
 *  and an "else" terminating a multi-line block.)
 *  Attempts to parse default single-line statement blocks and begin multi-line statement blocks.
 *  Also begins "else if" clauses.
 *
 *  If there is an error during parsing of this line, returns XTE_FALSE.  If parsing the line
 *  raises no error, returns XTE_TRUE.  Normal reporting of syntax errors applies.
 */
static int _xte_parse_block_else(struct ParseContext *in_context, int is_after_then, int is_checkpoint)
{
    assert(in_context != NULL);
    assert(IS_BOOL(is_after_then));
    assert(IS_BOOL(is_checkpoint));
    
    /* we expect the keyword preceeding this call to be "else" */
    assert(in_context->line_offset - 1 >= 0);
    assert(in_context->stream != NULL);
    assert(in_context->stream->children_count >= in_context->line_offset);
    assert(in_context->stream->children[in_context->line_offset-1]);
    assert(in_context->stream->children[in_context->line_offset-1]->type == XTE_AST_KEYWORD);
    assert(in_context->stream->children[in_context->line_offset-1]->value.keyword == XTE_AST_KW_ELSE);
    
    /* don't allow two else clauses for a condition */
    if (in_context->stack[in_context->stack_ptr].got_default)
    {
        ERROR_SYNTAX("Expected \"end if\" after \"else\".", NULL, NULL);
        return XTE_FALSE;
    }
    
    /* if the next word is "if", start another condition */
    if (in_context->line_offset <= in_context->line_end)
    {
        XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
        if (token && (token->type == XTE_AST_KEYWORD) && (token->value.keyword == XTE_AST_KW_IF))
        {
            /* style: don't allow "else if" to follow "then" on the same line;
             quickly ends up looking awful and a really long line */
            if (is_after_then)
            {
                ERROR_SYNTAX("\"else if\" cannot follow \"then\" on the same line.", NULL, NULL);
                return XTE_FALSE;
            }
            
            in_context->line_offset++;
            if (!_xte_parse_block_if(in_context, XTE_TRUE, is_checkpoint)) return XTE_FALSE;
            return XTE_TRUE;
        }
    }
    
    /* if there's still stuff on this line, make it into a single line else block */
    in_context->stack[in_context->stack_ptr].got_default = XTE_TRUE;
    XTEAST *cond = in_context->stack[in_context->stack_ptr].last_ctrl;
    assert(cond != NULL);
    if (in_context->line_offset <= in_context->line_end)
    {
        XTEAST *cond_block = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(cond_block != NULL);
        _register_chkpt(in_context, cond_block, is_checkpoint);
        _xte_ast_list_append(cond, cond_block);
        
        in_context->stmt_container = cond_block;
        return _xte_parse_stmt(in_context);
        
        /*XTEAST *stmt = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(stmt != NULL);
        _xte_ast_list_append(cond_block, stmt);
        
        for (; in_context->line_offset <= in_context->line_end; in_context->line_offset++)
        {
            XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
            _xte_ast_list_append(stmt, token);
            in_context->stream->children[in_context->line_offset] = NULL;
        }
        // **TODO** parse statement*/
    }
    else
    {
        /* nothing on this line following "else";
         start a multi-line block */
        in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
        
        if (in_context->stack_ptr + 1 >= XTALK_LIMIT_MAX_NESTED_BLOCKS)
        {
            ERROR_SYNTAX("Too many nested blocks.", NULL, NULL);
            return XTE_FALSE;
        }
        in_context->stack_ptr++;
        
        XTEAST *cond_block = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(cond_block != NULL);
        _xte_ast_list_append(cond, cond_block);
        in_context->stack[in_context->stack_ptr].block = cond_block;
        in_context->stack[in_context->stack_ptr].type = BLOCK_CONDITION;
        in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
    }
    
    return XTE_TRUE;
}


/*
 *  _xte_parse_global_decl
 *  ---------------------------------------------------------------------------------------------
 *  Invoked by _xte_parse_stmt() for lines beginning with "if".
 *  Parses a global declaration, consisting of a comma delimited list of one or more variable
 *  names.
 *
 *  If there is an error during parsing of this line, returns XTE_FALSE.  If parsing the line
 *  raises no error, returns XTE_TRUE.  Normal reporting of syntax errors applies.
 */
static int _xte_parse_global_decl(struct ParseContext *in_context)
{
    assert(in_context != NULL);
    
    /* we expect the keyword preceeding this call to be "global" */
    assert(in_context->line_offset - 1 >= 0);
    assert(in_context->stream != NULL);
    assert(in_context->stream->children_count >= in_context->line_offset);
    assert(in_context->stream->children[in_context->line_offset-1]);
    assert(in_context->stream->children[in_context->line_offset-1]->type == XTE_AST_KEYWORD);
    assert(in_context->stream->children[in_context->line_offset-1]->value.keyword == XTE_AST_KW_GLOBAL);
    
    /* build global declaration node */
    XTEAST *global_decl = _xte_ast_create(in_context->engine, XTE_AST_GLOBAL_DECL);
    assert(global_decl != NULL);
    _register_chkpt(in_context, global_decl, in_context->line_checkpoint);
    _xte_ast_list_append(in_context->stmt_container, global_decl);
    if (in_context->line_offset > in_context->line_end)
    {
        ERROR_SYNTAX("Expected global variable name after \"global\".", NULL, NULL);
        return XTE_FALSE;
    }
    
    /* enumerate identifiers */
    while (in_context->line_offset <= in_context->line_end)
    {
        XTEAST *identifier = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
        if ((!identifier) || (identifier->type != XTE_AST_WORD))
        {
            ERROR_SYNTAX("Expected global variable here but found \"%s\".", _lexer_term_desc(identifier), NULL);
            return XTE_FALSE;
        }
        if (!_xte_node_is_identifier(identifier))
        {
            ERROR_SYNTAX("\"%s\" is not a valid variable name.", _lexer_term_desc(identifier), NULL);
            return XTE_FALSE;
        }
        _xte_ast_list_append(global_decl, identifier);
        in_context->stream->children[in_context->line_offset-1] = NULL;
        
        /* check for another */
        if (in_context->line_offset <= in_context->line_end)
        {
            XTEAST *comma = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
            if ((!comma) || (comma->type != XTE_AST_COMMA))
            {
                ERROR_SYNTAX("Expected end of line but found \"%s\".", _lexer_term_desc(comma), NULL);
                return XTE_FALSE;
            }
            if (in_context->line_offset > in_context->line_end)
            {
                ERROR_SYNTAX("Expected another global variable name after \",\".", NULL, NULL);
                return XTE_FALSE;
            }
        }
    }
    
    return XTE_TRUE;
}


/*
 *  _in_loop
 *  ---------------------------------------------------------------------------------------------
 *  Invoked by _xte_parse_exit() for lines beginning with "exit".
 *  Checks if the currently parsing statement is within a loop at any nesting level within the
 *  current handler.
 *
 *  Returns XTE_TRUE if it is, XTE_FALSE if it isn't.
 */
static int _in_loop(struct ParseContext *in_context)
{
    assert(in_context != NULL);
    assert(in_context->stack_ptr >= 0);
    for (int i = 0; i <= in_context->stack_ptr; i++)
    {
        if (in_context->stack[i].type == BLOCK_LOOP) return XTE_TRUE;
    }
    return XTE_FALSE;
}


/*
 *  _xte_parse_exit
 *  ---------------------------------------------------------------------------------------------
 *  Invoked by _xte_parse_stmt() for lines beginning with "exit".
 *  Parses the "exit ..." syntax; specifically:
 *
 *  -   exit to user        escape the current call chain responding to a specific system event
 *                          (returns control to the user)
 *  -   exit repeat         escape from the current repeat loop
 *  -   exit <handler>      return from the command/function handler
 *
 *  Other forms of exit are parsed directly by _xte_parse_stmt().
 *
 *  If there is an error during parsing of this line, returns XTE_FALSE.  If parsing the line
 *  raises no error, returns XTE_TRUE.  Normal reporting of syntax errors applies.
 */
static int _xte_parse_exit(struct ParseContext *in_context)
{
    assert(in_context != NULL);
    
    /* we expect the keyword preceeding this call to be "exit" */
    assert(in_context->line_offset - 1 >= 0);
    assert(in_context->stream != NULL);
    assert(in_context->stream->children_count >= in_context->line_offset);
    assert(in_context->stream->children[in_context->line_offset-1]);
    assert(in_context->stream->children[in_context->line_offset-1]->type == XTE_AST_KEYWORD);
    assert(in_context->stream->children[in_context->line_offset-1]->value.keyword == XTE_AST_KW_EXIT);
    
    /* build exit node */
    XTEAST *exit_node = _xte_ast_create(in_context->engine, XTE_AST_EXIT);
    assert(exit_node != NULL);
    _register_chkpt(in_context, exit_node, in_context->line_checkpoint);
    _xte_ast_list_append(in_context->stmt_container, exit_node);
    
    if (in_context->line_offset > in_context->line_end)
    {
        if (_in_loop(in_context))
            ERROR_SYNTAX("Expected \"repeat\" after \"exit\".", NULL, NULL);
        else
            ERROR_SYNTAX("Expected \"%s\" after \"exit\".", in_context->handler->value.handler.name, NULL);
        return XTE_FALSE;
    }
    
    XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
    if (token && (token->type == XTE_AST_KEYWORD) && (token->value.keyword == XTE_AST_KW_REPEAT))
    {
        if (in_context->line_offset <= in_context->line_end)
        {
            token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
            ERROR_SYNTAX("Expected end of line but found %s.", _lexer_term_desc(token), NULL);
            return XTE_FALSE;
        }
        if (!_in_loop(in_context))
        {
            ERROR_SYNTAX("Found \"exit repeat\" outside a repeat loop.", NULL, NULL);
            return XTE_FALSE;
        }
        exit_node->value.exit = XTE_AST_EXIT_LOOP;
        return XTE_TRUE;
    }
    
    if (token && (token->type == XTE_AST_WORD) && (_xte_compare_cstr(token->value.string, "to") == 0))
    {
        if (in_context->line_offset > in_context->line_end)
        {
            ERROR_SYNTAX("Expected \"user\" after \"exit to\".", NULL, NULL);
            return XTE_FALSE;
        }
        token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
        if ((!token) || (token->type != XTE_AST_WORD) || (_xte_compare_cstr(token->value.string, "user") != 0))
        {
            ERROR_SYNTAX("Expected \"exit to user\" but found %s.", _lexer_term_desc(token), NULL);
            return XTE_FALSE;
        }
        exit_node->value.exit = XTE_AST_EXIT_EVENT;
        return XTE_TRUE;
    }
    
    if ((!token) || (token->type != XTE_AST_WORD) || (!xte_cstrings_equal(token->value.string, in_context->handler->value.handler.name)) )
    {
        ERROR_SYNTAX("Expected \"exit %s\" but found %s.", in_context->handler->value.handler.name, _lexer_term_desc(token));
        return XTE_FALSE;
    }
    
    exit_node->value.exit = XTE_AST_EXIT_HANDLER;
    return XTE_TRUE;
}


/*
 *  _xte_parse_stmt
 *  ---------------------------------------------------------------------------------------------
 *  Invoked by _xte_parse_hdlr_line() for lines not beginning with a keyword.
 *  Invoked by _xte_parse_block_then() for tokens on the same line and following "then".
 *  Invoked by _xte_parse_block_else() for tokens on the same line and following "else".
 *
 *  Parses or invokes the appropriate subroutine to handle:
 *  -   global <var> [, <var> [, <var>]]
 *  -   exit to user, exit repeat, exit <handlerName>
 *  -   pass <handlerName>
 *  -   next repeat
 *  -   return [<expression>]
 *  -   command [<arg1> [, <arg2> [, <argN>]]]
 *  -   built-in command syntax (via _cmds.c)
 *
 *  If there is an error during parsing of this line, returns XTE_FALSE.  If parsing the line
 *  raises no error, returns XTE_TRUE.  Normal reporting of syntax errors applies.
 */
static int _xte_parse_stmt(struct ParseContext *in_context)
{
    assert(in_context != NULL);
    
    /* check for known statement types;
     global, exit, pass, next, return */
    if (in_context->line_offset <= in_context->line_end)
    {
        XTEAST *keyword = _xte_ast_list_child(in_context->stream, in_context->line_offset);
        if (keyword && (keyword->type == XTE_AST_KEYWORD))
        {
            /* we have a keyword, check what it is */
            if (keyword->value.keyword == XTE_AST_KW_GLOBAL)
            {
                in_context->line_offset++;
                return _xte_parse_global_decl(in_context);
            }
            else if (keyword->value.keyword == XTE_AST_KW_EXIT)
            {
                in_context->line_offset++;
                return _xte_parse_exit(in_context);
            }
            else if (keyword->value.keyword == XTE_AST_KW_PASS)
            {
                in_context->line_offset++;
                if (in_context->line_offset > in_context->line_end)
                {
                    ERROR_SYNTAX("Expected \"%s\" after \"pass\".", in_context->handler->value.handler.name, NULL);
                    return XTE_FALSE;
                }
                XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
                if ((!token) || (token->type != XTE_AST_WORD) ||
                    (!xte_cstrings_equal(token->value.string, in_context->handler->value.handler.name)) )
                {
                    ERROR_SYNTAX("Expected \"pass %s\" here.", in_context->handler->value.handler.name, NULL);
                    return XTE_FALSE;
                }
                if (in_context->line_offset <= in_context->line_end)
                {
                    ERROR_SYNTAX("Expected end of line after \"pass %s\".", in_context->handler->value.handler.name, NULL);
                    return XTE_FALSE;
                }
                
                XTEAST *exit_node = _xte_ast_create(in_context->engine, XTE_AST_EXIT);
                assert(exit_node != NULL);
                _register_chkpt(in_context, exit_node, in_context->line_checkpoint);
                _xte_ast_list_append(in_context->stmt_container, exit_node);
                exit_node->value.exit = XTE_AST_EXIT_PASSING;
                return XTE_TRUE;
            }
            else if (keyword->value.keyword == XTE_AST_KW_NEXT)
            {
                in_context->line_offset++;
                if (in_context->line_offset > in_context->line_end)
                {
                    ERROR_SYNTAX("Expected \"repeat\" after \"next\".", NULL, NULL);
                    return XTE_FALSE;
                }
                XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
                if ((!token) || (token->type != XTE_AST_KEYWORD) || (token->value.keyword != XTE_AST_KW_REPEAT))
                {
                    ERROR_SYNTAX("Expected \"next repeat\" here.", NULL, NULL);
                    return XTE_FALSE;
                }
                if (!_in_loop(in_context))
                {
                    ERROR_SYNTAX("Found \"next repeat\" outside a repeat loop.", NULL, NULL);
                    return XTE_FALSE;
                }
                if (in_context->line_offset <= in_context->line_end)
                {
                    ERROR_SYNTAX("Expected end of line after \"next repeat\".", NULL, NULL);
                    return XTE_FALSE;
                }
                
                XTEAST *exit_node = _xte_ast_create(in_context->engine, XTE_AST_EXIT);
                assert(exit_node != NULL);
                _register_chkpt(in_context, exit_node, in_context->line_checkpoint);
                _xte_ast_list_append(in_context->stmt_container, exit_node);
                exit_node->value.exit = XTE_AST_EXIT_ITERATION;
                return XTE_TRUE;
            }
            else if (keyword->value.keyword == XTE_AST_KW_RETURN)
            {
                in_context->line_offset++;
                
                XTEAST *exit_node = _xte_ast_create(in_context->engine, XTE_AST_EXIT);
                assert(exit_node != NULL);
                _register_chkpt(in_context, exit_node, in_context->line_checkpoint);
                _xte_ast_list_append(in_context->stmt_container, exit_node);
                exit_node->value.exit = XTE_AST_EXIT_HANDLER;
                
                if (in_context->line_offset <= in_context->line_end)
                {
                    XTEAST *expr = _xte_ast_create(in_context->engine, XTE_AST_LIST);
                    assert(expr != NULL);
                    _xte_ast_list_append(exit_node, expr);
                    
                    for (; in_context->line_offset <= in_context->line_end; in_context->line_offset++)
                    {
                        XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
                        _xte_ast_list_append(expr, token);
                        in_context->stream->children[in_context->line_offset] = NULL;
                    }
                    
                    return _xte_parse_expression(in_context->engine, expr, in_context->line_source);
                }
                
                return XTE_TRUE;
            }
            
            /* keyword is unknown, thus illegal */
            ERROR_SYNTAX("Expected command name but found \"%s\".", _lexer_term_desc(keyword), NULL);
            return XTE_FALSE;
        }
    }
    
    /* transfer tokens on this line into a statement */
    XTEAST *stmt = _xte_ast_create(in_context->engine, XTE_AST_LIST);
    assert(stmt != NULL);
    
    _xte_ast_list_append(in_context->stmt_container, stmt);
    for (; in_context->line_offset <= in_context->line_end; in_context->line_offset++)
    {
        XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
        _xte_ast_list_append(stmt, token);
        in_context->stream->children[in_context->line_offset] = NULL;
    }
    
    int err = _xte_parse_command(in_context->engine, stmt, in_context->line_source);
    if (err == XTE_FALSE) return XTE_FALSE;
    
    _register_chkpt(in_context, stmt, in_context->line_checkpoint);
    return err;
}


/*
 *  _xte_parse_repeat
 *  ---------------------------------------------------------------------------------------------
 *  Invoked by _xte_parse_hdlr_line() for lines beginning with "repeat".
 *  Attempt to parse the loop definition line and configure the parser to parse a statement block
 *  for the loop content.
 *
 *  The resulting abstract syntax node is an XTE_AST_LOOP.  The nodes children are organised
 *  depending on the type of loop, which is represented by node->loop:
 *
 *  -   XTE_AST_LOOP_FOREVER    : all children are statements to be executed
 *  -   XTE_AST_LOOP_UNTIL,
 *      XTE_AST_LOOP_WHILE      : 1st child is loop condition
 *  -   XTE_AST_LOOP_NUMBER     : 1st child is number of times loop should iterate
 *  -   XTE_AST_LOOP_COUNT_UP,
 *      XTE_AST_LOOP_COUNT_DOWN : 1st child is loop count variable name
 *                                2nd child is counter start value expression
 *                                3rd child is counter end value expression
 *
 *  If there is an error during parsing of this line, returns XTE_FALSE.  If parsing the line
 *  raises no error, returns XTE_TRUE.  Normal reporting of syntax errors applies.
 */
static int _xte_parse_repeat(struct ParseContext *in_context)
{
    assert(in_context != NULL);
    assert(in_context->stream != NULL);
    assert(in_context->stream->children_count >= in_context->line_offset);
    assert(in_context->stream->children[in_context->line_offset-1] != NULL);
    assert(in_context->stream->children[in_context->line_offset-1]->type == XTE_AST_KEYWORD);
    assert(in_context->stream->children[in_context->line_offset-1]->value.keyword == XTE_AST_KW_REPEAT);
    
    /* start a loop block;
     the entire definition is on this line */
    XTEAST *loop_block = _xte_ast_create(in_context->engine, XTE_AST_LOOP);
    assert(loop_block != NULL);
    _register_chkpt(in_context, loop_block, in_context->line_checkpoint);
    _xte_ast_list_append(in_context->stack[in_context->stack_ptr].block, loop_block);
    
    if (in_context->stack_ptr + 1 >= XTALK_LIMIT_MAX_NESTED_BLOCKS)
    {
        ERROR_SYNTAX("Too many nested blocks.", NULL, NULL);
        return XTE_FALSE;
    }
    in_context->stack_ptr++;
    
    in_context->stack[in_context->stack_ptr].block = loop_block;
    in_context->stack[in_context->stack_ptr].type = BLOCK_LOOP;
    in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
    
    loop_block->value.loop = XTE_AST_LOOP_FOREVER;
    
    /* parse the loop definition */
    if (in_context->line_offset > in_context->line_end) return XTE_TRUE;
    
    XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
    if (token && (token->type == XTE_AST_WORD) && (_xte_compare_cstr(token->value.string, "forever") == 0))
    {
        /* forever */
        if (in_context->line_offset > in_context->line_end) return XTE_TRUE;
        
        ERROR_SYNTAX("Expected end of line after \"repeat forever\".", NULL, NULL);
        return XTE_FALSE;
    }
    else if (token && (token->type == XTE_AST_WORD) && (_xte_compare_cstr(token->value.string, "until") == 0))
    {
        /* until <condition> */
        loop_block->value.loop = XTE_AST_LOOP_UNTIL;
        
        XTEAST *loop_cond = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(loop_cond != NULL);
        _xte_ast_list_append(loop_block, loop_cond);
        
        for (; in_context->line_offset <= in_context->line_end; in_context->line_offset++)
        {
            XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
            _xte_ast_list_append(loop_cond, token);
            in_context->stream->children[in_context->line_offset] = NULL;
        }
        if (loop_cond->children_count == 0)
        {
            ERROR_SYNTAX("Expected true or false expression here.", NULL, NULL);
            return XTE_FALSE;
        }
        if (!_xte_parse_expression(in_context->engine, loop_cond, in_context->line_source)) return XTE_FALSE;
        
        return XTE_TRUE;
    }
    else if (token && (token->type == XTE_AST_WORD) && (_xte_compare_cstr(token->value.string, "while") == 0))
    {
        /* while <condition> */
        loop_block->value.loop = XTE_AST_LOOP_WHILE;
        
        XTEAST *loop_cond = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(loop_cond != NULL);
        _xte_ast_list_append(loop_block, loop_cond);
        
        for (; in_context->line_offset <= in_context->line_end; in_context->line_offset++)
        {
            XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
            _xte_ast_list_append(loop_cond, token);
            in_context->stream->children[in_context->line_offset] = NULL;
        }
        if (loop_cond->children_count == 0)
        {
            ERROR_SYNTAX("Expected true or false expression here.", NULL, NULL);
            return XTE_FALSE;
        }
        if (!_xte_parse_expression(in_context->engine, loop_cond, in_context->line_source)) return XTE_FALSE;
        
        return XTE_TRUE;
    }
    else if (token && (token->type == XTE_AST_WORD) && (_xte_compare_cstr(token->value.string, "with") == 0))
    {
        /* with <ident> = <expr> {to | down to} <expr> */
        loop_block->value.loop = XTE_AST_LOOP_COUNT_UP;
        
        if (in_context->line_offset > in_context->line_end)
        {
            ERROR_SYNTAX("Expected counter variable after \"repeat with\".", NULL, NULL);
            return XTE_FALSE;
        }
        token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
        if ((!token) || (token->type != XTE_AST_WORD))
        {
            ERROR_SYNTAX("Expected counter variable here but found \"%s\".", _lexer_term_desc(token), NULL);
            return XTE_FALSE;
        }
        if (!_xte_node_is_identifier(token))
        {
            ERROR_SYNTAX("\"%s\" is not a valid variable name.", _lexer_term_desc(token), NULL);
            return XTE_FALSE;
        }
        char const *var = token->value.string;
        _xte_ast_list_append(loop_block, token);
        in_context->stream->children[in_context->line_offset-1] = NULL;
        
        if (in_context->line_offset > in_context->line_end)
        {
            ERROR_SYNTAX("Expected \"=\" after \"repeat with %s\".", var, NULL);
            return XTE_FALSE;
        }
        token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
        if ((!token) || (token->type != XTE_AST_OPERATOR) || (token->value.op != XTE_AST_OP_EQUAL))
        {
            ERROR_SYNTAX("Expected \"=\" after \"repeat with %s\".", var, NULL);
            return XTE_FALSE;
        }
        
        XTEAST *loop_start = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(loop_start != NULL);
        _xte_ast_list_append(loop_block, loop_start);
        
        if (in_context->line_offset > in_context->line_end)
        {
            ERROR_SYNTAX("Expected integer but found end of line.", NULL, NULL);
            return XTE_FALSE;
        }
        for (; in_context->line_offset <= in_context->line_end; in_context->line_offset++)
        {
            token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
            
            if (token && (token->type == XTE_AST_WORD) && (_xte_compare_cstr(token->value.string, "to") == 0)) break;
            if (token && (token->type == XTE_AST_WORD) && (_xte_compare_cstr(token->value.string, "down") == 0)) break;
            
            _xte_ast_list_append(loop_start, token);
            in_context->stream->children[in_context->line_offset] = NULL;
        }
        if (!_xte_parse_expression(in_context->engine, loop_start, in_context->line_source)) return XTE_FALSE;
        
        if (in_context->line_offset > in_context->line_end)
        {
            ERROR_SYNTAX("Expected \"to\" or \"down to\" but found end of line.", NULL, NULL);
            return XTE_FALSE;
        }
        token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
        if (token && (token->type == XTE_AST_WORD) && (_xte_compare_cstr(token->value.string, "down") == 0))
        {
            if (in_context->line_offset >= in_context->line_end)
            {
                ERROR_SYNTAX("Expected \"down to\" but found end of line.", NULL, NULL);
                return XTE_FALSE;
            }
            token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
            loop_block->value.loop = XTE_AST_LOOP_COUNT_DOWN;
        }
        if ((!token) || (token->type != XTE_AST_WORD) || (_xte_compare_cstr(token->value.string, "to") != 0))
        {
            ERROR_SYNTAX("Expected \"to\" or \"down to\" but found end of line.", NULL, NULL);
            return XTE_FALSE;
        }
        
        XTEAST *loop_end = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(loop_end != NULL);
        _xte_ast_list_append(loop_block, loop_end);
        
        if (in_context->line_offset > in_context->line_end)
        {
            ERROR_SYNTAX("Expected integer but found end of line.", NULL, NULL);
            return XTE_FALSE;
        }
        for (; in_context->line_offset <= in_context->line_end; in_context->line_offset++)
        {
            token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
            _xte_ast_list_append(loop_end, token);
            in_context->stream->children[in_context->line_offset] = NULL;
        }
        if (!_xte_parse_expression(in_context->engine, loop_end, in_context->line_source)) return XTE_FALSE;
        
        return XTE_TRUE;
    }
    else if (token)
    {
        /* [for] <number> [times] */
        loop_block->value.loop = XTE_AST_LOOP_NUMBER;
        in_context->line_offset--;
        
        if (token && (token->type == XTE_AST_WORD) && (_xte_compare_cstr(token->value.string, "for") == 0))
            token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
        
        token = _xte_ast_list_child(in_context->stream, in_context->line_end);
        if (token && (token->type == XTE_AST_WORD) && (_xte_compare_cstr(token->value.string, "times") == 0))
            in_context->line_end--;
        
        XTEAST *loop_count = _xte_ast_create(in_context->engine, XTE_AST_LIST);
        assert(loop_count != NULL);
        _xte_ast_list_append(loop_block, loop_count);
        
        for (; in_context->line_offset <= in_context->line_end; in_context->line_offset++)
        {
            XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset);
            _xte_ast_list_append(loop_count, token);
            in_context->stream->children[in_context->line_offset] = NULL;
        }
        if (loop_count->children_count == 0)
        {
            ERROR_SYNTAX("Expected integer here.", NULL, NULL);
            return XTE_FALSE;
        }
        if (!_xte_parse_expression(in_context->engine, loop_count, in_context->line_source)) return XTE_FALSE;
        
        return XTE_TRUE;
    }
    
    /* shouldn't get here */
    _xte_raise_void(in_context->engine, XTE_ERROR_INTERNAL, "Internal error on line %ld of %s\n", __LINE__, __FILE__);
    return XTE_FALSE;
}


/*
 *  _xte_parse_hdlr_line
 *  ---------------------------------------------------------------------------------------------
 *  Invoked by _xte_parse_handler() for each actual line of the handler being parsed.
 *  Analyses the line's token stream, in_context->stream from the beginning of the line,
 *  in_context->line_start until the end, in_context->line_end.
 *
 *  The results of the analysis are an abstract syntax tree, in_context->handler.
 *
 *  Because more than one function is used to perform the analysis, an intermediate line_offset
 *  is also used to keep track of where we are in the token stream.
 *
 *  If there is an error during parsing of this line, returns XTE_FALSE.  If parsing the line
 *  raises no error, returns XTE_TRUE.  Normal reporting of syntax errors applies.
 */
static int _xte_parse_hdlr_line(struct ParseContext *in_context)
{
    assert(in_context != NULL);
    assert(IS_XTE(in_context->engine));
    assert(in_context->stream != NULL);
    assert((in_context->stack_ptr >= 0) && (in_context->stack_ptr < XTALK_LIMIT_MAX_NESTED_BLOCKS));
    
    /*printf("Parse handler line %d%s\n", in_context->line_number, (in_context->line_checkpoint ?  " (CHECKPOINT)" : ""));
    for (int i = 0; i < in_context->line_length; i++)
        _xte_ast_debug( in_context->stream->children[i + in_context->line_start] );*/
    
    /* shortcut if the line is empty */
    if (in_context->line_length == 0)
    {
        /* an empty line disallows an else to appear after a single-line then,
         as part of that condition, though it may still occur as normal as
         part of a multi-line if..then condition block */
        if (in_context->stack[in_context->stack_ptr].state == STATE_IN_BLOCK_MAY_ELSE)
            in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
        
        return XTE_TRUE;
    }
    
    in_context->line_offset = in_context->line_start;
    
    /* grab the first token */
    XTEAST *kw = _xte_ast_list_child(in_context->stream, in_context->line_start);
    
    /* handle "then" at start of line */
    if (in_context->stack[in_context->stack_ptr].state == STATE_COND_EXP_THEN)
    {
        if ((!kw) || (kw->type != XTE_AST_KEYWORD) || (kw->value.keyword != XTE_AST_KW_THEN))
        {
            ERROR_SYNTAX("Expecting \"then\" here.", NULL, NULL);
            return XTE_FALSE;
        }
        in_context->line_offset++;
        
        in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
        if (!_xte_parse_block_then(in_context, XTE_FALSE, in_context->line_checkpoint)) return XTE_FALSE;
        
        if (in_context->line_offset > in_context->line_end) return XTE_TRUE;
        
        kw = _xte_ast_list_child(in_context->stream, in_context->line_offset);
    }

    /* handle "else" on it's own line;
     following a single-line then */
    if (in_context->stack[in_context->stack_ptr].state == STATE_IN_BLOCK_MAY_ELSE)
    {
        if ( kw && (kw->type == XTE_AST_KEYWORD) && (kw->value.keyword == XTE_AST_KW_ELSE) )
        {
            in_context->line_offset++;
            
            in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
            if (!_xte_parse_block_else(in_context, XTE_FALSE, in_context->line_checkpoint)) return XTE_FALSE;
        }
        else in_context->stack[in_context->stack_ptr].state = STATE_IN_BLOCK;
        
        if (in_context->line_offset > in_context->line_end) return XTE_TRUE;
        
        kw = _xte_ast_list_child(in_context->stream, in_context->line_offset);
    }
    
    /* handle normal block keyword prefixes */
    if (in_context->stack[in_context->stack_ptr].state == STATE_IN_BLOCK)
    {
        if (kw->type == XTE_AST_KEYWORD)
        {
            in_context->line_offset++;
            switch (kw->value.keyword)
            {
                case XTE_AST_KW_REPEAT:
                {
                    return _xte_parse_repeat(in_context);
                }
                case XTE_AST_KW_END:
                    switch (in_context->stack[in_context->stack_ptr].type)
                    {
                        case BLOCK_LOOP:
                        {
                            if (in_context->line_offset > in_context->line_end)
                            {
                                ERROR_SYNTAX("Expected \"end repeat\" but found end of line.", NULL, NULL);
                                return XTE_FALSE;
                            }
                            XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
                            if ((!token) || (token->type != XTE_AST_KEYWORD) || (token->value.keyword != XTE_AST_KW_REPEAT))
                            {
                                ERROR_SYNTAX("Expected \"end repeat\" but found \"end %s\".", _lexer_term_desc(token), NULL);
                                return XTE_FALSE;
                            }
                            if (in_context->line_offset > in_context->line_end)
                            {
                                in_context->stack_ptr--;
                                return XTE_TRUE;
                            }
                            ERROR_SYNTAX("Expected end of line after \"end repeat\".", NULL, NULL);
                            return XTE_FALSE;
                        }
                        case BLOCK_CONDITION:
                        {
                            if (in_context->line_offset > in_context->line_end)
                            {
                                ERROR_SYNTAX("Expected \"end if\" but found end of line.", NULL, NULL);
                                return XTE_FALSE;
                            }
                            XTEAST *token = _xte_ast_list_child(in_context->stream, in_context->line_offset++);
                            if ((!token) || (token->type != XTE_AST_KEYWORD) || (token->value.keyword != XTE_AST_KW_IF))
                            {
                                ERROR_SYNTAX("Expected \"end if\" but found \"end %s\".", _lexer_term_desc(token), NULL);
                                return XTE_FALSE;
                            }
                            if (in_context->line_offset > in_context->line_end)
                            {
                                in_context->stack_ptr--;
                                return XTE_TRUE;
                            }
                            ERROR_SYNTAX("Expected end of line after \"end if\".", NULL, NULL);
                            return XTE_FALSE;
                        }
                        default: break;
                    }
                    ERROR_SYNTAX("Expected \"end %s\" here.", in_context->handler->value.handler.name, NULL);
                    return XTE_FALSE;
                    
                case XTE_AST_KW_IF:
                {
                    /* build a condition node */
                    XTEAST *cond = _xte_ast_create(in_context->engine, XTE_AST_CONDITION);
                    assert(cond != NULL);
                    _register_chkpt(in_context, cond, in_context->line_checkpoint);
                    in_context->stack[in_context->stack_ptr].last_ctrl = cond;
                    _xte_ast_list_append(in_context->stack[in_context->stack_ptr].block, cond);
                    in_context->stack[in_context->stack_ptr].got_default = XTE_FALSE;
                    
                    if (!_xte_parse_block_if(in_context, XTE_FALSE, XTE_FALSE)) return XTE_FALSE;
                    
                    return XTE_TRUE;
                }
                    
                case XTE_AST_KW_ELSE:
                {
                    /* escape from current block */
                    if (in_context->stack_ptr <= 0)
                    {
                        ERROR_SYNTAX("\"else\" not allowed here.", NULL, NULL);
                        return XTE_FALSE;
                    }
                    if (in_context->stack[in_context->stack_ptr].type != BLOCK_CONDITION)
                    {
                        ERROR_SYNTAX("\"else\" not allowed here.", NULL, NULL);
                        return XTE_FALSE;
                    }
                    in_context->stack_ptr--;
                    
                    if (!_xte_parse_block_else(in_context, XTE_FALSE, XTE_FALSE)) return XTE_FALSE;
                    
                    return XTE_TRUE;
                }
                
                default:
                    break;
            }
        }
    }
    
    in_context->line_offset = in_context->line_start;
    in_context->stmt_container = in_context->stack[in_context->stack_ptr].block;
    return _xte_parse_stmt(in_context);
}



/*
 *  _xte_line_has_checkpoint
 *  ---------------------------------------------------------------------------------------------
 *  Returns XTE_TRUE if the list of checkpoints <in_checkpoints> & <in_checkpoint_count> 
 *  contains the <in_line_number>.
 *
 *  <in_line_number> and <in_prev_line> are actual hard line numbers.  Due to line-continuation,
 *  it is not possible to do a straight-forward comparison.  Instead the function checks to see
 *  if any of the checkpoints occurs in the range of lines between the previous logical line
 *  and the current logical line (which may be more than one actual line difference.)
 *
 *  <in_checkpoint_offset> is a negative offset that the checkpoint list is adjusted by prior
 *  to comparison.  The supplied checkpoint list is for an entire script.  The supplied line
 *  numbers are handler specific.  This offset should be the number of lines to the first line
 *  of the handler currently being processed.
 *  This offset is also an actual line number, not a logical line number.
 */
static int _xte_line_has_checkpoint(int const in_checkpoints[], int in_checkpoint_count, int in_checkpoint_offset,
                             int in_line_number, int in_prev_line)
{
    for (int i = 0; i < in_checkpoint_count; i++)
    {
        int adj_chkpt_line = in_checkpoints[i] + in_checkpoint_offset - 1;
        if ((adj_chkpt_line >= in_prev_line) && (adj_chkpt_line < in_line_number)) return in_prev_line - in_checkpoint_offset + 1;
    }
    return 0;
}



/*********
 Entry Points
 */

/*
 *  _xte_parse_handler
 *  ---------------------------------------------------------------------------------------------
 *  Initiates the parsing of a command/function handler.  The token stream provided should be
 *  obtained from _xte_lex(), and the source for that from _xte_find_handler() below.
 *
 *  The checkpoint list if provided should be hard line numbers (first line is 1, lines continued
 *  with the continuation character (¬) are distinct).
 *  The <in_checkpoint_offset> is a negative integer to subtract from the supplied checkpoint
 *  list so that those checkpoints that belong to the specific handler line up with the correct
 *  lines in the token stream.
 *
 *  Iterates over the lines of the handler, delegating the parsing to the appropriate subroutine.
 *
 *  Checkpoints are determined for a given line using _xte_line_has_checkpoint, and applied to the 
 *  parser output by setting the XTE_AST_FLAG_IS_CHECKPOINT flag on the root AST node of the 
 *  relevant statement or control structure.
 *
 *  ! Modifies the input token stream, replacing it with the resulting abstract syntax tree.
 *
 *  The result is XTE_TRUE if the parse was successful (no syntax error.)
 *  Syntax errors are handled as per protocol, see xtalk_internal.c notes for details.
 */
int _xte_parse_handler(XTE *in_engine, XTEAST *in_stream, int const in_checkpoints[], int in_checkpoint_count, int in_checkpoint_offset)
{
    assert(IS_XTE(in_engine));
    assert(in_stream != NULL);
    assert(in_checkpoint_count >= 0);
    assert(IS_LINE_NUMBER(in_checkpoint_count) || (in_checkpoint_count == 0));
    assert(in_checkpoint_offset <= 0);
    
    XTEAST *kw;
    struct ParseContext context;
    struct ParseContext *in_context = &context; /* used for error reporting macros */
    
    /* prepare result */
    XTEAST *result = _xte_ast_create(in_engine, XTE_AST_HANDLER);
    assert(result != NULL);
    
    /* configure parsing context for state tracking
     and error handling */
    context.stream = in_stream;
    context.engine = in_engine;
    context.handler = result;
    context.line_number = 1;
    context.line_source = 1 - in_checkpoint_offset;
    result->source_line = context.line_source;
    
    context.stack_ptr = 0;
    context.stack[0].type = BLOCK_HANDLER;
    context.stack[0].state = STATE_IN_BLOCK;
    context.stack[0].block = result;
    
    /* assume handler begins with "on" or "function";
     this is a given since _xte_find_handler() will only
     find handlers that start in this way */
    kw = _xte_ast_list_child(in_stream, 0);
    assert(kw && (kw->type == XTE_AST_KEYWORD));
    assert((kw->value.keyword == XTE_AST_KW_ON) || (kw->value.keyword == XTE_AST_KW_FUNCTION));
    
    /* determine handler type */
    result->value.handler.is_func = (kw->value.keyword == XTE_AST_KW_FUNCTION);
    _xte_ast_destroy(_xte_ast_list_remove(in_stream, 0));
    
    /* get handler name;
     again we assume it is present, since _xte_find_handler()
     will only find handlers with a name */
    kw = _xte_ast_list_child(in_stream, 0);
    assert(kw && (kw->type == XTE_AST_WORD));
    assert(kw->value.string != NULL);
    result->value.handler.name = _xte_clone_cstr(in_engine, kw->value.string);
    _xte_ast_destroy(_xte_ast_list_remove(in_stream, 0));
    
    /* parse parameter names (if any) */
    XTEAST *param_names = _xte_ast_create(in_engine, XTE_AST_PARAM_NAMES);
    param_names->source_line = result->source_line;
    assert(param_names != NULL);
    _xte_ast_list_append(result, param_names);
    
    kw = _xte_ast_list_child(in_stream, 0);
    while (kw && (kw->type == XTE_AST_WORD))
    {
        _xte_ast_list_append(param_names, kw);
        _xte_ast_list_remove(in_stream, 0);
        
        /* look for another parameter */
        kw = _xte_ast_list_child(in_stream, 0);
        if ((!kw) || (kw->type != XTE_AST_COMMA)) break;
        _xte_ast_destroy(_xte_ast_list_remove(in_stream, 0));
        
        /* expect comma to be followed by another parameter name */
        kw = _xte_ast_list_child(in_stream, 0);
        if ((!kw) || (kw->type != XTE_AST_WORD))
        {
            _xte_ast_destroy(result);
            ERROR_SYNTAX("Expected parameter name after \",\".", NULL, NULL);
            return XTE_FALSE;
        }
    }
    
    /* expect first newline */
    kw = _xte_ast_list_child(in_stream, 0);
    if ((!kw) || (kw->type != XTE_AST_NEWLINE))
    {
        if (param_names->children_count > 0)
            ERROR_SYNTAX("Expected end of line after parameters.", NULL, NULL);
        else
            ERROR_SYNTAX("Expected parameters or end of line after \"%s\".", result->value.handler.name, NULL);
        _xte_ast_destroy(result);
        return XTE_FALSE;
    }
    int source_line_prev = kw->source_line;
    _xte_ast_destroy(_xte_ast_list_remove(in_stream, 0));
    
    /* check for and remove end */
    kw = _xte_ast_list_child(in_stream, in_stream->children_count-1);
    if ( (!kw) || (kw->type != XTE_AST_WORD) ||
        (!xte_cstrings_equal(kw->value.string, result->value.handler.name)) )
    {
        ERROR_SYNTAX("Expected \"end %s\".", result->value.handler.name, NULL);
        _xte_ast_destroy(result);
        return XTE_FALSE;
    }
    _xte_ast_destroy(_xte_ast_list_remove(in_stream, in_stream->children_count-1));
    kw = _xte_ast_list_child(in_stream, in_stream->children_count-1);
    if ((!kw) || (kw->type != XTE_AST_KEYWORD) || (kw->value.keyword != XTE_AST_KW_END))
    {
        _xte_ast_destroy(result);
        ERROR_SYNTAX("Expected \"end\" after \"on\".", NULL, NULL);
        return XTE_FALSE;
    }
    _xte_ast_destroy(_xte_ast_list_remove(in_stream, in_stream->children_count-1));
    
    /* remove the last newline;
     if it's not there, the handler should be empty */
    kw = _xte_ast_list_child(in_stream, in_stream->children_count-1);
    if ( (!kw) || ((kw->type != XTE_AST_NEWLINE) && (in_stream->children_count > 1)) )
    {
        
        ERROR_SYNTAX("\"end %s\" should be at the beginning of the line.", result->value.handler.name, NULL);
        _xte_ast_destroy(result);
        return XTE_FALSE;
    }
    _xte_ast_destroy(_xte_ast_list_remove(in_stream, in_stream->children_count-1));
    
    /* parse each line */
    int line_number = 2;
    int line_start = 0;
    int source_line = source_line_prev;
    for (int token_index = 0; token_index <= in_stream->children_count; token_index++)
    {
        if ((token_index == in_stream->children_count) || (in_stream->children[token_index]->type == XTE_AST_NEWLINE))
        {
            /* define the range of this line */
            source_line_prev = source_line;
            if (token_index == in_stream->children_count) source_line = XTALK_LIMIT_MAX_SOURCE_LINES;
            else source_line = in_stream->children[token_index]->source_line;
            
            context.line_start = line_start;
            context.line_length = token_index - line_start;
            if (token_index - line_start <= 0) context.line_end = context.line_start;
            else context.line_end = token_index - 1;
            context.line_number = line_number;
            context.line_source = source_line_prev - in_checkpoint_offset + 1;
            
            /* check if this line has a checkpoint */
            context.line_checkpoint = _xte_line_has_checkpoint(in_checkpoints, in_checkpoint_count,
                                                               in_checkpoint_offset, source_line, source_line_prev);
            
            /* attempt to parse this handler line */
            if (!_xte_parse_hdlr_line(&context))
            {
                _xte_ast_destroy(result);
                return XTE_FALSE; /* an appropriate syntax error has already been reported by the subroutine */
            }
            
            /* continue search for next line */
            line_start = token_index+1;
            line_number++;
            if (token_index >= in_stream->children_count) break;
        }
    }
    
    /* check exit of parsing at correct level */
    if (context.stack_ptr != 0)
    {
        assert(context.stack_ptr > 0);
        _xte_ast_destroy(result);
        switch (in_context->stack[in_context->stack_ptr].type)
        {
            case BLOCK_CONDITION:
                ERROR_SYNTAX("Expected \"end if\" after \"if\".", NULL, NULL);
                break;
            case BLOCK_LOOP:
                ERROR_SYNTAX("Expected \"end repeat\" after \"repeat\".", NULL, NULL);
                break;
            default:
                break;
        }
        return XTE_FALSE;
    }

    /* swap result with input stream and cleanup input stream */
    _xte_ast_swap_ptrs(result, in_stream);
    _xte_ast_destroy(result);
    return XTE_TRUE;
}



/*
 *  _xte_find_handler
 *  ---------------------------------------------------------------------------------------------
 *  Searches the supplied <in_script> for the specified handler, <in_name_utf8>.
 *
 *  If <is_func> is XTE_TRUE, searches for a function handler.
 *
 *  Returns the content of the matching handler, including the "on" and "end" lines, and sets
 *  <out_handler_line_begin> to the line number of the "on" line.  Line numbers begin at 1.
 *  The line number is based on actual lines, not logical lines (broken with the 
 *  line-continuation character.
 *
 *  If an appropriate "end" line is not found, the result will be the script from the "on" line
 *  onwards, to the end of the script.  This is deliberate and allows the parser to detect and
 *  raise an appropriate error condition.
 * 
 *  If the handler is not found, returns NULL and <out_handler_line_begin> is not modified.
 *
 *  This function may report a syntax error in the event the script length exceeds specified
 *  limits (_limits.h.)
 */
char* _xte_find_handler(XTE *in_engine, char const *in_script, int is_func, char const *in_name_utf8, int *out_handler_line_begin)
{
    assert(IS_XTE(in_engine));
    assert(in_script != NULL);
    assert(IS_BOOL(is_func));
    assert(in_name_utf8 != NULL);
    assert(in_name_utf8[0] != 0);
    assert(out_handler_line_begin != NULL);
    
    /* iterate over script lines; find start and end of matching handler (if present) */
    char const *kw_begin = (is_func ? "function" : "on");
    long kw_begin_bytes = strlen(kw_begin);
    long name_bytes = strlen(in_name_utf8);
    
    char const *result_begin = NULL;
    char const *result_end = NULL;
    int result_line = 0;
    
    char const *chr = in_script;
    long script_line_number = 1;
    char const *line_start = in_script;
    for (;;)
    {
        if ((*chr == 10) || (*chr == 13) || (*chr == 0))
        {
            /* identify the range of the line */
            char const *line_end = chr-1;
            long line_bytes = line_end - line_start + 1;
            //printf("LINE: (%ld) %s\n", line_bytes, line_start);
            
            /* skip leading whitespace */
            int offset;
            for (offset = 0; offset < line_bytes; offset++)
                if ( !isspace(*(line_start + offset)) ) break;
            
            /* does line begin with an appropriate keyword? */
            if (!result_begin)
            {
                if ( (line_bytes > kw_begin_bytes + offset) &&
                    (memcmp(kw_begin, line_start + offset, kw_begin_bytes) == 0) )
                {
                    //printf("APPROPRIATE HANDLER TYPE START at LINE %ld\n", script_line_number);
                    
                    int name_start;
                    for (name_start = offset + (int)kw_begin_bytes; name_start < line_bytes; name_start++)
                        if ( !isspace(*(line_start + name_start)) ) break;
                    int name_end;
                    for (name_end = name_start; name_end < line_bytes; name_end++)
                        if ( isspace(*(line_start + name_end)) ) break;
                    
                    if (xte_cstrings_szd_equal(in_name_utf8, name_bytes, line_start + name_start, name_end - name_start))
                    {
                        //printf("Matching handler start on line %ld\n", script_line_number);
                        result_begin = line_start;
                        result_line = (int)script_line_number;
                    }
                }
            }
            else
            {
                if ( (line_bytes > 3 + offset) &&
                    (memcmp("end", line_start + offset, 3) == 0) )
                {
                    int name_start;
                    for (name_start = offset + 3; name_start < line_bytes; name_start++)
                        if ( !isspace(*(line_start + name_start)) ) break;
                    int name_end;
                    for (name_end = name_start; name_end < line_bytes; name_end++)
                        if ( isspace(*(line_start + name_end)) ) break;
                    
                    if (xte_cstrings_szd_equal(in_name_utf8, name_bytes, line_start + name_start, name_end - name_start))
                    {
                        result_end = line_start + line_bytes;
                        break;
                    }
                }
            }
            
            /* go to next line */
            line_start = chr+1;
            script_line_number++;
            if (script_line_number > XTALK_LIMIT_MAX_SOURCE_LINES)
            {
                _xte_error_syntax(in_engine, 1, "Script is too long.", NULL, NULL, NULL);
                return NULL;
            }
            if (*chr == 0) break;
        }
        chr++;
    }
    
    if (!result_begin) return NULL;
    
    /* if we found the start of a handler;
     we rely on the handler parser (above) to detect the missing end
     and output an approprite (hopefully) error message! */
    if (!result_end) result_end = in_script + strlen(in_script);
    
    /* prepare the resulting handler */
    char *result = malloc(result_end - result_begin + 1);
    if (!result) return _xte_raise_null(in_engine, XTE_ERROR_MEMORY, NULL);
    
    memcpy(result, result_begin, result_end - result_begin);
    result[result_end - result_begin] = 0;
    
    *out_handler_line_begin = result_line;
    
    return result;
}







