/*
 
 Paint
 paint_bucket.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Bucket tool (flood fill)
 
 *************************************************************************************************
 */

#include "paint_int.h"



static int bytesPerRow = 0;
static int wt = 0;
static int ht = 0;
static unsigned char *data = NULL;
static long dataSize = 0;


static int buffer_cmp(int x, int y, int r, int g, int b, int a)
{
    int point_index = (bytesPerRow * (ht - y)) + (x * 4);
    if ((point_index < 0) || (point_index >= dataSize))
    {
        return 0; // prevent out-of-bounds access
    }
    return ((data[point_index+1] == r) && (data[point_index+2] == g) && (data[point_index+3] == b) && (data[point_index] == a));
}


static void buffer_get(int x, int y, int *r, int *g, int *b, int *a)
{
    int point_index = (bytesPerRow * (ht - y)) + (x * 4);
    if ((point_index < 0) || (point_index >= dataSize))
    {
        return; // prevent out-of-bounds access
    }
    *a = data[point_index];
    *r = data[point_index+1];
    *g = data[point_index+2];
    *b = data[point_index+3];
}


static void buffer_set(int x, int y, int r, int g, int b, int a)
{
    int point_index = (bytesPerRow * (ht - y)) + (x * 4);
    if ((point_index < 0) || (point_index >= dataSize))
    {
        return; // prevent out-of-bounds access
    }
    data[point_index] = a;
    data[point_index+1] = r;
    data[point_index+2] = g;
    data[point_index+3] = b;
}


static void buffer_blend(int x, int y, int r, int g, int b, int a)
{
    int point_index = (bytesPerRow * (ht - y)) + (x * 4);
    if ((point_index < 0) || (point_index >= dataSize))
    {
        return; // prevent out-of-bounds access
    }
    data[point_index] = (a + data[point_index]) / 2;
    data[point_index+1] = (r + data[point_index+1]) / 2;
    data[point_index+2] = (g + data[point_index+2]) / 2;
    data[point_index+3] = (b + data[point_index+3]) / 2;
}


void _paint_flood_fill(Paint *in_paint, CGPoint in_point)
{
    // adjust for the cursor hotspot being wrong in the prototype
    in_point.x += 12;
    
    // save some values for use by our c-function implementations for speed;
    // can probably do this just as efficiently with a context later on...  ***
    bytesPerRow = (int)CGBitmapContextGetBytesPerRow(in_paint->context_primary);
    wt = in_paint->width;
    ht = in_paint->height;
    data = in_paint->bitmap_data_primary;
    dataSize = in_paint->bitmap_data_primary_size;
    
    // setup a stack;
    // ideally this would be allocated on the heap, not on our function stack!  ***
    const int kStackLimit = 1500000;
    int stack[kStackLimit];
    int stack_ptr = 0;
    
    // setup some other locals
    int x,y;
    int oldr, oldg, oldb, olda;
    BOOL spanLeft, spanRight;
    
    /* need to calculate these using CoreGraphics dummy area so we get the same values **** TODO ****
     possibly use device color functions? - probably due to calibration/rounding/etc. */
    int fillRed = in_paint->red * 255.0;
    int fillGreen = in_paint->green * 255.0;
    int fillBlue = in_paint->blue * 255.0;
    
    //printf("Starting R G B = %d, %d, %d\n", fillRed, fillGreen, fillBlue);
    
    // save colour of starting point for later comparison
    buffer_get(in_point.x, in_point.y, &oldr, &oldg, &oldb, &olda);
    
    //printf("Existing R G B = %d, %d, %d\n", oldr, oldg, oldb);
    
    // abort if the starting point is the same colour as the fill colour
    if (buffer_cmp(in_point.x, in_point.y, fillRed, fillGreen, fillBlue, 255))
        return;
    
    // put the starting point on to a stack
    stack[stack_ptr++] = in_point.x;
    stack[stack_ptr++] = in_point.y;
    
    // iterate over the stack of points until it's empty
    while (stack_ptr >= 0)
    {
        y = stack[--stack_ptr];
        x = stack[--stack_ptr]; // pop
        
        // scan upward until we find a different color to the start point
        int y1 = y;
        while (y1 >= 0 && buffer_cmp(x, y1, oldr, oldg, oldb, olda)) y1--;
        y1++;
        
        // 1-pixel over-fill upward
        buffer_blend(x, y1-1, fillRed, fillGreen, fillBlue, 255);
        buffer_blend(x, y1-2, fillRed, fillGreen, fillBlue, 255);
        
        // scan down from the highest point of the same colour,
        // stopping at each point to do some checks
        // and finishing when we run into a different colour from the start point
        spanLeft = spanRight = NO;
        while(y1 <= ht && buffer_cmp(x, y1, oldr, oldg, oldb, olda) )
        {
            // colour in the current scan point in the fill colour
            buffer_set(x, y1, fillRed, fillGreen, fillBlue, 255);
            
            if(!spanLeft && x > 0 && buffer_cmp(x-1, y1, oldr, oldg, oldb, olda))
            {
                // if the point to the left is the old colour
                // push the point on to the stack
                if (stack_ptr > kStackLimit-4) return; // push
                stack[stack_ptr++] = x-1;
                stack[stack_ptr++] = y1;
                
                // don't consider the left vertical again unless we go past another colour change,
                // otherwise we'd end up adding a whole heap of redundant points to the stack
                spanLeft = 1;
            }
            else if(spanLeft && x > 0 && (!buffer_cmp(x-1, y1, oldr, oldg, oldb, olda)) )
            {
                // we went past a point on the left that is already filled with some other colour
                // and we can thus consider points further down on the left again
                spanLeft = 0;
                
                // 1-pixel over-fill left
                buffer_blend(x-1, y1, fillRed, fillGreen, fillBlue, 255);
                buffer_blend(x-2, y1, fillRed, fillGreen, fillBlue, 255);
            }
            if(!spanRight && x < wt - 1 && buffer_cmp(x+1, y1, oldr, oldg, oldb, olda))
            {
                // if the point to the left is the old colour
                // push the point on to the stack
                if (stack_ptr > kStackLimit-4) return; // push
                stack[stack_ptr++] = x+1;
                stack[stack_ptr++] = y1;
                
                // don't consider the right vertical again unless we go past another colour change,
                // otherwise we'd end up adding a whole heap of redundant points to the stack
                spanRight = 1;
            }
            else if(spanRight && x < wt - 1 && (!buffer_cmp(x+1, y1, oldr, oldg, oldb, olda)))
            {
                // we went past a point on the left that is already filled with some other colour
                // and we can thus consider points further down on the right again
                spanRight = 0;
                
                // 1-pixel over-fill right
                buffer_blend(x+1, y1, fillRed, fillGreen, fillBlue, 255);
                buffer_blend(x+2, y1, fillRed, fillGreen, fillBlue, 255);
            }
            
            // move to the next row down
            y1++;
        }
        
        // 1-pixel over-fill downward
        buffer_blend(x, y1, fillRed, fillGreen, fillBlue, 255);
        buffer_blend(x, y1+1, fillRed, fillGreen, fillBlue, 255);
    }
}


