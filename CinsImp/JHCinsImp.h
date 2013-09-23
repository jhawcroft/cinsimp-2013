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
 JHCinsImp: Cocoa Front-end: Startup/Shutdown/Event Queue
 
 -------------------------------------------------------------------------------------------------
 About the CinsImp Architecture
 
 CinsImp is designed to be as platform-independent as possible.  Thus it doesn't rely heavily on
 the Cocoa application frameworks.  The user interface is a shell around the actual application 
 logic, which is either currently located in, or will be moved to the ACU.
 
 The ACU (Application Control Unit) is a C-language module intended to take most of the
 responsibility for the behaviour of the CinsImp application.
 
 Additional documentation on the CinsImp architecture is available on the Internals website:
 <http://www.cinsimp.net/project/internals/>
 
 */

/******************
 Dependencies
 */

#import <Foundation/Foundation.h>

#import "JHPropertiesPalette.h"
#import "JHAbout.h"
#import "JHMessagePalette.h"


/******************
 Instance
 */

@interface JHCinsImp : NSObject <NSApplicationDelegate>
{
    /* TODO: (DEPRECATED) we shouldn't need variables for controller references;
     rewrite to use shared controller */
    /* controllers for various utility windows */
    JHPropertiesPalette *propertiesPalette;
    JHMessagePalette *messagePalette;
    JHAbout *about;
    
    /* DEPRECATED - moving to menubar controller */
    //IBOutlet NSMenuItem *ro_menu;
    //IBOutlet NSMenu *obj_menu;
    
    /* global event filter; supports keyboard shortcuts
     that are implemented across CinsImp */
    id _global_shortcut_handler;
    
    /* timers used to drive "idle" events
     and routinely poll the ACU;
     should be possible to put a custom event
     directly into the application event queue
     and avoid doing so much polling */
    NSTimer *_acu_timer;
    
    /* this timer is only initalized when there is
     one or more scripts running
     so we don't waste CPU cycles when things are idle */
    NSTimer *xtalk_rapid_fire_timer;
    
    
    BOOL _in_se;
}

/* presents the standard Save dialog and allows the user to create a new stack */
- (IBAction)newStack:(id)sender;

/* the ACU calls this via ACUGlue.m
 when the interval of our timers needs to be changed */
- (void)adjustTimers:(BOOL)xtalk_is_active;


@end
