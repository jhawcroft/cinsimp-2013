//
//  JHCardView+VisualEffects.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 23/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView+Base.h"
#import "JHCardView+LayoutManagement.h"

#import <QuartzCore/QuartzCore.h>
#import "JHLayerView.h"


@implementation JHCardView (VisualEffects)


- (void)saveScreen
{
    _saved_screen = [self _pictureOfCard];
}


- (void)releaseScreen
{
    _saved_screen = nil;
    
}


- (void)renderEffect:(int)in_effect speed:(int)in_speed destination:(int)in_dest
{
    /* build destination image */
    
    NSImage *dest_image = nil;
    switch (in_dest)
    {
        case ACU_DESTINATION_CARD:
            dest_image = [self _pictureOfCard];
    }
    
    /* render effect */
    effect_playing = YES;
    if (_auto_status )
        [_auto_status setHidden:YES];
    [self _renderVisualEffect:in_effect speed:in_speed from:_saved_screen to:dest_image];
    _saved_screen = dest_image;
}



- (void)_renderVisualEffect:(int)in_effect speed:(int)in_speed from:(NSImage*)in_begin to:(NSImage*)in_end
{
    /* hide the cursor */
    [NSCursor setHiddenUntilMouseMoves:YES];
    
    /* prepare slides */
    if (_last_effect_slide)
    {
        [_last_effect_slide removeFromSuperview];
        _last_effect_slide = nil;
    }
    [self _destroyLayout];
    JHLayerView *slide_begin = [[JHLayerView alloc] initWithFrameAsSlide:self.bounds picture:in_begin];
    JHLayerView *slide_end = [[JHLayerView alloc] initWithFrameAsSlide:self.bounds picture:in_end];
    [self addSubview:slide_begin];
    [self addSubview:slide_end];
    
    /* set initial state of animation */
    switch (in_effect)
    {
        case ACU_EFFECT_CUT:
            [slide_end setHidden:NO];
            break;
        case ACU_EFFECT_DISSOLVE:
            [slide_end setHidden:YES];
            break;
        case ACU_EFFECT_WIPE_LEFT:
            [slide_end setExposedRect:NSMakeRect(self.bounds.size.width, 0, 0, self.bounds.size.height)];
            [slide_end setFrame:NSMakeRect(self.bounds.size.width, 0, 0, self.bounds.size.height)];
            break;
        case ACU_EFFECT_WIPE_RIGHT:
            [slide_end setExposedRect:NSMakeRect(0, 0, 0, self.bounds.size.height)];
            [slide_end setFrame:NSMakeRect(0, 0, 0, self.bounds.size.height)];
            break;
        case ACU_EFFECT_WIPE_UP:
            [slide_end setExposedRect:NSMakeRect(0, 0, self.bounds.size.width, 0)];
            [slide_end setFrame:NSMakeRect(0, self.bounds.size.height, self.bounds.size.width, 0)];
            break;
        case ACU_EFFECT_WIPE_DOWN:
            [slide_end setExposedRect:NSMakeRect(0, self.bounds.size.height, self.bounds.size.width, 0)];
            [slide_end setFrame:NSMakeRect(0, 0, self.bounds.size.width, 0)];
            break;
        default:
            break;
    }
    
    /* flush pending display changes and display initial state */
    [self setNeedsDisplay:YES];
    [self display];
    
    /* do the animation */
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setCompletionHandler:^{
        /* cleanup */
        if (_auto_status )
            [_auto_status setHidden:NO];
        [slide_begin removeFromSuperview];
        //[slide_end removeFromSuperview];
        _last_effect_slide = slide_end;
        effect_playing = NO;
        if (stack)
            acu_effect_rendered(stack);
    }];
    
    /* set animation speed */
    switch (in_speed)
    {
        case ACU_SPEED_VERY_SLOW:
            [[NSAnimationContext currentContext] setDuration:3.0f];
            break;
        case ACU_SPEED_SLOW:
            [[NSAnimationContext currentContext] setDuration:1.5f];
            break;
        case ACU_SPEED_NORMAL:
            [[NSAnimationContext currentContext] setDuration:0.5f];
            break;
        case ACU_SPEED_FAST:
            [[NSAnimationContext currentContext] setDuration:0.25f];
            break;
        case ACU_SPEED_VERY_FAST:
            [[NSAnimationContext currentContext] setDuration:0.1f];
            break;
        default:
            break;
    }
    
    /* set final state of animation */
    switch (in_effect)
    {
        case ACU_EFFECT_CUT:
            break;
        case ACU_EFFECT_DISSOLVE:
            [[slide_end animator] setHidden:NO];
            break;
        case ACU_EFFECT_WIPE_LEFT:
        case ACU_EFFECT_WIPE_RIGHT:
        case ACU_EFFECT_WIPE_UP:
        case ACU_EFFECT_WIPE_DOWN:
            [[slide_end animator] setExposedRect:self.bounds];
            [[slide_end animator] setFrame:self.bounds];
            break;
        default:
            break;
    }
    
    /* render the animation */
    [NSAnimationContext endGrouping];
}



@end
