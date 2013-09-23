//
//  JHColourPaletteView.m
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 23/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHColourPaletteView.h"
#import "JHColourPaletteController.h"


struct ColourDef
{
    int red;
    int green;
    int blue;
    
} _g_colour_table[] = {
    {0,0,0},
    {41,41,41},
    {84,84,84},
    {128,128,128},
    {168,168,168},
    {212,212,212},
    
    {51,0,0},
    {102,0,0},
    {153,0,0},
    {204,0,0},
    {255,0,0},
    {255,255,255},
    
    {102,51,51},
    {153,51,51},
    {204,51,51},
    {255,51,51},
    {153,102,102},
    {204,153,153},
    
    {102,51,0},
    {153,51,0},
    {204,51,0},
    {255,102,102},
    {255,153,153},
    {255,204,204},
    
    {255,51,0},
    {204,153,51},
    {255,153,51},
    {255,153,102},
    {255,204,153},
    {255,255,204},
    
    {153,102,0},
    {153,102,51},
    {204,102,51},
    {255,102,51},
    {204,153,102},
    {255,255,153},
    
    {204,102,0},
    {255,102,0},
    {255,153,0},
    {255,204,0},
    {255,204,51},
    {255,204,102},
    
    {51,51,0},
    {51,102,0},
    {51,153,0},
    {51,204,0},
    {102,153,102},
    {102,204,102},
    
    {0,51,0},
    {0,102,0},
    {0,153,0},
    {51,102,51},
    {51,153,51},
    {153,255,153},
    
    {0,51,51},
    {51,102,102},
    {51,153,102},
    {102,204,153},
    {51,204,51},
    {51,255,51},
    
    {0,102,102},
    {0,153,153},
    {51,153,153},
    {102,255,153},
    {153,204,153},
    {204,255,204},
    
    {0,0,255},
    {102,102,255},
    {0,204,204},
    {51,204,204},
    {102,204,204},
    {153,204,204},
    
    {0,0,51},
    {0,0,102},
    {0,0,153},
    {0,0,204},
    {102,204,255},
    {153,204,255},
    
    {51,51,102},
    {51,51,153},
    {51,51,204},
    {51,51,255},
    {102,102,153},
    {102,102,204},
    
    {51,0,51},
    {51,0,102},
    {102,0,153},
    {102,0,204},
    {102,0,255},
    {204,153,204},
    
    {102,0,51},
    {153,0,51},
    {102,51,153},
    {102,51,204},
    {153,51,255},
    {255,204,255},
    
    {102,51,102},
    {153,102,153},
    {204,102,204},
    {153,102,255},
    {204,153,204},
    {255,153,255},
    
    {153,51,102},
    {204,0,102},
    {255,0,153},
    {255,51,153},
    {255,102,153},
    {255,153,204},
    
    -1
};


@implementation JHColourPaletteView


- (void)loadPalette
{
    [self setFrame:NSMakeRect(0, 0, 102, 306)];
    
    colour_index = 0;
    hover_index = -1;
    blink = NO;
    
    struct ColourDef *colour_def = _g_colour_table;
    while (colour_def->red >= 0)
    {
        
        
        colour_def++;
    }
    
    [self addTrackingArea:[[NSTrackingArea alloc]
                           initWithRect:self.bounds
                           options:NSTrackingMouseEnteredAndExited |
                           NSTrackingActiveInActiveApp |
                           NSTrackingEnabledDuringMouseDrag |
                           NSTrackingMouseMoved
                           owner:self
                           userInfo:nil]];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(notifyColourDidChange:)
                                                 name:notifyColourDidChange
                                               object:nil];
}


- (void)mouseExited:(NSEvent *)theEvent
{
    hover_index = -1;
    [self setNeedsDisplay:YES];
}


- (void)viewDidMoveToWindow
{
    if (self.window)
    {
        hover_index = -1;
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
            
            [[self window] setContentSize:self.frame.size];
        }
    }
}


- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) [self loadPalette];
    return self;
}


- (BOOL)isFlipped
{
    return YES;
}


- (int)swatchIndexForPoint:(NSPoint)in_point
{
    int col, row;
    if (!NSPointInRect(in_point, self.bounds)) return -1;
    col = in_point.x / 17;
    row = in_point.y / 17;
    return 6 * row + col;
}


- (void)mouseMoved:(NSEvent *)theEvent
{
    last_was_dragged = NO;
    NSPoint loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    int swatch_index = [self swatchIndexForPoint:loc];
    int last_hover_index = hover_index;
    hover_index = swatch_index;
    if (last_hover_index != hover_index)
        [self setNeedsDisplay:YES];
}


- (void)mouseDragged:(NSEvent *)theEvent
{
    [self mouseMoved:theEvent];
    last_was_dragged = YES;
}


- (NSColor*)currentColour
{
    if (colour_index < 0) return nil;
    
    struct ColourDef *colour_def = _g_colour_table + colour_index;
    return [NSColor colorWithDeviceRed:colour_def->red / 255.0
                                  green:colour_def->green / 255.0
                                   blue:colour_def->blue / 255.0
                                  alpha:1.0];
}


- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}


- (BOOL)acceptsFirstResponder
{
    return NO;
}


- (void)mouseDown:(NSEvent *)theEvent
{
    if (hover_index < 0) return;

    colour_index = hover_index;
    [self setNeedsDisplay:YES];
    //NSLog(@"setting colour_index= %d", colour_index);
    
    [[JHColourPaletteController sharedController] setCurrentColour:[self currentColour]];
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
            colour_index = hover_index;
            [[JHColourPaletteController sharedController] setCurrentColour:[self currentColour]];
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
}


- (void)drawRect:(NSRect)dirtyRect
{
    float alpha = [[self window] alphaValue];
    
    NSColor *black = [NSColor colorWithDeviceRed:0 green:0 blue:0 alpha:alpha];
    NSColor *white = [NSColor colorWithDeviceRed:1 green:1 blue:1 alpha:alpha];
    
    struct ColourDef *colour_def = _g_colour_table;
    int col = 0, row = 0;
    while (colour_def->red >= 0)
    {
        
        [[NSColor colorWithDeviceRed:colour_def->red / 255.0
                              green:colour_def->green / 255.0
                               blue:colour_def->blue / 255.0
                              alpha:alpha] setFill];
        
        NSRectFill(NSMakeRect(col * 17, row * 17, 17, 17));
        
        
        if ((hover_index == colour_def - _g_colour_table) && (!dont_hover))
        {
            [white setFill];
            NSFrameRectWithWidth(NSMakeRect(col * 17, row * 17, 17, 17), 2);
        }
        
        col++;
        if (col == 6)
        {
            col = 0;
            row++;
        }
        
        colour_def++;
    }
    
    if ((colour_index != -1) && (!blink))
    {
        row = colour_index / 6;
        col = colour_index - (row * 6);
        
        [white setFill];
        NSFrameRectWithWidth(NSMakeRect(col * 17 - 2, row * 17 - 2, 17 + 4, 17 + 4), 3);
        [black setFill];
        NSFrameRectWithWidth(NSMakeRect(col * 17 - 3, row * 17 - 3, 17 + 6, 17 + 6), 1);
        [black setFill];
        NSFrameRectWithWidth(NSMakeRect(col * 17 + 1, row * 17 + 1, 17 - 2, 17 - 2), 1);
    }
    
}


- (void)notifyColourDidChange:(NSNotification*)notification
{
    /* find the current colour, if we can, in this palette */
    NSColor *current_colour = [[JHColourPaletteController sharedController] currentColour];
    if (!current_colour) return;
    
    int red = [current_colour redComponent] * 255;
    int green = [current_colour greenComponent] * 255;
    int blue = [current_colour blueComponent] * 255;
    
    int new_color_index = colour_index;
    
    struct ColourDef *colour_def = _g_colour_table;
    while (colour_def->red >= 0)
    {
        if ((abs(colour_def->red - red) < 2) &&
            (abs(colour_def->green - green) < 2) &&
            (abs(colour_def->blue - blue) < 2))
        {
            new_color_index = (int)(colour_def - _g_colour_table);
            break;
        }
        
        colour_def++;
    }
    
    if (new_color_index != colour_index)
    {
        colour_index = new_color_index;
        [self setNeedsDisplay:YES];
    }
}


@end
