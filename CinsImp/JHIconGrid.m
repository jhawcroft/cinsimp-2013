//
//  JHIconGrid.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 7/09/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHIconGrid.h"
#import "JHPropertiesPalette.h"

#include "acu.h"


#define DEFAULT_THUMB_SIZE 128
#define THUMB_PADDING 4
#define CAPTION_PADDING 3


@implementation JHIconGrid


- (void)dealloc
{
    [self removeAllToolTips];
    [_full_captions removeAllObjects];
}


- (id)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self)
    {
        _thumbnail_size = 90;
        _grid_cols = 0;
        _grid_rows = 0;
        
        _selected_index = -1;
        
        NSFont *caption_font = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]];
        NSMutableParagraphStyle *style = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
        [style setLineBreakMode:NSLineBreakByTruncatingMiddle];
        _caption_atts = [[NSDictionary alloc] initWithObjectsAndKeys:caption_font, NSFontAttributeName,
                         style, NSParagraphStyleAttributeName, nil];
        
        NSSize sz = [@"Example" sizeWithAttributes:_caption_atts];
        
        _caption_height = sz.height;
        _cell_size = NSMakeSize(_thumbnail_size + (THUMB_PADDING * 2), _thumbnail_size + (THUMB_PADDING * 2) +
                                _caption_height + CAPTION_PADDING);
        
        _full_captions = [[NSMutableArray alloc] init];
    }
    return self;
}



- (BOOL)isFlipped
{
    return YES;
}


- (void)setStack:(StackHandle)in_stack
{
    _stack = in_stack;
}


- (StackHandle)stack
{
    return _stack;
}


- (void)viewDidMoveToWindow
{
    if (self.window)
    {
        [self reload];
        //[self setFrameSize:NSMakeSize(256, 768)];
    }
}


- (NSSize)scaleSize:(NSSize)in_size proportionallyToSize:(NSSize)in_max_size
{
    float lge_dim_sz = fmax(in_size.width, in_size.height);
    float lge_dim_max = fmax(in_max_size.width, in_max_size.height);
    if (lge_dim_sz > lge_dim_max)
    {
        float scale_factor = lge_dim_max / lge_dim_sz;
        in_size.width *= scale_factor;
        in_size.height *= scale_factor;
    }
    return in_size;
}


- (void)drawRect:(NSRect)dirtyRect
{
    if (!_stack) return;
    
    if (_grid_cols == 0) [self _recomputeGrid];
    
   
    int icon_count = _count;
    
    
    [self removeAllToolTips];
    [_full_captions removeAllObjects];

    
    NSScrollView *scroller = (NSScrollView*)[[self superview] superview];
    NSRect scroll_rect = [scroller documentVisibleRect];
    
    int starting_row = scroll_rect.origin.y / _cell_size.height;
    //NSLog(@"starting row: %d", starting_row);
    
    int rows_high = ceil(scroll_rect.size.height / _cell_size.height);
    if (rows_high < 1) rows_high = 1;
    //NSLog(@"display height: %d", rows_high);
    
    int start_from_number = (starting_row * _grid_cols) + 1;
    //NSLog(@"start from: %d", start_from_number);
    int end_to_number = (rows_high + 1) * _grid_cols + start_from_number - 1;
    if (end_to_number > icon_count) end_to_number = icon_count;
    //NSLog(@"end number: %d", end_to_number);
    
    acu_iconmgr_preview_range(_stack, start_from_number, end_to_number);
    
    //NSLog(@"Grid params COLS: %d ROWS: %d START: %d END: %d", _grid_cols, _grid_rows,
    //      start_from_number, end_to_number);
    
    int number = start_from_number;
    for (int r = starting_row; r < _grid_rows; r++)
    {
        for (int c = 0; c < _grid_cols; c++)
        {
            char *name;
            void *data_ptr;
            long size;
            int the_id;
            
            acu_iconmgr_icon_n(_stack, number-1, &the_id, &name, &data_ptr, &size);
            
            NSData *data = [[NSData alloc] initWithBytesNoCopy:data_ptr length:size freeWhenDone:NO];
            NSImage *image = [[NSImage alloc] initWithData:data];
            
            
            NSRect cell_rect = NSMakeRect(c * _cell_size.width, r * _cell_size.height, _cell_size.width, _cell_size.height);
            
            if (_selected_index + 1 == number)
            {
                [[NSColor selectedTextBackgroundColor] setFill];
            }
            else
            {
                [[NSColor whiteColor] setFill];
            }
            NSRectFill(cell_rect);
            
            //[[NSColor grayColor] setFill];
            //NSFrameRect(cell_rect);
            
        
            NSSize image_size_scaled = [self scaleSize:image.size proportionallyToSize:NSMakeSize(_thumbnail_size, _thumbnail_size)];
            
    
            NSRect image_rect = NSMakeRect(cell_rect.origin.x + THUMB_PADDING + ((_thumbnail_size - image_size_scaled.width) / 2),
                                           cell_rect.origin.y + THUMB_PADDING + ((_thumbnail_size - image_size_scaled.height) / 2),
                                               image_size_scaled.width, image_size_scaled.height);
            
            [[NSGraphicsContext currentContext] setShouldAntialias:NO];
            [image drawInRect:image_rect fromRect:NSZeroRect
                    operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
            [[NSGraphicsContext currentContext] setShouldAntialias:YES];
            
            
            NSString *caption;
            
            if (name && strlen(name)) caption = [NSString stringWithFormat:@"%s", name];
            else caption = [NSString stringWithFormat:@"%d", the_id];
            
            NSSize caption_size = [caption sizeWithAttributes:_caption_atts];
            BOOL caption_is_abbreviated = NO;
            if (caption_size.width > _thumbnail_size)
            {
                caption_size.width = _thumbnail_size;
                caption_is_abbreviated = YES;
            }
            
            NSRect caption_rect = NSMakeRect(cell_rect.origin.x + THUMB_PADDING + ((_thumbnail_size - caption_size.width) / 2),
                                             cell_rect.origin.y + THUMB_PADDING + _thumbnail_size + CAPTION_PADDING,
                                             caption_size.width, caption_size.height);
            
            [caption drawInRect:caption_rect withAttributes:_caption_atts];
            
            
            if (caption_is_abbreviated)
            {
                [_full_captions addObject:caption];
                [self addToolTipRect:caption_rect owner:self userData:(__bridge void *)([_full_captions lastObject])];
            }
            
     
            
            number++;
            if (number > end_to_number) break;
        }
        if (number > end_to_number) break;
    }
    
}


- (NSString*)view:(NSView *)view stringForToolTip:(NSToolTipTag)tag point:(NSPoint)point userData:(void *)data
{
    return (__bridge NSString*)data;
}


- (void)_recomputeGrid
{
    if (!_stack) return;
    
    int icon_count = acu_iconmgr_count(_stack);
    _count = icon_count;
    
    NSScrollView *scrollView = (NSScrollView*)[[self superview] superview];
    _grid_cols = (scrollView.bounds.size.width - [scrollView verticalScroller].bounds.size.width - 2) / _cell_size.width;
    if (_grid_cols < 1) _grid_cols = 1;
    
    _grid_rows = ceil((double)icon_count / (double)_grid_cols);
    
    NSSize sz;
    sz.width = _grid_cols * _cell_size.width;
    sz.height = _grid_rows * _cell_size.height;
    
    [self setFrameSize:sz];
    
    //NSLog(@"Sze: %@", NSStringFromSize(sz));
    
}


- (void)postSelectionNotification
{
    [[NSNotificationCenter defaultCenter] postNotificationName:@"DesignSelectionChange" object:self];
}


- (void)reload
{
    [self _recomputeGrid];
    [self setNeedsDisplay:YES];
    
    [self postSelectionNotification];
}


- (void)viewDidEndLiveResize
{
    [self _recomputeGrid];
}


- (void)mouseDown:(NSEvent *)theEvent
{
    NSPoint loc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    
    int col = fmin(loc.x / _cell_size.width, _grid_cols-1);
    int row = fmin(loc.y / _cell_size.height, _grid_rows-1);
    
    int index = row * _grid_cols + col;
    
    if ((index >= 0) && (index < _count))
    {
        _selected_index = index;
        [self setNeedsDisplay:YES];
        
        [self postSelectionNotification];
    }
}


- (void)mouseUp:(NSEvent *)theEvent
{
    if ([theEvent clickCount] == 2)
        [[[JHPropertiesPalette sharedInstance] window] makeKeyAndOrderFront:self];
}


- (BOOL)canBecomeKeyView
{
    return YES;
}


- (BOOL)acceptsFirstResponder
{
    return YES;
}


- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}


- (BOOL)hasSelection
{
    return ((_selected_index >= 0) && (_selected_index < _count));
}


- (IBAction)delete:(id)sender
{
    int the_id;
    if (![self hasSelection]) return;
    acu_iconmgr_icon_n(_stack, _selected_index, &the_id, NULL, NULL, NULL);
    acu_iconmgr_delete(_stack, the_id);
    _selected_index = -1;
    [self reload];
}


- (void)keyDown:(NSEvent *)theEvent
{
    unichar pressed_char = [[theEvent characters] characterAtIndex:0];
    //NSLog(@"%ld", (unsigned long)pressed_char);
    if ((pressed_char == 127) || (pressed_char == 0xF728))
    {
        [self delete:self];
    }
}


- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    if (menuItem.action == @selector(delete:))
    {
        return [self hasSelection];
    }
    return NO;
}


- (int)selectedIconIndex
{
    return _selected_index;
}


- (void)selectNone
{
    _selected_index = -1;
    [self setNeedsDisplay:YES];
    [self postSelectionNotification];
}


@end
