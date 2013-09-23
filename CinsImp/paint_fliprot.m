/*
 
 Paint
 paint_fliprot.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Flip horizontal/vertical, rotate left/right
 
 *************************************************************************************************
 */

#include "paint_int.h"

/*
 
 worst case scenario, and actually might be more sensible anyway,
 is that we do rotate and flip ourselves.  might be necessary to preserve scales anyway?
 
 */


void _paint_apply_xform(Paint *in_paint, CGAffineTransform in_transform, int in_rotate);
void _paint_target_dimensions(Paint *in_paint, long *out_width, long *out_height);


void paint_flip_horizontal(Paint *in_paint)
{
    long width;
    _paint_target_dimensions(in_paint, &width, NULL);
    _paint_apply_xform(in_paint, CGAffineTransformMake(-1.0, 0.0, 0.0, 1.0, width, 0.0), 0);
}


void paint_flip_vertical(Paint *in_paint)
{
    long height;
    _paint_target_dimensions(in_paint, NULL, &height);
    _paint_apply_xform(in_paint, CGAffineTransformMake(1.0, 0.0, 0, -1.0, 0.0, height), 0);
}


void paint_rotate_left(Paint *in_paint)
{
    long height;
    _paint_target_dimensions(in_paint, NULL, &height);
    _paint_apply_xform(in_paint, CGAffineTransformMake(0.0, 1.0, -1.0, 0.0, height, 0.0), 1);
}


void paint_rotate_right(Paint *in_paint)
{
    long width;
    _paint_target_dimensions(in_paint, &width, NULL);
    _paint_apply_xform(in_paint, CGAffineTransformMake(0.0, -1.0, 1.0, 0.0, 0.0, width), 1);
}




