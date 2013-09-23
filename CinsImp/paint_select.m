/*
 
 Paint
 paint_select.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Rectangular and lasso selection tools; associated selection management utilities
 
 *************************************************************************************************
 */

#include "paint_int.h"


/***********
 Selection Utilities
 */

/*
 *  _paint_grab_selection
 *  ---------------------------------------------------------------------------------------------
 *  Transfers the paint outlined by the selection from the canvas to a temporary bitmap so it
 *  may be moved around, etc.
 */

static void _paint_grab_selection(Paint *in_paint)
{
    assert(in_paint != NULL);
    assert(in_paint->selection_path != NULL);
    
    /* get selection bounds and prepare for drag */
    in_paint->selection_bounds = CGPathGetBoundingBox(in_paint->selection_path);
    
    /* keep bounds within the canvas to prevent glitches */
    in_paint->selection_bounds = CGRectIntersection(in_paint->selection_bounds,
                                                    CGRectMake(0, 0, in_paint->width, in_paint->height));
    
    /* prepare state information for mouse/touch dragging (movement)
     of the selected paint */
    in_paint->selection_bounds_moved = in_paint->selection_bounds;
    if (in_paint->selection_path_moved != NULL) CGPathRelease(in_paint->selection_path_moved);
    in_paint->selection_path_moved = CGPathCreateCopy(in_paint->selection_path);
    if (in_paint->selection_path_moved == NULL)
    {
        CGPathRelease(in_paint->selection_path);
        in_paint->selection_path = NULL;
        return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    }
    
    /* create an offscreen bitmap and context for the selection */
    if (in_paint->context_selection != NULL)
        _paint_dispose_context(in_paint->context_selection, in_paint->bitmap_data_selection);
    in_paint->context_selection = _paint_create_context(in_paint->selection_bounds.size.width,
                                                        in_paint->selection_bounds.size.height,
                                                        (void**)&(in_paint->bitmap_data_selection),
                                                        &(in_paint->bitmap_data_selection_size), PAINT_FALSE);
    if (in_paint->context_selection == NULL)
    {
        CGPathRelease(in_paint->selection_path);
        in_paint->selection_path = NULL;
        return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    }
    CGContextClearRect(in_paint->context_selection, CGRectMake(0, 0,
                                                               in_paint->selection_bounds.size.width,
                                                               in_paint->selection_bounds.size.height));
    
    /* grab the primary paint */
    CGImageRef canvas_image = CGBitmapContextCreateImage(in_paint->context_primary);
    if (canvas_image == NULL)
    {
        CGPathRelease(in_paint->selection_path);
        in_paint->selection_path = NULL;
        return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    }
    
    /* copy the selected paint into the offscreen bitmap */
    CGAffineTransform translation = CGAffineTransformMakeTranslation(-in_paint->selection_bounds.origin.x,
                                                                     -in_paint->selection_bounds.origin.y);
    CGPathRef clipping_path = CGPathCreateCopyByTransformingPath(in_paint->selection_path, &translation);
    if (clipping_path == NULL)
    {
        CGPathRelease(in_paint->selection_path);
        in_paint->selection_path = NULL;
        CGImageRelease(canvas_image);
        return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    }
    CGContextSetAllowsAntialiasing(in_paint->context_selection, PAINT_FALSE);
    CGContextSaveGState(in_paint->context_selection);
    CGContextBeginPath(in_paint->context_selection);
    CGContextAddPath(in_paint->context_selection, clipping_path);
    CGContextClip(in_paint->context_selection);
    CGContextDrawImage(in_paint->context_selection, CGRectMake(-in_paint->selection_bounds.origin.x,
                                                               -in_paint->selection_bounds.origin.y,
                                                               in_paint->width,
                                                               in_paint->height), canvas_image);
    CGContextRestoreGState(in_paint->context_selection);
    
    /* cleanup */
    CGImageRelease(canvas_image);
    CGPathRelease(clipping_path);
    
    /* clear paint underneath the selection */
    CGContextSaveGState(in_paint->context_primary);
    CGContextSetShouldAntialias(in_paint->context_primary, PAINT_FALSE);
    
    CGContextBeginPath(in_paint->context_primary);
    CGContextAddPath(in_paint->context_primary, in_paint->selection_path);
    CGContextClip(in_paint->context_primary);
    CGContextClearRect(in_paint->context_primary, in_paint->selection_bounds);
    
    CGContextRestoreGState(in_paint->context_primary);
}


/*
 *  _paint_dispose_selection
 *  ---------------------------------------------------------------------------------------------
 *  Completely disposes of the selection, including it's content.  Does not update the display.
 *
 *  !  To deselect the selected paint, call _paint_drop_selection() instead.
 *
 *  If there is no selection, does nothing.  Can therefore be called to ensure that there is no
 *  selection from elsewhere in the paint sub-system.
 *
 *  This function will never invoke an error condition and can be relied upon to safely dispose
 *  of the current selection in the event of an error occurring elsewhere the selection unit.
 */

void _paint_dispose_selection(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    if (in_paint->selection_path == NULL) return;
    
    CGPathRelease(in_paint->selection_path);
    in_paint->selection_path = NULL;
    if (in_paint->selection_path_moved) CGPathRelease(in_paint->selection_path_moved);
    in_paint->selection_path_moved = NULL;
    if (in_paint->scaled_selection) CGPathRelease(in_paint->scaled_selection);
    in_paint->scaled_selection = NULL;
    
    if (in_paint->context_selection)
    {
        _paint_dispose_context(in_paint->context_selection, in_paint->bitmap_data_selection);
        in_paint->context_selection = NULL;
        in_paint->bitmap_data_selection = NULL;
    }
}


/*
 *  _paint_drop_selection
 *  ---------------------------------------------------------------------------------------------
 *  Deselects the selected paint; drops it on the canvas at the current selection location.
 *  Does not update the display.
 *
 *  !  To completely dispose of (delete) the selected paint, call _paint_dispose_selection().
 *
 *  If there is no selection, does nothing.  Can therefore be called to ensure that there is no
 *  selection from elsewhere in the paint sub-system.  May cause an out of memory error, but is
 *  guaranteed to dispose of the selection regardless.
 */

void _paint_drop_selection(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    if (in_paint->selection_path == NULL) return;
    
    assert(in_paint->context_selection != NULL);
    
    CGImageRef selection_image = CGBitmapContextCreateImage(in_paint->context_selection);
    if (selection_image == NULL)
    {
        _paint_dispose_selection(in_paint);
        return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    }
    
    CGContextSaveGState(in_paint->context_primary);
    CGContextSetShouldAntialias(in_paint->context_primary, PAINT_FALSE);
    CGContextSetBlendMode(in_paint->context_primary, kCGBlendModeNormal);
    CGContextDrawImage(in_paint->context_primary, in_paint->selection_bounds_moved, selection_image);
    CGContextRestoreGState(in_paint->context_primary);
    
    CGImageRelease(selection_image);
    _paint_dispose_selection(in_paint);
}


/*
 *  _paint_select_area
 *  ---------------------------------------------------------------------------------------------
 *  Selects the specified area of the canvas.  Convenience for other units of the sub-system.
 */

void _paint_select_rect_area(Paint *in_paint, int in_x, int in_y, int in_width, int in_height)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_x) && IS_COORD(in_y) && IS_COORD(in_width) && IS_COORD(in_height));
    
    /* drop any existing selection */
    _paint_drop_selection(in_paint);
    
    in_paint->selection_is_rect = PAINT_TRUE;
    
    /* define a new selection */
    in_paint->selection_path = CGPathCreateMutable();
    if (in_paint->selection_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    CGPathAddRect(in_paint->selection_path, NULL, CGRectMake(in_x, in_y, in_width, in_height));
    
    /* grab the selection */
    _paint_grab_selection(in_paint);
}


/*
 *  _paint_selection_create_with_cgimage
 *  ---------------------------------------------------------------------------------------------
 *  Selects the specified area of the canvas.  Intended for use by pasteboard related functions.
 */

void _paint_selection_create_with_cgimage(Paint *in_paint, CGImageRef in_cgimage)
{
    assert(in_paint != NULL);
    assert(in_cgimage != NULL);
    
    /* drop any existing selection */
    _paint_drop_selection(in_paint);
    
    in_paint->selection_is_rect = PAINT_TRUE;
    
    /* get the image dimensions */
    CGRect image_rect = CGRectMake(0, 0, (int)CGImageGetWidth(in_cgimage), (int)CGImageGetHeight(in_cgimage));
    CGRect image_centred = image_rect;
    image_centred.origin.x += (int)((in_paint->display_width - image_rect.size.width) / 2 + in_paint->display_scroll_x);
    image_centred.origin.y += (int)((in_paint->display_height - image_rect.size.height) / 2 + in_paint->display_scroll_y);
    
    /* create a context and bitmap to hold the selection */
    in_paint->context_selection = _paint_create_context(image_rect.size.width,
                                                        image_rect.size.height,
                                                        (void**)&(in_paint->bitmap_data_selection),
                                                        &(in_paint->bitmap_data_selection_size), PAINT_FALSE);
    if (in_paint->context_selection == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    CGContextClearRect(in_paint->context_selection, image_rect);
    
    /* copy the image to the selection */
    CGContextDrawImage(in_paint->context_selection, image_rect, in_cgimage);
    
    /* create a rectangular path to enclose the selection */
    in_paint->selection_path = CGPathCreateMutable();
    if (in_paint->selection_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    CGPathAddRect(in_paint->selection_path, NULL, image_centred);
    in_paint->selection_bounds = CGPathGetBoundingBox(in_paint->selection_path);
    in_paint->selection_bounds_moved = in_paint->selection_bounds;
    
    /* prepare the selection for movement, etc. */
    in_paint->selection_path_moved = CGPathCreateCopy(in_paint->selection_path);
    if (in_paint->selection_path_moved == NULL)
    {
        CGPathRelease(in_paint->selection_path);
        in_paint->selection_path = NULL;
        return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    }
    
    /* change the tool to rectangular selection */
    _paint_change_tool(in_paint, PAINT_TOOL_SELECT, PAINT_FALSE);
}


/*
 *  _paint_move_selection
 *  ---------------------------------------------------------------------------------------------
 *  Moves the selection based on the delta between the last mouse/touch location and the present.
 *
 *  If in_finish is PAINT_TRUE, the selection state information is updated to enable future moves
 *  from the new location.
 */

static void _paint_move_selection(Paint *in_paint, int in_finish)
{
    assert(in_paint != NULL);
    assert(IS_BOOL(in_finish));
    assert(in_paint->selection_path != NULL);
    assert(IS_COORD(in_paint->starting_point.x) && IS_COORD(in_paint->starting_point.y));
    assert(IS_COORD(in_paint->ending_point.x) && IS_COORD(in_paint->ending_point.y));
    
    /* calculate the deltas */
    int delta_x, delta_y;
    delta_x = in_paint->ending_point.x - in_paint->starting_point.x;
    delta_y = in_paint->ending_point.y - in_paint->starting_point.y;
    
    /* adjust the selection tracking origin */
    in_paint->selection_bounds_moved.origin.x = in_paint->selection_bounds.origin.x + delta_x;
    in_paint->selection_bounds_moved.origin.y = in_paint->selection_bounds.origin.y + delta_y;
    
    /* move the selection 'marching ants' path */
    CGAffineTransform translation = CGAffineTransformMakeTranslation(delta_x, delta_y);
    if (in_paint->selection_path_moved) CGPathRelease(in_paint->selection_path_moved);
    in_paint->selection_path_moved = CGPathCreateCopyByTransformingPath(in_paint->selection_path, &translation);
    if (in_paint->selection_path_moved == NULL)
    {
        /* failed to move the path; invalidate the selection and error */
        _paint_dispose_selection(in_paint);
        return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    }
    
    /* update the display */
    _paint_needs_display(in_paint);
    
    /* should we finish? */
    if (in_finish)
    {
        /* update the selection tracking state to reflect the new location */
        in_paint->selection_bounds = in_paint->selection_bounds_moved;
        if (in_paint->selection_path) CGPathRelease(in_paint->selection_path);
        in_paint->selection_path = NULL;
        in_paint->selection_path = CGPathCreateMutableCopy(in_paint->selection_path_moved);
        if (in_paint->selection_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    }
}



/***********
 Tool Implementation
 */

/*
 *  _paint_select_begin
 *  ---------------------------------------------------------------------------------------------
 *  Begins a new selection with the rectangular or lasso selection tool, or begins a move of an
 *  existing selection, depending on the mouse/touch location.
 */

void _paint_select_begin(Paint *in_paint, int in_loc_x, int in_loc_y)
{
    assert(in_paint != NULL);
    assert((in_paint->current_tool == PAINT_TOOL_SELECT) || (in_paint->current_tool == PAINT_TOOL_LASSO));
    assert(IS_COORD(in_loc_x) && IS_COORD(in_loc_y));
    
    /* check if dragging on a selection */
    if (paint_loc_within_selection(in_paint, in_loc_x, in_loc_y))
    {
        /* begin drag of existing selection */
        in_paint->dragging_selection = TRUE;
        return;
    }
    
    /* drop current selection (if any) */
    _paint_drop_selection(in_paint);
    
    in_paint->selection_is_rect = (in_paint->current_tool == PAINT_TOOL_SELECT);
    
    /* begin lasso select */
    if (in_paint->current_tool == PAINT_TOOL_LASSO)
    {
        in_paint->selection_path = CGPathCreateMutable();
        if (in_paint->selection_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        
        CGPathMoveToPoint(in_paint->selection_path, NULL, in_paint->starting_point.x, in_paint->starting_point.y);
    }
}


/*
 *  _paint_select_continue
 *  ---------------------------------------------------------------------------------------------
 *  Continues plotting a new selection with the rectangular or lasso selection tool, or the move
 *  of an existing selection.
 *
 *  If the creation of the selection failed _paint_select_begin(), this function will safely 
 *  do nothing.
 */

void _paint_select_continue(Paint *in_paint, int in_loc_x, int in_loc_y)
{
    assert(in_paint != NULL);
    assert((in_paint->current_tool == PAINT_TOOL_SELECT) || (in_paint->current_tool == PAINT_TOOL_LASSO));
    assert(IS_COORD(in_loc_x) && IS_COORD(in_loc_y));
    
    /* are we dragging a selection? */
    if (in_paint->dragging_selection)
    {
        /* continue dragging selection */
        _paint_move_selection(in_paint, FALSE);
        return;
    }
    
    /* continue plotting with lasso */
    if (in_paint->current_tool == PAINT_TOOL_LASSO)
    {
        /* don't bawk at prior errors */
        if (in_paint->selection_path == NULL) return;
        
        CGPathAddLineToPoint(in_paint->selection_path, NULL, in_paint->last_point.x, in_paint->last_point.y);
    }
    
    /* continue plotting with rectangle */
    else if (in_paint->current_tool == PAINT_TOOL_SELECT)
    {
        in_paint->selection_bounds = CGRectMake(in_paint->starting_point.x,
                                                in_paint->starting_point.y,
                                                in_paint->ending_point.x - in_paint->starting_point.x,
                                                in_paint->ending_point.y - in_paint->starting_point.y);
        
        if (in_paint->selection_path) CGPathRelease(in_paint->selection_path);
        in_paint->selection_path = CGPathCreateMutable();
        if (in_paint->selection_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        CGPathAddRect(in_paint->selection_path, NULL, in_paint->selection_bounds);
    }
}


/*
 *  _paint_select_end
 *  ---------------------------------------------------------------------------------------------
 *  Finishes plotting a new selection with the rectangular or lasso selection tool, or the move
 *  of an existing selection.
 *
 *  If the creation/movement of the selection failed, this function will safely do nothing.
 */

void _paint_select_end(Paint *in_paint, int in_loc_x, int in_loc_y)
{
    assert(in_paint != NULL);
    assert((in_paint->current_tool == PAINT_TOOL_SELECT) || (in_paint->current_tool == PAINT_TOOL_LASSO));
    assert(IS_COORD(in_loc_x) && IS_COORD(in_loc_y));
    
    /* don't bawk at prior errors */
    if (in_paint->selection_path == NULL) return;
    
    /* are we dragging a selection? */
    if (in_paint->dragging_selection)
    {
        /* complete dragging selection to new location */
        in_paint->dragging_selection = FALSE;
        _paint_move_selection(in_paint, TRUE);
        return;
    }
    
    /* complete lasso selection */
    if (in_paint->current_tool == PAINT_TOOL_LASSO)
    {
        CGPathAddLineToPoint(in_paint->selection_path, NULL, in_paint->ending_point.x, in_paint->ending_point.y);
        CGPathCloseSubpath(in_paint->selection_path);
    }
    
    /* complete rectanglular selection */
    else if (in_paint->current_tool == PAINT_TOOL_SELECT)
    {
        if (in_paint->selection_path) CGPathRelease(in_paint->selection_path);
        in_paint->selection_path = CGPathCreateMutable();
        if (in_paint->selection_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        CGPathAddRect(in_paint->selection_path, NULL, CGRectMake(in_paint->starting_point.x,
                                                                 in_paint->starting_point.y,
                                                                 in_paint->ending_point.x - in_paint->starting_point.x,
                                                                 in_paint->ending_point.y - in_paint->starting_point.y));
    }
    
    /* are any pixels actually selected? */
    CGRect check_path = CGPathGetBoundingBox(in_paint->selection_path);
    if (CGRectIsEmpty(check_path))
    {
        /* no pixels; dispose of the selection */
        CGPathRelease(in_paint->selection_path);
        in_paint->selection_path = NULL;
    }
    else
    /* grab the selected paint and ready selection for dragging, etc. */
        _paint_grab_selection(in_paint);
}


/*
 *  _paint_draw_selection
 *  ---------------------------------------------------------------------------------------------
 *  Draws the selection at it's current location into the supplied graphics context.
 */

void _paint_draw_selection(Paint *in_paint, CGContextRef in_dest)
{
    assert(in_paint != NULL);
    assert(in_dest != NULL);
    
    /* do we have a selection? */
    if (in_paint->selection_path == NULL) return;
    
    /* configure the destination */
    CGContextSetShouldAntialias(in_dest, PAINT_FALSE);
    
    /* draw the selection paint */
    if (in_paint->context_selection != NULL)
    {
        CGImageRef selection_image = CGBitmapContextCreateImage(in_paint->context_selection);
        if (selection_image == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
        CGContextDrawImage(in_dest, _paint_cgrect_scale_auto(in_paint, in_paint->selection_bounds_moved), selection_image);
        CGImageRelease(selection_image);
    }
    
    /* draw the 'marching ants' around the selection */
    CGAffineTransform transformation = CGAffineTransformMakeScale(in_paint->scale, in_paint->scale);
    if (in_paint->scale == 1) transformation = CGAffineTransformConcat(transformation,
                                                                       CGAffineTransformMakeTranslation(0, 1));
    if (in_paint->scaled_selection) CGPathRelease(in_paint->scaled_selection);
    if (in_paint->selection_is_rect)
    {
        CGRect selection_bounds = (in_paint->selection_path_moved ?
                                   in_paint->selection_bounds_moved :
                                   in_paint->selection_bounds );
        if (in_paint->scale == 1)
        {
            /* adjust visible boundaries to more accurately reflect the actual selection */
            selection_bounds.size.width--;
            selection_bounds.size.height--;
        }
        in_paint->scaled_selection = CGPathCreateWithRect(selection_bounds, &transformation);
    }
    else
    {
        in_paint->scaled_selection = CGPathCreateCopyByTransformingPath((in_paint->selection_path_moved ?
                                                                         in_paint->selection_path_moved :
                                                                         in_paint->selection_path), &transformation);
    }
    if (in_paint->scaled_selection == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    
    CGContextSetRGBStrokeColor(in_dest, 0.0, 0.0, 0.0, 0.8);
    CGContextSetLineWidth(in_dest, 1.0);
    double dashArray[2] = {4.0, 4.0};
    
    CGContextBeginPath(in_dest);
    CGContextAddPath(in_dest, in_paint->scaled_selection);
    CGContextSetLineDash(in_dest, in_paint->ants_phase, dashArray, 2);
    CGContextStrokePath(in_dest);
    
    CGContextSetRGBStrokeColor(in_dest, 1.0, 1.0, 1.0, 0.8);
    CGContextBeginPath(in_dest);
    CGContextAddPath(in_dest, in_paint->scaled_selection);
    CGContextSetLineDash(in_dest, in_paint->ants_phase_180, dashArray, 2);
    CGContextStrokePath(in_dest);
}


/*
 *  _paint_selection_idle
 *  ---------------------------------------------------------------------------------------------
 *  Advances the 'marching ants' on the selection boundary.
 */

void _paint_selection_idle(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    if (in_paint->selection_path == NULL) return;
    
    in_paint->ants_phase++;
    in_paint->ants_phase_180++;
    if (in_paint->ants_phase >= 7) in_paint->ants_phase = 0;
    if (in_paint->ants_phase_180 >= 7) in_paint->ants_phase_180 = 0;
    
    _paint_needs_display(in_paint);
}


/*
 *  paint_has_selection
 *  ---------------------------------------------------------------------------------------------
 *  Returns PAINT_TRUE if there is a selection, PAINT_FALSE otherwise.
 */

int paint_has_selection(Paint *in_paint)
{
    assert(in_paint != NULL);
    return (in_paint->selection_path != NULL);
}


/*
 *  paint_delete
 *  ---------------------------------------------------------------------------------------------
 *  Deletes the selection and it's content.
 *
 *  If there is no selection, does nothing.
 */

void paint_delete(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    if (in_paint->selection_path == NULL) return;
    
    _paint_dispose_selection(in_paint);
    _paint_needs_display(in_paint);
}


/*
 *  paint_select_all
 *  ---------------------------------------------------------------------------------------------
 *  Selects the entire canvas area.
 */

void paint_select_all(Paint *in_paint)
{
    assert(in_paint != NULL);
    
    /* drop any existing selection */
    _paint_drop_selection(in_paint);
    
    in_paint->selection_is_rect = PAINT_TRUE;
    
    /* change the tool to the rectangular selection */
    paint_current_tool_set(in_paint, PAINT_TOOL_SELECT);
    
    /* create a selection rectangle */
    in_paint->selection_path = CGPathCreateMutable();
    if (in_paint->selection_path == NULL) return _paint_raise_error(in_paint, PAINT_ERROR_MEMORY);
    
    /* define the rectangle */
    CGPathAddRect(in_paint->selection_path, NULL, CGRectMake(0, 0, in_paint->width, in_paint->height));
    
    /* check if any pixels are selected */
    CGRect check_path = CGPathGetBoundingBox(in_paint->selection_path);
    if (CGRectIsEmpty(check_path))
    {
        /* no pixels selected, dispose selection */
        CGPathRelease(in_paint->selection_path);
        in_paint->selection_path = NULL;
    }
    else
        /* pixels selected; grab the selected paint
         and prepare it for movement, etc. */
        _paint_grab_selection(in_paint);
    
    /* refresh the display */
    _paint_needs_display(in_paint);
}


/*
 *  paint_loc_within_selection
 *  ---------------------------------------------------------------------------------------------
 *  Returns PAINT_TRUE if the supplied location is within the bounds of the selection.
 *
 *  If there is no selection, this returns PAINT_FALSE.
 */

int paint_loc_within_selection(Paint *in_paint, int in_loc_x, int in_loc_y)
{
    assert(in_paint != NULL);
    assert(IS_COORD(in_loc_x) && IS_COORD(in_loc_y));
    
    if (in_paint->scaled_selection != NULL)
    {
        return CGPathContainsPoint(in_paint->scaled_selection, NULL, CGPointMake(in_loc_x, in_loc_y), 1);
    }
    
    return FALSE;
}




