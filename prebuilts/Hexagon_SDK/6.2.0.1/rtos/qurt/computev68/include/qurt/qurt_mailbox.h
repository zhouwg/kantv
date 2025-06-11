#ifndef QURT_MAILBOX_H
#define QURT_MAILBOX_H

/**
  @file qurt_mailbox.h
  @brief  Definitions, macros, and prototypes used for QuRT mailbox

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2015, 2021-2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/
/* Definitions on typedef and return values */

#define   QURT_MAILBOX_ID_NULL               0
#define   QURT_MAILBOX_ERROR                -1
#define   QURT_MAILBOX_ID_ERROR             -2
#define   QURT_MAILBOX_NON_VALID_DATA       -3
#define   QURT_MAILBOX_FULL                 -4
#define   QURT_MAILBOX_DELETED              -5
#define   QURT_MAILBOX_RECEIVE_HALTED       -6
#define   QURT_MAILBOX_BANDWIDTH_LIMIT      -7


/*=============================================================================
                    FORWARD DECLARATIONS & TYPEDEFS
=============================================================================*/

#define        QURT_MAILBOX_AT_QURTOS     0U            // Receiver is QurtOS
#define        QURT_MAILBOX_AT_ROOTPD     1U            // Receiver is RootPD  (ASID=0)
#define        QURT_MAILBOX_AT_USERPD     2U            // Receiver is User PD (ASID!=0)
#define        QURT_MAILBOX_AT_SECUREPD   3U            // Receiver is Secure PD

typedef unsigned char qurt_mailbox_receiver_cfg_t;  

#define        QURT_MAILBOX_SEND_OVERWRITE        0U       // When there is already valid content, overwrite it
#define        QURT_MAILBOX_SEND_NON_OVERWRITE    1U       // When there is already valid content, return failure

typedef unsigned char qurt_mailbox_send_option_t;  


#define        QURT_MAILBOX_RECV_WAITING          0U          // When there is no valid content, wait for it 
#define        QURT_MAILBOX_RECV_NON_WAITING      1U          // When there is no valid content, return failure immediately
#define        QURT_MAILBOX_RECV_PEEK_NON_WAITING 2U          // Read the content, but doesn't remove it from the mailbox. No waiting.

typedef unsigned char qurt_mailbox_recv_option_t;


/*=============================================================================
                            EXTERNS & FUNCTIONS
=============================================================================*/
/* Function prototype */

/**@ingroup qurt_mailbox_create
  Creates a QuRT mailbox.
   
  @param name            Mailbox name up to 8 characters.
  @param recv_opt        Configuration on the receiver process.

  @return
  Mailbox ID --          Mailbox Identifier \n
  #QURT_MAILBOX_ID_NULL --  NULL, failure at creating mailbox

  @dependencies
  None.
*/
unsigned long long qurt_mailbox_create(char *name, qurt_mailbox_receiver_cfg_t recv_opt);


/**@ingroup qurt_mailbox_get_id
  Gets a QuRT mailbox identifier.
   
  @param name            Mailbox name up to 8 characters.

  @return
  Mailbox ID --            Mailbox identifier \n
  #QURT_MAILBOX_ID_NULL -- NULL, failure at getting mailbox ID

  @dependencies
  None.
*/
unsigned long long qurt_mailbox_get_id(char *name);


/**@ingroup qurt_mailbox_send
  Sends data to a QuRT mailbox.
   
  @param mailbox_id   Mailbox identifier.
  @param send_opt     Option for mailbox send.
  @param data         Data to send.


  @return
  #QURT_EOK                      Success \n
  #QURT_MAILBOX_ID_ERROR         Mailbox ID error.\n
  #QURT_MAILBOX_ERROR            Other errors.\n
  #QURT_MAILBOX_FULL             Valid data already exists, non-overwriting.\n
  #QURT_MAILBOX_BANDWIDTH_LIMIT  Reached the bandwidth limitation.   

  @dependencies
  None.
*/
int qurt_mailbox_send(unsigned long long mailbox_id, qurt_mailbox_send_option_t send_opt, unsigned long long data);


/**@ingroup qurt_mailbox_receive
  Receive data from QuRT mailbox
   
  @param mailbox_id   Mailbox Identifier
  @param send_opt     Option for mailbox receiving
  @param data         Pointer to data buffer for receiving

  @return
  #QURT_EOK                            Success \n
  #QURT_MAILBOX_ID_ERROR               Mailbox ID error. \n
  #QURT_MAILBOX_ERROR                  Other errors. \n
  #QURT_MAILBOX_NON_VALID_DATA         No current valid data, put the previous content in the buffer. \n
  #QURT_MAILBOX_RECEIVE_HALTED         Receive halted, the waiting thread is woken up. \n
  #QURT_MAILBOX_DELETED                Mailbox is deleted, and the waiting thread is woken up.

  @dependencies
  None.
*/
int qurt_mailbox_receive(unsigned long long mailbox_id, qurt_mailbox_recv_option_t recv_opt, unsigned long long *data);


/**@ingroup qurt_mailbox_delete
  Deletes a QuRT mailbox.

  A mailbox can only be deleted from the process that created the mailbox.
   
  @param mailbox_id   Mailbox identifier.

  @return
  #QURT_EOK                   Success. \n
  #QURT_MAILBOX_ID_ERROR      Mailbox ID error. \n
  #QURT_MAILBOX_ERROR         Other errors.

  @dependencies
  None.
*/
int qurt_mailbox_delete(unsigned long long mailbox_id);


/**@ingroup qurt_mailbox_receive_halt
  Halts a QuRT mailbox receiving and wakes up waiting threads.

  @param mailbox_id   Mailbox identifier.

  @return
  #QURT_EOK                   Success. \n
  #QURT_MAILBOX_ID_ERROR      Mailbox ID error.\n
  #QURT_MAILBOX_ERROR         Other errors.

  @dependencies
  None.
*/
int qurt_mailbox_receive_halt(unsigned long long mailbox_id);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif // QURT_MAILBOX_H
