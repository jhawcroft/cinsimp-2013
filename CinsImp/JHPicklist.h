/*
 
 Picklist
 JHPicklist.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 View used internally by the CardView to implement Picklists
 
 */

#import <Cocoa/Cocoa.h>

#import "JHWidget.h"

@interface JHPicklist : NSComboBox <JHWidget>

@property (nonatomic) Stack* stack;
@property (nonatomic) long widget_id;
@property (nonatomic) long current_card_id;
@property (nonatomic, weak) id card_view;
@property (nonatomic) BOOL is_selected;
@property (nonatomic) BOOL is_locked;
@property (nonatomic) BOOL is_card;

@end
