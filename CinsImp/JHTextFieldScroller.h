/*
 
 Multiline Text Field
 JHTextFieldScroller.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 View used internally by the CardView to implement multiline scrolling text fields
 
 */

#import <Cocoa/Cocoa.h>

#import "JHWidget.h"

@interface JHTextFieldScroller : NSScrollView <JHWidget, NSTextViewDelegate>
{
    NSRange find_indicator;
    BOOL is_editing;
    NSTrackingArea *trackingArea;
    BOOL dirty;
}

@property (nonatomic) Stack* stack;
@property (nonatomic) long widget_id;
@property (nonatomic) long current_card_id;
@property (nonatomic, weak) id card_view;
@property (nonatomic) BOOL is_selected;
@property (nonatomic) BOOL is_locked;
@property (nonatomic) BOOL is_card;


- (void)setBorder:(NSString*)in_border;
- (void)setFindIndicator:(NSRange)in_range;
- (void)stripFindIndicator;

@end
