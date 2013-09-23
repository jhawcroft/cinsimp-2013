/*
 
 Stack Tests: Integrity 1
 stack_test_integrity1.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Integrity test of the funamental features of the Stack file format under simulated layout usage:
 -  card creation/deletion
 -  widget creation/deletion
 -  undo/redo
 -  widget content
 -  widget tab sequence and adjustment
 -  pasteboard (implicit)
 
 *************************************************************************************************
 */

#include "stack_int.h"


#if STACK_TESTS

void _stack_test_general_integrity_1(void)
{
    long first_card_id;
    
    /* create a stack */
    Stack *in_stack = stack_create("/Users/josh/Desktop/unit.test.cinsstak", 512, 342, NULL, NULL);
    assert(in_stack);
    
    /* check the card size & window size */
    long width, height;
    stack_get_card_size(in_stack, &width, &height);
    assert(width == 512);
    assert(height == 342);
    stack_get_window_size(in_stack, &width, &height);
    assert(width == 512);
    assert(height == 342);
    
    /* take some card stats */
    assert(stack_card_count(in_stack) == 1);
    first_card_id = stack_card_id_for_index(in_stack, 1);
    assert(first_card_id == STACK_NO_OBJECT);
    first_card_id = stack_card_id_for_index(in_stack, 42);
    assert(first_card_id == STACK_NO_OBJECT);
    first_card_id = stack_card_id_for_index(in_stack, 0);
    assert(first_card_id != STACK_NO_OBJECT);
    
    /* create and delete some empty cards */
    long card_2;
    int err;
    card_2 = stack_card_create(in_stack, first_card_id, &err);
    assert(card_2 != STACK_NO_OBJECT);
    assert(card_2 != first_card_id);
    assert(stack_card_count(in_stack) == 2);
    assert(stack_card_index_for_id(in_stack, card_2) == 1);
    
    long card_3;
    card_3 = stack_card_create(in_stack, first_card_id, &err);
    assert(card_3 != STACK_NO_OBJECT);
    assert(card_3 != first_card_id);
    assert(stack_card_count(in_stack) == 3);
    assert(stack_card_index_for_id(in_stack, card_3) == 1);
    assert(stack_card_index_for_id(in_stack, card_2) == 2);
    
    long card_4;
    card_4 = stack_card_create(in_stack, card_2, &err);
    assert(card_4 != STACK_NO_OBJECT);
    assert(card_4 != first_card_id);
    assert(stack_card_count(in_stack) == 4);
    assert(stack_card_index_for_id(in_stack, card_4) == 3);
    
    long card_5;
    card_5 = stack_card_create(in_stack, card_4, &err);
    assert(card_5 != STACK_NO_OBJECT);
    assert(card_5 != first_card_id);
    assert(stack_card_count(in_stack) == 5);
    assert(stack_card_index_for_id(in_stack, card_5) == 4);
    
    long card_6;
    card_6 = stack_card_create(in_stack, card_3, &err);
    assert(card_6 != STACK_NO_OBJECT);
    assert(card_6 != first_card_id);
    assert(stack_card_count(in_stack) == 6);
    assert(stack_card_index_for_id(in_stack, card_6) == 2);
    assert(stack_card_index_for_id(in_stack, card_2) == 3);
    assert(stack_card_index_for_id(in_stack, card_4) == 4);
    
    long card_7;
    card_7 = stack_card_create(in_stack, card_5, &err);
    assert(card_7 != STACK_NO_OBJECT);
    assert(card_7 != first_card_id);
    assert(stack_card_count(in_stack) == 7);
    assert(stack_card_index_for_id(in_stack, card_5) == 5);
    assert(stack_card_index_for_id(in_stack, card_7) == 6);
    
    long card_id = stack_card_delete(in_stack, card_6);
    assert(stack_card_count(in_stack) == 6);
    assert(card_id == card_2);
    
    card_id = stack_card_delete(in_stack, card_3);
    assert(stack_card_count(in_stack) == 5);
    assert(card_id == card_2);
    
    card_id = stack_card_delete(in_stack, card_7);
    assert(stack_card_count(in_stack) == 4);
    assert(card_id == first_card_id);
    assert(stack_card_index_for_id(in_stack, card_5) == 3);
    assert(stack_card_index_for_id(in_stack, first_card_id) == 0);
    assert(stack_card_index_for_id(in_stack, card_2) == 1);
    assert(stack_card_index_for_id(in_stack, STACK_NO_OBJECT) == STACK_NO_OBJECT);
    assert(stack_card_index_for_id(in_stack, -5) == STACK_NO_OBJECT);
    assert(stack_card_index_for_id(in_stack, 42) == STACK_NO_OBJECT);
    assert(stack_card_index_for_id(in_stack, card_3) == STACK_NO_OBJECT);
    assert(stack_card_index_for_id(in_stack, card_6) == STACK_NO_OBJECT);
    assert(stack_card_index_for_id(in_stack, card_7) == STACK_NO_OBJECT);
    
    long card_8;
    card_8 = stack_card_create(in_stack, card_4, &err);
    assert(card_8 != STACK_NO_OBJECT);
    assert(stack_card_count(in_stack) == 5);
    assert(stack_card_index_for_id(in_stack, first_card_id) == 0);
    assert((first_card_id != card_2) &&
           (card_2 != card_4) &&
           (card_4 != card_8) &&
           (card_8 != card_5));
    assert(stack_card_index_for_id(in_stack, card_2) == 1);
    assert(stack_card_index_for_id(in_stack, card_4) == 2);
    assert(stack_card_index_for_id(in_stack, card_8) == 3);
    assert(stack_card_index_for_id(in_stack, card_5) == 4);
    assert(stack_card_id_for_index(in_stack, 4) == card_5);
    assert(stack_card_id_for_index(in_stack, 3) == card_8);
    assert(stack_card_id_for_index(in_stack, 2) == card_4);
    assert(stack_card_id_for_index(in_stack, 1) == card_2);
    assert(stack_card_id_for_index(in_stack, 0) == first_card_id);
    assert(stack_card_id_for_index(in_stack, -5) == STACK_NO_OBJECT);
    assert(stack_card_id_for_index(in_stack, 5) == STACK_NO_OBJECT);
    assert(stack_card_id_for_index(in_stack, -1) == STACK_NO_OBJECT);
    
    /* close and reopen the stack */
    stack_close(in_stack);
    StackOpenStatus stat;
    in_stack = stack_open("/Users/josh/Desktop/unit.test.cinsstak", NULL, NULL, &stat);
    assert(in_stack);
    
    /* verify card sequence and IDs */
    assert(stack_card_count(in_stack) == 5);
    assert(stack_card_index_for_id(in_stack, first_card_id) == 0);
    assert((first_card_id != card_2) &&
           (card_2 != card_4) &&
           (card_4 != card_8) &&
           (card_8 != card_5));
    assert(stack_card_index_for_id(in_stack, card_2) == 1);
    assert(stack_card_index_for_id(in_stack, card_4) == 2);
    assert(stack_card_index_for_id(in_stack, card_8) == 3);
    assert(stack_card_index_for_id(in_stack, card_5) == 4);
    assert(stack_card_id_for_index(in_stack, 4) == card_5);
    assert(stack_card_id_for_index(in_stack, 3) == card_8);
    assert(stack_card_id_for_index(in_stack, 2) == card_4);
    assert(stack_card_id_for_index(in_stack, 1) == card_2);
    assert(stack_card_id_for_index(in_stack, 0) == first_card_id);
    assert(stack_card_id_for_index(in_stack, -5) == STACK_NO_OBJECT);
    assert(stack_card_id_for_index(in_stack, 5) == STACK_NO_OBJECT);
    assert(stack_card_id_for_index(in_stack, -1) == STACK_NO_OBJECT);
    
    /* add some fields to verious cards and backgrounds */
    long field_1 = stack_create_widget(in_stack, WIDGET_FIELD_TEXT, first_card_id, STACK_NO_OBJECT, &err);
    assert(field_1 != STACK_NO_OBJECT);
    assert(field_1 > 0);
    
    long field_2 = stack_create_widget(in_stack, WIDGET_FIELD_TEXT, first_card_id, STACK_NO_OBJECT, &err);
    assert(field_2 != STACK_NO_OBJECT);
    assert(field_2 > 0);
    assert(stack_widget_count(in_stack, first_card_id, STACK_NO_OBJECT) == 2);
    assert(stack_widget_count(in_stack, card_2, STACK_NO_OBJECT) == 0);
    assert(stack_widget_count(in_stack, card_4, STACK_NO_OBJECT) == 0);
    assert(stack_widget_count(in_stack, first_card_id, STACK_NO_OBJECT) == 2);
    
    long field_3 = stack_create_widget(in_stack, WIDGET_FIELD_TEXT, STACK_NO_OBJECT, 1, &err);
    assert(field_3 != STACK_NO_OBJECT);
    assert(field_3 > 0);
    assert(stack_widget_count(in_stack, STACK_NO_OBJECT, 1) == 1);
    assert(stack_widget_count(in_stack, first_card_id, STACK_NO_OBJECT) == 2);
    assert(stack_widget_count(in_stack, card_2, STACK_NO_OBJECT) == 0);
    assert(stack_widget_count(in_stack, card_4, STACK_NO_OBJECT) == 0);
    assert(stack_widget_count(in_stack, first_card_id, STACK_NO_OBJECT) == 2);
    
    assert(stack_card_bkgnd_id(in_stack, first_card_id) == 1);
    
    /* add some content to the fields on multiple cards */
    stack_widget_content_set(in_stack, field_2, first_card_id, "Hello World!", NULL, 0);
    stack_widget_content_set(in_stack, field_3, card_4, "This is the 3rd card.", NULL, 0);
    stack_widget_content_set(in_stack, field_3, card_5, "This is the LAST card.", NULL, 0);
    stack_widget_content_set(in_stack, field_3, first_card_id, "This is the 1st card.", NULL, 0);
    
    /* verify text content */
    char *searchable;
    stack_widget_content_get(in_stack, field_2, first_card_id, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "Hello World!") == 0);
    
    stack_widget_content_get(in_stack, field_3, first_card_id, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "This is the 1st card.") == 0);
    
    stack_widget_content_get(in_stack, field_3, card_2, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    stack_widget_content_get(in_stack, field_3, card_4, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "This is the 3rd card.") == 0);
    
    stack_widget_content_get(in_stack, field_3, card_8, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    stack_widget_content_get(in_stack, field_3, card_5, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "This is the LAST card.") == 0);
    
    stack_widget_content_get(in_stack, field_1, first_card_id, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    stack_widget_content_get(in_stack, field_3, first_card_id, STACK_YES, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    /* delete a card and try undo */
    assert(stack_can_undo(in_stack) == NULL);
    assert(stack_can_redo(in_stack) == NULL);
    stack_undo_activity_begin(in_stack, "Delete Card", card_4);
    card_id = stack_card_delete(in_stack, card_4);
    stack_undo_activity_end(in_stack);
    assert(card_id == card_8);
    assert(stack_card_count(in_stack) == 4);
    //assert(strcmp(stack_can_undo(in_stack), "Delete Card") == 0);
    assert(stack_can_undo(in_stack) == NULL);
    assert(stack_can_redo(in_stack) == NULL);
    
    /* verify text content is gone from database */
    stack_widget_content_get(in_stack, field_3, card_4, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    /* verify text content */
    stack_widget_content_get(in_stack, field_2, first_card_id, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "Hello World!") == 0);
    
    stack_widget_content_get(in_stack, field_3, first_card_id, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "This is the 1st card.") == 0);
    
    stack_widget_content_get(in_stack, field_3, card_2, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    stack_widget_content_get(in_stack, field_3, card_8, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    stack_widget_content_get(in_stack, field_3, card_5, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "This is the LAST card.") == 0);
    
    stack_widget_content_get(in_stack, field_1, first_card_id, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    stack_widget_content_get(in_stack, field_3, first_card_id, STACK_YES, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    /* undo the delete card */
    stack_undo(in_stack, &card_id);
    assert(stack_card_count(in_stack) == 5);
    assert(strcmp(stack_can_redo(in_stack), "Delete Card") == 0);
    assert(stack_can_undo(in_stack) == NULL);
    assert(stack_can_redo(in_stack) == NULL);
    assert(card_id == 4);
    
    /* verify the text is back */
    stack_widget_content_get(in_stack, field_3, card_4, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "This is the 3rd card.") == 0);
    
    /* redo the delete card */
    stack_redo(in_stack, &card_id);
    assert(card_id == card_8);
    assert(stack_card_count(in_stack) == 4);
    
    /* verify text content is gone (again) */
    stack_widget_content_get(in_stack, field_3, card_4, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    /* undo the delete card (one more time) */
    stack_undo(in_stack, &card_id);
    assert(stack_card_count(in_stack) == 5);
    assert(strcmp(stack_can_redo(in_stack), "Delete Card") == 0);
    assert(stack_can_undo(in_stack) == NULL);
    assert(card_id == 4);
    
    /* verify the text is back, phew */
    stack_widget_content_get(in_stack, field_3, card_4, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "This is the 3rd card.") == 0);
    
    /* rearrange some widgets and tab sequence */
    long widgets[2];
    widgets[0] = field_1;
    stack_widgets_send_front(in_stack, widgets, 1);
    long widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    
    widgets[0] = field_2;
    stack_widgets_send_front(in_stack, widgets, 1);
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    
    /* add lots of widgets */
    long field_4 = stack_create_widget(in_stack, WIDGET_FIELD_TEXT, first_card_id, STACK_NO_OBJECT, &err);
    long field_5 = stack_create_widget(in_stack, WIDGET_FIELD_TEXT, first_card_id, STACK_NO_OBJECT, &err);
    long field_6 = stack_create_widget(in_stack, WIDGET_FIELD_TEXT, first_card_id, STACK_NO_OBJECT, &err);
    long field_7 = stack_create_widget(in_stack, WIDGET_FIELD_TEXT, first_card_id, STACK_NO_OBJECT, &err);
    long field_8 = stack_create_widget(in_stack, WIDGET_FIELD_TEXT, first_card_id, STACK_NO_OBJECT, &err);
    
    /* repeat the rearranging test with more widgets in more complex arrangements */
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_4);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    
    widgets[0] = field_6;
    widgets[1] = field_1;
    stack_widgets_shuffle_forward(in_stack, widgets, 2);
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_4);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    
    widgets[0] = field_6;
    widgets[1] = field_1;
    stack_widgets_shuffle_backward(in_stack, widgets, 2);
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_4);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    
    widgets[0] = field_6;
    widgets[1] = field_1;
    stack_widgets_send_back(in_stack, widgets, 2);
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_4);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    
    widgets[1] = field_2;
    widgets[0] = field_7;
    stack_widgets_send_front(in_stack, widgets, 2);
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_4);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    
    widgets[0] = field_2;
    widgets[1] = field_7;
    stack_widgets_send_back(in_stack, widgets, 2);
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_4);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    
    /* delete a widget and tab sequence */
    stack_undo_activity_begin(in_stack, "Delete Card", first_card_id);
    stack_delete_widget(in_stack, field_4);
    stack_undo_activity_end(in_stack);
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    
    /* undo delete widget and verify sequence */
    card_id = first_card_id;
    stack_undo(in_stack, &card_id);
    assert(card_id == first_card_id);
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_4);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    
    /* redo the delete & verify sequence */
    stack_redo(in_stack, &card_id);
    assert(card_id == first_card_id);
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    
    /* undo one more time */
    stack_undo(in_stack, &card_id);
    assert(card_id == first_card_id);
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_4);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    
    /* close and reopen the stack */
    stack_close(in_stack);
    in_stack = stack_open("/Users/josh/Desktop/unit.test.cinsstak", NULL, NULL, &stat);
    assert(in_stack);
    
    /* check cards & card sequences */
    widget_id = stack_widget_next(in_stack, STACK_NO_OBJECT, first_card_id, STACK_NO);
    assert(widget_id == field_3);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_4);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_5);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_8);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_1);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_6);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_2);
    widget_id = stack_widget_next(in_stack, widget_id, first_card_id, STACK_NO);
    assert(widget_id == field_7);
    
    stack_widget_content_get(in_stack, field_2, first_card_id, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "Hello World!") == 0);
    
    stack_widget_content_get(in_stack, field_3, first_card_id, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "This is the 1st card.") == 0);
    
    stack_widget_content_get(in_stack, field_3, card_2, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    stack_widget_content_get(in_stack, field_3, card_8, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    stack_widget_content_get(in_stack, field_3, card_5, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "This is the LAST card.") == 0);
    
    stack_widget_content_get(in_stack, field_1, first_card_id, STACK_NO, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    stack_widget_content_get(in_stack, field_3, first_card_id, STACK_YES, &searchable, NULL, NULL, NULL);
    assert(strcmp(searchable, "") == 0);
    
    assert(stack_card_count(in_stack) == 5);
    assert(stack_card_index_for_id(in_stack, first_card_id) == 0);
    assert((first_card_id != card_2) &&
           (card_2 != card_4) &&
           (card_4 != card_8) &&
           (card_8 != card_5));
    assert(stack_card_index_for_id(in_stack, card_2) == 1);
    assert(stack_card_index_for_id(in_stack, card_4) == 2);
    assert(stack_card_index_for_id(in_stack, card_8) == 3);
    assert(stack_card_index_for_id(in_stack, card_5) == 4);
    assert(stack_card_id_for_index(in_stack, 4) == card_5);
    assert(stack_card_id_for_index(in_stack, 3) == card_8);
    assert(stack_card_id_for_index(in_stack, 2) == card_4);
    assert(stack_card_id_for_index(in_stack, 1) == card_2);
    assert(stack_card_id_for_index(in_stack, 0) == first_card_id);
    assert(stack_card_id_for_index(in_stack, -5) == STACK_NO_OBJECT);
    assert(stack_card_id_for_index(in_stack, 5) == STACK_NO_OBJECT);
    assert(stack_card_id_for_index(in_stack, -1) == STACK_NO_OBJECT);
    
    stack_close(in_stack);
}


#endif

