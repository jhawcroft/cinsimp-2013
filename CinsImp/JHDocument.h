/*
 
 Stack Document
 JHDocument.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Represents the open Stack document; controls loading and interfacing between window, file and
 other parts of the application
 
 */

#import <Cocoa/Cocoa.h>

#import "JHCardWindow.h"
#import "JHMessagePalette.h"
#import "jhglobals.h"

#import "JHScriptEditorController.h"

#import "JHAnswerDialog.h"
#import "JHAskDialog.h"

#include "stack.h"
#include "xtalk_engine.h"


extern NSString *lastDocumentClosedNotification;


@interface JHDocument : NSDocument <NSWindowDelegate>
{
    Stack *stack;
    StackHandle _stack;
    IBOutlet JHCardWindow *the_window;
   // XTE *xtalk;
    
    NSMutableArray *script_editors;
    
    //IBOutlet NSWindow *_progress_sheet;
    //IBOutlet NSTextField *_progress_message;
    //IBOutlet NSButton *_progress_abort;
    //IBOutlet NSProgressIndicator *_progress_indicator;
    
    //BOOL script_status_should_show;
    
    id _last_answer_dlog;
}


- (Stack*)stack;
- (JHCardWindow*)window;
- (JHCardView*)view;




- (IBAction)abortRunningScript:(id)sender;
//- (void)setScriptStatusVisible:(BOOL)in_visible message:(NSString*)in_message abortText:(NSString*)in_abort_text abortable:(BOOL)in_abortable;


- (void)showAnswerDialogWithMessage:(NSString*)in_message button1:(NSString*)in_button1 button2:(NSString*)in_button2 button3:(NSString*)button3;
- (void)showAnswerFileDialogWithMessage:(NSString*)in_message type1:(NSString*)in_type1 type2:(NSString*)in_type2 type3:(NSString*)in_type3;
- (void)showAnswerFolderDialogWithMessage:(NSString*)in_message;
- (void)showAskDialogWithMessage:(NSString*)in_message response:(NSString*)in_response forPassword:(BOOL)in_password_mode;
- (void)showAskFileDialogWithMessage:(NSString*)in_message defaultFilename:(NSString*)in_default_filename;


- (IBAction)devTest:(id)sender;

@end
