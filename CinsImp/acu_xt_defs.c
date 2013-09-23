//
//  acu_xt_defs.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 27/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "acu_int.h"


int _retr_script(XTE *in_engine, StackMgrStack *in_register_entry, XTEVariant *in_object,
                        char const **out_script, int const *out_checkpoints[], int *out_checkpoint_count);
int _next_responder(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_responder, XTEVariant **out_responder);



XTEVariant* _obj_cd_fld_range(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_cd_fld_named(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_bg_fld_range(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_bg_fld_named(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count);



XTEVariant* _obj_cd_fld_id(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                           XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_bg_fld_id(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                           XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_fld_read(XTE *in_engine, StackMgrStack *in_registry_entry, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation);



XTEVariant* _obj_cd_btn_range(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_cd_btn_named(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_cd_btn_id(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                           XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_bg_btn_range(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_bg_btn_named(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                              XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_bg_btn_id(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                           XTEVariant *in_params[], int in_param_count);


XTEVariant* _obj_card_range(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                            XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_card_named(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                            XTEVariant *in_params[], int in_param_count);
XTEVariant* _obj_card_id(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_owner,
                         XTEVariant *in_params[], int in_param_count);


XTEVariant* _obj_fld_read(XTE *in_engine, StackMgrStack *in_registry_entry, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation);
void _obj_fld_write(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_owner,
                    XTEVariant *in_value, XTETextRange in_text_range, XTEPutMode in_mode);


XTEVariant* _acu_xt_objcount_get_number(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation);


XTEVariant* _acu_xt_const_this_card(XTE *in_engine, StackMgrStack *in_registry_entry, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation);
XTEVariant* _acu_xt_const_this_bkgnd(XTE *in_engine, StackMgrStack *in_registry_entry, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation);
XTEVariant* _acu_xt_const_this_stack(XTE *in_engine, StackMgrStack *in_registry_entry, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation);


XTEVariant* _acu_xt_widg_get_str(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation);
void _acu_xt_widg_set_str(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEVariant *in_new_value);
XTEVariant* _acu_xt_widg_get_bool(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation);
void _acu_xt_widg_set_bool(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEVariant *in_new_value);
XTEVariant* _acu_xt_widg_get_int(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation);
void _acu_xt_widg_set_int(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEVariant *in_new_value);

XTEVariant* _acu_xt_obj_get_id(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation);


XTEVariant* _acu_xt_stak_get_str(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation);
void _acu_xt_stak_set_str(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEVariant *in_new_value);

XTEVariant* _acu_xt_stak_get_bool(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEPropRep in_representation);
void _acu_xt_stak_set_bool(XTE *in_engine, StackMgrStack *in_stack, int in_prop, XTEVariant *in_object, XTEVariant *in_new_value);



struct XTEPropertyDef _acu_xt_bg_fld_props[] = {
    {"name", PROPERTY_NAME, "string", XTE_PROPERTY_UID, (XTEPropertyGetter)&_acu_xt_widg_get_str, (XTEElementGetter)&_obj_bg_fld_named,
        (XTEPropertySetter)_acu_xt_widg_set_str},
    
    {"id", 0, "integer", XTE_PROPERTY_UID, (XTEPropertyGetter)&_acu_xt_obj_get_id, (XTEElementGetter)&_obj_bg_fld_id},
    
    {"text", PROPERTY_CONTENT, "string", 0, (XTEPropertyGetter)&_obj_fld_read, NULL, NULL},
    {"locked", PROPERTY_LOCKED, "boolean", 0, (XTEPropertyGetter)&_acu_xt_widg_get_bool, NULL, (XTEPropertySetter)&_acu_xt_widg_set_bool},
    {"dontSearch", PROPERTY_DONTSEARCH, "boolean", 0, (XTEPropertyGetter)&_acu_xt_widg_get_bool, NULL, (XTEPropertySetter)&_acu_xt_widg_set_bool},
    {"allowStyledText", PROPERTY_RICHTEXT, "boolean", 0, (XTEPropertyGetter)&_acu_xt_widg_get_bool, NULL, (XTEPropertySetter)&_acu_xt_widg_set_bool},
    {"style", PROPERTY_STYLE, "string", 0, (XTEPropertyGetter)&_acu_xt_widg_get_str, NULL, (XTEPropertySetter)&_acu_xt_widg_set_str},
    {"border", PROPERTY_BORDER, "string", 0, (XTEPropertyGetter)&_acu_xt_widg_get_str, NULL, (XTEPropertySetter)&_acu_xt_widg_set_str},
    {"textStyle", PROPERTY_TEXTSTYLE, "string", 0, (XTEPropertyGetter)&_acu_xt_widg_get_str, NULL, (XTEPropertySetter)&_acu_xt_widg_set_str},
    {"textFont", PROPERTY_TEXTFONT, "string", 0, (XTEPropertyGetter)&_acu_xt_widg_get_str, NULL, (XTEPropertySetter)&_acu_xt_widg_set_str},
    {"textSize", PROPERTY_TEXTSIZE, "integer", 0, (XTEPropertyGetter)&_acu_xt_widg_get_int, NULL, (XTEPropertySetter)&_acu_xt_widg_set_int},
    
    {"shared", PROPERTY_SHARED, "boolean", 0, (XTEPropertyGetter)&_acu_xt_widg_get_bool, NULL, (XTEPropertySetter)&_acu_xt_widg_set_bool},
    
    NULL,
};

struct XTEPropertyDef _acu_xt_cd_fld_props[] = {
    {"name", PROPERTY_NAME, "string", XTE_PROPERTY_UID, (XTEPropertyGetter)&_acu_xt_widg_get_str, (XTEElementGetter)&_obj_cd_fld_named,
        (XTEPropertySetter)_acu_xt_widg_set_str},
    
    {"id", 0, "integer", XTE_PROPERTY_UID, (XTEPropertyGetter)&_acu_xt_obj_get_id, (XTEElementGetter)&_obj_cd_fld_id},
    
    {"text", PROPERTY_CONTENT, "string", 0, (XTEPropertyGetter)&_obj_fld_read, NULL, NULL},
    {"locked", PROPERTY_LOCKED, "boolean", 0, (XTEPropertyGetter)&_acu_xt_widg_get_bool, NULL, (XTEPropertySetter)&_acu_xt_widg_set_bool},
    {"dontSearch", PROPERTY_DONTSEARCH, "boolean", 0, (XTEPropertyGetter)&_acu_xt_widg_get_bool, NULL, (XTEPropertySetter)&_acu_xt_widg_set_bool},
    {"allowStyledText", PROPERTY_RICHTEXT, "boolean", 0, (XTEPropertyGetter)&_acu_xt_widg_get_bool, NULL, (XTEPropertySetter)&_acu_xt_widg_set_bool},
    {"style", PROPERTY_STYLE, "string", 0, (XTEPropertyGetter)&_acu_xt_widg_get_str, NULL, (XTEPropertySetter)&_acu_xt_widg_set_str},
    {"border", PROPERTY_BORDER, "string", 0, (XTEPropertyGetter)&_acu_xt_widg_get_str, NULL, (XTEPropertySetter)&_acu_xt_widg_set_str},
    {"textStyle", PROPERTY_TEXTSTYLE, "string", 0, (XTEPropertyGetter)&_acu_xt_widg_get_str, NULL, (XTEPropertySetter)&_acu_xt_widg_set_str},
    {"textFont", PROPERTY_TEXTFONT, "string", 0, (XTEPropertyGetter)&_acu_xt_widg_get_str, NULL, (XTEPropertySetter)&_acu_xt_widg_set_str},
    {"textSize", PROPERTY_TEXTSIZE, "integer", 0, (XTEPropertyGetter)&_acu_xt_widg_get_int, NULL, (XTEPropertySetter)&_acu_xt_widg_set_int},
    
    NULL,
};

struct XTEPropertyDef _acu_xt_bg_btn_props[] = {
    {"name", PROPERTY_NAME, "string", XTE_PROPERTY_UID, (XTEPropertyGetter)&_acu_xt_widg_get_str, (XTEElementGetter)&_obj_bg_btn_named, (XTEPropertySetter)_acu_xt_widg_set_str},
    
    {"id", 0, "integer", XTE_PROPERTY_UID, (XTEPropertyGetter)&_acu_xt_obj_get_id, (XTEElementGetter)&_obj_bg_btn_id},
    
    {"style", PROPERTY_STYLE, "string", 0, (XTEPropertyGetter)&_acu_xt_widg_get_str, NULL, (XTEPropertySetter)_acu_xt_widg_set_str},
    NULL,
};

struct XTEPropertyDef _acu_xt_cd_btn_props[] = {
    {"name", PROPERTY_NAME, "string", XTE_PROPERTY_UID, (XTEPropertyGetter)&_acu_xt_widg_get_str, (XTEElementGetter)&_obj_cd_btn_named, (XTEPropertySetter)_acu_xt_widg_set_str},
    
    {"id", 0, "integer", XTE_PROPERTY_UID, (XTEPropertyGetter)&_acu_xt_obj_get_id, (XTEElementGetter)&_obj_cd_btn_id},
    
    {"style", PROPERTY_STYLE, "string", 0, (XTEPropertyGetter)&_acu_xt_widg_get_str, NULL, (XTEPropertySetter)_acu_xt_widg_set_str},
    NULL,
};

struct XTEPropertyDef _acu_xt_card_props[] = {
    {"name", PROPERTY_NAME, "string", XTE_PROPERTY_UID, (XTEPropertyGetter)&_acu_xt_stak_get_str, (XTEElementGetter)&_obj_card_named, (XTEPropertySetter)&_acu_xt_stak_set_str},
    
    {"id", 0, "integer", XTE_PROPERTY_UID, (XTEPropertyGetter)&_acu_xt_obj_get_id, (XTEElementGetter)&_obj_card_id},
    
    {"dontSearch", PROPERTY_DONTSEARCH, "boolean", 0, (XTEPropertyGetter)&_acu_xt_stak_get_bool, NULL, (XTEPropertySetter)&_acu_xt_stak_set_bool},
    {"cantDelete", PROPERTY_CANTDELETE, "boolean", 0, (XTEPropertyGetter)&_acu_xt_stak_get_bool, NULL, (XTEPropertySetter)&_acu_xt_stak_set_bool},
    
    NULL,
};

struct XTEPropertyDef _acu_xt_bkgnd_props[] = {
    {"name", 0, "string", 0, NULL, NULL, NULL},
    
    {"id", 0, "integer", 0, (XTEPropertyGetter)&_acu_xt_obj_get_id, NULL, NULL},
    
    {"dontSearch", PROPERTY_DONTSEARCH, "boolean", 0, (XTEPropertyGetter)&_acu_xt_stak_get_bool, NULL, (XTEPropertySetter)&_acu_xt_stak_set_bool},
    {"cantDelete", PROPERTY_CANTDELETE, "boolean", 0, (XTEPropertyGetter)&_acu_xt_stak_get_bool, NULL, (XTEPropertySetter)&_acu_xt_stak_set_bool},
    
    NULL,
};

struct XTEPropertyDef _acu_xt_stack_props[] = {
    /*{"name", 0, "string", 0, NULL, NULL, NULL},*/
    NULL,
};


struct XTEPropertyDef _acu_xt_msgbox_props[] = {
    NULL,
};









struct XTEPropertyDef _acu_xt_objcount_props[] = {
    {"number", 0, "integer", 0, (XTEPropertyGetter)&_acu_xt_objcount_get_number},
    NULL,
};



struct XTEClassDef _acu_xt_classes[] = {
    /* special count object;
     allows querying the number of a specific object */
    {
        "objcount",
        0,
        sizeof(_acu_xt_objcount_props) / sizeof(struct XTEPropertyDef),
        _acu_xt_objcount_props,
        NULL,
    },
    
    /* stack objects */
    {
        "cdfld",
        0,
        sizeof(_acu_xt_cd_fld_props) / sizeof(struct XTEPropertyDef),
        _acu_xt_cd_fld_props,
        (XTEPropertyGetter)&_obj_fld_read,
        (XTEContainerWriter)&_obj_fld_write,
        (XTEScriptRetriever)&_retr_script,
        (XTENextResponder)&_next_responder,
    },
    {
        "bgfld", /* background field by ID must have a different accessor, thus the entire class definition is replicated,
                   but most implementations can probably be identical */
        0,
        sizeof(_acu_xt_bg_fld_props) / sizeof(struct XTEPropertyDef),
        _acu_xt_bg_fld_props,
        (XTEPropertyGetter)&_obj_fld_read,
        (XTEContainerWriter)&_obj_fld_write,
        (XTEScriptRetriever)&_retr_script,
        (XTENextResponder)&_next_responder,
    },
    {
        "cdbtn",
        0,
        sizeof(_acu_xt_cd_btn_props) / sizeof(struct XTEPropertyDef),
        _acu_xt_cd_btn_props,
        NULL,
        NULL,
        (XTEScriptRetriever)&_retr_script,
        (XTENextResponder)&_next_responder,
    },
    {
        "bgbtn",
        0,
        sizeof(_acu_xt_bg_btn_props) / sizeof(struct XTEPropertyDef),
        _acu_xt_bg_btn_props,
        NULL,
        NULL,
        (XTEScriptRetriever)&_retr_script,
        (XTENextResponder)&_next_responder,
    },
    {
        "card",
        0,
        sizeof(_acu_xt_card_props) / sizeof(struct XTEPropertyDef),
        _acu_xt_card_props,
        NULL,
        NULL,
        (XTEScriptRetriever)&_retr_script,
        (XTENextResponder)&_next_responder,
    },
    {
        "bkgnd",
        0,
        sizeof(_acu_xt_bkgnd_props) / sizeof(struct XTEPropertyDef),
        _acu_xt_bkgnd_props,
        NULL,
        NULL,
        (XTEScriptRetriever)&_retr_script,
        (XTENextResponder)&_next_responder,
    },
    {
        "stack",
        0,
        sizeof(_acu_xt_stack_props) / sizeof(struct XTEPropertyDef),
        _acu_xt_stack_props,
        NULL,
        NULL,
        (XTEScriptRetriever)&_retr_script,
        (XTENextResponder)&_next_responder,
    },
    
    /* palette windows */
    {
        "msgbox",
        0,
        sizeof(_acu_xt_msgbox_props) / sizeof(struct XTEPropertyDef),
        _acu_xt_msgbox_props,
        NULL,
        NULL,
        NULL,
        NULL,
    },
    
    /* special placeholder object for file existance checking */
    {
        "file",
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    },
    
    NULL,
};





struct XTEConstantDef _acu_xt_constants[] = {
    {
        "this stack",
        (XTEPropertyGetter)&_acu_xt_const_this_stack,
    },
    {
        "this background",
        (XTEPropertyGetter)&_acu_xt_const_this_bkgnd,
    },
    {
        "this card",
        (XTEPropertyGetter)&_acu_xt_const_this_card,
    },
    
    
    
    /*
     {
     "CinsImp",
     NULL,
     },
     {
     "message box",
     &imp_msgbox_read,
     },*/
    NULL,
};



struct XTEPropertyDef _acu_xt_properties[] = {
    /*{
     "message box", "message box", 0, &imp_msgbox_read, NULL
     },*/
    NULL,
};





struct XTEElementDef _acu_xt_elements[] = {
    /* field references default to the background layer */
    {
        "fields", "field", "bgfld", 0, (XTEElementGetter)&_obj_bg_fld_range, (XTEElementGetter)&_obj_bg_fld_named,
        (XTEPropertyGetter)&_acu_xt_objcount_get_number
    },
    {
        "card fields", "card field", "cdfld", 0, (XTEElementGetter)&_obj_cd_fld_range, (XTEElementGetter)&_obj_cd_fld_named,
        (XTEPropertyGetter)&_acu_xt_objcount_get_number
    },
    {
        "background fields", "background field", "bgfld", 0, (XTEElementGetter)&_obj_bg_fld_range, (XTEElementGetter)&_obj_bg_fld_named,
        (XTEPropertyGetter)&_acu_xt_objcount_get_number
    },
    
    /* button references default to the card layer */
    {
        "buttons", "button", "cdbtn", 0, (XTEElementGetter)&_obj_cd_btn_range, (XTEElementGetter)&_obj_cd_btn_named,
        (XTEPropertyGetter)&_acu_xt_objcount_get_number
    },
    {
        "card buttons", "card button", "cdbtn", 0, (XTEElementGetter)&_obj_cd_btn_range, (XTEElementGetter)&_obj_cd_btn_named,
        (XTEPropertyGetter)&_acu_xt_objcount_get_number
    },
    {
        "background buttons", "background button", "bgbtn", 0, (XTEElementGetter)&_obj_bg_btn_range, (XTEElementGetter)&_obj_bg_btn_named,
        (XTEPropertyGetter)&_acu_xt_objcount_get_number
    },
    
    {
        "cards", "card", "card", 0, (XTEElementGetter)&_obj_card_range, (XTEElementGetter)&_obj_card_named,
        (XTEPropertyGetter)&_acu_xt_objcount_get_number
    },
    NULL,
};




struct XTESynonymDef _acu_xt_synonyms[] = {
    {"cd", "card"},
    {"cds", "cards"},
    {"bg", "background"},
    {"bgs", "backgrounds"},
    {"bkgnd", "background"},
    {"bkgnds", "backgrounds"},
    {"fld", "field"},
    {"flds", "fields"},
    {"btn", "button"},
    {"btns", "buttons"},
    {"msg", "message box"},
    {"message", "message box"},
    {"msg box", "message box"},
    NULL,
};


struct XTEFunctionDef _acu_xt_functions[] = {
    /*{
     "offset", 2
     },*/
    NULL
};




