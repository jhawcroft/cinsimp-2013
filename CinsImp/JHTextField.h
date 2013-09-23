/*
 
 Text Field
 JHTextField.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 View used internally by the Multiline Text Field to implement multiline scrolling text fields
 
 */

#import <Cocoa/Cocoa.h>

#import "JHCardView.h"

@interface JHTextField : NSTextView <NSTextViewDelegate>
{
    /*NSDictionary *_selection_atts;
    NSFont *_selection_font;
    NSFontTraitMask _selection_traits;*/
}

@property (nonatomic, weak) id card_view;
@property (nonatomic) long widget_id;


- (void)setTheFont:(id)sender;



@end
