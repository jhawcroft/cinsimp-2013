/*
 
 About Panel Controller
 JHAbout.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHAbout.h"


@implementation JHAbout


- (void)windowDidLoad
{
    [the_version setStringValue:[NSString stringWithFormat:@"%@ [%@]",
                                 [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"],
                                 [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"]
    ]];
    [[self window] setLevel:NSFloatingWindowLevel];
}


- (IBAction)about:(id)sender
{
    [self.window makeKeyAndOrderFront:self];
}


- (IBAction)openAcknowledgements:(id)sender
{
    NSURL *rtfUrl = [[NSBundle mainBundle] URLForResource:@"Acknowledgements" withExtension:@"rtf"];
    [[NSWorkspace sharedWorkspace] openURL:rtfUrl];
}


@end
