//
//  xtalk_posix.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 15/08/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include <unistd.h>


void _xte_platform_suspend_thread(int in_microseconds)
{
    usleep(in_microseconds);
}



