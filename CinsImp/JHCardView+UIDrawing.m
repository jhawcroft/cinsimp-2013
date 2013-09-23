//
//  JHCardView+UIDrawing.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView.h"

#include "tools.h"

@implementation JHCardView (UIDrawing)



/************
 Drawing
 */

- (void)drawTabOrder
{
    int order = 1;
    
    NSFont *font = [NSFont fontWithName:@"Lucida Grande" size:9.0];
    NSColor *textColor = [NSColor whiteColor];
    NSRect rect;
    NSDictionary *textAtts = [NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName,
                              textColor, NSForegroundColorAttributeName, nil];
    
    for (NSView *view in self.subviews)
    {
        if ([view conformsToProtocol:@protocol(JHWidget)])
        {
            NSString *number = [NSString stringWithFormat:@"%d", order];
            NSSize bounds = [number sizeWithAttributes:textAtts];
            
            rect = NSMakeRect(view.frame.origin.x - bounds.width - 2, view.frame.origin.y, bounds.width + 3, bounds.height);
            [[NSColor blackColor] setFill];
            NSRectFill(rect);
            [number drawAtPoint:NSMakePoint(rect.origin.x + 1, rect.origin.y) withAttributes:textAtts];
            
            order++;
        }
    }
}


- (void)drawOutlines
{
    [[NSColor grayColor] setFill];
    NSColor *gray = [NSColor grayColor];
    NSColor *white = [NSColor whiteColor];
    if ((currentTool == AUTH_TOOL_FIELD) || (peek_flds))
    {
        [gray setFill];
        for (NSView *view in self.subviews)
        {
            if ([view conformsToProtocol:@protocol(JHWidget)])
            {
                /*for (NSView<JHWidget> *widget in self.subviews)
                 {
                 if ((![widget isKindOfClass:[JHSelectorView class]]) && (![widget isButton]))
                 {*/
                NSView<JHWidget> *widget = (NSView<JHWidget>*)view;
                if (![widget isButton])
                {
                    
                    NSFrameRect(NSMakeRect(widget.frame.origin.x-1, widget.frame.origin.y-1,
                                           widget.frame.size.width+2, widget.frame.size.height+2));
                }
            }
        }
        [white setFill];
        for (NSView *view in self.subviews)
        {
            if ([view conformsToProtocol:@protocol(JHWidget)])
            {
                /*for (NSView<JHWidget> *widget in self.subviews)
                 {
                 if ((![widget isKindOfClass:[JHSelectorView class]]) && (![widget isButton]))
                 {*/
                NSView<JHWidget> *widget = (NSView<JHWidget>*)view;
                if (![widget isButton])
                {
                    
                    NSFrameRect(NSMakeRect(widget.frame.origin.x-2, widget.frame.origin.y-2,
                                           widget.frame.size.width+4, widget.frame.size.height+4));
                }
            }
        }
    }
    if ((currentTool == AUTH_TOOL_BUTTON) || (peek_btns))
    {
        [gray setFill];
        for (NSView *view in self.subviews)
        {
            if ([view conformsToProtocol:@protocol(JHWidget)])
            {
                /*
                 for (NSView<JHWidget> *widget in self.subviews)
                 {
                 if ((![widget isKindOfClass:[JHSelectorView class]]) && ([widget isButton]))
                 {*/
                NSView<JHWidget> *widget = (NSView<JHWidget>*)view;
                if ([widget isButton])
                {
                    NSFrameRect(NSMakeRect(widget.frame.origin.x-1, widget.frame.origin.y-1,
                                           widget.frame.size.width+2, widget.frame.size.height+2));
                }
            }
        }
        [white setFill];
        for (NSView *view in self.subviews)
        {
            if ([view conformsToProtocol:@protocol(JHWidget)])
            {
                /*
                 for (NSView<JHWidget> *widget in self.subviews)
                 {
                 if ((![widget isKindOfClass:[JHSelectorView class]]) && ([widget isButton]))
                 {*/
                NSView<JHWidget> *widget = (NSView<JHWidget>*)view;
                if ([widget isButton])
                {
                    NSFrameRect(NSMakeRect(widget.frame.origin.x-2, widget.frame.origin.y-2,
                                           widget.frame.size.width+4, widget.frame.size.height+4));
                }
            }
        }
    }
}


- (void)drawRect:(NSRect)dirtyRect
{
    //NSLog(@"card view drawRect:");
    
    
    if (protected_hidden)
        return;
    
   
   /*if ((_saved_screen != nil) && (!_render_actual_card))
    {
        [_saved_screen drawAtPoint:NSMakePoint(0, 0) fromRect:NSZeroRect operation:NSCompositeCopy fraction:1.0];
        //[[NSColor whiteColor] set];
        //NSRectFill(self.bounds);
        
        return;
    }*/
    
    
    if (!_disable_drawing_backdrop)
    {
        [[NSColor whiteColor] set];
        NSRectFill(self.bounds);
    }
    
    
    if (_paint_subsys != NULL)
    {
        //NSLog(@"redraw card");
        
        if (_paint_predraw_cache)
            [_paint_predraw_cache drawInRect:self.bounds
                                    fromRect:NSZeroRect
                                   operation:NSCompositeSourceAtop
                                    fraction:1.0
                              respectFlipped:YES
                                       hints:nil];
        
        paint_draw_into(_paint_subsys, [[NSGraphicsContext currentContext] graphicsPort]);
        
        if (_paint_postdraw_cache)
            [_paint_postdraw_cache drawInRect:self.bounds
                                     fromRect:NSZeroRect
                                    operation:NSCompositeSourceAtop
                                     fraction:1.0
                               respectFlipped:YES
                                        hints:nil];
        
        return;
    }
    
    
    
    
    
    // this stuff is going to have to be drawn into another view, added a top all others
    // within build card, otherwise it's not going to appear once we have graphics at play as well...
    
    
    // either that, or draw using window like below, but after drawing evertything else
    
    // can use graphicsContextWithWindow
    /*NSGraphicsContext *view_context = [NSGraphicsContext currentContext];
     [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithWindow:[self window]]];
     [[NSGraphicsContext currentContext] saveGraphicsState];
     
     NSAffineTransform* xform = [NSAffineTransform transform];
     [xform translateXBy:0.0 yBy:self.bounds.size.height];
     [xform scaleXBy:1.0 yBy:-1.0];
     [xform concat];*/
    
    
    
    
    /*[[NSGraphicsContext currentContext] restoreGraphicsState];
     [NSGraphicsContext setCurrentContext:view_context];*/
}


- (void)drawOverlay
{
    [self drawOutlines];
    
    if (snap_line_x >= 0)
    {
        [[NSColor blueColor] setFill];
        NSFrameRect(NSMakeRect(snap_line_x, 0, 1, self.frame.size.height));
    }
    if (snap_line_y >= 0)
    {
        [[NSColor blueColor] setFill];
        NSFrameRect(NSMakeRect(0, snap_line_y, self.frame.size.width, 1));
    }
    
    if ((show_sequence) && (_edit_mode == CARDVIEW_MODE_LAYOUT))
        [self drawTabOrder];
    
    
    if (edit_bkgnd)
    {
        
        [[NSColor blackColor] setFill];
        NSSize sz = self.bounds.size;
        for (int x = 5; x < sz.width; x += 8)
        {
            NSFrameRect(NSMakeRect(x, 0, 2, 1));
            NSFrameRect(NSMakeRect(x+1, 1, 2, 1));
            NSFrameRect(NSMakeRect(x+2, 2, 2, 1));
            NSFrameRect(NSMakeRect(x+3, 3, 2, 1));
        }
        for (int x = 5; x < sz.width; x += 8)
        {
            NSFrameRect(NSMakeRect(x, sz.height-4, 2, 1));
            NSFrameRect(NSMakeRect(x+1, sz.height-3, 2, 1));
            NSFrameRect(NSMakeRect(x+2, sz.height-2, 2, 1));
            NSFrameRect(NSMakeRect(x+3, sz.height-1, 2, 1));
        }
        for (int y = 5; y < sz.height; y += 8)
        {
            NSFrameRect(NSMakeRect(0, y, 1, 2));
            NSFrameRect(NSMakeRect(1, y+1, 1, 2));
            NSFrameRect(NSMakeRect(2, y+2, 1, 2));
            NSFrameRect(NSMakeRect(3, y+3, 1, 2));
        }
        for (int y = 5; y < sz.height; y += 8)
        {
            NSFrameRect(NSMakeRect(sz.width-4, y, 1, 2));
            NSFrameRect(NSMakeRect(sz.width-3, y+1, 1, 2));
            NSFrameRect(NSMakeRect(sz.width-2, y+2, 1, 2));
            NSFrameRect(NSMakeRect(sz.width-1, y+3, 1, 2));
        }
    }
    
}


/*
 
 Previously we had tried to draw things ourselves into the cache, but frankly it's just too difficult to get right,
 (coordinate transforms, clipping, etc.) and if Apple change how things are drawn, it may break if we do it ourselves.
 
 Now just hiding the stuff we don't want, and setting needs display on the stuff we do.
 
 static void _draw_view_now(NSView *the_view, NSGraphicsContext *the_context)
 {
 [the_context saveGraphicsState];
 NSAffineTransform *transform = [NSAffineTransform transform];
 [transform translateXBy:the_view.frame.origin.x yBy:the_view.frame.origin.y];
 [transform concat];
 
 [the_view drawRect:the_view.bounds];
 for (NSView *child in the_view.subviews)
 {
 _draw_view_now(child, the_context);
 }
 
 [the_context restoreGraphicsState];
 }
 */


@end
