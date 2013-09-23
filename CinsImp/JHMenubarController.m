//
//  JHMenubarController.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 29/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHMenubarController.h"

#import "JHCardView.h"
#import "JHDocument.h"


static JHMenubarController *_g_shared_instance = nil;


@implementation JHMenubarController


/**********
 Document Notification Handlers
 */

/* TODO: designLayerChange notification should be handled directly by the menubar controller */
/* notification that the current layer type (card/background) has changed */
- (void)designLayerChange:(NSNotification*)notification
{
    /* card view may be nil if there is no current stack */
    JHCardView *card_view = [notification object]; 
    if (!card_view) return;
    
    /* set the title of the object menu according to what type of layer is being edited;
     provide visual indication to the user */
    if ([card_view isEditingBkgnd])
        [main_object_menu setTitle:NSLocalizedString(@"Background", @"object menu")];
    else
        [main_object_menu setTitle:NSLocalizedString(@"Card", @"object menu")];
    
    /* hide/show the pad-lock for locked stacks */
    if ([card_view stack])
        [main_readonly setHidden:!stack_prop_get_long([card_view stack], 0, 0, PROPERTY_LOCKED)];
}


/* notification that the last document window has been closed */
- (void)lastDocumentClosed:(NSNotification*)notification
{
    [main_readonly setHidden:YES];
    [main_object_menu setTitle:NSLocalizedString(@"Card", @"object menu")];
}


- (void)_loadController
{
    /* register to receive notification when the current layer type (card/background) has changed */
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(designLayerChange:)
                                                 name:designLayerChangeNotification
                                               object:nil];
    
    /* register to receive notification when last document closed */
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(lastDocumentClosed:)
                                                 name:lastDocumentClosedNotification
                                               object:nil];
}


- (id)init
{
    if (_g_shared_instance) return _g_shared_instance;
    
    self = [super init];
    if (!self) return nil;
    
    _g_shared_instance = self;
    [self _loadController];
    
    return self;
}


- (void)buildFontMenu
{
    static bool menu_inited = NO;
    
    if (menu_inited) return;
    menu_inited = YES;
    
    NSMutableArray *font_list = [[NSMutableArray alloc] initWithArray:[[NSFontManager
                                                                        sharedFontManager] availableFontFamilies]];
    [font_list sortUsingSelector:@selector(caseInsensitiveCompare:)];
    for (NSString *font_name in font_list)
    {
        NSMenuItem *the_item = [main_font_menu addItemWithTitle:font_name action:nil keyEquivalent:@""];
        [the_item setAction:@selector(setTheFont:)];
    }
}


- (void)awakeFromNib
{
    if (main_font_menu) [self buildFontMenu];
}


+ (JHMenubarController*)sharedController
{
    if (_g_shared_instance) return _g_shared_instance;
    return [[JHMenubarController alloc] init];
}


- (void)setDebugging:(BOOL)in_debugging
{
    _is_debugging = in_debugging;
    [main_debug setHidden:!in_debugging];
}


- (void)setMode:(int)inMode
{
    _mode = inMode;
    //NSLog(@"mode = %d", inMode);
    switch (inMode)
    {
        case MENU_MODE_PAINT:
            [main_object setHidden:YES];
            [main_paint setHidden:NO];
            [main_font setHidden: YES];
            [main_style setHidden: YES];
            [main_colour setHidden: NO];
            [main_tools setHidden:NO];
            
            [edit_checkpoint setHidden:YES];
            [edit_comment setHidden:YES];
            [edit_uncomment setHidden:YES];
            [edit_script_div setHidden:YES];
            [edit_script_div2 setHidden:YES];
            
            [go_find_again setHidden:YES];
            [go_find_selection setHidden:YES];
            [go_scroll_to_selection setHidden:YES];
            
            [edit_copy_style setHidden:NO];
            [edit_paste_style setHidden:NO];
            [edit_paste_matching_style setHidden:NO];
            [edit_new_card setHidden:NO];
            [edit_delete_card setHidden:NO];
            [edit_lay_div1 setHidden:NO];
            [edit_lay_div2 setHidden:NO];
            [edit_bkgnd setHidden:NO];
            [edit_spell setHidden:NO];
            [edit_subs setHidden:NO];
            [edit_trans setHidden:NO];
            [edit_speech setHidden:NO];
            break;
        case MENU_MODE_SCRIPT:
            [main_object setHidden:YES];
            [main_paint setHidden:YES];
            [main_font setHidden: YES];
            [main_style setHidden: YES];
            [main_colour setHidden: YES];
            [main_tools setHidden:YES];
            
            [edit_checkpoint setHidden:NO];
            [edit_comment setHidden:NO];
            [edit_uncomment setHidden:NO];
            //[edit_script_div setHidden:NO];
            [edit_script_div2 setHidden:NO];
            
            [go_find_again setHidden:NO];
            [go_find_selection setHidden:NO];
            [go_scroll_to_selection setHidden:NO];
            
            [edit_copy_style setHidden:YES];
            [edit_paste_style setHidden:YES];
            [edit_paste_matching_style setHidden:YES];
            [edit_new_card setHidden:YES];
            [edit_delete_card setHidden:YES];
            [edit_lay_div1 setHidden:YES];
            [edit_lay_div2 setHidden:YES];
            [edit_bkgnd setHidden:YES];
            [edit_spell setHidden:YES];
            [edit_subs setHidden:YES];
            [edit_trans setHidden:YES];
            [edit_speech setHidden:YES];
            break;
        case MENU_MODE_DEBUG:
        case MENU_MODE_BROWSE:
        case MENU_MODE_LAYOUT:
        default:
            [main_paint setHidden:YES];
            [main_object setHidden:NO];
            [main_font setHidden: NO];
            [main_style setHidden: NO];
            [main_colour setHidden: YES];
            [main_tools setHidden:NO];
            
            [edit_checkpoint setHidden:YES];
            [edit_comment setHidden:YES];
            [edit_uncomment setHidden:YES];
            [edit_script_div setHidden:YES];
            [edit_script_div2 setHidden:YES];
            
            [go_find_again setHidden:YES];
            [go_find_selection setHidden:YES];
            [go_scroll_to_selection setHidden:YES];
            
            [edit_copy_style setHidden:NO];
            [edit_paste_style setHidden:NO];
            [edit_paste_matching_style setHidden:NO];
            [edit_new_card setHidden:NO];
            [edit_delete_card setHidden:NO];
            [edit_lay_div1 setHidden:NO];
            [edit_lay_div2 setHidden:NO];
            [edit_bkgnd setHidden:NO];
            [edit_spell setHidden:NO];
            [edit_subs setHidden:NO];
            [edit_trans setHidden:NO];
            [edit_speech setHidden:NO];
            break;
    }
}


@end
