//
//  JHVariableWatcherEditor.m
//  DevelopDebugWindows
//
//  Created by Joshua Hawcroft on 16/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHVariableWatcherEditor.h"

#import "JHVariableWatcher.h"

@implementation JHVariableWatcherEditor


- (void)keyDown:(NSEvent *)theEvent
{
    unichar the_char = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    if ((the_char == NSCarriageReturnCharacter) || (the_char == NSEnterCharacter))
    {
        if ([theEvent modifierFlags] & NSCommandKeyMask) [self insertText:@"\n" replacementRange:[self selectedRange]];
        else [[JHVariableWatcher sharedController] finishEdit];
    }
    else [super keyDown:theEvent];
}


- (void)cancelOperation:(id)sender
{
    [self setString:_original_text];
}


- (BOOL)becomeFirstResponder
{
    if ([super becomeFirstResponder])
    {
        _original_text = [[self string] copy];
        return YES;
    }
    else return NO;
}


- (BOOL)resignFirstResponder
{
    if ([super resignFirstResponder])
    {
        if (![_original_text isEqualToString:[self string]])
            [[JHVariableWatcher sharedController] saveEdit:[self string]];
        return YES;
    }
    else return NO;
}


@end
