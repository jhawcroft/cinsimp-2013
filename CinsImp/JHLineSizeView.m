//
//  JHLineSizeView.m
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 24/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHLineSizeView.h"
#import "JHLineSizePaletteController.h"


#define _LEFT_PADDING 25
#define _TOP_PADDING 8
#define _SAMPLE_GAP 20
#define _SAMPLE_HEIGHT 40
#define _SIZE_COUNT 6
#define _SELECTOR_PADDING_VERT 4
#define _SELECTOR_PADDING_HORZ 8


@implementation JHLineSizeView


- (void)loadPalette
{
    [self setFrameSize:NSMakeSize(_LEFT_PADDING * 2 + _SAMPLE_GAP * (_SIZE_COUNT - 1) + _SIZE_COUNT,
                                  _TOP_PADDING * 2 + _SAMPLE_HEIGHT + 1)];
    
    [self addTrackingArea:[[NSTrackingArea alloc]
                           initWithRect:self.bounds
                           options:NSTrackingMouseEnteredAndExited |
                           NSTrackingActiveInActiveApp |
                           NSTrackingMouseMoved
                           owner:self
                           userInfo:nil]];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(notifyLineSizeDidChange:)
                                                 name:notifyLineSizeDidChange
                                               object:nil];
}


- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) [self loadPalette];
    return self;
}


- (void)viewDidMoveToWindow
{
    if (self.window)
    {        
        [(NSPanel*)[self window] setFloatingPanel:YES];
        [(NSPanel*)[self window] setBecomesKeyOnlyIfNeeded:YES];
        
        NSButton *button;
        button = [[self window] standardWindowButton:NSWindowMiniaturizeButton];
        [button setHidden:YES];
        button = [[self window] standardWindowButton:NSWindowZoomButton];
        [button setHidden:YES];
        
        [[self window] setContentSize:self.frame.size];
    }
}


- (BOOL)isFlipped
{
    return YES;
}


- (void)mouseExited:(NSEvent *)theEvent
{
    hover_index = -1;
}


- (void)mouseMoved:(NSEvent *)theEvent
{
    NSPoint loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    if (!NSPointInRect(loc, NSMakeRect(_LEFT_PADDING - (_SAMPLE_GAP/2),
                                       _TOP_PADDING,
                                       _SAMPLE_GAP * _SIZE_COUNT + (_SAMPLE_GAP/2),
                                       _SAMPLE_HEIGHT)))
    {
        hover_index = -1;
        return;
    }
    int line_index = (loc.x - _LEFT_PADDING + (_SAMPLE_GAP/2)) / _SAMPLE_GAP;
    if (line_index < 0) line_index = 0;
    if (line_index >= _SIZE_COUNT) line_index = _SIZE_COUNT-1;
    hover_index = line_index;
}


- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}


- (void)mouseDown:(NSEvent *)theEvent
{
    if (hover_index < 0) return;
    line_size = hover_index + 1;
    [self setNeedsDisplay:YES];
    
    [[JHLineSizePaletteController sharedController] setCurrentSize:line_size];
}


- (void)drawRect:(NSRect)dirtyRect
{
    [[NSColor whiteColor] setFill];
    NSRectFill(self.bounds);
    
    int x = _LEFT_PADDING;
    for (int sz = 1; sz <= 6; sz++)
    {
        [[NSColor blackColor] setFill];
        NSRectFill(NSMakeRect(x, _TOP_PADDING, sz, _SAMPLE_HEIGHT));
        
        if (line_size == sz)
        {
            NSFrameRectWithWidth(NSMakeRect(x - _SELECTOR_PADDING_HORZ, _TOP_PADDING - _SELECTOR_PADDING_VERT,
                                            _SELECTOR_PADDING_HORZ * 2 + sz, _SAMPLE_HEIGHT + 2 * _SELECTOR_PADDING_VERT), 2);
        }
        
        x += _SAMPLE_GAP;
    }
}


- (void)notifyLineSizeDidChange:(NSNotification*)notification
{
    line_size = [[JHLineSizePaletteController sharedController] currentSize];
}


@end
