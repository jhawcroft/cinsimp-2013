//
//  JHCardView+Paint.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView.h"


#import "JHColourPaletteController.h"
#import "JHToolPaletteController.h"
#import "JHLineSizePaletteController.h"


@implementation JHCardView (Paint)


/************
 Paint Sub-system Integration
 */

static void _handle_paint_display(Paint *in_paint, JHCardView *in_context, int in_x, int in_y, int in_width, int in_height)
{
    //NSLog(@"needs display");
    //[in_context setNeedsDisplay:YES];
    [in_context setNeedsDisplayInRect:in_context.bounds];
}


static void _handle_canvas_change(Paint *in_paint, JHCardView *in_context, int in_width, int in_height, int in_scroll_x, int in_scroll_y)
{
    [in_context setFrameSize:NSMakeSize(in_width, in_height)];
    NSRect scroller_bounds = [[[in_context superview] superview] bounds];
    [in_context scrollPoint:NSMakePoint(in_scroll_x - (scroller_bounds.size.width / 2), in_scroll_y - (scroller_bounds.size.height / 2))];
}


static void _handle_colour_change(Paint *in_paint, JHCardView *in_context, int in_red, int in_green, int in_blue)
{
    [[JHColourPaletteController sharedController] setCurrentColour:[NSColor colorWithDeviceRed:in_red/255.0
                                                                                         green:in_green/255.0
                                                                                          blue:in_blue/255.0 alpha:1.0]];
}


static void _handle_tool_change(Paint *in_paint, JHCardView *in_context, int in_tool)
{
    [[JHToolPaletteController sharedController] setCurrentTool:in_tool];
}


static void _handle_error(Paint *in_paint, JHCardView *in_context, int in_error_code, char const *in_error_message)
{
    NSLog(@"PAINT SUBSYSTEM REPORTS ERROR. %d\n", in_error_code);
}


static struct PaintCallbacks _paint_callbacks =
{
    (PaintDisplayHandler)&_handle_paint_display,
    (PaintDisplayChangeHandler)&_handle_canvas_change,
    (PaintColourChangeHandler)&_handle_colour_change,
    (PaintToolChangeHandler) &_handle_tool_change,
    (PaintErrorHandler) &_handle_error,
};



- (void)_exitPaintSession
{
    if (!_paint_subsys) return;
    
    void const *layer_data;
    long layer_data_size;
    layer_data_size = paint_canvas_get_png_data(_paint_subsys, PAINT_TRUE, &layer_data);
    if (!edit_bkgnd)
        stack_layer_picture_set(stack, stackmgr_current_card_id(stack), STACK_NO_OBJECT, (void*)layer_data, layer_data_size);
    else
        stack_layer_picture_set(stack, STACK_NO_OBJECT, stackmgr_current_bkgnd_id(stack), (void*)layer_data, layer_data_size);
    
    _paint_predraw_cache = nil;
    _paint_postdraw_cache = nil;
    if (_paint_subsys)
        paint_dispose(_paint_subsys);
    _paint_subsys = NULL;
}


// we probably want these two to take care of mode changes too**TODO**


- (void)_openPaintSession
{
    if (_paint_subsys) return;
    
    /* setup pre and post-draw caches */
    if (!edit_bkgnd)
    {
        _paint_predraw_cache = [self _pictureOfBkgndObjectsAndArt];
        _paint_postdraw_cache = [self _pictureOfCardObjects];
    }
    else
    {
        _paint_predraw_cache = nil;
        _paint_postdraw_cache = [self _pictureOfBkgndObjects];
    }
    
    /* create the Paint sub-system instance */
    _paint_subsys = paint_create(self.bounds.size.width, self.bounds.size.height, &_paint_callbacks, (__bridge void *)(self));
    paint_set_white_transparent(_paint_subsys, edit_bkgnd);
    
    /* load brush shape */
    NSImage *brush_shape = [NSImage imageNamed:@"brush_mask_1"];
    NSData *temp = [brush_shape TIFFRepresentation];
    paint_brush_shape_set(_paint_subsys, (void*)[temp bytes], [temp length]);
    
    /* configure sub-system with current global configuration */
    paint_current_tool_set(_paint_subsys, currentTool);
    paint_draw_filled_set(_paint_subsys, [[JHToolPaletteController sharedController] drawFilled]);
    NSColor *colour = [[JHColourPaletteController sharedController] currentColour];
    paint_current_colour_set(_paint_subsys,
                             colour.redComponent * 255.0,
                             colour.greenComponent * 255.0,
                             colour.blueComponent * 255.0);
    paint_line_size_set(_paint_subsys, [[JHLineSizePaletteController sharedController] currentSize]);
    
    /* load appropriate layer artwork into sub-system */
    void *layer_data;
    long layer_data_size;
    int visible;
    if (!edit_bkgnd)
        layer_data_size = stack_layer_picture_get(stack, stackmgr_current_card_id(stack), STACK_NO_OBJECT, &layer_data, &visible);
    else
        layer_data_size = stack_layer_picture_get(stack, STACK_NO_OBJECT, stackmgr_current_bkgnd_id(stack), &layer_data, &visible);
    if (layer_data_size != 0)
        paint_canvas_set_png_data(_paint_subsys, layer_data, layer_data_size);
    
    /* configure sub-system for display size and scroll */
    [self _tellPaintAboutDisplay];
}


- (void)_tellPaintAboutDisplay
{
    if (!_paint_subsys) return;
    
    NSScrollView *scroller = (NSScrollView*)[[self superview] superview];
    NSRect scroll_rect = [scroller documentVisibleRect];
    
    paint_display_scroll_tell(_paint_subsys,
                              scroll_rect.origin.x,
                              scroll_rect.origin.y,
                              scroll_rect.size.width,
                              scroll_rect.size.height);
}



/* invoked internally at the head of a number of user actions;
 exits the current paint session silently (without UI changes).
 designed to be paired with _resumePaint (below) */
- (void)_suspendPaint
{
    if (_paint_suspended) return;
    if (_edit_mode != CARDVIEW_MODE_PAINT)
    {
        _paint_suspended = NO;
        return;
    }
    
    /* edit any open paint session */
    [self _exitPaintSession];
    _edit_mode = CARDVIEW_MODE_BROWSE; /* quietly exit paint mode without changing menus */
    _paint_suspended = YES;
}


/* invoked internally at the foot of a number of user actions;
 enters a new paint session silently (without UI changes),
 but only where a session was previously suspended using
 _suspendPaint (above). */
- (void)_resumePaint
{
    if (!_paint_suspended) return;
    _paint_suspended = NO;
    
    [self _openPaintSession];
    [self chooseTool:currentTool]; /* re-enter paint mode; reset tool, rebuild layout, etc. */
}



@end
