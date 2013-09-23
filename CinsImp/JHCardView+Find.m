//
//  JHCardView+Find.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView.h"

#import "JHWidget.h"
#import "JHTextFieldScroller.h"


@implementation JHCardView (Find)


/************
 Find
 */

- (void)_showFindIndicator:(long)in_field_id forRange:(NSRange)in_range
{
    NSView<JHWidget> *widget = [self _widgetViewForID:in_field_id];
    if ([widget isKindOfClass:[JHTextFieldScroller class]])
    {
        [((JHTextFieldScroller*)widget) setFindIndicator:in_range];
    }
}


- (void)find:(NSString*)in_terms mode:(StackFindMode)in_mode inField:(long)in_field
{
    [[self window] makeFirstResponder:nil];
    
    in_field = 0; // **TODO** interestingly setting this to -1 stops the find from working properly with no field
    stack_find(stack, stackmgr_current_card_id(stack), in_mode, (char*)[in_terms cStringUsingEncoding:NSUTF8StringEncoding], in_field, 0);
    
    while (stack_find_step(stack)) ;
    
    long found_card_id, found_field_id, offset, length;
    char *found_text;
    if (stack_find_result(stack, &found_card_id, &found_field_id, &offset, &length, &found_text))
    {
        [self goToCard:found_card_id];
        [self _showFindIndicator:found_field_id forRange:NSMakeRange(offset, length)];
        
        //NSLog(@"found field: %ld, offset=%ld, length=%ld, text=%s", found_field_id, offset, length, found_text);
    }
    else
    {
        NSBeep();
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setAlertStyle:NSCriticalAlertStyle];
        [alert setMessageText:[NSString stringWithFormat:@"Couldn't find \"%s\".", stack_find_terms(stack)]];
        [alert beginSheetModalForWindow:[self window] modalDelegate:nil didEndSelector:nil contextInfo:nil];
        [[self window] makeFirstResponder:self];
    }
}


@end
