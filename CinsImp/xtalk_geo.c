//
//  xtalk_geo.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 6/09/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "xtalk_internal.h"


XTERect xte_string_to_rect(char const *in_string)
{
    XTERect result = {0, 0, 0, 0};
    char buffer[10];
    
    long len = strlen(in_string);
    long itemOffset = 0, itemLen;
    int itemNumber = 1;
    for (long i = 0; i <= len; i++)
    {
        if ((in_string[i] == ',') || (in_string[i] == 0))
        {
            itemLen = i - itemOffset;
            if ((itemLen > 0) && (itemLen < 10))
            {
                memcpy(buffer, &(in_string[itemOffset]), itemLen);
                buffer[itemLen] = 0;
                switch (itemNumber)
                {
                    case 1:
                        result.origin.x = atol(buffer);
                        break;
                    case 2:
                        result.origin.y = atol(buffer);
                        break;
                    case 3:
                        result.size.width = atol(buffer);
                        break;
                    case 4:
                        result.size.height = atol(buffer);
                        break;
                }
            }
            itemOffset = i + 1;
            itemNumber++;
            if (itemNumber > 4) break;
        }
    }
    
    return result;
}


/*
 *  xte_rect_to_string()
 *  ---------------------------------------------------------------------------------------------
 *  Public, accepts a buffer so that the result doesn't have to be memory managed by us, since
 *  we're technically going to be busy on another thread.
 *  Hint: Use XTE_GEO_CONV_BUFF_SIZE for the size of the buffer supplied.
 */
char const* xte_rect_to_string(XTERect in_rect, char *in_buffer, long in_buffer_size)
{
    sprintf(in_buffer, "%ld,%ld,%ld,%ld", in_rect.origin.x, in_rect.origin.y, in_rect.size.width, in_rect.size.height);
    return in_buffer;
}







