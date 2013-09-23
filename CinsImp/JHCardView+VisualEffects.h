//
//  JHCardView+VisualEffects.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 23/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView+Base.h"
#include "acu.h"


@interface JHCardView (VisualEffects)

- (void)saveScreen;
- (void)releaseScreen;

- (void)renderEffect:(int)in_effect speed:(int)in_speed destination:(int)in_dest;

- (void)_renderVisualEffect:(int)in_effect speed:(int)in_speed from:(NSImage*)in_begin to:(NSImage*)in_end;

@end
