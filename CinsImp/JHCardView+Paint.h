//
//  JHCardView+Paint.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 30/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHCardView+Base.h"

@interface JHCardView (Paint)

- (void)_tellPaintAboutDisplay;

- (void)_openPaintSession;
- (void)_exitPaintSession;


- (void)_suspendPaint;
- (void)_resumePaint;

@end
