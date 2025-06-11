#ifndef SEMAPHORE_H
#define SEMAPHORE_H

/*==========================================================================
 * FILE:         semaphore.h
 *
 * SERVICES:     POSIX semaphore API interface
 *
 * DESCRIPTION:  POSIX semaphore API interface based upon POSIX 1003.1-2004
 *
 *               Copyright (c) 2013, 2016  by Qualcomm Technologies, Inc.  All Rights Reserved. QUALCOMM Proprietary and Confidential.

 *==========================================================================*/
#include <sys/types.h> // Get all C sys types - includes POSIX specific
#include "sys/errno.h" // error values

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                        TYPEDEFS
=============================================================================*/
/** User facing semaphore container with opaque pointer to implementation */
typedef struct
{
    unsigned int *opaque;
} sem_t;
#define _SEM_T

/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/
/* constant definitions */
#define SEM_FAILED       ((sem_t*) 0)

/* @todo siqbal Should we put such configuration items in a common place
   instead of this user-facing header? */
#define SEM_VALUE_MAX    ((unsigned int) 30) // If need be increase this

/*=============================================================================
                        FUNCTIONS
=============================================================================*/

/** \details
 * POSIX standard comes with two kinds of semaphores: named and unnamed
 * semaphores.
 *
 * This implementation of POSIX kernel API provide unnamed & named semaphore.
 *
 * 
 * sem_timedwait() is not provided.
 */

/** \defgroup semaphore POSIX Semaphore API */

/** \ingroup semaphore */
/** @{ */

/** Initialize an unnamed semaphore.
 * Please refer to POSIX standard for details.
 * @param pshared [in] This implementation does not support non-zero value, 
 * i.e., semaphore cannot be shared between processes in this implementation. 
 */                 
int sem_init(sem_t *sem, int pshared, unsigned int value);

/** Lock a semaphore.
 * Please refer to POSIX standard for details.
 */
int sem_wait(sem_t *sem);

/** Lock a semaphore.
 * Please refer to POSIX standard for details.
 */
int sem_trywait(sem_t *sem);

/** Unlock a semaphore.
 * Please refer to POSIX standard for details.
 */
int sem_post(sem_t *sem);

/** Get the value of a semaphore.
 * Please refer to POSIX standard for details.
 */
int sem_getvalue(sem_t *sem, int *value);

/** Destroy an unnamed semaphore.
 * Please refer to POSIX standard for details.
 */
int sem_destroy(sem_t *sem);

/** creates and initializes a named semaphore.
 * Please refer to POSIX standard for details.
 */
sem_t * sem_open(const char* name , int oflag , ...);

/** closes a semaphore.
 * Please refer to POSIX standard for details.
 */
int sem_close(sem_t *sem);

/** unlinkes a named semaphore.
 * Please refer to POSIX standard for details.
 */
int sem_unlink(const char *name);
/** @} */


#ifdef __cplusplus
}
#endif

#endif  /* SEMAPHORE_H */

