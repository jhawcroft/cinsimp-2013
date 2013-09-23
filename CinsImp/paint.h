/*
 
 Paint Sub-system
 paint.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Paint sub-system; editor for card and background graphic layers, including implementation of
 paint tools, filters and associated functionality
 
 *************************************************************************************************
 
 Portability
 -------------------------------------------------------------------------------------------------
 The Paint sub-system API is designed for platform independence.  The internals of the sub-system
 are written against the Foundation, Image I/O and CoreGraphics toolkits on OS X, which should
 enable eventual deployment on iOS.
 
 
 Coordinate Spaces
 -------------------------------------------------------------------------------------------------
 The sub-system expects to receive coordinates on a scale whose origin (0,0) is the top-left of
 the canvas area.  It also reports coordinates in the same way.
 
 This is in contrast to the default coordinate orientation for most higher-level OS X drawing APIs,
 which place the origin at the bottom-left of the screen.
 
 
 Brushes
 -------------------------------------------------------------------------------------------------
 Brushes for the brush tool are provided by the application, not the sub-system.  The application
 must invoke paint_brush_shape_set() at least once or attempting to use the brush tool will 
 result in invokation of Error Protocol.
 
 
 Error Protocol
 -------------------------------------------------------------------------------------------------
 If something bad happens within the sub-system, it will report an error via the PaintErrorHandler
 callback.  At that stage, it is appropriate to dispose of the sub-system and report an error to
 the user.  The sub-system will not guarantee continued functionality once an error has been
 reported and cannot be recovered.
 
 Errors that may be reported are:
 
 PAINT_ERROR_MEMORY         The sub-system has run out of memory.
 PAINT_ERROR_INTERNAL       Something unintended has happened within the sub-system.
 
 */

#ifndef JH_PAINT_H
#define JH_PAINT_H


/***********
 Types
 */

/*
 *  Paint
 *  ---------------------------------------------------------------------------------------------
 *  Opaque type representing an instance of the paint sub-system.
 */
typedef struct Paint Paint;


/* eventually assign these the same numbers as HyperCard for backwards compatibility */

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


#define PAINT_FILTER_DARKEN     1
#define PAINT_FILTER_LIGHTEN    2
#define PAINT_FILTER_INVERT     3
#define PAINT_FILTER_GREYSCALE  4


#define PAINT_NO_ERROR          0
#define PAINT_ERROR_MEMORY      1   /* the sub-system has run out of memory */
#define PAINT_ERROR_INTERNAL    2   /* something unexpected has happened within the sub-system */
#define PAINT_ERROR_MISUSE      3   /* the sub-system is being misused; check the documentation
                                       for the function(s) you are calling */
#define PAINT_ERROR_FORMAT      4   /* unrecognised image format */


/* booleans */
#define PAINT_TRUE (!0)
#define PAINT_FALSE (!!0)


/*
 *  PaintDisplayHandler
 *  ---------------------------------------------------------------------------------------------
 *  The application view should be repainted in the designated region.  If the specified
 *  coordinates are -1, the entire region should be repainted.
 */

typedef void (*PaintDisplayHandler) (Paint *in_paint, void *in_context, int in_x, int in_y, int in_width, int in_height);


/*
 *  PaintDisplayChangeHandler
 *  ---------------------------------------------------------------------------------------------
 *  Invoked when the scale and position of the display relative to the enclosing scroll view has
 *  changed; usually in response to entering or leaving Fat Bits mode.
 *
 *  Informs the application view of where the enclosing scroller should be scrolled, and how big
 *  the painted canvas is currently, in screen coordinates.
 */

typedef void (*PaintDisplayChangeHandler) (Paint *in_paint, void *in_context, int in_width, int in_height, int in_scroll_x, int in_scroll_y);


/*
 *  PaintColourChangeHandler
 *  ---------------------------------------------------------------------------------------------
 *  Invoked when the current colour has been changed by the paint sub-system.  The new colour is
 *  supplied.
 */

typedef void (*PaintColourChangeHandler) (Paint *in_paint, void *in_context, int in_red, int in_green, int in_blue);


/*
 *  PaintToolChangeHandler
 *  ---------------------------------------------------------------------------------------------
 *  Invoked when the current tool has been changed by the paint sub-system.  The new tool is
 *  supplied.
 */

typedef void (*PaintToolChangeHandler) (Paint *in_paint, void *in_context, int in_tool);


/*
 *  PaintErrorHandler
 *  ---------------------------------------------------------------------------------------------
 *  Invoked if something bad happens.  Although the Paint object will remain in a consistent
 *  state, there is no longer any guarantee the sub-system will respond to user commands.
 *
 *  The application view should dispose of the Paint sub-system instance and notify the user
 *  with an appropriate error/warning dialog.  Thus the current paint session should be ended.
 *
 *  Only one error will be reported.
 *
 *  It is not advisable to create another paint session within the application error handler.
 */

typedef void (*PaintErrorHandler) (Paint *in_paint, void *in_context, int in_error_code, char const *in_error_message);


/*
 *  PaintCallbacks
 *  ---------------------------------------------------------------------------------------------
 *  A table of callbacks that are used by the paint sub-system to communicate with the host
 *  application view.
 *
 *  (see various type descriptions above for more information on each callback)
 */

typedef struct PaintCallbacks
{
    PaintDisplayHandler display_handler;
    PaintDisplayChangeHandler display_change_handler;
    PaintColourChangeHandler colour_handler;
    PaintToolChangeHandler tool_handler;
    PaintErrorHandler error_handler;
    
} PaintCallbacks;



/***********
 Allocation, Destruction and Initial Configuration
 */

/*
 *  paint_create
 *  ---------------------------------------------------------------------------------------------
 *  Create an instance of the paint sub-system ready for painting/drawing to occur.
 *
 *  Parameters:
 *      in_context          Should be a pointer to the application view that owns this instance.
 *                          Callback functions will provide this pointer so the correct view
 *                          can respond when there are multiple stacks being edited
 *                          simultaneously.
 *
 *      in_width            Height and width of the paintable canvas (identical to that of the
 *      in_height           card).
 *
 *      in_callbacks        Used to communicate with the application view.
 *                          (see above type descriptions for more information)
 */

Paint* paint_create(int in_width, int in_height, PaintCallbacks *in_callbacks, void *in_context);


/*
 *  paint_dispose
 *  ---------------------------------------------------------------------------------------------

 */

void paint_dispose(Paint *in_paint);



/* clone these? */

/*
 *  paint_dispose
 *  ---------------------------------------------------------------------------------------------
 
 */

long paint_canvas_get_png_data(Paint *in_paint, int in_finish, void const **out_data);
long paint_selection_get_png_data(Paint *in_paint, void const **out_data);

/*
 *  paint_dispose
 *  ---------------------------------------------------------------------------------------------
 
 */

int paint_paste(Paint *in_paint, void const **in_data, long in_size);
int paint_canvas_set_png_data(Paint *in_paint, void const **in_data, long in_size);


void paint_set_white_transparent(Paint *in_paint, int in_transparent);


/***********
 Scrolling
 */

void paint_display_scroll_tell(Paint *in_paint, int in_x, int in_y, int in_width, int in_height);





/***********
 Event Handling
 */

void paint_event_mouse_down(Paint *in_paint, int in_loc_x, int in_loc_y);
void paint_event_mouse_dragged(Paint *in_paint, int in_loc_x, int in_loc_y);
void paint_event_mouse_moved(Paint *in_paint, int in_loc_x, int in_loc_y);
void paint_event_mouse_up(Paint *in_paint, int in_loc_x, int in_loc_y);


void paint_event_escape(Paint *in_paint);



/***********
 Selection
 */

int paint_loc_within_selection(Paint *in_paint, int in_loc_x, int in_loc_y);
int paint_has_selection(Paint *in_paint);
void paint_select_all(Paint *in_paint);

void paint_delete(Paint *in_paint);


/***********
 Device Drawing
 */
void paint_draw_into(Paint *in_paint, void *in_context);
void paint_draw_minature_into(Paint *in_paint, void  *in_context, int in_width, int in_height);


/***********
 State Accessor/Mutation
 */

int paint_current_tool_get(Paint *in_paint);
void paint_current_tool_set(Paint *in_paint, int in_tool);

void paint_fat_bits_set(Paint *in_paint, int in_fat_bits);
int paint_fat_bits_get(Paint *in_paint);

void paint_current_colour_set(Paint *in_paint, int in_red, int in_green, int in_blue);

void paint_draw_filled_set(Paint *in_paint, int in_draw_filled);
int paint_draw_filled_get(Paint *in_paint);

void paint_line_size_set(Paint *in_paint, int in_line_size);

void paint_brush_shape_set(Paint *in_paint, void *in_data, long in_size);


/***********
 Filters
 */

void paint_apply_filter(Paint *in_paint, int in_filter);



/***********
 Transformation
 */

void paint_flip_horizontal(Paint *in_paint);
void paint_flip_vertical(Paint *in_paint);




#endif
