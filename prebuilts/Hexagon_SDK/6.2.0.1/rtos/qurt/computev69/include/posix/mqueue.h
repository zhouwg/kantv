#ifndef _POSIX_MQUEUE_H_
#define _POSIX_MQUEUE_H_

/*==========================================================================
 * FILE:         mqueue.h
 *
 * SERVICES:     POSIX Message Queue API interface
 *
 * DESCRIPTION:  POSIX Message Queue API interface based upon POSIX 1003.1-2004
 *
 * Copyright (c) 2013, 2016, 2023 Qualcomm Technologies, Inc.  
 * All Rights Reserved. 
 * Confidential and Proprietary - Qualcomm Technlogies, Inc.
 *==========================================================================*/

#include <sys/types.h> /*ssize_t */
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MQ_PRIO_MAX        255     /* max priority */
#define MQ_PRIO_DEFAULT    0       /* default priority */

typedef int   mqd_t;

struct mq_attr
{
    long mq_flags;   /* message queue flags */
    long mq_maxmsg;  /* maximum number of messages */
    long mq_msgsize; /* maximum message size */
    long mq_curmsgs; /* number of messages currently queued */
};

typedef struct mq_attr mqueue_attr;

/** \details
 * This provides POSIX Message Queue API.
 *
 * mq_notify is not supported.
 *
 * Since this implementation of POSIX kernel API is a subset of PSE51,
 * it only supports Message sending and receiving within one process.
 * Message sending and receiving among processes are not supported.
 */

/** \defgroup mqueue POSIX Message Queue API */
/** \ingroup mqueue */
/** @{ */

/** Open a message queue.
 * Please refer to POSIX standard for details.
 */
mqd_t mq_open(const char *name, int oflag, /* mode_t mode, struct mq_attr *attr */...);

/** Close a message queue.
 * Please refer to POSIX standard for details.
 */
int mq_close(mqd_t mq_desc);

/** Remove a message queue.
 * Please refer to POSIX standard for details.
 */
int mq_unlink(const char *name);

/** Send a message to a message queue.
 * Please refer to POSIX standard for details.
 *
 * If the queue is full, instead of blocking the sender, this function
 * will return -1 with errno EAGAIN, in this implementation. This behavior
 * may change in the future.
 */
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio);

/** Send a message to a message queue with timeout.
 * Please refer to POSIX standard for details.
 * @param abs_timeout [in] Only abs_timeout={0,0} is supported in this
 * implementation. This behavior may change in the future.
 */
int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abs_timeout);

/** Receive a message from a message queue.
 * Please refer to POSIX standard for details.
 */
ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio);

/** Receive a message from a message queue with timeout.
 * Please refer to POSIX standard for details.
 * @param abs_timeout [in] Only abs_timeout={0,0} is supported in this
 * implementation. This behavior may change in the future.
 */
ssize_t mq_timedreceive(mqd_t mqdes, char *restrict msg_ptr, size_t msg_len, unsigned int *restrict msg_prio, const struct timespec *restrict abs_timeout);

/** Get message queue attributes.
 * Please refer to POSIX standard for details.
 */
int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat);

/** Set message queue attributes.
 * Please refer to POSIX standard for details.
 */
int mq_setattr(mqd_t mqdes, const struct mq_attr *restrict mqstat, struct mq_attr *restrict omqstat);

/** @} */

#define NBBY    8U               /* number of bits in a byte */

/*
 * Select uses bit masks of file descriptors in longs.  These macros
 * manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here should
 * be enough for most uses.
 */
#ifndef FD_SETSIZE
#define FD_SETSIZE    256U
#endif

typedef unsigned long   fd_mask;
#define NFDBITS    (sizeof(fd_mask) * (unsigned int)NBBY)     /* bits per mask */

#ifndef howmany
#define howmany(x, y)    (((x) + ((y) - 1U)) / (y))
#endif

//equivalent of fd_set fpr WINNT env
typedef struct fd_set
{
    fd_mask fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

/** \addtogroup mqueue */
/** @{ */

/** Sets the bit for the file descriptor fd in the file descriptor set fdset.
 */
#define FD_SET(n, p) ((p)->fds_bits[((unsigned int) (n)) / NFDBITS] |= (1UL << (((unsigned int) (n)) % NFDBITS)))

/** Clears the bit for the file descriptor fd in the file descriptor set fdset.
 */
#define FD_CLR(n, p) ((p)->fds_bits[((unsigned int) (n)) / NFDBITS] &= ~(1UL << (((unsigned int) (n)) % NFDBITS)))

/** Returns a non-zero value if the bit for the file descriptor fd is set in the file descriptor set pointed to by fdset, and 0 otherwise.
 */
#define FD_ISSET(n, p)    ((unsigned long)(p)->fds_bits[((unsigned int) (n)) / NFDBITS] & (unsigned long)((unsigned)1U << (((unsigned int) (n)) % NFDBITS)))

/** Copies the file descriptor set.
 */
#define FD_COPY(f, t)     (void)(memcpy)((t), (f), sizeof(*(f)))

/** Initializes the file descriptor set fdset to have zero bits for all file descriptors.
 */
#define FD_ZERO(p)        (void)memset((p), 0, sizeof(*(p)))

/** Error check the file descriptor set.
 */
#define FD_BAD(fd)        ((fd) < 0 /*|| fd >= fd_arraylen || fd_array[fd].obj == 0*/)

/*! Wait for both message queues and signals. In this implementation, only
 * message queue file descriptors are supported.
 * @param nfds [in] This is an integer one more than the maximum of any file
 * descriptor in any of the sets. In other words, while you are busy
 * adding file descriptors to your sets, you must calculate the maximum
 * integer value of all of them, then increment this value by one, and
 * then pass this as nfds to select().
 * @param readfds  [in] the file descriptor set on all message queues.
 * @param writefds [in] ignored in this implementation.
 * @param errorfds [in] ignored in this implementation.
 * @param timeout  [in] Only timeout={0,0} is supported in this
 * implementation. This behavior may change in the future.
 */
int pselect(int nfds, fd_set *restrict readfds,
            fd_set *restrict writefds, fd_set *restrict errorfds,
            const struct timespec *restrict timeout,
            const sigset_t *restrict sigmask);

/*! Wait for multiple message queues. In this implementation, only
 * message queue file descriptors are supported.
 * @param nfds [in] This is an integer one more than the maximum of any file
 * descriptor in any of the sets. In other words, while you are busy
 * adding file descriptors to your sets, you must calculate the maximum
 * integer value of all of them, then increment this value by one, and
 * then pass this as nfds to select().
 * @param readfds  [in] the file descriptor set on all message queues.
 * @param writefds [in] ignored in this implementation.
 * @param errorfds [in] ignored in this implementation.
 * @param timeout  [in] Only timeout={0,0} is supported in this
 * implementation. This behavior may change in the future.
 */
int select(int nfds, fd_set *restrict readfds,
           fd_set *restrict writefds, fd_set *restrict errorfds,
           struct timeval *restrict timeout);

/** @} */

/* this function is needed for test framework which needs to clean up memory when teardown */
void _mq_teardown(void);

#ifdef __cplusplus
}
#endif

#endif
