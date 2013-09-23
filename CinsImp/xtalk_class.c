/*
 
 xTalk Engine Class Unit
 xtalk_class.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Responsible for definition and lookup of class types
 
 */

#include "xtalk_internal.h"


/*********
 Dictionary
 */

/* prototypes for functions in related modules used by this one: */
struct XTEPropertyInt* _xte_property_add(XTE *in_engine, struct XTEPropertyDef *in_def, struct XTEClassInt *in_class);


/* adds a single class to the internal class catalogue */
static struct XTEClassInt* _xte_class_add(XTE *in_engine, struct XTEClassDef *in_def)
{
    if (!in_def->name) return NULL;
    
    /* check a class with the specified name doesn't already exist */
    for (int i = 0; i < in_engine->class_count; i++)
    {
        if (_xte_compare_cstr(in_engine->classes[i]->name, in_def->name) == 0)
            return _xte_panic_null(in_engine, XTE_ERROR_MISUSE, "Can't add class \"%s\"; class name already defined", in_def->name);
    }
    
    /* add a class to the class table */
    struct XTEClassInt **new_table = realloc(in_engine->classes, sizeof(struct XTEClassInt*) * (in_engine->class_count + 1));
    if (!new_table) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    in_engine->classes = new_table;
    struct XTEClassInt *the_class = new_table[in_engine->class_count++] = calloc(sizeof(struct XTEClassInt), 1);
    if (!the_class) return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    
    /* define the class */
    the_class->name = _xte_clone_cstr(in_engine, in_def->name);
    the_class->container_read = in_def->container_read;
    the_class->container_write = in_def->container_write;
    the_class->script_retr = in_def->script_retr;
    the_class->next_responder = in_def->next_responder;
    
    /* add the class' properties */
    the_class->properties = calloc(in_def->property_count, sizeof(struct XTEPropertyInt*));
    if (!the_class->properties)
        return _xte_panic_null(in_engine, XTE_ERROR_MEMORY, NULL);
    the_class->property_count = in_def->property_count;
    for (int i = 0; i < in_def->property_count; i++)
        the_class->properties[i] = _xte_property_add(in_engine, &(in_def->properties[i]), the_class);
    
    return the_class;
}


/* returns a pointer to the class definition for a class with the given name;
 or NULL if no such class exists */
struct XTEClassInt* _xte_class_for_name(XTE *in_engine, const char *in_name)
{
    for (int i = 0; i < in_engine->class_count; i++)
    {
        if (_xte_compare_cstr(in_engine->classes[i]->name, in_name) == 0)
            return in_engine->classes[i];
    }
    return NULL;
}


/* add one or more classes with one call */
void _xte_classes_add(XTE *in_engine, struct XTEClassDef *in_defs)
{
    if (!in_defs) return;
    struct XTEClassDef *the_classes = in_defs;
    while (the_classes->name)
    {
        _xte_class_add(in_engine, the_classes);
        the_classes++;
    }
}



/*********
 Built-ins
 */


XTEVariant* _xte_prp_month(XTE *in_engine, void *in_context, int prop, XTEVariant *in_owner, XTEPropRep in_representation);
XTEVariant* _xte_prp_weekday(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation);

static struct XTEPropertyDef _xte_builtin_int_properties[] = {
    {"month", 0, "string", 0, &_xte_prp_month, NULL},
    {"weekday", 0, "string", 0, &_xte_prp_weekday, NULL},
    /* {"weekday", "string", 0, &_xte_prp_weekday, NULL},*/
    NULL,
};

static struct XTEPropertyDef _xte_builtin_real_properties[] = {
    {"month", 0, "string", 0, &_xte_prp_month, NULL},
    {"weekday", 0, "string", 0, &_xte_prp_weekday, NULL},
    /* {"weekday", "string", 0, &_xte_prp_weekday, NULL},*/
    NULL,
};

static struct XTEPropertyDef _xte_builtin_str_properties[] = {
    {"month", 0, "string", 0, &_xte_prp_month, NULL},
    {"weekday", 0, "string", 0, &_xte_prp_weekday, NULL},
    /* {"weekday", "string", 0, &_xte_prp_weekday, NULL},*/
    NULL,
};


/* primitive types: */
static struct XTEClassDef _class_integer = {
    "integer",
    0,
    sizeof(_xte_builtin_int_properties) / sizeof(struct XTEPropertyDef),
    _xte_builtin_int_properties,
    NULL,
};

static struct XTEClassDef _class_real = {
    "real",
    0,
    sizeof(_xte_builtin_real_properties) / sizeof(struct XTEPropertyDef),
    _xte_builtin_real_properties,
    NULL,
};

static struct XTEClassDef _class_boolean = {
    "boolean",
    0,
    0,
    NULL,
    NULL,
};

static struct XTEClassDef _class_string = {
    "string",
    0,
    sizeof(_xte_builtin_str_properties) / sizeof(struct XTEPropertyDef),
    _xte_builtin_str_properties,
    NULL,
};



/* "the System": */

static XTEVariant* _bi_system_version(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    int major,minor,bugfix;
    char buffer[64];
    _xte_platform_sys_version(&major, &minor, &bugfix);
    switch (in_representation)
    {
        case XTE_PROPREP_NORMAL:
        case XTE_PROPREP_SHORT:
            sprintf(buffer, "%d.%02d%02d", major, minor, bugfix);
            return xte_string_create_with_cstring(in_engine, buffer);
            return xte_real_create(in_engine, atof(buffer));
        case XTE_PROPREP_ABBREVIATED:
        {
            char buffer[64];
            sprintf(buffer, "%d.%d", major, minor);
            return xte_string_create_with_cstring(in_engine, buffer);
        }
        case XTE_PROPREP_LONG:
        {
            char buffer[64];
            sprintf(buffer, "%d.%d.%d", major, minor, bugfix);
            return xte_string_create_with_cstring(in_engine, buffer);
        }
    }
}

static XTEVariant* _bi_system_name(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, _xte_platform_sys());
}

static XTEVariant* _bi_system_test(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    return xte_string_create_with_cstring(in_engine, "OK");
}

static struct XTEPropertyDef _system_properties[] = {
    {"version", 0, "real", 0, &_bi_system_version, NULL},
    {"name", 0, "string", 0, &_bi_system_name, NULL},
    {"_test_parser", 0, "string", 0, &_bi_system_test, NULL},
    NULL
};

static struct XTEClassDef _class_system = {
    "system",
    0,
    sizeof(_system_properties) / sizeof(struct XTEPropertyDef),
    _system_properties,
    NULL,
};



extern struct XTEClassDef _xte_builtin_class_chunk;


/* add built-in language classes */
void _xte_classes_add_builtins(XTE *in_engine)
{
    /* add primitive type support */
    _xte_class_add(in_engine, &_class_integer);
    _xte_class_add(in_engine, &_class_real);
    _xte_class_add(in_engine, &_class_boolean);
    _xte_class_add(in_engine, &_class_string);
    
    /* add chunk support */
    _xte_class_add(in_engine, &_xte_builtin_class_chunk);
    
    /* add "the System" object for accessing host system information */
    _xte_class_add(in_engine, &_class_system);
}



