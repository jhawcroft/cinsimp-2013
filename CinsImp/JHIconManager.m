/*
 
 CinsImp <http://www.cinsimp.net>
 Copyright (C) 2009-2013 Joshua Hawcroft
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 *************************************************************************************************
 JHCinsImp: Icon Manager UI
 
 Provides a window that allows the user to browse, edit, add and remove icons from within a
 specific stack.
 
 */

#import "JHIconManager.h"
#import "JHIconManagerController.h"
#import "JHIconGrid.h"

@implementation JHIconManager


- (NSData*)pngDataForImage:(NSImage*)in_image
{
    [in_image lockFocus];
    [[NSGraphicsContext currentContext] setShouldAntialias:NO];
    NSBitmapImageRep *bitmapRep = [[NSBitmapImageRep alloc]
                                   initWithFocusedViewRect:NSMakeRect(0, 0, in_image.size.width, in_image.size.height)];
    [in_image unlockFocus];
    return [bitmapRep representationUsingType:NSPNGFileType properties:Nil];
}


- (BOOL)importIconFile:(NSURL*)in_url
{
    /* load the image file */
    NSImage *image = [[NSImage alloc] initByReferencingURL:in_url];
    
    /* get the PNG data */
    NSData *data = [self pngDataForImage:image];
    
    /* tell ACU to create a new resource */
    int icon_id = acu_iconmgr_create([[self windowController] stack]);
    
    /* tell ACU to store PNG data */
    acu_iconmgr_mutate([[self windowController] stack], icon_id, NULL, NULL, [data bytes], [data length]);
    
    return YES; /* return NO to cancel importing any further */
}


- (IBAction)importIcon:(id)sender
{
    NSOpenPanel *open_panel = [NSOpenPanel openPanel];
    
    [open_panel setMessage:NSLocalizedString(@"PROMPT_IMPORT_ICON", @"Prompt to import icon")];
    [open_panel setAllowedFileTypes:[NSArray arrayWithObjects:@"png", @"tiff", nil]];
    [open_panel setAllowsMultipleSelection:YES];
    [open_panel setAllowsOtherFileTypes:NO];
    
    [open_panel beginSheetModalForWindow:self completionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton)
        {
            NSArray *urls = [open_panel URLs];
            for (NSURL *url in urls)
            {
                if (![self importIconFile:url]) return;
            }
            
            [_grid reload];
            
            //[[url path] UTF8String]
        }
    }];
}


- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    if (menuItem.action == @selector(importIcon:))
    {
        //[menuItem setHidden:NO];
        return YES;
    }
    return NO;
}


- (void)designSelectionDidChange:(NSNotification*)notification
{
    if (_choose)
    {
        _choose = NO;
        
    }
    
}


- (void)awakeFromNib
{
    _choose = NO;
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(designSelectionDidChange:)
                                                 name:@"DesignSelectionChange"
                                               object:nil];
}


- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}


- (void)chooseIcon
{
    [_grid selectNone];
    [self makeKeyAndOrderFront:self];
    _choose = YES;
}


- (BOOL)isChoosing
{
    return _choose;
}



@end
