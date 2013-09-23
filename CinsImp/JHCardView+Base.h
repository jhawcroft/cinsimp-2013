/*
 
 Stack's Card View
 JHCardView.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Display and editing hub for a stack; draws the current card and is responsible for layout
 
 */

#import <Cocoa/Cocoa.h>

#import "JHWidget.h"
#import "JHSelectorView.h"

#include "stack.h"
#include "paint.h"


#import "JHScriptEditorController.h"


/**********
 Constants
 */


extern NSString *designLayerChangeNotification;


/* tool selection */
enum JHTool
{
    kJHToolBrowse = 0,
    kJHToolButton,
    kJHToolField,    
};


/* type of selected object(s) */
enum SelectionType
{
    SELECTION_NONE = 0,
    SELECTION_STACK,
    SELECTION_BKGND,
    SELECTION_CARD,
    SELECTION_WIDGETS, /* all widgets within the same layer, ie. card/bkgnd */
};


#define CARDVIEW_MODE_BROWSE 0
#define CARDVIEW_MODE_LAYOUT 1
#define CARDVIEW_MODE_PAINT  2



/**********
 Class
 */

@interface JHCardView : NSView
{
    /* template widgets; configured in interface builder, these are the templates from
     which all widgets (buttons and fields) are built */
    IBOutlet NSView *templateWidgetSelector;
    IBOutlet NSView *templateWidgetTextSimple;
    IBOutlet NSView *templateWidgetPushButton;
    IBOutlet NSView *templateWidgetClearButton;
    IBOutlet NSView *templateWidgetCheckbox;
    IBOutlet NSView *templateWidgetComboBox;
    
    /* mode: browse, layout or paint */
    int _edit_mode;
    
    /* if we're in paint mode, we need a cache of a couple of pictures */
    NSImage *_paint_predraw_cache;
    NSImage *_paint_postdraw_cache;
    BOOL _disable_drawing_backdrop;
    
    /* paint subsystem instance, when in paint mode */
    Paint *_paint_subsys;
    BOOL _paint_suspended;
    
    /* event dispatch */
    //__weak NSView *target_view;
    //__weak NSEvent *dispatching_event;
    
    /* selection */
    NSMutableArray *dragTargets;
    enum SelectionType selection_type;
    NSMutableArray *saved_selection;
    enum SelectionType saved_selection_type;
    
    /* drag tracking */
    NSPoint dragStart;
    BOOL dragIsResize;
    BOOL did_drag;
    
    /* layout guidelines */
    long snap_line_x;
    long snap_line_y;
    
    /* current tool selection */
    int currentTool;
    
    /* mouse location tracking */
    NSTrackingArea *trackingArea;
    
    /* displayed/edited stack */
    Stack *stack;
    
    /* what are we looking at? */
    //long current_card_id;
    //long current_bkgnd_id;
    long card_index;
    
    /* edit background mode */
    bool edit_bkgnd;
    
    /* keeps the content of the stack hidden if the stack is access protected;
     is flipped OFF when the correct password is entered */
    BOOL protected_hidden;
    
    /* state of 'peek' feature; depends on modifier keys pressed */
    BOOL peek_flds;
    BOOL peek_btns;
    
    /* tracks if the stack is locked */
    BOOL was_locked;
    
    /* field which has focus */
    long focus_widget_id;
    
    /* show tab order? */
    BOOL show_sequence;
    
    /* inhibit selection change notifications during refreshes?
     (may not be needed in future if we refresh more efficiently...) */
    BOOL quiet_selection_notifications;
    
    /* what to draw; sometimes we want to draw parts of the card into an offscreen context
     for rendering visual effects, or setting up the Paint sub-system for editing */
    __weak NSView *layer_card;
    __weak NSView *layer_bkgnd;
    __weak NSView *_layer_overlay;
    
    NSView *_auto_status;
    
    /* open scripts */
    NSMutableArray *_open_scripts;
    
    /* cached screen image */
    NSImage *_saved_screen;
    NSView *_last_effect_slide;
    BOOL _render_actual_card;
    BOOL effect_playing;
    
    /* script debug close delay */
    NSTimer *_script_debug_close_timer;
}


/* initalization */
- (void)setStack:(Stack*)inStack;
- (Stack*)stack;

/* protected access */
- (BOOL)securityEnabled;
- (void)disableSecurity;

/* sets the cursor appropriate to where the pointer is located;
 editability of the widget beneath the pointer and irrespective of which window is key */
- (void)_setAppropriateCursor;

/* invoked when the window is activated and after the Protect Stack sheet is dismissed;
 gives view opportunity to check display is appropriate in case settings have changed.
 Ideally will do this a different way, perhaps by callback through Stack*, thus not 
 requiring any special coupling in Objective-C (even the name is inappropriate probably) */
- (void)checkLocked;

/* ID for the currently displayed card */
- (long)currentCardID;

/* refresh display as something has been changed; invoked by properties palette usually
 when a change has been made */
- (void)refreshWidgetWithID:(long)inID;
- (void)refreshCard;

/* are we in Edit Background mode? */
- (BOOL)isEditingBkgnd;


/* focus */
- (void)_handleTab:(NSEvent *)event;
- (void)_handleBackTab:(NSEvent *)event;
- (void)_handleFieldFocusChange:(NSNumber*)in_widget_id;

/* services */
- (void)chooseTool:(int)inTool;
- (void)goToCard:(long)in_id;

- (int)currentTool;



@end
