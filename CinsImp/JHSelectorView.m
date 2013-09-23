/*
 
 Selector View
 JHSelectorView.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHSelectorView.h"

@implementation JHSelectorView


/*********
 Configuration
 */

/* size of drag handles */
const float kHandleSize = 10.0;

/* offset from the edge of the underlying widget */
const float kJHFrameOffset = 2.0;




/*********
 View Configuration
 */

- (BOOL)isFlipped
{
    return YES;
}

- (BOOL)isOpaque
{
    return NO;
}


/*********
 Initalization
 */

- (void)viewDidMoveToWindow {
    // trackingRect is an NSTrackingRectTag instance variable
    // eyeBox is a region of the view (instance variable)
    if (![self window]) return;
    
    trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds options:(NSTrackingActiveAlways | NSTrackingCursorUpdate | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved)
                                                  owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
    
}

- (void)viewWillMoveToWindow:(NSWindow *)newWindow {
    if ( [self window] && trackingArea ) {
        [self removeTrackingArea:trackingArea];
    }
}



/*********
 Resize
 */

- (void)updateTrackingAreas
{
    [self removeTrackingArea:trackingArea];
    trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds options:(NSTrackingActiveInKeyWindow | NSTrackingCursorUpdate | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved)
                                                  owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}



/*********
 Drag
 */

- (BOOL)mouseWithinResizeHandle:(NSPoint)inMouseLoc
{
    resizeHandleRect = NSMakeRect(self.bounds.size.width - kHandleSize, self.bounds.size.height - kHandleSize, kHandleSize, kHandleSize);
    return NSPointInRect(inMouseLoc, resizeHandleRect);
}


- (void)mouseDown:(NSEvent *)theEvent
{
    [[self superview] mouseDown:theEvent];
}



/*********
 Drawing
 */

- (void)drawRect:(NSRect)dirtyRect
{
    [[NSColor colorWithCalibratedRed:51.0/255.0 green:204.0/255.0 blue:255.0/255.0 alpha:0.5] setFill];
    NSRectFillUsingOperation(self.bounds, NSCompositeSourceOver);

    [[NSColor blackColor] setFill];
    resizeHandleRect = NSMakeRect(self.bounds.size.width - kHandleSize, self.bounds.size.height - kHandleSize, kHandleSize, kHandleSize);
    NSRectFill(resizeHandleRect);
    
}



/*********
 Layout Changes
 */

- (void)setFrame:(NSRect)frameRect
{
    [super setFrame:frameRect];
}


- (void)setNewOrigin:(NSPoint)inOrigin
{
    [self setFrameOrigin:inOrigin];
    [self.widget setFrameOrigin:NSMakePoint(inOrigin.x + kJHFrameOffset, inOrigin.y + kJHFrameOffset)];
}


- (void)setNewSize:(NSSize)inSize
{
    [self setFrameSize:inSize];
    [self.widget setFrameSize:NSMakeSize(inSize.width - kJHFrameOffset * 2, inSize.height - kJHFrameOffset * 2)];
}



@end
