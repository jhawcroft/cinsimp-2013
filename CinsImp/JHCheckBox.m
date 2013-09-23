/*
 
 Check Box
 JHCheckBox.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHCheckBox.h"

#include "acu.h"

@implementation JHCheckBox


- (BOOL)isButton
{
    return NO;
}


- (BOOL)becomeFirstResponder
{
    BOOL result = [super becomeFirstResponder];
    if (result) [self.card_view performSelector:@selector(handleFieldFocusChange:) withObject:[NSNumber numberWithLong:self.widget_id]];
    return result;
}


- (BOOL)resignFirstResponder
{
    BOOL result = [super resignFirstResponder];
    if (result) [self.card_view performSelector:@selector(handleFieldFocusChange:) withObject:nil];
    return result;
}

- (void)mouseDown:(NSEvent *)theEvent
{
    if (!acu_ui_acquire_stack(self.stack)) return;
    [super mouseDown:theEvent];
}




- (void)didChangeState:(id)sender
{
    if (self.state == NSOnState)
        stack_undo_activity_begin(self.stack, "Check", self.current_card_id);
    else
        stack_undo_activity_begin(self.stack, "Uncheck", self.current_card_id);
    NSString *searchable = ((self.state == NSOnState) ? @"true" : @"false");
    stack_widget_content_set(self.stack, self.widget_id, self.current_card_id,
                             (char*)[searchable cStringUsingEncoding:NSUTF8StringEncoding],
                             NULL, 0);
    stack_undo_activity_end(self.stack);
}


- (void)setContent:(NSData*)in_content searchable:(NSString*)in_searchable editable:(BOOL)in_editable
{
    if ([in_searchable isEqualToString:@"true"])
        self.state = NSOnState;
    else
        self.state = NSOffState;
    self.is_locked = !in_editable;
    [self setEnabled:in_editable];
}


- (void)keyDown:(NSEvent *)event
{
    if (!acu_ui_acquire_stack(self.stack)) return;
    
    NSString *chars = [event characters];
    unichar character = [chars characterAtIndex: 0];
    
    //NSLog(@"keycode=%d", [theEvent keyCode]);
    if (character == NSTabCharacter) // tab
    {
        [self.card_view performSelector:@selector(handleTab:) withObject:event];
        return;
    }
    else if (character == NSBackTabCharacter) // tab
    {
        [self.card_view performSelector:@selector(handleBackTab:) withObject:event];
        return;
    }
    else if (character == ' ')
    {
        self.state = !self.state;
        [self didChangeState:self];
        return;
    }
    [super keyDown:event];
}

- (NSView*)eventTargetBrowse
{
    return self;
}



@end
