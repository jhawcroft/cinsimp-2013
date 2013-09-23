/*
 
 xTalk Engine Date/Time
 xtalk_date.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Implementation of date/time properties, functions and commands
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


static XTEVariant* _xte_prp_date(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    _xte_os_current_datetime(in_engine->os_datetime_context);
    switch (in_representation)
    {
        case XTE_PROPREP_NORMAL:
        case XTE_PROPREP_SHORT:
            return xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_DATE_SHORT));
        case XTE_PROPREP_ABBREVIATED:
            return xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_DATE_ABBREVIATED));
        case XTE_PROPREP_LONG:
            return xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_DATE_LONG));
    }
}


static XTEVariant* _xte_fnc_date(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    return _xte_prp_date(in_engine, NULL, 0, NULL, XTE_PROPREP_NORMAL);
}


static XTEVariant* _xte_prp_time(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    _xte_os_current_datetime(in_engine->os_datetime_context);
    switch (in_representation)
    {
        case XTE_PROPREP_NORMAL:
        case XTE_PROPREP_SHORT:
        case XTE_PROPREP_ABBREVIATED:
            return xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_TIME_SHORT));
        case XTE_PROPREP_LONG:
            return xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_TIME_LONG));
    }
}


static XTEVariant* _xte_fnc_time(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    return _xte_prp_time(in_engine, NULL, 0, NULL, XTE_PROPREP_NORMAL);
}


static XTEVariant* _xte_prp_timestamp(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    _xte_os_current_datetime(in_engine->os_datetime_context);
    return xte_real_create(in_engine, _xte_os_timestamp(in_engine->os_datetime_context));
}


static XTEVariant* _xte_fnc_timestamp(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    return _xte_prp_timestamp(in_engine, NULL, 0, NULL, XTE_PROPREP_NORMAL);
}


static XTEVariant* _xte_prp_dateitems(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    char buffer[1024];
    int y,m,d,h,n,s,w;
    _xte_os_current_datetime(in_engine->os_datetime_context);
    _xte_os_dateitems(in_engine->os_datetime_context, &y, &m, &d, &h, &n, &s, &w);
    sprintf(buffer, "%d,%d,%d,%d,%d,%d,%d", y,m,d,h,n,s,w);
    return xte_string_create_with_cstring(in_engine, buffer);
}


static XTEVariant* _xte_fnc_dateitems(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    return _xte_prp_dateitems(in_engine, NULL, 0, NULL, XTE_PROPREP_NORMAL);
}


XTEVariant* _xte_prp_month(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
    return NULL;
    
    if (!xte_variant_convert(in_engine, in_owner, XTE_TYPE_INTEGER))
    {
        xte_callback_error(in_engine, "Expected integer here.", NULL, NULL, NULL);
        return NULL;
    }
    
    char buffer[100];
    sprintf(buffer, "2013-%02d-01", xte_variant_as_int(in_owner));
    _xte_os_parse_date(in_engine->os_datetime_context, buffer);
    switch (in_representation)
    {
        case XTE_PROPREP_SHORT:
        case XTE_PROPREP_ABBREVIATED:
        case XTE_PROPREP_NORMAL:
            return xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_MONTH_SHORT));
        case XTE_PROPREP_LONG:
            return xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_MONTH_LONG));
    }
}


static XTEVariant* _xte_fnc_month(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    return _xte_prp_month(in_engine, NULL, 0, in_params[0], XTE_PROPREP_NORMAL);
}


XTEVariant* _xte_prp_weekday(XTE *in_engine, void *in_context, int in_prop, XTEVariant *in_owner, XTEPropRep in_representation)
{
    if (!xte_variant_convert(in_engine, in_owner, XTE_TYPE_INTEGER))
    {
        xte_callback_error(in_engine, "Expected integer here.", NULL, NULL, NULL);
        return NULL;
    }
    if ((xte_variant_as_int(in_owner) < 1) || (xte_variant_as_int(in_owner) > 7))
    {
        xte_callback_error(in_engine, "Expected integer between 1 and 7 here.", NULL, NULL, NULL);
        return NULL;
    }
    
    char buffer[100];
    sprintf(buffer, "2013-08-%02d", 19 + xte_variant_as_int(in_owner) - 2);
    _xte_os_parse_date(in_engine->os_datetime_context, buffer);
    switch (in_representation)
    {
        case XTE_PROPREP_SHORT:
        case XTE_PROPREP_ABBREVIATED:
        case XTE_PROPREP_NORMAL:
            return xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_WEEKDAY_SHORT));
        case XTE_PROPREP_LONG:
            return xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_WEEKDAY_LONG));
    }
}


static XTEVariant* _xte_fnc_weekday(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    return _xte_prp_weekday(in_engine, NULL, 0, in_params[0], XTE_PROPREP_NORMAL);
}


struct XTEFunctionDef _xte_builtin_date_functions[] = {
    { "date", 0, &_xte_fnc_date },
    { "time", 0, &_xte_fnc_time },
    { "timestamp", 0, &_xte_fnc_timestamp },
    { "dateitems", 0, &_xte_fnc_dateitems },
    { "month", 1, &_xte_fnc_month },
    { "weekday", 1, &_xte_fnc_weekday },
    NULL
};


struct XTEPropertyDef _xte_builtin_date_properties[] = {
    {"date", 0, "string", 0, &_xte_prp_date, NULL},
    {"time", 0, "string", 0, &_xte_prp_time, NULL},
    {"timestamp", 0, "real", 0, &_xte_prp_timestamp, NULL},
    {"dateitems", 0, "string", 0, &_xte_prp_dateitems, NULL},
    /*{"month", "string", 0, &_xte_prp_month, NULL},
    {"weekday", "string", 0, &_xte_prp_weekday, NULL},*/
    NULL,
};


static void _xte_cmd_convert_date(XTE *in_engine, void *in_context, XTEVariant *in_params[], int in_param_count)
{
    /* grab the value of the input, without loosing it,
     if it's a container reference */
    XTEVariant *subject = in_params[0];
    XTEVariant *value = xte_variant_value(in_engine, subject);
    if (!value)
    {
        xte_callback_error(in_engine, "Expected value here.", NULL, NULL, NULL);
        return;
    }
    
    /* determine the input format and convert to platform dependent date/time value */
    if (xte_variant_type(value) == XTE_TYPE_STRING)
    {
        /* look for commas -> dateItems, or no commas -> date */
        char const *str = xte_variant_as_cstring(value);
        char const *comma = strchr(str, ',');
        if (comma == NULL)
        {
            /* it's a date */
            _xte_os_parse_date(in_engine->os_datetime_context, str);
        }
        else
        {
            /* it's dateitems, split into components */
            char **components;
            int component_count;
            _xte_itemize_cstr(in_engine, str, ",", &components, &component_count);
            if (component_count != 7)
            {
                _xte_cstrs_free(components, component_count);
                xte_callback_error(in_engine, "Expected dateItems here.", NULL, NULL, NULL);
                return;
            }
            
            _xte_os_conv_dateitems(in_engine->os_datetime_context, atoi(components[0]), atoi(components[1]),
                                   atoi(components[2]), atoi(components[3]), atoi(components[4]),
                                   atoi(components[5]), atoi(components[6]));
            _xte_cstrs_free(components, component_count);
        }
    }
    else
    {
        /* assume a timestamp */
        if (!xte_variant_convert(in_engine, value, XTE_TYPE_NUMBER))
        {
            xte_callback_error(in_engine, "Expected number or string here.", NULL, NULL, NULL);
            return;
        }
        _xte_os_conv_timestamp(in_engine->os_datetime_context, xte_variant_as_double(in_params[0]));
    }
    
    /* convert to requested output format */
    XTEVariant *result = NULL;
    switch ( xte_variant_as_int(in_params[1]) )
    {
        case XTE_FORMAT_DATEITEMS:
        {
            char buffer[1024];
            int y,m,d,h,n,s,w;
            _xte_os_dateitems(in_engine->os_datetime_context, &y, &m, &d, &h, &n, &s, &w);
            sprintf(buffer, "%d,%d,%d,%d,%d,%d,%d", y,m,d,h,n,s,w);
            result = xte_string_create_with_cstring(in_engine, buffer);
            break;
        }
        case XTE_FORMAT_TIMESTAMP:
            result = xte_real_create(in_engine, _xte_os_timestamp(in_engine->os_datetime_context));
            break;
        case XTE_DATE_SHORT:
            result = xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_DATE_SHORT));
            break;
        case XTE_DATE_ABBREVIATED:
            result = xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_DATE_ABBREVIATED));
            break;
        case XTE_DATE_LONG:
            result = xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_DATE_LONG));
            break;
        case XTE_TIME_SHORT:
            result = xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_TIME_SHORT));
            break;
        case XTE_TIME_LONG:
            result = xte_string_create_with_cstring(in_engine, _xte_os_date_string(in_engine->os_datetime_context, XTE_TIME_LONG));
            break;
    }
    
    /* if the original input was a container;
     then update it */
    if (xte_variant_is_container(subject) && result)
        xte_container_write(in_engine, subject, result, XTE_TEXT_RANGE_ALL, XTE_PUT_INTO);
    
    /* otherwise, set "it" to the result */
    else if (result)
        xte_set_global(in_engine, "it", result);
}


struct XTECommandDef _xte_builtin_date_cmds[] = {
    {
        "convert <expr> to {`fmtto``-1`timestamp | `-2`dateitems | `0`short date | "
        "`1`abbr date | `1`abbrev date | `1`abbreviated date | `2`long date |"
        "`3`short time | `4`long time}",
        "expr,fmtto",
        &_xte_cmd_convert_date,
    },
    NULL,
};



