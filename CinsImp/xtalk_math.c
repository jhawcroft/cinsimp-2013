/*
 
 xTalk Engine Math
 xtalk_math.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Math operations and inbuilt functions
 
 */

#include "xtalk_internal.h"



/*********
 Operators
 */

/* ! memory policy : interpreter manages operands for us, we're free to reuse or create anew.
 do not call release on any operand. */

XTEVariant* _xte_op_math_negate(XTE *in_engine, XTEVariant *in_right)
{
    if (!xte_variant_convert(in_engine, in_right, XTE_TYPE_NUMBER))
    {
        xte_callback_error(in_engine, "Expected number here.", NULL, NULL, NULL);
        return NULL;
    }
    
    if (in_right->type == XTE_TYPE_INTEGER)
        in_right->value.integer *= -1;
    else
        in_right->value.real *= -1;
    return in_right;
}


XTEVariant* _xte_op_math_exponent(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_NUMBER)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_NUMBER)) )
    {
        xte_callback_error(in_engine, "Expected number here.", NULL, NULL, NULL);
        return NULL;
    }
    
    if ((in_left->type == XTE_TYPE_INTEGER) && (in_right->type == XTE_TYPE_INTEGER))
        in_left->value.integer = pow(in_left->value.integer, in_right->value.integer);
    else
    {
        in_left->value.real = pow(xte_variant_as_double(in_left), xte_variant_as_double(in_right));
        in_left->type = XTE_TYPE_REAL;
    }
    return in_left;
}


XTEVariant* _xte_op_math_multiply(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_NUMBER)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_NUMBER)) )
    {
        xte_callback_error(in_engine, "Expected number here.", NULL, NULL, NULL);
        return NULL;
    }
    
    if ((in_left->type == XTE_TYPE_INTEGER) && (in_right->type == XTE_TYPE_INTEGER))
        in_left->value.integer = in_left->value.integer * in_right->value.integer;
    else
    {
        in_left->value.real = xte_variant_as_double(in_left) * xte_variant_as_double(in_right);
        in_left->type = XTE_TYPE_REAL;
    }
    
    return in_left;
}


XTEVariant* _xte_op_math_divide_fp(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_NUMBER)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_NUMBER)) )
    {
        xte_callback_error(in_engine, "Expected number here.", NULL, NULL, NULL);
        return NULL;
    }
    
    if (xte_variant_as_double(in_right) == 0)
    {
        in_left->value.real = INFINITY;
        in_left->type = XTE_TYPE_REAL;
        /*in_left->value.utf8_string = _xte_clone_cstr(in_engine, "INF"); 
        in_left->type = XTE_TYPE_STRING;*/
    }
    else
    {
        in_left->value.real = xte_variant_as_double(in_left) / xte_variant_as_double(in_right);
        in_left->type = XTE_TYPE_REAL;
    }
    
    return in_left;
}


XTEVariant* _xte_op_math_divide_int(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_NUMBER)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_NUMBER)) )
    {
        xte_callback_error(in_engine, "Expected number here.", NULL, NULL, NULL);
        return NULL;
    }
    
    if (xte_variant_as_double(in_right) == 0)
    {
        xte_callback_error(in_engine, "Can't divide by zero.", NULL, NULL, NULL);
        return NULL;
    }

    in_left->value.integer = xte_variant_as_double(in_left) / xte_variant_as_double(in_right);
    in_left->type = XTE_TYPE_INTEGER;
    return in_left;
}


XTEVariant* _xte_op_math_modulus(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_NUMBER)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_NUMBER)) )
    {
        xte_callback_error(in_engine, "Expected number here.", NULL, NULL, NULL);
        return NULL;
    }
    
    if (xte_variant_as_double(in_right) == 0)
    {
        xte_callback_error(in_engine, "Can't divide by zero.", NULL, NULL, NULL);
        return NULL;
    }
    
    if ((in_left->type == XTE_TYPE_INTEGER) && (in_right->type == XTE_TYPE_INTEGER))
    {
        in_left->value.integer = xte_variant_as_int(in_left) % xte_variant_as_int(in_right);
        in_left->type = XTE_TYPE_INTEGER;
    }
    else
    {
        in_left->value.real = fmod(xte_variant_as_double(in_left), xte_variant_as_double(in_right));
        in_left->type = XTE_TYPE_REAL;
    }
    return in_left;
}


XTEVariant* _xte_op_math_add(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_NUMBER)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_NUMBER)) )
    {
        xte_callback_error(in_engine, "Expected number here.", NULL, NULL, NULL);
        return NULL;
    }
    
    if ((in_left->type == XTE_TYPE_INTEGER) && (in_right->type == XTE_TYPE_INTEGER))
        in_left->value.integer = in_left->value.integer + in_right->value.integer;
    else
    {
        in_left->value.real = xte_variant_as_double(in_left) + xte_variant_as_double(in_right);
        in_left->type = XTE_TYPE_REAL;
    }
    return in_left;
}


XTEVariant* _xte_op_math_subtract(XTE *in_engine, XTEVariant *in_left, XTEVariant *in_right)
{
    if ( (!xte_variant_convert(in_engine, in_left, XTE_TYPE_NUMBER)) ||
        (!xte_variant_convert(in_engine, in_right, XTE_TYPE_NUMBER)) )
    {
        xte_callback_error(in_engine, "Expected number here.", NULL, NULL, NULL);
        return NULL;
    }
    
    if ((in_left->type == XTE_TYPE_INTEGER) && (in_right->type == XTE_TYPE_INTEGER))
        in_left->value.integer = in_left->value.integer - in_right->value.integer;
    else
    {
        in_left->value.real = xte_variant_as_double(in_left) - xte_variant_as_double(in_right);
        in_left->type = XTE_TYPE_REAL;
    }
    
    return in_left;
}



/*********
 Functions
 */

/* return the absolute value of the parameter */
static XTEVariant* _xte_math_abs(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER)) return NULL;
    if (xte_variant_type(in_params[0]) == XTE_TYPE_INTEGER)
        return xte_integer_create(in_engine, (xte_variant_as_int(in_params[0]) < 0 ?
                                              xte_variant_as_int(in_params[0])*-1 :
                                              xte_variant_as_int(in_params[0])));
    else
        return xte_real_create(in_engine, (xte_variant_as_double(in_params[0]) < 0 ?
                                           xte_variant_as_double(in_params[0])*-1 :
                                           xte_variant_as_double(in_params[0])));
}


static void _xte_math_aggregate_params(XTE *in_engine, XTEVariant *in_params[], int in_param_count,
                                         double *out_sum, int *out_count, double *out_min, double *out_max)
{
    double double_sum = 0, double_min = INFINITY, double_max = -INFINITY;
    int count = 0;
    for (int p = 0; p < in_param_count; p++)
    {
        XTEVariant *the_param = in_params[p];
        if ((xte_variant_type(the_param) == XTE_TYPE_INTEGER)
            || (xte_variant_type(the_param) == XTE_TYPE_REAL))
        {
            double value = xte_variant_as_double(the_param);
            double_sum += value;
            if (value < double_min) double_min = value;
            if (value > double_max) double_max = value;
            count++;
        }
        else if (xte_variant_convert(in_engine, the_param, XTE_TYPE_STRING))
        {
            char **items;
            int item_count;
            _xte_itemize_cstr(in_engine, xte_variant_as_cstring(the_param), ",", &items, &item_count);
            for (int i = 0; i < item_count; i++)
            {
                double value = atof(items[i]);
                double_sum += value;
                if (value < double_min) double_min = value;
                if (value > double_max) double_max = value;
                count++;
            }
            _xte_cstrs_free(items, item_count);
        }
        else
        {
            xte_callback_error(in_engine, "Expected number.", NULL, NULL, NULL);
            return;
        }
    }
    if (count == 0)
    {
        double_min = 0;
        double_max = 0;
    }
    if (out_sum) *out_sum = double_sum;
    if (out_count) *out_count = count;
    if (out_min) *out_min = double_min;
    if (out_max) *out_max = double_max;
}


/* return the minimum of a list of numbers (see also sum & average for more info) */
static XTEVariant* _xte_math_min(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    double double_min = 0;
    if (in_param_count == 0) return NULL;
    _xte_math_aggregate_params(in_engine, in_params, in_param_count, NULL, NULL, &double_min, NULL);
    return xte_real_create(in_engine, double_min);
}


/* return the maximum of a list of numbers (see also sum & average for more info) */
static XTEVariant* _xte_math_max(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    double double_max = 0;
    if (in_param_count == 0) return NULL;
    _xte_math_aggregate_params(in_engine, in_params, in_param_count, NULL, NULL, NULL, &double_max);
    return xte_real_create(in_engine, double_max);
}


/* return the sum of the numbers in each of the container parameters,
 if a parameter is a string, comma-delimited numbers are summed (regardless of the itemDelimiter). */
static XTEVariant* _xte_math_sum(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    double double_sum = 0;
    if (in_param_count == 0) return NULL;
    _xte_math_aggregate_params(in_engine, in_params, in_param_count, &double_sum, NULL, NULL, NULL);
    return xte_real_create(in_engine, double_sum);
}


/* return the average of the numbers in each of the container parameters,
 if a parameter is a string, comma-delimited numbers are summed (regardless of the itemDelimiter). */
static XTEVariant* _xte_math_average(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    double double_sum = 0;
    int count;
    if (in_param_count == 0) return NULL;
    _xte_math_aggregate_params(in_engine, in_params, in_param_count, &double_sum, &count, NULL, NULL);
    return xte_real_create(in_engine, double_sum / count);
}


/* return the arc-tangent of a number in radians */
static XTEVariant* _xte_math_atan(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER)) return NULL;
    return xte_real_create(in_engine, atan( xte_variant_as_double(in_params[0]) ));
}


/* return the cosine of a number in radians */
static XTEVariant* _xte_math_cos(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER)) return NULL;
    return xte_real_create(in_engine, cos( xte_variant_as_double(in_params[0]) ));
}

/* return the sine of a number in radians */
static XTEVariant* _xte_math_sin(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER)) return NULL;
    return xte_real_create(in_engine, sin( xte_variant_as_double(in_params[0]) ));
}

/* return the tangent of a number in radians */
static XTEVariant* _xte_math_tan(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER)) return NULL;
    return xte_real_create(in_engine, tan( xte_variant_as_double(in_params[0]) ));
}

/* return the square root of a number */
static XTEVariant* _xte_math_sqrt(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER)) return NULL;
    return xte_real_create(in_engine, sqrt( xte_variant_as_double(in_params[0]) ));
}


static XTEVariant* _xte_math_round(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER)) return NULL;
    return xte_real_create(in_engine, round( xte_variant_as_double(in_params[0]) ));
}


static XTEVariant* _xte_math_trunc(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER)) return NULL;
    return xte_real_create(in_engine, trunc( xte_variant_as_double(in_params[0]) ));
}


static XTEVariant* _xte_math_random(XTE *in_engine, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER)) return NULL;
    return xte_integer_create(in_engine, _xte_random( xte_variant_as_double(in_params[0]) ));
}



struct XTEFunctionDef _xte_builtin_math_functions[] = {
    { "abs", 1, &_xte_math_abs },
    { "sum", -1, &_xte_math_sum },
    { "average", -1, &_xte_math_average },
    { "min", -1, &_xte_math_min },
    { "max", -1, &_xte_math_max },
    { "atan", 1, &_xte_math_atan },
    { "cos", 1, &_xte_math_cos },
    { "sin", 1, &_xte_math_sin },
    { "tan", 1, &_xte_math_tan },
    { "sqrt", 1, &_xte_math_sqrt },
    { "round", 1, &_xte_math_round },
    { "trunc", 1, &_xte_math_trunc },
    { "random", 1, &_xte_math_random },
    NULL
};

