/*
 
 Objective-C Globals
 jhglobals.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Application global delcarations (Objective-C) for use in other parts of the application
 
 */

#import <Foundation/Foundation.h>

#include "xtalk_engine.h"


/* card view of current stack */
@class JHCardView;
extern JHCardView *gCurrentCardView;
//extern XTE *gXTalk;

extern XTE *g_app_xtalk;


/* cursors */
extern NSCursor *gCursor_arrow;
extern NSCursor *gCursor_hand;
extern NSCursor *gCursor_ibeam;


/* font menu */
NSMenu* create_font_menu(void);


/* message palette */
//@class JHMessagePalette;
//extern JHMessagePalette *gMessagePalette;


#include "jh_c_int.h"

