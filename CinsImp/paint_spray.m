/*
 
 Paint
 paint_spray.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 'Spray' paint tool; applies a pattern of paint more 'thickly' the slower the drag/touch
 
 *************************************************************************************************
 */

#include "paint_int.h"



/***********
 Configuration
 */

/*
 *  _SPRAY_HEAD_SIZE
 *  ---------------------------------------------------------------------------------------------
 *  Spray head size in pixels square;
 *  This number must correspond to the size of the _spray_head_pattern[] array below.
 */

#define _SPRAY_HEAD_SIZE 16


/*
 *  _spray_head_pattern
 *  ---------------------------------------------------------------------------------------------
 *  The pattern of paint applied with the spray tool.
 *
 *  Each float corresponds to the alpha of a single pixel, thus 0 means no paint is applied, 
 *  1 means paint is applied completely opaque.
 *
 *  Terminated with -1 for code validation purposes.
 *
 *  This pattern corresponds to that used in HyperCard.
 */

static float _spray_head_pattern[] = {
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    -1
};



/***********
 Pre-computed Data
 */


/*
 *  _paint_spray_recompute
 *  ---------------------------------------------------------------------------------------------
 *  Recomputes the spray head bitmap in the current colour, ready to be used for painting.
 *
 *  Upon failure, the head will be set to NULL and an error reported to the application.
 *
 *  Should be invoked ONLY from: _paint_state_dependent_tools_recompute() in paint.m
 */

void _paint_spray_recompute(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    /* dispose the existing spray head */
    if (in_paint->spray_head) CGImageRelease(in_paint->spray_head);
    in_paint->spray_head = NULL;
    
    /* create a new bitmap and context for the spray head */
    void *data;
    long size;
    CGContextRef head_context = _paint_create_context(_SPRAY_HEAD_SIZE, _SPRAY_HEAD_SIZE, &data, &size, PAINT_FALSE);
    if (head_context == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    
    /* clear the bitmap */
    CGContextClearRect(head_context, CGRectMake(0, 0, _SPRAY_HEAD_SIZE, _SPRAY_HEAD_SIZE));
    
    /* copy the predefined pattern to the spray head bitmap
     using the current colour */
    for (int row = 0; row < _SPRAY_HEAD_SIZE; row++)
    {
        for (int col = 0; col < _SPRAY_HEAD_SIZE; col++)
        {
            CGColorRef alphaColour = CGColorCreateGenericRGB(in_paint->red, in_paint->green, in_paint->blue,
                                                              _spray_head_pattern[row * _SPRAY_HEAD_SIZE + col]);
            if (alphaColour == NULL)
            {
                /* failed to complete spray head */
                _paint_dispose_context(head_context, data);
                return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
            }
            CGContextSetFillColorWithColor(head_context, alphaColour);
            CGContextFillRect(head_context, CGRectMake(col, _SPRAY_HEAD_SIZE - row - 1, 1, 1));
            CGColorRelease(alphaColour);
        }
    }
    
    /* set the head to the completed bitmap */
    in_paint->spray_head = CGBitmapContextCreateImage(head_context);
    if (in_paint->spray_head == NULL)
    {
        /* failed to complete spray head */
        _paint_dispose_context(head_context, data);
        return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    }
    
    /* cleanup */
    _paint_dispose_context(head_context, data);
}



/***********
 Tool Implementation
 */

void _paint_spray_begin(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y));
    
    /* if the spray head has not been created successfully,
     report the error */
    if (in_paint->spray_head == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    
    /* paint spray head pattern */
    CGContextDrawImage(in_paint->context_primary, CGRectMake(in_x - (_SPRAY_HEAD_SIZE/2),
                                                             in_y - (_SPRAY_HEAD_SIZE/2),
                                                             _SPRAY_HEAD_SIZE,
                                                             _SPRAY_HEAD_SIZE), in_paint->spray_head);
}


void _paint_spray_continue(Paint *in_paint, int in_x, int in_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y));
    
    /* if the spray head has not been created successfully,
     report the error */
    if (in_paint->spray_head == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_INTERNAL);
    
    /* paint spray head pattern */
    CGContextDrawImage(in_paint->context_primary, CGRectMake(in_x - (_SPRAY_HEAD_SIZE/2),
                                                             in_y - (_SPRAY_HEAD_SIZE/2),
                                                             _SPRAY_HEAD_SIZE,
                                                             _SPRAY_HEAD_SIZE), in_paint->spray_head);
}


/*
 
 Notes:
 
 An alternative implementation of _continue() could work similarly to the paint brush,
 but time precisely how long has elapsed between mouse events.
 
 This is unlikely to be necessary?  The system should deliver mouse/touch events at
 a controlled pace anyway to allow for measurement of velocity?
 
 We could just ignore events where the mouse hasn't actually moved?  But the system
 probably already does that!
 
 */


