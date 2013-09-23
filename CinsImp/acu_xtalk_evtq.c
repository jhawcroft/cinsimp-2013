/*
 
 Application Control Unit - Event Queue
 acu_xtalk_evtq.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 System event queue for an instance of the xtalk engine; allows the user to continue generating
 system events, eg. by pressing multiple keys on the keyboard in rapid succession, while the 
 xtalk interpreter is still handling the first of the series of keypresses.
 
 Queue size is configured at compile time with ACU_SYS_EVENT_QUEUE_SIZE, see _limits.h.
 
 *************************************************************************************************
 */

#include "acu_int.h"


/******************
 Internal Configuration (Unpublished)
 */

/*
 *  _MAX_SENSIBLE_QUEUE_SIZE
 *  ---------------------------------------------------------------------------------------------
 *  Used only during debugging to help ensure validity of the queue.  Queues cannot be created
 *  to hold more than this number of items without raising a failed assertion.
 */
#define _MAX_SENSIBLE_QUEUE_SIZE 1000000



/******************
 Implementation
 */

/*
 *  _acu_event_queue_create
 *  ---------------------------------------------------------------------------------------------
 *  Creates and returns a new event queue of the specified size (the maximum number of items the
 *  queue may hold.)
 *
 *  If there is a problem allocating the memory for the queue, returns NULL.
 */
ACUEventQueue* _acu_event_queue_create(int in_queue_size)
{
    assert(in_queue_size > 0);
    assert(in_queue_size < _MAX_SENSIBLE_QUEUE_SIZE);
    
    ACUEventQueue *queue = calloc(1, sizeof(ACUEventQueue));
    queue->size = in_queue_size;
    
    queue->events = calloc(in_queue_size, sizeof(ACUEvent));
    
    return queue;
}


/*
 *  _acu_event_queue_evt_zap
 *  ---------------------------------------------------------------------------------------------
 *  Safely cleans up allocated objects within an event.
 */
static void _acu_event_queue_evt_zap(ACUEvent *in_event)
{
    if (in_event->message) free(in_event->message);
    if (in_event->target) xte_variant_release(in_event->target);
    for (int p = 0; p < in_event->param_count; p++)
    {
        if (in_event->params[p]) xte_variant_release(in_event->params[p]);
    }
    in_event->target = NULL;
    in_event->param_count = 0;
}


/*
 *  _acu_event_queue_dispose
 *  ---------------------------------------------------------------------------------------------
 *  Safely disposes of a queue and any contents when it's no longer needed.
 */
void _acu_event_queue_dispose(ACUEventQueue *in_queue)
{
    assert(in_queue != NULL);
    
    for (int i = in_queue->front; i < in_queue->count; i++)
    {
        ACUEvent *event = &(in_queue->events[i]);
        _acu_event_queue_evt_zap(event);
    }
    
    ACUEvent *event = &(in_queue->last_event);
    _acu_event_queue_evt_zap(event);
    
    if (in_queue->events) free(in_queue->events);
    free(in_queue);
}


/*
 *  _acu_event_queue_post
 *  ---------------------------------------------------------------------------------------------
 *  Posts an event to the end of the specified queue.  The event definition is copied and is not 
 *  required after this call has returned.
 *
 *  Returns ACU_TRUE if successful, or ACU_FALSE if there is a problem.  The most common problem
 *  is that the queue is full - ie. it contains the maximum number of events defined at queue
 *  creation.
 */
int _acu_event_queue_post(ACUEventQueue *in_queue, ACUEvent *in_event)
{
    assert(in_queue != NULL);
    assert(in_event != NULL);
    
    //printf("Queue: %d\n", in_queue->count);
    
    if (in_queue->count == in_queue->size) return ACU_FALSE;
    
    if (in_queue->back == in_queue->size) in_queue->back = 0;
    ACUEvent *the_event = &(in_queue->events[in_queue->back++]);
    the_event->message = _acu_clone_cstr(in_event->message);
    the_event->target = xte_variant_retain(in_event->target);
    the_event->type = in_event->type;
    assert(the_event->param_count >= 0);
    assert(the_event->param_count <= ACU_LIMIT_MAX_SYS_EVENT_PARAMS);
    for (int p = 0; p < in_event->param_count; p++)
    {
        the_event->params[p] = xte_variant_retain(in_event->params[p]);
    }
    the_event->param_count = in_event->param_count;
    
    in_queue->count++;
    
    return ACU_TRUE;
}


/*
 *  _acu_event_queue_count
 *  ---------------------------------------------------------------------------------------------
 *  Returns the number of items currently in the specified queue.
 */
int _acu_event_queue_count(ACUEventQueue *in_queue)
{
    assert(in_queue != NULL);
    return in_queue->count;
}


/*
 *  _acu_event_queue_next
 *  ---------------------------------------------------------------------------------------------
 *  Returns a pointer to the next item in the queue and removes that item from the front of the
 *  queue.  The pointer will remain valid until either the queue is disposed or the function is
 *  invoked again.
 *
 *  If the queue is currently empty, returns NULL.
 */
ACUEvent* _acu_event_queue_next(ACUEventQueue *in_queue)
{
    assert(in_queue != NULL);
    
    if (in_queue->count == 0) return NULL;
    
    if (in_queue->front == in_queue->size) in_queue->front = 0;
    ACUEvent *the_event = &(in_queue->events[in_queue->front++]);
    
    _acu_event_queue_evt_zap(&(in_queue->last_event));
    
    in_queue->last_event = *the_event;
    the_event->message = NULL;
    for (int p = 0; p < the_event->param_count; p++)
    {
        the_event->params[p] = NULL;
    }
    the_event->param_count = 0;
    
    in_queue->count--;
    
    return &(in_queue->last_event);
}





