/*
 
 Protect Stack
 JHProtectStack.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHProtectStack.h"


@implementation JHProtectStack


/* included here for debugging to ensure document closure is not leaking; not required in production */
/*- (void)dealloc
{
    NSLog(@"Protect stack controller dealloc");
}*/



/***********
 Display Protect Stack
 */


/* invoked either immediately once the sheet is loaded or pending successful validation of stack password */
- (void)goAhead
{
    /* show the Protect Stack sheet */
    [NSApp beginSheet:self.window modalForWindow:a_parent modalDelegate:nil didEndSelector:nil contextInfo:nil];
}


/* request the Protect Stack sheet for a given stack */
- (void)setStackAndShowProperties:(Stack*)in_stack forWindow:(NSWindow*)in_parent
{
    /* load our nib */
    [self window];
    
    /* record who our parent is */
    a_parent = in_parent;
    
    /* get our stack */
    stack = in_stack;
    stack_file_locked = stack_file_is_locked(stack);
    
    /* get the password for the stack */
    the_password = [NSString stringWithCString:stack_prop_get_string(stack, 0, 0, PROPERTY_PASSWORD) encoding:NSUTF8StringEncoding];
    
    /* load stack properties */
    if (stack_prop_get_long(stack, 0, 0, PROPERTY_LOCKED))
        checkLocked.state = NSOnState;
    else
        checkLocked.state = NSOffState;
    if (stack_prop_get_long(stack, 0, 0, PROPERTY_CANTPEEK))
        checkCantPeek.state = NSOnState;
    else
        checkCantPeek.state = NSOffState;
    if (stack_prop_get_long(stack, 0, 0, PROPERTY_PRIVATE))
        checkPrivate.state = NSOnState;
    else
        checkPrivate.state = NSOffState;
    
    [radiosLimitUserLevel selectCellAtRow:stack_prop_get_long(stack, 0, 0, PROPERTY_USERLEVEL) column:0];
    
    /* disable options if stack file is locked */
    if (stack_file_locked)
    {
        [checkLocked setEnabled:NO];
        [checkPrivate setEnabled:NO];
        [checkCantPeek setEnabled:NO];
        [radiosLimitUserLevel setEnabled:NO];
        [buttonSetPassword setEnabled:NO];
    }
    
    /* check for stack password */
    if ([the_password length] != 0)
    {
        /* request and verify password prior to showing sheet */
        showStackPropertiesAfterPassword = YES;
        [NSApp beginSheet:get_password modalForWindow:in_parent modalDelegate:nil
           didEndSelector:nil contextInfo:nil];
    }
    else
        /* show the sheet immediately; no password check required */
        [self goAhead];

}


/* protect stack sheet cancelled */
- (IBAction)cancel:(id)sender
{
    [NSApp endSheet:self.window returnCode:NSRunStoppedResponse];
    [self.window orderOut:self];
}


/* protect stack sheet - save changes */
- (IBAction)ok:(id)sender
{
    /* make the changes according to the UI */
    if (!stack_file_locked)
    {
        stack_prop_set_long(stack, 0, 0, PROPERTY_LOCKED, (checkLocked.state == NSOnState));
        stack_prop_set_long(stack, 0, 0, PROPERTY_CANTPEEK, (checkCantPeek.state == NSOnState));
        stack_prop_set_long(stack, 0, 0, PROPERTY_PRIVATE, (checkPrivate.state == NSOnState));
        
        stack_prop_set_long(stack, 0, 0, PROPERTY_USERLEVEL, [radiosLimitUserLevel selectedRow]);
        
        stack_prop_set_string(stack, 0, 0, PROPERTY_PASSWORD, (char*)[the_password cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    
    /* close the sheet */
    [NSApp endSheet:self.window returnCode:NSRunContinuesResponse];
    [self.window orderOut:self];
    
    /* flush the stack undo manager */
    stack_undo_flush(stack);
}


/* display the set password sheet */
- (IBAction)setAPassword:(id)sender
{
    [NSApp beginSheet:set_password modalForWindow:self.window modalDelegate:self
       didEndSelector:@selector(didSetPassword:returnCode:contextInfo:) contextInfo:nil];
}



/***********
 Display Set Password
 */


- (void)didSetPassword:(NSWindow *)in_set_password returnCode:(NSInteger)in_return_code contextInfo:(void *)in_context
{
    if (in_return_code != NSRunContinuesResponse) return;
}


- (IBAction)cancelSetPassword:(id)sender
{
    [NSApp endSheet:set_password returnCode:NSRunStoppedResponse];
    [set_password orderOut:self];
}


- (IBAction)okSetPassword:(id)sender
{
    if (![password.stringValue isEqualToString:password_again.stringValue])
    {
        /* user entered different passwords into the two boxes */
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Passwords don't match."];
        [alert setAlertStyle:NSCriticalAlertStyle];
        [alert setInformativeText:@"Check that you've typed the same password into both boxes, then try again."];
        [alert runModal];
        return;
    }
    
    /* store the password to be saved if the Protect Stack sheet is acknowledged by the user */
    the_password = password.stringValue;
    
    /* close the set password sheet */
    [NSApp endSheet:set_password returnCode:NSRunContinuesResponse];
    [set_password orderOut:self];
}


- (IBAction)noneSetPassword:(id)sender
{
    the_password = @"";
    
    [NSApp endSheet:set_password returnCode:NSRunContinuesResponse];
    [set_password orderOut:self];
}




/***********
 Display Get Password ('What's the password?')
 */


- (IBAction)confirmGuessPassword:(id)sender
{
    NSString *guess = a_password.stringValue;
    is_correct = [the_password isEqualToString:guess];
    [NSApp endSheet:get_password];
    [get_password orderOut:self];
    
    if (is_correct && showStackPropertiesAfterPassword) [self goAhead];
    else  {
        [ok_del performSelector:ok_sel]; // ignore warning
        /* but wouldn't mind doing something different here anyway... in future... */
    }
}


- (BOOL)isPasswordGuessCorrect
{
    return is_correct;
}


- (void)guessPasswordForStack:(Stack*)in_stack forWindow:(NSWindow*)in_window withDelegate:(id)in_del selector:(SEL)in_sel
{
    [self window]; // load the nib
    a_parent = in_window;
    stack = in_stack;
    
    self.checked_password = YES;
    
    the_password = [NSString stringWithCString:stack_prop_get_string(stack, 0, 0, PROPERTY_PASSWORD) encoding:NSUTF8StringEncoding];
    
    ok_del = in_del;
    ok_sel = in_sel;
    showStackPropertiesAfterPassword = NO;

    [NSApp beginSheet:get_password modalForWindow:in_window modalDelegate:nil didEndSelector:nil contextInfo:nil];
}



@end
