//
//  JHProgressView.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 29/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHProgressView.h"

@implementation JHProgressView


#define _INDICATOR_WIDTH  50
#define _INDICATOR_HEIGHT 50


- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        NSRect indicator_rect = NSMakeRect(0, 0,
                                           _INDICATOR_WIDTH, _INDICATOR_HEIGHT);
        NSProgressIndicator *indicator = [[NSProgressIndicator alloc] initWithFrame:indicator_rect];
        
        [indicator setControlTint:NSClearControlTint];
        [indicator setStyle:NSProgressIndicatorSpinningStyle];
        [indicator sizeToFit];
        
        [indicator setFrameOrigin:NSMakePoint((frame.size.width - indicator.frame.size.width) / 2,
                                              (frame.size.height - indicator.frame.size.height) / 2)];

        [self addSubview:indicator];
        
        [indicator startAnimation:self];
    }
    
    return self;
}


- (BOOL)isOpaque
{
    return NO;
}


- (BOOL)isFlipped
{
    return YES;
}


- (void)drawRect:(NSRect)dirtyRect
{
    NSColor *gray = [NSColor colorWithDeviceRed:0 green:0 blue:0 alpha:0.6];
    [gray setFill];
    NSRectFill(self.bounds);
}

@end
