/*
 
 Properties Palette Base View
 JHPropertiesView.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Base container view for properties palette;
 other views for each type of editable object appear within this view
 
 */

#import <Cocoa/Cocoa.h>

@interface JHPropertiesView : NSView
{
    /* pointer to the current content pane view */
    NSView *pane;
}


/* set the content pane to the specified view */
- (void)setPane:(NSView*)inView;


@end
