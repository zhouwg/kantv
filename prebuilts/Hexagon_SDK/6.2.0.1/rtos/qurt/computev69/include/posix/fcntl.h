#ifndef _FCNTL_H
#define _FCNTL_H

/*==========================================================================
 * FILE:         fcntl.h
 *
 * SERVICES:     POSIX fcntl.h
 *
 * DESCRIPTION:  The <fcntl.h> header is needed by the open() and fcntl()
 *               system calls, which have a variety of parameters and
 *               flags. They are described here.
 *
 *               The formats of the calls to each of these are:
 *
 *               open(path, oflag [,mode]) open a file
 *               fcntl(fd, cmd [,arg]) get or set file attributes
 *
 *               Copyright (c) 2013,2016  by Qualcomm Technologies, Inc.  All Rights Reserved. QUALCOMM Proprietary and Confidential.

 *==========================================================================*/

#include <sys/types.h>
#include <generic/fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Oflag values for open(). POSIX Table 6-4. */
#define POSIX_O_CREAT       0x100  /* creat file if it doesn't exist */
#define POSIX_O_EXCL        0x200  /* exclusive use flag */
#define POSIX_O_NOCTTY      0x400  /* do not assign a controlling terminal */
#define POSIX_O_TRUNC       0x1000 /* truncate flag */

/* File status flags for open() and fcntl(). POSIX Table 6-5. */
#define POSIX_O_APPEND      0x2000 /* set append mode */
#define POSIX_O_NONBLOCK    0x4000 /* no delay */

/* File access modes for open() and fcntl(). POSIX Table 6-6. */
#define POSIX_O_RDONLY      0 /* open(name, POSIX_O_RDONLY) opens read only */
#define POSIX_O_WRONLY      1 /* open(name, POSIX_O_WRONLY) opens write only */
#define POSIX_O_RDWR        2 /* open(name, POSIX_O_RDWR) opens read/write */

/* Mask for use with file access modes. POSIX Table 6-7. */
#define POSIX_O_ACCMODE     0x3 /* mask for file access modes */

#ifdef __cplusplus
}
#endif

#endif /* _FCNTL_H */
