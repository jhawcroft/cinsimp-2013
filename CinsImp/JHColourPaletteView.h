//
//  JHColourPaletteView.h
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 23/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface JHColourPaletteView : NSView
{
    int hover_index;
    
    int blink_menu_counter;
    BOOL blink;
    BOOL last_was_dragged;
    BOOL dont_hover;
    
    int colour_index;
}

@end
