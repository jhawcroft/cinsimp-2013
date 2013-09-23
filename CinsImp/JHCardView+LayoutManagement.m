//
//  JHCardView+LayoutManagement.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView.h"

#import "JHLayerView.h"
#import "JHSelectorView.h"
#import "JHWidget.h"
#import "JHPushButton.h"
#import "JHTransparentButton.h"
#import "JHTextFieldScroller.h"
#import "JHTextField.h"
#import "JHCheckBox.h"
#import "JHPicklist.h"

#import "JHProgressView.h"

#include "cstritems.h"
#include "tools.h"


@implementation JHCardView (LayoutManagement)



/************
 Targeting
 */

- (NSView*)_hitTarget:(NSPoint)aPoint
{
    NSArray *children = [self subviews];
    NSView *child;
    long i, count = [children count];
    ///NSLog(@"hit testing... (%ld) ", count);
    for (i = count-1; i >= 0; i--)
    {
        child = [children objectAtIndex:i];
        if ([child isKindOfClass:[JHLayerView class]]) continue;
        //NSLog(@"ht %@", child);
        
        if (NSPointInRect(aPoint, [child frame]))
        {
            if (![child isKindOfClass:[JHSelectorView class]])
            {
                switch (currentTool)
                {
                    case kJHToolButton:
                        if ( (![child isKindOfClass:[JHPushButton class]]) &&
                            (![child isKindOfClass:[JHTransparentButton class]]) ) continue;
                        break;
                    case kJHToolField:
                        if ( (![child isKindOfClass:[JHTextFieldScroller class]]) &&
                            (![child isKindOfClass:[JHCheckBox class]]) &&
                            (![child isKindOfClass:[JHPicklist class]]) ) continue;
                        break;
                    case kJHToolBrowse:
                        if ([child conformsToProtocol:@protocol(JHWidget)])
                        {
                            //NSLog(@"got widget");
                            return child;
                            return [(NSView<JHWidget>*)child eventTargetBrowse];
                        }
                        
                    default:
                        break;
                }
            }
            //if ([child isKindOfClass:[JHSelectorView class]])
            //    return self;
            //NSLog(@"found object %@\n", child);
            return child;
        }
    }
    return self;
}



- (NSView *)hitTest:(NSPoint)aPoint
{
    if (peek_btns || peek_flds) return self;
    
    if (currentTool == AUTH_TOOL_BROWSE)
    {
        // NSLog(@"hit test card view");
        return [super hitTest:aPoint];
        // return [self hitTarget:aPoint];
    }
    //    return [super hitTest:aPoint];
    return self;
}




/************
 Utility
 */

- (NSView<JHWidget>*)_widgetViewForID:(long)inID
{
    for (NSView *view in self.subviews)
    {
        if ([view conformsToProtocol:@protocol(JHWidget)])
        {
            /*for (NSView<JHWidget>* widget_view in self.subviews)
             {*/
            NSView<JHWidget> *widget_view = (NSView<JHWidget>*)view;
            if (widget_view.widget_id == inID) return widget_view;
        }
    }
    return nil;
}




/************
 Card Construction
 */

- (void)buildWidget:(long)in_widget_id
{
    long x,y,width,height;
    int hidden;
    enum Widget type;
    char *formatted, *searchable;
    long formatted_size;
    int editable;
    
    NSView<JHWidget> *widget_view;
    
    long current_card_id = stackmgr_current_card_id(stack);
    
    if (!stack_widget_basics(stack, in_widget_id, &x, &y, &width, &height, &hidden, &type)) return;
    
    switch (type)
    {
        case WIDGET_BUTTON_PUSH:
        {
            widget_view = [NSKeyedUnarchiver unarchiveObjectWithData:[NSKeyedArchiver archivedDataWithRootObject:templateWidgetPushButton]];
            
            ((NSButton*)widget_view).title = [NSString stringWithCString:
                                              stack_widget_prop_get_string(stack, in_widget_id, stackmgr_current_card_id(stack), PROPERTY_NAME) encoding:NSUTF8StringEncoding];
            
            /* load icon, if any */
            int icon_id = (int)stack_widget_prop_get_long(stack, in_widget_id, stackmgr_current_card_id(stack), PROPERTY_ICON);
            if (icon_id != 0)
            {
                void *data_ptr;
                long data_size;
                acu_icon_data(stack, icon_id, &data_ptr, &data_size);
                
                NSData *data = [[NSData alloc] initWithBytes:data_ptr length:data_size];
                NSImage *icon = [[NSImage alloc] initWithData:data];
                
                [((NSButton*)widget_view) setImage:icon];
            }
            
            break;
        }
        case WIDGET_BUTTON_TRANSPARENT:
            widget_view = [NSKeyedUnarchiver unarchiveObjectWithData:[NSKeyedArchiver archivedDataWithRootObject:templateWidgetClearButton]];
            
            break;
        case WIDGET_FIELD_TEXT:
        {
            widget_view = [NSKeyedUnarchiver unarchiveObjectWithData:[NSKeyedArchiver archivedDataWithRootObject:templateWidgetTextSimple]];
            
            
            [(JHTextFieldScroller*)widget_view setBorder:[[NSString stringWithCString:
                                                           stack_widget_prop_get_string(stack, in_widget_id, stackmgr_current_card_id(stack), PROPERTY_BORDER)
                                                                             encoding:NSUTF8StringEncoding] lowercaseString]];
            
            JHTextField *text_field = [(JHTextFieldScroller*)widget_view documentView];
            char const *text_style_string = stack_widget_prop_get_string(stack, in_widget_id, current_card_id, PROPERTY_TEXTSTYLE);
            BOOL text_bold = cstr_has_item(text_style_string, "bold");
            BOOL text_italic = cstr_has_item(text_style_string, "italic");
            [text_field setRichText:stack_widget_prop_get_long(stack, in_widget_id, stackmgr_current_card_id(stack), PROPERTY_RICHTEXT)];
            NSString *font_name = [NSString stringWithCString:stack_widget_prop_get_string(stack, in_widget_id,
                                                                                           current_card_id, PROPERTY_TEXTFONT)
                                                     encoding:NSUTF8StringEncoding];
            int font_size = (int)stack_widget_prop_get_long(stack, in_widget_id, current_card_id, PROPERTY_TEXTSIZE);
            NSFont *text_font = [[NSFontManager sharedFontManager]
                                 fontWithFamily:font_name
                                 traits:( (text_bold ? NSBoldFontMask : 0) | (text_italic ? NSItalicFontMask : 0) )
                                 weight:5
                                 size:font_size];
            if (text_font != nil)
                [text_field setFont:text_font];
            
            [text_field setContinuousSpellCheckingEnabled:NO];
            
            
            stack_widget_content_get(stack, in_widget_id, stackmgr_current_card_id(stack), edit_bkgnd, &searchable, &formatted, &formatted_size, &editable);
            [widget_view setContent:[NSData dataWithBytesNoCopy:formatted length:formatted_size freeWhenDone:NO] searchable:
             [NSString stringWithCString:searchable encoding:NSUTF8StringEncoding] editable:editable];
            
            if (!stack_widget_prop_get_long(stack, in_widget_id, stackmgr_current_card_id(stack), PROPERTY_RICHTEXT))
            {
                if (text_font != nil)
                    [text_field setFont:text_font];
            }
            else
            {
                
            }
            
            break;
        }
        case WIDGET_FIELD_CHECK:
            widget_view = [NSKeyedUnarchiver unarchiveObjectWithData:[NSKeyedArchiver archivedDataWithRootObject:templateWidgetCheckbox]];
            
            ((NSButton*)widget_view).title = [NSString stringWithCString:
                                              stack_widget_prop_get_string(stack, in_widget_id, stackmgr_current_card_id(stack), PROPERTY_NAME) encoding:NSUTF8StringEncoding];
            stack_widget_content_get(stack, in_widget_id, stackmgr_current_card_id(stack), edit_bkgnd, &searchable, &formatted, &formatted_size, &editable);
            [widget_view setContent:[NSData dataWithBytesNoCopy:formatted length:formatted_size freeWhenDone:NO] searchable:
             [NSString stringWithCString:searchable encoding:NSUTF8StringEncoding] editable:editable];
            [((NSButton*)widget_view) setAction:@selector(didChangeState:)];
            [((NSButton*)widget_view) setTarget:widget_view];
            
            break;
        case WIDGET_FIELD_PICKLIST:
            widget_view = [NSKeyedUnarchiver unarchiveObjectWithData:[NSKeyedArchiver archivedDataWithRootObject:templateWidgetComboBox]];
            break;
        case WIDGET_FIELD_GRID:
            break;
    }
    
    widget_view.stack = stack;
    widget_view.widget_id = in_widget_id;
    widget_view.current_card_id = stackmgr_current_card_id(stack);
    widget_view.card_view = self;
    widget_view.is_card = stack_widget_is_card(stack, in_widget_id);
    
    [self addSubview:widget_view];
    [widget_view setFrame:NSMakeRect(x, y, width, height)];
}


- (void)_destroyLayout
{
    layer_card = nil;
    layer_bkgnd = nil;
    [[self window] makeFirstResponder:self];
    [self _deselectAll];
    [[self subviews] makeObjectsPerformSelector:@selector(removeFromSuperview)];
}


- (void)_buildCard
{
    if (!stack) return;
    assert(stack != NULL);
    
    
    long index, count;
    long widget_id;
    
    /* does the stack still required password authorisation?
     if yes, don't construct a card */
    if (protected_hidden) return;
    
    /* destroy any existing card layout */
    [self _destroyLayout];
    
    /* check edit mode */
    if (_edit_mode == CARDVIEW_MODE_PAINT)
    {
        
        return;
    }
    
    // target_view = nil;
    
    /* configure card size */
    long cardWidth, cardHeight;
    stack_get_card_size(stack, &cardWidth, &cardHeight);
    [self setFrame:NSMakeRect(0, 0, cardWidth, cardHeight)];
    
    /* add bkgnd picture, if any */
    // decode and construct NSImage here
    
    void *picture_data;
    long picture_data_size;
    int visible;
    picture_data_size = stack_layer_picture_get(stack, STACK_NO_OBJECT, stackmgr_current_bkgnd_id(stack), &picture_data, &visible);
    NSData *data = [[NSData alloc] initWithBytes:picture_data length:picture_data_size];
    NSBitmapImageRep * bitmap = [NSBitmapImageRep imageRepWithData:data];
    NSImage *layer_picture = [[NSImage alloc] init];
    [layer_picture addRepresentation:bitmap];
    JHLayerView *layer_view = [[JHLayerView alloc] initWithFrame:self.bounds picture:layer_picture isBackground:YES];
    [self addSubview:layer_view];
    layer_bkgnd = layer_view;
    
    /* build bkgnd widget layout */
    count = stack_widget_count(stack, 0, stackmgr_current_bkgnd_id(stack));
    for (index = 0; index < count; index++)
    {
        widget_id = stack_widget_n(stack, 0, stackmgr_current_bkgnd_id(stack), index);
        [self buildWidget:widget_id];
    }
    
    /* are we in edit background mode?
     if we are, we don't continue to build the card */
    if (!edit_bkgnd)
    {
    
        /* add card picture, if any */
        picture_data_size = stack_layer_picture_get(stack, stackmgr_current_card_id(stack), STACK_NO_OBJECT, &picture_data, &visible);
        data = [[NSData alloc] initWithBytes:picture_data length:picture_data_size];
        bitmap = [NSBitmapImageRep imageRepWithData:data];
        layer_picture = [[NSImage alloc] init];
        [layer_picture addRepresentation:bitmap];
        layer_view = [[JHLayerView alloc] initWithFrame:self.bounds picture:layer_picture isBackground:NO];
        [self addSubview:layer_view];
        layer_card = layer_view;
        
        /* build card widget layout */
        count = stack_widget_count(stack, stackmgr_current_card_id(stack), 0);
        
        for (index = 0; index < count; index++)
        {
            widget_id = stack_widget_n(stack, stackmgr_current_card_id(stack), 0, index);
            [self buildWidget:widget_id];
        }
    
    }
    
    //if (!_saved_screen)
    //{
    
        /* build the top overlay */
        layer_view = [[JHLayerView alloc] initWithFrameAsOverlay:self.bounds];
        [self addSubview:layer_view];
        _layer_overlay = layer_view;
        
    //}
    
    /* show the auto-status (if available) */
    //if (_auto_status)
    //    [self addSubview:_auto_status];
    
    //if (!_saved_screen)
    //{
        [self layout];
        [self setNeedsDisplay:YES];
        
    //}
}




/*********
 Image Representations
 */

/* these will be used to allow entry into a bitmap editing mode using the Paint sub-system;
 and to enable the creation of visual effect transitions */



/* transparent image including only the background object images as they appear currently */
- (NSImage*)_pictureOfBkgndObjects
{
    NSBitmapImageRep *bitmap = [self bitmapImageRepForCachingDisplayInRect:self.bounds];
    if (!bitmap) return nil;
    NSImage *image = [[NSImage alloc] init];
    if (!image) return nil;
    [image addRepresentation:bitmap];
    
    // hide all the card objects and the background art
    for (NSView *child in self.subviews)
    {
        if ((child == layer_card) || (child == layer_bkgnd))
            [child setHidden:YES];
        else if ( ([child conformsToProtocol:@protocol(JHWidget)]) && (((NSView<JHWidget>*)child).is_card) )
            [child setHidden:YES];
    }
    
    // cache display into the cache bitmap
    _disable_drawing_backdrop = YES;
    [self setNeedsDisplayInRect:self.bounds];
    [self cacheDisplayInRect:self.bounds toBitmapImageRep:bitmap];
    _disable_drawing_backdrop = NO;
    
    // show all the card objects and the background art
    for (NSView *child in self.subviews)
    {
        if ((child == layer_card) || (child == layer_bkgnd))
            [child setHidden:NO];
        else if ( ([child conformsToProtocol:@protocol(JHWidget)]) && (((NSView<JHWidget>*)child).is_card) )
            [child setHidden:NO];
    }
    
    return image;
}



/* composite picture of background, including artwork and objects, and white backing layer? */
- (NSImage*)_pictureOfBkgndObjectsAndArt
{
    NSBitmapImageRep *bitmap = [self bitmapImageRepForCachingDisplayInRect:self.bounds];
    if (!bitmap) return nil;
    NSImage *image = [[NSImage alloc] init];
    if (!image) return nil;
    [image addRepresentation:bitmap];
    
    // hide all the card objects
    for (NSView *child in self.subviews)
    {
        if (child == layer_card)
            [child setHidden:YES];
        else if ( ([child conformsToProtocol:@protocol(JHWidget)]) && (((NSView<JHWidget>*)child).is_card) )
            [child setHidden:YES];
    }
    
    // cache display into the cache bitmap
    [self setNeedsDisplayInRect:self.bounds];
    [self cacheDisplayInRect:self.bounds toBitmapImageRep:bitmap];
    
    // show all the card objects
    for (NSView *child in self.subviews)
    {
        if (child == layer_card)
            [child setHidden:NO];
        else if ( ([child conformsToProtocol:@protocol(JHWidget)]) && (((NSView<JHWidget>*)child).is_card) )
            [child setHidden:NO];
    }
    
    return image;
}


/* transparent image including only the card object images as they appear currently */
- (NSImage*)_pictureOfCardObjects
{
    NSBitmapImageRep *bitmap = [self bitmapImageRepForCachingDisplayInRect:self.bounds];
    if (!bitmap) return nil;
    NSImage *image = [[NSImage alloc] init];
    if (!image) return nil;
    [image addRepresentation:bitmap];
    
    // hide everything except the card objects
    for (NSView *child in self.subviews)
    {
        if (!( ([child conformsToProtocol:@protocol(JHWidget)]) && (((NSView<JHWidget>*)child).is_card) ))
            [child setHidden:YES];
    }
    
    // cache display into the cache bitmap
    _disable_drawing_backdrop = YES;
    [self setNeedsDisplayInRect:self.bounds];
    [self setNeedsLayout:YES];
    [self cacheDisplayInRect:self.bounds toBitmapImageRep:bitmap];
    _disable_drawing_backdrop = NO;
    
    // show everything except the card objects
    for (NSView *child in self.subviews)
    {
        if (!( ([child conformsToProtocol:@protocol(JHWidget)]) && (((NSView<JHWidget>*)child).is_card) ))
            [child setHidden:NO];
    }
    
    return image;
}





/* complete picture of however the current card should look, taking into account edit background mode */
- (NSImage*)_pictureOfCard
{
    NSBitmapImageRep *bitmap = [self bitmapImageRepForCachingDisplayInRect:self.bounds];
    if (!bitmap) return nil;
    NSImage *image = [[NSImage alloc] init];
    if (!image) return nil;
    [image addRepresentation:bitmap];
    
    // cache display into the cache bitmap
    [self setNeedsDisplayInRect:self.bounds];
    _render_actual_card = YES;
    [self cacheDisplayInRect:self.bounds toBitmapImageRep:bitmap];
    _render_actual_card = NO;
    
    return image;
}




- (void)acuLayoutWillChange
{
    [self _suspendPaint];
    [[self window] makeFirstResponder:self];
}


- (void)acuLayoutDidChange
{
    selection_type = SELECTION_CARD;
    card_index = stack_card_index_for_id(stack, stackmgr_current_card_id(stack));
    
    //if (_last_effect_slide != nil) return;
    
    [self _buildCard];
    [self _resumePaint];
}



- (void)setAutoStatusVisible:(BOOL)in_visible canAbort:(BOOL)in_can_abort
{
    if (in_visible)
    {
        if (!_auto_status)
        {
            _auto_status = [[JHProgressView alloc] initWithFrame:[[[self window] contentView] bounds]];
            [[[self window] contentView] addSubview:_auto_status];
        }
    }
    else
    {
        if (_auto_status)
        {
            [_auto_status removeFromSuperview];
            _auto_status = nil;
            
        }
    }
}



@end
