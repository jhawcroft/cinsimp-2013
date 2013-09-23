//
//  JHVariableWatcher.m
//  DevelopDebugWindows
//
//  Created by Joshua Hawcroft on 16/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#import "JHVariableWatcher.h"

#include "acu.h"


static JHVariableWatcher *_shared = nil;


@interface JHVariableWatcher ()

- (IBAction)_variableContextChanged:(id)sender;

- (void)_updateVariableListFlushingCache:(BOOL)in_flush;
- (void)_editVariableValue;

@end

@implementation JHVariableWatcher


- (id)initWithWindow:(NSWindow *)window
{
    if (_shared) return _shared;
    
    self = [super initWithWindow:window];
    if (self) {
        _shared = self;
    }
    
    return self;
}


- (NSString*)windowNibName
{
    return @"JHVariableWatcher";
}


- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}


+ (JHVariableWatcher*)sharedController
{
    if (!_shared) _shared = [[JHVariableWatcher alloc] init];
    return _shared;
}


- (IBAction)toggleVariableWatcher:(id)sender
{
    [self loadWindow];
    if ([[self window] isVisible])
        [[self window] orderOut:self];
    else
        [[self window] makeKeyAndOrderFront:self];
}


- (void)awakeFromNib
{
    while([[_var_table tableColumns] count] > 0) {
        [_var_table removeTableColumn:[[_var_table tableColumns] lastObject]];
    }
    
    _col_name = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    [_col_name setWidth:100];
    
    _col_value = [[NSTableColumn alloc] initWithIdentifier:@"value"];
    [_col_value setWidth:150];
    
    [_var_table addTableColumn:_col_name];
    [_var_table addTableColumn:_col_value];
    
    [_var_table setHeaderView:nil];
    
    [_value_editor setTextContainerInset:NSMakeSize(0, 4)];
    
    [self finishEdit];
    [self debugContextChanged];
}


- (void)debugContextChanged
{
    acu_debug_begin_inspection();
    
    char const *_context_name = acu_debug_context();
    if (_context_name == NULL)
        [_handler_name setStringValue:@"Global Variables"];
    else
        [_handler_name setStringValue:[NSString stringWithCString:_context_name encoding:NSUTF8StringEncoding]];
    
    acu_debug_end_inspection();
    
    [_var_table reloadData];
    
    /*
    long selection = [_context_popup indexOfSelectedItem];
    long original_selection = selection;
    
    if (original_selection != 0) [self finishEdit];
    
    char const* const* context_list = stackmgr_debug_var_contexts();
    
    [_context_popup removeAllItems];
    if (context_list == NULL)
    {
        [_context_popup addItemWithTitle:@"No open stack"];
        [_context_popup setEnabled:NO];
        [self _updateVariableListFlushingCache:YES];
        return;
    }
    [_context_popup setEnabled:YES];
    
    [_context_popup addItemWithTitle:NSLocalizedString(@"Globals", @"variable watcher popup menu")];
    
    if (*context_list)
        [[_context_popup menu] addItem:[NSMenuItem separatorItem]];
    
    
    while (*context_list)
    {
        [_context_popup addItemWithTitle:[NSString stringWithCString:*context_list encoding:NSUTF8StringEncoding]];
        context_list++;
    }
    
    if (selection != 0)
    {
        long count = [[_context_popup itemArray] count];
        if (count > 2) selection = count-1;
        else selection = 0;
    }
    [_context_popup selectItemAtIndex:selection];
    
    if (original_selection != 0)
    {
        [_var_table deselectAll:self];
    }
    
    [self _updateVariableListFlushingCache:(selection != original_selection)];*/
}


- (void)_updateVariableListFlushingCache:(BOOL)in_flush
{
    /*
    int new_context = (int)( [_context_popup indexOfSelectedItem] - 2 );
    if (new_context < 0) new_context = -1;
    
    _current_context = new_context;
    
    [_var_table reloadData]; // could be made more efficient, by asking for the changes ***
    
    */
}


- (void)_editVariableValue
{
    int var_index = (int)[_var_table selectedRow];
    
    if (var_index < 0)
    {
        [self finishEdit];
        return;
    }
    
    NSString *string = [self tableView:_var_table objectValueForTableColumn:_col_value row:var_index];
    [_value_editor setString:string];
    [_value_editor setEditable:YES];
    [_value_editor setSelectable:YES];
    
    //_editing_context = _current_context;
    _editing_index = var_index;
    _editing_name = [[self tableView:_var_table objectValueForTableColumn:_col_name row:var_index] copy];
}


- (IBAction)_variableContextChanged:(id)sender
{
    [self finishEdit];
    [self _updateVariableListFlushingCache:YES];
}


- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return acu_debug_variable_count();
}


- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
    char const *c_text;
    if (aTableColumn == _col_name)
        c_text = acu_debug_variable_name((int)rowIndex);
    else
        c_text = acu_debug_variable_value((int)rowIndex);
    if (!c_text) c_text = "";
    return [NSString stringWithCString:c_text encoding:NSUTF8StringEncoding];
}


- (BOOL)tableView:(NSTableView *)tableView shouldEditTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    return NO;
}


- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row
{
    [self finishEdit];
    return YES;
}


- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    [self _editVariableValue];
}


- (void)windowDidResignKey:(NSNotification *)notification
{
    [self finishEdit];
}


- (void)finishEdit
{
    [[self window] makeFirstResponder:_var_table];
    
    [_value_editor setString:@""];
    [_value_editor setEditable:NO];
    [_value_editor setSelectable:NO];
    
    //_editing_context = -1;
    _editing_index = -1;
    _editing_name = nil;
}


- (void)saveEdit:(NSString*)in_new_text
{
    acu_debug_set_variable(_editing_index, [_editing_name UTF8String], [in_new_text UTF8String]);
    //stackmgr_debug_var_set(_editing_context, _editing_index, [_editing_name UTF8String], [in_new_text UTF8String]);
}


@end
