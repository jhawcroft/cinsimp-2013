//
//  JHScriptEditorController.m
//  CinsImp
//
//  Created by Joshua Hawcroft on 6/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHScriptEditorController.h"

#import "JHMenubarController.h"
#import "JHVariableWatcher.h"


NSString *notifyScriptEditorDidClose = @"scriptEditorDidClose";



@implementation JHScriptEditorController


- (NSString*)windowNibName
{
    return @"JHScriptEditor";
}


- (void)dealloc
{
    stackmgr_handle_release(edited_object);
    //NSLog(@"dealloc script editor controller");
    if (xtalk_view)
    {
        [[xtalk_view enclosingScrollView] removeFromSuperview];
        //[xtalk_view removeFromSuperview];
        xtalk_view = nil;
    }
}


- (id)initWithStackHandle:(StackHandle)in_handle
{
    self = [super init];
    if (self)
    {
        _has_closed = NO;
        
        edited_object = in_handle;
        stackmgr_handle_retain(edited_object);
        
        acu_script_editor_opened(edited_object);
        
        [self loadWindow];
    }
    return self;
}




- (void)awakeFromNib
{
    /* set the title of the window */
    [[self window] setTitle:[NSString stringWithFormat:@"Script of %s",
                             stackmgr_handle_description(edited_object, STACKMGR_FLAG_LONG)]];
    
    /* load the script of the edited object here */
    char const *the_script;
    int const *the_checkpoints;
    int the_checkpoint_count;
    long the_sel_offset;
    if (stackmgr_script_get(edited_object, &the_script, &the_checkpoints, &the_checkpoint_count, &the_sel_offset) == STACK_ERR_NONE)
        [xtalk_view setScript:the_script checkpoints:the_checkpoints count:the_checkpoint_count selection:the_sel_offset];
    
    /* grab the script window position */
    int x,y,w,h;
    stackmgr_script_rect_get(edited_object, &x, &y, &w, &h);
    if (!((x == 0) && (y == 0) && (w == 0) && (h == 0)))
    {
        NSRect constrained = [self.window constrainFrameRect:NSMakeRect(x, y, w, h) toScreen:self.window.screen];
        [[self window] setFrame:constrained display:NO];
    }
    
    /* clear the window dirty indicator */
    [[self window] setDocumentEdited:NO];
    
    /* display the script editor */
    [[self window] makeKeyAndOrderFront:self];
}


- (void)saveScript
{
    /* if the script can't be saved successfully, just output an appropriate error message;
     thus deleted objects can still have open scripts (just like HC), good for editing anyway */
    char const *the_script;
    int const *the_checkpoints;
    int the_checkpoint_count;
    long the_sel_offset;
    [xtalk_view script:&the_script checkpoints:&the_checkpoints count:&the_checkpoint_count selection:&the_sel_offset];
    if (stackmgr_script_set(edited_object, the_script, the_checkpoints, the_checkpoint_count, the_sel_offset) == STACKMGR_ERROR_NO_OBJECT)
    {
        NSBeep();
        NSAlert *error = [[NSAlert alloc] init];
        [error setAlertStyle:NSCriticalAlertStyle];
        [error setMessageText:@"Sorry, the script couldn't be saved."];
        [error setInformativeText:@"The object no longer exists."];
        [error runModal];
    }
}


- (BOOL)windowShouldClose:(id)sender
{
    /* user has closed the window, not the program */
    
    /* check for unsaved changes */
    [self saveScript]; // ***TODO*** needs to be removed and handled via standard save & File menu, etc.
    // just for debugging at the moment - script auto-save
    // alternatively, we could build in an auto-save feature and just provide a File->Revert item??
    
    if (acu_script_is_active(edited_object))
    {
        acu_script_abort(edited_object);
        return NO; /* let the abort tell us to close the window */
    }
    
    return YES;
}


- (void)windowDidBecomeKey:(NSNotification *)notification
{
    [[JHMenubarController sharedController] setMode:MENU_MODE_SCRIPT];
}


- (void)windowWillClose:(NSNotification *)notification
{
    /* save window position */
    int x = self.window.frame.origin.x,y = self.window.frame.origin.y;
    int w = self.window.frame.size.width,h = self.window.frame.size.height;
    stackmgr_script_rect_set(edited_object, x, y, w, h);
    
    
    
    
    if (!_has_closed)
    {
        _has_closed = YES; /* necessary to stop us calling _closed() twice due to effects of
                            calling _closed() may cause closure of script editors and end of debug session */
        acu_script_editor_closed(edited_object);
        
        [[NSNotificationCenter defaultCenter] postNotification:
         [NSNotification notificationWithName:notifyScriptEditorDidClose object:self]];
    }
    
    

}


- (StackHandle)handle
{
    return edited_object;
}


- (void)setSelectedLine:(long)in_source_line debugging:(BOOL)in_debugging
{
    if (in_debugging)
        [xtalk_view showExecutingLine:in_source_line];
    else
        [xtalk_view showSourceLine:in_source_line];
}


- (IBAction)debugContinue:(id)sender
{
    acu_script_continue(edited_object);
}

- (IBAction)debugStepOver:(id)sender
{
    acu_debug_step_over(edited_object);
}

- (IBAction)debugStepOut:(id)sender
{
    acu_debug_step_out(edited_object);
}

- (IBAction)debugStepInto:(id)sender
{
    acu_debug_step_into(edited_object);
}


- (IBAction)debugAbort:(id)sender
{
    acu_script_abort(edited_object);
}


- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    if (acu_script_is_resumable(edited_object))
    {
        if ( (menuItem.action == @selector(debugContinue:)) ||
            (menuItem.action == @selector(debugStepOver:)) ||
            (menuItem.action == @selector(debugStepOut:)) ||
            (menuItem.action == @selector(debugStepInto:)) )
            return YES;
    }
    if (acu_script_is_active(edited_object))
    {
        if (menuItem.action == @selector(debugAbort:))
            return YES;
    }
    return NO;
}


- (void)windowDidBecomeMain:(NSNotification *)notification
{
    stackmgr_stack_set_active(edited_object);
    [[JHVariableWatcher sharedController] debugContextChanged];
}


@end
