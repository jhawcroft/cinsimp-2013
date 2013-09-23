//
//  JHxTalkTextView.h
//  RulerTrial
//
//  Created by Joshua Hawcroft on 31/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface JHxTalkTextView : NSTextView
{
    NSFont *_font_regular;
    NSFont *_font_comment;
    NSFont *_font_keyword;
    NSMutableCharacterSet *_part_delimiters;
    NSDictionary *_keyword_atts;
    NSDictionary *_comment_atts;
    NSTimer *_reindent_timer;
    BOOL _user_reindent;
    NSRange _execute_range;
    NSString *_returned_script;
    int *_returned_checkpoints;
}

- (void)showExecutingLine:(long)in_line_number;
- (void)showSourceLine:(long)in_line_number;

- (void)setScript:(char const*)in_script checkpoints:(int const*)in_checkpoints count:(int)in_checkpoint_count selection:(long)in_selection;
- (void)script:(char const**)out_script checkpoints:(int const**)out_checkpoints count:(int*)out_checkpoint_count selection:(long*)out_selection;

@end
