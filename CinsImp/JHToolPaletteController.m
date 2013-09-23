//
//  JHToolPaletteController.m
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 23/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHToolPaletteController.h"
#import "JHToolPaletteView.h"
#import "JHAppScriptEditorNotifications.h"


NSString *notifyToolDidChange = @"notifyToolDidChange";
NSString *notifyToolDoubleClicked = @"notifyToolDoubleClicked";


@implementation JHToolPaletteController


static JHToolPaletteController *_g_shared_instance = NULL;


+ (JHToolPaletteController*)sharedController
{
    if (_g_shared_instance) return _g_shared_instance;
    _g_shared_instance = [[JHToolPaletteController alloc] initWithWindowNibName:@"JHToolPalette"];
    return _g_shared_instance;
}


- (id)initWithWindow:(NSWindow *)window
{
    if (_g_shared_instance) return _g_shared_instance;
    /*{
        [NSException raise:@"JHToolPaletteController shared instance already instantiated!" format:@""];
        return nil;
    }*/
    
    self = [super initWithWindow:window];
    if (self)
    {
        if (!window)
            [[NSBundle mainBundle] loadNibNamed:@"JHToolPalette" owner:self topLevelObjects:nil];
        
        _g_shared_instance = self;
        [self setup];
    }
    return self;
}


- (void)scriptEditorBecameActive:(NSNotification*)notification
{
    _was_visible_se = [[self window] isVisible];
    [[self window] orderOut:self];
}


- (void)scriptEditorBecameInactive:(NSNotification*)notification
{
    if (_was_visible_se)
    {
        [[self window] orderFront:self];
    }
}


- (void)setup
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(scriptEditorBecameActive:)
                                                 name:scriptEditorBecameActive
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(scriptEditorBecameInactive:)
                                                 name:scriptEditorBecameInactive
                                               object:nil];
}


- (void)awakeFromNib
{
    if ((toolMenu) && (toolMenuView))
    {
        NSMenuItem *toolMenuItem = [[NSMenuItem alloc] init];
        [toolMenuItem setView:toolMenuView];
        [toolMenu addItem:toolMenuItem];
    }
}


- (IBAction)toggleTools:(id)sender
{
    if (self.window.isVisible)
        [self.window orderOut:self];
    else
        [self.window orderFront:self];
}


- (void)setCurrentTool:(int)toolCode
{
    _currentTool = toolCode;
    
    //[paletteView setCurrentTool:toolCode];
    //[paletteView setNeedsDisplay:YES];
    
    
    [[NSNotificationCenter defaultCenter] postNotificationName:notifyToolDidChange object:self];
}


- (int)currentTool
{
    return _currentTool;
}


/* occurs when user double-clicks on a tool */
- (void)toolAuxMode
{
     [[NSNotificationCenter defaultCenter] postNotificationName:notifyToolDoubleClicked object:self];
}


/* might need a similar arrangement for drawFilled as with currentTool;  ***TODO***
 using notifications, etc. as per ColorPalette & LineSizePalette */

- (void)setDrawFilled:(BOOL)inDrawFilled
{
    _drawFilled = inDrawFilled;
    [paletteView setFilledShapes:inDrawFilled];
}


- (BOOL)drawFilled
{
    return _drawFilled;
}


- (void)windowDidLoad
{
    [super windowDidLoad];
    
    
}


@end
