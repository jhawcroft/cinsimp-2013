/*
 
 Paint
 paint.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Allocation, setup, destruction, event routing and basic state management
 
 *************************************************************************************************
 
 Graphics Context Configuration
 -------------------------------------------------------------------------------------------------
 The graphics context that is the target of the majority of drawing routines (usually contained
 within other source unit files) is preconfigured prior to calls to those units with certain state
 information:
 -  colour
 -  line size
 -  antialiasing default
 -  blend mode
 
 In addition, the graphics state of the destination context is also generally saved and restored
 post subroutine call.  Other attributes should be customised on a per-tool basis.
 
 
 Scaling and Fat Bits
 -------------------------------------------------------------------------------------------------
 The implementation of a 'Fat Bits' mode presents challenges for the drawing routines.  The scale
 code unit provides a variety of convenience functions that often automatically apply the 
 appropriate scaling factor (if any) to a shape or path before drawing.  See the internal header
 for more details.
 
 */

#include "paint_int.h"


/***********
Prototypes
 */

/* _select.m: */
void _paint_selection_idle(Paint *in_paint);

/* paint.m: */
void _paint_canvas_changed(Paint *in_paint);


/**********
 Paint Internals
 */

/*
 *  _paint_handle_timer
 *  ---------------------------------------------------------------------------------------------
 *  Callback handler for the in_paint->timer;
 *  Updates the selection 'marching ants'.
 */

static void _paint_handle_timer(CFRunLoopTimerRef timer, Paint *in_paint)
{
    assert(timer != NULL);
    assert(in_paint != NULL);
    
    if ((in_paint->current_tool == PAINT_TOOL_LASSO) || (in_paint->current_tool == PAINT_TOOL_SELECT))
        _paint_selection_idle(in_paint);
}


/*
 *  _paint_needs_display
 *  ---------------------------------------------------------------------------------------------
 *  Causes the entire application view to refresh
 *
 *  Invoked frequently by the paint sub-system to display the effects of drawing/stroking 
 *  operations upon the canvas.
 */

void _paint_needs_display(Paint *in_paint)
{
    assert(in_paint != NULL);
    in_paint->callbacks.display_handler(in_paint, in_paint->callback_context, -1, -1, -1, -1);
}


/*
 *  _paint_state_dependent_tools_recompute
 *  ---------------------------------------------------------------------------------------------
 *  Certain tools rely on processing/memory intensive precomputations that are dependent upon
 *  the state of the Paint sub-system, ie. colour and current tool, etc.
 *
 *  This function recomputes this information for the specific tool when such changes occur.
 *
 *  It is invoked by: _paint_colour_changed()
 */

void _paint_state_dependent_tools_recompute(Paint *in_paint)
{
    assert(in_paint != NULL);
    switch (in_paint->current_tool)
    {
        case PAINT_TOOL_BRUSH:
        {
            /* _brush.m: */
            void _paint_brush_recompute(Paint *in_paint);
            
            _paint_brush_recompute(in_paint);
            break;
        }
        case PAINT_TOOL_SPRAY:
        {
            /* _spray.m: */
            void _paint_spray_recompute(Paint *in_paint);
            
            _paint_spray_recompute(in_paint);
            break;
        }
    }
}


/*
 *  _paint_raise_error
 *  ---------------------------------------------------------------------------------------------
 *  Raises an error with the enclosing application view.
 *
 *  The Paint object remains in a consistent state, but is no longer guaranteed to respond to 
 *  user commands.
 */

void _paint_raise_error(Paint *in_paint, int in_error_code)
{
    assert(in_paint != NULL);
    
    /* don't report more than one error */
    if (in_paint->error_reported) return;
    in_paint->error_reported = PAINT_TRUE;
    
    /* report the error to the application view */
    in_paint->callbacks.error_handler(in_paint, in_paint->callback_context, in_error_code, NULL);
}


/*
 *  _paint_init
 *  ---------------------------------------------------------------------------------------------
 *  Initalize the instance of the paint sub-system ready for painting/drawing.
 *
 *  Returns TRUE if successful, or FALSE if it failed.
 *
 *  Upon failure, the memory allocated for the Paint should be immediately deallocated and assumed
 *  uninitalized.
 */

static int _paint_init(Paint *in_paint)
{
    assert(in_paint != NULL);
    assert(in_paint->context_primary == NULL);
    assert(in_paint->bitmap_data_primary == NULL);
    
    /* create an offscreen bitmap and context for painting/drawing */
    in_paint->context_primary = _paint_create_context(in_paint->width,
                                                      in_paint->height,
                                                      (void**)&(in_paint->bitmap_data_primary),
                                                      &(in_paint->bitmap_data_primary_size), PAINT_FALSE);
    if (in_paint->context_primary == NULL) return FALSE;
    CGContextClearRect(in_paint->context_primary, CGRectMake(0, 0, in_paint->width, in_paint->height));
    
    assert(in_paint->bitmap_data_primary != NULL);
    assert(in_paint->bitmap_data_primary_size > 0);
    
    in_paint->bitmap_data_primary_bytes_per_row = CGBitmapContextGetBytesPerRow(in_paint->context_primary);
    
    /* configure the offscreen drawing context */
    CGContextSetBlendMode(in_paint->context_primary, kCGBlendModeNormal);
    CGContextSetAllowsAntialiasing(in_paint->context_primary, TRUE);
    
    /* create a timer to drive the 'marching ants' of the selection boundary */
    CFRunLoopTimerContext timer_context = {0, in_paint, NULL, NULL, NULL};
    in_paint->timer = CFRunLoopTimerCreate(NULL,
                                           0,
                                           _MARCHING_ANTS_INTERVAL,
                                           0,
                                           0,
                                           (CFRunLoopTimerCallBack)_paint_handle_timer,
                                           &timer_context);
    if (in_paint->timer == NULL)
    {
        _paint_dispose_context(in_paint->context_primary, in_paint->bitmap_data_primary);
        return FALSE;
    }
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), in_paint->timer, kCFRunLoopCommonModes);
    
    /* configure some default starting values */
    in_paint->current_tool = PAINT_TOOL_PENCIL;
    in_paint->ants_phase = 4;
    in_paint->scale = 1.0;
    in_paint->round_rect_radius = 10;
    in_paint->red = 0;
    in_paint->green = 0;
    in_paint->blue = 0;
    in_paint->line_size = 1;
    in_paint->colour = CGColorCreateGenericRGB(in_paint->red, in_paint->green, in_paint->blue, 1.0);
    if (in_paint->colour == NULL)
    {
        CFRunLoopTimerInvalidate(in_paint->timer);
        CFRelease(in_paint->timer);
        _paint_dispose_context(in_paint->context_primary, in_paint->bitmap_data_primary);
        return FALSE;
    }
    CGContextSetFillColorWithColor(in_paint->context_primary, in_paint->colour);
    CGContextSetStrokeColorWithColor(in_paint->context_primary, in_paint->colour);
    
    /* notify the application view of the initial display configuration */
    _paint_canvas_changed(in_paint);
    
    /* configure anything that is delayed pending tool change */
    _paint_state_dependent_tools_recompute(in_paint);
    
    return TRUE;
}



/*
 *  _paint_change_colour
 *  ---------------------------------------------------------------------------------------------
 *  There are various ways in which the current colour can be changed: by the user and by the
 *  Paint sub-system itself (eg. the eyedropper tool.)  This function is called in each of those
 *  instances.
 *
 *  Although the colour is stored as a set of 3 component floats (red, green, blue),
 *  we also cache a CGColor for speed and preset the colour on the primary context.
 */

static void _paint_change_colour(Paint *in_paint, float in_red, float in_green, float in_blue, int in_by_user)
{
    assert(in_paint != NULL);
    assert(IS_BOOL(in_by_user));
    
    /* save the new colour */
    in_paint->red = in_red;
    in_paint->green = in_green;
    in_paint->blue = in_blue;
    
    /* recreate the CG colour */
    if (in_paint->colour != NULL) CGColorRelease(in_paint->colour);
    in_paint->colour = CGColorCreateGenericRGB(in_paint->red, in_paint->green, in_paint->blue, 1.0);
    if (in_paint->colour == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    
    /* set the fill and stroke colours of the primary context */
    CGContextSetFillColorWithColor(in_paint->context_primary, in_paint->colour);
    CGContextSetStrokeColorWithColor(in_paint->context_primary, in_paint->colour);
    
    /* recompute state dependent tool data */
    _paint_state_dependent_tools_recompute(in_paint);
    
    /* notify the application view if the colour wasn't changed by them */
    if (in_by_user) return;
    in_paint->callbacks.colour_handler(in_paint, in_paint->callback_context,
                                       in_paint->red * 255.0, in_paint->green * 255.0, in_paint->blue * 255.0);
}


/*
 *  _paint_change_tool
 *  ---------------------------------------------------------------------------------------------
 *  There are various ways in which the current tool can be changed: by the user and by the
 *  Paint sub-system itself (eg. the pasteboard operations.)  This function is called in each of 
 *  those instances.
 *
 *  Does not refresh the display and makes no changes to the selection (if there is one.)
 */

void _paint_change_tool(Paint *in_paint, int in_tool, int in_by_user)
{
    assert(in_paint != NULL);
    assert(IS_BOOL(in_by_user));
    
    in_paint->current_tool = in_tool;
    _paint_state_dependent_tools_recompute(in_paint);
    
    if (in_by_user == PAINT_FALSE)
        in_paint->callbacks.tool_handler(in_paint, in_paint->callback_context, in_tool);
}




/**********
 Public API
 */


/*
 *  paint_create
 *  ---------------------------------------------------------------------------------------------
 *  Creates an instance of a Paint object; ready to begin editing a specific card/background
 *  graphic layer.
 */

Paint* paint_create(int in_width, int in_height, PaintCallbacks *in_callbacks, void *in_context)
{
    assert((in_width > 0) && (in_width < _MAX_CANVAS_SIZE_PIXELS));
    assert((in_height > 0) && (in_height < _MAX_CANVAS_SIZE_PIXELS));
    assert(in_callbacks);
    assert((in_callbacks->display_handler != NULL) && (in_callbacks->display_change_handler != NULL));
    assert(in_callbacks->colour_handler != NULL);
    assert(in_callbacks->error_handler != NULL);
    assert(in_callbacks->tool_handler != NULL);
    
    /* allocate memory and initalize to NULL/0s */
    Paint *paint = calloc(1, sizeof(struct Paint));
    if (!paint) return NULL;
    
    assert(paint->error_reported == PAINT_FALSE);
    
    /* configure canvas size and callbacks */
    paint->width = in_width;
    paint->height = in_height;
    paint->callbacks = *in_callbacks;
    paint->callback_context = in_context;
    
    /* initalize the Paint instance */
    if (!_paint_init(paint))
    {
        free(paint);
        paint = NULL;
    }
    
    /* return the instance */
    return paint;
}


/*
 *  paint_dispose
 *  ---------------------------------------------------------------------------------------------
 *  Destroys a Paint object.  Releases all memory allocated by it.
 */

void paint_dispose(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    if (in_paint->selection_path) CGPathRelease(in_paint->selection_path);
    if (in_paint->selection_path_moved) CGPathRelease(in_paint->selection_path_moved);
    if (in_paint->scaled_selection) CGPathRelease(in_paint->scaled_selection);
    if (in_paint->context_selection)
        _paint_dispose_context(in_paint->context_selection, in_paint->bitmap_data_selection);
    
    if (in_paint->shape_path) CGPathRelease(in_paint->shape_path);
    
    if (in_paint->colour) CGColorRelease(in_paint->colour);
    
    if (in_paint->spray_head) CGImageRelease(in_paint->spray_head);
    if (in_paint->brush_mask) CGImageRelease(in_paint->brush_mask);
    if (in_paint->brush_computed) CGImageRelease(in_paint->brush_computed);
    
    if (in_paint->temp_export_data) free(in_paint->temp_export_data);
    
    if (in_paint->timer)
    {
        CFRunLoopTimerInvalidate(in_paint->timer);
        CFRelease(in_paint->timer);
    }
    
    if (in_paint->context_primary)
        _paint_dispose_context(in_paint->context_primary, in_paint->bitmap_data_primary);
    
    free(in_paint);
}


int paint_current_tool_get(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    return in_paint->current_tool;
}


void paint_current_tool_set(Paint *in_paint, int in_tool)
{
    assert(in_paint != NULL);
    
    if (in_tool == in_paint->current_tool) return;
    
    _paint_drop_selection(in_paint);
    _paint_change_tool(in_paint, in_tool, PAINT_TRUE);
    _paint_needs_display(in_paint);
}






void _paint_flood_fill(Paint *in_paint, CGPoint in_point);

void _paint_select_begin(Paint *in_paint, int in_loc_x, int in_loc_y);
void _paint_pencil_begin(Paint *in_paint, int in_loc_x, int in_loc_y);
void _paint_brush_begin(Paint *in_paint, int in_x, int in_y);
void _paint_spray_begin(Paint *in_paint, int in_x, int in_y);
void _paint_eraser_begin(Paint *in_paint, int in_x, int in_y);
void _paint_freeshape_begin(Paint *in_paint, int in_x, int in_y);
void _paint_freepoly_begin(Paint *in_paint, int in_x, int in_y);


void paint_event_mouse_down(Paint *in_paint, int in_loc_x, int in_loc_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_loc_x) && IS_COORD(in_loc_y));
    
    
    in_paint->drawing_to_canvas = PAINT_TRUE;
    
    if (in_paint->stroking_poly) return;
    
    in_paint->stroking = TRUE;
    
    
    /* apply scale */
    int loc_x = in_loc_x, loc_y = in_loc_y;
    _paint_coord_scale_to_internal(in_paint, &loc_x, &loc_y);
    
    /* tracking */
    in_paint->last_point = CGPointMake(loc_x, loc_y);
    in_paint->starting_point = in_paint->last_point;
    in_paint->ending_point = in_paint->last_point;

    /* tool appropriate action */
    CGContextSaveGState(in_paint->context_primary);
    switch (in_paint->current_tool)
    {
        case PAINT_TOOL_FREEPOLY:
            _paint_freepoly_begin(in_paint, loc_x, loc_y);
            break;
        case PAINT_TOOL_FREESHAPE:
            _paint_freeshape_begin(in_paint, loc_x, loc_y);
            break;
        case PAINT_TOOL_BUCKET:
            _paint_flood_fill(in_paint, in_paint->last_point);
            _paint_needs_display(in_paint);
            break;
        case PAINT_TOOL_ERASER:
            _paint_eraser_begin(in_paint, loc_x, loc_y);
            break;
        case PAINT_TOOL_EYEDROPPER:
        {
            float red,green,blue,alpha;
            _paint_pixel_getf(in_paint, loc_x, loc_y, &red, &green, &blue, &alpha);
            if (alpha == 0.0)
            {
                red = 1.0;
                green = 1.0;
                blue = 1.0;
            }
            _paint_change_colour(in_paint, red, green, blue, PAINT_FALSE);
            break;
        }
        case PAINT_TOOL_SPRAY:
            _paint_spray_begin(in_paint, loc_x, loc_y);
            break;
        case PAINT_TOOL_BRUSH:
            _paint_brush_begin(in_paint, loc_x, loc_y);
            break;
        case PAINT_TOOL_PENCIL:
            _paint_pencil_begin(in_paint, loc_x, loc_y);
            break;
        case PAINT_TOOL_SELECT:
        case PAINT_TOOL_LASSO:
            _paint_select_begin(in_paint, in_loc_x, in_loc_y);
            break;
    }
    CGContextRestoreGState(in_paint->context_primary);
}



void _paint_select_continue(Paint *in_paint, int in_loc_x, int in_loc_y);
void _paint_pencil_continue(Paint *in_paint, int in_loc_x, int in_loc_y);
void _paint_brush_continue(Paint *in_paint, int in_x, int in_y);
void _paint_spray_continue(Paint *in_paint, int in_x, int in_y);
void _paint_eraser_continue(Paint *in_paint, int in_x, int in_y);
void _paint_freeshape_continue(Paint *in_paint, int in_x, int in_y);


void paint_event_mouse_dragged(Paint *in_paint, int in_loc_x, int in_loc_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_loc_x) && IS_COORD(in_loc_y));
    
    in_paint->drawing_to_canvas = PAINT_TRUE;
    
    
    /* apply scale */
    _paint_coord_scale_to_internal(in_paint, &in_loc_x, &in_loc_y);
    
    in_paint->ending_point = CGPointMake(in_loc_x, in_loc_y);
    
    
    if (in_paint->stroking_poly)
    {
        _paint_needs_display(in_paint);
        return;
    }
    

    CGContextSaveGState(in_paint->context_primary);
    switch (in_paint->current_tool)
    {
        case PAINT_TOOL_FREESHAPE:
            _paint_freeshape_continue(in_paint, in_loc_x, in_loc_y);
            break;
        case PAINT_TOOL_ERASER:
            _paint_eraser_continue(in_paint, in_loc_x, in_loc_y);
            break;
        case PAINT_TOOL_SPRAY:
            _paint_spray_continue(in_paint, in_loc_x, in_loc_y);
            break;
        case PAINT_TOOL_BRUSH:
            _paint_brush_continue(in_paint, in_loc_x, in_loc_y);
            break;
        case PAINT_TOOL_PENCIL:
            _paint_pencil_continue(in_paint, in_loc_x, in_loc_y);
            break;
        case PAINT_TOOL_SELECT:
        case PAINT_TOOL_LASSO:
            _paint_select_continue(in_paint, in_loc_x, in_loc_y);
            break;
    }
    CGContextRestoreGState(in_paint->context_primary);

    
    _paint_needs_display(in_paint);
    
    in_paint->last_point = CGPointMake(in_loc_x, in_loc_y);
}


void paint_event_mouse_moved(Paint *in_paint, int in_loc_x, int in_loc_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_loc_x) && IS_COORD(in_loc_y));
    
    in_paint->drawing_to_canvas = PAINT_TRUE;
    
    if (in_paint->stroking_poly)
    {
        _paint_coord_scale_to_internal(in_paint, &in_loc_x, &in_loc_y);
        in_paint->ending_point = CGPointMake(in_loc_x, in_loc_y);
        _paint_needs_display(in_paint);
    }
}


void paint_current_colour_set(Paint *in_paint, int in_red, int in_green, int in_blue)
{
    assert(in_paint != NULL);
    
    if (_paint_colours_samef(in_paint, in_red/255.0, in_green/255.0, in_blue/255.0, 1.0,
                             in_paint->red, in_paint->green, in_paint->blue, 1.0))
        return;
    
    _paint_change_colour(in_paint, in_red / 255.0, in_green / 255.0, in_blue / 255.0, TRUE);
}



void _paint_select_end(Paint *in_paint, int in_loc_x, int in_loc_y);
void _paint_line_end(Paint *in_paint, int in_x, int in_y);
void _paint_rect_draw(Paint *in_paint, CGContextRef in_context);
void _paint_round_rect_draw(Paint *in_paint, CGContextRef in_context);
void _paint_oval_draw(Paint *in_paint, CGContextRef in_context);
void _paint_freeshape_end(Paint *in_paint, int in_x, int in_y);
void _paint_freepoly_end(Paint *in_paint, int in_x, int in_y);
void _paint_freepoly_continue(Paint *in_paint, int in_x, int in_y);

void paint_event_mouse_up(Paint *in_paint, int in_loc_x, int in_loc_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_loc_x) && IS_COORD(in_loc_y));
    
    in_paint->drawing_to_canvas = PAINT_TRUE;
    
    /* apply scale */
    _paint_coord_scale_to_internal(in_paint, &in_loc_x, &in_loc_y);
    in_paint->ending_point = CGPointMake(in_loc_x, in_loc_y);
    
    CGContextSaveGState(in_paint->context_primary);
    if (in_paint->stroking_poly)
    {
        // need to check if ending point is near shape start here
        // also need to put down another point
        _paint_freepoly_continue(in_paint, in_loc_x, in_loc_y);
    }
    else
    {
        switch (in_paint->current_tool)
        {
            case PAINT_TOOL_FREESHAPE:
                _paint_freeshape_end(in_paint, in_loc_x, in_loc_y);
                break;
            case PAINT_TOOL_OVAL:
                _paint_oval_draw(in_paint, in_paint->context_primary);
                break;
            case PAINT_TOOL_ROUNDED_RECT:
                _paint_round_rect_draw(in_paint, in_paint->context_primary);
                break;
            case PAINT_TOOL_RECTANGLE:
                _paint_rect_draw(in_paint, in_paint->context_primary);
                break;
            case PAINT_TOOL_LINE:
                _paint_line_end(in_paint, in_loc_x, in_loc_y);
                break;
            case PAINT_TOOL_SELECT:
            case PAINT_TOOL_LASSO:
                _paint_select_end(in_paint, in_loc_x, in_loc_y);
                break;
        }
    }
    CGContextRestoreGState(in_paint->context_primary);
    
    in_paint->stroking = FALSE;
    
    _paint_needs_display(in_paint);
}


void paint_event_escape(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    in_paint->drawing_to_canvas = PAINT_TRUE;
    
    if (in_paint->stroking_poly)
    {
        _paint_freepoly_end(in_paint, in_paint->last_point.x, in_paint->last_point.y);
        return;
    }
    
}




void _paint_draw_selection(Paint *in_paint, CGContextRef in_dest);
void _paint_line_drawing(Paint *in_paint, CGContextRef in_dest);
void _paint_freeshape_drawing(Paint *in_paint, CGContextRef in_dest);
void _paint_freepoly_drawing(Paint *in_paint, CGContextRef in_dest);


void paint_draw_into(Paint *in_paint, void *in_context)
{
    assert(in_paint != NULL);
    assert(in_context != NULL);
    
    CGContextRef in_dest = in_context;
    
    
    
    /* draw the current canvas into the context */
    CGContextSetInterpolationQuality(in_dest, kCGInterpolationNone);
    CGContextSetBlendMode(in_dest, kCGBlendModeSourceAtop);
    CGImageRef image = CGBitmapContextCreateImage(in_paint->context_primary);
    CGContextDrawImage(in_dest, CGRectMake(0, 0, in_paint->width * in_paint->scale, in_paint->height * in_paint->scale), image);
    CGImageRelease(image);
    
    /* configure the context based on our state */
    CGContextSetFillColorWithColor(in_dest, in_paint->colour);
    CGContextSetStrokeColorWithColor(in_dest, in_paint->colour);
    CGContextSetLineWidth(in_dest, in_paint->line_size * in_paint->scale);
    in_paint->drawing_to_canvas = PAINT_FALSE;

    CGContextSaveGState(in_dest);
    switch (in_paint->current_tool)
    {
        case PAINT_TOOL_FREEPOLY:
            if (in_paint->stroking_poly)
                _paint_freepoly_drawing(in_paint, in_dest);
            break;
        case PAINT_TOOL_FREESHAPE:
            if (in_paint->stroking)
                _paint_freeshape_drawing(in_paint, in_dest);
            break;
        case PAINT_TOOL_OVAL:
            if (in_paint->stroking)
                _paint_oval_draw(in_paint, in_dest);
            break;
        case PAINT_TOOL_ROUNDED_RECT:
            if (in_paint->stroking)
                _paint_round_rect_draw(in_paint, in_dest);
            break;
        case PAINT_TOOL_RECTANGLE:
            if (in_paint->stroking)
                _paint_rect_draw(in_paint, in_dest);
            break;
        case PAINT_TOOL_LINE:
            _paint_line_drawing(in_paint, in_dest);
            break;
        case PAINT_TOOL_SELECT:
        case PAINT_TOOL_LASSO:
            _paint_draw_selection(in_paint, in_dest);
            break;
    }
    CGContextRestoreGState(in_dest);
    

    
    if (in_paint->scale != 1.0)
    {
        /* draw a pixel grid */
        CGContextSaveGState(in_dest);
        CGContextSetBlendMode(in_dest, kCGBlendModeDifference);
        CGContextSetRGBStrokeColor(in_dest, 1.0, 1.0, 1.0, 0.5);
        CGContextSetLineWidth(in_dest, 0.5);
        
        for (int x = 0; x < in_paint->width; x++)
        {
            CGContextBeginPath(in_dest);
            CGContextMoveToPoint(in_dest, x * in_paint->scale, 0);
            CGContextAddLineToPoint(in_dest, x * in_paint->scale, in_paint->height * in_paint->scale);
            CGContextDrawPath(in_dest, kCGPathStroke);
        }
        for (int y = 0; y < in_paint->height; y++)
        {
            CGContextBeginPath(in_dest);
            CGContextMoveToPoint(in_dest, 0, y * in_paint->scale);
            CGContextAddLineToPoint(in_dest, in_paint->width * in_paint->scale, y * in_paint->scale);
            CGContextDrawPath(in_dest, kCGPathStroke);
        }
        
        CGContextRestoreGState(in_dest);
    }
    
    
}


void paint_display_scroll_tell(Paint *in_paint, int in_x, int in_y, int in_width, int in_height)
{
    //printf("tell y= %d\n", in_y);
    
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y) && IS_COORD(in_width) && IS_COORD(in_height));
    
    
    _paint_coord_scale_to_internal(in_paint, &in_x, &in_y);
    _paint_coord_scale_to_internal(in_paint, &in_width, &in_height);
    
    
    /* this information is what is actually visible of the primary context during fat bits: */
    //printf("Scrolled, new region: %d,%d,%d,%d\n", in_x, in_y, in_width, in_height);
    
    
    
    in_paint->display_scroll_x = in_x;
    
    if (in_y < 0) in_y = 0;
    in_paint->display_scroll_y = in_y;
    
    
    
    if (in_width < in_paint->width)
        in_paint->display_width = in_width;
    else
    {
        in_paint->display_width = in_paint->width;
        
    }
    if (in_height < in_paint->height)
        in_paint->display_height = in_height;
    else
    {
        in_paint->display_height = in_paint->height;
        in_paint->display_scroll_y = 0;
    }
    
    
    
}


void _paint_pixel_getf(Paint *in_paint, int in_x, int in_y, float *out_red, float *out_green, float *out_blue, float *out_alpha)
{
    assert(in_paint != NULL);
    
    long point_index = (in_paint->bitmap_data_primary_bytes_per_row * (in_paint->height - in_y)) + (in_x * 4);
    if ((point_index < 0) || (point_index >= in_paint->bitmap_data_primary_size)) return;
    *out_alpha = in_paint->bitmap_data_primary[point_index] / 255.0;
    *out_red = in_paint->bitmap_data_primary[point_index+1] / 255.0;
    *out_green = in_paint->bitmap_data_primary[point_index+2] / 255.0;
    *out_blue = in_paint->bitmap_data_primary[point_index+3] / 255.0;
}


#define _COLOURS_SAME_THRESHOLD 0.05

int _paint_colours_samef(Paint *in_paint, float in_red1, float in_green1, float in_blue1, float in_alpha1,
                         float in_red2, float in_green2, float in_blue2, float in_alpha2)
{
    assert(in_paint != NULL);
    /*float diff1 = absf(in_red1 - in_red2);
    float diff2 = absf(in_green1 - in_green2);
    float diff3 = absf(in_blue1 - in_blue2);
    float diff4 = absf(in_alpha1 - in_alpha2);*/
    
    return ((fabs(in_red1 - in_red2) < _COLOURS_SAME_THRESHOLD) &&
            (fabs(in_green1 - in_green2) < _COLOURS_SAME_THRESHOLD) &&
            (fabs(in_blue1 - in_blue2) < _COLOURS_SAME_THRESHOLD) &&
            (fabs(in_alpha1 - in_alpha2) < _COLOURS_SAME_THRESHOLD));
            
}


void paint_line_size_set(Paint *in_paint, int in_line_size)
{
    assert(in_paint != NULL);
    
    in_paint->line_size = in_line_size;
    CGContextSetLineWidth(in_paint->context_primary, in_paint->line_size);
}


void paint_draw_filled_set(Paint *in_paint, int in_draw_filled)
{
    assert(in_paint != NULL);
    assert(IS_BOOL(in_draw_filled));
    in_paint->draw_filled = in_draw_filled;
}

int paint_draw_filled_get(Paint *in_paint)
{
    assert(in_paint != NULL);
    return in_paint->draw_filled;
}


void paint_set_white_transparent(Paint *in_paint, int in_transparent)
{
    assert(in_paint != NULL);
    assert(IS_BOOL(in_transparent));
    in_paint->white_is_transparent = in_transparent;
}




