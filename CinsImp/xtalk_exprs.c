/*
 
 xTalk Engine Expression Parsing Unit
 xtalk_exprs.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Responsible for parsing expressions
 
 */

#include "xtalk_internal.h"



/*********
 Utilities
 */

/* check if node is "the" */
int _xte_node_is_the(XTEAST *in_node)
{
    return ( in_node && (in_node->type == XTE_AST_WORD) &&
            in_node->value.string && (_xte_compare_cstr(in_node->value.string, "the") == 0) );
}


/* check if node is an allowable identifier;
 ie. a word starting with an alphabetic character or the underscore
 and containing only those and digits (0-9);
 some checks are implicit in that the lexer will not produce a word
 starting with a digit or an unflagged reserved word. */
int _xte_node_is_identifier(XTEAST *in_node)
{
    if (!in_node) return XTE_FALSE;
    if (in_node->type != XTE_AST_WORD) return XTE_FALSE;
    if (!in_node->value.string) return XTE_FALSE;
    
    char *ch = in_node->value.string;
    if (*ch == 0) return XTE_FALSE;
    
    /* basically everything is allowed, except punctuation */
    while (*ch)
    {
        if (ispunct(*ch) && (*ch != '_')) return XTE_FALSE;
        ch++;
    }
    
    /* disallow 'special' words so our current parser implementation handles
     object reference expressions properly... */
    if (_xte_compare_cstr(in_node->value.string, "to") == 0)
        return XTE_FALSE;
    
    /* check the length of the identifier */
    if (xte_cstring_length(in_node->value.string) > XTALK_LIMIT_IDENTIFIER_CHARS)
        return XTE_FALSE;
    
    return XTE_TRUE;
}



/*********
 Expression Parsing & Verification
 */

/*
 *  A Note About Future Enhancements
 *  ---------------------------------------------------------------------------------------------
 *  This expression parser is somewhat more pessimistic than the one in Apple's HyperCard.  As a
 *  result, some constructions HyperCard would allow this parser does not.
 *
 *  In general, this is a good thing.  The types of constructions that are deemed unacceptable
 *  are usually very difficult for humans to understand.
 *  eg.  card field id card field card field id 7 of bkgnd field "poly"
 *
 *  If it is decided that the parser should eventually be modified to be greedy and optimistic,
 *  it is probably best that this component (namely _exprs.c and many of the units to which it
 *  refers be rewritten from scratch, to take a token-by-token approach rather than attempting to
 *  translate in multiple passes as we do now.
 *
 *  Such a rewrite would be beneficial to the speed of the parser anyway.  The current
 *  implementation is likely to be quite slow compared to what it could be.
 */

/*
 *  ERROR_SYNTAX
 *  ---------------------------------------------------------------------------------------------
 *  Short macro wrapper for the engine's syntax error reporting function.
 */
#ifdef ERROR_SYNTAX
#undef ERROR_SYNTAX
#endif
#define ERROR_SYNTAX(in_template, in_arg1, in_arg2) _xte_error_syntax(in_stmt->engine, in_source_line, \
in_template, in_arg1, in_arg2, NULL)


/*
 *  _xte_parse_parentheses
 *  ---------------------------------------------------------------------------------------------
 *  Parses parenthesised subexpressions, eg. 2 * (3 + 1) to produce subtrees of type
 *  XTE_AST_EXPRESSION.
 *
 *  ! Mutates the input token stream.
 *
 *  The result is XTE_TRUE if the parse was successful (no syntax error.)
 *  Syntax errors are handled as per protocol.
 */
static int _xte_parse_parentheses(XTEAST *in_stmt, long in_source_line)
{
    assert(in_stmt != NULL);
    
    /* find an opening parenthesis */
    int paren_level;
    for (int i = 0; i < in_stmt->children_count; i++)
    {
        XTEAST *child_start = in_stmt->children[i];
        if (child_start->type == XTE_AST_PAREN_OPEN)
        {
            /* find the matching closing parenthesis */
            paren_level = 0;
            int found_it = XTE_FALSE;
            while (i+1 < in_stmt->children_count)
            {
                /* grab this node and remove it from the token stream */
                XTEAST *child_end = in_stmt->children[i+1];
                _xte_ast_list_remove(in_stmt, i+1);
                
                /* track opening parenthesis */
                if (child_end->type == XTE_AST_PAREN_OPEN) paren_level++;
                
                /* if we find the matching closing parenthesis */
                if ((child_end->type == XTE_AST_PAREN_CLOSE) && (paren_level == 0))
                {
                    _xte_ast_destroy(child_end);
                    child_start->type = XTE_AST_EXPRESSION;
                    if (!_xte_parse_parentheses(child_start, in_source_line)) return XTE_FALSE;
                    found_it = XTE_TRUE;
                    break;
                }
                else
                {
                    /* append this node to the subexpression we're building */
                    _xte_ast_list_append(child_start, child_end);
                }
                
                /* track closing parenthesis */
                if (child_end->type == XTE_AST_PAREN_CLOSE)
                {
                    paren_level--;
                    assert(paren_level >= 0); /* should never get lower, since we pick up closing parenthesis above */
                }
            }
            if (!found_it)
            {
                ERROR_SYNTAX("Expected \")\".", NULL, NULL);
                return XTE_FALSE;
            }
        }
        else if (child_start->type == XTE_AST_PAREN_CLOSE)
        {
            /* found a closing parenthesis before an opening! */
            ERROR_SYNTAX("Can't understand \")\".", NULL, NULL);
            return XTE_FALSE;
        }
    }
    
    return XTE_TRUE;
}


/*
 *  Expression Parsing Forward Declarations
 *  ---------------------------------------------------------------------------------------------
 *  Permits distribution of the parser across multiple translation units,
 *  without exposing these functions to the rest of the engine.
 */
void _xte_parse_convert_synonyms(XTE *in_engine, XTEAST *in_stmt);
void _xte_parse_reverse_synonyms(XTE *in_engine, XTEAST *in_stmt);
void _xte_parse_subexpressions(XTEAST *in_stmt);
void _xte_parse_consts(XTE *in_engine, XTEAST *in_stmt);
void _xte_parse_properties(XTE *in_engine, XTEAST *in_stmt);
void _xte_parse_functions(XTE *in_engine, XTEAST *in_stmt);
void _xte_parse_operators(XTE *in_engine, XTEAST *in_stmt);
void _xte_parse_refs(XTE *in_engine, XTEAST *in_stmt);

int _xte_coalesce_refs(XTE *in_engine, XTEAST *in_stmt, long in_source_line);
void _xte_post_process_funcs(XTE *in_engine, XTEAST *in_func);


/*
 *  _xte_parse_subexpression
 *  ---------------------------------------------------------------------------------------------
 *  Runs a variety of transformation routines to parse the majority of expression constructs,
 *  including synonyms, properties, constants, object references, function calls, and operators.
 *
 *  ! Mutates the input token stream.
 *
 *  ! The sequence of calls in this function is deliberate and changes may impact the functioning
 *    of the parser.
 *
 *  The result is XTE_TRUE if the parse was successful (no syntax error.)
 *  Syntax errors are handled as per protocol.
 */
static int _xte_parse_subexpression(XTEAST *in_subexpr, long in_source_line)
{
    assert(in_subexpr != NULL);
    
    /* parse subexpressions contained within this one */
    for (int i = 0; i < in_subexpr->children_count; i++)
    {
        XTEAST *child = in_subexpr->children[i];
        if (!child) continue;
        if (child->type == XTE_AST_EXPRESSION)
        {
            if (!_xte_parse_subexpression(child, in_source_line)) return XTE_FALSE;
        }
    }
    
    /* translate synonyms, eg. "msg box" is expanded to "message box" */
    _xte_parse_convert_synonyms(in_subexpr->engine, in_subexpr);
    
    /*
     tag known property names when used in an appropriate context,
     ie. following "the" or preceeding "of";
     
     because properties can potentially be mutated and in some instances
     there are properties with the same name as constants for syntactic reasons,
     eg. "message box" and "the message box", property names must be searched 
     before constants. 
     */
    _xte_parse_properties(in_subexpr->engine, in_subexpr);
    
    /* translate known constants */
    _xte_parse_consts(in_subexpr->engine, in_subexpr);
    
    /* tag and begin the decode of known class-related expressions;
     references to elements of a class or an entire collection of objects of that class,
     eg. "card field id 5" or "the cards" */
    _xte_parse_refs(in_subexpr->engine, in_subexpr);
    
    /* tag known functions, and obvious function calls,
     ie. where a valid identifier is followed by a parameter list (subexpression) */
    _xte_parse_functions(in_subexpr->engine, in_subexpr);
    //_xte_ast_debug(in_subexpr);
    
    /* move object reference parameters to the appropriate object reference branch,
     and move owners to their appropriate referring property, constant or object reference */
    if (!_xte_coalesce_refs(in_subexpr->engine, in_subexpr, in_source_line)) return XTE_FALSE;

    /* translate operators and their operands */
    _xte_parse_operators(in_subexpr->engine, in_subexpr);
    
    return XTE_TRUE;
}


/*
 *  _xte_parse_expression
 *  ---------------------------------------------------------------------------------------------
 *  Checks if a given expression is legal from a syntax perspective.  Would it make sense to 
 *  the interpreter?
 *
 *  The result is XTE_TRUE if the expression is valid (no syntax error.)
 *  Syntax errors are handled as per protocol.
 */
static int _xte_validate_subexpression(XTE *in_engine, XTEAST *in_stmt, long in_source_line)
{
    assert(IS_XTE(in_engine));
    assert(in_stmt != NULL);
    
    if (!in_stmt) return XTE_FALSE;
    switch (in_stmt->type)
    {
        case XTE_AST_EXPRESSION:
            /* valid sub-expressions are not allowed to have more than one term */
            if (in_stmt->children_count > 1)
            {
                ERROR_SYNTAX("Expected operator but found something else.", NULL, NULL);
                return XTE_FALSE;
            }
            if (in_stmt->children_count == 1)
            {
                if (!in_stmt->children[0])
                {
                    ERROR_SYNTAX("Can't understand \"( )\".", NULL, NULL);
                    return XTE_FALSE;
                }
                if (!_xte_validate_subexpression(in_engine, in_stmt->children[0], in_source_line)) return XTE_FALSE;
            }
            return XTE_TRUE;
        
        case XTE_AST_WORD:
        case XTE_AST_LITERAL_BOOLEAN:
        case XTE_AST_LITERAL_INTEGER:
        case XTE_AST_LITERAL_REAL:
        case XTE_AST_LITERAL_STRING:
            /* literals and keywords on their own are always valid, 
             assuming the rest of the expression checks out okay */
            return XTE_TRUE;
            
        case XTE_AST_REF:
            if (in_stmt->value.ref.is_collection)
            {
                /* collection references, eg "the cards", "the buttons", "the items",
                 are valid as long as all their children (parameters) are valid;
                 however their parameters may be NULL */
                for (int i = 0; i < in_stmt->children_count; i++)
                {
                    if (in_stmt->children[i] != NULL)
                    {
                        if (!in_stmt->children[0])
                        {
                            ERROR_SYNTAX("Can't understand arguments of \"%s\".", in_stmt->value.ref.type, NULL);
                            return XTE_FALSE;
                        }
                        if (!_xte_validate_subexpression(in_engine, in_stmt->children[i], in_source_line)) return XTE_FALSE;
                    }
                }
                return XTE_TRUE;
            }
        case XTE_AST_FUNCTION:
        case XTE_AST_CONSTANT:
        case XTE_AST_PROPERTY:
        case XTE_AST_OPERATOR:
            /* non-collection references, functions, properties, constants and operators;
             must all have valid, non-NULL parameters/operands */
            for (int i = 0; i < in_stmt->children_count; i++)
            {
                XTEAST *subexpr = in_stmt->children[i];
                if (!subexpr)
                {
                    switch (in_stmt->type)
                    {
                        case XTE_AST_REF:
                            if (strcmp(in_stmt->value.ref.type, "string") == 0)
                                ERROR_SYNTAX("Can't understand arguments of chunk expression.", NULL, NULL);
                            else
                                ERROR_SYNTAX("Can't understand arguments of \"%s\".", in_stmt->value.ref.type, NULL);
                            break;
                        case XTE_AST_FUNCTION:
                        case XTE_AST_CONSTANT:
                        case XTE_AST_PROPERTY:
                            ERROR_SYNTAX("Can't understand arguments of \"%s\".", in_stmt->note, NULL);
                            break;
                        case XTE_AST_OPERATOR:
                            ERROR_SYNTAX("Can't understand arguments of \"%s\".", _lexer_term_desc(in_stmt), NULL);
                            break;
                        default:
                            assert(0);
                            break;
                    }
                    return XTE_FALSE;
                }
                //if ((subexpr->type == XTE_AST_EXPRESSION) && (subexpr->children_count == 0))
                //{
                    /* it's an error for a reference type to have an empty subexpression parameter */
                    // this is probably a nonsensical test, since an expression could contain
                    // another subexpression that was empty... unless we use a recursive function
                    // to check?
                    // even then, what it's more probably going to come down to,
                    // is a runtime check to see if the given expression evaluates to or can
                    // be coerced to something useable as an operand/parameter
                    //return XTE_FALSE;
                //}
                if (!_xte_validate_subexpression(in_engine, subexpr, in_source_line)) return XTE_FALSE;
            }
            return XTE_TRUE;
            
        case XTE_AST_LIST:
            /* lists and other types are not allowed in valid expressions */
        default:
            ERROR_SYNTAX("Can't understand this.", NULL, NULL);
            return XTE_FALSE;
    }
    
    /* should never get here, all cases should be handled by switch */
    assert(0);
}


/*
 *  _xte_parse_expression
 *  ---------------------------------------------------------------------------------------------
 *  Initiates the parsing of an expression.  The token stream provided should be as obtained
 *  from _xte_lex().
 *
 *  ! Modifies the input token stream, replacing it with the resulting abstract syntax tree.
 *
 *  The result is XTE_TRUE if the parse was successful (no syntax error.)
 *  Syntax errors are handled as per protocol, see xtalk_internal.c notes for details.
 */
int _xte_parse_expression(XTE *in_engine, XTEAST *in_stream, long in_source_line)
{
    assert(IS_XTE(in_engine));
    assert(in_stream != NULL);
    assert(in_stream->children_count > 0);
    
    in_stream->type = XTE_AST_EXPRESSION;
    
    /* fix negate when expression was produced as a substream of a stream;
     ie. when the lexer is unable to determine that the - operator is actually a negate
     (this can happen in command arguments for example) */
    if (in_stream->children[0] && (in_stream->children[0]->type == XTE_AST_OPERATOR) &&
        (in_stream->children[0]->value.op == XTE_AST_OP_SUBTRACT))
        in_stream->children[0]->value.op = XTE_AST_OP_NEGATE;
    
    if (!_xte_parse_parentheses(in_stream, in_source_line)) return XTE_FALSE;
    
    if (!_xte_parse_subexpression(in_stream, in_source_line)) return XTE_FALSE;
    
    /* once functions have all their arguments,
     rearrange their arguments into separate subexpression groupings;
     each group is delimited by the comma (,) */
    _xte_post_process_funcs(in_engine, in_stream);
    
    //_xte_ast_debug(in_stream);
    
    return _xte_validate_subexpression(in_engine, in_stream, in_source_line);
}






