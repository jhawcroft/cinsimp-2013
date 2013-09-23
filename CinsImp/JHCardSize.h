/*
 
 Card Size View Controller
 JHCardSize.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Custom view allows drag-resizing and visualisation of a Stack's card size,
 as well as selection from a set of presets.
 
 In future may also allow manual entry using centimetres, milimetres or inches.
 
 (used both in the application's New Stack save panel and the Properties palette)
 
 */

#import <Cocoa/Cocoa.h>

#import "JHCardSizeView.h"

@class JHCardSize;

@protocol JHCardSizeDelegate <NSObject>
@optional
- (void)cardSize:(JHCardSize*)card_size changed:(NSSize)new_size;
@end


@interface JHCardSize : NSViewController <JHCardSizeViewDelegate>
{
    id actionTarget;
    SEL actionSelector;
    NSSize the_card_size;
    NSSize the_last_custom_size;
    
    NSMutableArray *preset_sizes;
    NSMutableArray *preset_titles;
    
    IBOutlet NSMenu *presetMenu;
    IBOutlet NSPopUpButton *presetButton;
    IBOutlet JHCardSizeView *cardSizeView;
    IBOutlet NSTextField *dimensions;
    NSMenuItem *customMenuItem;
}

@property (nonatomic, assign) id <JHCardSizeDelegate> delegate;

- (NSSize)card_size;
- (void)set_card_size:(NSSize)in_size;

@end
