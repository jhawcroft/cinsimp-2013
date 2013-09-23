//
//  JHColourPaletteController.m
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 23/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHColourPaletteController.h"

#import "JHColourPaletteView.h"
#import "JHAppScriptEditorNotifications.h"


NSString *notifyColourDidChange = @"notifyColourDidChange";


@implementation JHColourPaletteController


static JHColourPaletteController *_g_shared_instance = NULL;


- (NSString*)windowNibName
{
    return @"JHColourPalette";
}


+ (JHColourPaletteController*)sharedController
{
    if (_g_shared_instance) return _g_shared_instance;
    _g_shared_instance = [[JHColourPaletteController alloc] init];
    return _g_shared_instance;
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


- (id)initWithWindow:(NSWindow *)window
{
    if (_g_shared_instance)
    {
        [NSException raise:@"JHColourPaletteController shared instance already instantiated!" format:@""];
        return nil;
    }
    
    self = [super initWithWindow:window];
    if (self)
    {
        //if (!window)
        //    [[NSBundle mainBundle] loadNibNamed:@"JHColourPalette" owner:self topLevelObjects:nil];
        _inited_menu = NO;
        _currentColour = [NSColor colorWithDeviceRed:0 green:0 blue:0 alpha:1];
        _g_shared_instance = self;
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(scriptEditorBecameActive:)
                                                     name:scriptEditorBecameActive
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(scriptEditorBecameInactive:)
                                                     name:scriptEditorBecameInactive
                                                   object:nil];
    }
    return self;
}


- (void)awakeFromNib
{
    [self loadWindow];
    if ((colourMenu) && (colourMenuView) && (!_inited_menu))
    {
        _inited_menu = YES;
        NSMenuItem *colourMenuItem = [[NSMenuItem alloc] init];
        [colourMenuItem setView:colourMenuView];
        [colourMenu addItem:colourMenuItem];
    }
}









- (IBAction)toggleColours:(id)sender
{
    if (self.window.isVisible)
        [self.window orderOut:self];
    else
        [self.window orderFront:self];
}


- (void)setCurrentColour:(NSColor*)colour
{
    if (!colour) return;
    
    _currentColour = colour;
    
    [[NSNotificationCenter defaultCenter] postNotificationName:notifyColourDidChange object:self];
}


- (NSColor*)currentColour
{
    return _currentColour;
}


@end
