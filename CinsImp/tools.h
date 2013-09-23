//
//  tools.h
//  CinsImp
//
//  Created by Joshua Hawcroft on 28/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#ifndef CinsImp_tools_h
#define CinsImp_tools_h

#define AUTH_TOOL_BROWSE        0
#define AUTH_TOOL_BUTTON        1
#define AUTH_TOOL_FIELD         2

#define PAINT_TOOL_PENCIL       3
#define PAINT_TOOL_ERASER       4
#define PAINT_TOOL_BUCKET       5
#define PAINT_TOOL_SPRAY        6
#define PAINT_TOOL_LINE         7
#define PAINT_TOOL_LASSO        8
#define PAINT_TOOL_SELECT       9
#define PAINT_TOOL_BRUSH        10
#define PAINT_TOOL_EYEDROPPER   11
#define PAINT_TOOL_RECTANGLE    12
#define PAINT_TOOL_ROUNDED_RECT 13
#define PAINT_TOOL_OVAL         14
#define PAINT_TOOL_FREESHAPE    15
#define PAINT_TOOL_FREEPOLY     16


#define TOOL_IS_PAINTING(x) ((x != AUTH_TOOL_BROWSE) && (x != AUTH_TOOL_BUTTON) && (x != AUTH_TOOL_FIELD))


#endif
