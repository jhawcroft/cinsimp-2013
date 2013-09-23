/*
 
 Card Size View Controller
 JHCardSize.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHCardSize.h"

/*********
 Configuration
 */

/* pixels before drag-resize snaps to a preset */
#define PRESET_TOLERANCE 10




@implementation JHCardSize


- (void)registerPreset:(NSString*)in_title size:(NSSize)in_size
{
    [preset_titles addObject:in_title];
    [preset_sizes addObject:NSStringFromSize(in_size)];
}


- (void)loadPresetSizes
{
    [self registerPreset:@"Small" size:NSMakeSize(350, 250)];
    [self registerPreset:@"Classic" size:NSMakeSize(512, 342)];
    [self registerPreset:@"Standard" size:NSMakeSize(800, 600)];
    [self registerPreset:@"MacBook" size:NSMakeSize(1280, 800)];
    //[self registerPreset:@"iMac" size:NSMakeSize(1280, 800)];
    //[self registerPreset:@"Thunderbolt" size:NSMakeSize(1280, 800)];
}


- (void)createPresetMenu
{
    [presetMenu removeAllItems];
    for (int i = 0; i < [preset_titles count]; i++)
    {
        NSMenuItem *preset_item = [presetMenu addItemWithTitle:[preset_titles objectAtIndex:i] action:nil keyEquivalent:@""];
        [preset_item setTag:i];
    }
    [presetMenu addItem:[NSMenuItem separatorItem]];
    [[presetMenu addItemWithTitle:@"Screen" action:nil keyEquivalent:@""] setTag:-1];
    customMenuItem = [presetMenu addItemWithTitle:@"Custom" action:nil keyEquivalent:@""];
    [customMenuItem setTag:-2];
}


- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
    {
        self.delegate = nil;
        the_card_size = NSMakeSize(512, 342);
        the_last_custom_size = the_card_size;
        preset_sizes = [NSMutableArray array];
        preset_titles = [NSMutableArray array];
        [self loadPresetSizes];
    }
    return self;
}


- (void)awakeFromNib
{
    [self createPresetMenu];
    [presetButton selectItemWithTitle:@"Standard"];
    [self presetSelected:self];
}


- (NSSize)card_size
{
    return the_card_size;
}


- (void)set_card_size:(NSSize)the_size
{
    the_card_size = the_size;
    [cardSizeView set_card_size:the_size];
    int preset_index = [self indexOfPresetForSize:the_size];
    if (preset_index >= 0)
    {
        the_size = NSSizeFromString([preset_sizes objectAtIndex:preset_index]);
        [presetButton selectItemWithTitle:[preset_titles objectAtIndex:preset_index]];
    }
    else
    {
        [presetButton selectItemWithTag:-2];
        the_last_custom_size = the_size;
    }
    [dimensions setStringValue:[NSString stringWithFormat:@"%d x %d", (int)the_card_size.width, (int)the_card_size.height]];
}


- (int)indexOfPresetForSize:(NSSize)in_size
{
    for (int i = 0; i < [preset_sizes count]; i++)
    {
        NSSize sz = NSSizeFromString([preset_sizes objectAtIndex:i]);
        if ( (fabs(in_size.width - sz.width) <= PRESET_TOLERANCE)
            && (fabs(in_size.height - sz.height) <= PRESET_TOLERANCE) )
            return i;
    }
    return -1;
}


- (void)cardSizeView:(JHCardSizeView *)the_view changed:(NSSize)the_size finished:(BOOL)is_finished
{
    int preset_index = [self indexOfPresetForSize:the_size];
    if (preset_index >= 0)
    {
        the_size = NSSizeFromString([preset_sizes objectAtIndex:preset_index]);
        [presetButton selectItemWithTitle:[preset_titles objectAtIndex:preset_index]];
    }
    else
    {
        [presetButton selectItemWithTag:-2];
        the_last_custom_size = the_size;
    }
    the_card_size = the_size;
    [dimensions setStringValue:[NSString stringWithFormat:@"%d x %d", (int)the_card_size.width, (int)the_card_size.height]];
    
    if (self.delegate && is_finished) [self.delegate cardSize:self changed:the_card_size];
}


- (IBAction)presetSelected:(id)sender
{
    NSInteger preset_id = [[presetButton selectedItem] tag];
    if (preset_id == -1)
        the_card_size = [[NSScreen mainScreen] frame].size;
    else if (preset_id == -2)
        the_card_size = the_last_custom_size;
    else
        the_card_size = NSSizeFromString([preset_sizes objectAtIndex:preset_id]);
    [dimensions setStringValue:[NSString stringWithFormat:@"%d x %d", (int)the_card_size.width, (int)the_card_size.height]];
    [cardSizeView set_card_size:the_card_size];
    
    if (self.delegate) [self.delegate cardSize:self changed:the_card_size];
}


@end
