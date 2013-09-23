//
//  JHIconManagerController.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 7/09/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHIconManagerController.h"

#import "JHIconManager.h"


@interface JHIconManagerController ()

@end

@implementation JHIconManagerController



- (NSString*)windowNibName
{
    return @"JHIconManagerController";
}


- (void)dealloc
{
    acu_release(_stack);
}


- (id)initWithStack:(StackHandle)in_stack parentWindow:(NSWindow*)in_window
{
    self = [super init];
    if (self)
    {
        _stack = in_stack;
        acu_retain(in_stack);
        
        _parent_win = in_window;
        
        [self loadWindow];
    }
    return self;
}


- (StackHandle)stack
{
    return _stack;
}


- (void)awakeFromNib
{
   
    /* set the title of the window */
    [[self window] setTitle:[NSString stringWithFormat:@"Icons of %s",
                             stackmgr_handle_description(_stack, STACKMGR_FLAG_LONG)]];
    
    [[self window] setParentWindow:_parent_win];
    
    [_grid setStack:_stack];
}


- (void)windowDidBecomeKey:(NSNotification *)notification
{
    [_grid postSelectionNotification];
}


- (void)windowWillClose:(NSNotification *)notification
{
    if (![(JHIconManager*)self.window isChoosing])
        [[NSNotificationCenter defaultCenter] postNotificationName:@"DesignSelectionChange" object:nil];
}




@end
