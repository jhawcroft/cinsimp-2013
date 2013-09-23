//
//  JHVariableWatcher.h
//  DevelopDebugWindows
//
//  Created by Joshua Hawcroft on 16/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface JHVariableWatcher : NSWindowController <NSTableViewDataSource, NSTableViewDelegate, NSWindowDelegate>
{
    IBOutlet NSTextField *_handler_name;
    IBOutlet NSTableView *_var_table;
    
    NSTableColumn *_col_name;
    NSTableColumn *_col_value;
    
    IBOutlet NSTextView *_value_editor;
    
    int _editing_index;
    NSString *_editing_name;
}

+ (JHVariableWatcher*)sharedController;

- (IBAction)toggleVariableWatcher:(id)sender;

- (void)debugContextChanged;

- (void)finishEdit;
- (void)saveEdit:(NSString*)in_new_text;


@end
