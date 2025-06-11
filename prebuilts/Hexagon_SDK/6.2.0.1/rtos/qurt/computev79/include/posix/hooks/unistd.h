#ifndef UNISTD_H
#define UNISTD_H
/**
  @file posix/hooks/unistd.h
  @brief POSIX related declarations in <unistd.h> that are missing in toolchain 
         header

EXTERNAL FUNCTIONS    
   None 

INITIALIZATION AND SEQUENCING REQUIREMENTS    
   DONT include this header directly! Instead include unistd.h. 

Copyright (c) 2018, 2023 by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.    
    
==============================================================================*/
#include <types.h> /* For various POSIX ID types from toolchain headers */

#ifdef __cplusplus
extern "C" {
#endif
extern long pathconf (char const * path, int name);

/* Process*/

/** The getppid() function shall return the parent process ID of the calling process.
 * Please refer to POSIX standard for details.
 * @param thread    [in]  none
 * @param value_ptr [out] the  parent process ID
 */
pid_t getppid(void);

/** The getpgid() function shall return the process group ID of the process whose process ID is equal to pid
 * Please refer to POSIX standard for details.
 * @param thread    [in]  process ID
 * @param value_ptr [out] process group ID
 */
pid_t getpgid(pid_t pid);

/** The getpgrp() function shall return the process group ID of the calling process
 * Please refer to POSIX standard for details.
 * @param thread    [in]  none
 * @param value_ptr [out] process group ID of the calling process
 */
pid_t getpgrp(void);

/**The getuid() function shall return the real user ID of the calling process.
 * Please refer to POSIX standard for details.
 * @param thread    [in]  none
 * @param value_ptr [out] the real user ID of the calling process.
 */
uid_t getuid(void); 

/** The geteuid() function shall return the effective user ID of the calling process
 * Please refer to POSIX standard for details.
 * @param thread    [in]  none
 * @param value_ptr [out] effective user ID of the calling process
 */
uid_t geteuid(void); 

/** The getegid() function shall return the effective group ID of the calling process.
 * Please refer to POSIX standard for details.
 * @param thread    [in]  none
 * @param value_ptr [out] effective group ID of the calling process.
 */
gid_t getegid(void);

/** The getgid() function shall return the real group ID of the calling process
 * Please refer to POSIX standard for details.
 * @param thread    [in]  none
 * @param value_ptr [out] real group ID of the calling process.
 */
 gid_t getgid(void); 

/** seteuid set effective user ID
 * Please refer to POSIX standard for details.
 * @param thread    [in] effective user ID
 * @param value_ptr [out] Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
 */
int seteuid(uid_t uid);

/** setpgrp - set the process group ID
 * Please refer to POSIX standard for details.
 * @param thread    [in] none
 * @param value_ptr [out] Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
 */ 
pid_t setpgrp(void);

/** setuid - set user ID
 * Please refer to POSIX standard for details.
 * @param thread    [in]  user ID
 * @param value_ptr [out] Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
 */
int setuid(uid_t uid);

/** setpgid - set process group ID for job control
 * Please refer to POSIX standard for details.
 * @param thread    [in] PID of process, PGID to be set
 * @param value_ptr [out] Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
 */
int setpgid(pid_t pid, pid_t pgid);

/** setsid - create session and set process group ID
 * Please refer to POSIX standard for details.
 * @param thread    [in] none
 * @param value_ptr [out] Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
 */
pid_t setsid(void);

#ifdef __cplusplus
}
#endif
#endif

