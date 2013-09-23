/*
 
 Paint
 paint_filter.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Filter effects; lighten, darken, invert, etc.
 
 *************************************************************************************************
 */

#include "paint_int.h"


static void _paint_filter_invert(Paint *in_paint, CGContextRef in_context)
{
    CGContextSetRGBFillColor(in_context, 1, 1, 1, 1);
    CGContextSetBlendMode(in_context, kCGBlendModeDifference);
    CGContextFillRect(in_context, CGRectMake(0, 0, in_paint->width, in_paint->height));
}


static void _paint_filter_lighten(Paint *in_paint, CGContextRef in_context)
{
    CGContextSetRGBFillColor(in_context, 1, 1, 1, 0.05);
    CGContextSetBlendMode(in_context, kCGBlendModePlusLighter);
    CGContextFillRect(in_context, CGRectMake(0, 0, in_paint->width, in_paint->height));
}


static void _paint_filter_darken(Paint *in_paint, CGContextRef in_context)
{
    CGContextSetRGBFillColor(in_context, 0, 0, 0, 0.05);
    CGContextSetBlendMode(in_context, kCGBlendModePlusDarker);
    CGContextFillRect(in_context, CGRectMake(0, 0, in_paint->width, in_paint->height));
}


static void _paint_filter_greyscale(Paint *in_paint, CGContextRef in_context)
{
    CGContextSetRGBFillColor(in_context, 1, 1, 1, 1);
    CGContextSetBlendMode(in_context, kCGBlendModeColor);
    CGContextFillRect(in_context, CGRectMake(0, 0, in_paint->width, in_paint->height));
}


void paint_apply_filter(Paint *in_paint, int in_filter)
{
    CGContextRef the_target;
    
    if (in_paint->selection_path)
        the_target = in_paint->context_selection;
    else
        the_target = in_paint->context_primary;
    
    switch (in_filter)
    {
        case PAINT_FILTER_LIGHTEN:
            _paint_filter_lighten(in_paint, the_target);
            break;
        case PAINT_FILTER_DARKEN:
            _paint_filter_darken(in_paint, the_target);
            break;
        case PAINT_FILTER_INVERT:
            /* difference + white */
            _paint_filter_invert(in_paint, the_target);
            break;
            
        case PAINT_FILTER_GREYSCALE:
            _paint_filter_greyscale(in_paint, the_target);
            break;
    }
    
    _paint_needs_display(in_paint);
}







