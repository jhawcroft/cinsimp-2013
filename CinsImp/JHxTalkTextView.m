//
//  JHxTalkTextView.m
//  RulerTrial
//
//  Created by Joshua Hawcroft on 31/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHxTalkTextView.h"
#import "JHCheckpointRuler.h"

#include "jhglobals.h"

#include "acu.h"


#define _INDENT_SPACES 2

#define _MAX_NESTED_BLOCKS 50


@implementation JHxTalkTextView



- (void)awakeFromNib
{
    [[self textContainer] setContainerSize:NSMakeSize(FLT_MAX, FLT_MAX)];
    [[self textContainer] setWidthTracksTextView:NO];
    [self setHorizontallyResizable:YES];
    [self setContinuousSpellCheckingEnabled:NO];
    
    _returned_checkpoints = NULL;
    
    _font_regular = [[NSFontManager sharedFontManager] fontWithFamily:@"Courier" traits:0 weight:5 size:14.0];;
    _font_comment = [[NSFontManager sharedFontManager] fontWithFamily:@"Arial" traits:NSItalicFontMask weight:5 size:13.0];
    _font_keyword = [[NSFontManager sharedFontManager] fontWithFamily:@"Courier" traits:NSBoldFontMask weight:9 size:14.0];
    [self setFont:_font_regular];
    
    _keyword_atts = [NSDictionary dictionaryWithObjectsAndKeys:_font_keyword, NSFontAttributeName,
                     [NSColor redColor], NSForegroundColorAttributeName, nil];
    
    _comment_atts = [NSDictionary dictionaryWithObjectsAndKeys:_font_comment, NSFontAttributeName,
                     [NSColor colorWithDeviceRed:0.4 green:0.4 blue:0.4 alpha:1.0], NSForegroundColorAttributeName,
                     nil];
    
    _reindent_timer = nil;
    
    _execute_range = NSMakeRange(NSNotFound, 0);
    
    [[self enclosingScrollView] setVerticalRulerView:[[JHCheckpointRuler alloc] initWithScrollView:[self enclosingScrollView]
                                                                                       orientation:NSVerticalRuler]];
    [[self enclosingScrollView] setHasVerticalRuler:YES];
    [[self enclosingScrollView] setRulersVisible:YES];
    [[self enclosingScrollView] setHasHorizontalScroller:YES];
    
    _part_delimiters = [[NSMutableCharacterSet alloc] init];
    [_part_delimiters formUnionWithCharacterSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    [_part_delimiters formUnionWithCharacterSet:[NSCharacterSet punctuationCharacterSet]];
    [_part_delimiters formUnionWithCharacterSet:[NSCharacterSet symbolCharacterSet]];
    [_part_delimiters formUnionWithCharacterSet:[NSCharacterSet controlCharacterSet]];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(textDidChange:)
                                                 name:NSTextStorageDidProcessEditingNotification
                                               object:[self textStorage]];
    
    //[self setString:@"on mouseup\n  beep 3 times\n  visual effect dissolve\n  go to next card\nend mouseup\n\n"
    // "-- this is a short example script\n-- for the purposes of testing\n-- the checkpoint field that Josh\n-- is working on.\n"];
    //
    
    
}


- (void)_recolourRange:(NSRange)in_character_range
{
    NSTextStorage *text_storage = [self textStorage];
    
    [text_storage setAttributes:[NSDictionary dictionaryWithObjectsAndKeys:_font_regular, NSFontAttributeName, nil]
                          range:in_character_range];
    
    NSString *substring = [[self string] substringWithRange:in_character_range];
    //NSLog(@"----");
    
    BOOL in_comment = NO;
    BOOL has_newline = NO;
    long comment_offset;
    long offset = 0, last_offset = 0;
    long limit = [substring length];
    int line_part_index = 0;
    while (offset <= limit)
    {
        if (has_newline)
        {
            if (in_comment)
            {
            
                NSRange actual_range = NSMakeRange(comment_offset + in_character_range.location, offset - comment_offset);
                
                [text_storage setAttributes:_comment_atts range:actual_range];
            }
            
            has_newline = NO;
            in_comment = NO;
            line_part_index = 0;
        }
        
        NSRange found_delimiter = [substring rangeOfCharacterFromSet:_part_delimiters
                                                             options:0
                                                               range:NSMakeRange(offset, limit - offset)];
        if (found_delimiter.location == NSNotFound)
        {
            found_delimiter.location = limit;
            found_delimiter.length = 1;
            has_newline = NO;
            
            if (in_comment)
            {
                
                NSRange actual_range = NSMakeRange(comment_offset + in_character_range.location, limit - comment_offset);
                
                [text_storage setAttributes:_comment_atts range:actual_range];
            }
        }
        else
        {
            has_newline = ([substring rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]
                                                      options:0
                                                        range:found_delimiter].location != NSNotFound);
        }
        
        if (!in_comment)
        {
            NSRange part_range = NSMakeRange(last_offset, found_delimiter.location - last_offset);
            if (part_range.length > 0)
            {
                NSRange actual_range = NSMakeRange(in_character_range.location + part_range.location, part_range.length);
                NSString *part = [[substring substringWithRange:part_range] uppercaseString];
                
                //NSLog(@"part=%@",part);
                BOOL is_keyword = NO;
                
                if ([part isEqualToString:@"ON"])
                    is_keyword = YES;
                else if ([part isEqualToString:@"END"])
                    is_keyword = YES;
                else if ([part isEqualToString:@"FUNCTION"])
                    is_keyword = YES;
                else if ([part isEqualToString:@"IF"])
                    is_keyword = YES;
                else if ([part isEqualToString:@"ELSE"])
                    is_keyword = YES;
                else if ([part isEqualToString:@"THEN"])
                    is_keyword = YES;
                else if ([part isEqualToString:@"REPEAT"])
                    is_keyword = YES;
                
                if (line_part_index == 0)
                {
                    if ([part isEqualToString:@"EXIT"])
                        is_keyword = YES;
                    else if ([part isEqualToString:@"NEXT"])
                        is_keyword = YES;
                    else if ([part isEqualToString:@"PASS"])
                        is_keyword = YES;
                    else if ([part isEqualToString:@"GLOBAL"])
                        is_keyword = YES;
                    else if ([part isEqualToString:@"RETURN"])
                        is_keyword = YES;
                }
                
                //is_keyword = NO; /// to see what it looks like without keyword syntax colouring
                
                if (is_keyword)
                {
                    [text_storage setAttributes:_keyword_atts range:actual_range];
                }

                line_part_index++;
            }
            
            
        }
        
        
        
        if ((found_delimiter.length == 1) && (found_delimiter.location + 2 <= limit)
            && ([[substring substringWithRange:NSMakeRange(found_delimiter.location, 2)] isEqualToString:@"--"]))
        {
            if (!in_comment)
            {
                in_comment = YES;
                comment_offset = found_delimiter.location;
            }
            
        }
        
        last_offset = found_delimiter.location + found_delimiter.length;
        offset = last_offset;
    }
    [self setNeedsDisplay:YES];
}


- (void)drawRect:(NSRect)dirtyRect
{
    if ((_reindent_timer) && (_user_reindent))
    {
        [[NSColor whiteColor] setFill];
        NSRectFill(self.bounds);
        return;
    }
    [super drawRect:dirtyRect];
}


- (void)keyDown:(NSEvent *)theEvent
{
    NSString *chars = [theEvent characters];
    if ([chars rangeOfCharacterFromSet:
         [NSCharacterSet characterSetWithCharactersInString:@"\t"]].location != NSNotFound)
    {
        [self _requestReindent:YES];
        [self setNeedsDisplay:YES];
        return;
    }
    else if ( ([[theEvent characters] rangeOfCharacterFromSet:
              [NSCharacterSet characterSetWithCharactersInString:@"\r"]].location != NSNotFound) )
    {
        if ([theEvent modifierFlags] & NSCommandKeyMask)
        {
            [self replaceCharactersInRange:[self selectedRange] withString:@"¬\n"];
        }
        else
        {
            [self replaceCharactersInRange:[self selectedRange] withString:@"\n"];
        }
        
        return;
    }
    [super keyDown:theEvent];
}


- (long)_whitespacePrefixLength:(NSString*)in_text
{
    NSRange first_alpha = [in_text rangeOfCharacterFromSet:[[NSCharacterSet whitespaceCharacterSet] invertedSet]];
    if (first_alpha.location == NSNotFound) return 0;
    return first_alpha.location;
}


typedef struct {
    long line_index;
    long line_offset;
    long length;
    
} SavedSelection;

- (SavedSelection)_getSaveSelection
{
    NSRange selected_range = [self selectedRange];
    SavedSelection saved_selection = {0, 0, selected_range.length};
    
    if (selected_range.location == NSNotFound) return saved_selection;
    
    NSString *text = [self string];
    NSRange pos = NSMakeRange(0, selected_range.location);
    long line_count = 1;
    long first_line_offset = -1;
    while (pos.location != NSNotFound)
    {
        pos = [text rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet] options:NSBackwardsSearch range:pos];
        if (pos.location == NSNotFound) break;
        if (first_line_offset < 0) first_line_offset = pos.location;
        pos = NSMakeRange(0, MAX(0, (signed long)pos.location));
        line_count++;
    }
    if (first_line_offset < 0) first_line_offset = -1;
    
    saved_selection.line_index = MAX(0, (signed long)line_count-1);
    saved_selection.line_offset = MAX(0, (signed long)selected_range.location - first_line_offset - 1);
    
    NSRange line_range = [text lineRangeForRange:NSMakeRange(selected_range.location, 0)];
    NSString *line = [text substringWithRange:line_range];

    long prefix_whitespace = [self _whitespacePrefixLength:line];
    saved_selection.line_offset -= prefix_whitespace;
    //NSLog(@"SELECTION whitespace prefix: %ld\n", prefix_whitespace);
        
    return saved_selection;
}


NSString* _whitespace(int in_length)
{
    if (in_length > 90) return @"";
    return [@"                                                                                          " substringToIndex:in_length];
}



NSString* _first_word(NSString *in_line)
{
    NSString *first_word;
    NSRange first_space = [in_line rangeOfCharacterFromSet:[NSCharacterSet whitespaceCharacterSet]];
    if (first_space.location != NSNotFound)
        first_word = [[in_line substringToIndex:first_space.location] uppercaseString];
    else
        first_word = [in_line uppercaseString];
    return first_word;
}



static void _do_indent(XTE *in_engine, JHxTalkTextView *in_self, long in_char_begin, long in_char_length,
                       char const *in_spaces, int in_space_count)
{
    // these are byte offsets ***TODO*** !! fix
     [in_self replaceCharactersInRange:NSMakeRange(in_char_begin, in_char_length)
                            withString:[NSString stringWithCString:in_spaces encoding:NSUTF8StringEncoding]];
}


- (void)_reindent:(id)sender
{
    /* if we were called via a timer (delayed invokation);
     reset and remove the timer */
    if (_reindent_timer != nil)
    {
        [_reindent_timer invalidate];
        _reindent_timer = nil;
    }
    
    /* save the selection */
    NSRange selection = [self selectedRange];
    
    /* reindent the source code */
    acu_source_indent([[self string] UTF8String], (long*)&(selection.location), (long*)&(selection.length),
                      (XTEIndentingHandler)&_do_indent, (__bridge void *)(self));
    
    /* restore the selection */
    if (selection.length == 0)
    {
        /* place at end of line if possible */
        NSRange line_range = [[self string] lineRangeForRange:selection];
        if (line_range.length != 0)
            selection.location = line_range.location + line_range.length - 1;
    }
    [self setSelectedRange:selection];
    
    /* refresh the display */
    [self setNeedsDisplay:YES];
}


- (void)_requestReindent:(BOOL)in_user
{
    if (!_reindent_timer)
        _reindent_timer = [NSTimer scheduledTimerWithTimeInterval:(in_user ? 0.05 : 0.0)
                                                           target:self
                                                         selector:@selector(_reindent:)
                                                         userInfo:nil
                                                          repeats:YES];
    _user_reindent = in_user;
}


- (void)textDidChange:(NSNotification*)notification
{
    NSTextStorage *text_storage = [notification object];
    NSRange edited_range = [text_storage editedRange];
    
    [self hideExecutingLine];
    
    if (edited_range.location != NSNotFound)
    {
        NSString *the_text = [self string];
        long length = [the_text length];
        long recolour_start, recolour_end;
        
        
        
        NSRange found_range = [the_text rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]
                                                        options:NSBackwardsSearch
                                                          range:NSMakeRange(0, edited_range.location)];
        if (found_range.location == NSNotFound) recolour_start = 0;
        else recolour_start = found_range.location;
        
        NSRange scan_range = NSMakeRange(MIN(edited_range.location + edited_range.length + 2, length), 0);
        scan_range.length = fmax(0, length - scan_range.location);
        
        found_range = [the_text rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]
                                                options:0
                                                  range:scan_range];
        if (found_range.location == NSNotFound) recolour_end = length;
        else recolour_end = found_range.location;
        
        if (recolour_end > recolour_start)
        {
            //NSLog(@"recolour range: %ld, %ld", recolour_start, recolour_end);
            [self _recolourRange:NSMakeRange(recolour_start, recolour_end-recolour_start)];
        }
        
        NSString *changed = [the_text substringWithRange:edited_range];
        if (([changed length] == 1) && ([changed rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]].location != NSNotFound))
            [self _requestReindent:NO];
    }
    
    [[self window] setDocumentEdited:YES];
}


- (void)dealloc
{
    //NSLog(@"dealloc xTalk text view");
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if (_returned_checkpoints) free(_returned_checkpoints);
    _returned_checkpoints = NULL;
    
}


- (id)initWithFrame:(NSRect)frameRect textContainer:(NSTextContainer *)container
{
    self = [super initWithFrame:frameRect textContainer:container];
    //if (self) [self _initView];
    return self;
}


- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    //if (self) [self _initView];
    return self;
}


- (NSRange)_lookupRangeForLineIndex:(long)in_line_index
{
    NSString *the_text = [self string];
    long index = 0, line_index = 0;
    long length = [the_text length];
    while (index < length)
    {
        NSRange line_range = [the_text lineRangeForRange:NSMakeRange(index, 0)];
        if (line_index == in_line_index) return line_range;
        line_index++;
        index = line_range.location + line_range.length;
    }
    return NSMakeRange(length, 0);
}


- (void)showSourceLine:(long)in_line_number
{
    NSRange line_range = [self _lookupRangeForLineIndex:in_line_number - 1];
    [self setSelectedRange:NSMakeRange(line_range.location, 0)];
    //[self setNeedsDisplay:YES];
}


- (void)showExecutingLine:(long)in_line_number
{
    NSRange line_range = [self _lookupRangeForLineIndex:in_line_number - 1];
    //NSTextStorage *storage = [self textStorage];
    _execute_range = line_range;
    [self setNeedsDisplay:YES];
    //[storage addAttributes:_execute_atts range:line_range];
}


- (void)hideExecutingLine
{
    if (_execute_range.location == NSNotFound) return;
    _execute_range = NSMakeRange(NSNotFound, 0);
    [self setNeedsDisplay:YES];
}


- (void) drawViewBackgroundInRect:(NSRect)rect
{
    [super drawViewBackgroundInRect:rect];
    NSString *str = [self string];
    if ((_execute_range.location != NSNotFound) && (_execute_range.location <= [str length])) {
        NSRect lineRect = [self highlightRectForRange:_execute_range];
        
        [[NSColor colorWithDeviceRed:1.0 green:0.8 blue:0.8 alpha:1.0] setFill];
        NSRectFill(lineRect);
        
        [[NSColor redColor] setFill];
        NSFrameRect(lineRect);
    }
}


// Returns a rectangle suitable for highlighting a background rectangle for the given text range.
- (NSRect) highlightRectForRange:(NSRange)aRange
{
    NSRange gr = [[self layoutManager] glyphRangeForCharacterRange:aRange actualCharacterRange:NULL];
    NSRect br = [[self layoutManager] boundingRectForGlyphRange:gr inTextContainer:[self textContainer]];
    NSRect b = [self bounds];
    CGFloat h = br.size.height;
    CGFloat w = b.size.width;
    CGFloat y = br.origin.y;
    NSPoint containerOrigin = [self textContainerOrigin];
    NSRect aRect = NSMakeRect(0, y, w, h);
    // Convert from view coordinates to container coordinates
    aRect = NSOffsetRect(aRect, containerOrigin.x, containerOrigin.y);
    return aRect;
}


- (void)setScript:(char const*)in_script checkpoints:(int const*)in_checkpoints count:(int)in_checkpoint_count selection:(long)in_selection
{
    [self setString:[NSString stringWithCString:in_script encoding:NSUTF8StringEncoding]];
    NSMutableArray *checkpointsList = [[NSMutableArray alloc] init];
    for (int i = 0; i < in_checkpoint_count; i++)
        [checkpointsList addObject:[NSNumber numberWithLong:in_checkpoints[i]]];
    [(JHCheckpointRuler*)[[self enclosingScrollView] verticalRulerView] setCheckpoints:checkpointsList];
    [self setSelectedRange:NSMakeRange(in_selection, 0)];
}


- (void)script:(char const**)out_script checkpoints:(int const**)out_checkpoints count:(int*)out_checkpoint_count selection:(long*)out_selection
{
    _returned_script = [self string];
    *out_script = [_returned_script UTF8String];
    *out_checkpoints = NULL;
    *out_checkpoint_count = 0;
    *out_selection = 0;
    NSArray *checkpointList = [(JHCheckpointRuler*)[[self enclosingScrollView] verticalRulerView] checkpoints];
    if (_returned_checkpoints) free(_returned_checkpoints);
    _returned_checkpoints = malloc(sizeof(int) * [checkpointList count]);
    if (!_returned_checkpoints) return;
    for (int i = 0; i < [checkpointList count]; i++)
        _returned_checkpoints[i] = (int)[[checkpointList objectAtIndex:i] longValue];
    *out_checkpoints = _returned_checkpoints;
    *out_checkpoint_count = (int)[checkpointList count];
    *out_selection = [self selectedRange].location;
}



@end
