//
//  JHColourPaletteController.h
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 23/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@class JHColourPaletteView;


extern NSString *notifyColourDidChange;


@interface JHColourPaletteController : NSWindowController
{
    IBOutlet NSMenu *colourMenu;
    IBOutlet NSView *colourMenuView;
    BOOL _inited_menu;
    
    IBOutlet NSMenuItem *toggleMenuItem;
    IBOutlet JHColourPaletteView *paletteView;
    
    NSColor *_currentColour;
    
    BOOL _was_visible_se;
}


+ (JHColourPaletteController*)sharedController;

- (IBAction)toggleColours:(id)sender;

- (void)setCurrentColour:(NSColor*)colour;
- (NSColor*)currentColour;


@end
