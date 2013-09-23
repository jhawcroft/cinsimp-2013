/*
 
 Selector View
 JHSelectorView.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 View used internally by the CardView to overlay selected widget(s) (fields, buttons) 
 to make the selection visibile, manage the selection and display drag handles
 
 */

#import <Cocoa/Cocoa.h>

#import "JHWidget.h"


/* offset from the edge of the underlying widget */
extern const float kJHFrameOffset;


@interface JHSelectorView : NSView
{
    NSTrackingArea *trackingArea;
    NSRect resizeHandleRect;
}

@property (nonatomic) NSView<JHWidget> *widget;

/* the layout has been modified */
- (void)setNewOrigin:(NSPoint)inOrigin;
- (void)setNewSize:(NSSize)inSize;

/* is the mouse within the resize handle? */
- (BOOL)mouseWithinResizeHandle:(NSPoint)inMouseLoc;


@end
