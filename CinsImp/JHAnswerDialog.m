//
//  JHAnswerDialog.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 23/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHAnswerDialog.h"

#include "acu.h"

@interface JHAnswerDialog ()

- (IBAction)button1Action:(id)sender;
- (IBAction)button2Action:(id)sender;
- (IBAction)button3Action:(id)sender;

@end

@implementation JHAnswerDialog


- (id)initForOwner:(NSWindow*)in_owner withMessage:(NSString*)in_message button1:(NSString*)in_button1 button2:(NSString*)in_button2 button3:(NSString*)in_button3 stack:(StackHandle)in_stack
{
    self = [super initWithWindowNibName:@"JHAnswerDialog"];
    if (self) {
        _the_owner = in_owner;
        _message = in_message;
        _button1 = in_button1;
        _button2 = in_button2;
        _button3 = in_button3;
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
    
    float ht = [self _computeTextHeight:_message forWidth:_m.frame.size.width];
    [self.window setFrame:NSMakeRect(0, 0, self.window.frame.size.width, ht + 120) display:NO];
    
    [_m setStringValue:_message];

    if (_button1 != nil) [_b1 setTitle:_button1];
    else [_b1 setHidden:YES];
    
    if (_button2 != nil) [_b2 setTitle:_button2];
    else [_b2 setHidden:YES];
    
    if (_button3 != nil) [_b3 setTitle:_button3];
    else [_b3 setHidden:YES];
    
    [NSApp beginSheet:self.window modalForWindow:_the_owner modalDelegate:self didEndSelector:nil contextInfo:nil];
}


- (IBAction)button1Action:(id)sender
{
    [NSApp endSheet:self.window];
    [self.window orderOut:self];
    acu_answer_choice_reply(_stack, 0, [[_b1 title] UTF8String]);
}


- (IBAction)button2Action:(id)sender
{
    [NSApp endSheet:self.window];
    [self.window orderOut:self];
    acu_answer_choice_reply(_stack, 1, [[_b2 title] UTF8String]);
}


- (IBAction)button3Action:(id)sender
{
    [NSApp endSheet:self.window];
    [self.window orderOut:self];
    acu_answer_choice_reply(_stack, 2, [[_b3 title] UTF8String]);
}

/*
- (void)dealloc
{
    NSLog(@"answer dealloc");
}
*/


@end
