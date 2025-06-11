#ifndef QDI_H
#define QDI_H

/**
  @file qurt_qdi.h
  @brief Prototypes of QuRT Driver Invocation API functions      

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2013, 2021, 2023 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/


#include "qurt_qdi_constants.h"
#include "qurt_qdi_imacros.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@ingroup func_qurt_qdi_open
  Opens the specified driver for subsequent operations.
  qurt_qdi_open() is the primary mechanism by which a driver user can
  obtain a QDI handle. The user provides the name of the driver to the 
  qurt_qdi_open call, and gets back a handle referencing
  the named driver. \n
  @note1hang For reasons related to the Hexagon standard for varargs functions, the
             qurt_qdi_open function prototype is not actually defined as a varargs.


  @param[in] p   Driver name.
  @param[in] ... Up to nine additional device-specific arguments can be passed as parameters, 
                 and should follow the POSIX open() convention. \n
                 - flags -- Optional second parameter (POSIX flags), the handle 
                         access requested (read-only, write-only, or read-write,
                         for instance) and other flags such as whether the call 
                         should create a new device or only open an existing 
                         device.   \n
                 - mode  -- Optional third parameter (POSIX mode); permissions to
                         configure when a new device is created. @tablebulletend
 
  @return 
  Negative value -- Error. \n
  Non-negative value -- Success, this result value serves as a handle to the
                        opened driver.
  @dependencies
  None.
 */
// int qurt_qdi_open();
#define qurt_qdi_open(p,...) \
   qurt_qdi_handle_invoke(QDI_HANDLE_GENERIC,QDI_OPEN,(p),##__VA_ARGS__)

#define qurt_qdi_open_dt(p,q,...) \
   qurt_qdi_handle_invoke(QDI_HANDLE_GENERIC,QDI_OPEN_FROM_DT,(p),(q),##__VA_ARGS__)

/**@ingroup func_qurt_qdi_handle_invoke
  Performs a generic driver operation, which (depending on the specified operation) can be
  either be one of the predefined operations listed in @xhyperref{tbl:functionMapping,QDI function mapping} 
  or a driver-specific operation.
  The user provides a QDI handle and an integer
  method number, along with 0 to 8 optional 32-bit arguments.
  The device driver invocation function is invoked with the
  same method number and 0 to 8 optional arguments. The
  return value from the invocation function is passed back to
  the user as the return value of qurt_qdi_handle_invoke.

  @note1hang For reasons related to the Hexagon standard for varargs functions, the
             qurt_qdi_handle_invoke() function prototype is not actually defined as a
             varargs function (and would break if it were defined this way).
 
  @param[in]  h   Driver handle.
  @param[in]  m   Integer number for the operation to perform.
  @param[in]  ... Up to eight optional arguments can be passed to the device driver as operation-specific parameters: \n
               arg1 -- First parameter \n
               arg2 -- Second parameter  \n
               arg3 -- Third parameter  \n
               arg4 -- Fourth parameter  \n
               arg5 -- Fifth parameter  \n
               arg6 -- Sixth parameter  \n
               arg7 -- Seventh parameter  \n
               arg8 -- Eighth parameter 
 
  @return 
  Integer value defined by the device driver. \n
  -1 -- Error.

  @dependencies
  None.
 */
// int qurt_qdi_handle_invoke();
#define qurt_qdi_handle_invoke(h,m,...) \
   _QDMPASTE(_QDMHI,_QDMCNT(QDI_HANDLE_LOCAL_CLIENT,h,m,##__VA_ARGS__))(QDI_HANDLE_LOCAL_CLIENT,h,m,##__VA_ARGS__)
#define _QDMHI3(a,b,c) qurt_qdi_qhi3(0,b,c)
#define _QDMHI4(a,b,c,d) qurt_qdi_qhi4(0,b,c,(int)(d))
#define _QDMHI5(a,b,c,d,e) qurt_qdi_qhi5(0,b,c,(int)(d),(int)(e))
#define _QDMHI6(a,b,c,d,e,f) qurt_qdi_qhi6(0,b,c,(int)(d),(int)(e),(int)(f))
#define _QDMHI7(a,b,c,d,e,f,g) qurt_qdi_qhi7(8,b,c,(int)(d),(int)(e),(int)(f),(int)(g))
#define _QDMHI8(a,b,c,d,e,f,g,h) qurt_qdi_qhi8(8,b,c,(int)(d),(int)(e),(int)(f),(int)(g),(int)(h))
#define _QDMHI9(a,b,c,d,e,f,g,h,i) qurt_qdi_qhi9(16,b,c,(int)(d),(int)(e),(int)(f),(int)(g),(int)(h),(int)(i))
#define _QDMHI10(a,b,c,d,e,f,g,h,i,j) qurt_qdi_qhi10(16,b,c,(int)(d),(int)(e),(int)(f),(int)(g),(int)(h),(int)(i),(int)(j))
#define _QDMHI11(a,b,c,d,e,f,g,h,i,j,k) qurt_qdi_qhi11(24,b,c,(int)(d),(int)(e),(int)(f),(int)(g),(int)(h),(int)(i),(int)(j),(int)(k))
#define _QDMHI12(a,b,c,d,e,f,g,h,i,j,k,l) qurt_qdi_qhi12(24,b,c,(int)(d),(int)(e),(int)(f),(int)(g),(int)(h),(int)(i),(int)(j),(int)(k),(int)(l))
int qurt_qdi_qhi3(int,int,int);
int qurt_qdi_qhi4(int,int,int,int);
int qurt_qdi_qhi5(int,int,int,int,int);
int qurt_qdi_qhi6(int,int,int,int,int,int);
int qurt_qdi_qhi7(int,int,int,int,int,int,int);
int qurt_qdi_qhi8(int,int,int,int,int,int,int,int);
int qurt_qdi_qhi9(int,int,int,int,int,int,int,int,int);
int qurt_qdi_qhi10(int,int,int,int,int,int,int,int,int,int);
int qurt_qdi_qhi11(int,int,int,int,int,int,int,int,int,int,int);
int qurt_qdi_qhi12(int,int,int,int,int,int,int,int,int,int,int,int);

/**@ingroup func_qurt_qdi_write
  Writes data to the specified driver.
  A predefined invocation routine for drivers that
  support a POSIX-like write functionality.
  qqurt_qdi_write(handle, buf, len) is equivalent to
  qurt_qdi_handle_invoke(handle, QDI_WRITE, handle, buf, len);
 
  @param[in]  handle Driver handle.
  @param[in]  buf    Pointer to the memory address where the data to write is stored.
  @param[in]  len    Number of bytes of data to write.

  @return 
  Non-negative integer -- Number of bytes written. \n
  Negative error code -- Write could not take place.

  @dependencies
  None.
 */
int qurt_qdi_write(int handle, const void *buf, unsigned len);

/**@ingroup func_qurt_qdi_read
  User-visible API to read data from a QDI handle. 
  A predefined invocation routine for drivers that
  support a POSIX-like read functionality.
  qurt_qdi_read(handle, buf, len) is equivalent to:
  qurt_qdi_handle_invoke(handle, QDI_READ, handle, buf, len);
 
  @param[in]  handle   Driver handle.
  @param[in]  buf      Pointer to the memory address where the data read is stored.
  @param[in]  len      Number of bytes of data to read.

  @return 
  Non-negative integer number -- Bytes read. \n
  Negative error code -- Read could not take place.

  @dependencies
  None.
 */
int qurt_qdi_read(int handle, void *buf, unsigned len);

/**@ingroup func_qurt_qdi_close
  Closes the specified driver, releasing any resources associated with the open driver.
  User-visible API to close a QDI handle.
 
  This API should be called when the user is done using a
  QDI-based handle. When this function is called, the driver can release
  any resources held and perform other necessary cleanup
  operations. qurt_qdi_close(handle) is equivalent to
  qurt_qdi_handle_invoke(handle, QDI_CLOSE, handle)
 
  @param[in]  handle Driver handle.
 
  @return 
  0 -- Success.\n
  Negative error code -- Failure.

  @dependencies
  None.
 */
int qurt_qdi_close(int handle);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
