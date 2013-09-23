//
//  JHCursorController.m
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 25/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCursorController.h"


static JHCursorController *_g_shared_instance = nil;


@implementation JHCursorController


- (void)_loadCursors
{
    NSImage *image;
    
    image = [NSImage imageNamed:@"CursorBrowse"];
    _cursor_browse = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(6, 0)];
    
    image = [NSImage imageNamed:@"CursorCrosshair"];
    _cursor_crosshair = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(8, 8)];
    
    image = [NSImage imageNamed:@"CursorPencil"];
    _cursor_pencil = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(0, 17)];
    
    image = [NSImage imageNamed:@"CursorEraser"];
    _cursor_eraser = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(1, 1)];
    
    image = [NSImage imageNamed:@"CursorRound"];
    _cursor_round = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(8, 8)];
    
    image = [NSImage imageNamed:@"CursorBucket"];
    _cursor_bucket = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(1, 16)];
    
    image = [NSImage imageNamed:@"CursorEyedropper"];
    _cursor_eyedropper = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(1, 16)];
}


- (id)init
{
    if (_g_shared_instance) return _g_shared_instance;
    
    self = [super init];
    if (!self) return nil;
    
    _g_shared_instance = self;
    [self _loadCursors];
    
    return self;
}


+ (JHCursorController*)sharedController
{
    if (_g_shared_instance) return _g_shared_instance;
    return [[JHCursorController alloc] init];
}


- (void)setCursor:(int)inCursor
{
    switch (inCursor)
    {
        case CURSOR_IBEAM:
            [[NSCursor IBeamCursor] set];
            break;
        case CURSOR_BROWSE:
            [_cursor_browse set];
            break;
        case CURSOR_ARROW:
            [[NSCursor arrowCursor] set];
            break;
        case CURSOR_CROSSHAIR:
            [_cursor_crosshair set];
            break;
        case CURSOR_PENCIL:
            [_cursor_pencil set];
            break;
        case CURSOR_ERASER:
            [_cursor_eraser set];
            break;
        case CURSOR_ROUND:
            [_cursor_round set];
            break;
        case CURSOR_BUCKET:
            [_cursor_bucket set];
            break;
        case CURSOR_EYEDROPPER:
            [_cursor_eyedropper set];
            break;
    }
}


@end

