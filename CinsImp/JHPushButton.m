/*
 
 Push Button
 JHPushButton.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHPushButton.h"

#include "acu.h"

@implementation JHPushButton


- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        enable_mouse_up = NO;
    }
    return self;
}


- (void)setContent:(NSData*)in_content searchable:(NSString*)in_searchable editable:(BOOL)in_editable
{
    
}

- (BOOL)isButton
{
    return YES;
}


- (NSView*)eventTargetBrowse
{
    return self;
}


- (void)mouseDown:(NSEvent *)theEvent
{
    acu_finger_start();
    if (!acu_ui_acquire_stack(self.stack)) return;

    [self highlight:YES];
    [self setNeedsDisplay];
    enable_mouse_up = YES;
}



- (void)mouseUp:(NSEvent *)theEvent
{
    
    [self highlight:NO];
    [self setNeedsDisplay];
    
    //if (!acu_ui_acquire_stack(self.stack)) return;
    if (enable_mouse_up)
    {
        enable_mouse_up = NO;
        acu_post_system_event(acu_handle_for_button_id(STACKMGR_FLAG_AUTO_RELEASE, self.stack, self.widget_id), "mouseUp", NULL);
    }
        
}


/*
- (BOOL)canBecomeKeyView
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}
*/

/*
- (void)keyDown:(NSEvent *)event
{
    NSString *chars = [event characters];
    unichar character = [chars characterAtIndex: 0];
    
    
    if (character == NSTabCharacter) // tab
    {
        [self.card_view performSelector:@selector(_handleTab:) withObject:event];
        return;
    }
    else if (character == NSBackTabCharacter) // tab
    {
        [self.card_view performSelector:@selector(_handleBackTab:) withObject:event];
        return;
    }
    [super keyDown:event];
}
*/

@end
