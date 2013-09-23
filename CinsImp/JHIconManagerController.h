//
//  JHIconManagerController.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 7/09/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "JHIconGrid.h"
#include "acu.h"

@interface JHIconManagerController : NSWindowController <NSWindowDelegate>
{
    StackHandle _stack;
    __weak NSWindow *_parent_win;
    
    IBOutlet JHIconGrid *_grid;

}

- (id)initWithStack:(StackHandle)in_stack parentWindow:(NSWindow*)in_window;
- (StackHandle)stack;


@end
