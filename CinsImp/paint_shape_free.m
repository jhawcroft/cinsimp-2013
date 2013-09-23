/*
 
 Paint
 paint_shape_free.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Freehand shape tool; drawing a closed shape freehand
 
 *************************************************************************************************
 */

#include "paint_int.h"



/***********
 Tool Implementations
 */

/*
 *  _paint_freeshape_begin
 *  ---------------------------------------------------------------------------------------------
 *  Starts the drawing of a free shape.  The path the mouse/touch follows must be recorded.
 */

void _paint_freeshape_begin(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y));
    
    if (in_paint->shape_path) CGPathRelease(in_paint->shape_path);
    in_paint->shape_path = CGPathCreateMutable();
    if (in_paint->shape_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    
    CGPathMoveToPoint(in_paint->shape_path, NULL, in_x, in_y);
}


/*
 *  _paint_freeshape_continue
 *  ---------------------------------------------------------------------------------------------
 *  Continues the drawing of a free shape.  The path the mouse/touch follows is recorded.
 */

void _paint_freeshape_continue(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y));
    
    if (in_paint->shape_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    CGPathAddLineToPoint(in_paint->shape_path, NULL, in_x, in_y);
}


/*
 *  _paint_freeshape_end
 *  ---------------------------------------------------------------------------------------------
 *  Finishes the drawing of a free shape.  The recorded path is closed and stroked appropriately.
 */

void _paint_freeshape_end(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y));
    
    /* finish the recorded shape path */
    if (in_paint->shape_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    CGPathAddLineToPoint(in_paint->shape_path, NULL, in_x, in_y);
    CGPathCloseSubpath(in_paint->shape_path);
    
    /* copy the completed shape path to the primary context */
    CGContextBeginPath(in_paint->context_primary);
    CGContextAddPath(in_paint->context_primary, in_paint->shape_path);
    
    /* stroke */
    if (in_paint->draw_filled) CGContextFillPath(in_paint->context_primary);
    else CGContextStrokePath(in_paint->context_primary);
}


/*
 *  _paint_freeshape_drawing
 *  ---------------------------------------------------------------------------------------------
 *  A free shape is being drawn; we should draw the recorded path (thus far) to the destination.
 */

void _paint_freeshape_drawing(Paint *in_paint, CGContextRef in_dest)
{
    assert(in_paint != NULL);
    assert(in_dest != NULL);
    
    if (in_paint->shape_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    
    CGPathRef scaled_shape_path = _paint_cgpath_copy_scale_auto(in_paint, in_paint->shape_path);
    if (scaled_shape_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    
    CGContextBeginPath(in_dest);
    CGContextAddPath(in_dest, scaled_shape_path);
    CGPathRelease(scaled_shape_path);
    
    CGContextStrokePath(in_dest);
}





