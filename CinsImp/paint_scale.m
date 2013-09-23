/*
 
 Paint
 paint_scale.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Scaling system; used to implement 'Fat Bits' and 'zoom' functions in the UI
 
 *************************************************************************************************
 */

#include "paint_int.h"


/* I had it originally on 15, 10 seems alright, HC uses 8 */
#define _FAT_BITS_MAGNIFICATION_SCALE 10



/**********
 Internal Utilities
 */

void _paint_canvas_changed(Paint *in_paint)
{
    if (in_paint->selection_path)
    {
        in_paint->callbacks.display_change_handler(in_paint, in_paint->callback_context,
                                           in_paint->width * in_paint->scale,
                                           in_paint->height * in_paint->scale,
                                           (in_paint->selection_bounds.origin.x + (in_paint->selection_bounds.size.width / 2))
                                           * in_paint->scale,
                                           (in_paint->selection_bounds.origin.y + (in_paint->selection_bounds.size.height / 2))
                                           * in_paint->scale);
    }
    else
    {
        in_paint->callbacks.display_change_handler(in_paint, in_paint->callback_context,
                                           in_paint->width * in_paint->scale,
                                           in_paint->height * in_paint->scale,
                                           in_paint->starting_point.x * in_paint->scale,
                                           in_paint->starting_point.y * in_paint->scale);
    }
}


void _paint_coord_scale_to_internal(Paint *in_paint, int *io_x, int *io_y)
{
    *io_x /= in_paint->scale;
    *io_y /= in_paint->scale;
}


void _paint_coord_scale_to_external(Paint *in_paint, int *io_x, int *io_y)
{
    *io_x *= in_paint->scale;
    *io_y *= in_paint->scale;
}


CGPoint _paint_cgpoint_scale_to_display(Paint *in_paint, CGPoint in_point)
{
    in_point.x *= in_paint->scale;
    in_point.y *= in_paint->scale;
    return in_point;
}


CGPoint _paint_cgpoint_scale_to_canvas(Paint *in_paint, CGPoint in_point)
{
    in_point.x /= in_paint->scale;
    in_point.y /= in_paint->scale;
    return in_point;
}




CGRect _paint_cgrect_scale_to_display(Paint *in_paint, CGRect in_rect)
{
    return CGRectMake(in_rect.origin.x * in_paint->scale,
                      in_rect.origin.y * in_paint->scale,
                      in_rect.size.width * in_paint->scale,
                      in_rect.size.height * in_paint->scale);
}

/* deprecated; use _paint_cgrect_scale_to_display() */
CGRect _paint_cgrect_scale_to_external(Paint *in_paint, CGRect in_rect)
{
    return _paint_cgrect_scale_to_display(in_paint, in_rect);
}


/* convenience method for drawing; decides if the specified rect needs to be scaled based
 on where the drawing is currently taking place.  scales it if appropriate */
CGRect _paint_cgrect_scale_auto(Paint *in_paint, CGRect in_rect)
{
    if (in_paint->drawing_to_canvas) return in_rect;
    return _paint_cgrect_scale_to_display(in_paint, in_rect);
}


CGPathRef _paint_cgpath_copy_scale_auto(Paint *in_paint, CGPathRef in_path)
{
    CGAffineTransform transformation = CGAffineTransformMakeScale(in_paint->scale, in_paint->scale);
    return CGPathCreateCopyByTransformingPath(in_path, &transformation);
}


/* as above; but returns appropriate scaling factor for current drawing */

float _paint_scale_auto(Paint *in_paint, float in_float)
{
    if (in_paint->drawing_to_canvas) return in_float;
    else return in_paint->scale * in_float;
}



/**********
 Public API
 */

void paint_scale_set(Paint *in_paint, float in_scale)
{
    in_paint->scale = in_scale;
    _paint_canvas_changed(in_paint);
    _paint_needs_display(in_paint);
}


void paint_fat_bits_set(Paint *in_paint, int in_fat_bits)
{
    paint_scale_set(in_paint, (in_fat_bits ? _FAT_BITS_MAGNIFICATION_SCALE : 1));
}


int paint_fat_bits_get(Paint *in_paint)
{
    if (in_paint->scale != 1) return TRUE;
    return FALSE;
}

/*
 use display_scroll_x, display_with, etc. here to select a portion of the primary context
 and display it in minature ...
 */

void paint_draw_minature_into(Paint *in_paint, void *in_context, int in_width, int in_height)
{
    //CGContextRef in_dest = in_context;
}


