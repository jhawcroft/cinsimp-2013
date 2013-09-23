/*
 
 Transparent Button
 JHTransparentButton.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHTransparentButton.h"

@implementation JHTransparentButton

- (BOOL)isOpaque
{
    return NO;
}


- (void)setContent:(NSData*)in_content searchable:(NSString*)in_searchable editable:(BOOL)in_editable
{
    
}

- (BOOL)isButton
{
    return YES;
}

- (NSView*)eventTargetBrowse
{
    return self;
}






@end
