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
 
 (see header for additional documentation)
 
 */

/******************
 Dependencies
 */

#import "JHCinsImp.h"

/* TODO: (LOW) header dependencies needs cleaning up */

#import "JHCardView.h"
#import "JHCardSize.h"

#include "stack.h"
#include "acu.h"
#include "xtalk_engine.h"

#import "JHCursorController.h"
#import "JHMenubarController.h"

#import "JHToolPaletteController.h"
#import "JHColourPaletteController.h"
#import "JHMessageWatcher.h"
#import "JHVariableWatcher.h"
#import "JHScriptEditorController.h"
#import "JHScriptEditorWindow.h"


/******************
 Configuration
 */

/*
 *  ACU_TIMER_INTERVAL
 *  ---------------------------------------------------------------------------------------------
 *  The interval in seconds for the ACU timer
 */
#define ACU_TIMER_INTERVAL 0.3


/*
 *  ACU_XTALK_TIMER_INTERVAL
 *  ---------------------------------------------------------------------------------------------
 *  The interval in seconds for the ACU xTalk timer
 */
#define ACU_XTALK_TIMER_INTERVAL 0.0


/*
 *  TAB_KEYCODE
 *  ---------------------------------------------------------------------------------------------
 *  Key code for the Tab key
 */
#define TAB_KEYCODE 48



/******************
 Globals (Storage)
 */

/* out of memory alert */
NSAlert *gAlert_memory;

/* application-wide xtalk engine */
XTE *g_app_xtalk = NULL;

/* TODO: (DEPRECATED) remove gCurrentCardView global as we move code to the ACU */
/* current card view */
JHCardView *gCurrentCardView = nil;



/******************
 Notification Constants
 */

NSString *scriptEditorBecameActive = @"scriptEditorActivated";
NSString *scriptEditorBecameInactive = @"scriptEditorInactivated";



@implementation JHCinsImp



- (void)checkEnterLeaveScriptEditor
{
    /* check if a script editor is the foreground window;
     generate enter/leave script editor notifications for the benefit
     of palette windows */
    BOOL now_in_se = ([NSApp mainWindow] &&
                      ([[NSApp mainWindow] isKindOfClass:[JHScriptEditorWindow class]]));
    if (now_in_se && (!_in_se))
    {
        _in_se = YES;
        [[NSNotificationCenter defaultCenter] postNotificationName:scriptEditorBecameActive object:nil];
    }
    else if ((!now_in_se) && _in_se)
    {
        _in_se = NO;
        [[NSNotificationCenter defaultCenter] postNotificationName:scriptEditorBecameInactive object:nil];
    }
}



/******************
 Timers
 */


/*
 *  acuTimer:
 *  ---------------------------------------------------------------------------------------------
 *  ACU timer has fired; invoke the ACU
 */
- (void)acuTimer:(id)sender
{
    /* if the user isn't pressing/touching, tell the ACU */
    if ([NSEvent pressedMouseButtons] == 0) acu_finger_end();
    
    [self checkEnterLeaveScriptEditor];
    
    acu_timer();
}


/*
 *  xTalkRapidFire:
 *  ---------------------------------------------------------------------------------------------
 *  ACU xTalk timer has fired; invoke the ACU
 *
 *  This timer is only running when there is at least one script executing;
 *  it runs at a much higher frequency than the ACU timer (see ACU_XTALK_TIMER_INTERVAL)
 */
- (void)xtalkRapidFire:(NSTimer*)in_timer
{
    acu_xtalk_timer();
}


/*
 *  adjustTimers:
 *  ---------------------------------------------------------------------------------------------
 *  Invoked indirectly by the ACU (via ACUGlue.m) when the timers need to be adjusted;
 *  we only run the xTalk timer when there is an active xTalk script executing
 */
- (void)adjustTimers:(BOOL)xtalk_is_active
{
    if (xtalk_rapid_fire_timer)
    {
        [xtalk_rapid_fire_timer invalidate];
        xtalk_rapid_fire_timer = nil;
    }
    if (xtalk_is_active)
        xtalk_rapid_fire_timer = [NSTimer scheduledTimerWithTimeInterval:ACU_XTALK_TIMER_INTERVAL
                                                                  target:self
                                                                selector:@selector(xtalkRapidFire:)
                                                                userInfo:nil
                                                                 repeats:YES];
}


/*
 *  installTimers
 *  ---------------------------------------------------------------------------------------------
 *  Install the ACU timer; we invoke this ourselves at application launch
 */
- (void)installTimers
{
    _acu_timer = [NSTimer scheduledTimerWithTimeInterval:ACU_TIMER_INTERVAL
                                                  target:self
                                                selector:@selector(acuTimer:)
                                                userInfo:nil
                                                 repeats:YES];
}



/******************
 Application Startup and Shutdown
 */

/*
 *  applicationWillTerminate:
 *  ---------------------------------------------------------------------------------------------
 *  Tells the ACU to close any open stacks and files,
 *  shutdown the xTalk interpreters safely, etc.
 *
 *  If there's anything we need to to in terms of saving preferences,
 *  we must do it prior to calling acu_quit().
 *
 *  !  acu_quit() may currently terminate the application before it exits.
 */
- (void)applicationWillTerminate:(NSNotification *)notification
{
    acu_quit();
}


/*
 *  prepareOutOfMemoryAlert
 *  ---------------------------------------------------------------------------------------------
 *  Pre-prepares an alert box to display in the event of an out of memory condition
 */
- (void)prepareOutOfMemoryAlert
{
    gAlert_memory = [[NSAlert alloc] init];
    [gAlert_memory setAlertStyle:NSCriticalAlertStyle];
    [gAlert_memory setMessageText:NSLocalizedString(@"OUT_OF_MEMORY", "out of memory alert")];
}


/*
 *  installGlobalKeyboardShortcuts
 *  ---------------------------------------------------------------------------------------------
 *  Installs a handler that filters keystrokes to look for shortcuts that are applicable 
 *  throughout CinsImp
 */
- (void)installGlobalKeyboardShortcuts
{
    _global_shortcut_handler = [NSEvent addLocalMonitorForEventsMatchingMask:(NSKeyDownMask) handler:
                                ^ NSEvent* (NSEvent* inEvent)
                                {
                                    /* option-tab: toggle tool palette */
                                    if ( ([inEvent modifierFlags] & NSAlternateKeyMask) &&
                                        ([inEvent keyCode] == TAB_KEYCODE) &&
                                        (!([inEvent modifierFlags] & NSShiftKeyMask)) &&
                                        (!([inEvent modifierFlags] & NSControlKeyMask)) )
                                    {
                                        [[JHToolPaletteController sharedController] toggleTools:self];
                                        return nil;
                                    }
                                    /* tab (when painting): toggle colours palette */
                                    else if ( (!([inEvent modifierFlags] & NSAlternateKeyMask)) &&
                                             ([inEvent keyCode] == TAB_KEYCODE) &&
                                             (!([inEvent modifierFlags] & NSShiftKeyMask)) &&
                                             (!([inEvent modifierFlags] & NSControlKeyMask)) &&
                                             TOOL_IS_PAINTING([[JHToolPaletteController sharedController] currentTool]) )
                                    {
                                        [[JHColourPaletteController sharedController] toggleColours:self];
                                        return nil;
                                    }
                                    
                                    /* pass the event along if we haven't handled it here */
                                    return inEvent;
                                }
                                ];
}


/*
 *  dummyThread:
 *  ---------------------------------------------------------------------------------------------
 *  Does nothing.  Used as the target for a dummy thread created at application launch to ensure
 *  that the Cocoa framework is in multithreaded mode
 */
- (void)dummyThread:(id)object
{
    /* do nothing */
}


extern int _xte_allocator_debug;

/*
 *  applicationDidFinishLaunching:
 *  ---------------------------------------------------------------------------------------------
 *  Frameworks and resources have loaded, start the rest of the application
 */
- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    /* put the Cocoa framework into multi-threaded mode */
    NSThread *dummy = [[NSThread alloc] initWithTarget:self selector:@selector(dummyThread:) object:nil];
    [dummy start];
    dummy = nil;
    assert([NSThread isMultiThreaded]);
    
    /* prepare an application out of memory alert ahead on when it's needed
     (there may not be enough memory to make it if it's needed!) */
    [self prepareOutOfMemoryAlert];
    
    /* startup and initalize the application control unit;
     via ACUGlue.m */
    void ACUGlueInit(void);
    ACUGlueInit();
    
    /* initalize menubar and cursor controllers */
    [JHCursorController sharedController];
    [[JHMenubarController sharedController] setMode:MENU_MODE_BROWSE];
    
    /* initalize UI palettes */
    [[JHToolPaletteController sharedController] setCurrentTool:AUTH_TOOL_BROWSE];
    
    propertiesPalette = [[JHPropertiesPalette alloc] initWithWindowNibName:@"JHPropertiesPalette"];
    [NSApp setNextResponder:propertiesPalette];
    
    messagePalette = [[JHMessagePalette alloc] initWithWindowNibName:@"JHMessagePalette"];
    [propertiesPalette setNextResponder:messagePalette];
    
    about = [[JHAbout alloc] initWithWindowNibName:@"JHAbout"];
    [messagePalette setNextResponder:about];
    
    JHMessageWatcher *message_watcher = [JHMessageWatcher sharedController];
    [about setNextResponder:message_watcher];
    
    JHVariableWatcher *variable_watcher = [JHVariableWatcher sharedController];
    [message_watcher setNextResponder:variable_watcher];
    
    /* initalize global keyboard shortcuts */
    [self installGlobalKeyboardShortcuts];
    
    /* install timers to get things running */
    [self installTimers];
    
    
    /* run tests in debug mode */
#if DEBUG
    xte_test();
    stack_test();
#endif
    
    /*
#if NDEBUG
    [[NSAlert alertWithMessageText:@"XTE Memory Allocator NDEBUG is defined" defaultButton:@"OK" alternateButton:nil otherButton:nil informativeTextWithFormat:@""] runModal];
#else
    [[NSAlert alertWithMessageText:@"XTE Memory Allocator NDEBUG is NOT defined" defaultButton:@"OK" alternateButton:nil otherButton:nil informativeTextWithFormat:@""] runModal];
#endif
    
#if NDEBUG != 1
    [[NSAlert alertWithMessageText:@"XTE Memory Allocator NDEBUG != 1" defaultButton:@"OK" alternateButton:nil otherButton:nil informativeTextWithFormat:@""] runModal];
#else
    [[NSAlert alertWithMessageText:@"XTE Memory Allocator NDEBUG otherwise" defaultButton:@"OK" alternateButton:nil otherButton:nil informativeTextWithFormat:@""] runModal];
#endif
    if (_xte_allocator_debug)
        [[NSAlert alertWithMessageText:@"XTE Memory Allocator DEBUG" defaultButton:@"OK" alternateButton:nil otherButton:nil informativeTextWithFormat:@""] runModal];
    else
        [[NSAlert alertWithMessageText:@"XTE Memory Allocator NORMAL" defaultButton:@"OK" alternateButton:nil otherButton:nil informativeTextWithFormat:@""] runModal];
    */
    
    
    /* print a warning to stdout if certain Cocoa memory debugging mechanisms are enabled;
     in production these mechanisms will result in leaks */
    if( getenv("NSZombieEnabled") || getenv("NSAutoreleaseFreedObjectCheckEnabled") ) { NSLog(@"NSZombieEnabled/NSAutoreleaseFreedObjectCheckEnabled enabled!"); }
}



/******************
 Behaviour Configuration
 */


/*
 *  applicationShouldOpenUntitledFile:
 *  ---------------------------------------------------------------------------------------------
 *  At startup, do not create an empty JHDocument instance - the only way a new stack can be
 *  created is via the newStack: method (see below)
 */
- (BOOL)applicationShouldOpenUntitledFile:(NSApplication *)sender
{
    return NO;
}


/*
 *  applicationShouldHandleReopen:hasVisibleWindows:
 *  ---------------------------------------------------------------------------------------------
 *  Don't do anything when the dock icon is clicked and the application is already running
 */
- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender
                    hasVisibleWindows:(BOOL)flag
{
    return NO;
}



/******************
 New Stack Creation
 */

/*
 *  newStack:
 *  ---------------------------------------------------------------------------------------------
 *  Initiates the process for creating a new stack; invoked via the menubar.
 *  Presents a standard Save dialog box and customises with options required for stack creation.
 */
- (IBAction)newStack:(id)sender
{
    /* create a Save panel to ask for the name and location of the new stack */
    NSSavePanel *savePanel = [NSSavePanel savePanel];
    
    /* add a card-size widget to the save panel to ask what size card the user wants */
    JHCardSize *cardSize = [[JHCardSize alloc] initWithNibName:@"JHCardSize" bundle:nil];
    [cardSize loadView];
    [savePanel setAccessoryView:[cardSize view]];

    /* specify the type of file being created is a CinsImp stack */
    [savePanel setAllowedFileTypes:[NSArray arrayWithObject:@"cinsstak"]];
    
    /* get the new file name and path;
     if the user cancels, exit */
    NSUInteger result = [savePanel runModal];
    if (result != NSFileHandlingPanelOKButton) return;
    
    /* obtain url, path and size for the new stack from the Save panel */
    NSURL *stack_url = [savePanel URL];
    NSString *stack_path = [stack_url path];
    NSSize card_size = [cardSize card_size];
    
    /* check if a stack with the name specified already exists */
    if ([[NSFileManager defaultManager] fileExistsAtPath:stack_path])
    {
        /* a stack already exists with the user's specified name;
         try to delete the existing stack */
        if (![[NSFileManager defaultManager] removeItemAtPath:stack_path error:nil])
        {
            /* an existing stack couldn't be deleted;
             notify the user and exit */
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setAlertStyle:NSCriticalAlertStyle];
            [alert setMessageText:[NSString stringWithFormat:NSLocalizedString(@"CANT_OVERWRITE_STACK",
                                                                               @"couldn't overwrite existing stack during New Stack"), [stack_path lastPathComponent]]];
            [alert setInformativeText:NSLocalizedString(@"CANT_OVERWRITE_STACK_INFO", @"")];
            [alert runModal];
            return;
        }
    }
    
    /* create new stack */
    StackMgrStackCreateDef stack_def = {
        card_size.width,
        card_size.height
    };
    int err = stackmgr_stack_create((char const*)[stack_path cStringUsingEncoding:NSUTF8StringEncoding], &stack_def);
    if (!err == STACKMGR_ERROR_NONE)
    
    /* create a new stack */
    {
        /* stack creation failed;
         notify the user and exit */
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setAlertStyle:NSCriticalAlertStyle];
        [alert setMessageText:[NSString stringWithFormat:NSLocalizedString(@"CANT_CREATE_STACK",
                                                                           @"couldn't create new stack on locked media"), [stack_path lastPathComponent]]];
        [alert setInformativeText:NSLocalizedString(@"CANT_CREATE_STACK_INFO", @"")];
        [alert runModal];
        return;
    }
    
    /* hide file extension */
    NSDictionary* fileAttrs = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                                          forKey:NSFileExtensionHidden];
    [[NSFileManager defaultManager] setAttributes:fileAttrs
                                     ofItemAtPath:stack_path
                                            error:NULL];
    
    /* stack creation was successful,
     close the new stack and open it using the document controller */
    [[NSDocumentController sharedDocumentController] openDocumentWithContentsOfURL:stack_url display:YES error:nil];
}



/******************
 No Memory 
 */

/*
 *  app_out_of_memory_void
 *  ---------------------------------------------------------------------------------------------
 *  Presents the global memory alert box and quits; returns void
 */
void app_out_of_memory_void(void)
{
    [gAlert_memory runModal];
    exit(EXIT_FAILURE);
}

/*
 *  app_out_of_memory_null
 *  ---------------------------------------------------------------------------------------------
 *  Presents the global memory alert box and quits; returns NULL
 */
void* app_out_of_memory_null(void)
{
    [gAlert_memory runModal];
    exit(EXIT_FAILURE);
    return NULL;
}


@end
