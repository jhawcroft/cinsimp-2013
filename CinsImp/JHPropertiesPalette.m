/*
 
 Properties Palette Controller
 JHPropertiesPalette.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHPropertiesPalette.h"

#import "JHAppScriptEditorNotifications.h"

#import "JHCardView.h"
#import "JHCardView+LayoutSelection.h"
#import "JHPropertiesView.h"

#import "jhglobals.h"

#include "cstritems.h"

#import "JHCardWindow.h"


static JHPropertiesPalette *_shared = nil;


@implementation JHPropertiesPalette


/*********
 Auto-hide for Script Editing
 */

- (void)scriptEditorBecameActive:(NSNotification*)notification
{
    _was_visible_se = [[self window] isVisible];
    [[self window] orderOut:self];
}


- (void)scriptEditorBecameInactive:(NSNotification*)notification
{
    if (_was_visible_se)
    {
        [[self window] orderFront:self];
    }
}


/*********
 Controller Initalization
 */


+ (JHPropertiesPalette*)sharedInstance
{
    return _shared;
}


- (id)initWithWindowNibName:(NSString *)windowNibName
{
    self = [super initWithWindowNibName:windowNibName];
    if (!self) return nil;
    
    _shared = self;
    is_loading = NO;
    
    /* register to receive notification when the user selects different layout objects;
     we will display the properties of the current selection */
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(designSelectionDidChange:)
                                                 name:@"DesignSelectionChange"
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(scriptEditorBecameActive:)
                                                 name:scriptEditorBecameActive
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(scriptEditorBecameInactive:)
                                                 name:scriptEditorBecameInactive
                                               object:nil];
    
    /* create a font menu */
    font_menu = create_font_menu();
    
    return self;
}


- (void)windowDidLoad
{
    [super windowDidLoad];
    
    /* manually create an instance of our special card size widget;
     add it to the stack editor pane */
    cardSize = [[JHCardSize alloc] initWithNibName:@"JHCardSize" bundle:nil];
    [cardSize loadView];
    [paneStackCardSizeHole addSubview:[cardSize view]];
    cardSize.delegate = self;
}



/*********
 Adjustment/Change Handlers
 */

- (void)cardSize:(JHCardSize *)card_size changed:(NSSize)new_size
{
    stack_set_card_size(stack, new_size.width, new_size.height);
    [card_view refreshCard];
}


- (IBAction)paneChangedID:(NSTextField*)sender
{
    if (is_loading) return;
    
    if (icon_grid)
    {
        int new_id = [sender intValue];
        acu_iconmgr_mutate(stack, icon_id, &new_id, NULL, NULL, 0);
        [icon_grid reload];
        return;
    }
}


- (void)gotIcon:(NSNotification*)inNotification
{
    _grab_icon = NO;
    
    JHIconGrid *grid = (JHIconGrid*)[inNotification object];
    if (![grid isKindOfClass:[JHIconGrid class]]) return;
    
    
    int the_id;
    char *the_name;
    
    acu_iconmgr_icon_n(stack, [grid selectedIconIndex], &the_id, &the_name, NULL, NULL);
    
    //NSLog(@"got the icon: %d", the_id);

    if (selection_type == SELECTION_WIDGETS) 
    {
        long widget_id = [[selected_ids lastObject] longValue];
        stack_widget_prop_set_long(stack, widget_id, current_card_id, PROPERTY_ICON, the_id);
        [card_view refreshWidgetWithID:widget_id];
    }
    
    [[card_view window] makeKeyAndOrderFront:self];
}


- (IBAction)paneButtonSetIcon:(id)sender
{
    if (card_view)
    {
        if ([sender tag] == 1)
        {
            JHCardWindow *card_window = (JHCardWindow*)[card_view window];
            is_loading = YES;
            [card_window setIcon:self];
            is_loading = NO;
            _grab_icon = YES;
        }
        else
        {
            long widget_id = [[selected_ids lastObject] longValue];
            stack_widget_prop_set_long(stack, widget_id, current_card_id, PROPERTY_ICON, 0);
            [card_view refreshWidgetWithID:widget_id];
            [paneButtonSetIcon setTitle:@"Set Icon..."];
            [paneButtonSetIcon setTag:1];
        }
    }
}


- (IBAction)paneChangedIconDesc:(NSTextField*)sender
{
    /* !  not currently used */
    if (is_loading) return;
    if (card_view)
    {
    }
}


- (IBAction)paneChangedName:(NSTextField*)sender
{
    if (is_loading) return;
    
    if (icon_grid)
    {
        acu_iconmgr_mutate(stack, icon_id, NULL, [[sender stringValue] UTF8String], NULL, 0);
        [icon_grid reload];
        return;
    }
    
    stack_undo_activity_begin(stack, "Rename", current_card_id);
    if (selection_type != SELECTION_WIDGETS)
    {
        if (selection_type == SELECTION_CARD)
            stack_prop_set_string(stack, current_card_id, 0, PROPERTY_NAME,
                                  (char*)[sender.stringValue cStringUsingEncoding:NSUTF8StringEncoding]);
        else
            stack_prop_set_string(stack, 0, current_bkgnd_id, PROPERTY_NAME, (char*)[sender.stringValue cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    else
    {
        for (NSNumber *num in selected_ids)
        {
            long widget_id = num.longValue;
            stack_widget_prop_set_string(stack, widget_id, current_card_id, PROPERTY_NAME,
                                         (char*)[sender.stringValue cStringUsingEncoding:NSUTF8StringEncoding]);
            [card_view refreshWidgetWithID:widget_id];
        }
    }
    stack_undo_activity_end(stack);
}


- (IBAction)paneChangedStyle:(NSPopUpButton*)sender
{
    stack_undo_activity_begin(stack, "Object Style", current_card_id);
    for (NSNumber *num in selected_ids)
    {
        long widget_id = num.longValue;
        stack_widget_prop_set_string(stack, widget_id, current_card_id, PROPERTY_STYLE,
                                     (char*)[sender.titleOfSelectedItem cStringUsingEncoding:NSUTF8StringEncoding]);
        [card_view refreshWidgetWithID:widget_id];
    }
    stack_undo_activity_end(stack);
}


- (IBAction)paneChangedRichText:(NSButton*)sender
{
    stack_undo_activity_begin(stack, "Styled Text", current_card_id);
    for (NSNumber *num in selected_ids)
    {
        long widget_id = num.longValue;
        stack_widget_prop_set_long(stack, widget_id, current_card_id, PROPERTY_RICHTEXT, (sender.state == NSOnState));
        [card_view refreshWidgetWithID:widget_id];
    }
    stack_undo_activity_end(stack);
}



- (IBAction)paneChangedLocked:(NSButton*)sender
{
    stack_undo_activity_begin(stack, "Lock", current_card_id);
    for (NSNumber *num in selected_ids)
    {
        long widget_id = num.longValue;
        stack_widget_prop_set_long(stack, widget_id, current_card_id, PROPERTY_LOCKED, (sender.state == NSOnState));
        [card_view refreshWidgetWithID:widget_id];
    }
    stack_undo_activity_end(stack);
}


- (IBAction)paneChangedShared:(NSButton*)sender
{
    stack_undo_activity_begin(stack, "Share", current_card_id);
    for (NSNumber *num in selected_ids)
    {
        long widget_id = num.longValue;
        stack_widget_prop_set_long(stack, widget_id, current_card_id, PROPERTY_SHARED, (sender.state == NSOnState));
        [card_view refreshWidgetWithID:widget_id];
    }
    stack_undo_activity_end(stack);
}


- (IBAction)paneChangedDontSearch:(NSButton*)sender
{
    stack_undo_activity_begin(stack, "Don't Search", current_card_id);
    if (selection_type != SELECTION_WIDGETS)
    {
        if (selection_type == SELECTION_CARD)
            stack_prop_set_long(stack, current_card_id, 0, PROPERTY_DONTSEARCH, (sender.state == NSOnState));
        else
            stack_prop_set_long(stack, 0, current_bkgnd_id, PROPERTY_DONTSEARCH, (sender.state == NSOnState));
    }
    else
    {
        for (NSNumber *num in selected_ids)
        {
            long widget_id = num.longValue;
            stack_widget_prop_set_long(stack, widget_id, current_card_id, PROPERTY_DONTSEARCH, (sender.state == NSOnState));
            [card_view refreshWidgetWithID:widget_id];
        }
    }
    stack_undo_activity_end(stack);
}


- (IBAction)paneChangedCantDelete:(NSButton*)sender
{
    stack_undo_activity_begin(stack, "Can't Delete", current_card_id);
    switch (selection_type)
    {
        case SELECTION_CARD:
            stack_prop_set_long(stack, current_card_id, 0, PROPERTY_CANTDELETE, (sender.state == NSOnState));
            break;
        case SELECTION_BKGND:
            stack_prop_set_long(stack, 0, current_bkgnd_id, PROPERTY_CANTDELETE, (sender.state == NSOnState));
            break;
        default:
            break;
            
    }
    stack_undo_activity_end(stack);
}


- (IBAction)paneChangedTextStyle:(NSButton*)sender
{
    stack_undo_activity_begin(stack, "Text Style", current_card_id);
    const char *which_style = [[[sender title] lowercaseString] cStringUsingEncoding:NSUTF8StringEncoding];
    for (NSNumber *num in selected_ids)
    {
        long widget_id = num.longValue;
        const char *cstr = stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_TEXTSTYLE);
        if (sender.state == NSOnState)
            cstr = cstr_item_add(cstr, which_style);
        else
            cstr = cstr_item_remove(cstr, which_style);
        stack_widget_prop_set_string(stack, widget_id, current_card_id, PROPERTY_TEXTSTYLE, (char*)cstr);
        [card_view refreshWidgetWithID:widget_id];
    }
    stack_undo_activity_end(stack);
}


- (IBAction)paneChangedTextFontOrSize:(NSView*)sender
{
    stack_undo_activity_begin(stack, "Text Font", current_card_id);
    for (NSNumber *num in selected_ids)
    {
        long widget_id = num.longValue;
        if ([sender isKindOfClass:[NSPopUpButton class]])
        {
            stack_widget_prop_set_string(stack, widget_id, current_card_id, PROPERTY_TEXTFONT,
                                         (char*)[[(NSPopUpButton*)sender titleOfSelectedItem] cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        else
        {
            stack_widget_prop_set_string(stack, widget_id, current_card_id, PROPERTY_TEXTSIZE,
                                         (char*)[[(NSTextField*)sender stringValue] cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        [card_view refreshWidgetWithID:widget_id];
    }
    stack_undo_activity_end(stack);
}


- (IBAction)paneChangedBorder:(NSPopUpButton*)sender
{
    stack_undo_activity_begin(stack, "Border", current_card_id);
    for (NSNumber *num in selected_ids)
    {
        long widget_id = num.longValue;
        stack_widget_prop_set_string(stack, widget_id, current_card_id, PROPERTY_BORDER,
                                     (char*)[[sender titleOfSelectedItem] cStringUsingEncoding:NSUTF8StringEncoding]);
        [card_view refreshWidgetWithID:widget_id];
    }
    stack_undo_activity_end(stack);
}



/*********
 Button Editing
 */

- (void)loadPaneButton
{
    if (selected_ids.count == 1)
    {
        long widget_id = ((NSNumber*)[selected_ids lastObject]).longValue;
        paneButtonDesc.stringValue = [NSString stringWithFormat:@"%@ Button ID: %ld",
                                      (stack_widget_is_card(stack, widget_id) ? @"Card" : @"Bkgnd"),
                                      widget_id];
    }
    else
    {
        long a_widget_id = ((NSNumber*)[selected_ids lastObject]).longValue;
        paneButtonDesc.stringValue = [NSString stringWithFormat:@"%ld %@ Buttons",
                                      selected_ids.count,
                                      (stack_widget_is_card(stack, a_widget_id) ? @"Card" : @"Bkgnd")];
    }
    
    bool first = YES;
    for (NSNumber *num in selected_ids)
    {
        long widget_id = num.longValue;
        
        NSString *tempString;
        
        tempString = [NSString stringWithCString:
                      stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_NAME) encoding:NSUTF8StringEncoding];
        if ((!first) && (![paneButtonName.stringValue isEqualToString:tempString]))
            paneButtonName.stringValue = @"";
        else
            paneButtonName.stringValue = tempString;
        
        tempString = [NSString stringWithCString:
                      stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_STYLE) encoding:NSUTF8StringEncoding];
        [paneButtonStyle selectItemWithTitle:[tempString capitalizedString]];
        
        int the_icon = (int)stack_widget_prop_get_long(stack, widget_id, current_card_id, PROPERTY_ICON);
        if (the_icon == 0)
        {
            [paneButtonSetIcon setTitle:@"Set Icon..."];
            [paneButtonSetIcon setTag:1];
        }
        else
        {
            [paneButtonSetIcon setTitle:@"No Icon"];
            [paneButtonSetIcon setTag:2];
        }

        first = NO;
    }
}



/*********
 Field Editing
 */

- (void)loadPaneFieldsMany
{
    long widget_id = ((NSNumber*)[selected_ids lastObject]).longValue;
    paneFieldsDesc.stringValue = [NSString stringWithFormat:@"%ld %@ Fields",
                                  selected_ids.count,
                                  (stack_widget_is_card(stack, widget_id) ? @"Card" : @"Bkgnd")];
    
    bool first = YES;
    for (NSNumber *num in selected_ids)
    {
        long widget_id = num.longValue;
        
        NSString *tempString;
        
        tempString = [NSString stringWithCString:
                      stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_NAME) encoding:NSUTF8StringEncoding];
        if ((!first) && (![paneFieldsName.stringValue isEqualToString:tempString]))
            paneFieldsName.stringValue = @"";
        else
            paneFieldsName.stringValue = tempString;
        
        tempString = [NSString stringWithCString:
                      stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_STYLE) encoding:NSUTF8StringEncoding];
        if ((!first) && (![paneFieldsStyle.stringValue isEqualToString:tempString]))
            [paneFieldsStyle selectItem:nil];
        else
            [paneFieldsStyle selectItemWithTitle:[tempString capitalizedString]];
    }
}


- (void)loadPaneFieldText
{
    long widget_id = ((NSNumber*)[selected_ids lastObject]).longValue;
    paneFieldTextDesc.stringValue = [NSString stringWithFormat:@"%@ Field ID: %ld",
                                  (stack_widget_is_card(stack, widget_id) ? @"Card" : @"Bkgnd"),
                                  widget_id];
    
    NSString *tempString;
    //NSArray *tempParts;
    
    tempString = [NSString stringWithCString:
                  stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_NAME) encoding:NSUTF8StringEncoding];
    paneFieldTextName.stringValue = tempString;
    
    tempString = [NSString stringWithCString:
                  stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_STYLE) encoding:NSUTF8StringEncoding];
    [paneFieldTextStyle selectItemWithTitle:[tempString capitalizedString]];
    
    tempString = [NSString stringWithCString:
                  stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_BORDER) encoding:NSUTF8StringEncoding];
    if ([tempString length] == 0) tempString = @"transparent";
    [paneFieldTextBorder selectItemWithTitle:[tempString capitalizedString]];
    
    if (stack_widget_prop_get_long(stack, widget_id, current_card_id, PROPERTY_LOCKED))
        [paneFieldTextLocked setState:NSOnState];
    else
        [paneFieldTextLocked setState:NSOffState];
    if (stack_widget_prop_get_long(stack, widget_id, current_card_id, PROPERTY_DONTSEARCH))
        [paneFieldTextDontSearch setState:NSOnState];
    else
        [paneFieldTextDontSearch setState:NSOffState];
    if (stack_widget_prop_get_long(stack, widget_id, current_card_id, PROPERTY_SHARED))
        [paneFieldTextShared setState:NSOnState];
    else
        [paneFieldTextShared setState:NSOffState];
    [paneFieldTextShared setHidden:stack_widget_is_card(stack, widget_id)];
    
    if (stack_widget_prop_get_long(stack, widget_id, current_card_id, PROPERTY_RICHTEXT))
        [paneFieldTextRich setState:NSOnState];
    else
        [paneFieldTextRich setState:NSOffState];
    
    [paneFieldTextFont setMenu:font_menu];
    tempString = [NSString stringWithCString:
                  stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_TEXTFONT) encoding:NSUTF8StringEncoding];
    if ([tempString length] == 0) tempString = @"Lucida Grande";
    [paneFieldTextFont selectItemWithTitle:[tempString capitalizedString]];
    
    tempString = [NSString stringWithCString:
                  stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_TEXTSIZE) encoding:NSUTF8StringEncoding];
    if ([tempString length] == 0) tempString = @"12";
    paneFieldTextSize.stringValue = tempString;
    
    if (cstr_has_item(stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_TEXTSTYLE), "bold"))
        [paneFieldTextBold setState:NSOnState];
    else
        [paneFieldTextBold setState:NSOffState];
    if (cstr_has_item(stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_TEXTSTYLE), "italic"))
        [paneFieldTextItalic setState:NSOnState];
    else
        [paneFieldTextItalic setState:NSOffState];
}


- (void)loadPaneFieldCheck
{
    long widget_id = ((NSNumber*)[selected_ids lastObject]).longValue;
    paneFieldCheckDesc.stringValue = [NSString stringWithFormat:@"%@ Field ID: %ld",
                                     (stack_widget_is_card(stack, widget_id) ? @"Card" : @"Bkgnd"),
                                     widget_id];
    
    NSString *tempString;
    //NSArray *tempParts;
    
    tempString = [NSString stringWithCString:
                  stack_widget_prop_get_string(stack, widget_id, current_card_id, PROPERTY_NAME) encoding:NSUTF8StringEncoding];
    paneFieldCheckName.stringValue = tempString;
    
    [paneFieldCheckStyle selectItemWithTitle:@"Check Box"];
    
    if (stack_widget_prop_get_long(stack, widget_id, current_card_id, PROPERTY_SHARED))
        [paneFieldCheckShared setState:NSOnState];
    else
        [paneFieldCheckShared setState:NSOffState];
    [paneFieldCheckShared setHidden:stack_widget_is_card(stack, widget_id)];
}



/*********
 Card Editing
 */

- (void)loadPaneCard
{
    long card_id = ((NSNumber*)[selected_ids lastObject]).longValue;
    
    paneCardDesc.stringValue = [NSString stringWithFormat:@"Card ID: %ld", card_id];
    
    NSString *tempString;
    
    tempString = [NSString stringWithCString:
                  stack_prop_get_string(stack, current_card_id, 0, PROPERTY_NAME) encoding:NSUTF8StringEncoding];
    paneCardName.stringValue = tempString;
    
    if (stack_prop_get_long(stack, current_card_id, 0, PROPERTY_DONTSEARCH))
        [paneCardDontSearch setState:NSOnState];
    else
        [paneCardDontSearch setState:NSOffState];
    if (stack_prop_get_long(stack, current_card_id, 0, PROPERTY_CANTDELETE))
        [paneCardCantDelete setState:NSOnState];
    else
        [paneCardCantDelete setState:NSOffState];
}



/*********
 Background Editing
 */

- (void)loadPaneBkgnd
{
    long bkgnd_id = ((NSNumber*)[selected_ids lastObject]).longValue;
    
    paneBkgndDesc.stringValue = [NSString stringWithFormat:@"Bkgnd ID: %ld", bkgnd_id];
    
    NSString *tempString;
    
    tempString = [NSString stringWithCString:
                  stack_prop_get_string(stack, 0, bkgnd_id, PROPERTY_NAME) encoding:NSUTF8StringEncoding];
    paneBkgndName.stringValue = tempString;
    
    if (stack_prop_get_long(stack, 0, bkgnd_id, PROPERTY_DONTSEARCH))
        [paneBkgndDontSearch setState:NSOnState];
    else
        [paneBkgndDontSearch setState:NSOffState];
    if (stack_prop_get_long(stack, 0, bkgnd_id, PROPERTY_CANTDELETE))
        [paneBkgndCantDelete setState:NSOnState];
    else
        [paneBkgndCantDelete setState:NSOffState];
}



/*********
 Stack Editing
 */

- (void)loadPaneStack
{
    [paneStackDesc setStringValue:[NSString stringWithFormat:@"Stack: %@",
                                   [[[NSDocumentController sharedDocumentController] currentDocument] displayName]] ];
    
    long card_width, card_height;
    stack_get_card_size(stack, &card_width, &card_height);
    [cardSize set_card_size:NSMakeSize(card_width, card_height)];
}


- (IBAction)editScriptOfSelection:(id)sender
{
    [card_view editScript];
}



/*********
 Notification Handlers
 */




/* response to user changing the selected layout object(s) */
- (void)designSelectionDidChange:(NSNotification*)inNotification
{
    if (is_loading) return;
    
    if (_grab_icon)
    {
        [self gotIcon:inNotification];
        return;
    }
    
    enum Widget type;
    
    /* set keyboard focus to this window */
    [[self window] makeFirstResponder:self];
    
    /* get the current view */
    card_view = nil;
    icon_grid = nil;
    NSView *view = [inNotification object];
    if ([view isKindOfClass:[JHCardView class]])
        card_view = (JHCardView*)view;
    else if ([view isKindOfClass:[JHIconGrid class]])
        icon_grid = (JHIconGrid*)view;
    
    is_loading = YES;
    if (card_view)
    {
        /* obtain information about the selection */
        current_card_id = [card_view currentCardID];
        stack = [card_view stack];
        selected_ids = [card_view designSelectionIDs];
        selection_type = card_view.designSelectionType;
        
        /* branch to an appropriate pane loader;
         based on the selection */
        switch (selection_type)
        {
            case SELECTION_WIDGETS:
                /* get the widget type of the first selected widget */
                stack_widget_basics(stack, [[selected_ids lastObject] longValue], NULL, NULL, NULL, NULL, NULL, &type);
                
                /* when selected widgets are buttons... */
                if ((type == WIDGET_BUTTON_PUSH) || (type == WIDGET_BUTTON_TRANSPARENT))
                {
                    /* a single button;
                     note: buttons are not accessible in the UI of CinsImp 1.0
                     and are only partially implemented; buttons will be implemented
                     in a later version of CinsImp */
                    [self loadPaneButton];
                    [paneRoot setPane:paneButton];
                }
                
                /* when selected widgets are fields... */
                else if (selected_ids.count > 1)
                {
                    /* multiple fields */
                    [self loadPaneFieldsMany];
                    [paneRoot setPane:paneFieldsMany];
                }
                else if (type == WIDGET_FIELD_TEXT)
                {
                    /* a single text field */
                    [self loadPaneFieldText];
                    [paneRoot setPane:paneFieldText];
                }
                else if (type == WIDGET_FIELD_CHECK)
                {
                    /* a single checkbox */
                    [self loadPaneFieldCheck];
                    [paneRoot setPane:paneFieldCheck];
                }
                else
                    [paneRoot setPane:nil];
                break;
                
            case SELECTION_CARD:
                [self loadPaneCard];
                [paneRoot setPane:paneCard];
                break;
                
            case SELECTION_BKGND:
                /* get background ID */
                current_bkgnd_id = [[selected_ids lastObject] longValue];
                
                [self loadPaneBkgnd];
                [paneRoot setPane:paneBkgnd];
                break;
                
            case SELECTION_STACK:
                [self loadPaneStack];
                [paneRoot setPane:paneStack];
                break;
                
                /* when nothing is selected... */
            default:
                [paneRoot setPane:nil];
                
                break;
        }
    }
    
    else if (icon_grid)
    {
        /* obtain information about selected icon */
        stack = [icon_grid stack];
        
        if ([icon_grid hasSelection])
        {
            int the_id;
            char *the_name;
            acu_iconmgr_icon_n(stack, [icon_grid selectedIconIndex], &the_id, &the_name, NULL, NULL);
            icon_id = the_id;
            [paneIconID setIntValue:the_id];
            [paneIconName setStringValue:[NSString stringWithCString:the_name encoding:NSUTF8StringEncoding]];
            
            
            
            
            [paneRoot setPane:paneIcon];
        }
        else
            [paneRoot setPane:nil];
    }
    
    else
        [paneRoot setPane:nil];
    is_loading = NO;
}



/*********
 Event Propagation
 */

- (void)flagsChanged:(NSEvent *)theEvent
{
    /* pass the event along to the current card view */
    if (gCurrentCardView) [card_view flagsChanged:theEvent];
}



/*********
 Showing/Hiding the Palette
 */

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    if (menuItem.action == @selector(toggleProperties:))
    {
        if (self.window.isVisible)
            [menuItem setState:NSOnState];
        else
            [menuItem setState:NSOffState];
    }
    return YES;
}


- (IBAction)toggleProperties:(id)sender
{
    if (self.window.isVisible)
        [self.window orderOut:nil];
    else
        [self.window makeKeyAndOrderFront:nil];
    
}


- (IBAction)showProperties:(id)sender
{
    [self.window makeKeyAndOrderFront:nil];
}




@end
