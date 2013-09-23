//
//  xtalk_wait.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 3/09/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "xtalk_internal.h"


/*
 *  _cmd_wait
 *  ---------------------------------------------------------------------------------------------
 *  Force the script to wait for a specified time interval, or until a condition is satisified.
 */
static void _cmd_wait(XTE *in_engine, void *in_context, XTEVariant *in_params[], int in_param_count)
{
    if (in_param_count < 4)
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if (in_params[2])
    {
        /* while/until [2] condition [3] */
        if (!xte_variant_convert(in_engine, in_params[2], XTE_TYPE_INTEGER))
        {
            xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
            return;
        }
        // // while
        /* wait while/until condition */
        for (;;)
        {
            /* reevaluate the expression */
            XTEVariant *boolean = xte_variant_value(in_engine, xte_evaluate_delayed_param(in_engine, in_params[3]));
            if (!xte_variant_convert(in_engine, boolean, XTE_TYPE_BOOLEAN))
            {
                xte_variant_release(boolean);
                xte_callback_error(in_engine, "Expected true or false here.", NULL, NULL, NULL);
                return;
            }
            int result = xte_variant_as_int(boolean);
            xte_variant_release(boolean);
            if (xte_variant_as_int(in_params[2]) == 0)
            {
                /* while */
                if (!result) return;
            }
            else
            {
                /* until */
                if (result) return;
            }
            
            /* sleep for a short while */
            _xte_platform_suspend_thread(500);
            if (xte_callback_still_busy(in_engine) != XTE_OK) return;
        }
    }
    else
    {
        /* for interval [0] units [1]  */
        if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER))
        {
            xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
            return;
        }
        if ((in_params[1]) && (!xte_variant_convert(in_engine, in_params[1], XTE_TYPE_INTEGER)))
        {
            xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
            return;
        }
        
        /* convert seconds to ticks */
        double ticks;
        if ((!in_params[1]) || (xte_variant_as_int(in_params[1]) == 0))
            ticks = xte_variant_as_double(in_params[0]) * 1000;
        else
            ticks = xte_variant_as_double(in_params[0]);
        
        /* wait the specified number of milliseconds */
        int loop_intervals = ticks * 1000 / 500;
        for (int i = 0; i < loop_intervals; i++)
        {
            /* sleep for a short while */
            _xte_platform_suspend_thread(500);
            if (xte_callback_still_busy(in_engine) != XTE_OK) return;
        }
    }
}



struct XTECommandDef _xte_builtin_wait_cmds[] = {
    {
        "wait {`type``0`while|`1`until} <condition>",
        "interval,unit,type,condition<",
        &_cmd_wait,
    },
    {
        "wait [for] <interval> [`unit``1`tick | `1`ticks | `0`second | `0`seconds | `0`sec | `0`secs]",
        "interval,unit,type,condition",
        &_cmd_wait,
    },
    NULL
};


