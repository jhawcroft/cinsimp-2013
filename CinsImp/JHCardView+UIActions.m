//
//  JHCardView_UIActions.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//



#import "JHCardView.h"

#import "JHWidget.h"
#import "JHSelectorView.h"

#include "tools.h"

#import "JHToolPaletteController.h"

#import "JHScriptEditorController.h"




// **TODO*** most actions in paint mode (non paint) need to call exit paint to be sure painting is ended
// prior to performing the requested action.  as such, we're going to want to fix exitPaint to include changing
// the tool, etc. etc. and mode



@implementation JHCardView (JHCardView_UIActions)


- (BOOL)pasteboardHasPicture
{
    NSPasteboard *the_pasteboard = [NSPasteboard generalPasteboard];
    NSArray *items = [the_pasteboard readObjectsForClasses:[NSArray arrayWithObjects:[NSImage class], nil]
                                                   options:NULL];
    return ([items count] == 1);
}




/************
 UI Validation
 */

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    //if (menuItem.action == @selector(devTest:)) return YES;
    
    
    /**
     Debugger
     */
    if (acu_script_is_resumable(stack))
    {
        if ( (menuItem.action == @selector(debugContinue:)) ||
            (menuItem.action == @selector(debugStepOver:)) ||
            (menuItem.action == @selector(debugStepOut:)) ||
            (menuItem.action == @selector(debugStepInto:)) )
            return YES;
    }
    if (acu_script_is_active(stack))
    {
        if (menuItem.action == @selector(debugAbort:))
            return YES;
    }
    
    
    /**
     Script Control 
     */
    if (!acu_ui_acquire_stack(self.stack)) return NO;
    
    
    
    
    /**
     Paint Mode 
     */
    if (_edit_mode == CARDVIEW_MODE_PAINT)
    {
        /* selection actions */
        if (menuItem.action == @selector(selectAll:)) return YES;
        
        /* pasteboard actions */
        if ( (menuItem.action == @selector(delete:)) ||
            (menuItem.action == @selector(cut:)) ||
            (menuItem.action == @selector(copy:)) )
            return paint_has_selection(_paint_subsys);
        
        /* options */
        if (menuItem.action == @selector(toggleFatBits:))
        {
            [menuItem setState:paint_fat_bits_get(_paint_subsys)];
            return YES;
        }
        else if (menuItem.action == @selector(toggleDrawFilled:))
        {
            [menuItem setState:paint_draw_filled_get(_paint_subsys)];
            return YES;
        }
        
        /* filters */
        if ( (menuItem.action == @selector(paintFilterDarken:)) ||
            (menuItem.action == @selector(paintFilterLighten:)) ||
            (menuItem.action == @selector(paintFilterInvert:)) ||
            (menuItem.action == @selector(paintFilterGreyscale:)) )
            return paint_has_selection(_paint_subsys);
        
        /* transforms */
        if ( (menuItem.action == @selector(paintFlipHorz:)) ||
            (menuItem.action == @selector(paintFlipVert:)) )
            return paint_has_selection(_paint_subsys);
    }
    
    /**
     Layout Mode
     */
    if (_edit_mode == CARDVIEW_MODE_LAYOUT)
    {
        if (stack_is_writable(stack))
        {
            
            if (menuItem.action == @selector(selectAll:)) return NO;
            
            if (menuItem.action == @selector(delete:))
            {
                return (dragTargets.count > 0);
            }
            else if (menuItem.action == @selector(sendForward:))
            {
                if ([dragTargets count] == 0) return NO;
                return YES;
            }
            else if (menuItem.action == @selector(sendBack:))
            {
                if ([dragTargets count] == 0) return NO;
                return YES;
            }
            else if (menuItem.action == @selector(sendToFront:))
            {
                if ([dragTargets count] == 0) return NO;
                return YES;
            }
            else if (menuItem.action == @selector(sendToBack:))
            {
                if ([dragTargets count] == 0) return NO;
                return YES;
            }
            else if ( (menuItem.action == @selector(cut:)) && stack_is_writable(stack) )
            {
                return ([dragTargets count] > 0);
            }
            else if (menuItem.action == @selector(paste:))
            {
                NSPasteboard *pb = [NSPasteboard generalPasteboard];
                return ([pb canReadItemWithDataConformingToTypes:[NSArray arrayWithObject:@"com.joshhawcroft.cinsimp.widgets"]]);
            }
            
        }
        
        if (menuItem.action == @selector(toggleShowSequence:))
        {
            [menuItem setState:show_sequence];
            return YES;
        }
        else if (menuItem.action == @selector(copy:))
        {
            return ([dragTargets count] > 0);
        }
    }
    

    /**
     General
     */
    if (stack_is_writable(stack))
    {
        /* undo/redo are not currently supported by painting tools */
        if (_edit_mode != CARDVIEW_MODE_PAINT)
        {
            if (menuItem.action == @selector(undo:))
            {
                const char* desc = stack_can_undo(stack);
                if (!desc) {
                    [menuItem setTitle:@"Undo"];
                    return NO;
                }
                [menuItem setTitle:[NSString stringWithFormat:@"Undo %s", desc]];
                return YES;
            }
            else if (menuItem.action == @selector(redo:))
            {
                const char* desc = stack_can_redo(stack);
                if (!desc)
                {
                    [menuItem setTitle:@"Redo"];
                    return NO;
                }
                [menuItem setTitle:[NSString stringWithFormat:@"Redo %s", desc]];
                return YES;
            }
        }
        
        if ( (menuItem.action == @selector(newBackground:)) ||
            (menuItem.action == @selector(newCard:)) ||
            (menuItem.action == @selector(deleteCard:)) ) return YES;
        if (menuItem.action == @selector(compactStack:)) return YES;
        if (menuItem.action == @selector(newField:)) return YES;
        if (menuItem.action == @selector(newButton:)) return YES;
        if ( (menuItem.action == @selector(showPropertiesForCard:)) ||
            (menuItem.action == @selector(showPropertiesForBkgnd:)) ||
            (menuItem.action == @selector(showPropertiesForStack:)) ) return YES;
        
        
        if (menuItem.action == @selector(paste:)) return [self pasteboardHasPicture];
    }
    
    
    

    
    if (menuItem.action == @selector(toggleEditBkgnd:))
    {
        [menuItem setState:(edit_bkgnd?NSOnState:NSOffState)];
        return YES;
    }    
    else if ( (menuItem.action == @selector(goFirstCard:)) ||
             (menuItem.action == @selector(goPreviousCard:)) ||
             (menuItem.action == @selector(goNextCard:)) ||
             (menuItem.action == @selector(goLastCard:)) )
    {
        return YES;
    }
    
    
    return NO;
    //return stack_is_writable(stack);
}




/************
 UI Actions
 */

- (IBAction)delete:(id)sender
{
    if (_edit_mode == CARDVIEW_MODE_PAINT)
    {
        paint_delete(_paint_subsys);
        return;
    }
    
    if (stack_prop_get_long(stack, 0, 0, PROPERTY_LOCKED)) return;
    
    switch (currentTool)
    {
        case kJHToolButton:
        case kJHToolField:
        {
            if (dragTargets.count == 0) return;
            
            stack_undo_activity_begin(stack, "Delete", stackmgr_current_card_id(stack));
            NSView<JHWidget> *widget_view;
            for (long i = 0; i < dragTargets.count; i += 2)
            {
                widget_view = ((JHSelectorView*)[dragTargets objectAtIndex:i]).widget;
                stack_delete_widget(stack, widget_view.widget_id);
            }
            stack_undo_activity_end(stack);
            
            [self _buildCard];
            
            break;
        }
        case kJHToolBrowse:
        default:
            break;
    }
}


- (IBAction)toggleEditBkgnd:(id)sender
{
    [self _suspendPaint];
    
    /* exit editing field; force changes to disk */
    [[self window] makeFirstResponder:self];
    
    /* deselect layout objects */
    [self _deselectAll];
    
    /* change modes and rebuild layout */
    edit_bkgnd = !edit_bkgnd;
    [self _buildCard];
    
    [self _resumePaint];
    
    /* send relevant notifications so UI can update */
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:designLayerChangeNotification object:self]];
}


- (IBAction)newButton:(id)sender
{
    const int NEW_BUTTON_WIDTH = 100;
    const int NEW_BUTTON_HEIGHT = 22;
    long widget_id;
    int err;
    
    stack_undo_activity_begin(stack, "New Button", stackmgr_current_card_id(stack));
    if (edit_bkgnd) widget_id = stack_create_widget(stack, WIDGET_BUTTON_PUSH, 0, stackmgr_current_bkgnd_id(stack), &err);
    else widget_id = stack_create_widget(stack, WIDGET_BUTTON_PUSH, stackmgr_current_card_id(stack), 0, &err);
    if (widget_id != STACK_NO_OBJECT)
    {
        stack_widget_set_rect(stack, widget_id,
                              (self.frame.size.width - NEW_BUTTON_WIDTH) / 2,
                              (self.frame.size.height - NEW_BUTTON_HEIGHT) / 2,
                              NEW_BUTTON_WIDTH,
                              NEW_BUTTON_HEIGHT);
    }
    stack_undo_activity_end(stack);
    
    if (widget_id == STACK_NO_OBJECT)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Couldn't create button."];
        if (err == STACK_ERR_TOO_MANY_WIDGETS)
            [alert setInformativeText:@"There are too many objects on this card."];
        [alert runModal];
        return;
    }
    
    [self _buildCard];
    [self chooseTool:AUTH_TOOL_BUTTON];
    [[JHToolPaletteController sharedController] setCurrentTool:AUTH_TOOL_BUTTON];
    [self _selectWidget:[self _widgetViewForID:widget_id] withoutNotification:NO];
}


- (IBAction)newField:(id)sender
{
    const int NEW_FIELD_WIDTH = 200;
    const int NEW_FIELD_HEIGHT = 85;
    long widget_id;
    int err;
    
    stack_undo_activity_begin(stack, "New Field", stackmgr_current_card_id(stack));
    if (edit_bkgnd) widget_id = stack_create_widget(stack, WIDGET_FIELD_TEXT, 0, stackmgr_current_bkgnd_id(stack), &err);
    else widget_id = stack_create_widget(stack, WIDGET_FIELD_TEXT, stackmgr_current_card_id(stack), 0, &err);
    if (widget_id != STACK_NO_OBJECT)
    {
        stack_widget_set_rect(stack, widget_id,
                              (self.frame.size.width - NEW_FIELD_WIDTH) / 2,
                              (self.frame.size.height - NEW_FIELD_HEIGHT) / 2,
                              NEW_FIELD_WIDTH,
                              NEW_FIELD_HEIGHT);
        stack_widget_prop_set_string(stack, widget_id, stackmgr_current_card_id(stack), PROPERTY_BORDER, "transparent");
    }
    stack_undo_activity_end(stack);
    
    if (widget_id == STACK_NO_OBJECT)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Couldn't create field."];
        if (err == STACK_ERR_TOO_MANY_WIDGETS)
            [alert setInformativeText:@"There are too many objects on this card."];
        [alert runModal];
        return;
    }
    
    [self _buildCard];
    [self chooseTool:AUTH_TOOL_FIELD];
    [[JHToolPaletteController sharedController] setCurrentTool:AUTH_TOOL_FIELD];
    [self _selectWidget:[self _widgetViewForID:widget_id] withoutNotification:NO];
}


- (IBAction)compactStack:(id)sender
{
    [self chooseTool:AUTH_TOOL_BROWSE];
    stack_compact(stack);
}


- (IBAction)showPropertiesForStack:(id)sender
{
    selection_type = SELECTION_STACK;
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:@"DesignSelectionChange" object:self]];
    if ([NSEvent modifierFlags] & NSCommandKeyMask)
        [self editScript];
    else
        [NSApp sendAction:@selector(showProperties:) to:nil from:self];
}

- (IBAction)showPropertiesForCard:(id)sender
{
    selection_type = SELECTION_CARD;
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:@"DesignSelectionChange" object:self]];
    if ([NSEvent modifierFlags] & NSCommandKeyMask)
        [self editScript];
    else
        [NSApp sendAction:@selector(showProperties:) to:nil from:self];
}

- (IBAction)showPropertiesForBkgnd:(id)sender
{
    selection_type = SELECTION_BKGND;
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:@"DesignSelectionChange" object:self]];
    if ([NSEvent modifierFlags] & NSCommandKeyMask)
        [self editScript];
    else
        [NSApp sendAction:@selector(showProperties:) to:nil from:self];
}


- (IBAction)sendBack:(id)sender
{
    stack_undo_activity_begin(stack, "Send Back", stackmgr_current_card_id(stack));
    long *widgets;
    int count;
    [self _selectedWidgetsAsCArray:&widgets ofSize:&count];
    if (widgets)
    {
        stack_widgets_shuffle_backward(stack, widgets, count);
        free(widgets);
    }
    [self _saveSelection];
    [self _buildCard];
    [self _restoreSelection];
    stack_undo_activity_end(stack);
}


- (IBAction)sendForward:(id)sender
{
    stack_undo_activity_begin(stack, "Send Forward", stackmgr_current_card_id(stack));
    long *widgets;
    int count;
    [self _selectedWidgetsAsCArray:&widgets ofSize:&count];
    if (widgets)
    {
        stack_widgets_shuffle_forward(stack, widgets, count);
        free(widgets);
    }
    [self _saveSelection];
    [self _buildCard];
    [self _restoreSelection];
    stack_undo_activity_end(stack);
}


- (IBAction)sendToBack:(id)sender
{
    stack_undo_activity_begin(stack, "Send to Back", stackmgr_current_card_id(stack));
    long *widgets;
    int count;
    [self _selectedWidgetsAsCArray:&widgets ofSize:&count];
    if (widgets)
    {
        stack_widgets_send_back(stack, widgets, count);
        free(widgets);
    }
    [self _saveSelection];
    [self _buildCard];
    [self _restoreSelection];
    stack_undo_activity_end(stack);
}


- (IBAction)sendToFront:(id)sender
{
    stack_undo_activity_begin(stack, "Send to Front", stackmgr_current_card_id(stack));
    long *widgets;
    int count;
    [self _selectedWidgetsAsCArray:&widgets ofSize:&count];
    if (widgets)
    {
        stack_widgets_send_front(stack, widgets, count);
        free(widgets);
    }
    [self _saveSelection];
    [self _buildCard];
    [self _restoreSelection];
    stack_undo_activity_end(stack);
}


- (IBAction)toggleShowSequence:(id)sender
{
    show_sequence = !show_sequence;
    [self setNeedsDisplay:YES];
    [_layer_overlay setNeedsDisplay:YES];
}



- (IBAction)newBackground:(id)sender
{
    [self _suspendPaint];
    
    //stack_undo_activity_begin(stack, "New Card", stackmgr_current_card_id(stack));
    int err;
    long new_card_id = stack_bkgnd_create(stack, stackmgr_current_card_id(stack), &err);
    if (new_card_id != STACK_NO_OBJECT) [self goToCard:new_card_id];
    else
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Couldn't create new background."];
        switch (err)
        {
            case STACK_ERR_TOO_MANY_CARDS:
                [alert setInformativeText:@"There are too many cards in the stack."];
                break;
            default: break;
        }
        [alert runModal];
    }
    //stack_undo_activity_end(stack);
    
    [self _resumePaint];
}



- (IBAction)newCard:(id)sender
{
    [self _suspendPaint];
    
    stack_undo_activity_begin(stack, "New Card", stackmgr_current_card_id(stack));
    int err;
    long new_card_id = stack_card_create(stack, stackmgr_current_card_id(stack), &err);
    if (new_card_id != STACK_NO_OBJECT) [self goToCard:new_card_id];
    else
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Couldn't create new card."];
        switch (err)
        {
            case STACK_ERR_TOO_MANY_CARDS:
                [alert setInformativeText:@"There are too many cards in the stack."];
                break;
            default: break;
        }
        [alert runModal];
    }
    stack_undo_activity_end(stack);
    
    [self _resumePaint];
}


- (IBAction)deleteCard:(id)sender
{
    [self _suspendPaint];
    
    stack_undo_activity_begin(stack, "Delete Card", stackmgr_current_card_id(stack));
    [[self window] makeFirstResponder:self];
    long next_card_id = stack_card_delete(stack, stackmgr_current_card_id(stack));
    if (next_card_id != stackmgr_current_card_id(stack)) [self goToCard:next_card_id];
    else
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setAlertStyle:NSCriticalAlertStyle];
        if (stack_card_count(stack) == 1)
            [alert setMessageText:@"Can't delete the last card of the stack."];
        else
            [alert setMessageText:@"Can't delete this card because it is locked."];
        [alert runModal];
    }
    stack_undo_activity_end(stack);
    
    [self _resumePaint];
}


- (IBAction)goFirstCard:(id)sender
{
    [self _suspendPaint];
    [self goToCard:stack_card_id_for_index(stack, 0)];
    [self _resumePaint];
}


- (IBAction)goPreviousCard:(id)sender
{
    [self _suspendPaint];
    card_index--;
    if (card_index < 0) card_index = stack_card_count(stack)-1;
    [self goToCard:stack_card_id_for_index(stack, card_index)];
    [self _resumePaint];
}


- (IBAction)goNextCard:(id)sender
{
    [self _suspendPaint];
    card_index++;
    if (card_index >= stack_card_count(stack)) card_index = 0;
    [self goToCard:stack_card_id_for_index(stack, card_index)];
    [self _resumePaint];
}


- (IBAction)goLastCard:(id)sender
{
    [self _suspendPaint];
    [self goToCard:stack_card_id_for_index(stack, stack_card_count(stack)-1)];
    [self _resumePaint];
}



/*********
 Pasteboard
 */

- (IBAction)copy:(id)sender
{
    if (_edit_mode == CARDVIEW_MODE_PAINT)
    {
        void const *data;
        long size;
        size = paint_selection_get_png_data(_paint_subsys, &data);
        NSData *the_data = [NSData dataWithBytesNoCopy:(void*)data length:size freeWhenDone:NO];
        
        NSPasteboard *the_pasteboard = [NSPasteboard generalPasteboard];
        [the_pasteboard clearContents];
        NSPasteboardItem *item = [[NSPasteboardItem alloc] init];
        [item setData:the_data forType:NSPasteboardTypePNG];
        [the_pasteboard writeObjects:[NSArray arrayWithObject:item]];
        
        return;
    }
    
    long *widgets;
    int count;
    [self _selectedWidgetsAsCArray:&widgets ofSize:&count];
    if (!widgets) return;
    
    void *data_ptr;
    long data_size = stack_widget_copy(stack, widgets, count, &data_ptr);
    NSData *data = [NSData dataWithBytes:data_ptr length:data_size];
    
    free(widgets);
    
    NSPasteboardItem *item = [[NSPasteboardItem alloc] init];
    [item setData:data forType:@"com.joshhawcroft.cinsimp.widgets"];
    
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    [pb clearContents];
    [pb writeObjects:[NSArray arrayWithObject:item]];
}


- (IBAction)paste:(id)sender
{
    NSPasteboard *the_pasteboard = [NSPasteboard generalPasteboard];
    NSArray *items = nil;
    
    /* look for images */
    items = [the_pasteboard readObjectsForClasses:[NSArray arrayWithObjects:[NSImage class], nil]
                                                   options:NULL];
    if ([items count] > 0)
    {
        /* change to paint mode and SELECT tool */
        [self chooseTool:PAINT_TOOL_SELECT];
        
        /* get the image */
        NSImage *image = [items lastObject];
        NSData *the_data = [image TIFFRepresentation];
        
        /* paste the image */
        if (the_data)
            paint_paste(_paint_subsys, (void*)[the_data bytes], [the_data length]);
        return;
    }
    
    /* look for stack objects */
    items = [the_pasteboard readObjectsForClasses:[NSArray arrayWithObject:[NSPasteboardItem class]] options:nil];
    for (NSPasteboardItem *item in items)
    {
        NSData *data = [item dataForType:@"com.joshhawcroft.cinsimp.widgets"];
        if (data != nil)
        {
            stack_undo_activity_begin(stack, "Paste", stackmgr_current_card_id(stack));
            
            long *widgets;
            int count = stack_widget_paste(stack, (edit_bkgnd?0:stackmgr_current_card_id(stack)),
                                           (edit_bkgnd?stackmgr_current_bkgnd_id(stack):0),
                                           (void*)[data bytes], [data length], &widgets);
            
            if (count > 0)
            {
                if (stack_widget_is_field(stack, widgets[0]))
                    [self chooseTool:kJHToolField];
                else
                    [self chooseTool:kJHToolButton];
                
                saved_selection = [NSMutableArray arrayWithCapacity:count];
                for (int i = 0; i < count; i++)
                    [saved_selection addObject:[NSNumber numberWithLong:widgets[i]]];
                [self _buildCard];
                [self _restoreSelection];
            }
            
            stack_undo_activity_end(stack);
            
            return;
        }
    }
    
}


- (IBAction)cut:(id)sender
{
    if (_edit_mode == CARDVIEW_MODE_PAINT)
    {
        void const *data;
        long size;
        size = paint_selection_get_png_data(_paint_subsys, &data);
        NSData *the_data = [NSData dataWithBytesNoCopy:(void*)data length:size freeWhenDone:NO];
        
        NSPasteboard *the_pasteboard = [NSPasteboard generalPasteboard];
        [the_pasteboard clearContents];
        NSPasteboardItem *item = [[NSPasteboardItem alloc] init];
        [item setData:the_data forType:NSPasteboardTypePNG];
        [the_pasteboard writeObjects:[NSArray arrayWithObject:item]];
        
        paint_delete(_paint_subsys);
        return;
    }
    
    
    stack_undo_activity_begin(stack, "Cut", stackmgr_current_card_id(stack));
    
    long *widgets;
    int count;
    [self _selectedWidgetsAsCArray:&widgets ofSize:&count];
    if (!widgets) return;
    
    void *data_ptr;
    long data_size = stack_widget_cut(stack, widgets, count, &data_ptr);
    NSData *data = [NSData dataWithBytes:data_ptr length:data_size];
    
    free(widgets);
    
    NSPasteboardItem *item = [[NSPasteboardItem alloc] init];
    [item setData:data forType:@"com.joshhawcroft.cinsimp.widgets"];
    
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    [pb clearContents];
    [pb writeObjects:[NSArray arrayWithObject:item]];
    
    [self _buildCard];
    stack_undo_activity_end(stack);
}



- (IBAction)undo:(id)sender
{
    long new_card_id = stackmgr_current_card_id(stack);
    stack_undo(stack, &new_card_id);
    if ((new_card_id != stackmgr_current_card_id(stack)) && (new_card_id != 0))
        [self goToCard:new_card_id];
    else
        [self _buildCard];
}


- (IBAction)redo:(id)sender
{
    long new_card_id = stackmgr_current_card_id(stack);
    stack_redo(stack, &new_card_id);
    if ((new_card_id != stackmgr_current_card_id(stack)) && (new_card_id != 0))
        [self goToCard:new_card_id];
    else
        [self _buildCard];
}


- (IBAction)selectAll:(id)sender
{
    if (_paint_subsys) paint_select_all(_paint_subsys);
    
}



- (IBAction)toggleFatBits:(id)sender
{
    if (_paint_subsys) paint_fat_bits_set(_paint_subsys, !paint_fat_bits_get(_paint_subsys));
}

- (IBAction)toggleDrawFilled:(id)sender
{
    if (_paint_subsys) paint_draw_filled_set(_paint_subsys, !paint_draw_filled_get(_paint_subsys));
    [[JHToolPaletteController sharedController] setDrawFilled:paint_draw_filled_get(_paint_subsys)];
}



- (IBAction)paintFilterInvert:(id)sender
{
    if (_paint_subsys) paint_apply_filter(_paint_subsys, PAINT_FILTER_INVERT);
}

- (IBAction)paintFilterLighten:(id)sender
{
    if (_paint_subsys) paint_apply_filter(_paint_subsys, PAINT_FILTER_LIGHTEN);
}

- (IBAction)paintFilterDarken:(id)sender
{
    if (_paint_subsys) paint_apply_filter(_paint_subsys, PAINT_FILTER_DARKEN);
}

- (IBAction)paintFilterGreyscale:(id)sender
{
    if (_paint_subsys) paint_apply_filter(_paint_subsys, PAINT_FILTER_GREYSCALE);
}


- (IBAction)paintFlipHorz:(id)sender
{
    if (_paint_subsys) paint_flip_horizontal(_paint_subsys);
}


- (IBAction)paintFlipVert:(id)sender
{
    if (_paint_subsys) paint_flip_vertical(_paint_subsys);
}


/************
 Scripting
 */

- (void)_invalidateScriptCloseDelay
{
    if (_script_debug_close_timer)
    {
        [_script_debug_close_timer invalidate];
        _script_debug_close_timer = nil;
    }
}


- (void)openScriptEditorFor:(StackHandle)in_edited_object atSourceLine:(long)in_source_line debugging:(BOOL)in_debugging
{
    /* invalidate script close delay */
    [self _invalidateScriptCloseDelay];
    
    /* check if the script of the specified object is already being edited */
    for (JHScriptEditorController *controller in _open_scripts)
    {
        if (stackmgr_handles_equivalent([controller handle], in_edited_object))
        {
            /* bring the existing script editor to the front */
            [[controller window] makeKeyAndOrderFront:self];
            
            /* set the debug line */
            if (in_debugging)
            {
                if (in_source_line > 0) [controller setSelectedLine:in_source_line debugging:in_debugging];
            }
            return;
        }
    }
    
    /* create a new script editor */
    JHScriptEditorController *controller = [[JHScriptEditorController alloc] initWithStackHandle:in_edited_object];
    
    /* keep a register of open script editors */
    if (controller != nil) [_open_scripts addObject:controller];
    
    /* set the selected line */
    if (in_source_line > 0) [controller setSelectedLine:in_source_line debugging:in_debugging];
}


- (void)_closeScriptEditorsNow:(NSTimer*)in_timer
{
    [self _invalidateScriptCloseDelay];
    
    while ([_open_scripts count] > 0)
    {
        JHScriptEditorController *controller = [_open_scripts lastObject];
        NSLog(@"%@\n", [[controller window] title]);
        [controller close];
        [_open_scripts removeObject:controller];
    }
}


#define SCRIPT_DEBUG_CLOSE_DELAY 0.2


- (void)closeScriptEditorsWithDelay:(BOOL)in_with_delay
{
    /* invalidate any existing delay */
    [self _invalidateScriptCloseDelay];
    
    /* perform either close immediately, or close after a short delay */
    if (in_with_delay)
        _script_debug_close_timer = [NSTimer scheduledTimerWithTimeInterval:SCRIPT_DEBUG_CLOSE_DELAY
                                                                     target:self
                                                                   selector:@selector(_closeScriptEditorsNow:)
                                                                   userInfo:nil
                                                                    repeats:NO];
    else
        [self _closeScriptEditorsNow:nil];
}


- (void)editScript
{
    /* get a handle for the selected object */
    StackHandle edited_object = STACKMGR_INVALID_HANDLE;
    switch (selection_type)
    {
        case SELECTION_STACK:
            edited_object = (StackHandle)stack;
            break;
        case SELECTION_BKGND:
            edited_object = stackmgr_handle_for_bkgnd_id(STACKMGR_FLAG_AUTO_RELEASE, (StackHandle)stack, stackmgr_current_bkgnd_id(stack));
            break;
        case SELECTION_CARD:
            edited_object = acu_handle_for_card_id(STACKMGR_FLAG_AUTO_RELEASE, (StackHandle)stack, stackmgr_current_card_id(stack));
            break;
        case SELECTION_WIDGETS:
        {
            if ([dragTargets count] / 2 != 1) return;
            NSNumber *selection_id = [[self designSelectionIDs] lastObject];
            StackHandle owner = STACKMGR_INVALID_HANDLE;
            if (stack_widget_is_card(stack, [selection_id longValue]))
                owner = acu_handle_for_card_id(STACKMGR_FLAG_AUTO_RELEASE, (StackHandle)stack, stackmgr_current_card_id(stack));
            else
                owner = stackmgr_handle_for_bkgnd_id(STACKMGR_FLAG_AUTO_RELEASE, (StackHandle)stack, stackmgr_current_bkgnd_id(stack));
            if (stack_widget_is_field(stack, [selection_id longValue]))
                edited_object = acu_handle_for_field_id(STACKMGR_FLAG_AUTO_RELEASE, owner, [selection_id longValue], ACU_INVALID_HANDLE);
            else
                edited_object = acu_handle_for_button_id(STACKMGR_FLAG_AUTO_RELEASE, owner, [selection_id longValue]);
            break;
        }
        case SELECTION_NONE:
            break;
    }
    if (edited_object == STACKMGR_INVALID_HANDLE) return;
    
    [self openScriptEditorFor:edited_object atSourceLine:0 debugging:NO];
}


- (IBAction)debugContinue:(id)sender
{
    acu_script_continue(stack);
}

- (IBAction)debugStepOver:(id)sender
{
    acu_debug_step_over(stack);
}

- (IBAction)debugStepOut:(id)sender
{
    acu_debug_step_out(stack);
}

- (IBAction)debugStepInto:(id)sender
{
    acu_debug_step_into(stack);
}


- (IBAction)debugAbort:(id)sender
{
    acu_script_abort(stack);
}




@end
