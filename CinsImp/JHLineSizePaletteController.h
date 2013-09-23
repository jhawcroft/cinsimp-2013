//
//  JHLineSizePaletteController.h
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 24/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@class JHLineSizeView;


extern NSString *notifyLineSizeDidChange;


@interface JHLineSizePaletteController : NSWindowController <NSWindowDelegate>
{
    IBOutlet NSMenuItem *toggleMenuItem;
    IBOutlet JHLineSizeView *paletteView;
    
    NSRect _show_saved_frame;
    BOOL _show_invoked;
    BOOL _show_saved_visible;
    
    int line_size;
    
    BOOL _was_visible_se;
}

+ (JHLineSizePaletteController*)sharedController;

- (void)setCurrentSize:(int)size;
- (int)currentSize;


@end
