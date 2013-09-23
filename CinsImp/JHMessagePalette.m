/*
 
 Message Box Utility Window Controller
 JHMessagePalette.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHMessagePalette.h"

#import "JHAppScriptEditorNotifications.h"

#import "jhglobals.h"
#import "JHCardView.h"
#include "xtalk_engine.h"


static JHMessagePalette *_g_controller = nil;


@implementation JHMessagePalette


/*********
 Auto-hide for Script Editing
 */

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


/*********
 Initalization
 */

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self)
    {
        _g_controller = self;
        blind_buffer = [[NSMutableString alloc] init];
        [self loadWindow];
        
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


/*********
 Event Propagation
 */

- (void)flagsChanged:(NSEvent *)theEvent
{
    /* pass the event along to the current card view */
    if (gCurrentCardView) [gCurrentCardView flagsChanged:theEvent];
}


/*********
 Showing/Hiding the Utility Window
 */

- (IBAction)toggleMessage:(id)sender
{
    [blind_buffer setString:@""]; /* reset blind typing buffer */
    if (self.window.isVisible)
        [self.window orderOut:nil];
    else
        [self.window makeKeyAndOrderFront:nil];
}


- (IBAction)goFind:(id)sender
{
    [blind_buffer setString:@""]; /* reset blind typing buffer */
    
    NSString *current_string = [message_text stringValue];
    [self.window makeFirstResponder:message_text];
    NSText* textEditor = [self.window fieldEditor:YES forObject:message_text];
    NSArray *words = [current_string componentsSeparatedByString:@" "];
    
    if (([words count] > 0) && ([(NSString*)[words objectAtIndex:0] caseInsensitiveCompare:@"find"] == NSOrderedSame))
    {
        NSRange begin = [current_string rangeOfString:@"\""];
        if (begin.location != NSNotFound)
        {
            NSRange end = [current_string rangeOfString:@"\""
                                            options:0
                                              range:NSMakeRange(begin.location+1, [current_string length] - (begin.location+1))];
            if (end.location != NSNotFound)
            {
                [textEditor setSelectedRange:NSMakeRange(begin.location + 1, end.location - begin.location - 1)];
                [self.window makeKeyAndOrderFront:nil];
                return;
            }
        }
    }
    
    [message_text setStringValue:@"find \"\""];
    [textEditor setSelectedRange:NSMakeRange(6, 0)];
    [self.window makeKeyAndOrderFront:nil];
}


/*********
 Message Handling
 */

+ (JHMessagePalette*)sharedController
{
    return _g_controller;
}


- (IBAction)userHitEnter:(id)sender
{
    [blind_buffer setString:@""]; /* reset blind typing buffer */
    
    
    if (gCurrentCardView)
    {
        acu_message([gCurrentCardView stack], [[message_text stringValue] UTF8String]);
    }
    
    else
    {
        NSAlert *alert = [NSAlert alertWithMessageText:NSLocalizedString(@"MESSAGE_BOX_NOT_AVAILBLE", @"")
                                         defaultButton:NSLocalizedString(@"Cancel", @"") alternateButton:nil otherButton:nil informativeTextWithFormat:@""];
        [alert runModal];
    }
}


- (void)setText:(NSString*)in_text
{
    [blind_buffer setString:@""]; /* reset blind typing buffer */
    [message_text setStringValue:in_text];
    
    [[self window] display]; /* force display immediately */
}


- (void)handleUnfocusedKeyDown:(NSEvent*)in_event
{
    /* support for blind typing and automatic focus */
    if ([[self window] isVisible])
    {
        /* check if we should just run the command */
        unichar c = [[in_event charactersIgnoringModifiers] characterAtIndex:0];
        if ((c == NSCarriageReturnCharacter) || (c == NSEnterCharacter))
        {
            [self userHitEnter:self];
        }
        else
        {
            /* order the window to front and begin typing afresh */
            [[self window] makeKeyAndOrderFront:self];
            [message_text setStringValue:@""];
            [message_text setStringValue:[in_event characters]];
            NSText* field_editor = [[self window] fieldEditor:YES forObject:message_text];
            [field_editor setSelectedRange:NSMakeRange([[message_text stringValue] length], 0)];
        }
    }
    else
    {
        /* allow blind typing into an offscreen buffer */
        unichar c = [[in_event charactersIgnoringModifiers] characterAtIndex:0];
        if ((c == NSCarriageReturnCharacter) || (c == NSEnterCharacter))
        {
            [self userHitEnter:self];
        }
        else if ((c == NSBackspaceCharacter) || (c == NSDeleteCharacter))
        {
            if ([blind_buffer length] < 1) return;
            [blind_buffer deleteCharactersInRange:NSMakeRange([blind_buffer length]-1, 1)];
            [message_text setStringValue:blind_buffer];
        }
        else
        {
            [blind_buffer appendString:[in_event characters]];
            [message_text setStringValue:blind_buffer];
        }
    }
}



@end
