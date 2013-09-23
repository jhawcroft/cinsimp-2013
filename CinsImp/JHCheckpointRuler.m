//
//  JHCheckpointRuler.m
//  RulerTrial
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCheckpointRuler.h"

@implementation JHCheckpointRuler


#define _LABEL_MARGIN 4


- (id)initWithScrollView:(NSScrollView *)aScrollView orientation:(NSRulerOrientation)orientation
{
    self = [super initWithScrollView:aScrollView orientation:orientation];
    if (self)
    {
        _checkpoint_image = [NSImage imageNamed:@"IconLineCheckpoint"];
        [self setRuleThickness:_checkpoint_image.size.width + 2 * _LABEL_MARGIN];
        
        _line_indicies = nil;
        _display_line_offsets = nil;
        _checkpoints = [[NSMutableArray alloc] init];
        
        [self setClientView:[aScrollView documentView]];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(textDidChange:)
                                                     name:NSTextStorageDidProcessEditingNotification
                                                   object:[(NSTextView *)[aScrollView documentView] textStorage]];
        
        _last_text = [[[aScrollView documentView] string] copy];
    }
    
    return self;
}


- (void)dealloc
{
    //NSLog(@"dealloc'd the CheckpointRuler");
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}


- (void)_indexLines
{
    NSTextView *text_view = (NSTextView*)[self clientView];
    NSString *the_text = [text_view string];
    
    _line_indicies = [[NSMutableArray alloc] init];
    
    long index = 0;
    long length = [the_text length];
    while (index < length)
    {
        NSRange line_range = [the_text lineRangeForRange:NSMakeRange(index, 0)];
        [_line_indicies addObject:[NSNumber numberWithUnsignedLong:line_range.location]];
        index = line_range.location + line_range.length;
    }
}


- (void)_moveCheckpointsBeyondRange
{
    
}

/* might also need to get the other notification,
 so we can determine how many lines were there before and after, and check mask for what was actually done
 to textstorage, eg. paste vs keystroke  changeInLength?
 
 we really just need before and after, so we can do separate line counts, then check
 the line counts for shrinkage, same or growth
 
 growth, always means the stuff below moves down, but different kinds of growth mean we can't guarantee there isn't
 less lines in the selection afterwards (paste), which means we ought to be careful about it
 */

- (void)_moveCheckpointsBeyondLine:(long)in_line_number by:(long)in_delta
{
    //NSLog(@"move checkpoints beyond line: %ld by %ld", in_line_number, in_delta);
    
    NSMutableArray *new_checkpoints = [[NSMutableArray alloc] initWithCapacity:[_checkpoints count]];
    for (NSNumber *line_number in _checkpoints)
    {
        long this_number = [line_number longValue];
        if (this_number >= in_line_number)
        {
            //if (this_number + in_delta > 0)
                [new_checkpoints addObject:[NSNumber numberWithLong:this_number + in_delta]];
        }
        else
            [new_checkpoints addObject:line_number];
    }
    _checkpoints = new_checkpoints;
}


- (void)_removeCheckpointsBetweenLines:(long)in_from and:(long)in_to
{
    //NSLog(@"delete checkpoints from line: %ld to %ld", in_from, in_to);
    
    NSMutableArray *new_checkpoints = [[NSMutableArray alloc] initWithCapacity:[_checkpoints count]];
    for (NSNumber *line_number in _checkpoints)
    {
        long this_number = [line_number longValue];
        if ((this_number < in_from) || (this_number > in_to))
            [new_checkpoints addObject:line_number];
    }
    _checkpoints = new_checkpoints;
}


- (long)_countLinesInText:(NSString*)in_text
{
    /*
     Apple's method, doesn't do what I need:
     unsigned long numberOfLines, index, stringLength = [in_text length];
    for (index = 0, numberOfLines = 0; index < stringLength; numberOfLines++)
        index = NSMaxRange([in_text lineRangeForRange:NSMakeRange(index, 0)]);
    return numberOfLines;*/
    
    return [[in_text componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]] count];
}


- (BOOL)_textContainsOnlyOneLinebreak:(NSString*)in_text
{
    if (([in_text length] == 1) || ([in_text length] == 2))
    {
        if ([in_text isEqualToString:@"\n"] || [in_text isEqualToString:@"\r\n"] || [in_text isEqualToString:@"\r"]) return YES;
    }
    return NO;
}


- (void)textDidChange:(NSNotification*)notification
{
    NSTextStorage *text_storage = [notification object];
    NSRange edited_range = [text_storage editedRange];
    
    if (edited_range.location != NSNotFound)
    {
        NSTextView *text_view = (NSTextView*)[self clientView];
        NSTextStorage *text_storage = [notification object];
        NSRange original_range = edited_range;
        
        original_range.length -= [text_storage changeInLength];
        NSString *original_text = [_last_text substringWithRange:original_range];
        _last_text = [[text_view string] copy];
        NSString *revised_text = [_last_text substringWithRange:edited_range];
        
        long old_lines = [self _countLinesInText:original_text];
        long new_lines = [self _countLinesInText:revised_text];
        
        long line_start = [self lookupLineIndexForCharacterIndex:original_range.location] + 1;
        long line_end = [self lookupLineIndexForCharacterIndex:fmax(original_range.location,
                                                                          original_range.location + original_range.length - 1)] + 1;
        
        long line_delta = new_lines - old_lines;
        
        if ((line_delta != 0) || (line_start != line_end))
        {
            if ([self _textContainsOnlyOneLinebreak:original_text])
            {
                [self _removeCheckpointsBetweenLines:line_start + 1 and:line_start + 1];
                [self _moveCheckpointsBeyondLine:line_start + 1 by:line_delta];
            }
            else if ([self _textContainsOnlyOneLinebreak:revised_text])
            {
                [self _moveCheckpointsBeyondLine:line_start+1 by:line_delta];
            }
            else
            {
                [self _removeCheckpointsBetweenLines:line_start and:line_end];
                [self _moveCheckpointsBeyondLine:line_start by:line_delta];
            }
        }
        
        // technically we could recompute the line indicies just for the range that changed and above
        // but right now we'll just do everything
        _line_indicies = nil;
        
        [self setNeedsDisplay:YES];
    }
}


- (long)lookupLineIndexForCharacterIndex:(long)in_character_index
{
    if (!_line_indicies) return 0;
    long left_index = 0, right_index = [_line_indicies count];
    while ((right_index - left_index) > 1)
    {
        long middle_index = (right_index + left_index) / 2;
        long line_offset = [[_line_indicies objectAtIndex:middle_index] longValue];
        if (in_character_index < line_offset)
            right_index = middle_index;
        else if (in_character_index > line_offset)
            left_index = middle_index;
        else
            return middle_index;
    }
    return left_index;
    
    /*long index = 0;
    for (NSNumber *offset in _line_indicies)
    {
        if (in_character_index < [offset longValue]) return index - 1;
        index++;
    }
    return index-1;*/
}


- (long)lookupOffsetForLineIndex:(long)in_line_index
{
    if (in_line_index == 0) return 0;
    NSTextView *text_view = (NSTextView*)[self clientView];
    if (in_line_index >= [_line_indicies count]) return [[text_view string] length];
    return [[_line_indicies objectAtIndex:in_line_index] longValue];
}


- (BOOL)lineHasCheckpoint:(long)in_line_number
{
    for (NSNumber *line_number in _checkpoints)
    {
        if ([line_number longValue] == in_line_number) return YES;
    }
    return NO;
}


- (void)setLine:(long)in_line_number hasCheckpoint:(BOOL)in_has_checkpoint
{
    if (!in_has_checkpoint)
    {
        for (NSNumber *line_number in _checkpoints)
        {
            if ([line_number longValue] == in_line_number)
            {
                [_checkpoints removeObject:line_number];
                return;
            }
        }
    }
    else if (![self lineHasCheckpoint:in_line_number])
        [_checkpoints addObject:[NSNumber numberWithLong:in_line_number]];
}


- (void)drawHashMarksAndLabelsInRect:(NSRect)aRect
{
    NSTextView *text_view = (NSTextView*)[self clientView];
    NSTextContainer *text_container = [text_view textContainer];
    NSLayoutManager *layout_manager = [text_view layoutManager];
    NSRect visible_text_area = [[[self scrollView] contentView] bounds];
    
    NSRange visible_glyphs = [layout_manager glyphRangeForBoundingRect:visible_text_area inTextContainer:text_container];
    NSRange character_range = [layout_manager characterRangeForGlyphRange:visible_glyphs actualGlyphRange:NULL];
    
    if (_line_indicies == nil)
        [self _indexLines];
    
    long line_offset = character_range.location;
    long lines_limit = character_range.location + character_range.length;
    long line_index = [self lookupLineIndexForCharacterIndex:line_offset];
    
    NSDictionary *text_atts = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont fontWithName:@"Monaco" size:12.0], NSFontAttributeName,
                               nil];
    
    long inset_height = [text_view textContainerInset].height;
    
    _display_line_offsets = [[NSMutableArray alloc] init];
    
    while (line_offset < lines_limit)
    {
        unsigned long line_rect_count;
        NSRectArray line_rects = [layout_manager rectArrayForCharacterRange:NSMakeRange(line_offset, 0)
                                               withinSelectedCharacterRange:NSMakeRange(NSNotFound, 0)
                                                            inTextContainer:text_container
                                                                  rectCount:&line_rect_count];
        if (line_rect_count > 0)
        {
            NSRect line_rect = line_rects[0];
            long y = inset_height + line_rect.origin.y - visible_text_area.origin.y;
            
            [_display_line_offsets addObject:[NSNumber numberWithLong:line_index]];
            [_display_line_offsets addObject:[NSNumber numberWithLong:y]];
            
            NSString *line_label = [NSString stringWithFormat:@"%ld", (line_index + 1)];
            NSSize line_label_size = [line_label sizeWithAttributes:text_atts];
            
            //NSRect line_label_bounds = NSMakeRect(self.bounds.size.width - line_label_size.width - _LABEL_MARGIN,
            //                                      y + (line_rect.size.height - line_label_size.height) / 2.0,
            //                                      self.bounds.size.width - _LABEL_MARGIN * 2.0, line_rect.size.height);
            //[line_label drawInRect:line_label_bounds withAttributes:text_atts];
            
            _display_last_line_height = line_label_size.height; // could also be using: line_rect
            
            if ([self lineHasCheckpoint:line_index + 1])
            {
                NSRect line_checkpoint_bounds = NSMakeRect(_LABEL_MARGIN,
                                                           y + (line_rect.size.height - _checkpoint_image.size.height) / 2.0,
                                                           _checkpoint_image.size.width,
                                                           _checkpoint_image.size.height);
                [_checkpoint_image drawInRect:line_checkpoint_bounds
                                     fromRect:NSZeroRect
                                    operation:NSCompositeSourceAtop
                                     fraction:1.0
                               respectFlipped:YES
                                        hints:nil];
            }
        }
        
        line_index++;
        line_offset = [self lookupOffsetForLineIndex:line_index];
    }
}


- (void)_clickedLine:(long)in_line_number
{
    NSTextView *text_view = (NSTextView*)[self clientView];
    NSString *the_text = [text_view string];
    NSRange line_range = [the_text lineRangeForRange:NSMakeRange([self lookupOffsetForLineIndex:in_line_number - 1], 0)];
    NSString *line = [[the_text substringWithRange:line_range]
                      stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    long length = [line length];
    
    BOOL not_checkmarkable = NO;
    if ((length >= 2) && ([[line substringWithRange:NSMakeRange(0, 2)] isEqualToString:@"--"]))
        not_checkmarkable = YES;
    else if (length == 0)
        not_checkmarkable = YES;
    //else if ((length >= 2) && ([[[line substringWithRange:NSMakeRange(0, 2)] uppercaseString] isEqualToString:@"ON"]))
    //    not_checkmarkable = YES;
    //else if ((length >= 3) && ([[[line substringWithRange:NSMakeRange(0, 3)] uppercaseString] isEqualToString:@"END"]))
    //    not_checkmarkable = YES;
    //else if ((length >= 8) && ([[[line substringWithRange:NSMakeRange(0, 8)] uppercaseString] isEqualToString:@"FUNCTION"]))
    //    not_checkmarkable = YES;
    
    if (not_checkmarkable)
        [self setLine:in_line_number hasCheckpoint:NO];
    else
        [self setLine:in_line_number hasCheckpoint:![self lineHasCheckpoint:in_line_number]];
    
    [self setNeedsDisplay:YES];
}


- (void)mouseDown:(NSEvent *)theEvent
{
    NSPoint mouse_loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    long i, count = [_display_line_offsets count];
    for (i = 0; i < count; i += 2)
    {
        NSNumber *line_index = [_display_line_offsets objectAtIndex:i];
        NSNumber *line_offset = [_display_line_offsets objectAtIndex:i+1];
        if ((mouse_loc.y > [line_offset longValue]) &&
            (mouse_loc.y <= [line_offset longValue] + _display_last_line_height))
        {
            [self _clickedLine:[line_index longValue] + 1];
            return;
        }
    }
}


- (void)setCheckpoints:(NSArray*)in_checkpoints
{
    _checkpoints = [[NSMutableArray alloc] initWithArray:in_checkpoints];
    
    for (int i = (int)[_checkpoints count]-1; i >= 0; i--)
    {
        long line_number = [[_checkpoints objectAtIndex:i] longValue];
        if (line_number < 1)
        {
            [_checkpoints removeObjectAtIndex:i];
        }
    }
    
    [self setNeedsDisplay:YES];
}


- (NSArray*)checkpoints
{
    return _checkpoints;
}


@end
