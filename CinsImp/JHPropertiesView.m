/*
 
 Properties Palette Base View
 JHPropertiesView.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHPropertiesView.h"

@implementation JHPropertiesView


/********
 Custom View Configuration
 */

- (BOOL)isFlipped
{
    return YES;
}


/********
 Initalization
 */

- (void)initCustomView
{
    pane = nil;
}


- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) [self initCustomView];
    return self;
}


- (id)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self) [self initCustomView];
    return self;
}


/********
 Content Pane Switching
 */

- (void)setPane:(NSView*)inView
{
    if (pane) [pane removeFromSuperview];
    pane = inView;
    if (pane) [self addSubview:pane];
}



@end
