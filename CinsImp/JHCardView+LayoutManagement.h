//
//  JHCardView+LayoutManagement.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView+Base.h"

@interface JHCardView (LayoutManagement)

- (void)_destroyLayout;
- (void)_buildCard;
- (NSView<JHWidget>*)_widgetViewForID:(long)inID;
- (NSView*)_hitTarget:(NSPoint)aPoint;


- (NSImage*)_pictureOfBkgndObjects;
- (NSImage*)_pictureOfBkgndObjectsAndArt;
- (NSImage*)_pictureOfCardObjects;
- (NSImage*)_pictureOfCard;


- (void)acuLayoutWillChange;
- (void)acuLayoutDidChange;


/* automatic script status display */
- (void)setAutoStatusVisible:(BOOL)in_visible canAbort:(BOOL)in_can_abort;




@end
