/*
 
 Paint
 paint_eraser.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Eraser tool
 
 *************************************************************************************************
 */

#include "paint_int.h"


#define _ERASER_SIZE 16


void _paint_eraser_begin(Paint *in_paint, int in_x, int in_y)
{
    CGContextClearRect(in_paint->context_primary, CGRectMake(in_x, in_y, _ERASER_SIZE, _ERASER_SIZE));
    _paint_needs_display(in_paint);
}


void _paint_eraser_continue(Paint *in_paint, int in_x, int in_y)
{
    CGPoint vector = CGPointMake(in_paint->ending_point.x - in_paint->last_point.x, in_paint->ending_point.y - in_paint->last_point.y);
    CGFloat distance = hypotf(vector.x, vector.y);
    vector.x /= distance;
    vector.y /= distance;
    for (CGFloat i = 0; i < distance; i += 1.0f) {
        CGRect r = CGRectMake(in_paint->last_point.x + i * vector.x,
                              in_paint->last_point.y + i * vector.y,
                              _ERASER_SIZE,
                              _ERASER_SIZE);
        
        CGContextClearRect(in_paint->context_primary, r);
        
    }
    /*
    CGContextSetBlendMode(ctx, kCGBlendModeClear);
    CGContextSetLineWidth(ctx, 16);
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, in_paint->last_point.x + 8, in_paint->last_point.y - 8);
    CGContextAddLineToPoint(ctx, in_loc_x + 8, in_loc_y - 8);
    CGContextClosePath(ctx);
    CGContextStrokePath(ctx);*/
}




