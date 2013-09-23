/*
 
 Card Size View
 JHCardSize.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Custom view employed for the visual aspect of the Card Size View Controller;
 not intended for direct use by other parts of the application
 
 */

#import <Cocoa/Cocoa.h>

@class JHCardSizeView;

@protocol JHCardSizeViewDelegate <NSObject>
@optional
- (void)cardSizeView:(JHCardSizeView*)the_view changed:(NSSize)the_size finished:(BOOL)is_finished;
@end


@interface JHCardSizeView : NSView
{
    NSSize the_size;
    NSSize the_minature_size;
    NSSize start_drag_size;
    BOOL handle_bottom;
    BOOL handle_right;
    NSPoint start_drag;
    IBOutlet id <JHCardSizeViewDelegate> delegate;
}

- (void)set_card_size:(NSSize)in_size;

@end
