//
//  JHToolPaletteView.h
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 23/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>



@interface JHToolPaletteView : NSView
{
    NSImage *palette_icons;
    int palette_icons_height;
    
    NSImage *colour_wells;
    
    int hover_index;
    
    int line_size;
    NSColor *colour;
    
    NSMutableArray *my_tracking_rects;
    NSMutableArray *my_tracking_btns;
    
    int blink_menu_counter;
    BOOL blink;
    BOOL last_was_dragged;
    BOOL dont_hover;
    
    NSMenu *colour_menu;
    
    int height;
    
    int ignore_click_on;
}



- (void)chooseTool:(int)in_tool_code;

- (void)setFilledShapes:(BOOL)filledShapes;


@end
