//
//  JHCardView+LayoutSelection.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView.h"

@implementation JHCardView (LayoutSelection)


/************
 Selection
 */



- (void)_selectedWidgetsAsCArray:(long**)out_widgets ofSize:(int*)out_size
{
    *out_size = (int)([dragTargets count] / 2);
    *out_widgets = malloc(sizeof(long) * *out_size);
    if (!*out_widgets)
    {
        *out_size = 0;
        return;
    }
    int w = 0;
    for (long i = 0; i < [dragTargets count]; i += 2)
    {
        JHSelectorView *selector = [dragTargets objectAtIndex:i];
        long the_id = [[selector widget] widget_id];
        (*out_widgets)[w++] = the_id;
    }
}


- (void)_deselectAll
{
    [dragTargets removeAllObjects];
    NSArray *children = [self subviews];
    NSView *child;
    long i, c = [children count];
    for (i = c-1; i >= 0; i--)
    {
        child = [children objectAtIndex:i];
        if ([child isKindOfClass:[JHSelectorView class]])
        {
            [((JHSelectorView*)child) widget].is_selected = NO;
            [child removeFromSuperview];
        }
    }
    [self setNeedsDisplay:YES];
    
    if (edit_bkgnd) selection_type = SELECTION_BKGND;
    else selection_type = SELECTION_CARD;
    
    if (!quiet_selection_notifications)
        [[NSNotificationCenter defaultCenter] postNotification:
         [NSNotification notificationWithName:@"DesignSelectionChange" object:self]];
    
    
}


- (JHSelectorView*)_selectWidget:(NSView<JHWidget>*)inWidget withoutNotification:(BOOL)in_quiet
{
    NSRect selectorFrame = NSMakeRect(inWidget.frame.origin.x - kJHFrameOffset, inWidget.frame.origin.y - kJHFrameOffset,
                                      inWidget.frame.size.width + kJHFrameOffset*2, inWidget.frame.size.height + kJHFrameOffset*2);
    JHSelectorView *selector = [NSKeyedUnarchiver unarchiveObjectWithData:
                                [NSKeyedArchiver archivedDataWithRootObject:templateWidgetSelector]];
    [selector setFrame:selectorFrame];
    ((JHSelectorView*)selector).widget = inWidget;
    inWidget.is_selected = YES;
    [self addSubview:selector];
    
    [dragTargets addObject:selector];
    [dragTargets addObject:NSStringFromRect(inWidget.frame)];
    
    selection_type = SELECTION_WIDGETS;
    
    if (!in_quiet)
        [[NSNotificationCenter defaultCenter] postNotification:
         [NSNotification notificationWithName:@"DesignSelectionChange" object:self]];
    
    //NSLog(@"selected widget with bounds: %f,%f,%f,%f", inWidget.frame.origin.x, inWidget.frame.origin.y,
    //      inWidget.frame.origin.x + inWidget.frame.size.width, inWidget.frame.origin.y + inWidget.frame.size.height);
    
    return selector;
}


- (void)_deselectWidget:(NSView<JHWidget>*)inWidget
{
    for (NSView* selector in self.subviews)
    {
        if ( ([selector isKindOfClass:[JHSelectorView class]]) && (((JHSelectorView*)selector).widget == inWidget) )
        {
            [selector removeFromSuperview];
            [dragTargets removeObjectAtIndex:[dragTargets indexOfObject:selector] + 1];
            [dragTargets removeObject:selector];
            
            if (dragTargets.count == 0)
            {
                if (edit_bkgnd) selection_type = SELECTION_BKGND;
                else selection_type = SELECTION_CARD;
            }
            
            inWidget.is_selected = NO;
            
            if (!quiet_selection_notifications)
                [[NSNotificationCenter defaultCenter] postNotification:
                 [NSNotification notificationWithName:@"DesignSelectionChange" object:self]];
            return;
        }
    }
}


- (void)_saveSelection
{
    saved_selection = [NSMutableArray array];
    for (long i = 0; i < [dragTargets count]; i += 2)
    {
        JHSelectorView *selector = [dragTargets objectAtIndex:i];
        [saved_selection addObject:[NSNumber numberWithLong:[[selector widget] widget_id]]];
    }
    saved_selection_type = selection_type;
}


- (void)_restoreSelection
{
    [self _deselectAll];
    for (long i = 0; i < [saved_selection count]; i++)
    {
        long widget_id = [[saved_selection objectAtIndex:i] longValue];
        for (NSView *view in self.subviews)
        {
            if ([view conformsToProtocol:@protocol(JHWidget)])
            {
                /*for (NSView<JHWidget> *widget_view in self.subviews)
                 {*/
                NSView<JHWidget> *widget_view = (NSView<JHWidget>*)view;
                //if ([widget_view isKindOfClass:[JHSelectorView class]]) continue;
                if ([widget_view widget_id] == widget_id)
                {
                    [self _selectWidget:widget_view withoutNotification:YES];
                    break;
                }
            }
        }
    }
    selection_type = saved_selection_type;
    if (!quiet_selection_notifications)
        [[NSNotificationCenter defaultCenter] postNotification:
         [NSNotification notificationWithName:@"DesignSelectionChange" object:self]];
}


- (void)_snapSelectionToGuides
{
    NSMutableSet *x_edges_and_axes = [NSMutableSet set];
    NSMutableSet *y_edges_and_axes = [NSMutableSet set];
    const long SNAP_THRESHOLD = 10;
    
    /* iterate over all widgets outside the selection */
    for (NSView *view in self.subviews)
    {
        if ([view conformsToProtocol:@protocol(JHWidget)])
        {
            /*
             if (![view isKindOfClass:[JHSelectorView class]])
             {*/
            NSView<JHWidget> *widget = (NSView<JHWidget>*)view;
            if (!widget.is_selected)
            {
                /* append the edges and relevant axes of the widget to the set */
                [x_edges_and_axes addObject:[NSNumber numberWithLong:widget.frame.origin.x]];
                [x_edges_and_axes addObject:[NSNumber numberWithLong:widget.frame.origin.x + widget.frame.size.width]];
                [y_edges_and_axes addObject:[NSNumber numberWithLong:widget.frame.origin.y]];
                [y_edges_and_axes addObject:[NSNumber numberWithLong:widget.frame.origin.y + widget.frame.size.height]];
                //[x_edges_and_axes addObject:[NSNumber numberWithLong:widget.frame.origin.x + (widget.frame.size.width / 2)]];
                //[y_edges_and_axes addObject:[NSNumber numberWithLong:widget.frame.origin.y + (widget.frame.size.height / 2)]];
            }
        }
    }
    
    /* iterate over x edges and look for members of the selection that should snap */
    long shouldSnapXBy = -1;
    long snapXCoord = -1;
    long i, c = [dragTargets count];
    for (NSNumber *coord in x_edges_and_axes)
    {
        long coord_x = [coord longValue];
        for (i = 0; i < c; i += 2)
        {
            JHSelectorView *obj = [dragTargets objectAtIndex:i];
            NSView<JHWidget> *widget = [obj widget];
            
            int deltaLeft = widget.frame.origin.x - coord_x;
            int deltaRight = widget.frame.origin.x + widget.frame.size.width - coord_x;
            long deltaCenter = 99;//abs( widget.frame.origin.x + (widget.frame.size.width / 2) - coord_x );
            
            if ((abs(deltaLeft) <= SNAP_THRESHOLD) && ((abs(deltaLeft) < shouldSnapXBy) || (shouldSnapXBy < 0)))
            {
                if (!dragIsResize)
                {
                    shouldSnapXBy = -deltaLeft;
                    snapXCoord = coord_x;
                }
            }
            if ((abs(deltaRight) <= SNAP_THRESHOLD) && ((abs(deltaRight) < shouldSnapXBy) || (shouldSnapXBy < 0)))
            {
                shouldSnapXBy = -deltaRight;
                snapXCoord = coord_x;
            }
            if ((deltaCenter <= SNAP_THRESHOLD) && ((deltaCenter < shouldSnapXBy) || (shouldSnapXBy < 0)))
            {
                shouldSnapXBy = deltaCenter;
                snapXCoord = coord_x;
            }
            if (shouldSnapXBy == 0) break;
        }
        if (shouldSnapXBy == 0) break;
    }
    
    /* iterate over y edges and look for members of the selection that should snap */
    long shouldSnapYBy = -1;
    long snapYCoord = -1;
    for (NSNumber *coord in y_edges_and_axes)
    {
        long coord_y = [coord longValue];
        for (i = 0; i < c; i += 2)
        {
            JHSelectorView *obj = [dragTargets objectAtIndex:i];
            NSView<JHWidget> *widget = [obj widget];
            
            int deltaTop = widget.frame.origin.y - coord_y;
            int deltaBottom = widget.frame.origin.y + widget.frame.size.height - coord_y;
            long deltaMiddle = 99;//abs( widget.frame.origin.y + (widget.frame.size.height / 2) - coord_y );
            
            if ((abs(deltaTop) <= SNAP_THRESHOLD) && ((abs(deltaTop) < shouldSnapYBy) || (shouldSnapYBy < 0)))
            {
                if (!dragIsResize)
                {
                    shouldSnapYBy = -deltaTop;
                    snapYCoord = coord_y;
                }
            }
            if ((abs(deltaBottom) <= SNAP_THRESHOLD) && ((abs(deltaBottom) < shouldSnapYBy) || (shouldSnapYBy < 0)))
            {
                shouldSnapYBy = -deltaBottom;
                snapYCoord = coord_y;
            }
            if ((deltaMiddle <= SNAP_THRESHOLD) && ((deltaMiddle < shouldSnapYBy) || (shouldSnapYBy < 0)))
            {
                shouldSnapYBy = deltaMiddle;
                snapYCoord = coord_y;
            }
            if (shouldSnapYBy == 0) break;
        }
        if (shouldSnapYBy == 0) break;
    }
    
    /* draw snap lines */
    if (snapXCoord >= 0)
    {
        //NSLog(@"Snap to X: %ld by: %ld", snapXCoord, shouldSnapXBy);
        snap_line_x = snapXCoord;
    }
    else snap_line_x = -1;
    if (snapYCoord >= 0)
    {
        //NSLog(@"Snap to Y: %ld by: %ld", snapYCoord, shouldSnapYBy);
        snap_line_y = snapYCoord;
    }
    else snap_line_y = -1;
    
    /* adjust the position of the selection;
     but only if the Control key isn't down */
    if (!([NSEvent modifierFlags] & NSControlKeyMask))
    {
        for (i = 0; i < c; i += 2)
        {
            JHSelectorView *obj = [dragTargets objectAtIndex:i];
            NSView<JHWidget> *widget = [obj widget];
            NSRect objFrame = widget.frame;
            
            if (dragIsResize)
            {
                objFrame.size.width += shouldSnapXBy + (kJHFrameOffset * 2);
                objFrame.size.height += shouldSnapYBy + (kJHFrameOffset * 2);
                
                [obj setNewSize:objFrame.size];
            }
            else
            {
                if (snapXCoord >= 0)
                    objFrame.origin.x += shouldSnapXBy - kJHFrameOffset;
                if (snapYCoord >= 0)
                    objFrame.origin.y += shouldSnapYBy - kJHFrameOffset;
                
                [obj setNewOrigin:objFrame.origin];
            }
        }
    }
    //[self setNeedsDisplay:YES];
    [_layer_overlay setNeedsDisplay:YES];
}


- (NSArray*)designSelectionIDs
{
    NSMutableArray *result = nil;
    
    if (!stack) return nil;
    
    if (dragTargets.count != 0)
    {
        result = [NSMutableArray arrayWithCapacity:dragTargets.count / 2];
        for (long i = 0; i < dragTargets.count; i += 2)
        {
            JHSelectorView *selector = (JHSelectorView*)[dragTargets objectAtIndex:i];
            NSView<JHWidget>* widget_view = selector.widget;
            [result addObject:[NSNumber numberWithLong:widget_view.widget_id]];
        }
    }
    else
    {
        result = [NSMutableArray arrayWithCapacity:1];
        switch (selection_type)
        {
            case SELECTION_CARD:
                [result addObject:[NSNumber numberWithLong:stackmgr_current_card_id(stack)]];
                break;
            case SELECTION_BKGND:
                [result addObject:[NSNumber numberWithLong:stackmgr_current_bkgnd_id(stack)]];
                break;
            case SELECTION_STACK:
            default:
                break;
        }
    }
    
    return result;
}


- (enum SelectionType)designSelectionType
{
    if (!stack) return SELECTION_NONE;
    return selection_type;
}




@end
