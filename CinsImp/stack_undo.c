/*
 
 Stack File Format
 stack_undo.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Undo manager
 
 (see header file for module description)
 
 */

#include "stack_int.h"


/**********
 Configuration
 */


#define UNDO_STACK_DEPTH 1



/**********
 Implementation
 */


static void _undo_stack_frame_clear(Stack *in_stack, int in_frame_index)
{
    assert(in_stack->undo_stack != NULL);
    
    struct UndoFrame *frame = in_stack->undo_stack + in_frame_index;
    if (frame->description) _stack_free(frame->description);
    frame->description = NULL;
    for (int i = 0; i < frame->step_count; i++)
    {
        struct UndoStep *step = frame->steps + i;
        if (step->data) serbuff_destroy(step->data, 1);
    }
    if (frame->steps) _stack_free(frame->steps);
    frame->steps = NULL;
    frame->step_count = 0;
}


void stack_undo_flush(Stack *in_stack)
{
    if (!in_stack->undo_stack) return;
    
    for (int i = 0; i < in_stack->undo_stack_size; i++)
        _undo_stack_frame_clear(in_stack, i);
    in_stack->undo_redo_ptr = in_stack->undo_stack_top = 0;
}


void _undo_stack_destroy(Stack *in_stack)
{
    if (!in_stack->undo_stack) return;
    
    stack_undo_flush(in_stack);
    if (in_stack->undo_stack) _stack_free(in_stack->undo_stack);
}


void _undo_stack_create(Stack *in_stack)
{
    in_stack->undo_stack = _stack_malloc(sizeof(struct UndoFrame) * UNDO_STACK_DEPTH);
    if (!in_stack->undo_stack) app_out_of_memory_void();
    in_stack->undo_stack_size = UNDO_STACK_DEPTH;
    in_stack->undo_stack_top = 0;
    in_stack->undo_redo_ptr = 0;
    for (int i = 0; i < in_stack->undo_stack_size; i++)
    {
        struct UndoFrame *frame = in_stack->undo_stack + i;
        frame->description = NULL;
        frame->steps = NULL;
        frame->step_count = 0;
    }
}


void stack_undo_activity_begin(Stack *in_stack, const char *in_description, long in_card_id)
{
    if (!in_stack->undo_stack) return;
    
    if (in_stack->undo_redo_ptr < in_stack->undo_stack_top)
    {
        /* lop the top of the undo stack off */
        for (int i = in_stack->undo_redo_ptr; i < in_stack->undo_stack_top; i++)
            _undo_stack_frame_clear(in_stack, i);
        in_stack->undo_stack_top = in_stack->undo_redo_ptr;
    }
    
    struct UndoFrame *frame = in_stack->undo_stack + in_stack->undo_stack_top;
    in_stack->undo_redo_ptr = in_stack->undo_stack_top;
    
    _undo_stack_frame_clear(in_stack, in_stack->undo_stack_top);
    
    frame->description = _stack_clone_cstr((char*)in_description);
    frame->card_id = in_card_id;
    
    in_stack->record_undo_steps = 1;
}



/* this should ideally check to see if there are any steps recorded, if there are none,
 then remove the undo activity as if it never happened */
void stack_undo_activity_end(Stack *in_stack)
{
    if (!in_stack->undo_stack) return;
    
    if (in_stack->undo_stack_top == in_stack->undo_stack_size-1)
    {
        _undo_stack_frame_clear(in_stack, 0);
        memmove(in_stack->undo_stack, in_stack->undo_stack + 1, sizeof(struct UndoFrame) * (in_stack->undo_stack_size-1));
        struct UndoFrame *last_frame = in_stack->undo_stack + in_stack->undo_stack_top;
        last_frame->description = NULL;
        last_frame->steps = NULL;
        last_frame->step_count = 0;
    }
    else
        in_stack->undo_stack_top++;
    in_stack->undo_redo_ptr = in_stack->undo_stack_top;
    
    in_stack->record_undo_steps = 0;
}


void _undo_record_step(Stack *in_stack, enum UndoAction in_action, SerBuff *in_data)
{
    
    
    if ((!in_stack->undo_stack) || (!in_stack->record_undo_steps)) {
        if (in_data)
            serbuff_destroy(in_data, 1);
        return;
    }
    
    struct UndoFrame *frame = in_stack->undo_stack + in_stack->undo_redo_ptr;
    
    struct UndoStep *new_steps = _stack_realloc(frame->steps, sizeof(struct UndoStep) * (frame->step_count + 1));
    if (!new_steps) return;
    frame->steps = new_steps;
    struct UndoStep *step = new_steps + frame->step_count;
    frame->step_count++;
    
    step->action = in_action;
    step->data = in_data;
}


static void _undo_play_step(Stack *in_stack, enum UndoAction in_action, SerBuff *in_data)
{
    assert(in_stack->undo_stack != NULL);
    
    long widget_id, card_id, bkgnd_id;
    int widget_count;
    void *data;
    long size;
    long x,y,width,height;
    char *cstr;
    enum Property prop;
    long val;
    IDTable *table;
    SerBuff *undo_data;
    
    switch (in_action)
    {
        case UNDO_WIDGET_CREATE:
            /* delete the widget */
            stack_delete_widget(in_stack, serbuff_read_long(in_data));
            break;
            
        case UNDO_WIDGET_DELETE:
            /* create the widget */
            size = serbuff_read_data(in_data, &data);
            widget_count = _widgets_unserialize(in_stack, data, size, serbuff_read_long(in_data), serbuff_read_long(in_data),
                                                NULL, STACK_YES, STACK_YES, STACK_NO);
            break;
            
        case UNDO_WIDGET_SIZE:
            /* resize the widget */
            widget_id = serbuff_read_long(in_data);
            x = serbuff_read_long(in_data);
            y = serbuff_read_long(in_data);
            width = serbuff_read_long(in_data);
            height = serbuff_read_long(in_data);
            stack_widget_set_rect(in_stack, widget_id, x, y, width, height);
            break;
            
        case UNDO_WIDGET_CONTENT:
            widget_id = serbuff_read_long(in_data);
            card_id = serbuff_read_long(in_data);
            cstr = (char*)serbuff_read_cstr(in_data);
            size = serbuff_read_data(in_data, &data);
            stack_widget_content_set(in_stack, widget_id, card_id, cstr, data, size);
            break;
            
        case UNDO_WIDGET_PROPERTY_LONG:
            widget_id = serbuff_read_long(in_data);
            card_id = serbuff_read_long(in_data);
            prop = (enum Property)serbuff_read_long(in_data);
            val = serbuff_read_long(in_data);
            stack_widget_prop_set_long(in_stack, widget_id, card_id, prop, val);
            break;
            
        case UNDO_WIDGET_PROPERTY_STRING:
            widget_id = serbuff_read_long(in_data);
            card_id = serbuff_read_long(in_data);
            prop = (enum Property)serbuff_read_long(in_data);
            cstr = (char*)serbuff_read_cstr(in_data);
            stack_widget_prop_set_string(in_stack, widget_id, card_id, prop, cstr);
            break;
            
        case UNDO_PROPERTY_LONG:
            card_id = serbuff_read_long(in_data);
            bkgnd_id = serbuff_read_long(in_data);
            prop = (enum Property)serbuff_read_long(in_data);
            val = serbuff_read_long(in_data);
            stack_prop_set_long(in_stack, card_id, bkgnd_id, prop, val);
            break;
            
        case UNDO_PROPERTY_STRING:
            card_id = serbuff_read_long(in_data);
            bkgnd_id = serbuff_read_long(in_data);
            prop = (enum Property)serbuff_read_long(in_data);
            cstr = (char*)serbuff_read_cstr(in_data);
            stack_prop_set_string(in_stack, card_id, bkgnd_id, prop, cstr);
            break;
            
        case UNDO_REARRANGE_WIDGETS:
            undo_data = serbuff_create(in_stack, NULL, 0, 0);
            if (serbuff_read_long(in_data))
            {
                card_id = serbuff_read_long(in_data);
                
                serbuff_write_long(undo_data, 1);
                table = _stack_widget_seq_get(in_stack, card_id, 0);
                serbuff_write_long(undo_data, card_id);
                serbuff_write_cstr(undo_data, idtable_to_ascii(table));
                
                table = idtable_create_with_ascii(in_stack, serbuff_read_cstr(in_data));
                _stack_widget_seq_set(in_stack, card_id, 0, table);
            }
            else serbuff_write_long(undo_data, 0);
            if (serbuff_read_long(in_data))
            {
                bkgnd_id = serbuff_read_long(in_data);
                
                serbuff_write_long(undo_data, 1);
                table = _stack_widget_seq_get(in_stack, 0, bkgnd_id);
                serbuff_write_long(undo_data, bkgnd_id);
                serbuff_write_cstr(undo_data, idtable_to_ascii(table));
                
                table = idtable_create_with_ascii(in_stack, serbuff_read_cstr(in_data));
                _stack_widget_seq_set(in_stack, 0, bkgnd_id, table);
            }
            else serbuff_write_long(undo_data, 0);
            _undo_record_step(in_stack, UNDO_REARRANGE_WIDGETS, undo_data);
            break;
            
            
        case UNDO_CARD_CREATE:
            stack_card_delete(in_stack, serbuff_read_long(in_data));
            break;
            
        case UNDO_CARD_DELETE:
            size = serbuff_read_data(in_data, &data);
            _card_unserialize(in_stack, data, size, -1);
            break;
            
        default:
            break;
    }
}



const char* stack_can_undo(Stack *in_stack)
{
    if (!in_stack->undo_stack) return NULL;
    
    if ((in_stack->undo_stack_top == 0) || (in_stack->undo_redo_ptr == 0)) return NULL;
    struct UndoFrame *frame = in_stack->undo_stack + in_stack->undo_redo_ptr - 1;
    return frame->description;
}


const char* stack_can_redo(Stack *in_stack)
{
    if (!in_stack->undo_stack) return NULL;
    
    if (in_stack->undo_stack_top == in_stack->undo_redo_ptr) return NULL;
    struct UndoFrame *frame = in_stack->undo_stack + in_stack->undo_redo_ptr;
    return frame->description;
}


static void _undo_process_frame(Stack *in_stack, struct UndoFrame *in_frame)
{
    assert(in_stack->undo_stack != NULL);
    
    struct UndoFrame saved_frame;
    
    /* save the frame */
    saved_frame = *in_frame;
    
    /* reset the input frame */
    in_frame->step_count = 0;
    in_frame->steps = NULL;
    
    /* play back steps of saved frame */
    in_stack->record_undo_steps = 1;
    for (int i = saved_frame.step_count-1; i >= 0; i--)
    {
        struct UndoStep *step = saved_frame.steps + i;
        _undo_play_step(in_stack, step->action, step->data);
        
        /* cleanup */
        if (step->data) serbuff_destroy(step->data, 1);
    }
    if (saved_frame.steps) _stack_free(saved_frame.steps);
    in_stack->record_undo_steps = 0;
}



void stack_undo(Stack *in_stack, long *io_card_id)
{
    if (!in_stack->undo_stack) return;
    
    in_stack->undo_redo_ptr--;
    struct UndoFrame *frame = in_stack->undo_stack + in_stack->undo_redo_ptr;
    _undo_process_frame(in_stack, frame);
    
    long temp_card_id = *io_card_id;
    *io_card_id = frame->card_id;
    frame->card_id = temp_card_id;
}


void stack_redo(Stack *in_stack, long *io_card_id)
{
    if (!in_stack->undo_stack) return;
    
    struct UndoFrame *frame = in_stack->undo_stack + in_stack->undo_redo_ptr;
    _undo_process_frame(in_stack, frame);
    in_stack->undo_redo_ptr++;
    
    long temp_card_id = *io_card_id;
    *io_card_id = frame->card_id;
    frame->card_id = temp_card_id;
}




