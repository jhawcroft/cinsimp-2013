//
//  JHCardView_UIActions.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView+Base.h"

@interface JHCardView (JHCardView_UIActions)

/* pasteboard */
- (IBAction)delete:(id)sender;

/* object creation */
- (IBAction)newButton:(id)sender;
- (IBAction)newField:(id)sender;

/* selection */
- (IBAction)showPropertiesForStack:(id)sender;
- (IBAction)showPropertiesForCard:(id)sender;
- (IBAction)showPropertiesForBkgnd:(id)sender;

/* paint */
- (IBAction)toggleFatBits:(id)sender;
- (IBAction)toggleDrawFilled:(id)sender;

/* scripting */
- (void)editScript;
- (void)openScriptEditorFor:(StackHandle)in_edited_object atSourceLine:(long)in_source_line debugging:(BOOL)in_debugging;
- (void)closeScriptEditorsWithDelay:(BOOL)in_with_delay;




@end
