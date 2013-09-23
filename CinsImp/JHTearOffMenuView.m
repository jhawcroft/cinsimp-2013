//
//  JHTearOffMenuView.m
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 23/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHTearOffMenuView.h"


#define _GHOST_ALPHA 0.6
#define _SAFETY_MARGIN 20


@implementation JHTearOffMenuView


- (void)awakeFromNib
{
   [self setup];
}


- (void)setup
{
    if ((!palette) || (!theView) || (!ghostPalette)) return;
    
    [self setFrameSize:NSMakeSize(50, 50)];
    [self addSubview:theView];
    
    //[self setFrame:theView.frame];
    //[theView setFrameOrigin:NSMakePoint(0, 0)];
  
    [ghostPalette setAlphaValue:_GHOST_ALPHA];
    [ghostPalette setOpaque:NO];
    //[ghostPalette setFrame:[ghostPalette frameRectForContentRect:[theView frame]] display:NO];
}


- (void)timeToCheckForTearOff:(id)sender
{
    
    /* don't tear-off if the mouse button(s) not pressed */
    if ([NSEvent pressedMouseButtons] == 0) return;
    
    /* check where mouse is relative to menu */
    NSPoint global_loc = [NSEvent mouseLocation];
    NSPoint mouse_loc = [self.window convertScreenToBase:global_loc];
    
    /* get the bounds of the menu + a safety margin around the edges */
    NSRect container_rect = self.bounds;
    container_rect.origin.x -= _SAFETY_MARGIN;
    container_rect.size.width += _SAFETY_MARGIN * 2;
    container_rect.origin.y -= _SAFETY_MARGIN;
    container_rect.size.height += (_SAFETY_MARGIN * 3);
    
    /* is the mouse within the boundaries of the menu;
     or the safety margin? */
    
    if (NSPointInRect(mouse_loc, container_rect))
    {
        if (!_in_menu)
        {
            /* mouse entered menu; hide my_ghost */
            _in_menu = YES;
            track_palette = NO;
            [ghostPalette orderOut:self];
        }
    }
    else
    {
        if (_in_menu)
        {
            /* mouse exited menu */
            _in_menu = NO;
            track_palette = YES;
                
            /* show my_ghost centred at location of mouse */
            [ghostPalette setFrameOrigin:NSMakePoint(global_loc.x - (ghostPalette.frame.size.width / 2),
                                              global_loc.y - (ghostPalette.frame.size.height / 2))];
            
            [ghostPalette orderFront:self];
        }
    }
    
    /* move my_ghost, centred at location of mouse */
    if (track_palette)
        [ghostPalette setFrameOrigin:NSMakePoint(global_loc.x - (ghostPalette.frame.size.width / 2),
                                          global_loc.y - (ghostPalette.frame.size.height / 2))];
}


- (void)viewDidMoveToWindow
{
    if (self.window)
    {
        
        [self setFrame:theView.frame];
        [theView setFrameOrigin:NSMakePoint(0, 0)];
     
        
        /* menu is opening;
         create a timer to track the mouse position;
         (we need to track the mouse outside of the menu bounds) */
        _timer = [NSTimer timerWithTimeInterval:0.1
                                         target:self
                                       selector:@selector(timeToCheckForTearOff:)
                                       userInfo:nil
                                        repeats:YES];
        [[NSRunLoop currentRunLoop] addTimer:_timer forMode:NSEventTrackingRunLoopMode];
        
        /* set some state variables */
        _in_menu = NO;
        track_palette = NO;
    }
    else
    {
        /* menu is closing;
         remove the tracking timer */
        [_timer invalidate];
        _timer = nil;
        
        /* if the tear-off hasn't begun, no need to do anything further */
        if (!ghostPalette.isVisible) return;
        
        /* the mouse button(s) must be pressed to complete a tear-off;
         otherwise we cancel the tear-off */
        if ([NSEvent pressedMouseButtons] != 0)
        {
            [ghostPalette orderOut:self];
            return;
        }
        
        /* if the palette is already on screen, move it to the tear-off location */
        if (palette.isVisible)
            [palette setFrame:ghostPalette.frame display:YES animate:YES];
        
        /* otherwise, position the palette over the tear-off my_ghost and show it */
        else
        {
            [palette setFrameOrigin:ghostPalette.frame.origin];
            [palette orderFront:self];
        }
        
        /* hide the tear-off my_ghost */
        [ghostPalette orderOut:self];
    }
}


@end
