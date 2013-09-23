/*
 
 Paint
 paint_shape_poly.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Freeform polygon tool; drawing a closed shape with straight edges.
 
 *************************************************************************************************
 
 Usage Notes
 -------------------------------------------------------------------------------------------------
 Each mouse-up, or touch-off of the user is considered another point in the polygon.  The user can
 finish the polygon by either:
 i)  double-clicking/tapping, which sends the paint_event_escape() to the paint sub-system, or
 ii) clicking/tapping near the original start location (within _POLY_END_THRESHOLD below)
 
 */

#include "paint_int.h"


/***********
 Configuration
 */

/*
 *  _POLY_END_THRESHOLD
 *  ---------------------------------------------------------------------------------------------
 *  Maximum number of pixels between start and finish points for the points to be considered the
 *  same, eg. to allow user to imprecisely click near the end of the polygon to close it.
 */

#define _POLY_END_THRESHOLD 10


/***********
 Tool Implementations
 */

/*
 *  _paint_freepoly_begin
 *  ---------------------------------------------------------------------------------------------
 *  Starts the drawing of a free polygon.  The path the mouse/touch follows must be recorded.
 */

void _paint_freepoly_begin(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y));
    
    in_paint->stroking_poly = PAINT_TRUE; /* special mode for drawing polygons,
                                           allows the user to release the mouse/touch
                                           and make clicks/touches for each point */
    
    if (in_paint->shape_path) CGPathRelease(in_paint->shape_path);
    in_paint->shape_path = CGPathCreateMutable();
    if (in_paint->shape_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    
    CGPathMoveToPoint(in_paint->shape_path, NULL, in_x, in_y);
    in_paint->poly_start = CGPointMake(in_x, in_y);
    in_paint->poly_point_count = 1;
}


/*
 *  _paint_freepoly_end
 *  ---------------------------------------------------------------------------------------------
 *  Finishes the drawing of a free polygon.  The recorded path is closed and stroked appropriately.
 */

void _paint_freepoly_end(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y));
    
    if (in_paint->shape_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    
    /* add the last point */
    CGPathAddLineToPoint(in_paint->shape_path, NULL, in_x, in_y);
    CGPathCloseSubpath(in_paint->shape_path);
    
    /* copy the complete shape path to the primary context */
    CGContextBeginPath(in_paint->context_primary);
    CGContextAddPath(in_paint->context_primary, in_paint->shape_path);
    
    /* stroke */
    if (in_paint->draw_filled) CGContextFillPath(in_paint->context_primary);
    else CGContextStrokePath(in_paint->context_primary);
    
    in_paint->stroking_poly = PAINT_FALSE; /* end the special polygon point placement mode */
}


/*
 *  _paint_freepoly_continue
 *  ---------------------------------------------------------------------------------------------
 *  Continues the drawing of a free polygon.  The path the mouse/touch follows is recorded.
 */

void _paint_freepoly_continue(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y));
    
    if (in_paint->shape_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    
    /* determine if the user has placed a point very near the start (within _POLY_END_THRESHOLD),
     if they have, we should end the polygon at the start location */
    if ( (abs(in_x - in_paint->poly_start.x) <= _POLY_END_THRESHOLD) &&
        (abs(in_y - in_paint->poly_start.y) <= _POLY_END_THRESHOLD) &&
        (in_paint->poly_point_count > 1))
    {
        _paint_freepoly_end(in_paint, in_paint->poly_start.x, in_paint->poly_start.y);
        return;
    }
    
    /* continue adding points */
    CGPathAddLineToPoint(in_paint->shape_path, NULL, in_x, in_y);
    in_paint->poly_point_count++;
}


/*
 *  _paint_freepoly_drawing
 *  ---------------------------------------------------------------------------------------------
 *  A free polygon is being drawn; we should draw the recorded path (thus far) to the destination.
 */

void _paint_freepoly_drawing(Paint *in_paint, CGContextRef in_dest)
{
    assert(in_paint != NULL);
    assert(in_dest != NULL);
    
    if (in_paint->shape_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    
    CGPathRef scaled_shape_path = _paint_cgpath_copy_scale_auto(in_paint, in_paint->shape_path);
    if (scaled_shape_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    
    CGContextBeginPath(in_dest);
    CGContextAddPath(in_dest, scaled_shape_path);
    CGPathRelease(scaled_shape_path);
    
    CGContextAddLineToPoint(in_dest,
                            in_paint->ending_point.x * in_paint->scale,
                            in_paint->ending_point.y * in_paint->scale);
    
    CGContextStrokePath(in_dest);
}




