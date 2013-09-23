/*
 
 xTalk Engine - Internal Header
 xtalk_internal.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Internal types and assorted function prototypes for various inter-related modules of the engine
 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#include "xtalk_engine.h"
#include "xtalk_limits.h"
#include "xtalk_test_int.h"


#ifndef GeneralxTalkEngine_xtalk_internal_h
#define GeneralxTalkEngine_xtalk_internal_h



#define _XTE_STRUCT_ID "XTE_"


#define IS_LINE_NUMBER(x) ((x > 0) && (x < XTALK_LIMIT_MAX_SOURCE_LINES))
#define IS_BOOL(x) ((x == !0) || (x == !!0))
#define IS_XTE(x) ((x != NULL) && (strcmp(x->struct_id, _XTE_STRUCT_ID) == 0))
#define IS_COUNT(x) ((x >= 0) && (x <= XTALK_LIMIT_SANE_ITEM_COUNT))



#define REAL_THRESHOLD 0.00000000001






/* so that memory that is not able to be deallocated at dispose time;
 but doesn't constitute a leak, eg. first_word() isn't counted as a leak */
void* _xte_malloc_static(long in_bytes);
void _xte_free_static(void *in_mem);


/**********
 Variant
 */

typedef struct XTEAST XTEAST;

struct XTEVariant
{
    XTEVariantType type;
    union
    {
        char *utf8_string;
        int integer;
        double real;
        int boolean;
        struct
        {
            struct XTEClassInt *type;
            void *ident;
            XTEObjRefDeallocator deallocator;
        } ref;
        struct
        {
            struct XTEPropertyPtr *ptrs;
            XTEVariant *owner;
            XTEPropRep rep;
        } prop;
        XTEAST *ast;
        
    } value;
    int ref_count;
};



/**********
 BNF
 */

enum XTECmdPatternType
{
    XTE_CMDPAT_LIST,
    XTE_CMDPAT_LITERAL,
    XTE_CMDPAT_PARAM,
    XTE_CMDPAT_SET_OPT,
    XTE_CMDPAT_SET_REQ,
};

typedef struct XTECmdPattern XTECmdPattern;

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


XTECmdPattern* _xte_bnf_parse(const char *in_syntax);

void _xte_bnf_destroy(XTECmdPattern *in_bnf);

void _xte_bnf_debug(XTECmdPattern *in_bnf);




/**********
 Not Yet Organised: ***************************
 */

#define XTE_ORD_LAST -1
#define XTE_ORD_MIDDLE -2
#define XTE_ORD_RANDOM -3
#define XTE_ORD_FIRST 1
#define XTE_ORD_THIS 0
#define XTE_ORD_NEXT -4
#define XTE_ORD_PREV -5

#define IS_ORDINAL(x) ((x >= -5) && (x <= 1))
#define IS_ELEMENT_NUMBER(x, c) ((x >= 1) && (x <= c))

typedef int XTEOrdinal;





/*
typedef enum XTETokenType
{
    XTE_TOKEN_WORD,
    XTE_TOKEN_LITERAL_STRING,
    XTE_TOKEN_LITERAL_BOOLEAN,
    XTE_TOKEN_LITERAL_NUMBER,
    XTE_TOKEN_EXPRESSION,
    
} XTETokenType;

typedef struct XTEToken
{
    XTETokenType type;
    
} XTEToken;
*/

#define XTE_AST_FLAG_SYNREP 0x1
#define XTE_AST_FLAG_ORDINAL 0x2
#define XTE_AST_FLAG_RANGE 0x4
#define XTE_AST_FLAG_FINALIZED 0x8


#define XTE_AST_OP_EQUAL            1
#define XTE_AST_OP_NOT_EQUAL        2
#define XTE_AST_OP_GREATER          3
#define XTE_AST_OP_LESSER           4
#define XTE_AST_OP_LESS_EQ          5
#define XTE_AST_OP_GREATER_EQ       6
#define XTE_AST_OP_CONCAT           7
#define XTE_AST_OP_CONCAT_SP        8
#define XTE_AST_OP_EXPONENT         9
#define XTE_AST_OP_MULTIPLY         10
#define XTE_AST_OP_DIVIDE_FP        11
#define XTE_AST_OP_ADD              12
#define XTE_AST_OP_SUBTRACT         13
#define XTE_AST_OP_NEGATE           14
#define XTE_AST_OP_IS_IN            15
#define XTE_AST_OP_CONTAINS         16
#define XTE_AST_OP_IS_NOT_IN        17
#define XTE_AST_OP_DIVIDE_INT       18
#define XTE_AST_OP_MODULUS          19
#define XTE_AST_OP_AND              20
#define XTE_AST_OP_OR               21
#define XTE_AST_OP_NOT              22
#define XTE_AST_OP_IS_WITHIN        23
#define XTE_AST_OP_IS_NOT_WITHIN    24
#define XTE_AST_OP_THERE_IS_A       25
#define XTE_AST_OP_THERE_IS_NO      26

#define IS_OPERATOR(x) ((x >= 1) && (x <= 26))

typedef int XTEASTOperator;


typedef enum XTEASTKeyword
{
    XTE_AST_KW_NONE,
    XTE_AST_KW_END,
    XTE_AST_KW_EXIT,
    XTE_AST_KW_FUNCTION,
    XTE_AST_KW_GLOBAL,
    XTE_AST_KW_IF,
    XTE_AST_KW_THEN,
    XTE_AST_KW_ELSE,
    XTE_AST_KW_NEXT,
    XTE_AST_KW_ON,
    XTE_AST_KW_PASS,
    XTE_AST_KW_REPEAT,
    XTE_AST_KW_RETURN,
    
} XTEASTKeyword;



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
    XTE_AST_IN,/* generally the same as OF, but not always allowed */
    XTE_AST_COMMA,
    XTE_AST_KEYWORD, /* reserved words only */
    
    XTE_AST_SPACE, // ADDED 2013-08-2
    XTE_AST_COMMENT,
    XTE_AST_LINE_CONT,
    
    XTE_AST_HANDLER,
    XTE_AST_PARAM_NAMES,
    XTE_AST_CONDITION,
    XTE_AST_LOOP,
    XTE_AST_GLOBAL_DECL,
    XTE_AST_EXIT,
    
} XTEASTType;


struct XTEASTRef
{
    /* which of these we have determines what types we accept;
     and if both are provided, we choose the most appropriate
     based on the data type of the parameter */
    XTEElementGetter get_by_number;
    XTEElementGetter get_by_string;
    int is_range; /* specific range */
    int is_collection; /* collection */
    char *type; /* type name; used for type lookup - and also for error messages (tho it shouldn't for the later ***) */
    XTEPropertyGetter get_count;
    int params_are_indicies;
};


struct XTECmdInt;


#define XTE_AST_FLAG_DELAYED_EVAL 0x10

#define XTE_AST_FLAG_IS_CHECKPOINT 0x80


#define XTE_AST_LOOP_FOREVER    0
#define XTE_AST_LOOP_NUMBER     1
#define XTE_AST_LOOP_UNTIL      2
#define XTE_AST_LOOP_WHILE      3
#define XTE_AST_LOOP_COUNT_UP   4
#define XTE_AST_LOOP_COUNT_DOWN 5


#define XTE_AST_EXIT_EVENT      0 /* returns control to the user; exits current system event handling message chain */
#define XTE_AST_EXIT_HANDLER    1 /* returns control to the calling handler */
#define XTE_AST_EXIT_PASSING    3 /* passes control to the next handler in the message heirarchy */
#define XTE_AST_EXIT_LOOP       2 /* returns control to the enclosing block of this loop */
#define XTE_AST_EXIT_ITERATION  4 /* passes control to the next iteration of the enclosing loop */


struct XTEAST
{
    XTEASTType type;
    int flags;
    union
    {
        char *string;
        void *ptr;
        int integer;
        struct {
            int pmap_entry;
            XTEPropRep representation;
        } property;
        double real;
        XTEASTOperator op;
        XTEASTKeyword keyword;
        int boolean;
        struct XTEASTRef ref;
        struct {
            char *name;
            int is_func;
        } handler;
        int loop;
        int exit;
        struct {
            void *ptr;
            char *named;
        } command;
        struct {
            void *ptr;
            char *named;
        } function;
    } value;
    char *note;
    int children_count;
    XTEAST **children;
    XTE *engine;
    long source_offset;
    int source_line;
    int logical_line;
};




long _xte_mem_allocated(void);


struct XTESynonymInt
{
    int word_count;
    char **words;
    int rep_word_count;
    char **rep_words;
    char *name;
};


struct XTEConstantInt
{
    int word_count;
    char **words;
    char *name;
    XTEPropertyGetter func_get;
};


struct XTEClassInt
{
    char *name;
    XTEPropertyGetter container_read; /* if a class is a container, it will have a read handler */
    XTEContainerWriter container_write;
    
    int property_count;
    struct XTEPropertyInt **properties; /* table of properties belonging to the class;
                                         primarily used by the refs module to handle references
                                         to class by some unique identifier (name, ID, etc.) */
    
    XTEScriptRetriever script_retr;
    XTENextResponder next_responder;
};


struct XTERefIntUID
{
    char *uid_name;
    char *uid_type;
    XTEElementGetter get; /* get an element of a collection (see below) for a given identifier value */
};


struct XTERefIntOwnerType
{
    char *type;
    /* eventually this could contain the handler functions;
     enabling specific handlers to be invoked depending on the object on which
     the element is being accessed; for now we'll take care of that in our
     environment handler ourselves. 
     we'll still guarantee that the type of owner passed in to the handler is
     only one of the allowed types, or NULL, if global access is available. */
};

/*
 something like what we probably should have eventually, referenced from XTERefInt:
 (multiple refint's for singular + plural referring to the same XTEElementInt)
 
struct XTEElementInt
{
    int owner_type_count;
    struct XTERefIntOwnerType *owner_types;
};
 */


/* represents a collection of objects of a specific class;
 defines how to obtain a single object by one of many defined unique identifiers,
 a range of objects from the collection specified by number,
 a specific object by number,
 or the entire collection. */
struct XTERefInt
{
    int word_count;
    char **words; /* the actual words used in code to access the object(s);
                            eg. "card", "cards", "button", etc. */
    
    int singular; /* single instances and ranges can be obtained using a singular,
                   eg. button 1 to 3 of this card;
                   a collection can be obtained using the plural, 
                   eg. buttons of this card */
    
    char *name; /* what is the name of the collection of objects? eg. "cards" */
    char *type; /* what is the class of the objects? eg. "card", "button", "window", etc.
                 (contained within the collection) */
    
    int owner_type_count;
    struct XTERefIntOwnerType *owner_types; /* what types can contain this element?
                                             or NULL for global */
    
    XTEElementGetter get_range; /* handler to obtain 1+ numbered objects */
    XTEElementGetter get_named; /* handler to obtain a single named object
                                 (the unique identifier is not specified by the script) */
    
    XTEPropertyGetter get_count;
    
    int uid_alt_count;
    struct XTERefIntUID *uid_alt; /* table of unique identifiers that can be used to access
                                   a single element from the collection of objects;
                                   includes function handlers to obtain an object with the
                                   specified identifier */
};


/* these cannot be resolved until runtime;
 there is no point in having a pointer */
struct XTEPropertyInt
{
    int word_count;
    char **words;
    char *name;
    char *usual_type;
    int mapped_ptr;
    int is_unique_id;
    XTEElementGetter owner_inst_for_id; /* gives an instance of the owner class of this property,
                                         for a specified ID value (ie. name, ID, etc.) */
};


struct XTEPropertyPtr
{
    XTEPropertyGetter func_read;
    XTEPropertySetter func_write;
    struct XTEClassInt *owner;
    int env_id;
};

struct XTEPropertyPtrs
{
    char *name;
    int ptr_count;
    struct XTEPropertyPtr *ptrs;
};


struct XTECmdInt
{
    XTECmdPattern *pattern;
    XTECommandImp imp;
    int param_count;
    char **params;
    int *param_is_delayed;
};

struct XTECmdPrefix
{
    char *prefix_word;
    int command_count;
    struct XTECmdInt *commands;
};


struct XTEVariable
{
    char *name;
    XTEVariant *value;
    int is_global; /* makes it easier for the debugger to find out what's going on */
};


struct XTEFunctionInt
{
    char *name;
    int arg_count;
    XTEFunctionImp imp;
};



struct XTEHandlerFrame
{
    XTEAST *handler;
    
    int param_count;
    XTEVariant **params;
    
    /* **TODO** going to want a more efficient mechanism here than lists;
     probably a hash map or a sorted list with a binary search
     ** LOW PRIORITY ** */
    
    int local_count;
    struct XTEVariable *locals;
    
    int imported_global_count;
    struct XTEVariable **imported_globals;
    
    int nested_loops;
    int loop_abort_code;
    
    XTEVariant *return_value;
    
    int saved_run_error_line;
};



void _xte_out_of_memory(XTE *in_engine);

#define OUT_OF_MEMORY _xte_out_of_memory(in_engine); 
#define OUT_OF_MEMORY_RETURN_VOID {_xte_out_of_memory(in_engine); return;}
#define OUT_OF_MEMORY_RETURN_NULL {_xte_out_of_memory(in_engine); return void;}
#define OUT_OF_MEMORY_RETURN_ZERO {_xte_out_of_memory(in_engine); return 0;}


#define XTE_ERROR_NO_HANDLER 1
#define XTE_ERROR_NO_OBJECT  2


struct XTEErrorDetail
{
    int posted;
    int has_error;
    int is_runtime;
    XTEVariant *source_object;
    long source_line;
    char  *msg_template;
    char  *param1;
    char  *param2;
    char  *param3;
};


/* see .error_detail below */
#define TWO_ERRORS 2

#define ERROR_1 0
#define ERROR_2 1


#define INVALID_LINE_NUMBER 0


struct XTE
{
    char struct_id[sizeof(_XTE_STRUCT_ID)+1];
    
    /*********
     Dictionary
     used by the parser to make sense of the input script;
     contains representations of most language constructs with the notable exception of operators;
     populated with both engine built-ins & primitives, and with environment terminology.
     */

    /* built-in functions */
    int func_count;
    struct XTEFunctionInt **funcs;
    
    /* synonyms for expansion during parsing */
    int syns_count;
    struct XTESynonymInt *syns;
    
    /* constants */
    int cons_count;
    struct XTEConstantInt *cons;
    
    /* references to elements (collections of objects) */
    int ref_count;
    struct XTERefInt *refs;
    
    /* class and global properties */
    int class_prop_count;
    struct XTEPropertyInt **class_props;
    int global_prop_count;
    struct XTEPropertyInt **global_props;
    
    /* map of property name + object class to read/write function handlers */
    int prop_ptr_count;
    struct XTEPropertyPtrs *prop_ptr_table;
    
    /* commands */
    int cmd_prefix_count;
    struct XTECmdPrefix *cmd_prefix_table;
    
    /* classes */
    int class_count;
    struct XTEClassInt **classes;
    
    
    /*********
     Operations
     */
    
#if XTALK_TESTS
    /* allocated bytes by this unit immediately after creation of this structure */
    long init_alloc_bytes;
#endif
    
    /* engine status and operational callbacks */
    struct XTECallbacks callback;
    
    /* error state */
    XTEError error_fatal;
    XTEError error_operational;
    char *error_message;
    
    /* new error state for runtime and syntax errors;
     we have two, so that we can present the one with
     the most relevant information when processing
     a message via the message protocol. */
    struct XTEErrorDetail error_detail[TWO_ERRORS];
    int which_error;
    
    /* the result of the last expression evaluation, command or function execution */
    XTEVariant *the_result;
    
    /* the original target of the system event message currently being handled;
     or message protocol input */
    XTEVariant *the_target;
    
    /* the owner of the currently executing handler */
    XTEVariant *me;
    
    /* passing the message rather than consuming it */
    int exited_passing;
    
    /* exit a handler */
    int exit_handler;
    
    /* run state of the script, ie. running, paused, etc. */
    int run_state;
    
    /* debug handler level at time of runstate change */
    int debug_handler_ref_point;
    long debug_ref_line;
    
    /* invoked via message protocol */
    int invoked_via_message;
    
    /* source line for runtime errors */
    long run_error_line;
    
    /* global variables */
    int global_count;
    struct XTEVariable **globals;
    
    /* debug temporary output of variable */
    char *temp_debug_var_value;
    char **temp_debug_handlers;
    int temp_debug_handler_count;
    
    /* the 'itemDelimiter' */
    char *item_delimiter;
    
    /* the 'numberFormat' */
    char *number_format;
    
    /* context for the owning document */
    void *context;
    
    /* handler frames;
     for debugging and local variables */
    struct XTEHandlerFrame handler_stack[XTALK_LIMIT_NESTED_HANDLERS];
    int handler_stack_ptr;
    
    /* date/time */
    void *os_datetime_context;
    
    /* temporary function results */
    char *f_result_cstr;
};


#define XTE_RUNSTATE_RUN 0
#define XTE_RUNSTATE_PAUSE 1
#define XTE_RUNSTATE_ABORT -1

#define XTE_RUNSTATE_DONE -2

/* opportunity to inspect current state of interpreter in debug interface;
 only legal action left is to abort */
#define XTE_RUNSTATE_DEBUG_ERROR 5 

#define XTE_RUNSTATE_DEBUG_OVER 2
#define XTE_RUNSTATE_DEBUG_INTO 3
#define XTE_RUNSTATE_DEBUG_OUT 4


XTEVariant* xte_variant_copy(XTE *in_engine, XTEVariant *in_variant);




/******************
 Memory Management
 */

#define _XTE_MALLOC_ZONE_GENERAL 0  /* general memory; no specific requirements */
#define _XTE_MALLOC_ZONE_STANDARD -1 /* use standard library directly, without debugging capabilities */

void* _xte_malloc(long in_bytes, int in_zone, char const *in_file, long in_line);
void* _xte_calloc(long in_number, long in_bytes, int in_zone, char const *in_file, long in_line);
void* _xte_realloc(void *in_memory, long in_bytes, int in_zone, char const *in_file, long in_line);
void _xte_free(void *in_memory);

#ifndef _XTE_ALLOCATOR_INT
#define __func__ __FUNCTION__
#define malloc(bytes) _xte_malloc(bytes, _XTE_MALLOC_ZONE_GENERAL, __func__, __LINE__)
#define calloc(num, bytes) _xte_calloc(num, bytes, _XTE_MALLOC_ZONE_GENERAL, __func__, __LINE__)
#define realloc(mem, bytes) _xte_realloc(mem, bytes, _XTE_MALLOC_ZONE_GENERAL, __func__, __LINE__)
#define free(mem) _xte_free(mem)
#endif

#if DEBUG
long _xte_mem_allocated(void);
void _xte_mem_print(void);
void _xte_mem_baseline(void);
#endif




void _xte_cstrs_free(char *in_strings[], int const in_count);





void* _xte_panic_null(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...);
void _xte_panic_void(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...);
int _xte_panic_int(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...);

char const* _xte_first_word(XTE *in_engine, char const *in_string);
char* _xte_cstr_format_fill(const char *in_format, va_list in_list);
char* _xte_clone_cstr(XTE *in_engine, const char *in_string);
void _xte_itemize_cstr(XTE *in_engine, const char *in_string, const char *in_delimiter, char **out_items[], int *out_count);
int _xte_compare_cstr(const char *in_string_1, const char *in_string_2);
void _xte_cstrs__xte_free(char *in_strings[], int in_count);

int xte_variant_is_variable(XTEVariant *in_variant);

XTETextRange _xte_text_make_range(long in_offset, long in_length);

void _xte_set_result(XTE *in_engine, XTEVariant *in_value);

int _xte_node_is_the(XTEAST *in_node);
int _xte_node_is_identifier(XTEAST *in_node);



int _xte_random(int in_upper_limit);

struct XTEClassInt* _xte_class_for_name(XTE *in_engine, const char *in_name);


char const* _xte_format_number_r(XTE *in_engine, double in_real, char const *in_format_string);


void _xte_build_constant_table(XTE *in_engine);
void _xte_build_refs_table(XTE *in_engine);
void _xte_build_property_tables(XTE *in_engine);
void _xte_build_command_table(XTE *in_engine);


#define XTE_AST_FLAG_NOCOPY 0x1


XTEAST* _xte_ast_create(XTE *in_engine, XTEASTType in_type);
XTEAST* _xte_ast_create_word(XTE *in_engine, const char *in_word);
XTEAST* _xte_ast_create_literal_integer(XTE *in_engine, int in_integer);
XTEAST* _xte_ast_create_literal_real(XTE *in_engine, double in_real);
XTEAST* _xte_ast_create_pointer(XTE *in_engine, void *in_ptr, char *in_note);
XTEAST* _xte_ast_create_literal_string(XTE *in_engine, const char *in_string, int in_flags);
XTEAST* _xte_ast_create_operator(XTE *in_engine, XTEASTOperator in_operator);
XTEAST* _xte_ast_create_keyword(XTE *in_engine, XTEASTKeyword in_keyword);
XTEAST* _xte_ast_create_comment(XTE *in_engine, const char *in_string, int in_flags); // ADDED 2013-08-02
void _xte_ast_list_set_last_offset(XTEAST *in_list, long in_source_offset); // ADDED 2013-08-02
void _xte_ast_list_set_line_offsets(XTEAST *in_list, long in_source_offset, int in_source_line, int in_logical_line);
XTEVariant* _xte_ast_wrap(XTE *in_engine, XTEAST *in_ast);

void _xte_ast_destroy(XTEAST *in_tree);

void _xte_ast_swap_ptrs(XTEAST *in_ast1, XTEAST *in_ast2);

void _xte_ast_list_append(XTEAST *in_list, XTEAST *in_tree);
void _xte_ast_list_insert(XTEAST *in_list, int in_offset, XTEAST *in_tree);
XTEAST* _xte_ast_list_remove(XTEAST *in_list, long in_index);
XTEAST* _xte_ast_list_child(XTEAST *in_list, int in_index);

XTEAST* _xte_ast_list_last(XTEAST *in_list);

#ifdef XTALK_TESTS
#define XTE_AST_DEBUG_QUOTED_OUTPUT 0x1 /* quotes output lines for pasting into a unit test constant */
#define XTE_AST_DEBUG_NO_POINTERS 0x2 /* function pointers and related time-dependant figures are not included, for unit tests */
#define XTE_AST_DEBUG_NODE_ADDRESSES 0x4 /* output includes the address of each node, for memory debugging purposes */
void _xte_ast_debug(XTEAST *in_tree);
const char* _xte_ast_debug_text(XTEAST *in_tree, int in_flags);
#endif



XTEAST* _xte_lex(XTE *in_engine, const char *in_source);


int _xte_parse_command(XTE *in_engine, XTEAST *in_cmd, long in_source_line);
int _xte_parse_expression(XTE *in_engine, XTEAST *in_stream, long in_source_line);


XTEVariant* xte_property_ref(XTE *in_engine, struct XTEPropertyPtr *in_ptrs, XTEVariant *in_owner, XTEPropRep in_rep);

void _xte_interpret(XTE *in_engine, XTEAST *in_ast);


int _xte_make_variants_comparable(XTE *in_engine, XTEVariant *in_variant1, XTEVariant *in_variant2);
int _xte_compare_variants(XTE *in_engine, XTEVariant *in_variant1, XTEVariant *in_variant2);

int _xte_operator_operand_count(XTEASTOperator in_op);

void _xte_variable_write(XTE *in_engine, char const *in_var_name, XTEVariant *in_value, XTETextRange in_range, XTEPutMode in_mode);


XTEPropertyGetter _xte_property_getter(XTE *in_engine, int in_pmap_entry, struct XTEClassInt *in_class);
struct XTEPropertyPtr* _xte_property_ptrs(XTE *in_engine, int in_pmap_entry, struct XTEClassInt *in_class);




int _xte_utf8_compare(const char *in_string1, const char *in_string2);
int _xte_utf8_contains(const char *in_string1, const char *in_string2);


int _xte_global_exists(XTE *in_engine, const char *in_var_name);

int _xte_node_is_the(XTEAST *in_node);

XTEVariant* _xte_resolve_value(XTE *in_engine, XTEVariant *in_variant);


int _xte_send_message(XTE *in_engine, XTEVariant *in_target, XTEVariant *in_responder, int is_func, char const *in_message,
                      XTEVariant *in_params[], int in_param_count, void *in_builtin_hint, int *io_handled);


void _xte_error_syntax(XTE *in_engine, long in_source_line, char const *in_template,
                       char const *in_param1, char const *in_param2, char const *in_param3);
void _xte_error_runtime(XTE *in_engine, long in_source_line, char const *in_template,
                        char const *in_param1, char const *in_param2, char const *in_param3);
void _xte_reset_errors(XTE *in_engine);
int _xte_has_postable_error(XTE *in_engine);
int _xte_has_error(XTE *in_engine);
void _xte_error_post(XTE *in_engine);

#define ERROR_RUNTIME(in_source_line, in_template, in_param1, in_param2, in_param3) (_xte_error_runtime(in_engine, in_source_line, \
in_template, in_param1, in_param2, in_param3))
#define ERROR_SYNTAX(in_source_line, in_template, in_param1, in_param2, in_param3) (_xte_error_runtime(in_engine, in_source_line, \
in_template, in_param1, in_param2, in_param3))

void _xte_interpret_handler(XTE *in_engine, XTEAST *in_handler, XTEVariant *in_params[], int in_param_count);

char const* _lexer_term_desc(XTEAST *in_node);



/*
 *  Operator Implementation Prototypes
 *  ---------------------------------------------------------------------------------------------
 *  These are implemented in _string.c, math.c, _cmplogic.c.
 */
XTEVariant* _xte_op_string_concat(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_string_concat_sp(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_string_is_in(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_string_is_not_in(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);

XTEVariant* _xte_op_math_negate(XTE *in_engine, XTEVariant *in_right);
XTEVariant* _xte_op_math_exponent(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_math_multiply(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_math_divide_fp(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_math_divide_int(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_math_modulus(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_math_add(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_math_subtract(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);

XTEVariant* _xte_op_logic_not(XTE *in_engine, XTEVariant *in_right);
XTEVariant* _xte_op_logic_and(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_logic_or(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);

XTEVariant* _xte_op_comp_eq(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_comp_neq(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_comp_less(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_comp_less_eq(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_comp_more(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);
XTEVariant* _xte_op_comp_more_eq(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right);



void _xte_global_import(XTE *in_engine, char const *in_var_name);
struct XTEVariable* _xte_define_global(XTE *in_engine, char const *in_var_name);


struct XTEClassInt* _xte_variant_class(XTE *in_engine, XTEVariant *in_variant);


/*********
 PLATFORM SPECIFIC GLUE
 */

/*********
 Date/Time
 */

#define XTE_DATE_SHORT 0
#define XTE_DATE_ABBREVIATED 1
#define XTE_DATE_LONG 2

#define XTE_TIME_SHORT 3
#define XTE_TIME_LONG 4

#define XTE_MONTH_SHORT 5
#define XTE_MONTH_LONG 6
#define XTE_WEEKDAY_SHORT 7
#define XTE_WEEKDAY_LONG 8

#define XTE_FORMAT_TIMESTAMP -1
#define XTE_FORMAT_DATEITEMS -2


void* _xte_os_date_init(void);
void _xte_os_date_deinit(void *in_context);

void _xte_os_current_datetime(void *in_context);

double _xte_os_timestamp(void *in_context);
void _xte_os_dateitems(void *in_context, int *out_year, int *out_month, int *out_dayOfMonth,
                       int *out_hour24, int *out_minute, int *out_second, int *out_dayOfWeek);
char const* _xte_os_date_string(void *in_context, int in_format);

void _xte_os_parse_date(void *in_context, char const *in_date);
void _xte_os_conv_dateitems(void *in_context, int in_year, int in_month, int in_dayOfMonth,
                            int in_hour24, int in_minute, int in_second, int in_dayOfWeek);
void _xte_os_conv_timestamp(void *in_context, double in_timestamp);



/*********
 Misc
 */

void _xte_platform_suspend_thread(int in_microseconds);



void _xte_platform_sys_version(int *out_major, int *out_minor, int *out_bugfix);
const char* _xte_platform_sys(void);

/*
void* _xte_platform_utf8_begin(const char *in_string);
void _xte_platform_utf8_end(void *in_context);

int _xte_platform_utf8_length(void);
const char* _xte_platform_utf8_substring(int in_offset, int in_length);

int _xte_platform_utf8_word_count(void);
const char* _xte_platform_utf8_subwords(int in_offset, int in_length);

int _xte_platform_utf8_line_count(void);
const char* _xte_platform_utf8_sublines(int in_offset, int in_length);

void _xte_platform_utf8_set_itemdelimiter(const char *in_delim);
int _xte_platform_utf8_item_count(void);
const char* _xte_platform_utf8_subitems(int in_offset, int in_length);


*/



#endif
