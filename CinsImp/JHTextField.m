/*
 
 Text Field
 JHTextField.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHTextField.h"

#import "JHTextFieldScroller.h"

#include "acu.h"

@implementation JHTextField


- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    if ( (menuItem.action == @selector(setTheFont:)) ||
        (menuItem.action == @selector(alignLeft:)) ||
        (menuItem.action == @selector(alignCenter:)) ||
        (menuItem.action == @selector(alignRight:)) ||
        (menuItem.action == @selector(alignJustified:)) ||
        (menuItem.action == @selector(toggleFontStyle:)) ||
        (menuItem.action == @selector(setTheSize:)))
    {
        if (![self isRichText]) return NO;
        
        if (menuItem.action == @selector(toggleFontStyle:))
        {
            NSDictionary *atts = [self typingAttributes];
            NSFont *atts_font = [atts objectForKey:NSFontAttributeName];
            NSFontTraitMask atts_mask = [[NSFontManager sharedFontManager] traitsOfFont:atts_font];
            switch (menuItem.tag)
            {
                case 2: // bold
                    [menuItem setState:(atts_mask & NSBoldFontMask)];
                    break;
                case 1: // italic
                    [menuItem setState:(atts_mask & NSItalicFontMask)];
                    break;
                case 3: // underline
                    [menuItem setState:([[atts objectForKey:NSUnderlineStyleAttributeName] intValue] > 0)];
                    break;
            }
            
        }
        else if (menuItem.action == @selector(setTheSize:))
        {
            NSDictionary *atts = [self typingAttributes];
            NSFont *atts_font = [atts objectForKey:NSFontAttributeName];
            [menuItem setState:([[menuItem title] intValue] == [atts_font pointSize])];
            
        }
        else if (menuItem.action == @selector(setTheFont:))
        {
            NSDictionary *atts = [self typingAttributes];
            NSFont *atts_font = [atts objectForKey:NSFontAttributeName];
            [menuItem setState:([[menuItem title] isEqualToString:[atts_font familyName]])];
        }
        return YES;
    }
    return [super validateMenuItem:menuItem];
    //return YES;
}



- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        //[self setDelegate:self];
        /*_selection_font = nil;
        _selection_atts = nil;
        _selection_traits = 0;*/
    }
    return self;
}


- (void)dealloc
{
    //[self setDelegate:nil];
    //NSLog(@"text field dealloc");
}


- (void)awakeFromNib
{
    [super awakeFromNib];
    //[self updateSelectionCache];
}


- (void)keyDown:(NSEvent *)event
{
    NSString *chars = [event characters];
    unichar character = [chars characterAtIndex: 0];
    
    //NSLog(@"keycode=%d", [theEvent keyCode]);
    if (character == NSTabCharacter) // tab
    {
        [self.card_view performSelector:@selector(_handleTab:) withObject:event];
        return;
    }
    else if (character == NSBackTabCharacter) // tab
    {
        [self.card_view performSelector:@selector(_handleBackTab:) withObject:event];
        return;
    }
    [super keyDown:event];
}


- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}


- (void)cursorUpdate:(NSEvent *)event
{
    [self.card_view _setAppropriateCursor];
}


- (BOOL)becomeFirstResponder
{
    BOOL result = [super becomeFirstResponder];
    if (result) {
        [((JHTextFieldScroller*)[self enclosingScrollView]) stripFindIndicator];
        [self.card_view performSelector:@selector(_handleFieldFocusChange:) withObject:[NSNumber numberWithLong:self.widget_id]];
    }
    return result;
}


- (BOOL)resignFirstResponder
{
    if (!acu_ui_acquire_stack([self.card_view stack])) return NO;
    
    BOOL result = [super resignFirstResponder];
    if (result) [self.card_view performSelector:@selector(_handleFieldFocusChange:) withObject:nil];
    return result;
}






- (void)setTheFont:(id)sender
{
    if ([sender isKindOfClass:[NSMenuItem class]])
    {
        NSFont *existing_font, *new_font;
        NSDictionary *atts = [self typingAttributes];//[[self textStorage] fontAttributesInRange:[self selectedRange]];
        existing_font = [atts objectForKey:NSFontAttributeName];
        new_font = [[NSFontManager sharedFontManager] convertFont:existing_font toFamily:[sender title]];
        [self setFont:new_font range:[self selectedRange]];
        [self setTypingAttributes:[NSDictionary dictionaryWithObjectsAndKeys:new_font, NSFontAttributeName, nil]];
        [self setNeedsDisplay:YES];
        
        [[self delegate] textDidChange:nil];
    }
}





- (void)toggleFontStyle:(id)sender
{
    if ([sender isKindOfClass:[NSMenuItem class]])
    {
        NSFont *existing_font, *new_font;
        NSMutableDictionary *atts = [[self typingAttributes] mutableCopy];//[[self textStorage] fontAttributesInRange:[self selectedRange]];
        existing_font = [atts objectForKey:NSFontAttributeName];
        NSFontTraitMask mask = [[NSFontManager sharedFontManager] traitsOfFont:existing_font];
        switch ([sender tag])
        {
            case 2: // bold
                if (mask & NSFontBoldTrait)
                    new_font = [[NSFontManager sharedFontManager] convertFont:existing_font toNotHaveTrait:NSBoldFontMask];
                else
                    new_font = [[NSFontManager sharedFontManager] convertFont:existing_font toHaveTrait:NSBoldFontMask];
                
                break;
            case 1: // italic
                if (mask & NSFontItalicTrait)
                    new_font = [[NSFontManager sharedFontManager] convertFont:existing_font toNotHaveTrait:NSItalicFontMask];
                else
                    new_font = [[NSFontManager sharedFontManager] convertFont:existing_font toHaveTrait:NSItalicFontMask];
                
                break;
            case 3: // underline
                if ([sender state])
                {
                    [[self textStorage] removeAttribute:NSUnderlineStyleAttributeName range:[self selectedRange]];
                    [atts removeObjectForKey:NSUnderlineStyleAttributeName];
                }
                else
                {
                    [[self textStorage] addAttribute:NSUnderlineStyleAttributeName value:[NSNumber numberWithInt:1] range:[self selectedRange]];
                    [atts setObject:[NSNumber numberWithInt:1] forKey:NSUnderlineStyleAttributeName];
                }
                break;
        }
        if (new_font)
        {
            [self setFont:new_font range:[self selectedRange]];
            [atts setObject:new_font forKey:NSFontAttributeName];
        }
        [self setTypingAttributes:atts];
        [self setNeedsDisplay:YES];
        
        [[self delegate] textDidChange:nil];
    }
}




- (void)setTheSize:(id)sender
{
    if ([sender isKindOfClass:[NSMenuItem class]])
    {
        NSFont *existing_font, *new_font;
        NSDictionary *atts = [self typingAttributes];//[[self textStorage] fontAttributesInRange:[self selectedRange]];
        existing_font = [atts objectForKey:NSFontAttributeName];// not getting the font for the selected range
        float existing_size = [existing_font pointSize];
        if ([sender tag] == 3)
            new_font = [[NSFontManager sharedFontManager] convertFont:existing_font toSize:existing_size + 2];
        else if ([sender tag] == 4)
            new_font = [[NSFontManager sharedFontManager] convertFont:existing_font toSize:existing_size - 2];
        else
            new_font = [[NSFontManager sharedFontManager] convertFont:existing_font toSize:[[sender title] integerValue]];
        [self setFont:new_font range:[self selectedRange]];
        [self setTypingAttributes:[NSDictionary dictionaryWithObjectsAndKeys:new_font, NSFontAttributeName, nil]];
        [self setNeedsDisplay:YES];
        
        [[self delegate] textDidChange:nil];
    }
}

/*
 NSString *chars = [event characters];
 unichar character = [chars characterAtIndex: 0];
 
 if (character == NSDeleteCharacter) {
 [self delete:self];
 }
 if (character == NSTabCharacter)
 
 if ([theEvent keyCode] == 48) // tab
 {
 [self.card_view performSelector:@selector(handleTabKey:) withObject:theEvent];
 return;
 }
 */

@end
