#ifndef _PTHREAD_TYPES_H_
#define _PTHREAD_TYPES_H_

/*==========================================================================
 * FILE:         pthread_types.c
 *
 * SERVICES:     types usded in POSIX API interface
 *
 * DESCRIPTION:  POSIX API interface based upon POSIX 1003.1-2004
 *
 *               Copyright (c) 2016, 2023  by Qualcomm Technologies, Inc.  All Rights Reserved. QUALCOMM Proprietary and Confidential.

 *==========================================================================*/

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#define restrict __restrict__
#else
#define restrict
#endif

#define _SSIZE_T

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE    0
#endif

#define PTHREAD_MAX_THREADS          512U

#define PTHREAD_NAME_LEN             16
#define PTHREAD_MIN_STACKSIZE        512 //4096
#define PTHREAD_MAX_STACKSIZE        1048576
#define PTHREAD_DEFAULT_STACKSIZE    16384

#define PTHREAD_STACK_MIN            (4096U*2U)
#define PTHREAD_MIN_PRIORITY         0U
#define PTHREAD_MAX_PRIORITY         255U
#define PTHREAD_DEFAULT_PRIORITY     1

/*Mutex initialization status*/
#define PTHREAD_MUTEX_ATTR_UNINITIALIZED    0
#define PTHREAD_MUTEX_ATTR_INITIALIZED      1

/*Conditional attributes initialization status*/
#define PTHREAD_COND_ATTR_UNINITIALIZED     0
#define PTHREAD_COND_ATTR_INITIALIZED       1

#define PTHREAD_DEFAULT_NAME                "Anonymous"

#define PTHREAD_MUTEX_INITIALIZER    ((pthread_mutex_t) 0xFFFFFFFFU)
                                      
#define PTHREAD_COND_INITIALIZER     ((pthread_cond_t) 0xFFFFFFFFU)

/* mutex and cond_var shared */
#define PTHREAD_PROCESS_PRIVATE      0
#define PTHREAD_PROCESS_SHARED       1

/* mutex type */
#define PTHREAD_MUTEX_ERRORCHECK     0
#define PTHREAD_MUTEX_NORMAL         1
#define PTHREAD_MUTEX_RECURSIVE      2
#define PTHREAD_MUTEX_DEFAULT        3

/* mutex protocol */
#define PTHREAD_PRIO_NONE            0
#define PTHREAD_PRIO_INHERIT         1
#define PTHREAD_PRIO_PROTECT         2

#define PTHREAD_SPINLOCK_UNLOCKED    0
#define PTHREAD_SPINLOCK_LOCKED      1

#define PTHREAD_ONCE_INIT (0)

#define PTHREAD_MUTEX_OPAQUE //ToDo: amitkulk: debug

typedef signed int   ssize_t;

/*detatchstate of a pthread*/
#define PTHREAD_CREATE_JOINABLE             1
#define PTHREAD_CREATE_DETACHED             0

/*contention scope*/
#define PTHREAD_SCOPE_PROCESS 1 
#define PTHREAD_SCOPE_SYSTEM 0

/*scheduler*/
#define PTHREAD_INHERIT_SCHED 1
#define PTHREAD_EXPLICIT_SCHED 0

/*
 * Types and structure definitions
 *
 */
typedef unsigned int cpu_set_t;

typedef unsigned int pthread_t;

typedef struct pthread_attr_t
{
    void         *stackaddr;
    int          internal_stack; /* this flag==1 means the stack needs to be freed by posix */
    size_t       stacksize;
    int          priority;
    unsigned short timetest_id;
    /* This flag indicate if thread will be autostack thread*/    
	unsigned short autostack:1;
    /* This flag is to indicate thread's bus_priority high/low 
       bus_priority = 0  -- Bus_priority is low
       bus_priority = 1  -- Bus_priority is high
       bus_priority = 3  -- Bus_priority is default (takes the default set for the process)
    */
    unsigned short bus_priority:2;
    unsigned short reserved:13;
    cpu_set_t    cpumask;
    char         name[PTHREAD_NAME_LEN];
    /* This flag indicates whether pthread lib should create thread contexts for other OSALs */
    /* This is used internally by POSIX and not available for general usage */
    int          ext_context;
    int          detachstate;
} pthread_attr_t;

//mutex attr
typedef struct pthread_mutexattr_t   pthread_mutexattr_t;
struct pthread_mutexattr_t
{
    int is_initialized;
    int type;
    int pshared;
    int protocol;
};

typedef unsigned int              pthread_mutex_t;

typedef unsigned int              pthread_spinlock_t;

typedef struct pthread_condattr_t
{
    int is_initialized;
    int pshared;
    clockid_t clock_id;
} pthread_condattr_t;

typedef unsigned int             pthread_cond_t;

typedef struct pthread_barrierattr_t
{
    int is_initialized;
    int pshared;
} pthread_barrierattr_t;

typedef unsigned int                pthread_barrier_t;

typedef int pthread_key_t;

typedef int pthread_once_t;


/*Read-Write locks*/
#define PTW32_RWLOCK_MAGIC 0xfacade2
#define PTHREAD_RWLOCK_INITIALIZER ((pthread_rwlock_t)(size_t) -1)

struct pthread_rwlockattr_t_
{
  int pshared;
};

struct pthread_rwlock_t_
{
  pthread_mutex_t mtxExclusiveAccess;
  pthread_mutex_t mtxSharedAccessCompleted;
  pthread_cond_t cndSharedAccessCompleted;
  int nSharedAccessCount;
  int nExclusiveAccessCount;
  int nCompletedSharedAccessCount;
  int nMagic;
};

typedef struct pthread_rwlock_t_ * pthread_rwlock_t;
typedef struct pthread_rwlockattr_t_ * pthread_rwlockattr_t;
#ifdef __cplusplus
}
#endif

#endif /* _PTHERAD_TYPES_H_ */
