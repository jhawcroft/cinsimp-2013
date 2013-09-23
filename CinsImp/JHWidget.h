/*
 
 Widget Protocol
 JHWidget.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Protocol used internally by the CardView to access the properties of various widgets (buttons & fields)
 
 */

#import <Foundation/Foundation.h>

#include "stack.h"

@protocol JHWidget <NSObject>


/* widget state properties */
@property (nonatomic) Stack* stack;
@property (nonatomic) long widget_id;
@property (nonatomic) long current_card_id;
@property (nonatomic, weak) id card_view;
@property (nonatomic) BOOL is_selected;
@property (nonatomic) BOOL is_locked;
@property (nonatomic) BOOL is_card;

/* set the content of the widget */
- (void)setContent:(NSData*)in_content searchable:(NSString*)in_searchable editable:(BOOL)in_editable;

/* is the widget a button? */
- (BOOL)isButton;

- (NSView*)eventTargetBrowse;


@end
