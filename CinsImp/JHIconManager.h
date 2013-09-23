//
//  JHIconManager.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 7/09/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "JHIconGrid.h"

@interface JHIconManager : NSWindow
{
    IBOutlet JHIconGrid *_grid;
    
    BOOL _choose;
}


- (IBAction)importIcon:(id)sender;

- (void)chooseIcon;
- (BOOL)isChoosing;


@end
