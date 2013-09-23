/*
 
 Paint
 paint_util.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 General utilities; relied upon by various units of the paint sub-system
 
 *************************************************************************************************
 */

#include "paint_int.h"

#define _BYTES_PER_PIXEL 4
#define _BITS_PER_COMPONENT 8


/*
 *  _paint_create_context
 *  ---------------------------------------------------------------------------------------------
 *  Allocates memory for a bitmap of the supplied size and creates a CoreGraphics context wrapper
 *  around this memory to allow CG drawing operations to be performed.
 *
 *  The returned bitmap is not initalized, ie. contains random junk data and should be cleared
 *  or filled before use.
 *
 *  If there is an error, returns NULL and outputs a NULL bitmap pointer & a zero data size.
 */

CGContextRef _paint_create_context(long pixelsWide, long pixelsHigh, void **out_data, long *out_data_size, int in_flipped)
{
    assert(pixelsWide > 0);
    assert(pixelsHigh > 0);
    assert(out_data != NULL);
    assert(out_data_size != NULL);
    
    CGContextRef        context = NULL;
    CGColorSpaceRef     colorSpace;
    long                bitmapByteCount;
    long                bitmapBytesPerRow;
    void                *bitmapData;
    
    /* always assume the worst! */
    *out_data = NULL;
    *out_data_size = 0;
    
    /* compute some arguments for CGBitmapContextCreate */
    bitmapBytesPerRow   = pixelsWide * _BYTES_PER_PIXEL;
    bitmapByteCount     = bitmapBytesPerRow * pixelsHigh;
    
    colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    if (colorSpace == NULL) return NULL;
    
    /* allocate space for the bitmap */
    bitmapData = malloc(bitmapByteCount);
    if (bitmapData == NULL)
    {
        CGColorSpaceRelease(colorSpace);
        return NULL;
    }
    
    assert(bitmapBytesPerRow * pixelsHigh == bitmapByteCount);
    assert(pixelsWide * pixelsHigh * _BYTES_PER_PIXEL == bitmapByteCount);
    
    /* create a CG context wrapper around the bitmap data */
    context = CGBitmapContextCreate(bitmapData,
                                    pixelsWide,
                                    pixelsHigh,
                                    _BITS_PER_COMPONENT,
                                    bitmapBytesPerRow,
                                    colorSpace,
                                    kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(colorSpace);
    if (context == NULL)
    {
        free(bitmapData);
        return NULL;
    }
    
    /* flip the coordinate space */
    if (in_flipped)
    {
        CGContextTranslateCTM(context, 0.0f, pixelsHigh);
        CGContextScaleCTM(context, 1.0f, -1.0f);
    }
    
    /* return the resulting data, size and context */
    *out_data_size = bitmapByteCount;
    *out_data = CGBitmapContextGetData(context);
    return context;
}


/*
 *  _paint_dispose_context
 *  ---------------------------------------------------------------------------------------------
 *  Releases the context and associated bitmap data previously allocated by _paint_create_context()
 *
 *  Safely ignores NULL arguments individually.
 */

void _paint_dispose_context(CGContextRef in_context, void *in_data)
{
    if (in_context != NULL) CGContextRelease(in_context);
    if (in_data != NULL)
    {
        
        free(in_data);
    }
}


/*
 *  _paint_cgimage_clone
 *  ---------------------------------------------------------------------------------------------
 *  Creates a deep copy of the supplied CGImage.
 *
 *  Specifying in_flip = PAINT_TRUE causes the cloned image to be flipped vertically.
 *
 *  Returns NULL on failure.
 */

CGImageRef _paint_cgimage_clone(CGImageRef in_cgimage, int in_flip)
{
    assert(in_cgimage != NULL);
    assert(IS_BOOL(in_flip));
    
    /* create a new bitmap and context for the image */
    void *data;
    long size;
    CGContextRef context = _paint_create_context(CGImageGetWidth(in_cgimage),
                                                 CGImageGetHeight(in_cgimage),
                                                 &data,
                                                 &size,
                                                 in_flip);
    if (context == NULL) return NULL;
    
    /* copy the image */
    CGRect image_rect = CGRectMake(0, 0, CGImageGetWidth(in_cgimage), CGImageGetHeight(in_cgimage));
    CGContextClearRect(context, image_rect);
    CGContextDrawImage(context, image_rect, in_cgimage);
    
    /* convert the context to an image */
    return CGBitmapContextCreateImage(context);
}


