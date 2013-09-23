//
//  JHToolPaletteController.h
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 23/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "tools.h"


extern NSString *notifyToolDidChange;
extern NSString *notifyToolDoubleClicked;


@class JHToolPaletteView;


@interface JHToolPaletteController : NSWindowController
{
    IBOutlet NSMenu *toolMenu;
    IBOutlet NSView *toolMenuView;
    
    IBOutlet NSMenuItem *toggleMenuItem;
    IBOutlet JHToolPaletteView *paletteView;
    
    int _currentTool;
    BOOL _drawFilled;
    
    BOOL _was_visible_se;
}

+ (JHToolPaletteController*)sharedController;

- (IBAction)toggleTools:(id)sender;

- (void)setCurrentTool:(int)toolCode;
- (void)toolAuxMode;

- (int)currentTool;

- (void)setDrawFilled:(BOOL)inDrawFilled;
- (BOOL)drawFilled;


@end
