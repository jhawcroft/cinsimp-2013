//
//  JHCardView+JHCardView_UIEvents.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView.h"

#import "JHMessagePalette.h"


@implementation JHCardView (JHCardView_UIEvents)



/************
 Event Handlers
 */



- (void)mouseMoved:(NSEvent *)theEvent
{
    
    [self _setAppropriateCursor];
    
    if (stackmgr_script_running(stack)) return;
    
    
    /* check for painting */
    if (_paint_subsys)
    {
        NSPoint mouse_loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
        paint_event_mouse_moved(_paint_subsys, mouse_loc.x, mouse_loc.y);
        return;
    }
    
}


- (void)cursorUpdate:(NSEvent *)theEvent
{
    [self _setAppropriateCursor];
}


/*
 - (void)mouseEntered:(NSEvent *)theEvent
 {
 }
 
 
 - (void)mouseExited:(NSEvent *)theEvent
 {
 }
 */


- (void)flagsChanged:(NSEvent *)theEvent
{
    if (stackmgr_script_running(stack)) return;
    
    long flags = [theEvent modifierFlags];
    
    if (stack_prop_get_long(stack, 0, 0, PROPERTY_CANTPEEK))
    {
        peek_flds = NO;
        peek_btns = NO;
    }
    else
    {
        peek_flds = ((flags & NSCommandKeyMask) && (flags & NSAlternateKeyMask) && (flags & NSShiftKeyMask));
        peek_btns = ((flags & NSCommandKeyMask) && (flags & NSAlternateKeyMask));
    }
    
    //[self setNeedsDisplay:YES];
    [_layer_overlay setNeedsDisplay:YES];
}

/*
 - (void)forwardUIEvent:(NSEvent*)theEvent toSelector:(SEL)inSelector
 {
 static BOOL withinForward = NO;
 
 if (withinForward) return;
 withinForward = YES;
 
 NSView *targetView = [self hitTarget:dragStart];
 if (targetView == nil) return;
 if ([targetView respondsToSelector:inSelector])
 {
 NSLog(@"forwarding %@\n", NSStringFromSelector(inSelector));
 if (inSelector == @selector(mouseDown:))
 [targetView mouseDown:theEvent];
 else if (inSelector == @selector(mouseDragged:))
 [targetView mouseDragged:theEvent];
 else if (inSelector == @selector(mouseUp:))
 [targetView mouseUp:theEvent];
 else if (inSelector == @selector(mouseMoved:))
 [targetView mouseMoved:theEvent];
 }
 
 withinForward = NO;
 }*/


- (void)mouseDown:(NSEvent *)theEvent
{
    if (stackmgr_script_running(stack)) return;
    
    /* reset drag tracking */
    did_drag = NO;
    dragStart = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    
    /* check for painting */
    if (_paint_subsys)
    {
        paint_event_mouse_down(_paint_subsys, dragStart.x, dragStart.y);
        return;
    }
    
    
    /* find out what's being clicked */
    NSView *targetView = [self _hitTarget:dragStart];
    
    
    
    
    
    
    /* check if we're in layout mode;
     if we're not, then just pass the event to the appropriate target object */
    if (currentTool == kJHToolBrowse)
    {
        if ([targetView conformsToProtocol:@protocol(JHWidget)])
        {
            NSView<JHWidget> *widget = (NSView<JHWidget>*)targetView;
            if (([widget isButton]) && peek_btns)
            {
                //NSLog(@"clicked button in peek!");
                peek_btns = NO;
                peek_flds = NO;
                [self setNeedsDisplay:YES];
                [_layer_overlay setNeedsDisplay:YES];
                StackHandle target = acu_handle_for_button_id(STACKMGR_FLAG_AUTO_RELEASE, stack, widget.widget_id);
                [self openScriptEditorFor:target atSourceLine:INVALID_LINE_NUMBER debugging:NO];
            }
            else if ((![widget isButton]) && peek_flds)
            {
                //NSLog(@"clicked field in peek!");
                peek_btns = NO;
                peek_flds = NO;
                [self setNeedsDisplay:YES];
                [_layer_overlay setNeedsDisplay:YES];
                StackHandle target = acu_handle_for_field_id(STACKMGR_FLAG_AUTO_RELEASE, stack, widget.widget_id, ACU_INVALID_HANDLE);
                [self openScriptEditorFor:target atSourceLine:INVALID_LINE_NUMBER debugging:NO];
            }
        }
        /*if ([dispatching_event hash] == [theEvent hash]) return;
         dispatching_event = theEvent;
         target_view = targetView;
         if ([target_view respondsToSelector:@selector(mouseDown:)])
         [targetView mouseDown:theEvent];*/
        return;
    }
    
    /* in layout mode... handle selection of widgets */
    if ( (targetView) && ([targetView isKindOfClass:[JHSelectorView class]]) )
    {
        /* selected object clicked */
        
        /* if we're Shift-key multiple selecting; deselect that object */
        if ([NSEvent modifierFlags] & NSShiftKeyMask)
            [self _deselectWidget:((JHSelectorView*)targetView).widget];
        else
        {
            /* determine if we're attempting to resize the selected object */
            dragIsResize = ( [((JHSelectorView*)targetView) mouseWithinResizeHandle:
                              [((JHSelectorView*)targetView) convertPoint:[theEvent locationInWindow] fromView:nil]] );
        }
    }
    /*else if ( (targetView) && (([targetView isKindOfClass:[NSScrollView class]]) ||
     ([targetView isKindOfClass:[NSButton class]])) )*/
    else if ( targetView && [targetView conformsToProtocol:@protocol(JHWidget)] )
    {
        /* clicked on an object; button/field */
        
        /* unless we're Shift-key multiple selecting; deselect all other objects */
        if (!([NSEvent modifierFlags] & NSShiftKeyMask))
            [self _deselectAll];
        
        /* select the object */
        JHSelectorView *selector = [self _selectWidget:(NSView<JHWidget>*)targetView withoutNotification:NO];
        
        /* determine if we're attempting to resize the object */
        dragIsResize = ( [selector mouseWithinResizeHandle:
                          [selector convertPoint:[theEvent locationInWindow] fromView:nil]] );
        
    }
    else
    {
        /* clicked on card; deselect all objects */
        [self _deselectAll];
    }
}




- (void)mouseDragged:(NSEvent *)theEvent
{
    if (stackmgr_script_running(stack)) return;
    
    
    /* check for painting */
    if (_paint_subsys)
    {
        NSPoint mouse_loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
        paint_event_mouse_dragged(_paint_subsys, mouse_loc.x, mouse_loc.y);
        return;
    }
    
    
    /* check if we're in layout mode;
     if we're not, then just pass the event to the appropriate target object */
    if (currentTool == kJHToolBrowse)
    {
        //if ([target_view respondsToSelector:@selector(mouseDragged:)])
        //    [target_view mouseDragged:theEvent];
        return;
    }
    
    
    if ([dragTargets count] == 0) return;
    
    did_drag = YES;
    
    NSPoint dragCurrent = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    JHSelectorView *obj;
    NSRect objFrame;
    
    long i,c = [dragTargets count];
    for (i = 0; i < c; i += 2)
    {
        obj = [dragTargets objectAtIndex:i];
        objFrame = NSRectFromString([dragTargets objectAtIndex:i+1]);
        
        if (dragIsResize)
        {
            objFrame.size.width += (int)dragCurrent.x - (int)dragStart.x + (kJHFrameOffset * 2);
            objFrame.size.height += (int)dragCurrent.y - (int)dragStart.y + (kJHFrameOffset * 2);
            
            [obj setNewSize:objFrame.size];
        }
        else
        {
            objFrame.origin.x += (int)dragCurrent.x - (int)dragStart.x;
            objFrame.origin.y += (int)dragCurrent.y - (int)dragStart.y;
            
            [obj setNewOrigin:objFrame.origin];
        }
    }
    
    [self _snapSelectionToGuides];
}


- (void)mouseUp:(NSEvent *)theEvent
{
    if (stackmgr_script_running(stack)) return;
    
    
    /* check for painting */
    if (_paint_subsys)
    {
        NSPoint mouse_loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
        paint_event_mouse_up(_paint_subsys, mouse_loc.x, mouse_loc.y);
        if ([theEvent clickCount] > 1)
            paint_event_escape(_paint_subsys);
        return;
    }
    
    
    /* check if we're in layout mode;
     if we're not, then just pass the event to the appropriate target object */
    if (currentTool == kJHToolBrowse)
    {
        //if ([target_view respondsToSelector:@selector(mouseUp:)])
        //    [target_view mouseUp:theEvent];
        return;
    }
    
    
    JHSelectorView *selector;
    NSView<JHWidget>* widget_view;
    
    snap_line_x = -1;
    snap_line_y = -1;
    
    if (!did_drag)
    {
        if ([theEvent clickCount] == 2)
        {
            if ([theEvent modifierFlags] & NSCommandKeyMask) [self editScript];
            else [NSApp sendAction:@selector(showProperties:) to:nil from:self];
        }
        return;
    }
    
    long i, c = [dragTargets count];
    for (i = 0; i < c; i += 2)
    {
        selector = (JHSelectorView*)[dragTargets objectAtIndex:i];
        widget_view = selector.widget;
        
        [dragTargets setObject:NSStringFromRect(widget_view.frame) atIndexedSubscript:i+1];
        
        if (dragIsResize)
            stack_undo_activity_begin(stack, "Resize", stackmgr_current_card_id(stack));
        else
            stack_undo_activity_begin(stack, "Move", stackmgr_current_card_id(stack));
        stack_widget_set_rect(stack, widget_view.widget_id, widget_view.frame.origin.x, widget_view.frame.origin.y,
                              widget_view.frame.size.width, widget_view.frame.size.height);
        stack_undo_activity_end(stack);
    }
    

    [self setNeedsDisplay:YES];
    [_layer_overlay setNeedsDisplay:YES];
    
}


- (void) keyDown: (NSEvent *) event
{
    if (stackmgr_script_running(stack)) return;
    
    NSString *chars = [event characters];
    unichar character = [chars characterAtIndex: 0];
    
    if (character == NSDeleteCharacter) {
        [self delete:self];
    }
    if (character == NSTabCharacter)
    {
        [self _handleTab:event];
    }
    else if (character == NSBackTabCharacter) // tab
    {
        [self _handleBackTab:event];
    }
    else
    {
        [[JHMessagePalette sharedController] handleUnfocusedKeyDown:event];
    }
}





@end
