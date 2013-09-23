//
//  JHTearOffMenuView.h
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 23/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface JHTearOffMenuView : NSView
{
    NSTimer *_timer;
    BOOL _in_menu;
    BOOL track_palette;
    
    IBOutlet NSPanel *palette;
    IBOutlet NSView *theView;
    IBOutlet NSWindow *ghostPalette;

}

@end
