//
//  JHMenubarController.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 29/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Foundation/Foundation.h>


#define MENU_MODE_BROWSE 0
#define MENU_MODE_LAYOUT 1
#define MENU_MODE_PAINT  2
#define MENU_MODE_SCRIPT 3
#define MENU_MODE_DEBUG  4


@interface JHMenubarController : NSObject
{
    IBOutlet NSMenuItem *main_paint;
    IBOutlet NSMenuItem *main_object;
    IBOutlet NSMenu *main_object_menu;
    
    IBOutlet NSMenu *main_font_menu;
    IBOutlet NSMenuItem *main_font;
    IBOutlet NSMenuItem *main_colour;
    IBOutlet NSMenuItem *main_style;
    
    IBOutlet NSMenuItem *main_tools;
    
    IBOutlet NSMenuItem *edit_script_div;
    IBOutlet NSMenuItem *edit_comment;
    IBOutlet NSMenuItem *edit_uncomment;
    IBOutlet NSMenuItem *edit_script_div2;
    IBOutlet NSMenuItem *edit_checkpoint;
    
    IBOutlet NSMenuItem *go_find_again;
    IBOutlet NSMenuItem *go_find_selection;
    IBOutlet NSMenuItem *go_scroll_to_selection;
    
    IBOutlet NSMenuItem *edit_copy_style;
    IBOutlet NSMenuItem *edit_paste_style;
    IBOutlet NSMenuItem *edit_paste_matching_style;
    IBOutlet NSMenuItem *edit_new_card;
    IBOutlet NSMenuItem *edit_delete_card;
    IBOutlet NSMenuItem *edit_lay_div1;
    IBOutlet NSMenuItem *edit_lay_div2;
    IBOutlet NSMenuItem *edit_bkgnd;
    IBOutlet NSMenuItem *edit_spell;
    IBOutlet NSMenuItem *edit_subs;
    IBOutlet NSMenuItem *edit_trans;
    IBOutlet NSMenuItem *edit_speech;
    
    IBOutlet NSMenuItem *main_debug;
    
    IBOutlet NSMenuItem *main_readonly;
    
    int _mode;
    BOOL _is_debugging;
}

+ (JHMenubarController*)sharedController;

- (void)setMode:(int)inMode;
- (void)setDebugging:(BOOL)in_debugging;


@end
