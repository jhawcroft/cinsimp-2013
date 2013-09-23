/*
 
 Application Control Unit - Limits
 acu_limits.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Limits
 
 *************************************************************************************************
 */

#ifndef CinsImp_acu_limits__h
#define CinsImp_acu_limits__h



/* maximum handle description in bytes */
#define _MAX_DESC_LENGTH 4095



#define _OPEN_STACKS_ALLOC 10


#define _LIMIT_OPEN_STACKS 100


#define ACU_STACK_SIZE_XTE 1 * 1024 * 1024 /* 1 MB */


#define ACU_MAX_THREAD_IMP_ARGS 32


#define ACU_LIMIT_MAX_SYS_EVENT_PARAMS 10

#define ACU_SYS_EVENT_QUEUE_SIZE 100


#define ACU_LIMIT_MAX_VISUAL_EFFECTS 20


/* maximum number of files opened by xtalk scripts at one time */
#define ACU_LIMIT_MAX_OPEN_FILES 25


#define ACU_LIMIT_MAX_READ_LINE_BYTES 100 * 1024 * 1024 /* 100 MB */

/* maximum implementation channel parameters for SIC */
#define ACU_MAX_IC_PARAMS 32


#endif
