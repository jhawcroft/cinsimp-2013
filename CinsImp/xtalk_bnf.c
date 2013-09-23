/*
 
 xTalk Engine BNF Parser
 xtalk_bnf.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Converts command syntax definitions expressed as BNF (Backus Naur Form) into a tree form suitable
 for use by the Command Parsing Unit to match command statement syntax
 
 NOTE to make re allowable inputs - expects words to consist of only alphabetic characters in the
 standard ASCII roman set, or, a single comma (,), anything else will probably cause an infinite loop
 - checks for this should probably be included to fail pattern parsing if it contains anything 
 unsupported.. ***** TODO ***
 
 */

#include "xtalk_internal.h"


#define TOKEN_LIST_INCREASE_ALLOC 50


// start by getting tokens

struct BNFToken
{
    char *text;
};


struct BNFTokenContext
{
    struct BNFToken *tokens;
    int token_alloc;
    int token_count;
};


typedef struct StopWordSet
{
    int count;
    char **words;
} StopWordSet;




static char* _clone_cstr(const char *in_cstr)
{
    if (!in_cstr) in_cstr = "";
    long len = strlen(in_cstr);
    char *result = malloc(len + 1);
    if (!result) return NULL;
    strcpy(result, in_cstr);
    return result;
}


static StopWordSet* _sws_create(void)
{
    StopWordSet *result = calloc(sizeof(StopWordSet), 1);
    return result;
}


static void _sws_destroy(StopWordSet *in_set)
{
    //return;
    
    if (!in_set) return;
    for (int i = 0; i < in_set->count; i++)
    {
        if (in_set->words[i]) free(in_set->words[i]);
    }
    if (in_set->words) free(in_set->words);
    free(in_set);
}


static void _sws_add(StopWordSet *in_set, const char *in_word)
{
    for (int i = 0; i < in_set->count; i++)
    {
        if (strcmp(in_set->words[i], in_word) == 0) return;
    }
    char **new_words = realloc(in_set->words, sizeof(char*) * (in_set->count + 1));
    if (!new_words) return;
    in_set->words = new_words;
    in_set->words[in_set->count++] = _clone_cstr(in_word);
}


static void _sws_union(StopWordSet *in_1, StopWordSet *in_2)
{
    if (!in_2) return;
    for (int i = 0; i < in_2->count; i++)
        _sws_add(in_1, in_2->words[i]);
}


// clone
static char** _sws_grab(StopWordSet *in_set)
{
    char **stop_words = malloc(sizeof(char*) * (in_set->count + 1));
    if (!stop_words) return NULL;
    for (int i = 0; i < in_set->count; i++)
    {
        stop_words[i] = _clone_cstr(in_set->words[i]);
    }
    stop_words[in_set->count] = NULL;
    return stop_words;
}



static StopWordSet* _stop_words_for_subpattern(XTECmdPattern *in_pattern)
{
    StopWordSet *result = NULL, *set;
    if (!in_pattern) return NULL;
    switch (in_pattern->type)
    {
        case XTE_CMDPAT_SET_REQ:
        case XTE_CMDPAT_SET_OPT:
        {
            // each list element's stop words must be appended, because regardless of the list type,
            // the elements are each alternates
            for (int i = 0; i < in_pattern->child_count; i++)
            {
                set = _stop_words_for_subpattern(in_pattern->children[i]);
                if (set && set->count) {
                    if (!result) result = _sws_create();
                    if (!result) return NULL;
                    _sws_union(result, set);
                }
                _sws_destroy(set);
            }
            return result;
        }
            
        case XTE_CMDPAT_LIST:
        {
            // accumulate stop words of optional sets
            // and the first required set
            // or the first word we encounter in this element
            int got_req;
            for (int i = 0; i < in_pattern->child_count; i++)
            {
                XTECmdPattern *subpattern = in_pattern->children[i];
                if (!subpattern) continue;
                if (subpattern->type == XTE_CMDPAT_LITERAL)
                {
                    if (!result) result = _sws_create();
                    if (!result) return NULL;
                    _sws_add(result, subpattern->payload.literal);
                    return result;
                }
                got_req = 0;
                if ((subpattern->type == XTE_CMDPAT_SET_OPT) || (subpattern->type == XTE_CMDPAT_SET_REQ))
                {
                    set = _stop_words_for_subpattern(in_pattern->children[i]);
                    if (set && set->count) {
                        if (!result) result = _sws_create();
                        if (!result) return NULL;
                        _sws_union(result, set);
                        got_req = (subpattern->type == XTE_CMDPAT_SET_REQ);
                    }
                    _sws_destroy(set);
                }
                // continue appending until first required word(s)
                if (got_req) break;
            }
            return result;
        }
            
        case XTE_CMDPAT_LITERAL:
            // the stop word, is this literal! <-- first literal encountered at top of search
            result = _sws_create();
            _sws_add(result, in_pattern->payload.literal);
            return result;
            
            break;
        case XTE_CMDPAT_PARAM:
            // this is an error - if we encounter one of these at the top of the search
            // effectively two or more successive params with no syntax between - not allowed
            
            break;
    }
    return NULL;
}


static void _attach_stop_words(XTECmdPattern *in_pattern, StopWordSet *in_set)
{
    switch (in_pattern->type)
    {
        case XTE_CMDPAT_SET_REQ:
        case XTE_CMDPAT_SET_OPT:
            for (int i = 0; i < in_pattern->child_count; i++)
            {
                _attach_stop_words(in_pattern->children[i], in_set);
            }
            break;
            
        case XTE_CMDPAT_LIST:
            // need to process within lists
            // if we encounter a param, stop words from caller + those following but only up til the first required set
            for (int i = 0; i < in_pattern->child_count; i++)
            {
                // stop words include those we can obtain from the subpatterns beyond child i,
                // up to the first required subpattern or first literal
                StopWordSet *post_set = _sws_create();
                for (int j = i+1; j < in_pattern->child_count; j++)
                {
                    XTECmdPattern *subpattern = in_pattern->children[j];
                    StopWordSet *set = _stop_words_for_subpattern(subpattern);
                    if (set && set->count) _sws_union(post_set, set);
                    _sws_destroy(set);
                    if (subpattern->type == XTE_CMDPAT_LITERAL) break;
                }
                
                // create a union of the set we've been fed
                StopWordSet *union_set = _sws_create();
                _sws_union(union_set, in_set);
                _sws_union(union_set, post_set);
                _attach_stop_words(in_pattern->children[i], union_set);
                
                // cleanup
                _sws_destroy(post_set);
                _sws_destroy(union_set);
            }
            break;
            
        case XTE_CMDPAT_LITERAL:
            break;
            
        case XTE_CMDPAT_PARAM:
            // need to attach stop words here
            assert(in_pattern->stop_words == NULL);
            in_pattern->stop_words = _sws_grab(in_set);
            break;
    }
}





static void _init_context(struct BNFTokenContext *in_context)
{
    in_context->tokens = NULL;
    in_context->token_alloc = 0;
    in_context->token_count = 0;
}


/* we take ownership of in_token */
static int _append_token(struct BNFTokenContext *in_context, char *in_token)
{
    
    /* make the token list bigger (if needed) */
    if (in_context->token_count == in_context->token_alloc)
    {
        struct BNFToken *new_tokens = realloc(in_context->tokens,
                                              sizeof(struct BNFToken) * (in_context->token_alloc + TOKEN_LIST_INCREASE_ALLOC));
        if (!new_tokens)
        {
            if (in_token) free(in_token);
            return 0;
        }
        in_context->tokens = new_tokens;
        in_context->token_alloc += TOKEN_LIST_INCREASE_ALLOC;
    }
    
    /* append the token */
    struct BNFToken *token = in_context->tokens + in_context->token_count++;
    token->text = in_token;
    
    return 1;
}


static struct BNFToken* _tokenize(const char *in_syntax)
{
    /* prepare to tokenize */
    struct BNFTokenContext context;
    _init_context(&context);
    
    /* iterate through characters of syntax */
    long len = strlen(in_syntax);
    long token_start = 0, token_length;
    char *buffer;
    for (long offset = 0; offset <= len; offset++)
    {
        /* as soon as we encounter punctuation, whitespace or the end of the syntax;
         process the preceeding token, the punctuation and prepare for the next */
        if ( isspace(in_syntax[offset]) || (in_syntax[offset] == 0) || (!isalpha(in_syntax[offset])) )
        {
            /* append preceeding text token (if any) */
            token_length = offset - token_start;
            if (token_length)
            {
                buffer = malloc(token_length + 1);
                if (!buffer)
                {
                    if (context.tokens) free(context.tokens);
                    return NULL;
                }
                memcpy(buffer, in_syntax + token_start, token_length);
                buffer[token_length] = 0;
                if (!_append_token(&context, buffer))
                {
                    if (context.tokens) free(context.tokens);
                    return NULL;
                }
                buffer = NULL;
            }
            
            /* handle punctuation */
            if ( (offset < len) && (!isspace(in_syntax[offset])) )
            {
                buffer = malloc(2);
                if (!buffer)
                {
                    if (context.tokens) free(context.tokens);
                    return NULL;
                }
                buffer[0] = in_syntax[offset];
                buffer[1] = 0;
                if (!_append_token(&context, buffer))
                {
                    if (context.tokens) free(context.tokens);
                    return NULL;
                }
                buffer = NULL;
            }
            
            /* prepare for next text token */
            token_start = offset + 1;
        }
    }
    
    /* complete token list */
    _append_token(&context, NULL);
    return context.tokens;
}





static XTECmdPattern* _pattern_node_create(enum XTECmdPatternType in_type)
{
    XTECmdPattern *node = calloc(1, sizeof(struct XTECmdPattern));
    if (!node) return NULL;
    node->type = in_type;
    return node;
}


static XTECmdPattern* _pattern_node_create_literal(char *in_word)
{
    XTECmdPattern *node = _pattern_node_create(XTE_CMDPAT_LITERAL);
    if (!node) return NULL;
    node->payload.literal = _clone_cstr(in_word);
    return node;
}


static void _pattern_node_destroy(XTECmdPattern *in_node)
{
    if (!in_node) return;
    
    //if ( (in_node->type == XTE_CMDPAT_LITERAL) || (in_node->type == XTE_CMDPAT_PARAM) )
    //{
        if (in_node->payload.literal) free(in_node->payload.literal);
    //}
    for (int i = 0; i < in_node->child_count; i++)
        if (in_node->children[i]) _pattern_node_destroy(in_node->children[i]);
    if (in_node->children) free(in_node->children);
    
    char **stop_words = in_node->stop_words;
    if (stop_words)
    {
        while (*stop_words)
        {
            free(*stop_words);
            stop_words++;
        }
        free(in_node->stop_words);
    }
    
    free(in_node);
}
/*
 struct XTECmdPattern{
 enum XTECmdPatternType   type;
 int child_count;
 XTECmdPattern **children;
 union
 {
 char *literal;
 char *name;
 } payload;
 char **stop_words;
 };

 */


static int _pattern_node_append(XTECmdPattern *in_node, XTECmdPattern *in_child)
{
    XTECmdPattern **new_children = realloc(in_node->children, sizeof(XTECmdPattern*) * (in_node->child_count + 1));
    if (!new_children) return 0;
    in_node->children = new_children;
    new_children[in_node->child_count++] = in_child;
    return 1;
}


static int _pattern_node_append_param_word(XTECmdPattern *in_node, char *in_word)
{
    long word_len, existing_len;
    word_len = strlen(in_word);
    if (in_node->payload.literal) existing_len = strlen(in_node->payload.literal);
    else existing_len = 0;
    char *concatenation = realloc(in_node->payload.literal, word_len + existing_len + 1);
    if (!concatenation) return 0;
    in_node->payload.literal = concatenation;
    strcpy(concatenation + existing_len, in_word);
    return 1;
}


static void _parse_bnf(XTECmdPattern *io_pattern, struct BNFToken **io_tokens, XTECmdPattern *in_set, XTECmdPattern *in_element)
{
    /* check for end of token stream */
    while ( (*io_tokens)->text )
    {
        /* process list */
        if (io_pattern->type == XTE_CMDPAT_LIST)
        {
            /* append <params> */
            if ((*io_tokens)->text[0] == '<')
            {
                XTECmdPattern *param_node = _pattern_node_create(XTE_CMDPAT_PARAM);
                _pattern_node_append(io_pattern, param_node);
                (*io_tokens)++;
                while ( (*io_tokens)->text && ((*io_tokens)->text[0] != '>') )
                {
                    _pattern_node_append_param_word(param_node, (*io_tokens)->text);
                    (*io_tokens)++;
                }
                if ((*io_tokens)->text)
                    (*io_tokens)++;
            }
            
            /* `name` for namable element */
            else if ( ((*io_tokens)->text[0] == '`') && in_set && in_element )
            {
                (*io_tokens)++;
                if (!in_set->payload.name)
                {
                    while ( (*io_tokens)->text && ((*io_tokens)->text[0] != '`') )
                    {
                        _pattern_node_append_param_word(in_set, (*io_tokens)->text);
                        (*io_tokens)++;
                    }
                }
                else
                {
                    while ( (*io_tokens)->text && ((*io_tokens)->text[0] != '`') )
                    {
                        _pattern_node_append_param_word(in_element, (*io_tokens)->text);
                        (*io_tokens)++;
                    }
                }
                if ((*io_tokens)->text)
                    (*io_tokens)++;
            }
            
            /* check for exit of current list; beginning of another */
            else if ((*io_tokens)->text[0] == '|')
            {
                (*io_tokens)++;
                return;
            }
            
            /* handle set begin */
            else if ( ((*io_tokens)->text[0] == '[') || ((*io_tokens)->text[0] == '{') )
            {
                XTECmdPattern *set_node = _pattern_node_create( (((*io_tokens)->text[0] == '[') ?
                                                                 XTE_CMDPAT_SET_OPT : XTE_CMDPAT_SET_REQ) );
                (*io_tokens)++;
                
                _pattern_node_append(io_pattern, set_node);
                _parse_bnf(set_node, io_tokens, set_node, NULL);
            }
            
            /* handle set end */
            else if ( ((*io_tokens)->text[0] == ']') || ((*io_tokens)->text[0] == '}') )
            {
                return;
            }
            
            /* append words and ,  */
            else if ( ((*io_tokens)->text[0] == ',') || isalpha((*io_tokens)->text[0]) )
            {
                _pattern_node_append(io_pattern, _pattern_node_create_literal( (*io_tokens)->text ));
                (*io_tokens)++;
            }
        }
        
        /* if we're in a SET, begin by creating another LIST and branching to it to continue */
        else if ((io_pattern->type == XTE_CMDPAT_SET_OPT) || (io_pattern->type == XTE_CMDPAT_SET_REQ))
        {
            /* handle set end */
            if ( ((*io_tokens)->text[0] == ']') || ((*io_tokens)->text[0] == '}') )
            {
                (*io_tokens)++;
                return;
            }
            
            /* handle set continuation */
            XTECmdPattern *list_node = _pattern_node_create(XTE_CMDPAT_LIST);
            _pattern_node_append(io_pattern, list_node);
            _parse_bnf(list_node, io_tokens, in_set, list_node);
        }
    }
}




XTECmdPattern* _xte_bnf_parse(const char *in_syntax)
{
    
    
    XTECmdPattern *result = _pattern_node_create(XTE_CMDPAT_SET_REQ);
    if (!result) return NULL;
    
    struct BNFToken *tokens = _tokenize(in_syntax);
    if (!tokens)
    {
        _pattern_node_destroy(result);
        return NULL;
    }
    
    struct BNFToken *token = tokens;
    _parse_bnf(result, &token, NULL, NULL);
    
    
    /*
    if (memcmp(in_syntax, "go ", 3) == 0)
    {
        printf("Syntax: %s\n\n", in_syntax);
        _xte_bnf_debug(result);
       // _xte_bnf_destroy(result);
        //result = _pattern_node_create(XTE_CMDPAT_LIST);
    }*/
    
    _attach_stop_words(result, NULL);
    
    
    token = tokens;
    while ( token->text )
    {
        free((token++)->text);
    }
    free(tokens);
    
    return result;
}


void _xte_bnf_destroy(XTECmdPattern *in_bnf)
{
    _pattern_node_destroy(in_bnf);
}


static const char* _indentation(int in_amount)
{
    const char *whitespace = "                                                                                     ";
    static char buffer[100];
    strcpy(buffer, whitespace);
    buffer[in_amount * 4] = 0;
    return buffer;
}


static void _print_bnf(XTECmdPattern *in_node, int in_indent)
{
    if (!in_node) return;
    switch (in_node->type)
    {
        case XTE_CMDPAT_SET_REQ:
            printf("%sSET-OR-REQUIRED\n", _indentation(in_indent));
            if (in_node->payload.name) printf("%s `%s`\n", _indentation(in_indent), in_node->payload.name);
            for (int i = 0; i < in_node->child_count; i++)
                _print_bnf(in_node->children[i], in_indent + 1);
            break;
        case XTE_CMDPAT_SET_OPT:
            printf("%sSET-OR-OPTIONAL\n", _indentation(in_indent));
            if (in_node->payload.name) printf("%s `%s`\n", _indentation(in_indent), in_node->payload.name);
            for (int i = 0; i < in_node->child_count; i++)
                _print_bnf(in_node->children[i], in_indent + 1);
            break;
        case XTE_CMDPAT_LIST:
            printf("%sELEMENT-TERMS\n", _indentation(in_indent));
            if (in_node->payload.name) printf("%s `%s`\n", _indentation(in_indent), in_node->payload.name);
            for (int i = 0; i < in_node->child_count; i++)
                _print_bnf(in_node->children[i], in_indent + 1);
            break;
        case XTE_CMDPAT_LITERAL:
            printf("%sLITERAL \"%s\"\n", _indentation(in_indent), in_node->payload.literal);
            break;
        case XTE_CMDPAT_PARAM:
            printf("%sPARAM \"%s\"\n", _indentation(in_indent), in_node->payload.name);
            char **stop_words = in_node->stop_words;
            if (stop_words)
            {
                while (*stop_words)
                {
                    printf("%sSTOP-WORD: %s\n", _indentation(in_indent+1), *stop_words);
                    stop_words++;
                }
            }
            break;
    }
}


void _xte_bnf_debug(XTECmdPattern *in_bnf)
{
    _print_bnf(in_bnf, 0);
}




