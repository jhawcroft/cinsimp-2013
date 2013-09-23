/*
 
 Multiline Text Field
 JHTextFieldScroller.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHTextFieldScroller.h"
#import "JHTextField.h"

#include "stack.h"

@implementation JHTextFieldScroller

- (void)viewDidMoveToWindow
{
    if (![self window]) return;
    
    [self.documentView setDelegate:self];
    JHTextField *text_field = self.documentView;
    text_field.card_view = self.card_view;
    text_field.widget_id = self.widget_id;
    is_editing = NO;
    find_indicator = NSMakeRange(0, 0);
    trackingArea = nil;
}


- (void)updateTrackingAreas
{
    [self removeTrackingArea:trackingArea];
    trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds options:(NSTrackingActiveInActiveApp | NSTrackingCursorUpdate | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved)
                                                  owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}


- (void)cursorUpdate:(NSEvent *)event
{
    [self.card_view _setAppropriateCursor];
}


- (void)scrollWheel:(NSEvent *)theEvent
{
    [super scrollWheel:theEvent];
    [self.card_view _setAppropriateCursor];
}


- (void)setContent:(NSData*)in_content searchable:(NSString*)in_searchable editable:(BOOL)in_editable
{
    JHTextField *text_field = self.documentView;
    
    if ([in_content length] != 0)
        [text_field replaceCharactersInRange:NSMakeRange(0, text_field.string.length) withRTFD:in_content];
    else
        [text_field setString:in_searchable];
    
    self.is_locked = !in_editable;
    [text_field setEditable:in_editable];
    [text_field setSelectable:in_editable];
    
    [text_field scrollToBeginningOfDocument:self];
}


- (void)stripFindIndicator
{
    if (find_indicator.length != 0)
    {
        [[[self documentView] textStorage] removeAttribute:NSBackgroundColorAttributeName range:find_indicator];
        
        find_indicator = NSMakeRange(0, 0);
    }
}




- (void)textDidBeginEditing:(NSNotification *)notification
{
    [self stripFindIndicator];
    is_editing = YES;
    //dirty = NO;
    //NSLog(@"begin");
}


- (void)textDidChange:(NSNotification *)notification
{
    dirty = YES;
    //NSLog(@"change text");
}


- (void)textViewDidChangeSelection:(NSNotification *)notification
{
    if (!is_editing)
    {
        //NSLog(@"Got Focus");
        [self stripFindIndicator];
        is_editing = YES;
    }
}





- (void)textDidEndEditing:(NSNotification *)notification
{
    //NSLog(@"end");
    /* check that editing is actually occurring by the user */
    if (!is_editing) return;
    is_editing = NO;
    
    /* check if the field is dirty */
    if (!dirty) return;
    
    if (!acu_ui_acquire_stack(self.stack)) return;
    
    NSLog(@"saving field");
    /* save dirty field */
    stack_undo_activity_begin(self.stack, "Text", self.current_card_id);
    JHTextField *text_field = self.documentView;
    NSData *formatted = [text_field RTFDFromRange:NSMakeRange(0, text_field.string.length)];
    NSString *searchable = [text_field string];
    stack_widget_content_set(self.stack, self.widget_id, self.current_card_id,
                             (char*)[searchable cStringUsingEncoding:NSUTF8StringEncoding],
                             (char*)[formatted bytes], [formatted length]);
    stack_undo_activity_end(self.stack);
}


- (void)setFindIndicator:(NSRange)in_range;
{
    find_indicator = in_range;
    [[self documentView] scrollRangeToVisible:find_indicator];
    [[[self documentView] textStorage] addAttributes:[NSDictionary dictionaryWithObject:[NSColor yellowColor]
                                                                                 forKey:NSBackgroundColorAttributeName] range:find_indicator];
    //[[[self documentView] textStorage] setAttributes:
    // [NSDictionary dictionaryWithObject:[NSColor yellowColor]
    //                             forKey:NSBackgroundColorAttributeName] range:find_indicator];
    [self setNeedsDisplay:YES];
}


- (void)setBorder:(NSString*)in_border
{
    //NSLog(@"border=%@", in_border);
    JHTextField *text_field = self.documentView;
    if ([in_border isEqualToString:@"opaque"])
    {
        [self setBorderType:NSNoBorder];
        [self setHasVerticalScroller:NO];
        [self setDrawsBackground:YES];
        [text_field setDrawsBackground:YES];
    }
    else if ([in_border isEqualToString:@"transparent"])
    {
        [self setBorderType:NSNoBorder];
        [self setHasVerticalScroller:NO];
        [self setDrawsBackground:NO];
        [text_field setDrawsBackground:NO];
    }
    else if ([in_border isEqualToString:@"scrolling"])
    {
        [self setBorderType:NSLineBorder];
        [self setHasVerticalScroller:YES];
        [self setDrawsBackground:YES];
        [text_field setDrawsBackground:YES];
    }
    else if ([in_border isEqualToString:@"rectangle"])
    {
        [self setBorderType:NSLineBorder];
        [self setHasVerticalScroller:NO];
        [self setDrawsBackground:YES];
        [text_field setDrawsBackground:YES];
    }
}




- (BOOL)isButton
{
    return NO;
}

/*
- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    if (outlines)
    {
        [[NSColor grayColor] setStroke];
        NSFrameRect(self.bounds);
    }
}
*/


- (BOOL)becomeFirstResponder
{
    if ([super becomeFirstResponder])
    {
        //NSLog(@"Got Focus");
        [self stripFindIndicator];
        dirty = NO;
        is_editing = YES;
        return YES;
    }
    else return NO;
}


- (BOOL)resignFirstResponder
{
    if (!acu_ui_acquire_stack(self.stack)) return NO;
    
    if ([super resignFirstResponder])
    {
        //NSLog(@"Lost Focus");
        return YES;
    }
    else return NO;
}


- (NSView*)eventTargetBrowse
{
    //NSLog(@"event target widget ID %ld", self.widget_id);
    return [self documentView];
}

@end
