//
//  acu_callback_util.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 2/09/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "acu_int.h"

// NOT GOING TO USE THIS BECAUSE WE WILL MAKE IT DOCUMENTED THAT CERTAIN CALLBACKS USE THESE FUNCTIONS
// AND WE'LL CLEANUP MANUALLY ON A STACK-BY-STACK BASIS
void _acu_dispose_callback_results(void)
{
    /*_acu_thread_mutex_lock(&(_g_acu.ar_callback_result_mutex));
    _acu_autorelease_pool_drain(_g_acu.ar_callback_results);
    _acu_thread_mutex_unlock(&(_g_acu.ar_callback_result_mutex));*/
}


char* acu_callback_result_string(char const *in_string)
{
    if (!in_string) return NULL;
    long len = strlen(in_string);
    char *result = malloc(len + 1);
    if (!result) return NULL;
    strcpy(result, in_string);
    
    /*_acu_thread_mutex_lock(&(_g_acu.ar_callback_result_mutex));
    _acu_autorelease(_g_acu.ar_callback_results, result, _acu_free);
    _acu_thread_mutex_unlock(&(_g_acu.ar_callback_result_mutex));*/
    
    return result;
}


void* acu_callback_result_data(void const *in_data, int in_size)
{
    if (in_size == 0) return NULL;
    void *result = malloc(in_size);
    if (!result) return NULL;
    memcpy(result, in_data, in_size);
    
    /*_acu_thread_mutex_lock(&(_g_acu.ar_callback_result_mutex));
    _acu_autorelease(_g_acu.ar_callback_results, result, _acu_free);
    _acu_thread_mutex_unlock(&(_g_acu.ar_callback_result_mutex));*/
    
    return result;
}



