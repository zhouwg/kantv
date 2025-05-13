#ifndef QURT_ISLAND_H
#define QURT_ISLAND_H

/**
  @file qurt_island.h
  @brief  Prototypes of power API
          The APIs allow entering and exiting island mode where the memory
          accesses are limited to local memory.

  EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2018-2021,2023  by Qualcomm Technologies, Inc.  All Rights Reserved.

=============================================================================*/

#include <qurt_thread.h>
#include <qurt_memory.h>
#include <qurt_alloc.h>
#include <qurt_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@ingroup func_qurt_island_get_status
  Gets Island mode status.

  Returns a value that indicates whether the QuRT system executes in Island mode.

  @return
  0 - Normal mode. \n
  1 - Island mode. 

  @dependencies
  None.
*/
unsigned int qurt_island_get_status (void);

/**@ingroup func_qurt_island_get_status2
  Gets Island mode status especially that differentiates between island partial exit and complete exit.
 
  Returns a value that indicates the current state. 
  
  @note1hang Transition from NORMAL mode to ISLAND mode happens in single
             threaded mode. Whereas transition from ISLAND mode to other modes
             happen in multi-threaded mode. So, a thread that gets island mode
             status as NORMAL can assume the same status till it continues to
             run. A thread that gets island mode status as ISLAND should 
             assume that the status may change to EXITING or NORMAL while it
             runs. A thread that gets island mode status as EXITING should
             assume that the status may change to NORMAL while it runs. If 
             the thread goes to wait state in after reading the status, it should get
             the island mode state again and not assume the previous state. 
  @note2hang This api returns more intrinsic states than qurt_island_get_status,
             when qurt_island_get_status returns 0, this api could return 
             QURT_ISLAND_MODE_EXITING or QURT_ISLAND_MODE_ISLAND
          
  @param[in/out] data  field is reserved for future use. If NULL pointer is passed, 
                       the field will be ignored. If a valid pointer is passed, 
                  QuRT will return back a bitmask which can be interpreted as follows:
                  data[31] - Valid bit. Set to 1 to indicate data[30:0] are valid. 
                  Otherwise set to 0.
                  data[30:0] – Reserved for future definition. 
 
  @return
    QURT_ISLAND_MODE_NORMAL   - Main mode \n
    QURT_ISLAND_MODE_ISLAND   - Island mode \n
    QURT_ISLAND_MODE_EXITING  - Exiting Island mode \n
 
  @dependencies
  None.
*/
unsigned int qurt_island_get_status2 (unsigned int *data);



/**@ingroup func_qurt_island_get_exit_status
  Gets the reason for the last Island mode exit status.

  @param[out] cause_code Pointer that returns the cause code of the last
                         island exit reason. \n
                         - #QURT_EISLANDUSEREXIT -- Island exit due to user call for island exit.\n
                         - #QURT_ENOISLANDENTRY -- API called before exiting island. \n                
                         - #QURT_EISLANDINVALIDINT -- Island exit due to an invalid interrupt in Island mode. @tablebulletend

  @param[out] int_num Pointer that holds the invalid interrupt number that caused
                      island exit when the cause code is #QURT_EISLANDINVALIDINT.
                      For other cases, it is -1.

  @return
  None. 

  @dependencies
  None.
*/
void qurt_island_get_exit_status(unsigned int *cause_code, int *int_num);

/**@ingroup func_qurt_island_get_enter_timestamp
  Gets the recent timestamp when the system exits STM during island enter.

  @param[out]    island_enter_timestamp Returns a pointer to the recent timestamp
                                        recorded after the system exits STM during island enter. If the system never 
                                        attempts to enter island, the island_enter_timestamp return pointer holds a value 
                 of zero.
  
  @return
  None. 

  @dependencies
  None.
*/
void qurt_island_get_enter_timestamp(unsigned long long *island_enter_timestamp);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_ISLAND_H */
