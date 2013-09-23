//
//  JHAskDialog.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 25/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHAskDialog.h"

#include "acu.h"

@interface JHAskDialog ()

- (IBAction)ok:(id)sender;
- (IBAction)cancel:(id)sender;

@end

@implementation JHAskDialog


- (id)initForOwner:(NSWindow*)in_owner withMessage:(NSString*)in_message
          response:(NSString*)in_response forPassword:(BOOL)in_password_mode stack:(StackHandle)in_stack
{
    self = [super initWithWindowNibName:@"JHAskDialog"];
    if (self) {
        _the_owner = in_owner;
        _message = in_message;
        _response = in_response;
        _password_mode = in_password_mode;
        
        _stack = in_stack;
        [self loadWindow];
    }
    
    return self;
}


- (float)_computeTextHeight:(NSString*)in_text forWidth:(float)in_width
{
    NSTextStorage *text_storage = [[NSTextStorage alloc] initWithString:in_text];
    NSTextContainer *text_container = [[NSTextContainer alloc] initWithContainerSize:NSMakeSize(in_width, FLT_MAX)];
    NSLayoutManager *layout_manager = [[NSLayoutManager alloc] init];
    [layout_manager addTextContainer:text_container];
    [text_storage addLayoutManager:layout_manager];
    [text_storage addAttribute:NSFontAttributeName value:[_m font] range:NSMakeRange(0, [text_storage length])];
    [text_container setLineFragmentPadding:0.0];
    (void)[layout_manager glyphRangeForTextContainer:text_container];
    return [layout_manager usedRectForTextContainer:text_container].size.height;
}


- (void)awakeFromNib
{
    [super awakeFromNib];
    
    float ht = [self _computeTextHeight:_message forWidth:_m.frame.size.width] +
    [self _computeTextHeight:_response forWidth:_m.frame.size.width];
    [self.window setFrame:NSMakeRect(0, 0, self.window.frame.size.width, ht + 120) display:NO];
    
    [_m setStringValue:_message];
    
    if (_password_mode)
    {
        [_s setStringValue:_response];
        [_r setHidden:YES];
    }
    else
    {
        [_r setStringValue:_response];
        [_s setHidden:YES];
    }
    
    [NSApp beginSheet:self.window modalForWindow:_the_owner modalDelegate:self didEndSelector:nil contextInfo:nil];
}


- (IBAction)ok:(id)sender
{
    [NSApp endSheet:self.window];
    [self.window orderOut:self];
    acu_ask_choice_reply(_stack, (_password_mode ? [[_s stringValue] UTF8String] : [[_r stringValue] UTF8String]));
}


- (IBAction)cancel:(id)sender
{
    [NSApp endSheet:self.window];
    [self.window orderOut:self];
    acu_ask_choice_reply(_stack, NULL);
}


@end
