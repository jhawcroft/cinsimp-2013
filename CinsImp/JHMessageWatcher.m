//
//  JHMessageWatcher.m
//  DevelopDebugWindows
//
//  Created by Joshua Hawcroft on 16/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHMessageWatcher.h"

#include "acu.h"


static JHMessageWatcher *_shared = nil;


@interface JHMessageWatcher ()

@end

@implementation JHMessageWatcher


- (id)initWithWindow:(NSWindow *)window
{
    if (_shared) return _shared;
    
    self = [super initWithWindow:window];
    if (self) {
        _shared = self;
    }
    
    return self;
}


- (NSString*)windowNibName
{
    return @"JHMessageWatcher";
}


- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}


+ (JHMessageWatcher*)sharedController
{
    if (!_shared) _shared = [[JHMessageWatcher alloc] init];
    return _shared;
}


- (IBAction)toggleMessageWatcher:(id)sender
{
    [self loadWindow];
    if ([[self window] isVisible])
        [[self window] orderOut:self];
    else
        [[self window] makeKeyAndOrderFront:self];
}


- (void)_logMessage:(NSString*)in_message level:(int)in_level
{
    NSMutableString *text = [[NSMutableString alloc] init];
    for (int i = 0; i < in_level; i++)
        [text appendString:@"  "];
    [text appendFormat:@"%@\n", in_message];
    BOOL scroll = (NSMaxY(_message_list.visibleRect) == NSMaxY(_message_list.bounds));
    [[_message_list textStorage] appendAttributedString:[[NSAttributedString alloc] initWithString:text]];
    while (_message_list.string.length > 1024)
        [[_message_list textStorage] deleteCharactersInRange:NSMakeRange(0, 100)];
    if (scroll)
        [_message_list scrollRangeToVisible: NSMakeRange(_message_list.string.length, 0)];
}


- (void)debugMessage:(char const*)in_message handlerLevel:(int)in_level wasHandled:(BOOL)in_handled
{
    if (![[self window] isVisible]) return;
    
    NSString *message = [NSString stringWithCString:in_message encoding:NSUTF8StringEncoding];
    
    if ((![_show_idle state]) && ([message isEqualToString:@"idle"])) return;
    if ((![_show_unhandled state]) && (!in_handled)) return;
    
    [self _logMessage:message level:in_level];
}



@end
