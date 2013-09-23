/*
 
 Paint
 paint_pencil.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Pencil tool
 
 *************************************************************************************************
 */

#include "paint_int.h"



void _paint_pencil_begin(Paint *in_paint, int in_loc_x, int in_loc_y)
{
    assert(in_paint->current_tool == PAINT_TOOL_PENCIL);
    
    float red, green, blue, alpha;
    _paint_pixel_getf(in_paint, in_loc_x, in_loc_y, &red, &green, &blue, &alpha);
    
    in_paint->pencil_is_clearing = _paint_colours_samef(in_paint, red, green, blue, alpha,
                                                        in_paint->red, in_paint->green, in_paint->blue, 1.0);

    CGContextSetShouldAntialias(in_paint->context_primary, FALSE);
    CGContextSetBlendMode(in_paint->context_primary, ((in_paint->pencil_is_clearing) ? kCGBlendModeClear : kCGBlendModeNormal));
    //CGContextSetRGBFillColor(in_paint->context_primary, in_paint->red, in_paint->green, in_paint->blue, 1.0);
    
    CGContextFillRect(in_paint->context_primary, CGRectMake(in_loc_x, in_loc_y, 1, 1));
    
    _paint_needs_display(in_paint);
}


void _paint_pencil_continue(Paint *in_paint, int in_loc_x, int in_loc_y)
{
    assert(in_paint->current_tool == PAINT_TOOL_PENCIL);
    
    CGContextSetShouldAntialias(in_paint->context_primary, FALSE);
    CGContextSetBlendMode(in_paint->context_primary, ((in_paint->pencil_is_clearing) ? kCGBlendModeClear : kCGBlendModeNormal));
    //CGContextSetRGBStrokeColor(in_paint->context_primary, in_paint->red, in_paint->green, in_paint->blue, 1.0);
    CGContextSetLineWidth(in_paint->context_primary, 1);
    
    CGContextBeginPath(in_paint->context_primary);
    CGContextMoveToPoint(in_paint->context_primary, in_paint->last_point.x, in_paint->last_point.y);
    CGContextAddLineToPoint(in_paint->context_primary, in_loc_x, in_loc_y);
    CGContextClosePath(in_paint->context_primary);
    CGContextStrokePath(in_paint->context_primary);
}




