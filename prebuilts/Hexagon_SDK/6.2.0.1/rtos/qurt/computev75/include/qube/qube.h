#ifndef QUBE_H
#define QUBE_H
/*=============================================================================

                 qube.h -- H E A D E R  F I L E

GENERAL DESCRIPTION
   Prototypes of qpd API

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

      Copyright (c) 2013  by Qualcomm Technologies, Inc.  All Rights Reserved.

=============================================================================*/



#ifdef __cplusplus
extern "C" {
#endif

#include <qurt.h>

/* Define Error codes as QuRT error codes preceed with QURT_ */
#ifndef EOK
#define EOK                             QURT_EOK
#endif /* EOK */
#ifndef EVAL
#define EVAL                            QURT_EVAL
#endif /* EVAL */
#ifndef EMEM
#define EMEM                            QURT_EMEM
#endif /* EMEM */
#ifndef EINVALID
#define EINVALID                        QURT_EINVALID
#endif /* EINVALID */


/*=============================================================================
                      FUNCTION DECLARATIONS                                
=============================================================================*/

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QUBE_H */
