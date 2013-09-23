/*
 
 xTalk Engine Lexical Analyser
 xtalk_lexer.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Lexical analyser for xTalk source code; outputs a 1-dimensional abstract syntax tree for further
 transformation by the parsing units
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


/*********
 Internal Limits
 */

#define MAX_BYTES_PER_UTF8_CHAR 6
#define CHECK_MARGIN_CHARS 4

/*
 *  MAX_TOKEN_LENGTH
 *  ---------------------------------------------------------------------------------------------
 *  In practice, only limits the number of bytes allowable for variable names, identifiers and 
 *  individual command syntax words (XTE_AST_WORD).
 *
 *  Technically applies to the construction of XTE_AST_LITERAL_INTEGER, XTE_AST_LITERAL_REAL,
 *  XTE_AST_KEYWORD, XTE_AST_OPERATOR, however, these are known in advance.
 *
 *  String literals are exempt from this limit and must simply fit within available memory.
 */
#define MAX_TOKEN_LENGTH ((XTALK_LIMIT_IDENTIFIER_CHARS + CHECK_MARGIN_CHARS) * MAX_BYTES_PER_UTF8_CHAR)


/* see also _limits.h for published limits */



/*********
 Character Utilities
 (UTF-8 aware)
 */

/*
 *  enum CharType
 *  ---------------------------------------------------------------------------------------------
 *  Categories of character recognised by the lexical analyser.
 */
enum CharType {
    CHAR_TYPE_UNKNOWN = 0,  /* invalid */
    CHAR_TYPE_PUNCTUATION,  /* defined by _char_is_punctuation() */
    CHAR_TYPE_ALPHABETIC,   /* defined as everything except punctuation and numeric */
    CHAR_TYPE_NUMERIC,      /* defined by _char_is_numeric() */
};


/*
 *  _char_is_punctuation
 *  ---------------------------------------------------------------------------------------------
 *  Punctuation characters (CHAR_TYPE_PUNCTUATION) recognised by the lexical analyser:
 *  -   ASCII characters defined by the C standard library is punctuation, ie.
 *      graphic (non-control) characters that are not latin alpha-numeric
 *  -   Underscore (_) is NOT considered punctuation for this unit
 *  -   Line-continuation character (Unicode math NOT symbol ¬)
 *  -   Less than/equal, greater than/equal symbols (Unicode ≤ ≥)
 *  -   Not-equal character (Unicode ≠)
 *
 *  The <in_source> input is expected to be in UTF-8 encoding as with all lexer functions.
 */
static int _char_is_punctuation(const char *in_source, long in_offset, long in_length)
{
    unsigned char ch1 = in_source[in_offset];
    
    /* underscore is allowed in identifiers and is considered part of the alphabet for lexing purposes */
    if (ch1 == '_') return XTE_FALSE;
    
    /* whitespace, punctuation and the NULL terminator are all considered punctuation */
    if (ispunct(ch1) || isspace(ch1) || (ch1 == 0)) return XTE_TRUE;
    
    /* if there's only 1 byte to go before the end of the string, get out of here */
    if (in_offset + 1 >= in_length) return XTE_FALSE;
    
    
    unsigned char ch2 = in_source[in_offset + 1];
    
    /* the 'line continuation character' (UTF-8, math NOT symbol) */
    if ((ch1 == 0xc2) && (ch2 == 0xac)) return XTE_TRUE;
    
    /* if there's only 2 bytes to go before the end of the string, get out of here */
    if (in_offset + 2 >= in_length) return XTE_FALSE;
    

    unsigned char ch3 = in_source[in_offset + 2];
    
    /* less than symbol (UTF-8) */
    if ((ch1 == 0xe2) && (ch2 == 0x89) && (ch3 == 0xa4)) return XTE_TRUE;
    
    /* greater than symbol (UTF-8) */
    if ((ch1 == 0xe2) && (ch2 == 0x89) && (ch3 == 0xa5)) return XTE_TRUE;
    
    /* not-equal symbol (UTF-8) */
    if ((ch1 == 0xe2) && (ch2 == 0x89) && (ch3 == 0xa0)) return XTE_TRUE;
    
    
    /* anything else isn't punctuation */
    return XTE_FALSE;
}


/*
 *  _char_is_numeric
 *  ---------------------------------------------------------------------------------------------
 *  Numeric characters (CHAR_TYPE_NUMERIC) recognised by the lexical analyser:
 *  -   ASCII/latin characters defined by the C standard library as digits, ie. isdigit() != 0
 *
 *  The <in_source> input is expected to be in UTF-8 encoding as with all lexer functions.
 */
static int _char_is_numeric(const char *in_source, long in_offset, long in_length)
{
    if (isdigit(in_source[in_offset])) return XTE_TRUE;
    return XTE_FALSE;
}


/*
 *  _char_type
 *  ---------------------------------------------------------------------------------------------
 *  Classifies the next Unicode character in <in_source> as:
 *  CHAR_TYPE_PUNCTUATION, CHAR_TYPE_NUMERIC or CHAR_TYPE_ALPHABETIC.
 *
 *  Anything other than punctuation or numeric is classified alphabetic.
 *  Punctuation and numeric are defined by their respective functions (_char_is_punctuation,
 *  _char_is_numeric).
 *
 *  The <in_source> input is expected to be in UTF-8 encoding as with all lexer functions.
 */
static enum CharType _char_type(const char *in_source, long in_offset, long in_length)
{
    if (_char_is_numeric(in_source, in_offset, in_length)) return CHAR_TYPE_NUMERIC;
    if (_char_is_punctuation(in_source, in_offset, in_length)) return CHAR_TYPE_PUNCTUATION;
    return CHAR_TYPE_ALPHABETIC;
}


/*
 *  _substring
 *  ---------------------------------------------------------------------------------------------
 *  Obtains a substring from the input <in_source> with the specified byte <in_offset> and
 *  <in_length>.
 *
 *  The result is statically allocated and remains available until the next call.  A zero-length
 *  range will return an empty string "".
 *
 *  The <in_source> input is expected to be in UTF-8 encoding as with all lexer functions.
 */
static const char* _substring(XTE *in_engine, const char *in_source, long in_offset, long in_length)
{
    char *buffer = in_engine->f_result_cstr;
    if (in_length > MAX_TOKEN_LENGTH) in_length = MAX_TOKEN_LENGTH;
    memcpy(buffer, in_source + in_offset, in_length);
    buffer[in_length] = 0;
    return buffer;
}


/*
 *  _xte_lex
 *  ---------------------------------------------------------------------------------------------
 *  The interface to the lexical analyser, and where most of the analysis occurs.  Accepts the 
 *  <in_source> as input and returns a flat abstract syntax tree - a single XTE_AST_LIST node
 *  with children for each of the tokens of the input source.
 *
 *  Node types output by this function are:
 *  XTE_AST_LITERAL_STRING, XTE_AST_LITERAL_BOOLEAN, XTE_AST_LITERAL_REAL, 
 *  XTE_AST_LITERAL_INTEGER, XTE_AST_OPERATOR, XTE_AST_OF, XTE_AST_IN, XTE_AST_KEYWORD,
 *  XTE_AST_PAREN_OPEN, XTE_AST_PAREN_CLOSE, XTE_AST_COMMA, XTE_AST_NEWLINE, XTE_AST_WORD
 *
 *  Line comments (started with --) and whitespace are not present in the output.
 *
 *  XTE_AST_NEWLINE provides a byte offset (source_offset), and both actual and logical line
 *  offsets (source_line, logical_line) for the line beginning immediately after the newline
 *  character.
 *
 *  Actual line numbers correspond to the position of newline characters in the source.
 *  Logical lines correspond to the lines with long lines split using the line-continuation 
 *  character coalesced into a single long line (as if the hard line break wasn't present.)
 *
 *  Line numbering assumes the first line is 1.
 *
 *  If the lexer runs out of memory, standard out of memory protocol will be followed.
 *
 *  May raise a XTE_ERROR_SYNTAX if certain limits are breached.
 *
 *  The <in_source> input is expected to be in UTF-8 encoding as with all lexer functions.
 */
XTEAST* _xte_lex(XTE *in_engine, const char *in_source)
{
    assert(IS_XTE(in_engine));
    assert(in_source != NULL);
    
    /* create an empty AST to hold the resulting tokens */
    XTEAST *result = _xte_ast_create(in_engine, XTE_AST_LIST);
    if (!result) return NULL;
    
    /* setup some state variables */
    long length = strlen(in_source);
    long alnum_offset = 0, alnum_length;
    long punc_offset;
    char punc_buffer[2];
    punc_buffer[1] = 0;
    int parse_real = XTE_FALSE;
    int source_line = 1;
    int logical_line = 1;
    
    if (in_engine->f_result_cstr) free(in_engine->f_result_cstr);
    in_engine->f_result_cstr = malloc(MAX_TOKEN_LENGTH + 1);
    
    /* iterate over the bytes of the input source */
    for (long offset = 0; offset <= length; offset++)
    {
        /* check the type of the current character; punctuation, digit, etc. */
        enum CharType char_type = _char_type(in_source, offset, length);
        if ((char_type == CHAR_TYPE_PUNCTUATION) || parse_real)
        {
            /* ordinary number, keyword or identifier */
            alnum_length = offset - alnum_offset;
            if (alnum_length > 0)
            {
                /* number */
                if (isdigit(in_source[alnum_offset]) || parse_real)
                {
                    parse_real = XTE_FALSE;
                    int is_real = XTE_FALSE;
                    long scan_offset = alnum_offset;
                    for (; scan_offset <= length; scan_offset++)
                    {
                        if (!isdigit(in_source[scan_offset]))
                        {
                            if (in_source[scan_offset] == '.')
                            {
                                if (!is_real) is_real = XTE_TRUE;
                                else break;
                            }
                            else break;
                        }
                    }
                    long num_length = scan_offset - alnum_offset;
                    const char *number_text = _substring(in_engine, in_source, alnum_offset, num_length);
                    if (!is_real) _xte_ast_list_append(result, _xte_ast_create_literal_integer(in_engine, atoi(number_text)));
                    else _xte_ast_list_append(result, _xte_ast_create_literal_real(in_engine, atof(number_text)));
                    offset = alnum_offset + num_length - 1;
                    alnum_offset = offset + 1;
                    continue;
                }
                
                /* text operator */
                char word1[MAX_TOKEN_LENGTH+1];
                strcpy(word1, _substring(in_engine, in_source, alnum_offset, alnum_length));
                if (_xte_compare_cstr(word1, "is") == 0)
                {
                    if ((alnum_offset + 6 <= length) && (_xte_compare_cstr(_substring(in_engine, in_source, alnum_offset, 6), "is not") == 0))
                    {
                        if ((alnum_offset + 13 <= length) && (_xte_compare_cstr(_substring(in_engine, in_source, alnum_offset, 13), "is not within") == 0))
                        {
                            offset = alnum_offset + 13;
                            _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_IS_NOT_WITHIN));
                        }
                        else if ((alnum_offset + 9 <= length) && (_xte_compare_cstr(_substring(in_engine, in_source, alnum_offset, 9), "is not in") == 0))
                        {
                            offset = alnum_offset + 9;
                            _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_IS_NOT_IN));
                        }
                        else /* is not */
                        {
                            offset = alnum_offset + 6;
                            _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_NOT_EQUAL));
                        }
                    }
                    else
                    {
                        if ((alnum_offset + 9 <= length) && (_xte_compare_cstr(_substring(in_engine, in_source, alnum_offset, 9), "is within") == 0))
                        {
                            offset = alnum_offset + 9;
                            _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_IS_WITHIN));
                        }
                        else if ((alnum_offset + 5 <= length) && (_xte_compare_cstr(_substring(in_engine, in_source, alnum_offset, 5), "is in") == 0))
                        {
                            offset = alnum_offset + 5;
                            _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_IS_IN));
                        }
                        else /* is */
                            _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_EQUAL));
                    }
                }
                else if (_xte_compare_cstr(word1, "there") == 0)
                {
                    if ((alnum_offset + 10 <= length) && (_xte_compare_cstr(_substring(in_engine, in_source, alnum_offset, 10), "there is a") == 0)
                        && (_char_type(in_source, alnum_offset + 10, 1) != CHAR_TYPE_ALPHABETIC))
                    {
                        offset = alnum_offset + 10;
                        _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_THERE_IS_A));
                    }
                    else if ((alnum_offset + 11 <= length) && (_xte_compare_cstr(_substring(in_engine, in_source, alnum_offset, 11), "there is no") == 0)
                             && (_char_type(in_source, alnum_offset + 11, 1) != CHAR_TYPE_ALPHABETIC))
                    {
                        offset = alnum_offset + 11;
                        _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_THERE_IS_NO));
                    }
                    else
                        _xte_ast_list_append(result, _xte_ast_create_word(in_engine, word1));
                }
                else if (_xte_compare_cstr(word1, "of") == 0)
                    _xte_ast_list_append(result, _xte_ast_create(in_engine, XTE_AST_OF));
                else if (_xte_compare_cstr(word1, "in") == 0)
                    _xte_ast_list_append(result, _xte_ast_create(in_engine, XTE_AST_IN));
                else if (_xte_compare_cstr(word1, "contains") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_CONTAINS));
                else if (_xte_compare_cstr(word1, "div") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_DIVIDE_INT));
                else if (_xte_compare_cstr(word1, "mod") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_MODULUS));
                else if (_xte_compare_cstr(word1, "and") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_AND));
                else if (_xte_compare_cstr(word1, "or") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_OR));
                else if (_xte_compare_cstr(word1, "not") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_NOT));
                
                /* reserved keywords */
                else if (_xte_compare_cstr(word1, "end") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_END));
                else if (_xte_compare_cstr(word1, "exit") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_EXIT));
                else if (_xte_compare_cstr(word1, "function") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_FUNCTION));
                else if (_xte_compare_cstr(word1, "global") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_GLOBAL));
                else if (_xte_compare_cstr(word1, "if") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_IF));
                else if (_xte_compare_cstr(word1, "then") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_THEN));
                else if (_xte_compare_cstr(word1, "else") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_ELSE));
                else if (_xte_compare_cstr(word1, "next") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_NEXT));
                else if (_xte_compare_cstr(word1, "on") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_ON));
                else if (_xte_compare_cstr(word1, "pass") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_PASS));
                else if (_xte_compare_cstr(word1, "repeat") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_REPEAT));
                else if (_xte_compare_cstr(word1, "return") == 0)
                    _xte_ast_list_append(result, _xte_ast_create_keyword(in_engine, XTE_AST_KW_RETURN));
                
                /* word or identifier */
                else
                {
                    /* check the Unicode character length against internal limits */
                    if (xte_cstring_length(word1) > XTALK_LIMIT_IDENTIFIER_CHARS)
                    {
                        _xte_ast_destroy(result);
                        ERROR_SYNTAX(source_line, "Keyword or identifier too long.", NULL, NULL, NULL);
                        return NULL;
                    }
                    else
                        _xte_ast_list_append(result, _xte_ast_create_word(in_engine, word1));
                }
            }
            
            punc_offset = offset;
            
            /* punctuation operator */
            if (in_source[offset] == '=')
                _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_EQUAL));
            else if (in_source[offset] == '(')
                _xte_ast_list_append(result, _xte_ast_create(in_engine, XTE_AST_PAREN_OPEN));
            else if (in_source[offset] == ')')
                _xte_ast_list_append(result, _xte_ast_create(in_engine, XTE_AST_PAREN_CLOSE));
            else if (in_source[offset] == ',')
                _xte_ast_list_append(result, _xte_ast_create(in_engine, XTE_AST_COMMA));
            else if (in_source[offset] == '^')
                _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_EXPONENT));
            else if (in_source[offset] == '*')
                _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_MULTIPLY));
            else if (in_source[offset] == '+')
                _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_ADD));
            else if (  (in_source[offset] == '-') && (! ((offset + 1 < length) && (in_source[offset+1] == '-')) )  )
            {
                /* check the surrounds to determine if the minus sign is a negation operator,
                 or a subtraction operator */
                XTEAST *last = _xte_ast_list_child(result, result->children_count-1);
                if ((!last) ||
                    (last->type == XTE_AST_OPERATOR) ||
                    (last->type == XTE_AST_PAREN_OPEN) ||
                    (last->type == XTE_AST_NEWLINE) ||
                    (last->type == XTE_AST_OF) ||
                    (last->type == XTE_AST_COMMA))
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_NEGATE));
                else
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_SUBTRACT));
            }
            
            else if (in_source[offset] == '/')
                _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_DIVIDE_FP));
            else if (in_source[offset] == '<')
            {
                if ((offset + 1 < length) && (in_source[offset+1] == '='))
                {
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_LESS_EQ));
                    offset++;
                }
                else if ((offset + 1 < length) && (in_source[offset+1] == '>'))
                {
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_NOT_EQUAL));
                    offset++;
                }
                else
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_LESSER));
            }
            else if (in_source[offset] == '>')
            {
                if ((offset + 1 < length) && (in_source[offset+1] == '='))
                {
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_GREATER_EQ));
                    offset++;
                }
                else
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_GREATER));
            }
            else if (in_source[offset] == '&')
            {
                if ((offset + 1 < length) && (in_source[offset+1] == '&'))
                {
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_CONCAT_SP));
                    offset++;
                }
                else
                    _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_CONCAT));
            }
            else if ((offset + 2 < length) && ((unsigned char)in_source[offset] == 0xE2) && ((unsigned char)in_source[offset+1] == 0x89)
                     && ((unsigned char)in_source[offset+2] == 0xA4))
            {
                _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_LESS_EQ));
                offset += 2;
            }
            else if ((offset + 2 < length) && ((unsigned char)in_source[offset] == 0xE2) && ((unsigned char)in_source[offset+1] == 0x89)
                     && ((unsigned char)in_source[offset+2] == 0xA5))
            {
                _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_GREATER_EQ));
                offset += 2;
            }
            else if ((offset + 2 < length) && ((unsigned char)in_source[offset] == 0xE2) && ((unsigned char)in_source[offset+1] == 0x89)
                     && ((unsigned char)in_source[offset+2] == 0xA0))
            {
                _xte_ast_list_append(result, _xte_ast_create_operator(in_engine, XTE_AST_OP_NOT_EQUAL));
                offset += 2;
            }
            
            /* line comment */
            else if ((offset + 1 < length) && (in_source[offset] == '-') && (in_source[offset+1] == '-'))
            {
                for (offset++; offset <= length; offset++)
                {
                    if ((in_source[offset] == 13) || (in_source[offset] == 10)) break;
                }
                
                if ((offset < length) && (in_source[offset] == 13) && (in_source[offset + 1] == 10))
                    offset++;
                _xte_ast_list_append(result, _xte_ast_create(in_engine, XTE_AST_NEWLINE));
                _xte_ast_list_set_line_offsets(result, offset, source_line++, logical_line++);
            }
            
            /* 'line continuation' character */
            else if ((offset + 1 < length) && ((unsigned char)in_source[offset] == 0xC2) && ((unsigned char)in_source[offset+1] == 0xAC))
            {
                for (offset++; offset <= length; offset++)
                {
                    if ((in_source[offset] == 13) || (in_source[offset] == 10)) break;
                }
                if ((offset < length) && (in_source[offset] == 13) && (in_source[offset + 1] == 10))
                    offset++;
                
                source_line++;
            }
            
            /* newline */
            else if ((in_source[offset] == 13) || (in_source[offset] == 10))
            {
                if ((offset < length) && (in_source[offset] == 13) && (in_source[offset + 1] == 10))
                    offset++;
                _xte_ast_list_append(result, _xte_ast_create(in_engine, XTE_AST_NEWLINE));
                _xte_ast_list_set_line_offsets(result, offset, source_line++, logical_line++);
            }
            
            /* "quoted string" literal */
            else if (in_source[offset] == '"')
            {
                long string_length = 0;
                char *string_buffer = NULL;
                long string_offset = offset + 1;
                for (offset++; offset <= length; offset++)
                {
                    if (in_source[offset] == '"') break;
                }
                string_length = offset - string_offset;
                string_buffer = malloc(string_length + 1);
                if (!string_buffer)
                {
                    _xte_ast_destroy(result);
                    return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
                }
                memcpy(string_buffer, in_source + string_offset, string_length);
                string_buffer[string_length] = 0;
                
                /* check Unicode character length is within limits */
                if (xte_cstring_length(string_buffer) > XTALK_LIMIT_STRING_LITERAL_CHARS)
                {
                    free(string_buffer);
                    _xte_ast_destroy(result);
                    ERROR_SYNTAX(source_line, "Quoted string literal too long.", NULL, NULL, NULL);
                    return NULL;
                }
                
                _xte_ast_list_append(result, _xte_ast_create_literal_string(in_engine, string_buffer, XTE_AST_FLAG_NOCOPY));
            }
            
            /* real numbers with no digit prefix */
            else if ((in_source[offset] == '.') && isdigit(in_source[offset+1]))
            {
                alnum_offset = offset;
                parse_real = XTE_TRUE;
                continue;
            }
            
            /* all other punctuation characters except whitespace */
            else if ( (!isspace(in_source[offset])) && (offset < length) )
            {
                punc_buffer[0] = in_source[offset];
                _xte_ast_list_append(result, _xte_ast_create_word(in_engine, punc_buffer));
            }
            
            alnum_offset = offset + 1;
        }
    }
    
    return result;
}


/*
 *  _lexer_term_desc
 *  ---------------------------------------------------------------------------------------------
 *  Returns a string description of the supplied AST node.
 *
 *  The AST node must be one of the types output by the lexical analyser (see _xte_lex().)
 *
 *  The description will remain available while the node itself remains valid, and until the 
 *  next call.
 *
 *  Used to provide helpful syntax error messages to the user.
 */
char const* _lexer_term_desc(XTEAST *in_node)
{
    assert(in_node != NULL);
    assert(in_node->engine != NULL);
    
    if (in_node->engine->f_result_cstr) free(in_node->engine->f_result_cstr);
    in_node->engine->f_result_cstr = malloc(512);
    char *buffer = in_node->engine->f_result_cstr;

    switch (in_node->type)
    {
        case XTE_AST_LITERAL_STRING:
            return in_node->value.string;
        case XTE_AST_LITERAL_BOOLEAN:
            if (in_node->value.boolean) return "true";
            return "false";
        case XTE_AST_LITERAL_REAL:
            sprintf(buffer, "%f", in_node->value.real);
            return buffer;
        case XTE_AST_LITERAL_INTEGER:
            sprintf(buffer, "%d", in_node->value.integer);
            return buffer;
        case XTE_AST_OPERATOR:
        {
            switch (in_node->value.op)
            {
                case XTE_AST_OP_EQUAL://is, =
                    return "=";
                case XTE_AST_OP_NOT_EQUAL:// is not, <>
                    return "<>";
                case XTE_AST_OP_GREATER://>
                    return ">";
                case XTE_AST_OP_LESSER://<
                    return "<";
                case XTE_AST_OP_LESS_EQ://<=
                    return "<=";
                case XTE_AST_OP_GREATER_EQ://>=
                    return ">=";
                case XTE_AST_OP_CONCAT://&
                    return "&";
                case XTE_AST_OP_CONCAT_SP://&&
                    return "&&";
                case XTE_AST_OP_EXPONENT://^
                    return "^";
                case XTE_AST_OP_MULTIPLY://*
                    return "*";
                case XTE_AST_OP_DIVIDE_FP:///
                    return "/";
                case XTE_AST_OP_ADD://+
                    return "+";
                case XTE_AST_OP_SUBTRACT://-
                case XTE_AST_OP_NEGATE://- (preceeded by any operator, left parenthesis, beginning of stream, newline or OF)
                    return "-";
                case XTE_AST_OP_IS_IN: // reverse operands for "contains"
                    return "is in";
                case XTE_AST_OP_CONTAINS:
                    return "contains";
                case XTE_AST_OP_IS_NOT_IN:
                    return "is not in";
                case XTE_AST_OP_DIVIDE_INT://div
                    return "div";
                case XTE_AST_OP_MODULUS://mod
                    return "mod";
                case XTE_AST_OP_AND://and
                    return "and";
                case XTE_AST_OP_OR://or
                    return "or";
                case XTE_AST_OP_NOT://not
                    return "not";
                case XTE_AST_OP_IS_WITHIN://is within (geometric)
                    return "is within";
                case XTE_AST_OP_IS_NOT_WITHIN://is not within (geometric)
                    return "is not within";
                case XTE_AST_OP_THERE_IS_A:
                    return "there is a";
                case XTE_AST_OP_THERE_IS_NO:
                    return "there is no";
                default: break;
            }
            break;
        }
        case XTE_AST_OF:
            return "of";
        case XTE_AST_IN:
            return "in";
        case XTE_AST_KEYWORD:
        {
            switch (in_node->value.keyword)
            {
                case XTE_AST_KW_END:
                    return "end";
                case XTE_AST_KW_EXIT:
                    return "exit";
                case XTE_AST_KW_FUNCTION:
                    return "function";
                case XTE_AST_KW_GLOBAL:
                    return "global";
                case XTE_AST_KW_IF:
                    return "if";
                case XTE_AST_KW_THEN:
                    return "then";
                case XTE_AST_KW_ELSE:
                    return "else";
                case XTE_AST_KW_NEXT:
                    return "next";
                case XTE_AST_KW_ON:
                    return "on";
                case XTE_AST_KW_PASS:
                    return "pass";
                case XTE_AST_KW_REPEAT:
                    return "repeat";
                case XTE_AST_KW_RETURN:
                    return "return";
                default: break;
            }
            break;
        }
        case XTE_AST_PAREN_OPEN:
            return "(";
        case XTE_AST_PAREN_CLOSE:
            return ")";
        case XTE_AST_COMMA:
            return ",";
        case XTE_AST_NEWLINE:
            return "end of line";
        case XTE_AST_WORD:
            return in_node->value.string;
        default:
            assert(0);
    }
    assert(0);
}


