//
//  JHLineSizePaletteController.m
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 24/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHLineSizePaletteController.h"

#import "JHLineSizeView.h"

#import "JHAppScriptEditorNotifications.h"


NSString *notifyLineSizeDidChange = @"notifyLineSizeDidChange";


@implementation JHLineSizePaletteController


static JHLineSizePaletteController *_g_shared_instance = NULL;


- (NSString*)windowNibName
{
    return @"JHLineSizePalette";
}


+ (JHLineSizePaletteController*)sharedController
{
    if (_g_shared_instance) return _g_shared_instance;
    _g_shared_instance = [[JHLineSizePaletteController alloc] init];
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
        [NSException raise:@"JHLineSizePaletteController shared instance already instantiated!" format:@""];
        return nil;
    }
    
    self = [super initWithWindow:window];
    if (self)
    {
        line_size = 1;
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
    [self setCurrentSize:1];
}


- (void)setCurrentSize:(int)size
{
    line_size = size;
    
    if (_show_invoked)
    {
        if (!_show_saved_visible)
            [[self window] orderOut:self];
        [[self window] setFrame:_show_saved_frame display:_show_saved_visible animate:_show_saved_visible];
        [self finishShowMode];
    }
    
    [[NSNotificationCenter defaultCenter] postNotificationName:notifyLineSizeDidChange object:self];
}


- (int)currentSize
{
    return line_size;
}


- (void)finishShowMode
{
    [[self window] setTitle:@""];
    _show_invoked = NO;
}


- (IBAction)showWindow:(id)sender
{
    if (!_show_invoked)
    {
        _show_saved_frame = [[self window] frame];
        _show_saved_visible = [[self window] isVisible];
    }
    
    NSRect centred_frame = [[self window] frame];
    NSRect screen_frame = [[[self window] screen] visibleFrame];
    centred_frame.origin.x = (screen_frame.size.width - centred_frame.size.width) / 2;
    centred_frame.origin.y = (screen_frame.size.height - centred_frame.size.height) / 2 + centred_frame.size.height;
    if ([[self window] isVisible])
        [[self window] setFrame:centred_frame display:YES animate:YES];
    else
    {
        [[self window] setFrame:centred_frame display:YES animate:NO];
        [[self window] orderFront:self];
    }
    
    [[self window] setTitle:@"Set Line Size"];
    _show_invoked = YES;
}


- (IBAction)toggleSizes:(id)sender
{
    if ([[self window] isVisible]) [[self window] orderOut:self];
    else [[self window] orderFront:self];
    [self finishShowMode];
}


- (void)windowWillClose:(NSNotification *)notification
{
    BOOL _show_was_invoked = _show_invoked;
    [self finishShowMode];
    if (_show_was_invoked)
    {
        [[self window] orderOut:self];
        [[self window] setFrame:_show_saved_frame display:NO animate:NO];
    }
}


- (void)windowDidMove:(NSNotification *)notification
{
    [self finishShowMode];
}



@end
