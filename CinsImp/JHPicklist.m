/*
 
 Picklist
 JHPicklist.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHPicklist.h"

@implementation JHPicklist


- (void)setContent:(NSData*)in_content searchable:(NSString*)in_searchable editable:(BOOL)in_editable
{
    
}

- (BOOL)isButton
{
    return NO;
}

- (NSView*)eventTargetBrowse
{
    return self;
}

@end
