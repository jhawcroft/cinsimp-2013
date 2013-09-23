//
//  JHCheckpointRuler.h
//  RulerTrial
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface JHCheckpointRuler : NSRulerView
{
    NSMutableArray *_line_indicies;
    NSMutableArray *_display_line_offsets;
    long _display_last_line_height;
    NSMutableArray *_checkpoints;
    NSImage *_checkpoint_image;
    NSString *_last_text;
}

- (void)setCheckpoints:(NSArray*)in_checkpoints;
- (NSArray*)checkpoints;

@end
