/*
 
 xTalk Engine (XTE), Public API
 xtalk_engine.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Public API for the xTalk engine; implementation of an xTalk dialect similar to HyperTalk,
 environment features, functions and commands are all configurable at runtime.
 
 *************************************************************************************************
 Out-of-Memory Protocol
 -------------------------------------------------------------------------------------------------
 Some portable devices have limited memory.  It is also conceivable that workstation systems might
 be used with very large scripts.
 
 In the event the XTE runs out of memory (ie. calls to _xte_malloc() fail):
 1.  the current task will abort
 2.  the engine error properties will reflect the out of memory situation
 3.  a fatal error callback will be invoked (as was configured at engine initalization)
 4.  no further actions on the engine will have any effect
 
 XTE cannot currently be restored from an out-of-memory situation and the engine must be disposed.
 
 The current call chain should not be given the opportunity to continue, ie. the out of memory
 callback should not return and the application should be terminated for safety.
 
 Various units expect this behaviour and the callback runner is programmed to abort() the task
 if the callback doesn't handle this appropriately.
 
 
 *************************************************************************************************
 Environment Error Protocols
 -------------------------------------------------------------------------------------------------
 In case of an error in an environment callback function invoked by the engine, you should let the
 engine know using a standard error handling function:
 
 
 
 *************************************************************************************************
 The Message Protocol
 -------------------------------------------------------------------------------------------------
 
 xte_message()
 message_result callback
 
 
 *************************************************************************************************
 Line Numbering
 -------------------------------------------------------------------------------------------------
 
 Unless otherwise specified, line numbers begin at 1.  Line indicies begin at 0.
 
 There are two types of line number:
 -  source / actual
 -  logical
 
 Source line numbers are the actual line numbers, as delimited by the platform-specific line break
 character.
 
 Logical lines are what the parser/interpreter will read.  If a line is 'split' using the 
 line-continuation character (¬) that split will not be visible to the parser/interpreter, 
 resulting in differences between logical and actual source lines.
 
 
 *************************************************************************************************
 Line Breaks
 -------------------------------------------------------------------------------------------------
 Classic MacOS used a two-byte line break of 0x0D0A.  We will expect line reaks in scripts to
 be delimited by a single byte, 0x0D or 0x0A for simplicity.  The UI should ensure scripts are
 made available using one of these supported line break formats.
 
 NOTE:  The lexer currently supports all three formats.  However some other units, such as the
 message heirarchy do not.  Given that the two-byte format shouldn't be found on modern systems,
 the added complexity of supporting it is considered a waste of time.
 
 */

#ifndef XTALK_ENGINE_H
#define XTALK_ENGINE_H

#include "xtalk_test.h"

#include "xtalk_limits.h"


/* boolean constants: */
#define XTE_TRUE 1
#define XTE_FALSE 0





typedef enum 
{
    XTE_TYPE_NULL,
    XTE_TYPE_BOOLEAN,
    XTE_TYPE_INTEGER,
    XTE_TYPE_REAL,
    XTE_TYPE_STRING,
    XTE_TYPE_GLOBAL,
    XTE_TYPE_LOCAL,
    XTE_TYPE_NUMBER,
    XTE_TYPE_VALUE,
    XTE_TYPE_OBJECT,
    XTE_TYPE_PROP,
    XTE_TYPE_LIST,
    XTE_TYPE_AST, /* unresolved AST; the result of a successful parse, 
                   but for delayed evaluation, see _cmds.c,
                   specifically delayed parameter evaluation */
    
} XTEVariantType;


/* variant data type, used for all scripting language values within the engine */
typedef struct XTEVariant XTEVariant;


/* xTalk engine instance */
typedef struct XTE XTE;





typedef enum XTEError
{
    /* no error */
    XTE_ERROR_NONE = 0,
    
    /* serious internal errors, often unrecoverable */
    XTE_ERROR_MEMORY,
    XTE_ERROR_INTERNAL,
    XTE_ERROR_UNIMPLEMENTED,
    XTE_ERROR_MISUSE,
    
   
    
} XTEError;


typedef enum
{
    XTE_PROPREP_NORMAL = 0,
    XTE_PROPREP_SHORT,
    XTE_PROPREP_ABBREVIATED,
    XTE_PROPREP_LONG
    
} XTEPropRep;



typedef struct
{
    long offset;
    long length;
    
} XTETextRange;


/* to be used with conversion functions, see xtalk_geo.c */
#define XTE_GEO_CONV_BUFF_SIZE 256

typedef struct
{
    long x;
    long y;
    
} XTEPoint;


typedef struct
{
    long width;
    long height;
    
} XTESize;


typedef struct
{
    XTEPoint origin;
    XTESize size;
    
} XTERect;



typedef enum
{
    XTE_PUT_INTO,
    XTE_PUT_AFTER,
    XTE_PUT_BEFORE
    
} XTEPutMode;


extern XTETextRange const XTE_TEXT_RANGE_ALL;




/*
 *  Environment Callback Types
 *  ---------------------------------------------------------------------------------------------
 *  These callback types are used by the engine to query, mutate and interact with the 
 *  environment, and to provide custom command and function implementations.
 */

/*
 *  XTEObjRefDeallocator
 *  ---------------------------------------------------------------------------------------------
 *  The environment should _xte_free() the memory associated with the specified object reference
 *  (if any.)
 *
 *  Invoked automatically immediately prior to the destruction of an object reference originally
 *  created by the environment.
 *
 *  Specified in arguments to xte_object_ref().
 */
typedef void (*XTEObjRefDeallocator) (XTEVariant *in_variant, const char *in_type, void *in_ident);
typedef void (*XTEObjectDeallocator) (XTEVariant *in_variant, const char *in_type, void *in_ident);

/* access property and constant values;
 MUST NOT HAVE SIDE-EFFECTS!
 we use this for both properties and constants, because sometimes they point to the same thing,
 and it's handy to have them use a single function implementation rather than 2! 
 
 should return NULL if there's an error, raises a default runtime error message,
 probably: Can't understand arguments of "<property>".
 
 */
typedef XTEVariant* (*XTEPropertyGetter) (XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation);

/* set a property */
typedef void (*XTEPropertySetter) (XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEVariant *in_new_value);


/* build an object reference and return it;
 ! IMPORTANT: you must not evaluate the object reference in any way - just package the details
 
 **TODO** it might be better if we build the object references internally within the interpreter,
 then actually use this callback for it's intended purpose, ie. to resolve an object reference given certain params */

typedef XTEVariant* (*XTEElementGetter) (XTE *in_engine, void *in_context, XTEVariant *in_owner, XTEVariant *in_params[], int in_param_count);





/*
 *  XTEContainerWriter
 *  ---------------------------------------------------------------------------------------------
 *  The environment should write the specified <in_new_value> to the container <in_owner>.
 *
 *  <in_owner>          Object reference    ** NEED TO ENFORCE THIS IN THE INTERPRETER **
 *
 *  <in_text>           String              ** NEED TO ENFORCE THIS IN THE INTERPRETER **
 *
 *  <in_offset>         Character offset where the write should begin
 *
 *  <in_length>         Character length to be replaced with the new value
 *
 *                      If -1, replace the entire content of the container
 *
 

 
 *  If there is a problem writing the value to the container, Environment Error Protocols apply.
 */
typedef void (*XTEContainerWriter) (XTE *in_engine, void *in_context, XTEVariant *in_owner, XTEVariant *in_value, XTETextRange in_text_range, XTEPutMode in_mode);

/*
 *  XTEFunctionImp
 *  ---------------------------------------------------------------------------------------------
 *  Environment provided xTalk function() implementation.
 *
 *  The <in_param_count> is only guaranteed if you have defined the function with a fixed number
 *  of arguments.
 *
 *  If the function is successful, you should return a non-NULL variant reference.
 *
 *  Returning NULL is the same as requesting a "Can't understand arguments" runtime error.
 *
 *  Custom functions are defined using _xte_function_add() and _xte_functions_add().
 */
typedef XTEVariant* (*XTEFunctionImp) (XTE *in_engine, XTEVariant *in_params[], int in_param_count);

/*
 *  XTECommandImp
 *  ---------------------------------------------------------------------------------------------
 *  Environment provided xTalk command syntax implementation.
 *
 *  <in_param_count> is guaranteed to be the number of parameters you defined in the command
 *  syntax definition.  Parameters are also provided in the order specified by your definition.
 *
 *  Custom commands are defined using _xte_command_add() and _xte_commands_add().
 */
typedef void (*XTECommandImp) (XTE *in_engine, void *in_context, XTEVariant *in_params[], int in_param_count);


/*
 *  XTEScriptRetriever
 *  ---------------------------------------------------------------------------------------------
 *  Environment provided xTalk script retrieval mechanism.
 *
 *  <in_object> will be a variant reference type as was previously constructed by the environment
 *  using the xte_object_ref() function.
 *
 *  The environment should return either XTE_NO_ERROR or XTE_ERROR_NO_OBJECT for success or 
 *  failure of the retrieval.  On success, <out_script> should be set to point to the string
 *  representation of the script, and <out_checkpoints> & <out_checkpoint_count> an array of int
 *  line numbers for any checkpoints within the script.
 */
typedef int (*XTEScriptRetriever) (XTE *in_engine, void *in_context, XTEVariant *in_object,
char const **out_script, int const *out_checkpoints[], int *out_checkpoint_count);
/* no wait functionality ^ */


/*
 *  XTENextResponder
 *  ---------------------------------------------------------------------------------------------
 *  Environment provided xTalk next responder mechanism; obtains the next message responder in
 *  the message heirarchy.
 *
 *  <in_current_responder> will be a variant reference type as was previously constructed by the 
 *  environment using the xte_object_ref() function.
 *
 *  The environment should return either XTE_NO_ERROR or XTE_ERROR_NO_OBJECT for success or
 *  failure of the retrieval.  On success, <out_next_responder> should be set to the next responder.
 */
typedef int (*XTENextResponder) (XTE *in_engine, void *in_context, XTEVariant *in_current_responder, XTEVariant **out_next_responder);

/* no wait functionality ^ */


/*
 *  Environment Terminology Definition Structures
 *  ---------------------------------------------------------------------------------------------
 *  These types are used by the environment to define terminology for which it provides the
 *  implementation, including custom function and command syntaxes.
 */

struct XTEElementDef
{
    char *name; /* descriptive name used in code, for all elements */
    char *singular; /* single instance name, used referring to one or more instances */
    char *type; /* class name, not used in code ( required to resolve UIDs & for error message generation );
                 all objects within this element belong to this class */
    int flags;
    
    /* no params -> entire collection, 1 param -> single field, 2 params -> numbered range of fields;
     if we return a collection of multiple elements, use a new variant collection type; obviously
     this type will not be able to be coaxed to string for display - so can only be used within expressions */
    
    XTEElementGetter get_range;
    
    
    XTEElementGetter get_named; /* these may eventually be moved into a substructure, including allowable owner types */
    XTEPropertyGetter get_count; /* return the number of items in the subset of items provided,
                                  where the subset is obtained by way of get_range with no parameters;
                                  thus interpreter counting is two callbacks:
                                  the count is assumed to be a property of whatever class is normally obtained
                                  by calling get_range! (which is currently left to the implementor to define....)  **** REVIEW LATER */
};


#define XTE_PROPERTY_UID 0x4 // seems to be getting use in _prop.c


struct XTEPropertyDef
{
    char *name;
    int env_id;
    char *type;
    int flags;
    XTEPropertyGetter func_get;
    XTEElementGetter func_element_by_prop; /* get an element with supplied property value;
                                            only applicable when property is part of a class (not global) */
    
    XTEPropertySetter func_set;
};

struct XTEConstantDef
{
    char *name;
    XTEPropertyGetter func_get;
};

struct XTEFunctionDef
{
    char *name;
    int arg_count;
    XTEFunctionImp ptr;
};

struct XTEClassDef
{
    char *name;
    int flags;
    int property_count;
    struct XTEPropertyDef *properties;
    /*int element_count;
    struct XTEElementDef *elements;*/
    XTEPropertyGetter container_read;
    XTEContainerWriter container_write;
    XTEScriptRetriever script_retr;
    XTENextResponder next_responder;
};

struct XTECommandDef
{
    char *syntax; /* BNF-type xtalk syntax and command name */
    char *params; /* named, comma-delimited list of parameters in the appropriate sequence;
                   parameter evaluation can be delayed by postfixing the name with a <
                   so that the AST is passed to the implementation (as a variant) instead of
                   evaluating the AST as would normally be the case */
    XTECommandImp imp; /* ptr to a C implementation function */
};

/* synonyms will be expanded (temporarily) during certain phases of parsing;
 but if a word turns out not to be part of a valid reference, ie. a variable identifier,
 the expansion will no longer apply,
 suggest that tokens created as a result of synonym expansion cannot be further expanded,
 otherwise we could end up in some weird loops - maybe the expansion should be applied
 to a token? */

/* must be lowercase, single space between words,
 sorted by number of words on left hand side with larger # toward bottom */
struct XTESynonymDef
{
    char *synonym;
    char *name;
};



/*
 *  Engine Callbacks
 *  ---------------------------------------------------------------------------------------------
 *  These callbacks are invoked by the engine to communicate with the environment about errors,
 *  and for other trivial protocols including the Message Protocol.
 */


typedef char const* (*XTELocalizedClassNameCB) (XTE *in_engine, void *in_context, char const *in_class_name);


typedef void (*XTEMessageResultCB) (XTE *in_engine, void *in_context, char const *in_result);



/* these two we don't need wait functionality on them:
 all other callbacks should have new wait functionality so that they can potentially wait for a
 response to be generated and signalled via another thread, ie. the UI thread */

typedef void (*XTEScriptErrorCB) (XTE *in_engine, void *in_context, XTEVariant *in_handler_object, long in_source_line,
char const *in_template, char const *in_arg1, char const *in_arg2, char const *in_arg3, int in_runtime);

typedef void (*XTEScriptProgressCB) (XTE *in_engine, void *in_context, int in_paused, XTEVariant *in_handler_object, long in_source_line);


typedef void (*XTEDebugMessageCB) (XTE *in_engine, void *in_context, char const *in_message, int in_level, int in_handled);

typedef void (*XTEDebugHandlerCB) (XTE *in_engine, void *in_context, char const *in_handlers[], int in_handler_count);
typedef void (*XTEDebugVariableCB) (XTE *in_engine, void *in_context, char const *in_name, int is_global, char const *in_value);


/* paused, not necessarily because of a checkpoint on the specific line */
typedef void (*XTEDebugCheckpointCB) (XTE *in_engine, void *in_context, XTEVariant *in_handler_object, long in_source_line);



struct XTECallbacks
{
    XTEMessageResultCB message_result;
    XTEScriptErrorCB script_error;
    XTEScriptProgressCB script_progress;
    
    XTEDebugMessageCB debug_message;
    XTEDebugVariableCB debug_variable;
    XTEDebugHandlerCB debug_handler;
    
    XTELocalizedClassNameCB localized_class_name;
    
    XTEDebugCheckpointCB debug_checkpoint;
    
    //void (*message_result) (XTE *in_engine, void *in_context, const char *in_result);
    //void (*script_error) (XTE *in_engine, void *in_context, const char *in_message);
    //XTEScriptRetriever script_retriever;
};



/*
 *  Engine Initalization Functions
 *  ---------------------------------------------------------------------------------------------
 *  Create and configure an instance of the xTalk engine for a specific environment.
 */

XTE* xte_create(void *in_context);

void xte_configure_environment(XTE *in_engine, struct XTEClassDef *in_classes, struct XTEConstantDef *in_consts,
                            struct XTEPropertyDef *in_props, struct XTEElementDef *in_elements,
                            struct XTEFunctionDef *in_funcs, struct XTECommandDef *in_commands,
                            struct XTESynonymDef *in_syns);

void xte_configure_callbacks(XTE *in_engine, struct XTECallbacks in_callbacks);



/*
 *  Engine Shutdown
 *  ---------------------------------------------------------------------------------------------
 *  Dispose of the engine when no longer needed.
 */

void xte_dispose(XTE *in_engine);



/*
 *  Engine Script Entry Points
 *  ---------------------------------------------------------------------------------------------
 *  These functions provide various 'entry-points' into the xTalk interpreter.  Invoking any
 *  initiates a script/command/expression execution.
 */


void xte_message(XTE *in_engine, const char *in_message, XTEVariant *in_first_responder);

/* used by sort to evaluate the sort expression as the current card changes, etc. */
XTEVariant* xte_evaluate_delayed_param(XTE *in_engine, XTEVariant *in_param);


void xte_post_system_event(XTE *in_engine, XTEVariant *in_target, char const *in_event, XTEVariant *in_params[], int in_param_count);



/*
 *  Engine Runtime Error Functions
 *  ---------------------------------------------------------------------------------------------
 *  Call any of these to initiate a runtime scripting error.
 */

void _xte_raise_void(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...);
void* _xte_raise_null(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...);
int _xte_raise_int(XTE *in_engine, XTEError in_error, const char *in_msg_format, ...);


int xte_has_error(XTE *in_engine);

/*
 *  Variant Functions
 *  ---------------------------------------------------------------------------------------------
 *  Variant types are used throughout the engine to represent all scriptable data, including
 *  primitive strings, integers, reals, booleans and complex types such as object references
 *  and chunk expressions.
 *
 *  Use these functions to create, inspect and manipulate variant data.
 *
 *  Variants are reference counted.  They initially begin life with a reference count of 1.
 *  Use the _retain() and _release() calls to manipulate the reference count.  When a variant's
 *  reference count reaches 0, the variant is destroyed.
 *
 *  Read the relevant callback or function documentation to find out how to manage variant
 *  memory usage.  In most cases, memory management is semi-automatic and often explicit use of
 *  _retain() and _release() is unnecessary.
 */

XTEVariant* xte_variant_create(XTE *in_engine);
XTEVariant* xte_string_create_with_cstring(XTE *in_engine, const char *in_utf8_string);
XTEVariant* xte_boolean_create(XTE *in_engine, int in_boolean);
XTEVariant* xte_integer_create(XTE *in_engine, int in_integer);
XTEVariant* xte_real_create(XTE *in_engine, double in_real);
XTEVariant* xte_global_ref(XTE *in_engine, char *in_var_name);
XTEVariant* xte_object_ref(XTE *in_engine, char *in_class, void *in_ident, XTEObjRefDeallocator in_dealloc);

XTEVariant* xte_object_create(XTE *in_engine, char const *in_class_name, void *in_data_ptr, XTEObjectDeallocator in_dealloc);


void xte_variant_release(XTEVariant *in_variant);
XTEVariant* xte_variant_retain(XTEVariant *in_variant);

void* xte_variant_ref_ident(XTEVariant *in_variant);
const char* xte_variant_ref_type(XTEVariant *in_variant);

void* xte_variant_object_data_ptr(XTEVariant *in_variant);
int xte_variant_is_class(XTEVariant *in_variant, char const *in_class_name);


int xte_variant_is_container(XTEVariant *in_variant);

XTEVariant* xte_variant_value(XTE *in_engine, XTEVariant *in_variant);


/* NOTE for future addns, if we have containers that support other types of data, we can introduce additional functions
 for non-string data.  this is a string-protocol.*/
int xte_container_write(XTE *in_engine, XTEVariant *in_container, XTEVariant *in_value, XTETextRange in_range, XTEPutMode in_mode);


int xte_property_write(XTE *in_engine, XTEVariant *in_property, XTEVariant *in_value);

XTEVariantType xte_variant_type(XTEVariant *in_variant);
int xte_variant_convert(XTE *in_engine, XTEVariant *in_variant, XTEVariantType in_new_type);

const char* xte_variant_as_cstring(XTEVariant *in_variant);
double xte_variant_as_double(XTEVariant *in_variant);
int xte_variant_as_int(XTEVariant *in_variant);

//void xte_variant__xte_free(XTEVariant *in_variant);


int xte_compare_variants(XTE *in_engine, XTEVariant *in_variant1, XTEVariant *in_variant2);




long _xte_utf8_strlen(char const *in_string);


/* public UTF-8 string utilities */

char const* xte_cstring_index_word(char const *in_string, long in_word_index);
char const* xte_cstring_index_word_end(char const *in_string);

int xte_cstrings_equal(char const *in_string1, char const *in_string2);
int xte_cstrings_szd_equal(char const *in_string1, long in_bytes1, char const *in_string2, long in_bytes2);

long xte_cstring_chars_between(char const *in_begin, char const *const in_end);


long xte_cstring_length(char const *in_string);



/*
 *  Source Formatting Utilities
 *  ---------------------------------------------------------------------------------------------
 *  Supports syntax colouring/styling and automatic indentation.
 */

/*
 *  xte_source_indented
 *  ---------------------------------------------------------------------------------------------
 *  Indent
 */

typedef void (*XTEIndentingHandler)(XTE *in_engine, void *in_context, long in_char_begin, long in_char_length,
char const *in_spaces, int in_space_count);

void xte_source_indent(XTE *in_engine, char const *in_source, long *io_selection_start, long *io_selection_length,
                       XTEIndentingHandler in_indenting_handler, void *in_context);

typedef void (*XTEColourisationHandler)(XTE *in_engine, void *in_context, long in_char_begin, long in_char_length, int in_type);

void xte_source_colourise(XTE *in_engine, char const *in_source, XTEColourisationHandler in_colorisation_handler, void *in_context);




/* note we include this here, on the assumption that it will be invoked in a callback for some reason or other
 and not from a thread - xte is not thread safe - locking and mutexes must be used by the caller;
 
 stack overflow (http://stackoverflow.com/questions/7223164/is-mutex-needed-to-synchronize-a-simple-flag-between-pthreads)
 suggests that because of the complexity of caches between CPU cores, etc. we need to use mutexes, even
 for a simple flag.  even volatile keyword on var declaration isn't likely to be sufficient.
 */

/* signals the running engine should abort;
 has no callback effects and doesn't necessarily take place immediately,
 only recommends that it occur */
void xte_abort(XTE *in_engine);





void xte_continue(XTE *in_engine);


#define XTE_OK 0
#define XTE_ABORT 1
#define XTE_PANIC 2

/* enables a callback function that is taking a long time to 
 give the engine an opportunity to report progress, etc. and interact
 with debugging interfaces, or stop the long running callback.
 
 can return XTE_OK, XTE_ABORT, or XTE_PANIC
 if it returns XTE_ABORT, the callback should stop at the next possible opportunity,
 and finish in a consistent state.
 if it returns XTE_PANIC, the callback should terminate immediately
 without worring about cleaning up
 
 */
int xte_callback_still_busy(XTE *in_engine);



void xte_debug_step_over(XTE *in_engine);
void xte_debug_step_into(XTE *in_engine);
void xte_debug_step_out(XTE *in_engine);


void xte_debug_mutate_variable(XTE *in_engine, char const *in_name, int is_global, char *in_value);
void xte_debug_enumerate_handlers(XTE *in_engine);

#define XTE_CONTEXT_CURRENT -2

void xte_debug_enumerate_variables(XTE *in_engine, int in_context);


/* standard mechanism other than returning NULL for a callback handler in the environment
 to produce an ordinary runtime/syntax error */
void xte_callback_error(XTE *in_engine, char const *in_template, char const *in_param1, char const *in_param2, char const *in_param3);
void* xte_callback_error_null(XTE *in_engine, char const *in_template, char const *in_param1, char const *in_param2, char const *in_param3);
void xte_callback_failed(XTE *in_engine);
void* xte_callback_failed_null(XTE *in_engine);


void xte_set_global(XTE *in_engine, char const *in_name, XTEVariant *in_value);
void xte_set_result(XTE *in_engine, XTEVariant *in_result);




XTERect xte_string_to_rect(char const *in_string);
char const* xte_rect_to_string(XTERect in_rect, char *in_buffer, long in_buffer_size);


#endif
