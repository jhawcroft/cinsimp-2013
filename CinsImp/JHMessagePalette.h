/*
 
 Message Box Utility Window Controller
 JHMessagePalette.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Window controller for the message box utility window
 
 */

#import <Cocoa/Cocoa.h>

@interface JHMessagePalette : NSWindowController <NSTextFieldDelegate>
{
    IBOutlet NSTextField *message_text;
    NSMutableString *blind_buffer;
    
    BOOL _was_visible_se;
}

+ (JHMessagePalette*)sharedController;

- (IBAction)userHitEnter:(id)sender;
- (void)setText:(NSString*)in_text;

- (void)handleUnfocusedKeyDown:(NSEvent*)in_event;

@end
