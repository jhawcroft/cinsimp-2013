//
//  JHIconGrid.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 7/09/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "acu.h"

@interface JHIconGrid : NSView
{
    StackHandle _stack;
    
    NSSize _cell_size;
    int _thumbnail_size;
    NSDictionary *_caption_atts;
    int _caption_height;
    
    int _grid_cols;
    int _grid_rows;
    int _count;
    
    int _selected_index;
    
    NSMutableArray *_full_captions;
}

- (void)setStack:(StackHandle)in_stack;
- (StackHandle)stack;

- (BOOL)hasSelection;
- (int)selectedIconIndex;

- (void)reload;

- (void)postSelectionNotification;
- (void)selectNone;

@end
