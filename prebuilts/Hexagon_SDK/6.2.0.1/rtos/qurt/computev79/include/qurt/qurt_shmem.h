#ifndef QURT_SHMEM_H
#define QURT_SHMEM_H

/**
  @file qurt_shmem.h

  @brief
  Prototypes of QuRT inter-process shared memory APIs

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2013, 2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MODE_T
#define MODE_T
typedef unsigned int mode_t;
#endif //MODE_T

/**
 * The shm_open() function establishes a connection between a shared memory object and a file descriptor.
 * The file descriptor is used by other functions such as mmap() to refer to that shared memory object.
 * 
 *
 * @param name      Pointer to string naming a shared memory object. Name has to start with "/shm/"
 * @param oflag     File status flags and file access modes of the open file description. Following
 *                  flags are defined in <fcntl.h> and supported:
 *                  O_RDONLY: oepn for read access only
 *                  O_RDWR: Open for read or write access
 *                  O_CREAT: If shared memory object doesn't exist, create one.
 * @param mode      Permission flags (currently ignored)
 *
 * @return    file descriptor (positive number) if operation successful.
 *                  negative error code if failed
 *
*/

int shm_open(const char * name, int oflag, mode_t mode);

/**
 * The shm_mmap() function create a shared memory mapping in the virtual address space of the
 * the calling process. 
 * 
 * @param addr      The starting address for the new mapping is specified in addr.
 * @param len       Specifies the lengh of the shared memory region.
 * @param prot      Describes the desired memory protection of the mapping. Same as the one in mmap of POSIX.
 * @param flags     Determines whether updates to the mapping is visible or not to other process. Same as
 *                  the one in mmap of POSIX.
 * @param fd        The starting adddress for the new mapping is returned.
 * @param offset    unused.
 *
 * @return    The starting adddress for the new mapping is returned.
 *                  negative error code if failed
 *
*/

void *shm_mmap(void *addr, unsigned int len, int prot, int flags, int fd, unsigned int offset);

/**
 * The shm_close() function removes a connection between a shared memory object and a file descriptor.
 * If there is no file descriptor connects to the shared memory object, the shared memory object will
 * be deleted automatically. Shared memory object has same virtual address in any process. This is 
 * restriction of single virtual address space. 
 * 
 *
 * @param fd        File descriptor of shared memory object
 *
 * @return    0 if operation successful.
 *                  negative error code if failed
 *
*/


int shm_close(int fd);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
