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
 JHCinsImp: Cocoa Front-end: Utilities
 
 Miscellaneous globally accessible utilities
 
 */

#import <Cocoa/Cocoa.h>



/*
 *  create_font_menu
 *  ---------------------------------------------------------------------------------------------
 *  Creates and returns a new font menu; enumerates all fonts on the system
 */
NSMenu* create_font_menu(void)
{
    NSMenu *font_menu = [[NSMenu alloc] init];
    NSMutableArray *font_list = [[NSMutableArray alloc] initWithArray:[[NSFontManager
                                                                        sharedFontManager] availableFontFamilies]];
    [font_list sortUsingSelector:@selector(caseInsensitiveCompare:)];
    for (NSString *font_name in font_list)
    {
        [font_menu addItemWithTitle:font_name action:nil keyEquivalent:@""];
    }
    
    return font_menu;
}



