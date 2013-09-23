//
//  JHCursorController.h
//  TestPaintTools
//
//  Created by Joshua Hawcroft on 25/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>


#define CURSOR_BROWSE       0
#define CURSOR_ARROW        1
#define CURSOR_CROSSHAIR    2
#define CURSOR_PENCIL       3
#define CURSOR_ERASER       4
#define CURSOR_ROUND        5
#define CURSOR_BUCKET       6
#define CURSOR_EYEDROPPER   7
#define CURSOR_IBEAM        8


@interface JHCursorController : NSObject
{
    NSCursor *_cursor_browse;
    NSCursor *_cursor_crosshair;
    NSCursor *_cursor_pencil;
    NSCursor *_cursor_eraser;
    NSCursor *_cursor_round;
    NSCursor *_cursor_bucket;
    NSCursor *_cursor_eyedropper;
}

+ (JHCursorController*)sharedController;

- (void)setCursor:(int)inCursor;

@end


