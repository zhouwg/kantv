#ifndef ADSP_MMAP_H
#define ADSP_MMAP_H

#ifdef __cplusplus
extern "C" {
#endif
#include "AEEStdDef.h"
/**
 * @param buf, the buffer virtual address
 * @param bufLen, the length
 * @param flags, the flags it was mapped with, 0 by default
 */
int adsp_addref_mmap(void* buf, int bufLen, uint32 flags);

/**
 * @param buf, the buffer virtual address
 * @param bufLen, the length
 */
int adsp_release_mmap(void* buf, int bufLen);


#ifdef __cplusplus
}
#endif
#endif// ADSP_MMAP_H
