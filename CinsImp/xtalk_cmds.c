/*
 
 xTalk Engine Command Parsing Unit
 xtalk_cmds.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Responsible for parsing command calls; both types:
 
 -  specific grammar; uses a terminology dictionary within the engine to identify valid commands 
    and parse the command syntax against the grammar
 
    (grammars are expressed in BNF form and parsed using _bnf.c.)
 
 -  generic; call consists of command name followed by an optional comma-delimited parameter list
 
 Both call types result in the same form of abstract syntax tree for a successful parse,
 an XTE_AST_COMMAND node with either .ptr or .named specified (depending on whether the command
 is known in advance) and a child XTE_AST_EXPRESSION for each parameter (if any.)
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


#define _MAX_GRAMMAR_COMPLEXITY 50


/*********
 Types
 */

struct Param
{
    char *name;
    int offset;
    int length;
    char *value;
};


struct Frame
{
    int old_stmt_position;
    int old_param_list_size;
    char *old_set_name;// don't free
};


struct CmdMatchContext
{
    int stack_index;
    int stack_size;
    struct Frame *stack;
    
    XTEAST *stmt;
    int stmt_position;
    
    int param_list_size;
    struct Param *param_list;
    
    char *set_name;// don't free
    
    XTE *engine;
    long line_source;
};


struct ErrorContext
{
    XTE *engine;
    long line_source;
};


static struct XTECmdPrefix* _cmd_prefix(XTE *in_engine, const char *in_prefix_word, int in_create_new);



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
 Parsing
 */

/* these two functions will enable tracking and disposal of parameters from a stack
 based on match progression */
static void _match_element_begin(struct CmdMatchContext *in_context, XTECmdPattern *in_pattern)
{
    /* push a stack frame */
    struct Frame *frame = &(in_context->stack[++in_context->stack_index]);
    
    /* record the current statement position */
    frame->old_stmt_position = in_context->stmt_position;
    
    /* record the current parameter list size */
    frame->old_param_list_size = in_context->param_list_size;
    
    /* record the current set name */
    frame->old_set_name = in_context->set_name;
}


static void _match_element_end(struct CmdMatchContext *in_context, int in_success)
{
    /* pop a stack frame */
    struct Frame *frame = &(in_context->stack[in_context->stack_index--]);
    
    /* was the match unsuccessful? */
    if (!in_success)
    {
        /* restore previous statement position (prior to beginning failed match) */
        in_context->stmt_position = frame->old_stmt_position;
        
        /* restore matched parameter list (prior to beginning failed match) */
        for (int i = frame->old_param_list_size; i < in_context->param_list_size; i++)
        {
            if (in_context->param_list[i].name) free(in_context->param_list[i].name);
        }
        in_context->param_list_size = frame->old_param_list_size;
    }
    
    /* restore previous set name */
    in_context->set_name = frame->old_set_name;
}


static void _matched_parameter(struct CmdMatchContext *in_context, int in_offset, int in_length, char *in_name, char *in_value)
{
    struct Param *new_param_list = realloc(in_context->param_list, sizeof(struct Param) * (in_context->param_list_size + 1));
    if (!new_param_list) return _xte_panic_void(in_context->engine, XTE_ERROR_MEMORY, NULL);
    in_context->param_list = new_param_list;
    struct Param *param = &(in_context->param_list[in_context->param_list_size++]);
    param->name = _xte_clone_cstr(in_context->engine, in_name);
    param->offset = in_offset;
    param->length = in_length;
    param->value = _xte_clone_cstr(in_context->engine, in_value);
}


static void _matched_set(struct CmdMatchContext *in_context, char *in_value)
{
    if (!in_value || !in_context->set_name) return;
    _matched_parameter(in_context, -1, -1, in_context->set_name, in_value);
}


static void _set_name(struct CmdMatchContext *in_context, char *in_name)
{
    in_context->set_name = in_name;
}


static char const* _word_or_keyword(XTEAST *in_token)
{
    if (!in_token) return NULL;
    if (in_token && (in_token->type == XTE_AST_WORD)) return in_token->value.string;
    if (in_token && (in_token->type == XTE_AST_OF)) return "of";
    if (in_token && (in_token->type == XTE_AST_OPERATOR) && (in_token->value.op == XTE_AST_OP_OR))
        return "or";
    if (in_token && (in_token->type == XTE_AST_KEYWORD))
    {
        switch (in_token->value.keyword)
        {
            case XTE_AST_KW_NEXT:
                return "next";
            case XTE_AST_KW_END:
                return "end";
            default:
                break;
        }
    }
    return NULL;
}


/* this function assumes a short list of patterns has already been identified, and is only
 interested in trying to match to the one specified pattern */
static int _stream_matches(struct CmdMatchContext *in_context, XTECmdPattern *in_pattern)
{
    int matched = XTE_FALSE;
    _match_element_begin(in_context, in_pattern);
    
    switch (in_pattern->type)
    {
        case XTE_CMDPAT_SET_REQ:
            /* an element of this set must match */
            matched = XTE_FALSE;
            _set_name(in_context, in_pattern->payload.name);
            for (int i = 0; i < in_pattern->child_count; i++)
            {
                if (_stream_matches(in_context, in_pattern->children[i]))
                {
                    matched = XTE_TRUE;
                    break;
                }
            }
            break;
            
        case XTE_CMDPAT_SET_OPT:
            /* it doesn't matter if this matches anything or not,
             but we must attempt to match one of the set elements.
             if a match is found, don't try and match another element */
            matched = XTE_TRUE;
            _set_name(in_context, in_pattern->payload.name);
            for (int i = 0; i < in_pattern->child_count; i++)
            {
                if (_stream_matches(in_context, in_pattern->children[i]))
                    break;
            }
            break;
            
        case XTE_CMDPAT_LIST:
            /* check if every term of the set matches */
            matched = XTE_TRUE;
            for (int i = 0; i < in_pattern->child_count; i++)
            {
                if (!_stream_matches(in_context, in_pattern->children[i]))
                {
                    matched = XTE_FALSE;
                    break;
                }
            }
            /* save the match as a named subexpression to the context */
            if (matched)
                _matched_set(in_context, in_pattern->payload.name);
            break;
            
        case XTE_CMDPAT_LITERAL:
        {
            /* check if literal matches stream */
            XTEAST *word = _xte_ast_list_child(in_context->stmt, in_context->stmt_position);
            char const *word_text = _word_or_keyword(word);
            if (word_text && (_xte_compare_cstr(word_text, in_pattern->payload.literal) == 0))
            {
                in_context->stmt_position++;
                matched = XTE_TRUE;
            }
            else
                matched = XTE_FALSE;
            break;
        }
            
        case XTE_CMDPAT_PARAM:
        {
            /* find the end of the stream or a stop word (whichever comes first) */
            int end_offset;
            for (end_offset = in_context->stmt_position;
                 end_offset < in_context->stmt->children_count;
                 end_offset++)
            {
                XTEAST *word = _xte_ast_list_child(in_context->stmt, end_offset);
                char const *word_text = _word_or_keyword(word);
                char **stop_words = in_pattern->stop_words;
                int found_stop_word = XTE_FALSE;
                if ((stop_words) && word_text)
                {
                    while (*stop_words)
                    {
                        if (_xte_compare_cstr(word_text, *stop_words) == 0)
                        {
                            found_stop_word = XTE_TRUE;
                            break;
                        }
                        stop_words++;
                    }
                }
                if (found_stop_word) break;
            }
            
            /* check for a match (at least one node) */
            int match_length = end_offset - in_context->stmt_position;
            if (match_length == 0)
            {
                matched = XTE_FALSE;
                break;
            }
            matched = XTE_TRUE;
            
            /* save the matching nodes as a named subexpression to the context */
            _matched_parameter(in_context, in_context->stmt_position, match_length, in_pattern->payload.name, NULL);
            in_context->stmt_position += match_length;
            break;
        }
    }
    
    _match_element_end(in_context, matched);
    return matched;
}


static void _xte_context_cleanup(struct CmdMatchContext *io_context)
{
    if (io_context->stack) free(io_context->stack);
    for (int i = 0; i < io_context->param_list_size; i++)
    {
        struct Param *the_param = io_context->param_list + i;
        if (the_param->name) free(the_param->name);
        if (the_param->value) free(the_param->value);
    }
    if (io_context->param_list) free(io_context->param_list);
}


/*
 *  _xte_cmd_stmt_matches
 *  ---------------------------------------------------------------------------------------------
 *  Supervises the process of matching the given <in_pattern> with the given token stream
 *  <in_stmt>.  If successful, returns XTE_TRUE and provides an XTE_AST_LIST of command
 *  parameters in the appropriate sequence.
 *
 *  ! Mangles the input token stream <in_stmt> if the match is successful.
 *
 *  If unsuccessful, makes no changes to the inputs and raises no errors.  Returns XTE_FALSE.
 */
static int _xte_cmd_stmt_matches(XTE *in_engine, XTEAST *in_stmt, XTECmdPattern *in_pattern, XTEAST **out_params)
{
    assert(IS_XTE(in_engine));
    assert(in_stmt != NULL);
    assert(in_pattern != NULL);
    assert(out_params != NULL);
    
    /* prepare to match statement against command pattern */
    struct CmdMatchContext context;
    context.engine = in_engine;
    context.stack_size = _MAX_GRAMMAR_COMPLEXITY;
    context.stack_index = -1;
    context.stack = malloc(sizeof(struct Frame) * context.stack_size);
    if (!context.stack) return _xte_panic_int(in_engine, XTE_ERROR_MEMORY, NULL);
    context.param_list_size = 0;
    context.param_list = NULL;
    context.stmt = in_stmt;
    context.stmt_position = 0;
    context.set_name = NULL;
    
    /* perform match analysis */
    int matches = _stream_matches(&context, in_pattern);
    if (context.stmt_position != context.stmt->children_count)
        matches = XTE_FALSE;
    if (!matches)
    {
        _xte_context_cleanup(&context);
        return XTE_FALSE;
    }
    
    /* build parameter list */
    XTEAST *params = *out_params = _xte_ast_create(in_engine, XTE_AST_LIST);
    for (int i = 0; i < context.param_list_size; i++)
    {
        XTEAST *param = _xte_ast_create(in_engine, XTE_AST_LIST);
        _xte_ast_list_append(params, param);
        
        param->value.string = _xte_clone_cstr(in_engine, context.param_list[i].name);
        
        if ((context.param_list[i].offset < 0) && (context.param_list[i].length < 0))
            _xte_ast_list_append(param, _xte_ast_create_word(in_engine, context.param_list[i].value));
        else if ((context.param_list[i].offset >= 0) && (context.param_list[i].length > 0))
        {
            for (int j = 0; j < context.param_list[i].length; j++)
            {
                int stmt_node_index = context.param_list[i].offset + j;
                _xte_ast_list_append(param, _xte_ast_list_child(in_stmt, stmt_node_index));
                in_stmt->children[stmt_node_index] = NULL;
            }
        }
    }
    
    _xte_context_cleanup(&context);
    return XTE_TRUE;
}


/*
 *  _append_cmd_params
 *  ---------------------------------------------------------------------------------------------
 *  Moves the <in_supplied_params> items to their appropriate sequence within the <io_command>
 *  XTE_AST_COMMAND send node, based on what the grammar for the command prescribes.  Each 
 *  parameter expression is parsed.
 *
 *  If a parameter wasn't specified by the front of the grammar, but is prescribed, it's set to 
 *  NULL so the appropriate built-in routine can detect and handle accordingly.
 *
 *  The result is XTE_TRUE if the movement was successful (no syntax error.)
 *  Syntax errors are handled as per protocol.
 */
static int _append_cmd_params(XTEAST *io_command, char *in_prescribed_params[], int in_prescribed_count, int in_prescribed_is_delayed[],
                              XTEAST *in_supplied_params, long in_source_line)
{
    assert(io_command != NULL);
    assert(in_supplied_params != NULL);
    
    for (int prescribed_index = 0; prescribed_index < in_prescribed_count; prescribed_index++)
    {
        int got_this_param = XTE_FALSE;
        for (int supplied_index = 0; supplied_index < in_supplied_params->children_count; supplied_index++)
        {
            XTEAST *supplied_param = in_supplied_params->children[supplied_index];
            if (!supplied_param) continue;
            if (supplied_param->type != XTE_AST_LIST) continue;
            if (strcmp(in_supplied_params->children[supplied_index]->value.string,
                       in_prescribed_params[prescribed_index]) == 0)
            {
                got_this_param = XTE_TRUE;
                if (!_xte_parse_expression(io_command->engine, in_supplied_params->children[supplied_index], in_source_line))
                    return XTE_FALSE;
                
                _xte_ast_list_append(io_command, in_supplied_params->children[supplied_index]);
                if (in_prescribed_is_delayed[prescribed_index])
                    in_supplied_params->children[supplied_index]->flags |= XTE_AST_FLAG_DELAYED_EVAL;
                in_supplied_params->children[supplied_index] = NULL;
            }
        }
        if (!got_this_param)
            _xte_ast_list_append(io_command, NULL);
    }
    return XTE_TRUE;
}


/*
 *  _xte_parse_generic_command
 *  ---------------------------------------------------------------------------------------------
 *  Attempts to parse a generic command call of the form: command [param1 [,param2 [, paramN]]]
 *
 *  ! Modifies the input token stream, replacing it with the resulting abstract syntax tree.
 *
 *  The result is XTE_TRUE if the parse was successful (no syntax error.)
 *  Syntax errors are handled as per protocol.
 */
static int _xte_parse_generic_command(XTE *in_engine, XTEAST *in_stmt, long in_source_line)
{
    assert(in_engine != NULL);
    assert(in_stmt != NULL);
    assert(in_stmt->children_count > 0);
    assert(_xte_node_is_identifier(in_stmt->children[0]));
    
    /* build command message send */
    XTEAST *cmd = _xte_ast_create(in_engine, XTE_AST_COMMAND);
    assert(cmd != NULL);
    cmd->note = _xte_clone_cstr(in_engine, in_stmt->children[0]->value.string);
    cmd->value.command.ptr = NULL;
    cmd->value.command.named = _xte_clone_cstr(in_engine, in_stmt->children[0]->value.string);
    assert(cmd->value.command.named != NULL);
    
    /* build parameters (if any) */
    if (in_stmt->children_count > 1)
    {
        XTEAST *param = _xte_ast_create(in_engine, XTE_AST_LIST);
        assert(param != NULL);
        _xte_ast_list_append(cmd, param);
        for (int i = 1; i < in_stmt->children_count; i++)
        {
            XTEAST *token = _xte_ast_list_child(in_stmt, i);
            if (token && (token->type == XTE_AST_COMMA))
            {
                /* note: it's permitted syntax to have no tokens within each comma-delimited parameter */
                param = _xte_ast_create(in_engine, XTE_AST_LIST);
                _xte_ast_list_append(cmd, param);
                continue;
            }
            _xte_ast_list_append(param, token);
            in_stmt->children[i] = NULL;
        }
    }
    
    /* cleanup and swap input with result */
    _xte_ast_swap_ptrs(in_stmt, cmd);
    _xte_ast_destroy(cmd);
    
    /* parse subexpressions */
    for (int i = 0; i < in_stmt->children_count; i++)
    {
        if (!_xte_parse_expression(in_engine, in_stmt->children[i], in_source_line)) return XTE_FALSE;
    }
    
    return XTE_TRUE;
}


/*
 *  _xte_find_and_parse_command
 *  ---------------------------------------------------------------------------------------------
 *  Initiates the parsing of a command call.  The token stream provided should be as obtained
 *  from _xte_lex().
 *
 *  ! Modifies the input token stream, replacing it with the resulting abstract syntax tree.
 *
 *  The result is XTE_TRUE if the parse was successful (no syntax error.)
 *  Syntax errors are handled as per protocol, see xtalk_internal.c notes for details.
 */
static int _xte_find_and_parse_command(XTE *in_engine, XTEAST *in_stmt, long in_source_line)
{
    assert(IS_XTE(in_engine));
    assert(in_stmt != NULL);
    assert(in_stmt->children_count > 0);
    assert(in_stmt->children[0] != NULL);
    
    /* setup a context for error handling purposes */
    struct ErrorContext error_context = {in_engine, in_source_line};
    struct ErrorContext *in_context = &error_context;

    /* do some basic syntax checks */
    if (!_xte_node_is_identifier(in_stmt->children[0]))
    {
        ERROR_SYNTAX("Can't understand \"%s\".", _lexer_term_desc(in_stmt->children[0]), NULL);
        return XTE_FALSE;
    }
    
    /* search for matching command(s) with the same prefix */
    struct XTECmdPrefix *prefix = _cmd_prefix(in_engine, in_stmt->children[0]->value.string, XTE_FALSE);
    if (prefix)
    {
        /* iterate over syntax patterns beginning with the same prefix */
        for (int syn = 0; syn < prefix->command_count; syn++)
        {
            struct XTECmdInt *cmd = &(prefix->commands[syn]);
            
            /* attempt to match each syntax pattern beginning with the same prefix */
            XTEAST *params = NULL;
            if (_xte_cmd_stmt_matches(in_engine, in_stmt, cmd->pattern, &params))
            {
                /* found a match!
                 create a new abstract syntax tree to hold the parsed command */
                XTEAST *temp_tree = _xte_ast_create(in_engine, XTE_AST_COMMAND);
                assert(temp_tree != NULL);
                temp_tree->note = _xte_clone_cstr(in_engine, prefix->prefix_word);
                temp_tree->value.command.ptr = cmd->imp;
                temp_tree->value.command.named = _xte_clone_cstr(in_engine, prefix->prefix_word);
                /* eventually going to need to fill this with the command name, eg. prefix word,
                                                        also keep ptr, so that we know which grammar was triggered and can avoid issues
                                                        at the end of the message handling protocol... **TODO** */
                
                /* move the parameter nodes from the input statement tree
                 to the correct sequence within the result tree;
                 parse the parameter expressions */
                int ok = _append_cmd_params(temp_tree, cmd->params, cmd->param_count, cmd->param_is_delayed, params, in_source_line);
                
                /* swap the result tree with the input statement tree;
                 the input statement pointer now points to the result */
                _xte_ast_swap_ptrs(in_stmt, temp_tree);
                
                /* destroy the old input statement tree */
                _xte_ast_destroy(temp_tree);
                
                /* destroy the empty match params list */
                _xte_ast_destroy(params);
        
                return ok;
            }
        }
    } /* if prefix found */
    
    /* no matches, parse as generic message send */
    return _xte_parse_generic_command(in_engine, in_stmt, in_source_line);
}



/*********
 Entry Points
 */

/*
 *  _xte_parse_command
 *  ---------------------------------------------------------------------------------------------
 *  Initiates the parsing of a command call.  The token stream provided should be as obtained 
 *  from _xte_lex().
 *
 *  ! Modifies the input token stream, replacing it with the resulting abstract syntax tree.
 *
 *  The result is XTE_TRUE if the parse was successful (no syntax error.)
 *  Syntax errors are handled as per protocol, see xtalk_internal.c notes for details.
 */
int _xte_parse_command(XTE *in_engine, XTEAST *in_stmt, long in_source_line)
{
    assert(IS_XTE(in_engine));
    assert(in_stmt != NULL);
    return _xte_find_and_parse_command(in_engine, in_stmt, in_source_line);
}




/*********
 Dictionary
 */

/*
 *  _cmd_prefix
 *  ---------------------------------------------------------------------------------------------
 *  Lookup a command prefix in the terminology dictionary.  A command prefix is the first word
 *  of any number of command grammars which share that first word, or prefix.
 *
 *  If found, the result is a XTECmdPrefix*, which provides a list of command grammars that all
 *  begin with the given prefix word.
 *
 *  If <in_create_new> is specified XTE_TRUE and the prefix doesn't yet exist in the dictionary,
 *  it will be created.
 */
static struct XTECmdPrefix* _cmd_prefix(XTE *in_engine, const char *in_prefix_word, int in_create_new)
{
    assert(IS_XTE(in_engine));
    assert(in_prefix_word != NULL);
    assert(IS_BOOL(in_create_new));
    
    /* check if the specified prefix word is already in the prefix table;
     if it is, return a pointer to the appropriate entry, otherwise
     create and return a new entry in the prefix table */
    for (int i = 0; i < in_engine->cmd_prefix_count; i++)
    {
        if (_xte_compare_cstr(in_engine->cmd_prefix_table[i].prefix_word, in_prefix_word) == 0)
            return &(in_engine->cmd_prefix_table[i]); /* return matching prefix entry */
    }
    if (!in_create_new) return NULL; /* not found */
    
    /* create a new prefix entry */
    struct XTECmdPrefix *new_prefix_table = realloc(in_engine->cmd_prefix_table,
                                                         sizeof(struct XTECmdPrefix) * (in_engine->cmd_prefix_count + 1));
    if (!new_prefix_table) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    in_engine->cmd_prefix_table = new_prefix_table;
    struct XTECmdPrefix *prefix = &(new_prefix_table[in_engine->cmd_prefix_count++]);
    
    /* configure and return the new entry */
    prefix->prefix_word = _xte_clone_cstr(in_engine, in_prefix_word);
    prefix->command_count = 0;
    prefix->commands = NULL;
    return prefix;
}





/* adds a single command to the internal command dictionary */
static void _xte_command_add(XTE *in_engine, struct XTECommandDef *in_def)
{
    
    /* obtain a prefix table entry for this command definition */
    struct XTECmdPrefix *prefix = _cmd_prefix(in_engine, _xte_first_word(in_engine, in_def->syntax), XTE_TRUE);

    /* append this command definition to the prefix table entry */
    struct XTECmdInt *new_commands = realloc(prefix->commands, sizeof(struct XTECmdInt) * (prefix->command_count + 1));
    if (!new_commands) return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
    prefix->commands = new_commands;
    struct XTECmdInt *cmd = &(new_commands[prefix->command_count++]);
    cmd->imp = in_def->imp;
    //cmd->pattern = NULL;
    cmd->pattern = _xte_bnf_parse(in_def->syntax);
    _xte_itemize_cstr(in_engine, in_def->params, ",", &(cmd->params), &(cmd->param_count));
    cmd->param_is_delayed = calloc(cmd->param_count, sizeof(int));
    if (!cmd->param_is_delayed) return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
    for (int i = 0; i < cmd->param_count; i++)
    {
        char *param_name = cmd->params[i];
        long len = strlen(param_name);
        if (param_name[len-1] == '<')
        {
            param_name[len-1] = 0;
            cmd->param_is_delayed[i] = 1;
        }
        else
            cmd->param_is_delayed[i] = 0;
    }
}


/* add one or more commands with one call */
void _xte_commands_add(XTE *in_engine, struct XTECommandDef *in_defs)
{
    if (!in_defs) return;
    struct XTECommandDef *the_cmds = in_defs;
    while (the_cmds->syntax)
    {
        _xte_command_add(in_engine, the_cmds);
        the_cmds++;
    }
}



/*********
 Built-ins
 */

extern struct XTECommandDef _xte_builtin_mutator_cmds[];
extern struct XTECommandDef _xte_builtin_date_cmds[];
extern struct XTECommandDef _xte_builtin_wait_cmds[];


/* add built-in language commands */
void _xte_commands_add_builtins(XTE *in_engine)
{
    _xte_commands_add(in_engine, _xte_builtin_wait_cmds);
    _xte_commands_add(in_engine, _xte_builtin_mutator_cmds);
    _xte_commands_add(in_engine, _xte_builtin_date_cmds);
}




