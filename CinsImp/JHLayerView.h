/*
 
 Card / Background Layer Picture
 JHLayerPicture.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Displays either a card/background layer picture during browse or layout modes;
 and top-level view for drawing of layout guides and widget outlines
 
 */

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>


@interface JHLayerView : NSView <NSAnimatablePropertyContainer>
{
    NSImage *_picture;
    BOOL _is_bkgnd;
    BOOL _is_overlay;
    BOOL _is_effect_presenter;
    float _exposed_x;
    float _exposed_width;
    NSRect _exposed;
}

- (id)initWithFrame:(NSRect)inFrameRect picture:(NSImage*)inPicture isBackground:(BOOL)inIsBkgnd;
- (id)initWithFrameAsOverlay:(NSRect)inFrameRect;
- (id)initWithFrameAsSlide:(NSRect)inFrameRect picture:(NSImage*)inPicture;

- (void)setExposedRect:(NSRect)in_rect;
- (NSRect)exposedRect;

- (void)setExposedX:(float)in_x;
- (float)exposedX;
- (void)setExposedWidth:(float)in_width;
- (float)exposedWidth;


@end
