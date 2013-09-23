/*
 
 Card Window
 JHCardWindow.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 The window that displays the current card of the open stack
 
 */

#import <Cocoa/Cocoa.h>
#import "JHCardView.h"
#import "JHProtectStack.h"
#include "xtalk_engine.h"

#import "JHIconManagerController.h"

@interface JHCardWindow : NSWindow
{
    IBOutlet JHCardView *cardView;
    
    /* sheet used by the user to set stack security and miscellaneous access options */
    JHProtectStack *protect_stack;
    
    /* track if we've queried the user for the access password */
    BOOL checkedPassword;
    
    /* xtalk engine */
    //XTE *xtalk;
    
    JHIconManagerController *_icon_manager;
    
}

//- (void)setXTalk:(XTE*)in_xtalk;

- (JHCardView*)cardView;

- (IBAction)checkPassword:(id)sender;


- (IBAction)manageIcons:(id)sender;
- (IBAction)setIcon:(id)sender;


@end
