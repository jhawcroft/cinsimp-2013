/*
 
 Paint
 paint_shape_bas.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Basic shapes: rectangle, rounded rectangle, oval
 
 *************************************************************************************************
 */

#include "paint_int.h"


/***********
 Tool Implementations
 */

/*
 *  _paint_oval_draw
 *  ---------------------------------------------------------------------------------------------
 *  Draws an oval into the destination context.
 */

void _paint_oval_draw(Paint *in_paint, CGContextRef in_dest)
{
    assert(in_paint != NULL);
    assert(in_dest != NULL);
    
    CGContextBeginPath(in_dest);
    CGRect the_rect = CGRectMake(in_paint->starting_point.x,
                                 in_paint->starting_point.y,
                                 in_paint->ending_point.x - in_paint->starting_point.x,
                                 in_paint->ending_point.y - in_paint->starting_point.y);
    the_rect = _paint_cgrect_scale_auto(in_paint, the_rect);
    CGContextAddEllipseInRect(in_dest, the_rect);
    
    if (in_paint->draw_filled) CGContextFillPath(in_dest);
    else CGContextStrokePath(in_dest);
}


/*
 *  _paint_round_rect_draw
 *  ---------------------------------------------------------------------------------------------
 *  Draws a round-rectangle into the destination context.
 */

void _paint_round_rect_draw(Paint *in_paint, CGContextRef in_dest)
{
    assert(in_paint != NULL);
    assert(in_dest != NULL);
    
    /* don't draw a round-rectangle if the size would be smaller than twice the radius;
     we'll end up with arcs pointing in all directions and it wont look nice */
    if ( (abs(in_paint->ending_point.x - in_paint->starting_point.x) < in_paint->round_rect_radius * 2) ||
        (abs(in_paint->ending_point.y - in_paint->starting_point.y) < in_paint->round_rect_radius * 2) ) return;
    
    CGRect rrect = CGRectMake(in_paint->starting_point.x,
                              in_paint->starting_point.y,
                              in_paint->ending_point.x - in_paint->starting_point.x,
                              in_paint->ending_point.y - in_paint->starting_point.y);
    rrect = _paint_cgrect_scale_auto(in_paint, rrect);
    
    /* see below for how this works;
     code borrowed from an Apple Quartz drawing example */
    CGFloat minx = CGRectGetMinX(rrect), midx = CGRectGetMidX(rrect), maxx = CGRectGetMaxX(rrect);
    CGFloat miny = CGRectGetMinY(rrect), midy = CGRectGetMidY(rrect), maxy = CGRectGetMaxY(rrect);
    
    CGContextMoveToPoint(in_dest, minx, midy);
    CGContextAddArcToPoint(in_dest, minx, miny, midx, miny, _paint_scale_auto(in_paint, in_paint->round_rect_radius));
    CGContextAddArcToPoint(in_dest, maxx, miny, maxx, midy, _paint_scale_auto(in_paint, in_paint->round_rect_radius));
    CGContextAddArcToPoint(in_dest, maxx, maxy, midx, maxy, _paint_scale_auto(in_paint, in_paint->round_rect_radius));
    CGContextAddArcToPoint(in_dest, minx, maxy, minx, midy, _paint_scale_auto(in_paint, in_paint->round_rect_radius));
    CGContextClosePath(in_dest);
    
    if (in_paint->draw_filled) CGContextFillPath(in_dest);
    else CGContextStrokePath(in_dest);
}

/*
 Apple explanation for round-rectangle drawing using arcs:
 -------------------------------------------------------------------------------------------------
 In order to draw a rounded rectangle, we will take advantage of the fact that
 CGContextAddArcToPoint will draw straight lines past the start and end of the arc
 in order to create the path from the current position and the destination position.
 
 In order to create the 4 arcs correctly, we need to know the min, mid and max positions
 on the x and y lengths of the given rectangle.
 
 Next, we will go around the rectangle in the order given by the figure below.
        minx    midx    maxx
 miny   2       3       4
 midy   1       9       5
 maxy   8       7       6
 
 Which gives us a coincident start and end point, which is incidental to this technique, but still doesn't
 form a closed path, so we still need to close the path to connect the ends correctly.
 Thus we start by moving to point 1, then adding arcs through each pair of points that follows.
 You could use a similar technique to create any shape with rounded corners.
 */



/*
 *  _paint_rect_draw
 *  ---------------------------------------------------------------------------------------------
 *  Draws a rectangle into the destination context.
 */

void _paint_rect_draw(Paint *in_paint, CGContextRef in_dest)
{
    //CGContextSetShouldAntialias(in_dest, PAINT_FALSE);
    
    CGContextBeginPath(in_dest);
    CGRect the_rect = CGRectMake(in_paint->starting_point.x,
                                 in_paint->starting_point.y,
                                 in_paint->ending_point.x - in_paint->starting_point.x,
                                 in_paint->ending_point.y - in_paint->starting_point.y);
    the_rect = _paint_cgrect_scale_auto(in_paint, the_rect);
    CGContextAddRect(in_dest, the_rect);
    
    if (in_paint->draw_filled) CGContextFillPath(in_dest);
    else CGContextStrokePath(in_dest);
}






