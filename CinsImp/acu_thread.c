/*
 
 Application Control Unit - Threading
 acu_thread.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Platform-independent threading API; used internally
 
 *************************************************************************************************
 */

#include "acu_int.h"




int _acu_thread_create(ACUThread *out_thread, void *in_routine, void *in_context, long in_stack_size)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, in_stack_size);
    
    int err = pthread_create(out_thread,
                             &attr,
                             in_routine,
                             in_context);
    
    pthread_attr_destroy(&attr);
    
    return (err == 0);
}


void _acu_thread_exit(void)
{
    pthread_exit(NULL);
}



int _acu_thread_mutex_create(ACUThreadMutex *out_mutex)
{
    pthread_mutexattr_t attr_mutex;
    pthread_mutexattr_init(&attr_mutex);
    int err = pthread_mutex_init(out_mutex, &attr_mutex);
    pthread_mutexattr_destroy(&attr_mutex);
    
    return (err == 0);
}


int _acu_thread_cond_create(ACUThreadCond *out_cond)
{
    pthread_condattr_t attr_cond;
    pthread_condattr_init(&attr_cond);
    int err = pthread_cond_init(out_cond, &attr_cond);
    pthread_condattr_destroy(&attr_cond);
    
    return (err == 0);
}


void _acu_thread_mutex_dispose(ACUThreadMutex *in_mutex)
{
    if (pthread_mutex_destroy(in_mutex)) _acu_raise_error(ACU_ERROR_INTERNAL);
}


void _acu_thread_cond_dispose(ACUThreadCond *in_cond)
{
    if (pthread_cond_destroy(in_cond)) _acu_raise_error(ACU_ERROR_INTERNAL);
}


void _acu_thread_mutex_lock(ACUThreadMutex *in_mutex)
{
    if (pthread_mutex_lock(in_mutex)) _acu_raise_error(ACU_ERROR_INTERNAL);
}


void _acu_thread_mutex_unlock(ACUThreadMutex *in_mutex)
{
    if (pthread_mutex_unlock(in_mutex)) _acu_raise_error(ACU_ERROR_INTERNAL);
}


void _acu_thread_cond_signal(ACUThreadCond *in_cond)
{
    if (pthread_cond_signal(in_cond)) _acu_raise_error(ACU_ERROR_INTERNAL);
}


void _acu_thread_cond_wait(ACUThreadCond *in_cond, ACUThreadMutex *in_mutex)
{
    if (pthread_cond_wait(in_cond, in_mutex)) _acu_raise_error(ACU_ERROR_INTERNAL);
}


void _acu_thread_usleep(int in_usecs)
{
    usleep(in_usecs);
}



