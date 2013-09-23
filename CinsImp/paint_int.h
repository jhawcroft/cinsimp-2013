/*
 
 Paint
 paint_int.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Internal paint API; opaque types and commonly used functions
 
 *************************************************************************************************
 
 ! Important Colour Management Note
 -------------------------------------------------------------------------------------------------
 CGColorCreateGenericRGB() is used throughout the sub-system instead of direct use of the more 
 obvious ..SetRGBFill.. and ..SetRGBStroke.. functions.  The primary context is configured to use
 the GenericRGB colour-space.  As a result, using the shorter mechanism results in inprecise 
 colour configuration.
 
 */

/* include guard */
#ifndef JH_PAINT_INT_H
#define JH_PAINT_INT_H


/***********
 Headers
 */

/* our own public API */
#include "paint.h"

/* platform agnostic C headers */
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Apple host OS headers */
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h> /* Application Services (OS X) / Image IO framework (iOS) */



/***********
 Limits
 */

#define _MAX_CANVAS_SIZE_PIXELS 32000

#define _MAX_SANE_DATA_SIZE 100 * 1024 * 1024 /* 100 MB */



/***********
 Configuration
 */

/* in seconds; the interval between each refresh and advance of the 'marching ants'
 used for selection boundaries */
#define _MARCHING_ANTS_INTERVAL 0.1 /* 1/10th of a second */




/***********
 Assertion Related Macros
 */

#define IS_BOOL(x) ((x == !0) || (x == !!0))
#define IS_COORD(x) ((x >= 0) || (x <= _MAX_CANVAS_SIZE_PIXELS))
#define IS_DATA_SIZE(x) ((x >= 0) && (x <= _MAX_SANE_DATA_SIZE))



/***********
 Internal Types
 */




/* state for an instance of the paint sub-system */
struct Paint
{
    /*
     fundamentals
     */
    
    /* error reporting */
    int error_reported;
    
    /* size of the paintable canvas area */
    int width;
    int height;
    
    /* application callbacks and context */
    PaintCallbacks callbacks;
    void *callback_context;
    
    /* offscreen bitmap and context for the paintable canvas;
     drawing happens on this;
     allocated by _paint_init() and not disposed of until
     paint_dispose() is called by the application. */
    CGContextRef context_primary;
    unsigned char *bitmap_data_primary;
    long bitmap_data_primary_size;
    long bitmap_data_primary_bytes_per_row;
    
    /* is white transparent?
     currently only used in I/O to determine if the canvas is 'empty' */
    int white_is_transparent;
    
    /* currently selected tool */
    int current_tool;
    
    /* magnification factor for editing;
     1 = normal,
     > 1 = 'Fat Bits' mode (see _scale.m for more information) */
    float scale;
    
    /* set to PAINT_TRUE when painting routines are drawing to the internal canvas (primary context),
     and PAINT_FALSE when painting routines are drawing to the display area (scaled) */
    int drawing_to_canvas;
    
    /*
     general event tracking (most tools)
     */
    
    /* where did the drag/touch start? */
    CGPoint starting_point;
    
    /* where (currently) is the drag/touch ending? */
    CGPoint ending_point;
    
    /* where was the drag/touch immediately prior to this latest event? */
    CGPoint last_point;
    
    /* is the user currently drawing/stroking?
     ie. is the mouse down or is there finger still touching the trackpad/screen */
    int stroking;
    
    /* region of the canvas currently displayed in containing scroll view (if present)
     coordinates are not scaled?   ***TO BE CHECKED***
     */
    int display_width;
    int display_height;
    int display_scroll_x;
    int display_scroll_y;
    
    /* current colour */
    float red;
    float green;
    float blue;
    CGColorRef colour;
    
    
    /*
     selection
     */
    
    /* is the selection rectangular? */
    int selection_is_rect;
    
    /* timer to animate the 'marching ants' selection boundary */
    CFRunLoopTimerRef timer;
    
    /* selection path;
     also used during construction of the selection (mutable) */
    CGMutablePathRef selection_path;
    
    /*  ?? */
    CGPathRef selection_path_moved;
    
    /* ?? */
    CGPathRef scaled_selection;
    
    /* phase of the 'marching ants' boundary of the selection */
    int ants_phase;
    int ants_phase_180;
    
    /* separate offscreen bitmap and context for the selected paint */
    CGContextRef context_selection;
    unsigned char *bitmap_data_selection;
    long bitmap_data_selection_size;
    
    /* the selection is being moved by the user */
    int dragging_selection;
    
    /* smallest rectangle that completely contains the selection */
    CGRect selection_bounds;
    
    CGRect selection_bounds_moved;
    
    
    /*
     managed temporary data 
     */
    
    /* used by _io.m to provide data to the application */
    void *temp_export_data;
    
    
    /*
     tool: spray
     */
    
    CGImageRef spray_head;
    
    /*
     tool: pencil
     */
 
    BOOL pencil_is_clearing;
    
    /*
     tool: brush 
     */
    CGImageRef brush_mask;
    CGImageRef brush_computed;
    int brush_width;
    int brush_height;
    
    /* 
     tools: line, rectangle, rounded rect, oval, freeform shape, freeform poly 
     */
    
    /* thickness of a drawn line */
    int line_size;
    
    
    /*
     tools: rectangle, rounded rect, oval, freeform shape, freeform poly
     */
    
    /* should shapes be filled (coloured) in? */
    int draw_filled;
    
    /* path under construction of shape (mutable) */
    CGMutablePathRef shape_path;
    
    
    /*
     tool: rounded rectangle
     */
    
    /* radius of the rounded rectangle, see _shape_bas.m for more information */
    int round_rect_radius;
    
    
    /*
     tool: freeform poly
     */
    
    /* where was the polygon started?
     must be handled separately to starting_point because the mouse may be clicked
     several times while the user is constructing a polygon */
    CGPoint poly_start;
    
    /* is the user currently constructing a polygon?
     ie. have they pressed the mouse down or started a touch at least once;
     used by display routines to determine that display will require refresh
     in response to movement of the mouse/finger */
    BOOL stroking_poly;
    
    /* how many points have been plotted so far?
     used to detect if the user clicks/touches very close to the poly_start
     so that the polygon can be closed and stroked */
    int poly_point_count;
};



/***********
 Shared Internal API
 */

void _paint_change_tool(Paint *in_paint, int in_tool, int in_by_user);

void _paint_raise_error(Paint *in_paint, int in_error_code);

CGImageRef _paint_png_data_to_cgimage(Paint *in_paint, void *in_data, long in_size);
CGImageRef _paint_cgimage_clone(CGImageRef in_cgimage, int in_flip);

void _paint_select_rect_area(Paint *in_paint, int in_x, int in_y, int in_width, int in_height);
void _paint_selection_create_with_cgimage(Paint *in_paint, CGImageRef in_cgimage);

CGContextRef _paint_create_context(long pixelsWide, long pixelsHigh, void **out_data, long *out_data_size, int in_flipped);
void _paint_dispose_context(CGContextRef in_context, void *in_data);

void _paint_coord_scale_to_internal(Paint *in_paint, int *io_x, int *io_y);
void _paint_coord_scale_to_external(Paint *in_paint, int *io_x, int *io_y);
CGRect _paint_cgrect_scale_to_external(Paint *in_paint, CGRect in_rect);

CGRect _paint_cgrect_scale_auto(Paint *in_paint, CGRect in_rect);

float _paint_scale_auto(Paint *in_paint, float in_float);
CGPathRef _paint_cgpath_copy_scale_auto(Paint *in_paint, CGPathRef in_path);

CGPoint _paint_cgpoint_scale_to_display(Paint *in_paint, CGPoint in_point);
CGPoint _paint_cgpoint_scale_to_canvas(Paint *in_paint, CGPoint in_point);

CGRect _paint_cgrect_scale_to_display(Paint *in_paint, CGRect in_rect);


void _paint_needs_display(Paint *in_paint);

void _paint_drop_selection(Paint *in_paint);

void _paint_pixel_getf(Paint *in_paint, int in_x, int in_y, float *out_red, float *out_green, float *out_blue, float *out_alpha);
int _paint_colours_samef(Paint *in_paint, float in_red1, float in_green1, float in_blue1, float in_alpha1,
                         float in_red2, float in_green2, float in_blue2, float in_alpha2);


#endif
