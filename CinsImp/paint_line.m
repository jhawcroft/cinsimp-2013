/*
 
 Paint
 paint_line.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Line tool
 
 *************************************************************************************************
 */

#include "paint_int.h"


void _paint_line_end(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint->current_tool == PAINT_TOOL_LINE);
    
    CGContextSetShouldAntialias(in_paint->context_primary, 1);
    CGContextSetBlendMode(in_paint->context_primary, kCGBlendModeNormal);
   
    //CGContextSetRGBStrokeColor(in_paint->context_primary, in_paint->red, in_paint->green, in_paint->blue, 1.0);
    CGContextSetLineWidth(in_paint->context_primary, in_paint->line_size);
    CGContextBeginPath(in_paint->context_primary);
    CGContextMoveToPoint(in_paint->context_primary, in_paint->starting_point.x + 0.5, in_paint->starting_point.y + 0.5);
    CGContextAddLineToPoint(in_paint->context_primary, in_paint->ending_point.x + 0.5, in_paint->ending_point.y + 0.5);
    CGContextClosePath(in_paint->context_primary);
    CGContextStrokePath(in_paint->context_primary);
    CGContextSetShouldAntialias(in_paint->context_primary, 1);
    
    
}


void _paint_line_drawing(Paint *in_paint, CGContextRef in_dest)
{
    if ((in_paint->current_tool != PAINT_TOOL_LINE) || (!in_paint->stroking)) return;
    
    int x,y;
    
    CGContextSaveGState(in_dest);

    CGContextSetShouldAntialias(in_dest, 1);
    CGContextSetBlendMode(in_dest, kCGBlendModeNormal);
    if ((in_paint->scale != 1) && (in_paint->line_size == 1))
        CGContextSetLineCap(in_dest, kCGLineCapSquare);
    //CGContextSetRGBStrokeColor(in_dest, in_paint->red, in_paint->green, in_paint->blue, 1.0);
    CGContextSetLineWidth(in_dest, in_paint->line_size * in_paint->scale);
    
    CGContextBeginPath(in_dest);
    x = in_paint->starting_point.x;
    y = in_paint->starting_point.y;
    _paint_coord_scale_to_external(in_paint, &x, &y);
    CGContextMoveToPoint(in_dest, x+ (1 * in_paint->scale * 0.6), y+ (1 * in_paint->scale * 0.5));
    x = in_paint->ending_point.x;
    y = in_paint->ending_point.y;
    _paint_coord_scale_to_external(in_paint, &x, &y);
    CGContextAddLineToPoint(in_dest, x+ (1 * in_paint->scale * 0.6), y+ (1 * in_paint->scale * 0.5));
    //CGContextClosePath(in_dest);
    CGContextStrokePath(in_dest);
    
    CGContextRestoreGState(in_dest);
}



