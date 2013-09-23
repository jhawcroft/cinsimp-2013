/*
 
 Document
 JHDocument.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHDocument.h"

#import "JHVariableWatcher.h"
#import "JHToolPaletteController.h"


NSString *lastDocumentClosedNotification = @"lastDocumentClosed";


@implementation JHDocument


/**********
 Document Configuration
 */

- (NSString *)windowNibName
{
    return @"JHDocument";
}


+ (BOOL)autosavesInPlace
{
    return NO;
}


- (void)updateChangeCount:(NSDocumentChangeType)changeType
{
    /* deliberately empty; this stops NSDocument from recording a state of 'unsaved changes' */
}



/**********
 Window Behaviour
 */


/* returns the best fit window frame for the current card size */
- (NSRect)windowWillUseStandardFrame:(NSWindow *)window defaultFrame:(NSRect)newFrame
{
    long card_width, card_height;
    stack_get_card_size(stack, &card_width, &card_height);
    NSRect best_frame = NSMakeRect(newFrame.origin.x, newFrame.origin.y, card_width, card_height);
    best_frame = [window frameRectForContentRect:best_frame];
    best_frame.origin.y += newFrame.size.height;
    return best_frame;
}



/**********
 Opening
 */

- (void)windowControllerDidLoadNib:(NSWindowController *)aController
{
    [super windowControllerDidLoadNib:aController];
    [aController setShouldCascadeWindows:YES];
    
   
}

- (void)awakeFromNib
{
    script_editors = [[NSMutableArray alloc] init];
    
    //NSLog(@"awakeFromnib");
    /* set the size and position of the stack window */
    long width,height;
    stack_get_window_size(stack, &width, &height);
    [the_window setContentSize:NSMakeSize(width, height)];
    
    
    /* tell the card view to load the stack */
    [[the_window cardView] setStack:stack];
    
    /* stack was opened successfully, load an xtalk engine */
    //xtalk = stackmgr_stack_xtalk(_stack);
    
}


- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)typeName error:(NSError *__autoreleasing *)outError
{
    NSString *path = [url path];
    
    /* open stack (with new Stack Manager) */
    int error;
    _stack = stackmgr_stack_open((char const*)[path cStringUsingEncoding:NSUTF8StringEncoding],
                                 (__bridge void *)(self),
                                 &error);
    
    /* open the stack (with old direct file wrapper) */
    //StackOpenStatus status;
    //stack = stack_open((char*)[path cStringUsingEncoding:NSUTF8StringEncoding], &_handle_stack_fatal_error, (__bridge void*)self, &status);
    if (_stack == STACKMGR_INVALID_HANDLE)
    {
        /* stack couldn't be opened, return an appropriate error */
        switch (error)//status
        {
            case STACK_TOO_NEW:
                *outError = [NSError errorWithDomain:@"com.joshhawcroft.CardFile.errors" code:1 userInfo:
                             [NSDictionary dictionaryWithObjectsAndKeys:
                              [NSString stringWithFormat:@"Couldn't open stack \"%@\".", [path lastPathComponent]], NSLocalizedDescriptionKey,
                              @"The stack was created by a newer version of CinsImp.", NSLocalizedFailureReasonErrorKey, nil]];
                break;
            case STACK_TOO_OLD:
                *outError = [NSError errorWithDomain:@"com.joshhawcroft.CardFile.errors" code:1 userInfo:
                             [NSDictionary dictionaryWithObjectsAndKeys:
                              [NSString stringWithFormat:@"Couldn't open stack \"%@\".", [path lastPathComponent]], NSLocalizedDescriptionKey,
                              @"The stack was created by a very old version of CinsImp.", NSLocalizedFailureReasonErrorKey, nil]];
                break;
            case STACK_PERMISSION:
                *outError = [NSError errorWithDomain:@"com.joshhawcroft.CardFile.errors" code:1 userInfo:
                             [NSDictionary dictionaryWithObjectsAndKeys:
                              [NSString stringWithFormat:@"Couldn't open stack \"%@\".", [path lastPathComponent]], NSLocalizedDescriptionKey,
                              @"You may not have privileges to access the stack.", NSLocalizedFailureReasonErrorKey, nil]];
                break;
            case STACK_UNKNOWN:
                *outError = [NSError errorWithDomain:@"com.joshhawcroft.CardFile.errors" code:1 userInfo:
                             [NSDictionary dictionaryWithObjectsAndKeys:
                              [NSString stringWithFormat:@"Couldn't open stack \"%@\".", [path lastPathComponent]], NSLocalizedDescriptionKey,
                              @"There may not be enough memory to load the stack.", NSLocalizedFailureReasonErrorKey, nil]];
                break;
            case STACK_CORRUPT:
            default:
                *outError = [NSError errorWithDomain:@"com.joshhawcroft.CardFile.errors" code:1 userInfo:
                             [NSDictionary dictionaryWithObjectsAndKeys:
                              [NSString stringWithFormat:@"Couldn't open stack \"%@\".", [path lastPathComponent]], NSLocalizedDescriptionKey,
                              @"The stack is not a recognised format or is corrupt.", NSLocalizedFailureReasonErrorKey, nil]];
                break;
        }
        return NO;
    }
    
    /* provide warning if status is something other than OK */
    switch (error) //status
    {
        case STACK_RECOVERED:
        {
            NSAlert *warning = [[NSAlert alloc] init];
            [warning setMessageText:[NSString stringWithFormat:@"The stack \"%@\" was damaged, but has been repaired.",
                                     [path lastPathComponent]]];
            [warning setInformativeText:@"Some data loss may have occurred.  See the manual for possible causes of stack corruption."];
            [warning setAlertStyle:NSWarningAlertStyle];
            [warning runModal];
            break;
        }
        default: break;
    }
    
    /* get Stack* for old code that hasn't yet been updated */
    stack = stackmgr_stack_ptr(_stack);
    return YES;
}


/* included here for debugging to ensure document closure is not leaking; not required in production */
- (void)dealloc
{
    NSLog(@"doc dealloc");
    stackmgr_handle_release(_stack);
}




/**********
 Closing
 */


- (void)shouldCloseWindowController:(NSWindowController *)windowController delegate:(id)delegate shouldCloseSelector:(SEL)shouldCloseSelector contextInfo:(void *)contextInfo
{
    [[the_window cardView] setStack:NULL];
    [super shouldCloseWindowController:windowController delegate:delegate shouldCloseSelector:shouldCloseSelector contextInfo:contextInfo];
}


- (void)close
{
    //if (xtalk)
    //    xte_dispose(xtalk);
    //xtalk = NULL;
    
    stackmgr_stack_set_inactive(stack);
    [[JHVariableWatcher sharedController] debugContextChanged];
    
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:@"DesignSelectionChange" object:nil]];
    
    if ([[[NSDocumentController sharedDocumentController] documents] count] == 1)
        [[NSNotificationCenter defaultCenter] postNotification:
         [NSNotification notificationWithName:lastDocumentClosedNotification object:nil]];
    
    //stack_close(stack);
    stackmgr_stack_close(_stack);
    
    //[_progress_sheet close];
    
    [super close];
}



/**********
 Handling Errors
 */

- (void)beginFatalFileError
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setAlertStyle:NSCriticalAlertStyle];
    [alert setMessageText:@"Sorry, the stack \"%@\" must now close as there is a problem accessing the stack file."];
    [alert setInformativeText:@"The media containing the stack file has probably been removed."];
    [alert beginSheetModalForWindow:the_window modalDelegate:self didEndSelector:@selector(endFatalFileError:) contextInfo:nil];
}


- (void)endFatalFileError
{
    [self close];
}


/* invoked by the stack if something major goes wrong;
 for example, if the media containing the stack is improperly removed or goes offline */
static void _handle_stack_fatal_error(Stack *in_stack, void *in_context)
{
    JHDocument *stack_doc = (__bridge JHDocument*)in_context;
    [stack_doc beginFatalFileError];
}



/**********
 Printing
 */

- (NSPrintOperation*)printOperationWithSettings:(NSDictionary *)printSettings error:(NSError *__autoreleasing *)outError
{
    JHCardWindow *cardWindow = (JHCardWindow*)[[[self windowControllers] lastObject] window];
    return [NSPrintOperation printOperationWithView:[cardWindow cardView]];
}



/**********
 Accessors
 */

- (Stack*)stack
{
    return stack;
}


- (JHCardWindow*)window
{
    return the_window;
}


- (JHCardView*)view
{
    return [the_window cardView];
}



/**********
 Window Resize Handler
 */

/* save the new window size to the stack file */
- (void)windowDidEndLiveResize:(NSNotification *)notification
{
    NSWindow *window = [notification object];
    stack_set_window_size(stack, [window.contentView bounds].size.width, [window.contentView bounds].size.height);
}



/**********
 Script Control/Status
 */

- (void)scriptErrorDismissed:(NSAlert*)in_alert returnCode:(NSInteger)in_code contextInfo:(void *)contextInfo;
{
    switch (in_code)
    {
        case NSAlertFirstButtonReturn:
            acu_script_abort(stack);
            break;
        case NSAlertSecondButtonReturn:
            /* script */
            [[self view] openScriptEditorFor:stackmgr_script_error_target(stack)
                                atSourceLine:stackmgr_script_error_source_line(stack)
                                   debugging:NO];
            acu_script_abort(stack);
            break;
        case NSAlertThirdButtonReturn:
            /* debug */
            acu_script_debug(stack);
            [[self view] openScriptEditorFor:stackmgr_script_error_target(stack)
                                atSourceLine:stackmgr_script_error_source_line(stack)
                                   debugging:YES];
            break;
    }
}


/* DEFUNCT */
- (IBAction)abortRunningScript:(id)sender
{
    //[_progress_abort setEnabled:NO];
    acu_script_abort(stack);
}

/*
- (void)showScriptStatus:(NSTimer*)sender
{
    if (!script_status_should_show) return;
    [_progress_abort setEnabled:YES];
    [_progress_indicator startAnimation:self];
    [NSApp beginSheet:_progress_sheet
       modalForWindow:[self window]
        modalDelegate:self
       didEndSelector:nil
          contextInfo:NULL];
}*/

/*
- (void)setScriptStatusVisible:(BOOL)in_visible message:(NSString*)in_message abortText:(NSString*)in_abort_text abortable:(BOOL)in_abortable
{
    script_status_should_show = in_visible;
    if ((in_visible) && (![_progress_sheet isVisible]))
    {
        [NSTimer scheduledTimerWithTimeInterval:0.15 target:self selector:@selector(showScriptStatus:) userInfo:nil repeats:NO];
        
        //[_progress_sheet makeKeyAndOrderFront:self];
    }
    else if ((!in_visible) && ([_progress_sheet isVisible]))
    {
        
        [NSApp endSheet:_progress_sheet];
        [_progress_sheet orderOut:self];
        [_progress_indicator stopAnimation:self];
    }
}
*/

/**********
 Ask/Answer
 */

- (void)showAnswerDialogWithMessage:(NSString*)in_message button1:(NSString*)in_button1 button2:(NSString*)in_button2 button3:(NSString*)in_button3
{
    _last_answer_dlog = [[JHAnswerDialog alloc] initForOwner:self.window
                                                 withMessage:in_message
                                                     button1:in_button1
                                                     button2:in_button2
                                                     button3:in_button3
                                                       stack:stack];
}


- (void)showAskDialogWithMessage:(NSString*)in_message response:(NSString*)in_response forPassword:(BOOL)in_password_mode
{
    _last_answer_dlog = [[JHAskDialog alloc] initForOwner:self.window withMessage:in_message response:in_response forPassword:in_password_mode stack:stack];
}



- (void)showAskFileDialogWithMessage:(NSString*)in_message defaultFilename:(NSString*)in_default_filename
{
    NSSavePanel *ask_file_panel = [NSSavePanel savePanel];
    
    [ask_file_panel setMessage:in_message];
    [ask_file_panel setNameFieldStringValue:in_default_filename];
    
    [ask_file_panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton)
        {
            NSURL *url = [ask_file_panel URL];
            acu_ask_file_reply(stack, [[url path] UTF8String]);
        }
        else
            acu_ask_file_reply(stack, "");
    }];
    
}


- (void)showAnswerFileDialogWithMessage:(NSString*)in_message type1:(NSString*)in_type1 type2:(NSString*)in_type2 type3:(NSString*)in_type3
{
    NSOpenPanel *answer_file_panel = [NSOpenPanel openPanel];
    
    [answer_file_panel setMessage:in_message];
    
    NSMutableArray *allowed_types = [[NSMutableArray alloc] init];
    if (in_type1) [allowed_types addObject:in_type1];
    if (in_type2) [allowed_types addObject:in_type2];
    if (in_type3) [allowed_types addObject:in_type3];
    if ([allowed_types count] > 0)
        [answer_file_panel setAllowedFileTypes:allowed_types];
    
    [answer_file_panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton)
        {
            NSURL *url = [answer_file_panel URL];
            acu_answer_file_reply(stack, [[url path] UTF8String]);
        }
        else
            acu_answer_file_reply(stack, "");
    }];
}


- (void)showAnswerFolderDialogWithMessage:(NSString*)in_message
{
    NSOpenPanel *answer_file_panel = [NSOpenPanel openPanel];
    
    [answer_file_panel setMessage:in_message];
    
    [answer_file_panel setCanChooseDirectories:YES];
    [answer_file_panel setCanChooseFiles:NO];
    
    [answer_file_panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton)
        {
            NSURL *url = [answer_file_panel URL];
            acu_answer_folder_reply(stack, [[url path] UTF8String]);
        }
        else
            acu_answer_folder_reply(stack, "");
    }];
}


- (IBAction)devTest:(id)sender
{
    //[self showAskDialogWithMessage:@"What is your age?" response:@"I don't know" forPassword:YES];
}


/**********
 Current Document
 */

- (void)windowDidBecomeMain:(NSNotification *)notification
{
    stackmgr_stack_set_active(stack);
    
    [[JHToolPaletteController sharedController] setCurrentTool:[[self view] currentTool]];
    //stackmgr_set_tool(stack, [[self view] currentTool]);
    
    [[JHVariableWatcher sharedController] debugContextChanged];
}


- (void)windowDidResignMain:(NSNotification *)notification
{
    
}




/**********
 Debugging
 */

/* obtain how many bytes have been allocated in debug builds for all xtalk engine instances */
//long _xte_mem_allocated(void);
long _stack_mem_allocated(void);

- (void)logInternalHealth:(NSTimer*)in_timer
{
    return;
#if XTALK_TESTS
    /*static long xtalk_allocation = 0;
    if (xtalk)
    {
        long xtalk_new_allocation = _xte_mem_allocated();
        if (xtalk_new_allocation != xtalk_allocation)
        {

            printf("Stack \"%s\": xTalk allocation changed by: %ld bytes to %ld KB (%ld bytes)\n", [the_window.title UTF8String],
                  (xtalk_new_allocation - xtalk_allocation), xtalk_new_allocation / 1024, xtalk_new_allocation);
            xtalk_allocation = xtalk_new_allocation;
        }
    }*/
#endif
#if STACK_TESTS
    static long stack_allocation = 0;
    if (stack)
    {
        long stack_new_allocation = _stack_mem_allocated();
        if (stack_new_allocation != stack_allocation)
        {
            printf("Stack \"%s\": Stack allocation changed by: %ld bytes to %ld KB (%ld bytes)\n", [the_window.title UTF8String],
                  (stack_new_allocation - stack_allocation), stack_new_allocation / 1024, stack_new_allocation);
            stack_allocation = stack_new_allocation;
        }
    }
#endif
}


/**********
 Script Editor Management
 */

//- (void)openScript

/* suggest we build a referncing mechansim into Stack* to allow referencing objects using an opaque
 structure.  this way, we can get a reference to an object for which we want a script.  then keep 
 that within the script editing window;
 such references should be safe, that is, they shouldn't cause errors if the original object ceases
 to exist.  they should also be easily debuggable, preferably with a debug version stored along side
 when in debug mode (char*) #if DEBUG.  gradually as we develop app, we can port other systems to use
 these references and obsolete the existing separate property and text content accessors for various
 stack object classes. 
 xTalk glue MUST use it's own reference format, since it can know about more than just Stacks, Cards,
 etc., eg. Files and Folders, etc. But it could eventually wrap this same mechanism for neatness. */





@end
