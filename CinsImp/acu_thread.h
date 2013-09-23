/*
 
 Application Control Unit - Threading
 acu_thread.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Platform-independent threading API; used internally
 
 *************************************************************************************************
 */

#ifndef CINSIMP_ACU_THREAD_H
#define CINSIMP_ACU_THREAD_H


/* POSIX threads */

#include <pthread.h>
#include <unistd.h>

typedef pthread_t ACUThread;
typedef pthread_mutex_t ACUThreadMutex;
typedef pthread_cond_t ACUThreadCond;


#endif
