/*
 
 Paint
 paint_io.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Input/output of bitmap graphics, primarily for load/store and pasteboard operations
 
 *************************************************************************************************
 */

#include "paint_int.h"


/**********
 Internal I/O Utilities
 */

/*
 *  paint_png_data_get
 *  ---------------------------------------------------------------------------------------------
 *  Obtains PNG data for either the entire canvas, or the selection (whichever is specified.)
 *
 *  If in_finish is PAINT_TRUE, any selection is currently dropped on to the canvas prior to 
 *  obtaining the data.
 *
 *  The data is given in out_data, and the size of the data is returned.  If the data cannot be
 *  obtained for some reason, returns zero and sets out_data to a NULL pointer.
 *
 *  The data returned remains available until the next call, or the paint sub-system instance is
 *  disposed.
 *
 *  ! Obtaining data for the selection when there is NO selection, is an error and will be raised
 *    with the application view.
 */

long _paint_png_data_get(Paint *in_paint, int in_selection, int in_finish, void const **out_data)
{
    assert(in_paint != NULL);
    assert(IS_BOOL(in_selection));
    assert(IS_BOOL(in_finish));
    assert(out_data != NULL);
    
    /* assume the worst */
    *out_data = NULL;
    
    /* if we're finishing the current paint session;
     drop any selection */
    if (in_finish == PAINT_TRUE) _paint_drop_selection(in_paint);
    
    /* clear any previous exported PNG data */
    if (in_paint->temp_export_data) free(in_paint->temp_export_data);
    in_paint->temp_export_data = NULL;
    
    /* get an image of the appropriate context */
    CGImageRef image = NULL;
    if (in_selection == PAINT_TRUE)
    {
        if (in_paint->selection_path == NULL)
        {
            _paint_raise_error(in_paint, PAINT_ERROR_MISUSE);
            return 0;
        }
        image = CGBitmapContextCreateImage(in_paint->context_selection);
    }
    else
        image = CGBitmapContextCreateImage(in_paint->context_primary);
    if (image == NULL)
    {
        _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        return 0;
    }
    
    /* flip the image, upside down! */
    CGImageRef upside_down_image = _paint_cgimage_clone(image, PAINT_TRUE);
    CGImageRelease(image);
    if (upside_down_image == NULL)
    {
        _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        return 0;
    }
    image = upside_down_image;
    
    /* convert the image to raw PNG data */
    CFMutableDataRef data_provider = CFDataCreateMutable(NULL, 0);
    if (data_provider == NULL)
    {
        CGImageRelease(image);
        _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        return 0;
    }
    CGImageDestinationRef dest = CGImageDestinationCreateWithData(data_provider, kUTTypePNG, 1, NULL);
    if (dest == NULL)
    {
        CGImageRelease(image);
        CFRelease(data_provider);
        _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        return 0;
    }
    CGImageDestinationAddImage(dest, image, NULL);
    CGImageDestinationFinalize(dest);
    
    /* copy the data from the Core Foundation data provider
     to an empty block */
    long bytes = CFDataGetLength(data_provider);
    void *data = NULL;
    if ((bytes > 0) && (bytes <= _MAX_SANE_DATA_SIZE))
        data = malloc(bytes);
    if (data == NULL)
    {
        CFRelease(dest);
        CGImageRelease(image);
        CFRelease(data_provider);
        _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        return 0;
    }
    memcpy(data, CFDataGetBytePtr(data_provider), bytes);
    
    /* cleanup */
    CFRelease(dest);
    CGImageRelease(image);
    CFRelease(data_provider);
    
    /* return the result;
     save the result pointer so we can free it later */
    *out_data = data;
    in_paint->temp_export_data = data;
    return bytes;
}



/*
 *  _paint_png_data_to_cgimage
 *  ---------------------------------------------------------------------------------------------
 *  Creates a CGImage wrapper around the supplied data.
 *
 *  Supported formats are those supported by CoreGraphics.  PNG is suggested.
 *
 *  Upon failure, returns NULL.  If the failure is due to an unrecognised format, no further
 *  action is taken.  Otherwise the error will be raised with the application view.
 *
 *  !  The wrapper is only valid while the original data remains valid.  If you want the CGImage
 *     to remain available, you must clone it immediately.
 * 
 *     Apparently CGImageCreateCopy() does not create a deep enough copy of the bitmap.
 *     Use _paint_cgimage_clone() instead.
 */

CGImageRef _paint_png_data_to_cgimage(Paint *in_paint, void *in_data, long in_size)
{
    assert(in_paint != NULL);
    assert(in_data != NULL);
    assert(IS_DATA_SIZE(in_size));
    
    CGDataProviderRef data_provider = CGDataProviderCreateWithData(NULL, in_data, in_size, NULL);
    if (data_provider == NULL)
    {
        _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        return NULL;
    }
    
    CFStringRef       myKeys[2];
    CFTypeRef         myValues[2];
    myKeys[0] = kCGImageSourceShouldCache;
    myValues[0] = (CFTypeRef)kCFBooleanFalse;
    myKeys[1] = kCGImageSourceShouldAllowFloat;
    myValues[1] = (CFTypeRef)kCFBooleanTrue;
    CFDictionaryRef options = CFDictionaryCreate(NULL,
                                                 (const void **)myKeys,
                                                 (const void **)myValues, 2,
                                                 &kCFTypeDictionaryKeyCallBacks,
                                                 &kCFTypeDictionaryValueCallBacks);
    if (options == NULL)
    {
        CGDataProviderRelease(data_provider);
        _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        return NULL;
    }
    
    CGImageSourceRef source = CGImageSourceCreateWithDataProvider(data_provider, options);
    if (source == NULL)
    {
        CFRelease(options);
        CGDataProviderRelease(data_provider);
        return NULL;
    }
    
    CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, NULL);
    
    CFRelease(source);
    CFRelease(options);
    CGDataProviderRelease(data_provider);
    
    return image;
}


/*
 *  _paint_png_data_set
 *  ---------------------------------------------------------------------------------------------
 *  Replaces the canvas paint with that supplied, or pastes a selection with the supplied data.
 *
 *  Supported formats are those supported by CoreGraphics.  PNG is suggested.
 *
 *  Data is copied and can be released or mutated after this call at the discretion of the caller.
 *
 *  Returns PAINT_NO_ERROR if successful, or PAINT_ERROR_FORMAT if the format is unrecognised.
 *  All other errors return an appropriate error code and raise the error in the usual way with
 *  the application.
 */

int _paint_png_data_set(Paint *in_paint, int in_paste, void *in_data, long in_size)
{
    assert(in_paint != NULL);
    assert(IS_BOOL(in_paste));
    assert(in_data != NULL);
    assert(IS_DATA_SIZE(in_size));
    assert(in_paint->context_primary != NULL);
    
    _paint_drop_selection(in_paint);
    
    CGImageRef image = _paint_png_data_to_cgimage(in_paint, in_data, in_size);
    if (image == NULL) return PAINT_ERROR_FORMAT;
    
    /* flip the image, upside down! */
    CGImageRef upside_down_image = _paint_cgimage_clone(image, PAINT_TRUE);
    CGImageRelease(image);
    if (upside_down_image == NULL)
    {
        _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        return 0;
    }
    image = upside_down_image;
    
    if (in_paste) _paint_selection_create_with_cgimage(in_paint, image);
    else
    {
        CGContextClearRect(in_paint->context_primary, CGRectMake(0, 0, in_paint->width, in_paint->height));
        CGContextDrawImage(in_paint->context_primary, CGRectMake(0, 0, CGImageGetWidth(image), CGImageGetHeight(image)), image);
    }
    
    CGImageRelease(image);
    
    _paint_needs_display(in_paint);
    return PAINT_NO_ERROR;
}


/*
 *  _paint_is_empty
 *  ---------------------------------------------------------------------------------------------
 *  Returns PAINT_TRUE if the entire paint area can be considered 'empty'; ie. if the entire
 *  canvas is completely transparent, or in case of a background layer, if it's all white.
 */
static int _paint_is_empty(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    //printf("white: %d\n", in_paint->white_is_transparent);
    //printf("checking: ");
    for (int i = 0; i + 3 < in_paint->bitmap_data_primary_size; i += 4)
    {
        if (in_paint->bitmap_data_primary[i] == 0) continue;
        if ((in_paint->white_is_transparent) &&
            (in_paint->bitmap_data_primary[i+1] == 255) &&
            (in_paint->bitmap_data_primary[i+2] == 255) &&
            (in_paint->bitmap_data_primary[i+3] == 255)) continue;
        //printf("(%d) %d %d %d %d\n", i, in_paint->bitmap_data_primary[i],
        //       in_paint->bitmap_data_primary[i+1],
        //       in_paint->bitmap_data_primary[i+2],
        //       in_paint->bitmap_data_primary[i+3]);
        
        //printf("opaque\n");
        return PAINT_FALSE;
    }
    
    //printf("transparent\n");
    return PAINT_TRUE;
}



/**********
 Public API
 */

long paint_canvas_get_png_data(Paint *in_paint, int in_finish, void const **out_data)
{
    assert(in_paint != NULL);
    assert(IS_BOOL(in_finish));
    assert(out_data != NULL);
    
    /* look for completely transparent paint layer;
     if it's completely transparent then return zero length data */
    if (_paint_is_empty(in_paint))
    {
        *out_data = NULL;
        return 0;
    }
    
    /* return entire canvas */
    long result = _paint_png_data_get(in_paint, PAINT_FALSE, in_finish, out_data);
    if (in_finish) _paint_needs_display(in_paint);
    return result;
}


long paint_selection_get_png_data(Paint *in_paint, void const **out_data)
{
    return _paint_png_data_get(in_paint, PAINT_TRUE, PAINT_FALSE, out_data);
}


int paint_paste(Paint *in_paint, void const **in_data, long in_size)
{
    return _paint_png_data_set(in_paint, PAINT_TRUE, in_data, in_size);
}


int paint_canvas_set_png_data(Paint *in_paint, void const **in_data, long in_size)
{
    return _paint_png_data_set(in_paint, PAINT_FALSE, in_data, in_size);
}








