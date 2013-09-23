//
//  JHCardView+LayoutSelection.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView+Base.h"

@interface JHCardView (LayoutSelection)


/* selection */
- (void)_deselectAll;
- (JHSelectorView*)_selectWidget:(NSView<JHWidget>*)inWidget withoutNotification:(BOOL)in_quiet;
- (void)_deselectWidget:(NSView<JHWidget>*)inWidget;
- (void)_selectedWidgetsAsCArray:(long**)out_widgets ofSize:(int*)out_size;
- (enum SelectionType)designSelectionType;
- (NSArray*)designSelectionIDs;
- (void)_saveSelection;
- (void)_restoreSelection;
- (void)_snapSelectionToGuides;


@end
