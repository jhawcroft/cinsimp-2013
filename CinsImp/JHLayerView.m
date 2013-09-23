/*
 
 Card / Background Layer Picture
 JHLayerPicture.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHLayerView.h"

//#import "JHCardView+UIDrawing.m"

@interface JHCardView
- (void)drawOverlay;
@end


@implementation JHLayerView


- (id)initWithFrame:(NSRect)inFrameRect picture:(NSImage*)inPicture isBackground:(BOOL)inIsBkgnd
{
    self = [super initWithFrame:inFrameRect];
    if (self)
    {
        _picture = inPicture;
        _is_bkgnd = inIsBkgnd;
        _is_overlay = NO;
        _is_effect_presenter = NO;
    }
    return self;
}


- (id)initWithFrameAsOverlay:(NSRect)inFrameRect
{
    self = [super initWithFrame:inFrameRect];
    if (self)
    {
        _picture = nil;
        _is_bkgnd = NO;
        _is_overlay = YES;
        _is_effect_presenter = NO;
    }
    return self;
}


- (id)initWithFrameAsSlide:(NSRect)inFrameRect picture:(NSImage*)inPicture
{
    self = [super initWithFrame:inFrameRect];
    if (self)
    {
        _picture = inPicture;
        _is_bkgnd = NO;
        _is_overlay = NO;
        _is_effect_presenter = YES;
        //_exposed_rect = inFrameRect;
    }
    return self;
}


- (BOOL)isFlipped
{
    return YES;
}


- (BOOL)isOpaque
{
    return NO;
}


- (void)setExposedX:(float)in_x
{
    _exposed_x = in_x;
    [self setNeedsDisplay:YES];
}
- (float)exposedX
{
    return _exposed_x;
}
- (void)setExposedWidth:(float)in_width
{
    _exposed_width = in_width;
    [self setNeedsDisplay:YES];
}
- (float)exposedWidth
{
    return _exposed_width;
}

- (void)setExposedRect:(NSRect)in_rect
{
    _exposed = in_rect;
    [self setNeedsDisplay:YES];
}


- (NSRect)exposedRect
{
    return _exposed;
}


+ (id)defaultAnimationForKey:(NSString *)key {
    if ([key isEqualToString:@"exposedX"] || [key isEqualToString:@"exposedWidth"] || [key isEqualToString:@"exposedRect"]) {
        
        // By default, animate border color changes with simple linear interpolation to the new color value.
        return [CABasicAnimation animation];
    }
    //} else {
        // Defer to super's implementation for any keys we don't specifically handle.
     //   return [super defaultAnimationForKey:key];
   // }
    return [super defaultAnimationForKey:key];
}


- (void)drawRect:(NSRect)dirtyRect
{
    /* draw picture (if any) */
    if (_picture)
    {
        if (_is_effect_presenter)
        {
            //NSLog(@"drawing frame");
            //[[NSColor redColor] setFill];
            //NSRectFill(self.bounds);
            
            [_picture drawInRect:self.bounds
                        fromRect:NSMakeRect(_exposed.origin.x, _exposed.origin.y,
                                            _exposed.size.width, _exposed.size.height)
                       operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
            
            //[_picture drawInRect:self.bounds
            //            fromRect:NSMakeRect(_exposed_x, 0, _exposed_width, self.bounds.size.height)
            //           operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
            return;
        }
        
        [_picture drawInRect:self.bounds fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
        return;
    }
    
    /* if we are an overlay, draw that */
    if (_is_overlay)
    {
        //NSLog(@"draw overlay");
        JHCardView *card_view = (JHCardView*)[self superview];
        if (card_view) [card_view drawOverlay];
    }
    
    
    /* draw test patterns */
    /*if (_is_bkgnd)
    {
        NSColor *color = [NSColor colorWithDeviceRed:0.5 green:0.5 blue:0.5 alpha:0.2];
        [color setFill];
        for (int x = 0; x < self.bounds.size.width; x += 40)
        {
            NSRectFillUsingOperation(NSMakeRect(x, 0, 20, self.bounds.size.height), NSCompositeSourceOver);
        }
    }
    else
    {
        NSColor *color = [NSColor colorWithDeviceRed:1 green:1 blue:0 alpha:0.3];
        [color setFill];
        for (int x = 20; x < self.bounds.size.width; x += 40)
        {
            NSRectFillUsingOperation(NSMakeRect(x, 0, 20, self.bounds.size.height), NSCompositeSourceOver);
        }
    }*/
}


/* don't allow the layer to receive Cocoa events */
- (NSView*)hitTest:(NSPoint)aPoint
{
    return nil;
}



@end
