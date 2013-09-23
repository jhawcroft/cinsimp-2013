/*
 
 xTalk Engine Abstract Syntax Trees
 xtalk_lexer.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Manipulation, creation, cleanup and serialisation of abstract syntax trees as produced by the 
 lexer and parser components of the engine
 
 */

#include "xtalk_internal.h"

// AST serialization could be a useful feature along with runtime caching
// to avoid continuous re-compilation of routines, without resorting to an additional translation phase


XTEAST* _xte_ast_create(XTE *in_engine, XTEASTType in_type)
{
    //printf("created AST node\n");
    XTEAST *tree = calloc(sizeof(struct XTEAST), 1);
    if (!tree) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    tree->type = in_type;
    tree->engine = in_engine;
    return tree;
}


XTEAST* _xte_ast_create_word(XTE *in_engine, const char *in_word)
{
    XTEAST *word = _xte_ast_create(in_engine, XTE_AST_WORD);
    if (!word) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    word->value.string = _xte_clone_cstr(in_engine, in_word);
    return word;
}


XTEAST* _xte_ast_create_literal_integer(XTE *in_engine, int in_integer)
{
    XTEAST *intg = _xte_ast_create(in_engine, XTE_AST_LITERAL_INTEGER);
    if (!intg) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    intg->value.integer = in_integer;
    return intg;
}


XTEAST* _xte_ast_create_literal_real(XTE *in_engine, double in_real)
{
    XTEAST *intg = _xte_ast_create(in_engine, XTE_AST_LITERAL_REAL);
    if (!intg) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    intg->value.real = in_real;
    return intg;
}


XTEAST* _xte_ast_create_operator(XTE *in_engine, XTEASTOperator in_operator)
{
    XTEAST *op = _xte_ast_create(in_engine, XTE_AST_OPERATOR);
    if (!op) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    op->value.op = in_operator;
    return op;
}


XTEAST* _xte_ast_create_keyword(XTE *in_engine, XTEASTKeyword in_keyword)
{
    XTEAST *op = _xte_ast_create(in_engine, XTE_AST_KEYWORD);
    if (!op) return NULL;
    op->value.keyword = in_keyword;
    return op;
}


XTEAST* _xte_ast_create_literal_string(XTE *in_engine, const char *in_string, int in_flags)
{
    XTEAST *str = _xte_ast_create(in_engine, XTE_AST_LITERAL_STRING);
    if (!str) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    if (in_flags & XTE_AST_FLAG_NOCOPY) str->value.string = (char*)in_string;
    else
    {
        str->value.string = malloc(strlen(in_string) + 1);
        if (!(str->value.string)) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
        strcpy(str->value.string, in_string);
    }
    return str;
}


XTEAST* _xte_ast_create_comment(XTE *in_engine, const char *in_string, int in_flags)
{
    XTEAST *str = _xte_ast_create(in_engine, XTE_AST_COMMENT);
    if (!str) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    if (in_flags & XTE_AST_FLAG_NOCOPY) str->value.string = (char*)in_string;
    else
    {
        str->value.string = malloc(strlen(in_string) + 1);
        if (!(str->value.string)) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
        strcpy(str->value.string, in_string);
    }
    return str;
}


XTEAST* _xte_ast_create_pointer(XTE *in_engine, void *in_ptr, char *in_note)
{
    XTEAST *intg = _xte_ast_create(in_engine, XTE_AST_POINTER);
    if (!intg) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    intg->value.ptr = in_ptr;
    if (in_note) intg->note = _xte_clone_cstr(in_engine, in_note);
    return intg;
}
/*
 typedef enum XTEASTType
 {
 XTE_AST_LIST,
 XTE_AST_LITERAL_STRING,
 XTE_AST_LITERAL_BOOLEAN,
 XTE_AST_LITERAL_REAL,
 XTE_AST_LITERAL_INTEGER,
 XTE_AST_EXPRESSION,
 XTE_AST_FUNCTION,
 XTE_AST_WORD,
 XTE_AST_PAREN_OPEN,
 XTE_AST_PAREN_CLOSE,
 XTE_AST_CONSTANT,
 XTE_AST_REF,
 XTE_AST_PROPERTY,
 XTE_AST_POINTER,
 XTE_AST_COMMAND,
 XTE_AST_NEWLINE,
 XTE_AST_OPERATOR,
 XTE_AST_OF,
 XTE_AST_IN,
XTE_AST_COMMA,
XTE_AST_KEYWORD,

} XTEASTType;
*/

void _xte_ast_destroy(XTEAST *in_tree)
{
    if (!in_tree) return;
    //printf("destroyed AST node\n");
    for (int i = 0; i < in_tree->children_count; i++)
    {
        if (in_tree->children[i]) _xte_ast_destroy(in_tree->children[i]);
    }
    if (in_tree->children) free(in_tree->children);
    switch (in_tree->type)
    {
        case XTE_AST_LIST:
        case XTE_AST_EXPRESSION:
        case XTE_AST_LITERAL_STRING:
        case XTE_AST_WORD:
        case XTE_AST_COMMENT:
            if (in_tree->value.string) free(in_tree->value.string);
            break;
        case XTE_AST_HANDLER:
            if (in_tree->value.handler.name) free(in_tree->value.handler.name);
            break;
        case XTE_AST_COMMAND:
            if (in_tree->value.command.named) free(in_tree->value.command.named);
            break;
        case XTE_AST_FUNCTION:
            if (in_tree->value.function.named) free(in_tree->value.function.named);
            break;
            
        default:
            break;
    }
    if (in_tree->note) free(in_tree->note);
    free(in_tree);
}


void _xte_ast_list_insert(XTEAST *in_list, int in_offset, XTEAST *in_tree)
{
    int new_children_count = in_list->children_count + 1;
    XTEAST **new_children = realloc(in_list->children, sizeof(XTEAST*) * new_children_count);
    if (!new_children) return _xte_panic_void(in_list->engine, XTE_ERROR_MEMORY, NULL);
    in_list->children = new_children;
    memmove(new_children + in_offset + 1, new_children + in_offset, sizeof(XTEAST*) * (in_list->children_count - in_offset));
    new_children[in_offset] = in_tree;
    in_list->children_count++;
}


void _xte_ast_list_append(XTEAST *in_list, XTEAST *in_tree)
{
    int new_children_count = in_list->children_count + 1;
    XTEAST **new_children = realloc(in_list->children, sizeof(XTEAST*) * new_children_count);
    if (!new_children) return _xte_panic_void(in_list->engine, XTE_ERROR_MEMORY, NULL);
    in_list->children = new_children;
    new_children[new_children_count-1] = in_tree;
    in_list->children_count++;
}


XTEAST* _xte_ast_list_remove(XTEAST *in_list, long in_index)
{
    XTEAST *removed_node;
    if ((in_index < 0) || (in_index >= in_list->children_count)) return NULL;
    removed_node = in_list->children[in_index];
    memmove(in_list->children + in_index, in_list->children + in_index + 1, sizeof(XTEAST*) * (in_list->children_count - in_index - 1));
    in_list->children_count--;
    return removed_node;
}


XTEAST* _xte_ast_list_child(XTEAST *in_list, int in_index)
{
    if ((in_index < 0) || (in_index >= in_list->children_count)) return NULL;
    return in_list->children[in_index];
}


void _xte_ast_swap_ptrs(XTEAST *in_ast1, XTEAST *in_ast2)
{
    XTEAST temp_node;
    temp_node = *in_ast1;
    *in_ast1 = *in_ast2;
    *in_ast2 = temp_node;
}


void _xte_ast_list_set_last_offset(XTEAST *in_list, long in_source_offset)
{
    assert(in_list != NULL);
    assert(in_list->children_count > 0);
    
    XTEAST *last_node = in_list->children[in_list->children_count-1];
    last_node->source_offset = in_source_offset;
}


void _xte_ast_list_set_line_offsets(XTEAST *in_list, long in_source_offset, int in_source_line, int in_logical_line)
{
    assert(in_list != NULL);
    assert(in_list->children_count > 0);
    XTEAST *last_node = in_list->children[in_list->children_count-1];
    last_node->source_offset = in_source_offset;
    last_node->source_line = in_source_line;
    last_node->logical_line = in_logical_line;
}


XTEAST* _xte_ast_list_last(XTEAST *in_list)
{
    if (in_list->children_count == 0) return NULL;
    return in_list->children[in_list->children_count-1];
}






#ifdef XTALK_TESTS

static const char* _ast_indent(int in_indent)
{
    static char buffer[1024];
    static char *spaces = "                                                                                                    ";
    in_indent *= 3;
    memcpy(buffer, spaces, in_indent);
    buffer[in_indent] = 0;
    return buffer;
}


struct XTEASTDebugContext
{
    XTE *engine;
    char *buffer;
    int buffer_allocated;
    int buffer_length;
    int flags;
};

#define XTE_AST_DEBUG_BUFFER_ALLOC 4096


static void _xte_ast_debug_vwrite(struct XTEASTDebugContext *in_context, const char *in_msg_format, va_list in_args)
{
    const char *text = _xte_cstr_format_fill(in_msg_format, in_args);
    int length = (int)strlen(text);
    if (in_context->buffer_length + length > in_context->buffer_allocated)
    {
        char *new_buffer = realloc(in_context->buffer, in_context->buffer_length + length + XTE_AST_DEBUG_BUFFER_ALLOC);
        if (!new_buffer) _xte_panic_void(in_context->engine, XTE_ERROR_MEMORY, NULL);
        in_context->buffer = new_buffer;
        in_context->buffer_allocated = in_context->buffer_length + length + XTE_AST_DEBUG_BUFFER_ALLOC;
    }
    strcpy(in_context->buffer + in_context->buffer_length, text);
    in_context->buffer_length += length;
}


static void _ast_printf(struct XTEASTDebugContext *in_context, const char *in_msg_format, ...)
{
    va_list args;
    va_start(args, in_msg_format);
    _xte_ast_debug_vwrite(in_context, in_msg_format, args);
    va_end(args);
}


static void _xte_ast_debug_subtree(struct XTEASTDebugContext *in_context, XTEAST *in_tree, int in_indent)
{
    if (!in_tree)
    {
        _ast_printf(in_context, "%sNULL\n", _ast_indent(in_indent));
        return;
    }
    
    if (in_context->flags & XTE_AST_DEBUG_NODE_ADDRESSES)
        _ast_printf(in_context, "%s<%x>\n", _ast_indent(in_indent), (long)in_tree);
    
    if (in_tree->flags & XTE_AST_FLAG_IS_CHECKPOINT)
        _ast_printf(in_context, "%s! CHECKPOINT: !  (source line: %d)\n", _ast_indent(in_indent), in_tree->source_line);
    //else if (in_tree->source_line != INVALID_LINE_NUMBER)
     //   _ast_printf(in_context, "%s (source line: %d)\n", _ast_indent(in_indent), in_tree->source_line);
    
    switch (in_tree->type)
    {
        case XTE_AST_EXPRESSION:
            _ast_printf(in_context, "%sEXPRESSION (%s)\n", _ast_indent(in_indent), in_tree->value.string);
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        case XTE_AST_COMMA:
            _ast_printf(in_context, "%sCOMMA\n", _ast_indent(in_indent));
            break;
        case XTE_AST_OF:
            _ast_printf(in_context, "%sOF\n", _ast_indent(in_indent));
            break;
        case XTE_AST_IN:
            _ast_printf(in_context, "%sIN\n", _ast_indent(in_indent));
            break;
        case XTE_AST_PAREN_OPEN:
            _ast_printf(in_context, "%sPAREN-OPEN\n", _ast_indent(in_indent));
            break;
        case XTE_AST_PAREN_CLOSE:
            _ast_printf(in_context, "%sPAREN-CLOSE\n", _ast_indent(in_indent));
            break;
        case XTE_AST_LITERAL_BOOLEAN:
            _ast_printf(in_context, "%sBOOLEAN %s\n", _ast_indent(in_indent), (in_tree->value.boolean?"TRUE":"FALSE"));
            break;
        case XTE_AST_NEWLINE:
            _ast_printf(in_context, "%sNEWLINE\n", _ast_indent(in_indent));
            break;
        case XTE_AST_POINTER:
            _ast_printf(in_context, "%sfPTR=$%x (%s)\n", _ast_indent(in_indent),
                        (in_context->flags & XTE_AST_DEBUG_NO_POINTERS?0:(long)(in_tree->value.ptr)), in_tree->note);
            break;
        case XTE_AST_COMMAND:
            _ast_printf(in_context, "%sCOMMAND fPTR=$%x (%s)\n", _ast_indent(in_indent),
                        (in_context->flags & XTE_AST_DEBUG_NO_POINTERS?0:(long)(in_tree->value.ptr)), in_tree->note);
            _ast_printf(in_context, "%sPARAMS:\n", _ast_indent(in_indent));
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        case XTE_AST_FUNCTION:
            _ast_printf(in_context, "%sFUNCTION fPTR=$%x (%s)\n", _ast_indent(in_indent),
                        (in_context->flags & XTE_AST_DEBUG_NO_POINTERS?0:(long)(in_tree->value.ptr)), in_tree->note);
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        case XTE_AST_PROPERTY:
            _ast_printf(in_context, "%sPROPERTY fPTR-map=%d rep=%d (%s)\n", _ast_indent(in_indent),
                        (in_context->flags & XTE_AST_DEBUG_NO_POINTERS?0:in_tree->value.property.pmap_entry),
                        in_tree->value.property.representation, in_tree->note);
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        case XTE_AST_REF:
            _ast_printf(in_context, "%sREFERENCE %s %s%s(%s)\n", _ast_indent(in_indent), in_tree->value.ref.type,
                   (in_tree->value.ref.is_range?"[RANGE] ":""),
                   (in_tree->value.ref.is_collection?"[COLLECTION] ":""), in_tree->note);
            if (in_tree->value.ref.get_by_number)
                _ast_printf(in_context, "%s  INTEGER fPTR=$%x\n", _ast_indent(in_indent),
                            (in_context->flags & XTE_AST_DEBUG_NO_POINTERS?0:in_tree->value.ref.get_by_number));
            if (in_tree->value.ref.get_by_string)
                _ast_printf(in_context, "%s  STRING  fPTR=$%x\n", _ast_indent(in_indent),
                            (in_context->flags & XTE_AST_DEBUG_NO_POINTERS?0:in_tree->value.ref.get_by_string));
            //if (in_tree->value.ref.is_range)
            //if (in_tree->children[0])
            //    printf("%s   IDENT TYPE=$%X\n", _ast_indent(in_indent), in_tree->children[0]->value.integer);
           // if (in_tree->flags & XTE_AST_FLAG_ORDINAL)
             //   printf("%s   ORDINAL=%d\n", _ast_indent(in_indent), in_tree->value.integer);
            
            
                //_ast_print(in_tree->children[1], in_indent+1);
                for (int i = 0; i < in_tree->children_count; i++)
                {
                    _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
                }
                //_ast_print(in_tree, in_indent+1);
                //for (int i = 0; i < in)
            
            
            //
            break;
        case XTE_AST_LITERAL_INTEGER:
            _ast_printf(in_context, "%sINTEGER %d\n", _ast_indent(in_indent), in_tree->value.integer);
            break;
        case XTE_AST_LITERAL_REAL:
            _ast_printf(in_context, "%sREAL %f\n", _ast_indent(in_indent), in_tree->value.real);
            break;
        case XTE_AST_KEYWORD:
        {
            char *op_desc = NULL;
            switch (in_tree->value.keyword)
            {
                case XTE_AST_KW_RETURN:
                    op_desc = "return";
                    break;
                case XTE_AST_KW_NONE:
                    op_desc = "???";
                    break;
                case XTE_AST_KW_GLOBAL:
                    op_desc = "global";
                    break;
                case XTE_AST_KW_ELSE:
                    op_desc = "else";
                    break;
                case XTE_AST_KW_END:
                    op_desc = "end";
                    break;
                case XTE_AST_KW_EXIT:
                    op_desc = "exit";
                    break;
                case XTE_AST_KW_FUNCTION:
                    op_desc = "function";
                    break;
                case XTE_AST_KW_IF:
                    op_desc = "if";
                    break;
                case XTE_AST_KW_NEXT:
                    op_desc = "next";
                    break;
                case XTE_AST_KW_ON:
                    op_desc = "on";
                    break;
                case XTE_AST_KW_PASS:
                    op_desc = "pass";
                    break;
                case XTE_AST_KW_REPEAT:
                    op_desc = "repeat";
                    break;
                case XTE_AST_KW_THEN:
                    op_desc = "then";
                    break;
            }
            _ast_printf(in_context, "%sKEYWORD %s\n", _ast_indent(in_indent), op_desc);
            break;
        }
        case XTE_AST_OPERATOR:
        {
            char *op_desc = NULL;
            switch (in_tree->value.op)
            {
                case XTE_AST_OP_THERE_IS_A:
                    op_desc = "there is a";
                    break;
                case XTE_AST_OP_THERE_IS_NO:
                    op_desc = "there is no";
                    break;
                case XTE_AST_OP_EQUAL://is, =
                    op_desc = "equal";
                    break;
                case XTE_AST_OP_NOT_EQUAL://is not, <>
                    op_desc = "not equal";
                    break;
                case XTE_AST_OP_GREATER://>
                    op_desc = "greater than";
                    break;
                case XTE_AST_OP_LESSER://<
                    op_desc = "less than";
                    break;
                case XTE_AST_OP_LESS_EQ://<=
                    op_desc = "less or equal";
                    break;
                case XTE_AST_OP_GREATER_EQ://>=
                    op_desc = "greater or equal";
                    break;
                case XTE_AST_OP_CONCAT://&
                    op_desc = "concat";
                    break;
                case XTE_AST_OP_CONCAT_SP://&&
                    op_desc = "concat space";
                    break;
                case XTE_AST_OP_EXPONENT://^
                    op_desc = "exponent";
                    break;
                case XTE_AST_OP_MULTIPLY://*
                    op_desc = "multiply";
                    break;
                case XTE_AST_OP_DIVIDE_FP:///
                    op_desc = "divide fp";
                    break;
                case XTE_AST_OP_ADD://+
                    op_desc = "add";
                    break;
                case XTE_AST_OP_SUBTRACT://-
                    op_desc = "subtract";
                    break;
                case XTE_AST_OP_NEGATE://-
                    op_desc = "negate";
                    break;
                case XTE_AST_OP_IS_IN: // reverse operands for "contains"
                    op_desc = "is in";
                    break;
                case XTE_AST_OP_CONTAINS:
                    op_desc = "contains";
                    break;
                case XTE_AST_OP_IS_NOT_IN:
                    op_desc = "is not in";
                    break;
                case XTE_AST_OP_DIVIDE_INT://div
                    op_desc = "divide int";
                    break;
                case XTE_AST_OP_MODULUS://mod
                    op_desc = "modulus";
                    break;
                case XTE_AST_OP_AND://and
                    op_desc = "and";
                    break;
                case XTE_AST_OP_OR://or
                    op_desc = "or";
                    break;
                case XTE_AST_OP_NOT://not
                    op_desc = "not";
                    break;
                case XTE_AST_OP_IS_WITHIN://is within (geometric)
                    op_desc = "is within";
                    break;
                case XTE_AST_OP_IS_NOT_WITHIN://is not within (geometric)
                    op_desc = "is not within";
                    break;
                default:
                    return _xte_panic_void(in_tree->engine, XTE_ERROR_INTERNAL, NULL);
            }
            _ast_printf(in_context, "%sOPERATOR %s\n", _ast_indent(in_indent), op_desc);
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        }
        case XTE_AST_LITERAL_STRING:
            _ast_printf(in_context, "%sSTRING \"%s\"\n", _ast_indent(in_indent), in_tree->value.string);
            break;
        case XTE_AST_WORD:
            _ast_printf(in_context, "%sWORD %s (FLAGS=$%x)\n", _ast_indent(in_indent), in_tree->value.string, in_tree->flags);
            if ((in_tree->flags & XTE_AST_FLAG_SYNREP) && in_tree->children)
            {
                //printf("%s  ALTERNATIVE PHRASE:\n", _ast_indent(in_indent));
                //_ast_print(in_tree->children[0], in_indent+1);
            }
            break;
        case XTE_AST_CONSTANT:
            _ast_printf(in_context, "%sCONSTANT fPTR=$%x (%s)\n", _ast_indent(in_indent),
                        (in_context->flags & XTE_AST_DEBUG_NO_POINTERS?0:(long)(in_tree->value.ptr)),
                        in_tree->note);
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        case XTE_AST_LIST:
        {
            _ast_printf(in_context, "%sLIST (%s)\n", _ast_indent(in_indent), in_tree->value.string);
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        }
        case XTE_AST_COMMENT:
        {
            _ast_printf(in_context, "%sCOMMENT \"%s\"\n", _ast_indent(in_indent), in_tree->value.string);
            break;
        }
        case XTE_AST_SPACE:
            _ast_printf(in_context, "%sSPACE\n", _ast_indent(in_indent));
            break;
        case XTE_AST_LINE_CONT:
            _ast_printf(in_context, "%sLINE-CONT\n", _ast_indent(in_indent));
            break;
        case XTE_AST_HANDLER:
            _ast_printf(in_context, "%sHANDLER %s \"%s\"\n", _ast_indent(in_indent),
                        (in_tree->value.handler.is_func ? "F" : "C"), in_tree->value.handler.name);
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        case XTE_AST_CONDITION:
            _ast_printf(in_context, "%sCONDITION\n", _ast_indent(in_indent));
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        case XTE_AST_LOOP:
        {
            const char * desc = "forever";
            switch (in_tree->value.loop)
            {
                case XTE_AST_LOOP_NUMBER: desc = "number"; break;
                case XTE_AST_LOOP_UNTIL: desc = "until"; break;
                case XTE_AST_LOOP_WHILE: desc = "while"; break;
                case XTE_AST_LOOP_COUNT_UP: desc = "count up"; break;
                case XTE_AST_LOOP_COUNT_DOWN: desc = "count down"; break;
            }
            _ast_printf(in_context, "%sLOOP %s\n", _ast_indent(in_indent), desc);
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        }
        case XTE_AST_GLOBAL_DECL:
            _ast_printf(in_context, "%sGLOBAL-DELC\n", _ast_indent(in_indent));
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        case XTE_AST_EXIT:
        {
            const char * desc = NULL;
            switch (in_tree->value.loop)
            {
                case XTE_AST_EXIT_EVENT: desc = "event"; break;
                case XTE_AST_EXIT_HANDLER: desc = "handler (return)"; break;
                case XTE_AST_EXIT_LOOP: desc = "loop"; break;
                case XTE_AST_EXIT_ITERATION: desc = "iteration"; break;
                case XTE_AST_EXIT_PASSING: desc = "handler (pass)"; break;
            }
            _ast_printf(in_context, "%sEXIT %s\n", _ast_indent(in_indent), desc);
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        }
        case XTE_AST_PARAM_NAMES:
            _ast_printf(in_context, "%sPARAM-NAMES\n", _ast_indent(in_indent));
            for (int i = 0; i < in_tree->children_count; i++)
            {
                _xte_ast_debug_subtree(in_context, in_tree->children[i], in_indent+1);
            }
            break;
        //default:
        //    break;
    }
}


static char *_g_ast_debug_buffer = NULL;


static const char* _quote_string_lines(const char *in_string)
{
    long length = strlen(in_string);
    int line_quote_count = 0;
    long line_offset = 0;
    long line_length;
    static char *output_buffer = NULL;
    if (output_buffer) free(output_buffer);
    output_buffer = NULL;
    char *new_output_buffer;
    long output_buffer_size = 0;
    for (long i = 0; i <= length; i++)
    {
        if (in_string[i] == '"') line_quote_count++;
        else if ((in_string[i] == '\n') || (i == length))
        {
            if ((in_string[i] != '\n') && (i == length) && (i - line_offset == 0)) break;
            line_length = i - line_offset;
            long line_alloc = line_length + line_quote_count + 5;
            new_output_buffer = realloc(output_buffer, output_buffer_size + line_alloc + 1);
            if (!new_output_buffer)
            {
                if (output_buffer) free(output_buffer);
                return NULL;
            }
            output_buffer = new_output_buffer;
            long output_ptr = output_buffer_size;
            output_buffer_size += line_alloc;
            output_buffer[output_ptr++] = '"';
            for (int c = 0; c < line_length; c++)
            {
                if (in_string[line_offset + c] == '"')
                    output_buffer[output_ptr++] = '\\';
                output_buffer[output_ptr++] = in_string[line_offset + c];
            }
            if (in_string[i] == '\n')
            {
                output_buffer[output_ptr++] = '\\';
                output_buffer[output_ptr++] = 'n';
            }
            output_buffer[output_ptr++] = '"';
            output_buffer[output_ptr++] = '\n';
            output_buffer[output_ptr++] = 0;
            line_offset = i + 1;
            line_quote_count = 0;
            if (i == length) break;
        }
    }
    return output_buffer;
}


const char* _xte_ast_debug_text(XTEAST *in_tree, int in_flags)
{
    if (_g_ast_debug_buffer) free(_g_ast_debug_buffer);
    _g_ast_debug_buffer = NULL;
    
    if (!in_tree) return NULL;
    
    struct XTEASTDebugContext context;
    context.engine = in_tree->engine;
    context.buffer = NULL;
    context.buffer_allocated = 0;
    context.buffer_length = 0;
    context.flags = in_flags;
    
    _xte_ast_debug_subtree(&context, in_tree, 0);
    
    if ((in_flags & XTE_AST_DEBUG_QUOTED_OUTPUT) && context.buffer_length)
    {
        _g_ast_debug_buffer = _xte_clone_cstr(in_tree->engine, _quote_string_lines(context.buffer));
        if (context.buffer) free(context.buffer);
    }
    else
        _g_ast_debug_buffer = context.buffer;
    return _g_ast_debug_buffer;
}


void _xte_ast_debug(XTEAST *in_tree)
{
    _xte_ast_debug_text(in_tree, 0);
    printf("%s", _g_ast_debug_buffer);
}


#endif


