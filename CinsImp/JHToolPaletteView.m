//
//  JHToolPaletteView.m
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 23/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHToolPaletteView.h"

#import "JHToolPaletteController.h"

#include "tools.h"


@implementation JHToolPaletteView


#define _BUTTON_INDEX_FOR_COLOUR -1
#define _BUTTON_INDEX_FOR_LINE_SIZE -3
#define _BUTTON_INDEX_NONE -4

static int _g_tool_index = -1;
static int _g_draw_filled = 0;


static struct ToolButtonDef
{
    char *tooltip_en;
    int tool_code;
    int normal_col;
    int normal_row;
    int select_col;
    int select_row;
    int hover_col;
    int hover_row;
    int filled_col_offset;
    int post_row_space;
}
 _tool_buttons_defs[] = {
     {"Browse", AUTH_TOOL_BROWSE, 1, 1, 1, 8, 1, 15, 0, 0},
     {"Button", AUTH_TOOL_BUTTON, 2, 1, 2, 8, 2, 15, 0, 0},
     {"Field", AUTH_TOOL_FIELD, 3, 1, 3, 8, 3, 15, 0, 10},
     
     {"Select", PAINT_TOOL_SELECT, 1, 2, 1, 9, 1, 16, 0, 0},
     {"Lasso", PAINT_TOOL_LASSO, 2, 2, 2, 9, 2, 16, 0, 0},
     {"Pencil", PAINT_TOOL_PENCIL, 3, 2, 3, 9, 3, 16, 0, 0},
     
     {"Brush", PAINT_TOOL_BRUSH, 1, 3, 1, 10, 1, 17, 0, 0},
     {"Line", PAINT_TOOL_LINE, 2, 3, 2, 10, 2, 17, 0, 0},
     {"Eraser", PAINT_TOOL_ERASER, 3, 3, 3, 10, 3, 17, 0, 0},
     
     {"Spray", PAINT_TOOL_SPRAY, 1, 4, 1, 11, 1, 18, 0, 0},
     {"Rectangle", PAINT_TOOL_RECTANGLE, 2, 4, 2, 11, 2, 18, 2, 0},
     {"Round Rectangle", PAINT_TOOL_ROUNDED_RECT, 3, 4, 3, 11, 3, 18, 2, 0},
     
     {"Bucket", PAINT_TOOL_BUCKET, 1, 5, 1, 12, 1, 19, 0, 0},
     {"Freeform Shape", PAINT_TOOL_FREESHAPE, 2, 5, 2, 12, 2, 19, 2, 0},
     {"Oval", PAINT_TOOL_OVAL, 3, 5, 3, 12, 3, 19, 2, 0},
     
     {"Eyedropper", PAINT_TOOL_EYEDROPPER, 1, 7, 1, 14, 1, 21, 0, 0},
     /*{"Text", 7, 1, 6, 1, 13, 1, 20, 0, 0},*/
     {"Freeform Polygon", PAINT_TOOL_FREEPOLY, 2, 6, 2, 13, 2, 20, 2, 0},
     /*{"Polygon", 9, 3, 6, 3, 13, 3, 20, 2, 0},*/
     
     
     /*{"Magnifyer", 8, 2, 7, 2, 14, 2, 21, 0, 0},*/
     
     NULL
};


- (void)buildColourMenu
{
    colour_menu = [[NSMenu alloc] init];
    
    NSMenuItem *colour_picker_item = [[NSMenuItem alloc] init];
    
    [colour_picker_item setTitle:@"test"];
    
    [colour_menu addItem:colour_picker_item];
}


- (void)loadPalette
{
    //[self setFrame:NSMakeRect(0, 0, 102, 100)];
    ignore_click_on = -1;
    
    my_tracking_rects = [[NSMutableArray alloc] init];
    my_tracking_btns = [[NSMutableArray alloc] init];
    
    [self buildColourMenu];
    
    blink = NO;
    
    palette_icons = [NSImage imageNamed:@"ToolPaletteIcons"];
    palette_icons_height = [palette_icons size].height;
    
    colour_wells = [NSImage imageNamed:@"ToolColourWells"];
    
    hover_index = _BUTTON_INDEX_NONE;
    
    line_size = 1;
    colour = [NSColor blackColor];
    
    
    struct ToolButtonDef *button_def = _tool_buttons_defs;
    int x = 0, y = 5, index = 0;//, row = 0;
    while (button_def->tooltip_en)
    {
        if (button_def->tooltip_en[0])
        {
            NSDictionary *button_info = [NSDictionary dictionaryWithObject:[NSNumber numberWithInt:index]
                                                                    forKey:@"ButtonIndex"];
            [my_tracking_rects addObject:NSStringFromRect(NSMakeRect(x, y, 34, 21))];
            [my_tracking_btns addObject:button_info];
            
            //[self addToolTipRect:NSMakeRect(x, y, 34, 21) owner:self userData:(void*)CFBridgingRetain(button_info)];
        }
        x += 34;
        if (x == 34 * 3)
        {
            y += 21;
            //if (row == 0)
            y += button_def->post_row_space;
            //row++;
            x = 0;
        }
        index++;
        button_def++;
    }
    
    height = y + 5;
    if (x != 0) height += 21;
    //y = 157;
    
    /*
    NSDictionary *button_info;
    //NSTrackingArea *button_tracking_area;
    
    button_info = [NSDictionary dictionaryWithObject:[NSNumber numberWithInt:_BUTTON_INDEX_FOR_COLOUR]
                                                            forKey:@"ButtonIndex"];
    [my_tracking_rects addObject:NSStringFromRect(NSMakeRect(24, y + 12, 42, 42))];
    [my_tracking_btns addObject:button_info];
    
    [self addToolTipRect:NSMakeRect(24, y + 12, 42, 42) owner:self userData:button_info];
    
    button_info = [NSDictionary dictionaryWithObject:[NSNumber numberWithInt:_BUTTON_INDEX_FOR_LINE_SIZE]
                                                            forKey:@"ButtonIndex"];
    [my_tracking_rects addObject:NSStringFromRect(NSMakeRect(71, y + 12, 17, 17))];
    [my_tracking_btns addObject:button_info];
    
    [self addToolTipRect:NSMakeRect(71, y + 12, 17, 17) owner:self userData:button_info];
    */
        
    [self setFrame:NSMakeRect(0, 0, 34 * 3, height + 1)];

    [self addTrackingArea:[[NSTrackingArea alloc]
                           initWithRect:self.bounds
                           options:NSTrackingMouseEnteredAndExited |
                           NSTrackingActiveInActiveApp |
                           NSTrackingEnabledDuringMouseDrag |
                           NSTrackingMouseMoved
                           owner:self
                           userInfo:nil]];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(notifyToolDidChange:)
                                                 name:notifyToolDidChange
                                               object:nil];


}


- (int)buttonIndexForPoint:(NSPoint)in_point
{
    int i = 0, c = (int)[my_tracking_rects count];
    for (i = 0; i < c; i++)
    {
        NSRect tracking_rect = NSRectFromString([my_tracking_rects objectAtIndex:i]);
        if (NSPointInRect(in_point, tracking_rect))
            return [[[my_tracking_btns objectAtIndex:i] objectForKey:@"ButtonIndex"] intValue];
    }
    return _BUTTON_INDEX_NONE;
}


- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) [self loadPalette];
    return self;
}


- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) [self loadPalette];
    return self;
}

- (void)viewDidMoveToWindow
{
    if (self.window)
    {
        dont_hover = NO;
        
        if (![self enclosingMenuItem])
        {
            [(NSPanel*)[self window] setFloatingPanel:YES];
            [(NSPanel*)[self window] setBecomesKeyOnlyIfNeeded:YES];
            
            NSButton *button;
            button = [[self window] standardWindowButton:NSWindowMiniaturizeButton];
            [button setHidden:YES];
            button = [[self window] standardWindowButton:NSWindowZoomButton];
            [button setHidden:YES];
            
      
            
            
           /* button = [[self window] standardWindowButton:NSWindowCloseButton];
            
            NSArray *children = [[[self.window contentView] superview] subviews];
            for (NSView *subview in children)
            {
                if ((subview != button) && (subview != [self superview]))
                    [subview setFrameOrigin:NSMakePoint(button.frame.size.width, 0)];
            }*/
            
            [[self window] setContentSize:self.frame.size];
            
            
            //NSLog(@"became windowed");
        }
    }
}


- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}


- (BOOL)isFlipped
{
    return YES;
}


- (BOOL)acceptsFirstResponder
{
    return NO;
}


- (void)chooseTool:(int)in_tool_code
{
    struct ToolButtonDef *button_def = _tool_buttons_defs;
    while (button_def->tooltip_en)
    {
        if (button_def->tooltip_en[0])
        {
            if (button_def->tool_code == in_tool_code)
            {
                _g_tool_index = (int)(button_def - _tool_buttons_defs);
                [self setNeedsDisplay:YES];
                return;
            }
        }
        button_def++;
    }
}




- (void)mouseMoved:(NSEvent *)theEvent
{
    last_was_dragged = NO;
    NSPoint loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    int button_index = [self buttonIndexForPoint:loc];
    int last_hover_index = hover_index;
    if (button_index < 0)
    {
        hover_index = button_index;
    }
    else
    {
        struct ToolButtonDef *button_def = &(_tool_buttons_defs[ button_index ]);
        hover_index = (int)(button_def - _tool_buttons_defs);
    }
    if (last_hover_index != hover_index)
        [self setNeedsDisplay:YES];
}


- (void)mouseDragged:(NSEvent *)theEvent
{
    [self mouseMoved:theEvent];
    last_was_dragged = YES;
}


- (void)mouseDown:(NSEvent *)theEvent
{
    NSPoint loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    
    if (hover_index < 0)
    {
        switch (hover_index)
        {
            case _BUTTON_INDEX_FOR_COLOUR:
                NSLog(@"mouse down over COLOUR\n");
                [colour_menu popUpMenuPositioningItem:nil atLocation:loc inView:self];
                break;
            case _BUTTON_INDEX_FOR_LINE_SIZE:
                NSLog(@"mouse down over LINE SIZE\n");
                break;
        }
    }
    else
    {
        _g_tool_index = hover_index;
        [self setNeedsDisplay:YES];
        struct ToolButtonDef *button_def = &(_tool_buttons_defs[ _g_tool_index ]);
        [[JHToolPaletteController sharedController] setCurrentTool:button_def->tool_code];
        //[_delegate setCurrentTool:button_def->tool_code];
    }
}


- (void)viewDidMoveToSuperview
{
    
}



- (void)sendMenuAction {
    NSMenuItem *actualMenuItem = [self enclosingMenuItem];
    
    // Send the action set on the actualMenuItem to the target set on the actualMenuItem, and make come from the actualMenuItem.
    [NSApp sendAction:[actualMenuItem action] to:[actualMenuItem target] from:actualMenuItem];
    
    // dismiss the menu being tracked
    NSMenu *menu = [actualMenuItem menu];
    [menu cancelTracking];
    
    [self setNeedsDisplay:YES];
}


- (void)blinkMenuSelection:(NSTimer*)in_timer
{
    blink_menu_counter++;
    blink = !blink;
    [self setNeedsDisplay:YES];
    
    if (blink_menu_counter >= (last_was_dragged ? 5 : 4))
    {
        blink = NO;
        dont_hover = NO;
        [in_timer invalidate];
        [self sendMenuAction];
    }
}


- (void)mouseUp:(NSEvent *)theEvent
{
    if (hover_index < 0) return;
    
    if ([self enclosingMenuItem])
    {
        if (hover_index >= 0)
        {
            _g_tool_index = hover_index;
            struct ToolButtonDef *button_def = &(_tool_buttons_defs[ _g_tool_index ]);
            [[JHToolPaletteController sharedController] setCurrentTool:button_def->tool_code];
            //[_delegate setCurrentTool:button_def->tool_code];
        }
        
        blink_menu_counter = 0;
        dont_hover = YES;
        NSTimer *blink_timer = [NSTimer timerWithTimeInterval:0.08
                                                       target:self
                                                     selector:@selector(blinkMenuSelection:)
                                                     userInfo:nil
                                                      repeats:YES];
        [[NSRunLoop currentRunLoop] addTimer:blink_timer forMode:NSEventTrackingRunLoopMode];

        return;
    }
    
    if ([theEvent clickCount] > 1)
    {
        //NSLog(@"click count > 1");
        /*if (hover_index == ignore_click_on)
        {
            ignore_click_on = -1;
            return;
        }
        ignore_click_on = hover_index;*/
        
        /* sometimes seem to be randomly getting two up events in rapid succession;
         so we will time and make sure we only produce one event */
        //static double last_double = 0;
        //if (time(NULL) - last_double < 2) return;
        
        //last_double = time(NULL);
        //NSLog(@"mouse up-DOUBLE CLICK!");
        [[JHToolPaletteController sharedController] toolAuxMode];
        //[_delegate toolAuxMode];
    }
}



- (NSString *)view:(NSView *)view stringForToolTip:(NSToolTipTag)tag point:(NSPoint)point userData:(NSDictionary*)button_info
{
    int button_index = [[button_info objectForKey:@"ButtonIndex"] intValue];
    if (button_index < 0)
    {
        switch (button_index)
        {
            case _BUTTON_INDEX_FOR_COLOUR: return @"Colour";
            case _BUTTON_INDEX_FOR_LINE_SIZE: return @"Line Size";
        }
    }
    else
    {
        struct ToolButtonDef *button_def = &(_tool_buttons_defs[ button_index ]);
        return [NSString stringWithCString:button_def->tooltip_en encoding:NSUTF8StringEncoding];
    }
    return NULL;
}

- (void)mouseEntered:(NSEvent *)theEvent
{
    NSDictionary *button_info = [[theEvent trackingArea] userInfo];
    
    int button_index = [[button_info objectForKey:@"ButtonIndex"] intValue];
    if (!button_info) button_index = -1;
    if (button_index < 0)
    {
        hover_index = button_index;
    }
    else
    {
        struct ToolButtonDef *button_def = &(_tool_buttons_defs[ button_index ]);
        hover_index = (int)(button_def - _tool_buttons_defs);
        [self setNeedsDisplayInRect:[[theEvent trackingArea] rect]];
    }
}


- (void)mouseExited:(NSEvent *)theEvent
{
    hover_index = _BUTTON_INDEX_NONE;
    [self setNeedsDisplay:YES];
}


- (BOOL)isOpaque
{
    return NO;
}




- (void)drawRect:(NSRect)dirtyRect
{
    //[[NSColor colorWithDeviceRed:0.8 green:0.8 blue:0.8 alpha:1.0] setFill];
    //NSRectFill(self.bounds);
    if ([self enclosingMenuItem])
    {
    NSPoint loc = [self convertPoint:[[self window] mouseLocationOutsideOfEventStream] fromView:nil];
    if ((!NSPointInRect(loc, self.frame)) && (hover_index >= 0))
        hover_index = -1;
    }
    
    float alpha = [[self window] alphaValue];
    
    struct ToolButtonDef *button_def = _tool_buttons_defs;
    int x = 0, y = 5;//, row = 0;
    while (button_def->tooltip_en)
    {
        if (button_def->tooltip_en[0])
        {
            BOOL current = (button_def - _tool_buttons_defs == _g_tool_index);
            BOOL hover = ((button_def - _tool_buttons_defs == hover_index) && (!dont_hover));
            
            if (blink) current = NO;
            
            int filled_offset = ( ((button_def->filled_col_offset != 0) && _g_draw_filled) ?
                                 button_def->filled_col_offset : 0 );
            
            if (current)
                [palette_icons drawInRect:NSMakeRect(x, y, 34, 21)
                                 fromRect:NSMakeRect((button_def->select_col + filled_offset) * 34 - 34,
                                                     palette_icons_height - (button_def->select_row * 21),
                                                     34,
                                                     21)
                                operation:NSCompositeSourceOver
                                 fraction:alpha
                           respectFlipped:YES
                                    hints:nil];
            else if (hover)
                [palette_icons drawInRect:NSMakeRect(x, y, 34, 21)
                                 fromRect:NSMakeRect((button_def->hover_col + filled_offset) * 34 - 34,
                                                     palette_icons_height - (button_def->hover_row * 21),
                                                     34,
                                                     21)
                                operation:NSCompositeSourceOver
                                 fraction:alpha
                           respectFlipped:YES
                                    hints:nil];
            else
                [palette_icons drawInRect:NSMakeRect(x, y, 34, 21)
                                 fromRect:NSMakeRect((button_def->normal_col + filled_offset) * 34 - 34,
                                                     palette_icons_height - (button_def->normal_row * 21),
                                                     34,
                                                     21)
                                operation:NSCompositeSourceOver
                                 fraction:alpha
                           respectFlipped:YES
                                    hints:nil];
        }
        x += 34;
        if (x == 34 * 3)
        {
            y += 21;
            //if (row == 0)
            y += button_def->post_row_space;
            //row++;
            x = 0;
        }
        button_def++;
    }
    /*
    y = 157;//157
    
    [colour setFill];
    NSRectFill(NSMakeRect(24, y + 12, 42, 42));
    
    [[NSColor blackColor] setFill];
    NSRectFill(NSMakeRect(71, y + 20 - (line_size / 2) + ((line_size % 2 == 0) ? 1: 0), 17, line_size));
   
    [colour_wells drawInRect:NSMakeRect(0, y + 5, colour_wells.size.width, colour_wells.size.height) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];*/
}


- (void)notifyToolDidChange:(NSNotification*)notification
{
    [self chooseTool:[[JHToolPaletteController sharedController] currentTool]];
}


- (void)setFilledShapes:(BOOL)filledShapes
{
    _g_draw_filled = filledShapes;
    [self setNeedsDisplay:YES];
}



@end
