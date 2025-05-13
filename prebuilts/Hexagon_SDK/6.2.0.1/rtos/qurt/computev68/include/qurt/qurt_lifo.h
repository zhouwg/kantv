#ifndef QURT_LIFO_H
#define QURT_LIFO_H
/**
  @file qurt_lifo.h

  @brief
  Provide lock free LastInFirstOut algorithm, which can be used in a
  variety of situations for allocation/free fixed size buffer    
  This implementation touches the first word of your FREED buffer. Even
  though it does not matter how you use it when it is allocated, you might want
	to be a bit careful not to put your MAGIC number as the first field.
	Because it will not hold the magic value for "freed"

 EXTERNALIZED FUNCTIONS
 None

 INITIALIZATION AND SEQUENCING REQUIREMENTS
 None

 Copyright (c) 2013, 2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

	/*=====================================================================
	 Functions
	======================================================================*/

/*======================================================================*/
/**
  Pops an element out of the LIFO. 

  @param[in] freelist  Pointer to the head of your list. 
	
  @return 				
  Top object from the list 
		 
  @dependencies
  None.
*/
/* ======================================================================*/
void * qurt_lifo_pop(void *freelist);

 
/*======================================================================*/
/**
  Pushes an element into the LIFO.
	
  @param[in] freelist  Pointer to the head of your list. 
  @param[in] buf       Pointer to your buffer to push into the list. 
	
  @return
  None.
		 
  @dependencies
  None.
*/
/* ======================================================================*/
void qurt_lifo_push(void *freelist, void *buf);

void qurt_lifo_remove(void *freelist, void *buf);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_LIFO_H */

