/*
 
 Protect Stack
 JHProtectStack.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Controller for the Protect Stack sheet which provides options affecting access to a stack
 
 */

#import <Cocoa/Cocoa.h>

#include "stack.h"

@interface JHProtectStack : NSWindowController
{
    /* the stack being edited */
    Stack *stack;
    
    /* is the stack file locked outside of CinsImp? */
    BOOL stack_file_locked;
    
    /* the window for the stack being edited */
    __weak NSWindow *a_parent;
    
    /* the password for the stack being edited */
    NSString *the_password;
    
    /* Protect Stack - UI widgets */
    IBOutlet NSButton *checkLocked;
    IBOutlet NSButton *checkCantPeek;
    IBOutlet NSButton *checkPrivate;
    
    IBOutlet NSButton *buttonSetPassword;
    
    IBOutlet NSMatrix *radiosLimitUserLevel;
    
    IBOutlet NSPanel *set_password;
    
    IBOutlet NSTextField *password;
    IBOutlet NSTextField *password_again;
    
    /* What's the password? - UI widgets */
    IBOutlet NSPanel *get_password;
    IBOutlet NSTextField *a_password;
    
    /* is the user's password guess correct? */
    BOOL is_correct;
    
    /* miscellaneous state variables */
    BOOL showStackPropertiesAfterPassword;
    id ok_del;
    SEL ok_sel;
}

/* has password been checked with user? */
@property (nonatomic) BOOL checked_password;

/* handlers for checking password for given stack */
- (void)guessPasswordForStack:(Stack*)in_stack forWindow:(NSWindow*)in_window withDelegate:(id)in_del selector:(SEL)in_sel;
- (BOOL)isPasswordGuessCorrect;
- (IBAction)confirmGuessPassword:(id)sender;

/* action handlers for set password sub-sheet */
- (IBAction)cancelSetPassword:(id)sender;
- (IBAction)okSetPassword:(id)sender;
- (IBAction)noneSetPassword:(id)sender;

/* request to display the Protect Stack sheet for a given stack */
- (void)setStackAndShowProperties:(Stack*)in_stack forWindow:(NSWindow*)in_parent;

@end
