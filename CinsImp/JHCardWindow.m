/*
 
 Card Window
 JHCardWindow.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 (see header for module description)
 
 */

#import "JHCardWindow.h"
#import "JHIconManager.h"

#import "JHCinsImp.h"
#import "jhglobals.h"



@implementation JHCardWindow


/* included here for debugging to ensure document closure is not leaking; not required in production */
/* - (void)dealloc
{
    //NSLog(@"cardwindow dealloc");
} */
/**********
 Initialization
 */

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        _icon_manager = nil;
    }
    return self;
}




/**********
 Accessors
 */

- (JHCardView*)cardView
{
    return cardView;
}



/**********
 UI Handlers
 */

- (IBAction)protectStack:(id)sender
{
    protect_stack = [[JHProtectStack alloc] initWithWindowNibName:@"JHProtectStack"];
    [protect_stack setStackAndShowProperties:[cardView stack] forWindow:self];
}


/**********
 Access Protected Stacks
 */

/* invoked by the guess password sheet after the user keys their guess */
- (void)disableSecurity
{
    if ([protect_stack isPasswordGuessCorrect])
        [cardView disableSecurity];
    else
        [self close];
}


/* invoked after loading if the stack is access protected */
- (IBAction)checkPassword:(id)sender
{
    protect_stack = [[JHProtectStack alloc] initWithWindowNibName:@"JHProtectStack"];
    [protect_stack guessPasswordForStack:[cardView stack] forWindow:self withDelegate:self selector:@selector(disableSecurity)];
}


/* invoked by the timer that decouples loading from access protection */
- (void)doCheckPassword:(NSTimer*)in_timer
{
    [in_timer invalidate];
    [self checkPassword:self];
}



/**********
 Activation
 */

/*
- (void)setXTalk:(XTE*)in_xtalk
{
    xtalk = in_xtalk;
}
*/

- (void)becomeKeyWindow
{
    [super becomeKeyWindow];
    
    /* record the current xTalk engine */
    //gXTalk = stackmgr_stack_xtalk([cardView stack]);
    
    
    /* check the locked status of the stack */
    [cardView checkLocked];
    
    /* send notifications to update the menus and palettes in case anything has changed */
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:@"DesignSelectionChange" object:cardView]];
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:designLayerChangeNotification object:cardView]];
    
    /* if the stack is access protected; kick off the What's the Password? sheet */
    if ([cardView securityEnabled])
    {
        if (![protect_stack checked_password])
            [NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(doCheckPassword:) userInfo:nil repeats:NO];
    }
}


- (void)becomeMainWindow
{
    [super becomeMainWindow];
    
    /* track which card view is current */
    gCurrentCardView = cardView;
    
    
    
    /* check the locked status of the stack */
    [cardView checkLocked];
    
    /* send notifications to update the menus and palettes in case anything has changed */
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:designLayerChangeNotification object:cardView]];
}


- (void)resignMainWindow
{
    gCurrentCardView = nil;
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:designLayerChangeNotification object:nil]];
    
    [super resignMainWindow];
}


/**********
 Safe Cleanup
 */
- (void)close
{
    /* we must do this manually until stack handles are always real handles - at the moment
     they're Stack unit ptrs posing as handles */
    if (_icon_manager) [[_icon_manager window] close];
    _icon_manager = nil;
    
    [super close];
}



/**********
 Stack-wide Menu Commands
 */

- (IBAction)manageIcons:(id)sender
{
    if (_icon_manager == nil) _icon_manager = [[JHIconManagerController alloc] initWithStack:[cardView stack] parentWindow:self];
    [[_icon_manager window] makeKeyAndOrderFront:self];
}


- (IBAction)setIcon:(id)sender
{
    if (_icon_manager == nil) _icon_manager = [[JHIconManagerController alloc] initWithStack:[cardView stack] parentWindow:self];
    [(JHIconManager*)[_icon_manager window] chooseIcon];
}





@end
