/*
 
 Stack's Card View
 JHCardView.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

/*
 
 Locked Stacks
 -------------------------------------------------------------------------------------------------
 We ensure that user interface actions (such as menuitems) which are not allowable if the stack is
 locked are not enabled, ie. cannot be actioned by the user.
 
 The Stack itself will prevent writes in the event there is a problem with the user interface code.
 
 */

#import "JHCardView.h"


#import "JHDocument.h"

#import "JHToolPaletteController.h"
#import "JHMenubarController.h"
#import "JHCursorController.h"
#import "JHColourPaletteController.h"
#import "JHLineSizePaletteController.h"

#import "JHLayerView.h"



#import "jhglobals.h"

#include "stack.h"



NSString *designLayerChangeNotification = @"designLayerChange";


@implementation JHCardView



/* included here for debugging to ensure document closure is not leaking; not required in production */

- (void)dealloc
{
     NSLog(@"card view dealloc");
}



/************
 Opening
 */

/* initalize view state */
- (void)loadView
{
    /* make this a layer backed view, for speed and animation
     of visual effects */
    [self setWantsLayer:YES]; // is causing problems for drawing the outlines, etc.
    
    
    /* assume stack has protected access;
     ensure nothing is displayed until the password is verified or security disabled */
    protected_hidden = YES;
    
    /* reset state */
    stack = NULL;
    trackingArea = nil;
    edit_bkgnd = NO;
    peek_flds = NO;
    peek_btns = NO;
    show_sequence = NO;
    snap_line_x = -1;
    snap_line_y = -1;
    dragTargets = [[NSMutableArray alloc] init];
    quiet_selection_notifications = NO;
    _paint_suspended = NO;
    
    layer_bkgnd = nil;
    layer_card = nil;
    _layer_overlay = nil;
    _auto_status = nil;
    
    _paint_subsys = NULL;
    
    _script_debug_close_timer = nil;
    
    _last_effect_slide = nil;
}


- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) [self loadView];
    return self;
}

- (id)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self) [self loadView];
    return self;
}


- (void)_closeStack
{
    //edit_bkgnd = NO;
    //[[NSNotificationCenter defaultCenter] postNotification:
    // [NSNotification notificationWithName:@"DesignLayerChange" object:self]];
    
    /* close open script editors */
    while ([_open_scripts count] > 0)
    {
        JHScriptEditorController *controller = [_open_scripts lastObject];
        [controller close];
    }
    
    /* finish any open paint session */
    [self _exitPaintSession];
    
    /* force currently edited field to loose selection,
     thus writing any changes to disk */
    [[self window] makeFirstResponder:[self window]];
    
    /* dispose layout selection */
    [self _deselectAll];
    selection_type = SELECTION_NONE;
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:@"DesignSelectionChange" object:nil]];
    
    /* update our internal state */
    _open_scripts = nil;
    stack = NULL;
    
    /* unregister all notification handlers */
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}


- (void)_openStack:(Stack*)inStack
{
    /* configure our internal state */
    stack = inStack;
    _disable_drawing_backdrop = NO;
    
    //current_card_id = stack_card_id_for_index(stack, 0);
    //current_bkgnd_id = 1;
    selection_type = SELECTION_CARD;
    card_index = 0;
    _edit_mode = CARDVIEW_MODE_BROWSE;
    _open_scripts = [[NSMutableArray alloc] init];
    protected_hidden = ( (stack_prop_get_string(stack, 0, 0, PROPERTY_PASSWORD)[0] != 0)
                        && stack_prop_get_long(stack, 0, 0, PROPERTY_PRIVATE) );
    was_locked = stack_prop_get_long(stack, 0, 0, PROPERTY_LOCKED);
    currentTool = [[JHToolPaletteController sharedController] currentTool];
    
    
    /* register to receive notifications */
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(notifyToolDidChange:)
                                                 name:notifyToolDidChange
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(notifyToolDoubleClicked:)
                                                 name:notifyToolDoubleClicked
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(notifyColourDidChange:)
                                                 name:notifyColourDidChange
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(notifyLineSizeDidChange:)
                                                 name:notifyLineSizeDidChange
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(notifyScriptEditorDidClose:)
                                                 name:notifyScriptEditorDidClose
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(windowDidBecomeKey:)
                                                 name:NSWindowDidBecomeKeyNotification
                                               object:[self window]];
    
    /* build current card layout */
    [self _buildCard];
}


- (void)setStack:(Stack*)inStack
{
    if (inStack == NULL) [self _closeStack];
    else [self _openStack:inStack];
}



/************
 View Configuration
 */

- (BOOL)isFlipped
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}



/************
 Closing
 */

- (void)viewWillMoveToWindow:(NSWindow *)newWindow
{
    if (newWindow == nil)
    {
        /* cleanup on exit */
        [self setStack:NULL];
        return;
    }
}



/************
 Resize
 */

- (void)updateTrackingAreas
{
    [self removeTrackingArea:trackingArea];
    trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds options:(NSTrackingActiveInActiveApp | NSTrackingCursorUpdate | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved)
                                                  owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}



/************
 Access Control
 */

- (void)disableSecurity
{
    protected_hidden = NO;
    [self _buildCard];
}

- (BOOL)securityEnabled
{
    return protected_hidden;
}



/************
 Accessors
 */


- (Stack*)stack
{
    return stack;
}


- (long)currentCardID
{
    if (!stack) return STACK_NO_OBJECT;
    return stackmgr_current_card_id(stack);
    //return (stack_sorting_card(stack) > 0 ? stack_sorting_card(stack) : current_card_id);
}


- (BOOL)isEditingBkgnd
{
    return edit_bkgnd;
}










/************
 Cursor
 */

- (void)_setAppropriateCursor
{
    switch (currentTool)
    {
        case AUTH_TOOL_BROWSE:
        {
            NSPoint mouseLoc = [self convertPoint:[[self window]  mouseLocationOutsideOfEventStream] fromView:nil];
            NSView *view = [self _hitTarget:mouseLoc];
            if ([view conformsToProtocol:@protocol(JHWidget)])
            {
                NSView<JHWidget> *widget = (NSView<JHWidget>*)view;
                if ((!widget.is_locked) && (![widget isButton]))
                {
                    //NSLog(@"setting ibeam");
                    [[JHCursorController sharedController] setCursor:CURSOR_IBEAM];
                    return;
                }
            }
           // NSLog(@"setting browse");
            [[JHCursorController sharedController] setCursor:CURSOR_BROWSE];
            break;
        }
        case AUTH_TOOL_BUTTON:
        case AUTH_TOOL_FIELD:
            [[JHCursorController sharedController] setCursor:CURSOR_ARROW];
            break;
        case PAINT_TOOL_EYEDROPPER:
            [[JHCursorController sharedController] setCursor:CURSOR_EYEDROPPER];
            break;
        case PAINT_TOOL_PENCIL:
            [[JHCursorController sharedController] setCursor:CURSOR_PENCIL];
            break;
        case PAINT_TOOL_ERASER:
            [[JHCursorController sharedController] setCursor:CURSOR_ERASER];
            break;
        case PAINT_TOOL_BUCKET:
            [[JHCursorController sharedController] setCursor:CURSOR_BUCKET];
            break;
        case PAINT_TOOL_SPRAY:
        case PAINT_TOOL_BRUSH:
            [[JHCursorController sharedController] setCursor:CURSOR_ROUND];
            break;
        case PAINT_TOOL_LINE:
        case PAINT_TOOL_RECTANGLE:
        case PAINT_TOOL_ROUNDED_RECT:
        case PAINT_TOOL_OVAL:
        case PAINT_TOOL_FREESHAPE:
        case PAINT_TOOL_FREEPOLY:
            [[JHCursorController sharedController] setCursor:CURSOR_CROSSHAIR];
            break;
        case PAINT_TOOL_LASSO:
        case PAINT_TOOL_SELECT:
        {
            NSPoint mouseLoc = [self convertPoint:[[self window] mouseLocationOutsideOfEventStream] fromView:nil];
            if (_paint_subsys && paint_loc_within_selection(_paint_subsys, mouseLoc.x, mouseLoc.y))
                [[JHCursorController sharedController] setCursor:CURSOR_ARROW];
            else
                [[JHCursorController sharedController] setCursor:CURSOR_CROSSHAIR];
            break;
        }
    }
}











/************
 Mode
 */





- (void)setMode:(int)inMode
{
    if ((_edit_mode == CARDVIEW_MODE_PAINT) && (inMode != _edit_mode))
    {
        /* leaving paint mode; cleanup */
        [self _exitPaintSession];
    }
    else if ((_edit_mode != CARDVIEW_MODE_PAINT) && (inMode == CARDVIEW_MODE_PAINT))
    {
        /* entering paint mode; setup */
        [self _openPaintSession];
    }
    
    _edit_mode = inMode;
    
    switch (_edit_mode)
    {
        case CARDVIEW_MODE_BROWSE:
            [[JHMenubarController sharedController] setMode:MENU_MODE_BROWSE];
            break;
        case CARDVIEW_MODE_LAYOUT:
            [[JHMenubarController sharedController] setMode:MENU_MODE_LAYOUT];
            break;
        case CARDVIEW_MODE_PAINT:
            [[JHMenubarController sharedController] setMode:MENU_MODE_PAINT];
            break;
    }
}

- (void)chooseTool:(int)inTool
{
    //[self buildCard];
    
    if (inTool == currentTool) return;
    
    currentTool = inTool;
    stackmgr_set_tool(stack, inTool);
    [self _deselectAll];
    
    /* change edit mode */
    switch (inTool)
    {
        case AUTH_TOOL_BROWSE:
            [self setMode:CARDVIEW_MODE_BROWSE];
            break;
        case AUTH_TOOL_BUTTON:
        case AUTH_TOOL_FIELD:
            [self setMode:CARDVIEW_MODE_LAYOUT];
            break;
        default:
            [self setMode:CARDVIEW_MODE_PAINT];
            break;
    }
    
    if (_paint_subsys)
    {
        paint_current_tool_set(_paint_subsys, inTool);
    }
    
    [self _buildCard];
    [self setNeedsDisplay:YES];
}


- (int)currentTool
{
    return currentTool;
}



- (void)goToCard:(long)in_id
{
    if (in_id < 1) return;
    [self _suspendPaint];
    [[self window] makeFirstResponder:self];
    
    stackmgr_set_current_card_id(stack, in_id);
    //current_card_id = in_id;
    //current_bkgnd_id = 1;
    
    selection_type = SELECTION_CARD;
    card_index = stack_card_index_for_id(stack, in_id);
    [self _buildCard];
    [self _resumePaint];
}






- (void)doMessage:(NSString*)in_message
{
    //NSLog(@"card view got message: %@", in_message);
    //[self find:in_message];
    
}



/************
 Focus
 */



// generated by us and other widgets
- (void)_handleTab:(NSEvent *)event
{
    long new_focus_id = stack_widget_next(stack, focus_widget_id, stackmgr_current_card_id(stack), edit_bkgnd);
    if (new_focus_id == STACK_NO_OBJECT) return;
    NSView<JHWidget>* view = [self _widgetViewForID:new_focus_id];
    if (view) [[self window] makeFirstResponder:view];
}

- (void)_handleBackTab:(NSEvent *)event
{
    long new_focus_id = stack_widget_previous(stack, focus_widget_id, stackmgr_current_card_id(stack), edit_bkgnd);
    if (new_focus_id == STACK_NO_OBJECT) return;
    NSView<JHWidget>* view = [self _widgetViewForID:new_focus_id];
    if (view) [[self window] makeFirstResponder:view];
}


- (void)_handleFieldFocusChange:(NSNumber*)in_widget_id
{
    if (in_widget_id == nil)
    {
        //NSLog(@"field lost focus: %ld", focus_widget_id);
        
        
        
        focus_widget_id = 0;
    }
    else
    {
        focus_widget_id = [in_widget_id longValue];
        
        
        
        //NSLog(@"field got focus: %ld", focus_widget_id);
        //NSLog(@"next field: %ld, previous field: %ld", stack_widget_next(stack, focus_widget_id), stack_widget_previous(stack, focus_widget_id));
    }
}






/************
 Notification Handlers
 */


- (NSRect)adjustScroll:(NSRect)proposedVisibleRect
{
    if (_paint_subsys) [self _tellPaintAboutDisplay];
    return proposedVisibleRect;
}


- (void)viewDidEndLiveResize
{
    [self _tellPaintAboutDisplay];
}


- (void)windowDidBecomeKey:(NSNotification *)notification
{
    switch (currentTool)
    {
        case AUTH_TOOL_BROWSE:
            [self setMode:CARDVIEW_MODE_BROWSE];
            break;
        case AUTH_TOOL_BUTTON:
        case AUTH_TOOL_FIELD:
            [self setMode:CARDVIEW_MODE_LAYOUT];
            break;
        default:
            [self setMode:CARDVIEW_MODE_PAINT];
            break;
    }
    [self setNeedsDisplay:YES];
}


- (void)checkLocked
{
    if (!stack) return;
    
    BOOL is_locked = stack_prop_get_long(stack, 0, 0, PROPERTY_LOCKED);
    if (was_locked != is_locked)
    {
        was_locked = is_locked;
        [self _buildCard];
    }
}


- (void)refreshWidgetWithID:(long)inID
{
    quiet_selection_notifications = YES;
    [self _saveSelection];
    [self _buildCard];
    [self _restoreSelection];
    quiet_selection_notifications = NO;
}


- (void)refreshCard
{
    _saved_screen = nil;
    if (_last_effect_slide)
    {
        [_last_effect_slide removeFromSuperview];
        _last_effect_slide = nil;
        //return;
    }
    
    [self _saveSelection];
    [self _buildCard];
    [self _restoreSelection];
}


- (void)notifyToolDidChange:(NSNotification*)notification
{
    if (stack && stackmgr_stack_is_active(stack))
        [self chooseTool:[[JHToolPaletteController sharedController] currentTool]];
}


- (void)notifyToolDoubleClicked:(NSNotification*)notification
{
    switch (currentTool)
    {
        case PAINT_TOOL_FREEPOLY:
        case PAINT_TOOL_FREESHAPE:
        case PAINT_TOOL_OVAL:
        case PAINT_TOOL_RECTANGLE:
        case PAINT_TOOL_ROUNDED_RECT:
            [self toggleDrawFilled:self];
            break;
    }
}


- (void)notifyColourDidChange:(NSNotification*)notification
{
    NSColor *colour = [[JHColourPaletteController sharedController] currentColour];
    if (_paint_subsys) paint_current_colour_set(_paint_subsys,
                                                colour.redComponent * 255.0,
                                                colour.greenComponent * 255.0,
                                                colour.blueComponent * 255.0);
}



- (void)notifyLineSizeDidChange:(NSNotification*)notification
{
    if (_paint_subsys) paint_line_size_set(_paint_subsys, [[JHLineSizePaletteController sharedController] currentSize]);
}


- (void)notifyScriptEditorDidClose:(NSNotification*)notification
{
    [_open_scripts removeObject:[notification object]];
}



@end

