//
//  JHMessageWatcher.h
//  DevelopDebugWindows
//
//  Created by Joshua Hawcroft on 16/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface JHMessageWatcher : NSWindowController
{
    IBOutlet NSTextView *_message_list;
    
    IBOutlet NSButton *_show_idle;
    IBOutlet NSButton *_show_unhandled;
}

+ (JHMessageWatcher*)sharedController;

- (IBAction)toggleMessageWatcher:(id)sender;

- (void)debugMessage:(char const*)in_message handlerLevel:(int)in_level wasHandled:(BOOL)in_handled;

@end
