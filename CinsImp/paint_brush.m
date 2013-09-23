/*
 
 Paint
 paint_brush.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Paint brush tool; applies a continuous pattern of paint in the current brush shape
 
 *************************************************************************************************
 */

#include "paint_int.h"


/***********
 Pre-computed Data
 */

/*
 *  paint_brush_shape_set
 *  ---------------------------------------------------------------------------------------------
 *  Set the paint brush shape to the supplied image.
 *
 *  The image should have an alpha mask.
 *  Only the non-transparent pixels of the image will be used to define the brush shape.
 *
 *  Supported formats are those supported by CoreGraphics.  PNG is suggested.
 *
 *  Upon failure, stores a NULL brush shape and reports an error to the application.
 */

void paint_brush_shape_set(Paint *in_paint, void *in_data, long in_size)
{
    assert(in_paint != NULL);
    assert(in_data != NULL);
    assert(in_size > 0);
    assert(IS_DATA_SIZE(in_size));
    
    /* dispose the current brush */
    if (in_paint->brush_mask)
    {
        CGImageRelease(in_paint->brush_mask);
        if (in_paint->brush_computed) CGImageRelease(in_paint->brush_computed);
        in_paint->brush_mask = NULL;
        in_paint->brush_computed = NULL;
    }
    
    /* convert the supplied data to a CG image */
    CGImageRef temp_mask = _paint_png_data_to_cgimage(in_paint, in_data, in_size);
    if (temp_mask == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    
    /* cache a few basic attributes */
    in_paint->brush_width = (int)CGImageGetWidth(temp_mask);
    in_paint->brush_height = (int)CGImageGetHeight(temp_mask);
    
    /* copy the image and store */
    in_paint->brush_mask = _paint_cgimage_clone(temp_mask, PAINT_FALSE);
    CGImageRelease(temp_mask);
    if (in_paint->brush_mask == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
}


/*
 *  _paint_brush_recompute
 *  ---------------------------------------------------------------------------------------------
 *  Recomputes the brush head bitmap in the current colour, ready to be used for painting.
 *
 *  Upon failure, the head will be set to NULL and an error reported to the application.
 *
 *  Should be invoked ONLY from: _paint_state_dependent_tools_recompute() in paint.m
 */

void _paint_brush_recompute(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    /* check current colour is available */
    if (in_paint->colour == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    
    /* if the brush shape has not been specified successfully,
     report the error */
    if (in_paint->brush_mask == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    
    /* dispose current precomputed brush */
    if (in_paint->brush_computed) CGImageRelease(in_paint->brush_computed);
    in_paint->brush_computed = NULL;
    
    /* create a new bitmap and context for the brush head */
    void *data;
    long size;
    CGContextRef brush_computed_context = _paint_create_context(in_paint->brush_width,
                                                                in_paint->brush_height,
                                                                &data,
                                                                &size,
                                                                PAINT_FALSE);
    if (brush_computed_context == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    
    /* copy the brush shape to the precomputed brush head
     using the current colour */
    CGContextSetBlendMode(brush_computed_context, kCGBlendModeNormal);
    CGContextDrawImage(brush_computed_context, CGRectMake(0,
                                                          0,
                                                          in_paint->brush_width,
                                                          in_paint->brush_height), in_paint->brush_mask);
    CGContextSetBlendMode(brush_computed_context, kCGBlendModeSourceIn);
    CGContextSetFillColorWithColor(brush_computed_context, in_paint->colour);
    CGContextFillRect(brush_computed_context, CGRectMake(0, 0, in_paint->brush_width, in_paint->brush_height));
    
    /* create the precomputed brush image */
    in_paint->brush_computed = CGBitmapContextCreateImage(brush_computed_context);
    if (in_paint->brush_computed == NULL)
    {
        /* failed to complete brush head */
        _paint_dispose_context(brush_computed_context, data);
        return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    }
    
    /* cleanup */
    _paint_dispose_context(brush_computed_context, data);
}



/***********
 Implementation
 */

/* begin drawing using the brush */
void _paint_brush_begin(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y));
    
    /* check the brush is computed;
     otherwise report an error */
    if (!in_paint->brush_computed) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    
    /* paint */
    CGContextDrawImage(in_paint->context_primary,
                       CGRectMake(in_x, in_y, in_paint->brush_width, in_paint->brush_height),
                       in_paint->brush_computed);
}


/* continue drawing using the brush */
void _paint_brush_continue(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y));
    
    /* check the brush is computed;
     otherwise report an error */
    if (!in_paint->brush_computed) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    
    /* paint along a line from the last mouse/touch location
     to the present location */
    CGPoint vector = CGPointMake(in_paint->ending_point.x - in_paint->last_point.x, in_paint->ending_point.y - in_paint->last_point.y);
    CGFloat distance = hypotf(vector.x, vector.y);
    vector.x /= distance;
    vector.y /= distance;
    for (CGFloat i = 0; i < distance; i += 1.0f)
    {
        CGRect r = CGRectMake(in_paint->last_point.x + i * vector.x, in_paint->last_point.y + i * vector.y,
                              in_paint->brush_width, in_paint->brush_height);
        CGContextDrawImage(in_paint->context_primary,
                           r,
                           in_paint->brush_computed);
        
    }
}


