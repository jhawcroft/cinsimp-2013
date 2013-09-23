//
//  JHScriptEditorController.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 6/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "JHxTalkTextView.h"

#include "acu.h"


extern NSString *notifyScriptEditorDidClose;


@interface JHScriptEditorController : NSWindowController <NSWindowDelegate>
{
    StackHandle edited_object;
    IBOutlet JHxTalkTextView *xtalk_view;
    BOOL _has_closed;
}

- (id)initWithStackHandle:(StackHandle)in_handle;
- (StackHandle)handle;
- (void)setSelectedLine:(long)in_source_line debugging:(BOOL)in_debugging;

- (IBAction)debugContinue:(id)sender;
- (IBAction)debugAbort:(id)sender;

@end
