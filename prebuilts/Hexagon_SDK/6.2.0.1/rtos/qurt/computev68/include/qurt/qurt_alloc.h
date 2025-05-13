#ifndef QURT_ALLOC_H
#define QURT_ALLOC_H

/**
  @file qurt_alloc.h 
  @brief Prototypes of kernel memory allocation API functions.      

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2021, 2023 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

/*======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**@ingroup func_qurt_malloc
  Dynamically allocates the specified array on the QuRT system heap.
  The return value is the address of the allocated memory area.

  @note1hang The allocated memory area is automatically initialized to zero.

  @param[in] size     Size (in bytes) of the memory area.
  
  @return
  Nonzero -- Pointer to the allocated memory area. \n
  0 -- Not enough memory in heap to allocate memory area.

  @dependencies
  None.    

 */
/* ======================================================================*/
void *qurt_malloc( unsigned int size);

/*======================================================================*/
/**@ingroup func_qurt_calloc
  Dynamically allocates the specified array on the QuRT system heap.
  The return value is the address of the allocated array. 

  @note1hang The allocated memory area is automatically initialized to zero.

  @param[in] elsize Size (in bytes) of each array element.
  @param[in] num    Number of array elements.

  @return 
  Nonzero -- Pointer to allocated array.\n
  Zero -- Not enough memory in heap to allocate array.

  @dependencies
  None.
  
 */
 /* ======================================================================*/
void *qurt_calloc(unsigned int elsize, unsigned int num);

/*======================================================================*/
/**@ingroup func_qurt_realloc
  Reallocates memory on the heap. \n
  Changes the size of a memory area that is already allocated on the QuRT system heap. 
  The reallocate memory operation is functionally similar to realloc. It accepts a pointer
  to an existing memory area on the heap, and resizes the memory area to the specified size
  while preserving the original contents of the memory area.

  @note1hang This function might change the address of the memory area.
             If the value of ptr is NULL, this function is equivalent to 
             qurt_malloc().
             If the value of new_size is 0, it is equivalent to qurt_free().  
             If the memory area is expanded, the added memory is not initialized.

  @param[in] *ptr   Pointer to the address of the memory area.
  @param[in] newsize Size (in bytes) of the reallocated memory area.
	               	
  @return
  Nonzero -- Pointer to reallocated memory area. \n
  0 -- Not enough memory in heap to reallocate the memory area.

  @dependencies
  None.
	 
 */
 /* ======================================================================*/
void *qurt_realloc(void *ptr,  int newsize);

/*======================================================================*/
/**@ingroup func_qurt_free
  Frees allocated memory from the heap.\n
  Deallocates the specified memory from the QuRT system heap.

  @param[in] *ptr Pointer to the address of the memory to deallocate.
	
  @return
  None.

  @dependencies
  The memory item that the ptr value specifies must have been previously 
  allocated using one of the qurt_calloc(), 
  qurt_malloc(), or qurt_realloc() memory allocation functions. 
  Otherwise the behavior of QuRT is undefined.
  
 */
 /* ======================================================================*/
void qurt_free( void *ptr);


void *qurt_memalign(unsigned int alignment, unsigned int size);

/*
||  Macro to define a static heap for a QuRT program.
||
||  Usage:
||   Declare at the top-level of any C source file that
||    is part of the build (and is guaranteed
||    to actually be pulled into the build). Place
||    it in the same function with main():
||
||    QURT_DECLARE_STATIC_HEAP(512000);
||
||  The only argument is the size in bytes, and it is
||   rounded up to the nearest 64 bytes (size of an
||   L2 cache block).
||
*/

#define QURT_DECLARE_STATIC_HEAP(sz)                    \
   static struct qurt_static_heap {                     \
      char space[(sz)] __attribute__((aligned(64)));      \
   } static_heap[1];                                    \
   void * const override_heap_Base = &static_heap[0];   \
   void * const override_heap_Limit = &static_heap[1]

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_ALLOC_H */

