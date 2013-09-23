/*
 
 Card Size View
 JHCardSize.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHCardSizeView.h"


/*********
 Configuration
 */

/* maximum pixel dimensions of a card */
#define MAX_CARD_WIDTH 5000
#define MAX_CARD_HEIGHT 5000

/* pixel size (square) of the drag handle */
#define DRAG_HANDLE_SIZE 10

/* minimum on-screen size of the box representing the card size
 in pixels */
#define MIN_BOX_SIZE 10




@implementation JHCardSizeView


- (void)setMinatureSize
{
    the_minature_size.width = the_size.width * (fmin(self.bounds.size.width, self.bounds.size.height) / MAX_CARD_WIDTH);
    the_minature_size.height = the_size.height * (fmin(self.bounds.size.width, self.bounds.size.height) / MAX_CARD_HEIGHT);
}


- (id)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self)
    {
        delegate = nil;
        the_size = NSMakeSize(512, 342);
        [self setMinatureSize];
    }
    return self;
}


- (BOOL)isFlipped
{
    return YES;
}


- (void)mouseDown:(NSEvent *)theEvent
{
    NSPoint mouse_loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    
    handle_bottom = ((mouse_loc.y <= the_minature_size.height) && (mouse_loc.y >= the_minature_size.height - DRAG_HANDLE_SIZE));
    handle_right = ((mouse_loc.x <= the_minature_size.width) && (mouse_loc.x >= the_minature_size.width - DRAG_HANDLE_SIZE));
    
    start_drag = mouse_loc;
    start_drag_size = the_minature_size;
}


- (void)mouseDragged:(NSEvent *)theEvent
{
    NSPoint mouse_loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    
    long deltaX = 0, deltaY = 0;
    
    if (handle_bottom) deltaY = mouse_loc.y - start_drag.y;
    if (handle_right) deltaX = mouse_loc.x - start_drag.x;
    
    the_minature_size = NSMakeSize(fmin(fmax(start_drag_size.width + deltaX, MIN_BOX_SIZE), self.bounds.size.width-MIN_BOX_SIZE),
                                   fmin(fmax(start_drag_size.height + deltaY, MIN_BOX_SIZE), self.bounds.size.height-MIN_BOX_SIZE));
    
    the_size = NSMakeSize(the_minature_size.width * (MAX_CARD_WIDTH / fmin(self.bounds.size.width, self.bounds.size.height)),
                          the_minature_size.height * (MAX_CARD_HEIGHT / fmin(self.bounds.size.width, self.bounds.size.height)));
    
    the_size.width = (int)the_size.width / 2 * 2;
    the_size.height = (int)the_size.height / 2 * 2;
    
    if (delegate) [delegate cardSizeView:self changed:the_size finished:NO];
    
    [self setNeedsDisplay:YES];
}


- (void)mouseUp:(NSEvent *)theEvent
{
    if (delegate) [delegate cardSizeView:self changed:the_size finished:YES];
}


- (void)drawRect:(NSRect)dirtyRect
{
    [[NSColor darkGrayColor] setFill];
    NSRectFill(self.bounds);
    
    [[NSColor whiteColor] setFill];
    NSRectFill(NSMakeRect(0, 0, (int)the_minature_size.width, (int)the_minature_size.height));
    [[NSColor blackColor] setFill];
    NSFrameRect(NSMakeRect(0, 0, (int)the_minature_size.width, (int)the_minature_size.height));
}


- (void)set_card_size:(NSSize)in_size
{
    the_size = in_size;
    [self setMinatureSize];
    [self setNeedsDisplay:YES];
}


@end
