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
 JHCinsImp: Cocoa <-> ACU Glue
 
 Glue between the Cocoa user interface/application front-end and the ACU
 
 Although the ACU is threaded internally, the majority of callbacks are invoked on the 
 main thread.  Exceptions are explcitly noted in the function comments.
 
 */


/******************
 Dependencies
 */

#import <Cocoa/Cocoa.h>

#import "JHCinsImp.h"

#import "JHDocument.h"

#import "JHMessagePalette.h"
#import "JHMessageWatcher.h"
#import "JHMenubarController.h"
#import "JHVariableWatcher.h"
#import "JHAnswerDialog.h"

#include "acu.h"

#include "jhassert.h"



/******************
 Utilities
 */

/*
 *  _no_open_stack
 *  ---------------------------------------------------------------------------------------------
 *  Displays a message that a specific command cannot be used when there is no open stack,
 *  for example, the "find" command which makes no sense when there's nothing open
 */
static void _no_open_stack(void)
{
    NSBeep();
    NSAlert *alert = [NSAlert alertWithMessageText:NSLocalizedString(@"NO_OPEN_STACK", @"can't perform that action")
                                     defaultButton:@"Cancel" alternateButton:nil otherButton:nil
                         informativeTextWithFormat:@""];
    [alert runModal];
}



/******************
 Callback Handlers (Stack Specific)
 */


/*
 *  _auto_status_control
 *  ---------------------------------------------------------------------------------------------
 *  Controls the visibility of the automatic status overlay that is displayed atop the card
 *  when a long running script is executing
 */
static void _auto_status_control(JHDocument *in_document, int in_visible, int in_can_abort)
{
    assert(in_document != NULL);
    assert(IS_BOOL(in_visible));
    assert(IS_BOOL(in_can_abort));
    
    /* JHCardView is responsible for the status overlay */
    [[in_document view] setAutoStatusVisible:in_visible canAbort:in_can_abort];
}


/*
 *  _enter_leave_debugger
 *  ---------------------------------------------------------------------------------------------
 *  ACU requests that the debug interface be made active/inactive, we respond by informing the
 *  menubar controller to change the visible menus and requesting an appropriate object's script
 *  be opened for inspection.
 */
static void _enter_leave_debugger(JHDocument *in_document, int is_debugging, StackHandle in_handler_object, long in_source_line)
{
    assert(in_document != NULL);
    assert(IS_BOOL(is_debugging));
    
    /* show/hide debug menu */
    [[JHMenubarController sharedController] setDebugging:is_debugging];
    
    if (is_debugging)
    {
        if (in_handler_object == ACU_INVALID_HANDLE)
        {
            /* reset open script editors to no longer show 'line executing highlight';
             potentially close script editors which weren't already open */
            [[in_document view] closeScriptEditorsWithDelay:YES];
        }
        else
        {
            /* display the line about to execute */
            [[in_document view] openScriptEditorFor:in_handler_object atSourceLine:in_source_line debugging:YES];
        }
    }
    else
    {
        /* reset open script editors to no longer show 'line executing highlight' */
        [[in_document view] closeScriptEditorsWithDelay:NO];
    }
}


/*
 *  _script_debug
 *  ---------------------------------------------------------------------------------------------
 *  ACU requests that the currently executing line be displayed within the open script editor
 *  and possibly that a different object's script should now be opened for inspection.
 */
static void _script_debug(JHDocument *in_document, StackHandle in_handler_object, long in_source_line)
{
    assert(in_document != NULL);
    assert(in_handler_object != NULL);
    assert(IS_LINE_NUMBER(in_source_line) || IS_INVALID_LINE_NUMBER(in_source_line));
    
    [[in_document view] openScriptEditorFor:in_handler_object atSourceLine:in_source_line debugging:YES];
}


/*
 *  _handle_stack_closed
 *  ---------------------------------------------------------------------------------------------
 *  Currently invoked if the stack SHOULD be closed due to an error, most probably file I/O;
 *  may eventually be used to indicate that the user interface for an open stack should now be
 *  destroyed as the stack is no longer open.
 */
static void _handle_stack_closed(JHDocument *in_document, int in_error_code)
{
    assert(in_document != NULL);

    NSString *error_message = [NSString stringWithFormat:NSLocalizedString(@"UNEXPECTED_STACK_CLOSURE",
                                                                           @"unexpected stack closure alert text"),
                               [[in_document window] title]];
    [in_document close];
    
    NSAlert *alert = [NSAlert alertWithMessageText:error_message
                                     defaultButton:NSLocalizedString(@"OK", "OK button") alternateButton:nil otherButton:nil
                         informativeTextWithFormat:NSLocalizedString(@"UNEXPECTED_STACK_CLOSURE_INFO",
                                                                     @"explanation for unexpected stack closure alert")];
    [alert setAlertStyle:NSWarningAlertStyle];
    [alert runModal];
}


/*
 *  _handle_script_error
 *  ---------------------------------------------------------------------------------------------
 *  Raises a script error alert box; optionally with the option to open the script of the 
 *  problem object, or "Debug".
 *
 *  If the ACU indicates the error is a runtime error and there is a source object (the problem
 *  didn't occur within the message box) then the "Debug" option is enabled.
 *
 *  If there is a source object (the problem wasn't in the message box itself) the "Script"
 *  option is enabled.
 *
 *  The error message is supplied as <in_template> and may take one or more parameters '%s'.
 *  Error messages that will require localization can be found within the xTalk sources by looking
 *  for the ERROR_RUNTIME and ERROR_SYNTAX macros.
 *
 *  A comprehensive list of error messages is maintained on the CinsImp Internals website:
 *  <http://www.cinsimp.net/project/internals/>
 */
static void _handle_script_error(JHDocument *in_document, StackHandle in_source_object, long in_source_line,
                                 char const *in_template, char const *in_arg1, char const *in_arg2, char const *in_arg3, int in_runtime)
{
    assert(in_document != NULL);
    assert(IS_LINE_NUMBER(in_source_line) || IS_INVALID_LINE_NUMBER(in_source_line));
    assert(in_template != NULL);
    assert(IS_BOOL(in_runtime));
    
    /* localize and fill template;
     we must convert any parameters in the template from C string placeholders
     to Objective-C string placeholders */
    NSString *template = [[NSBundle mainBundle] localizedStringForKey:[NSString stringWithCString:in_template encoding:NSUTF8StringEncoding]
                                                                value:nil table:nil];
    NSMutableString *template_adjust = [template mutableCopy];
    [template_adjust replaceOccurrencesOfString:@"%s"
                                     withString:@"%@"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0, [template_adjust length])];
    template = [template_adjust copy];
    NSString *arg1 = nil, *arg2 = nil, *arg3 = nil;
    if (in_arg1 != NULL) arg1 = [NSString stringWithCString:in_arg1 encoding:NSUTF8StringEncoding];
    if (in_arg2 != NULL) arg2 = [NSString stringWithCString:in_arg2 encoding:NSUTF8StringEncoding];
    if (in_arg3 != NULL) arg3 = [NSString stringWithCString:in_arg3 encoding:NSUTF8StringEncoding];
    
    NSString *localized = nil;
    if (arg1 && arg2 && arg3)
        localized = [NSString stringWithFormat:template, arg1, arg2, arg3];
    else if (arg1 && arg2)
        localized = [NSString stringWithFormat:template, arg1, arg2];
    else if (arg1)
        localized = [NSString stringWithFormat:template, arg1];
    else
        localized = template;
    
    /* prepare an appropriate alert */
    NSAlert *alert;
    alert = [[NSAlert alloc] init];
    
    [alert setAlertStyle:NSWarningAlertStyle];
    [alert setMessageText:localized];
    [alert addButtonWithTitle:@"Cancel"];
    
    if (in_source_line > 0)
    {
        [alert addButtonWithTitle:@"Script"];
        
        if (in_runtime)
        {
            [alert addButtonWithTitle:@"Debug"];
        }
    }
    
    /* display the alert;
     JHDocument handles the various buttons, Cancel, Script and Debug;
     the ACU supplies the source object and line number again when we need them */
    NSBeep();
    if (!in_document)
        [alert runModal];
    else
        [alert beginSheetModalForWindow:[in_document window]
                          modalDelegate:in_document
                         didEndSelector:@selector(scriptErrorDismissed:returnCode:contextInfo:)
                            contextInfo:nil];
}


/*
 *  _handle_view_refresh
 *  ---------------------------------------------------------------------------------------------
 *  Repaints the card; handled by JHCardView
 */
static void _handle_view_refresh(JHDocument *in_document)
{
    assert(in_document != NULL);
    
    [[in_document view] refreshCard];
}


/*
 *  _handle_find
 *  ---------------------------------------------------------------------------------------------
 *  Invokes the "find" command on the stack, via JHCardView & the Stack unit
 */
/* TODO: DEPRECATED: "find" should be implemented within the ACU;
 and only final display/highlighting driven via callback */ 
static void _handle_find(JHDocument *in_document, char const *in_terms, StackFindMode in_mode, long in_field_id)
{
    assert(in_document != NULL);
    assert(in_terms != NULL);
    
    /* find is not available when there is no open stack */
    if (!in_document) return _no_open_stack();
    
    [[in_document view] find:[NSString stringWithCString:in_terms encoding:NSUTF8StringEncoding]
                        mode:in_mode
                     inField:in_field_id];
}


/*
 *  _save_screen
 *  ---------------------------------------------------------------------------------------------
 *  The ACU requests we save a copy of whatever appears on the card at present;
 *  this is in preparation for a visual effect transition from the previously displayed card
 *  to the new one, or upon 'unlocking the screen' in CinsTalk parlance
 *
 *  From this point onwards, only visual effects rendered and/or the last saved screen can be 
 *  displayed by the card view until _release_screen()
 */
void _save_screen(JHDocument *in_document)
{
    assert(in_document != NULL);
    
    [[in_document view] saveScreen];
}


/*
 *  _release_screen
 *  ---------------------------------------------------------------------------------------------
 *  The ACU requests we release the card view to display as it pleases; this occurs at idle
 *  after script has completed and is the final stage in 'unlocking'
 */
void _release_screen(JHDocument *in_document)
{
    assert(in_document != NULL);
    
    [[in_document view] releaseScreen];
}


/*
 *  _render_effect
 *  ---------------------------------------------------------------------------------------------
 *  We request the JHCardView to render a visual effect transition from whatever card was last
 *  saved using _save_screen() above, to the supplied <in_dest>.
 */
void _render_effect(JHDocument *in_document, int in_effect, int in_speed, int in_dest)
{
    assert(in_document != NULL);
    
    [[in_document view] renderEffect:in_effect speed:in_speed destination:in_dest];
}


/*
 *  _view_layout_will_change
 *  ---------------------------------------------------------------------------------------------
 *  The ACU is notifying us it's about to change the card layout, possibly by changing the
 *  current card.  If we are doing anything, such as having an open paint session, we must 
 *  suspend it.
 */
void _view_layout_will_change(JHDocument *in_document)
{
    assert(in_document != NULL);
    
    [[in_document view] acuLayoutWillChange];
}


/*
 *  _view_layout_did_change
 *  ---------------------------------------------------------------------------------------------
 *  Paired with _view_layout_will_change() above, this notifies us the layout change has been
 *  completed.  Whatever the UI was doing prior to the change should be resumed (as closely as
 *  possible - bearing in mind things are unlikely to be the same).
 */
void _view_layout_did_change(JHDocument *in_document)
{
    assert(in_document != NULL);
    
    [[in_document view] acuLayoutDidChange];
}


/*
 *  _answer_choice
 *  ---------------------------------------------------------------------------------------------
 *  Asks JHDocument to display an answer dialog sheet. 
 *  The user response is sent back to the ACU in a separate unit. 
 */
static void _answer_choice(JHDocument *in_document, char const *in_message,
                           char const *in_btn1, char const *in_btn2, char const *in_btn3)
{
    assert(in_document != NULL);
    assert(in_message != NULL);
    
    NSString *btn1, *btn2, *btn3;
    
    if (in_btn1) btn1 = [NSString stringWithCString:in_btn1 encoding:NSUTF8StringEncoding];
    else btn1 = nil;
    if (in_btn2) btn2 = [NSString stringWithCString:in_btn2 encoding:NSUTF8StringEncoding];
    else btn2 = nil;
    if (in_btn3) btn3 = [NSString stringWithCString:in_btn3 encoding:NSUTF8StringEncoding];
    else btn3 = nil;
    
    [in_document showAnswerDialogWithMessage:[NSString stringWithCString:in_message encoding:NSUTF8StringEncoding]
                                     button1:btn1 button2:btn2 button3:btn3];
}


/*
 *  _answer_file
 *  ---------------------------------------------------------------------------------------------
 *  Asks JHDocument to display a standard Open file dialog sheet.
 *  The user response is sent back to the ACU in a separate unit.
 */
static void _answer_file(JHDocument *in_document, char const *in_message,
                         char const *in_type1, char const *in_type2, char const *in_type3)
{
    assert(in_document != NULL);
    assert(in_message != NULL);
    
    NSString *type1, *type2, *type3;
    
    if (in_type1) type1 = [NSString stringWithCString:in_type1 encoding:NSUTF8StringEncoding];
    else type1 = nil;
    if (in_type2) type2 = [NSString stringWithCString:in_type2 encoding:NSUTF8StringEncoding];
    else type2 = nil;
    if (in_type3) type3 = [NSString stringWithCString:in_type3 encoding:NSUTF8StringEncoding];
    else type3 = nil;
    
    [in_document showAnswerFileDialogWithMessage:[NSString stringWithCString:in_message encoding:NSUTF8StringEncoding]
                                           type1:type1 type2:type2 type3:type3];
}


/*
 *  _answer_folder
 *  ---------------------------------------------------------------------------------------------
 *  Asks JHDocument to display a standard Open folder dialog sheet.
 *  The user response is sent back to the ACU in a separate unit.
 */
static void _answer_folder(JHDocument *in_document, char const *in_message)
{
    assert(in_document != NULL);
    assert(in_message != NULL);
    
    [in_document showAnswerFolderDialogWithMessage:[NSString stringWithCString:in_message encoding:NSUTF8StringEncoding]];
}


/*
 *  _ask_text
 *  ---------------------------------------------------------------------------------------------
 *  Asks JHDocument to display an ask dialog sheet.
 *  The user response is sent back to the ACU in a separate unit.
 */
static void _ask_text(JHDocument *in_document, char const *in_message, char const *in_response, int in_password_mode)
{
    assert(in_document != NULL);
    assert(in_message != NULL);
    assert(in_response != NULL);
    assert(IS_BOOL(in_password_mode));
    
    [in_document showAskDialogWithMessage:[NSString stringWithCString:in_message encoding:NSUTF8StringEncoding]
                                 response:[NSString stringWithCString:in_response encoding:NSUTF8StringEncoding]
                              forPassword:in_password_mode];
}


/*
 *  _ask_file
 *  ---------------------------------------------------------------------------------------------
 *  Asks JHDocument to display a standard Save file dialog sheet.
 *  The user response is sent back to the ACU in a separate unit.
 */
static void _ask_file(JHDocument *in_document, char const *in_message, char const *in_default_filename)
{
    assert(in_document != NULL);
    assert(in_message != NULL);
    assert(in_default_filename != NULL);
    
    [in_document showAskFileDialogWithMessage:[NSString stringWithCString:in_message encoding:NSUTF8StringEncoding]
                              defaultFilename:[NSString stringWithCString:in_default_filename encoding:NSUTF8StringEncoding]
     ];
}



/******************
 Callback Handlers (Application General)
 */

/*
 *  _handle_fatal_error
 *  ---------------------------------------------------------------------------------------------
 *  Something really bad has happened.  Display an error message and terminate the application.
 */
static void _handle_fatal_error(int in_error_code)
{
    NSBeep();
    NSAlert *alert = [NSAlert alertWithMessageText:NSLocalizedString(@"INTERNAL_ERROR", @"CinsImp application")
                                     defaultButton:NSLocalizedString(@"Quit","Quit button") alternateButton:nil otherButton:nil
                         informativeTextWithFormat:@""];
    [alert runModal];
    
    exit(EXIT_FAILURE);
}


/*
 *  _handle_message_result
 *  ---------------------------------------------------------------------------------------------
 *  Set the content of the message box
 */
static void _handle_message_result(const char *in_result)
{
    assert(in_result != NULL);
    
    /* set the text of the message box */
    NSString *msg = [NSString stringWithCString:in_result encoding:NSUTF8StringEncoding];
    [[JHMessagePalette sharedController] setText:msg];
    
    /* show the message box if it's not visible */
    [[[JHMessagePalette sharedController] window] displayIfNeeded];
    if (![[[JHMessagePalette sharedController] window] isVisible])
        [[[JHMessagePalette sharedController] window] orderFront:nil];
}



/*
 *  _do_beep
 *  ---------------------------------------------------------------------------------------------
 *  Play the default system alert sound
 *
 *  ! May be invoked by a thread other than the main thread
 */
static void _do_beep(void)
{
    @autoreleasepool { /* required due to secondary xTalk thread invocation */
        
#if DEBUG
        NSLog(@"Beep!"); /* make the beep visible when debugging CinsImp */
#endif
        NSBeep();
        usleep(500000); /* wait half a second to help ensure multiple beeps are played;
                         if NSBeep() is called multiple times in rapid succession it
                         generally only emits a single beep */
        
        /* TODO: consider replacing the system beep with an alternative noise;
         1) we can guarantee it will always play
         2) we needn't wait for it to complete
         Doesn't play well with system preferences though, 
         given there's an existing Apple API for this purpose,
         but given CinsImp is not a 'normal' application it may be sensible;
         just as long as we don't start allowing the user to build standalone apps
         that subvert Apple's apparent policy on annoying beeps! */
    }
}


/*
 *  _handle_stackmgr_mutate_rtf
 *  ---------------------------------------------------------------------------------------------
 *  Mutates the supplied RTF data by replacing the specified <in_edit_range> of characters with
 *  a <in_new_string>.  Also outputs a plain-text UTF-8 version of the newly mutated text.
 *
 *  It is vaguely possible that the supplied range is out of bounds or nonsensical;
 *  in that case the entire text block shall be replaced with the new text.
 *  (its quite possible this will cause data loss for the user,
 *  but the alternative is that we don't do anything at all which would create
 *  an even bigger problem - a field's value would no longer be able to be updated, ever.)
 *
 *  It's also worth noting the only time the range has ever been invalid is in the past when 
 *  this method was not returning a string in a safe way.  As that bug has since been fixed,
 *  it seems unlikely this should ever be an issue?
 *
 *  TODO:  The ACU should use styled text in a form that is cross-platform and can be supported internally;
 *  without reliance upon platform specific APIs/toolkits/callbacks - that could include using a freely
 *  available library if one is available.
 *
 *  ! May be invoked by a thread other than the main thread
 */
static void _handle_mutate_rtf(void **io_rtf, long *io_size, char **out_plain,
                                        char const *in_new_string, XTETextRange in_edit_range)
{
    assert(io_rtf != NULL);
    assert(io_size != NULL);
    assert(out_plain != NULL);
    assert(in_new_string != NULL);
    
    @autoreleasepool { /* required due to secondary xTalk thread invocation */
        
        /* convert the RTF data into a mutable attributed string */
        NSMutableAttributedString *the_text = NULL;
        if (*io_size == 0)
            the_text = [[NSMutableAttributedString alloc] init];
        else
            the_text = [[NSMutableAttributedString alloc] initWithRTFD:[NSData dataWithBytes:*io_rtf length:*io_size] documentAttributes:NULL];
        
        /* convert the new text into a string */
        NSString *new_text = [NSString stringWithCString:in_new_string encoding:NSUTF8StringEncoding];
        
        /* verify the supplied range and fix it if it's incorrect;
         mutate the rich text */
        if ((in_edit_range.offset > [the_text length]) ||
            (in_edit_range.offset + in_edit_range.length > [the_text length]))
        {
            [the_text replaceCharactersInRange:NSMakeRange(0, [the_text length]) withString:new_text];
        }
        else
            [the_text replaceCharactersInRange:NSMakeRange(in_edit_range.offset, in_edit_range.length) withString:new_text];
        
        /* return the RTF data and a plain text UTF-8 string to the ACU */
        NSData *oc_formatted_data = [the_text RTFDFromRange:NSMakeRange(0, the_text.length) documentAttributes:nil];
        NSString *oc_plain = [the_text string];
        
        *io_rtf = acu_callback_result_data([oc_formatted_data bytes], (int)[oc_formatted_data length]);
        *io_size = [oc_formatted_data length];
        *out_plain = acu_callback_result_string([oc_plain UTF8String]);
    }
}


/*
 *  _adjust_timers
 *  ---------------------------------------------------------------------------------------------
 *  ACU is asking us to adjust the timing calls we provide to it; either to enable/disable the
 *  secondary xTalk execution timer, this is handled by JHCinsImp
 *
 *  TODO:  Investigate posting messages to the main event queue instead of using rapid timers;
 *  this is a clumsy way of implementing inter-thread communication and wastes CPU cycles
 *  as well as making the app power hungry on mobile devices
 */
static void _adjust_timers(int in_xtalk_active)
{
    assert(IS_BOOL(in_xtalk_active));
    
    [[NSApp delegate] adjustTimers:in_xtalk_active];
}



/*
 *  _handle_builtin_resource_path
 *  ---------------------------------------------------------------------------------------------
 *  Returns the path to the built-in resources stack which is embedded in the bundle on OS X
 *  and iOS.
 */
static char const* _handle_builtin_resource_path(void)
{
    return [[[NSBundle mainBundle] pathForResource:@"BuiltinResources" ofType:@"cinsstak"] UTF8String];
}



/* TODO: remove document parameter from this callback;
 ensure the ACU blocks other running scripts in stacks other than the front-most active from outputing messages */
static void _handle_debug_message(XTE *in_engine, JHDocument *in_document, char const *in_message, int in_level, int in_handled)
{
    [[JHMessageWatcher sharedController] debugMessage:in_message handlerLevel:in_level wasHandled:in_handled];
}



static void _handle_debug_vars_changed()
{
    [[JHVariableWatcher sharedController] debugContextChanged];
}



/* ??? this might be a nonsense, since technically the only reason it's here is for error message generation
 and those are supposed to be handled & localised in one place anyway */
static char* _localized_class_name(char const *in_class_name)
{
    @autoreleasepool {
        NSString *string = NSLocalizedString([NSString stringWithCString:in_class_name encoding:NSUTF8StringEncoding], @"Class name");
        return acu_callback_result_string([string UTF8String]);
    }
}




/******************
 Startup and Initalization
 */

/*
 *  _callbacks
 *  ---------------------------------------------------------------------------------------------
 *  These are the callbacks in this unit that are responsible for providing the ACU with access
 *  to the CinsImp user interface; see each callback function for specific details.
 */
/* TODO: callbacks need categorizing, commenting and tidying up */
/* TODO: callbacks that are using XTE types need to be changed to use their own ACU types */
static ACUCallbacks _callbacks = {
    (ACUFatalErrorCB)&_handle_fatal_error,
    (ACUNoOpenStackErrorCB) &_no_open_stack,
    (ACUScriptErrorCB)&_handle_script_error,
    
    (ACUStackClosedCB)&_handle_stack_closed,
    
    (ACUMessageSetCB)&_handle_message_result,
    (ACUMessageGetCB) NULL,
    
    (ACUCardRepaintCB)&_handle_view_refresh,
    
    (StackMgrCBSystemBeep)&_do_beep,
    (StackMgrCBMutateRTF)&_handle_mutate_rtf,
    
    (StackMgrCBFind)&_handle_find,
    
    //NULL,//(StackMgrCBScriptStatus)&_script_status,
    (StackMgrCBDebug)&_script_debug,
    (XTEDebugMessageCB) &_handle_debug_message,
    (ACUIsDebuggingCB) &_enter_leave_debugger,
    (ACUDebugVarsChanged) &_handle_debug_vars_changed,
    
    (ACUTimerAdjustmentCB) &_adjust_timers,
    
    (ACUSaveScreen) &_save_screen,
    (ACUReleaseScreen) &_release_screen,
    
    (ACURenderEffect) &_render_effect,
    
    (ACULayoutWillChange) &_view_layout_will_change,
    (ACULayoutDidChange) &_view_layout_did_change,
    
    (ACUAnswerChoice) &_answer_choice,
    (ACUAnswerFile) &_answer_file,
    (ACUAnswerFolder) &_answer_folder,
    (ACUAskText) &_ask_text,
    (ACUAskFile) &_ask_file,
    
    (ACULocalizedClassName) &_localized_class_name,
    
    (ACUAutoStatusControl) &_auto_status_control,
    
    (ACUBuiltinResourcesPathCB) &_handle_builtin_resource_path,
};


/*
 *  ACUGlueInit
 *  ---------------------------------------------------------------------------------------------
 *  Initalizes the ACU and configures the callback handlers from within this unit;
 *  invoked by JHCinsImp at application startup 
 */
void ACUGlueInit(void)
{
    if (!acu_init(&_callbacks))
    {
        /* if the ACU fails to initalize; display an alert and quit */
        NSBeep();
        NSAlert *alert = [NSAlert alertWithMessageText:NSLocalizedString(@"INTERNAL_ERROR", @"CinsImp application")
                                         defaultButton:NSLocalizedString(@"Quit","Quit button")
                                       alternateButton:@"" otherButton:@"" informativeTextWithFormat:@""];
        [alert runModal];
        exit(EXIT_SUCCESS);
    }
}


