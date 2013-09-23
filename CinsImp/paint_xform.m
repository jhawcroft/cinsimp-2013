/*
 
 Paint
 paint_xform.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 General transform routines
 
 *************************************************************************************************
 */

#include "paint_int.h"


void _paint_dispose_selection(Paint *in_paint);


void _paint_apply_xform(Paint *in_paint, CGAffineTransform in_transform, int in_rotate)
{
    CGContextRef the_target;
    
    /* get the target */
    if (in_paint->selection_path)
        the_target = in_paint->context_selection;
    else
        the_target = in_paint->context_primary;
    CGImageRef transform_image = CGBitmapContextCreateImage(the_target);
    
    /* create a transform buffer */
    long width, rotwidth, height, rotheight;
    width = CGImageGetWidth(transform_image);
    height = CGImageGetHeight(transform_image);
    if (in_rotate)
    {
        rotwidth = height;
        rotheight = width;
    }
    else
    {
        rotwidth = width;
        rotheight = height;
    }
    CGContextRef transform_context;
    void *bitmap_data;
    long bitmap_data_size;
    transform_context = _paint_create_context(rotwidth, rotheight, &bitmap_data, &bitmap_data_size, PAINT_FALSE);
    CGContextClearRect(transform_context, CGRectMake(0, 0, rotwidth, rotheight));
    
    /* transform the buffer coordinate space */
    CGContextConcatCTM(transform_context, in_transform);
    
    /* draw the target into the buffer */
    CGContextSetBlendMode(transform_context, kCGBlendModeNormal);
    CGContextDrawImage(transform_context,
                       CGRectMake(0, 0, rotwidth, rotheight),
                       transform_image);
    CGImageRelease(transform_image);
    
    /* replace the target */
    if ((!in_paint->selection_path) || (!in_rotate))
    {
        transform_image = CGBitmapContextCreateImage(transform_context);
        CGContextSetBlendMode(the_target, kCGBlendModeNormal);
        CGContextDrawImage(the_target, CGRectMake(0, 0, rotwidth, rotheight), transform_image);
        CGImageRelease(transform_image);
    }
    else
    {
        /* NOT CURRENTLY REACHABLE AS TRANSFORM ROTATE IS NOT ENABLED
         - NEED TO RECHECK MEMORY ALLOCATION OF SELECTION PATHS, ETC. WHEN THIS IS ENABLED */
        /* replace the selection with the transform buffer */
        CGMutablePathRef new_selection_path = CGPathCreateMutableCopyByTransformingPath(in_paint->selection_path, &in_transform);
        
        _paint_dispose_selection(in_paint);
        
        in_paint->selection_path = new_selection_path;
        in_paint->selection_path_moved = CGPathCreateCopy(in_paint->selection_path);
        in_paint->context_selection = transform_context;
        in_paint->bitmap_data_selection = bitmap_data;
        in_paint->bitmap_data_selection_size = bitmap_data_size;
        
        in_paint->selection_bounds = CGPathGetBoundingBox(in_paint->selection_path);
        in_paint->selection_bounds_moved = in_paint->selection_bounds;
        
        transform_context = NULL; /* don't dispose */
    }
    
    /* cleanup */
    if (transform_context)
        _paint_dispose_context(transform_context, bitmap_data);
    _paint_needs_display(in_paint);
}


void _paint_target_dimensions(Paint *in_paint, long *out_width, long *out_height)
{
    
    if (!in_paint->selection_path)
    {
        if (out_width) *out_width = in_paint->width;
        if (out_height) *out_height = in_paint->height;
    }
    else
    {
        if (out_width) *out_width = in_paint->selection_bounds.size.width;
        if (out_height) *out_height = in_paint->selection_bounds.size.height;
    }
     /*   the_target = in_paint->context_selection;
    else
        the_target = in_paint->context_primary;
    CGImageRef transform_image = CGBitmapContextCreateImage(the_target);*/
}


