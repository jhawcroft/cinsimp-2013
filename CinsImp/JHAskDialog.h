//
//  JHAskDialog.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 25/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "acu.h"

@interface JHAskDialog : NSWindowController
{
    /* information we will use at initalization */
    __weak NSWindow *_the_owner;
    NSString *_message;
    NSString *_response;
    BOOL _password_mode;
    
    /* window components */
    IBOutlet NSTextField *_m;
    IBOutlet NSTextField *_r;
    IBOutlet NSSecureTextField *_s;
    
    /* the stack to which this dialog is attached */
    StackHandle _stack;
}

- (id)initForOwner:(NSWindow*)in_owner withMessage:(NSString*)in_message
          response:(NSString*)in_response forPassword:(BOOL)in_password_mode stack:(StackHandle)in_stack;

@end
