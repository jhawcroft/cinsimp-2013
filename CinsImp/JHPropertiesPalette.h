/*
 
 Properties Palette Controller
 JHPropertiesPalette.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Window controller for the properties inspector palette
 
 */

#import <Cocoa/Cocoa.h>

#import "JHPropertiesView.h"
#import "JHCardView.h"
#import "JHCardSize.h"
#import "JHIconGrid.h"

#include "stack.h"



@interface JHPropertiesPalette : NSWindowController <JHCardSizeDelegate>
{
    NSMenu *font_menu;
    
    IBOutlet JHPropertiesView *paneRoot;
    
    enum SelectionType selection_type;
    
    BOOL is_loading;
    
    IBOutlet NSView *paneBlank;
    
    IBOutlet NSView *paneButton;
    IBOutlet NSView *paneFieldsMany;
    IBOutlet NSView *paneFieldText;
    IBOutlet NSView *paneFieldCheck;
    
    IBOutlet NSView *paneStack;
    IBOutlet NSView *paneBkgnd;
    IBOutlet NSView *paneCard;
    IBOutlet NSView *paneIcon;
    
    Stack *stack;
    JHCardView *card_view;
    long current_card_id;
    long current_bkgnd_id;
    NSArray *selected_ids;
    
    IBOutlet NSTextField *paneStackDesc;
    IBOutlet NSView *paneStackCardSizeHole;
    JHCardSize *cardSize;
    
    IBOutlet NSTextField *paneButtonDesc;
    IBOutlet NSPopUpButton *paneButtonStyle;
    IBOutlet NSTextField *paneButtonName;
    IBOutlet NSTextField *paneButtonIconDesc;
    BOOL _grab_icon;
    IBOutlet NSButton *paneButtonSetIcon;
    
    IBOutlet NSTextField *paneFieldsDesc;
    IBOutlet NSPopUpButton *paneFieldsStyle;
    IBOutlet NSTextField *paneFieldsName;
    IBOutlet NSButton *paneFieldsLocked;
    
    IBOutlet NSTextField *paneFieldTextDesc;
    IBOutlet NSPopUpButton *paneFieldTextStyle;
    IBOutlet NSPopUpButton *paneFieldTextBorder;
    IBOutlet NSTextField *paneFieldTextName;
    IBOutlet NSButton *paneFieldTextLocked;
    IBOutlet NSButton *paneFieldTextDontSearch;
    IBOutlet NSButton *paneFieldTextShared;
    IBOutlet NSPopUpButton *paneFieldTextFont;
    IBOutlet NSTextField *paneFieldTextSize;
    IBOutlet NSButton *paneFieldTextBold;
    IBOutlet NSButton *paneFieldTextItalic;
    IBOutlet NSButton *paneFieldTextRich;
    
    IBOutlet NSTextField *paneFieldCheckDesc;
    IBOutlet NSPopUpButton *paneFieldCheckStyle;
    IBOutlet NSTextField *paneFieldCheckName;
    IBOutlet NSButton *paneFieldCheckShared;
    
    IBOutlet NSTextField *paneCardDesc;
    IBOutlet NSTextField *paneCardNumber;
    IBOutlet NSTextField *paneCardName;
    IBOutlet NSButton *paneCardDontSearch;
    IBOutlet NSButton *paneCardCantDelete;
    
    IBOutlet NSTextField *paneBkgndDesc;
    IBOutlet NSTextField *paneBkgndCards;
    IBOutlet NSTextField *paneBkgndName;
    IBOutlet NSButton *paneBkgndDontSearch;
    IBOutlet NSButton *paneBkgndCantDelete;
    
    JHIconGrid *icon_grid;
    int icon_id;
    IBOutlet NSTextField *paneIconID;
    IBOutlet NSTextField *paneIconName;
    
    
    BOOL _was_visible_se;
}

- (id)initWithWindowNibName:(NSString *)windowNibName;

- (IBAction)paneChangedName:(NSTextField*)sender;

- (IBAction)paneChangedLocked:(NSButton*)sender;
- (IBAction)paneChangedShared:(NSButton*)sender;
- (IBAction)paneChangedDontSearch:(NSButton*)sender;
- (IBAction)paneChangedCantDelete:(NSButton*)sender;

- (IBAction)paneChangedRichText:(id)sender;

- (IBAction)editScriptOfSelection:(id)sender;


+ (JHPropertiesPalette*)sharedInstance;


@end
