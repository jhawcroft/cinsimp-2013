//
//  JHAnswerDialog.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 23/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "acu.h"

@interface JHAnswerDialog : NSWindowController
{
    /* information we will use at initalization */
    __weak NSWindow *_the_owner;
    NSString *_message;
    NSString *_button1;
    NSString *_button2;
    NSString *_button3;
    
    /* window components */
    IBOutlet NSTextField *_m;
    IBOutlet NSButton *_b1;
    IBOutlet NSButton *_b2;
    IBOutlet NSButton *_b3;
    
    /* the stack to which this dialog is attached */
    StackHandle _stack;
}

- (id)initForOwner:(NSWindow*)in_owner withMessage:(NSString*)in_message
button1:(NSString*)in_button1 button2:(NSString*)in_button2 button3:(NSString*)in_button3 stack:(StackHandle)in_stack;


@end
