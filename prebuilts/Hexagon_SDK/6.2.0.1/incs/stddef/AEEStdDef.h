#ifndef AEESTDDEF_H
#define AEESTDDEF_H
/*
=======================================================================

FILE:         AEEStdDef.h

DESCRIPTION:  definition of basic types, constants,
                 preprocessor macros

=======================================================================
*/
/*==============================================================================
  Copyright (c) 2005,2007,2012-2013, 2020 Qualcomm Technologies, Inc.
  All rights reserved.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its contributors
  may be used to endorse or promote products derived from this software without
  specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
==============================================================================*/

#include <stdint.h>

#if defined(COMDEF_H) /* guards against a known re-definer */
#define _BOOLEAN_DEFINED
#define _UINT32_DEFINED
#define _UINT16_DEFINED
#define _UINT8_DEFINED
#define _INT32_DEFINED
#define _INT16_DEFINED
#define _INT8_DEFINED
#define _UINT64_DEFINED
#define _INT64_DEFINED
#define _BYTE_DEFINED
#endif /* #if !defined(COMDEF_H) */

/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */

/* The following definitions are the same accross platforms.  This first
** group are the sanctioned types.
*/
/** @defgroup  stddef standard data type definitions
*  @{
*/
#ifndef _BOOLEAN_DEFINED
typedef  unsigned char      boolean;     /**<  Boolean value type. */
#define _BOOLEAN_DEFINED
#endif

#ifndef _UINT32_DEFINED
typedef  uint32_t           uint32;      /**<  Unsigned 32-bit value */
#define _UINT32_DEFINED
#endif

#ifndef _UINT16_DEFINED
typedef  unsigned short     uint16;      /**< Unsigned 16-bit value */
#define _UINT16_DEFINED
#endif

#ifndef _UINT8_DEFINED
typedef  unsigned char      uint8;       /**< Unsigned 8-bit value */
#define _UINT8_DEFINED
#endif

#ifndef _INT32_DEFINED
typedef  int32_t            int32;       /**< Signed 32-bit value */
#define _INT32_DEFINED
#endif

#ifndef _INT16_DEFINED
typedef  signed short       int16;       /**< Signed 16-bit value */
#define _INT16_DEFINED
#endif

#ifndef _INT8_DEFINED
typedef  signed char        int8;        /**< Signed 8-bit value */
#define _INT8_DEFINED
#endif

#ifndef _INT64_DEFINED
#if defined(__GNUC__)
#define __int64 long long
#endif
typedef  __int64            int64;       /**< Signed 64-bit value */
#define _INT64_DEFINED
#endif

#ifndef _UINT64_DEFINED
typedef  unsigned __int64   uint64;      /**< Unsigned 64-bit value */
#define _UINT64_DEFINED
#endif

#ifndef _BYTE_DEFINED
typedef  unsigned char      byte;        /**< byte type */
#define  _BYTE_DEFINED
#endif

/**
 * @}
 */

 /** @defgroup  stdret standard return values
*  @{
*/

//! @cond Doxygen_Suppress
#ifndef _AEEUID_DEFINED
typedef uint32             AEEUID;
#define _AEEUID_DEFINED
#endif

#ifndef _AEEIID_DEFINED
typedef uint32             AEEIID;
#define _AEEIID_DEFINED
#endif

#ifndef _AEECLSID_DEFINED
typedef uint32             AEECLSID;
#define _AEECLSID_DEFINED
#endif

#ifndef _AEEPRIVID_DEFINED
typedef uint32             AEEPRIVID;
#define _AEEPRIVID_DEFINED
#endif

#ifndef _AECHAR_DEFINED
typedef uint16             AECHAR;
#define _AECHAR_DEFINED
#endif
//! @endcond

/**
 * @brief Return value of functions indicating success or failure. return value 0 indicates success. A non zero value indicates a failure. Any data in rout parameters is not propagated back.
 */
#ifndef _AEERESULT_DEFINED
typedef int                AEEResult;
#define _AEERESULT_DEFINED
#endif

/**
 * @}
 */


/* -----------------------------------------------------------------------
** Function Calling Conventions
** ----------------------------------------------------------------------- */

#ifndef CDECL
#ifdef _MSC_VER
#define CDECL __cdecl
#else
#define CDECL
#endif /* _MSC_VER */
#endif /* CDECL */

/* -----------------------------------------------------------------------
** Constants
** ----------------------------------------------------------------------- */
 /** @defgroup  stdminmax Standard Min and Max for all data types
*  @{
*/

#ifndef TRUE
#define TRUE   1   /**< Boolean true value. */
#endif

#ifndef FALSE
#define FALSE  0   /**< Boolean false value. */
#endif

#ifndef NULL
#define NULL  0     /**< NULL = 0. */
#endif

#ifndef MIN_INT8
#define MIN_INT8 -128           /**< MIN 8-bit integer */
#endif
#ifndef MIN_INT16
#define MIN_INT16 -32768        /**< MIN 16-bit integer */
#endif
#ifndef MIN_INT32
#define MIN_INT32 (~0x7fffffff)   /**<  MIN 32-bit unsigned */
#endif
#ifndef MIN_INT64
#define MIN_INT64 (~0x7fffffffffffffffLL) /**< MIN 64-bit integer */
#endif

#ifndef MAX_INT8
#define MAX_INT8 127                /**< MAX 8-bit integer */
#endif
#ifndef MAX_INT16
#define MAX_INT16 32767             /**< MAX 16-bit integer */
#endif
#ifndef MAX_INT32
#define MAX_INT32 2147483647        /**< MAX 32-bit integer */
#endif
#ifndef MAX_INT64
#define MAX_INT64 9223372036854775807LL     /**< MAX 64-bit integer */
#endif

#ifndef MAX_UINT8
#define MAX_UINT8 255                   /**< MAX 8-bit unsigned integer */
#endif
#ifndef MAX_UINT16
#define MAX_UINT16 65535                /**< MAX 16-bit unsigned integer */
#endif
#ifndef MAX_UINT32
#define MAX_UINT32 4294967295u          /**< MAX 32-bit unsigned integer */
#endif
#ifndef MAX_UINT64
#define MAX_UINT64 18446744073709551615uLL      /**< MAX 64-bit unsigned integer */
#endif

//! @cond Doxygen_Suppress
#ifndef MIN_AECHAR
#define MIN_AECHAR 0
#endif

#ifndef MAX_AECHAR
#define MAX_AECHAR 65535
#endif

//! @endcond

/**
 * @}
 */

/* -----------------------------------------------------------------------
** Preprocessor helpers
** ----------------------------------------------------------------------- */
#define __STR__(x) #x
#define __TOSTR__(x) __STR__(x)
#define __FILE_LINE__ __FILE__ ":" __TOSTR__(__LINE__)

/* -----------------------------------------------------------------------
** Types for code generated from IDL
** ----------------------------------------------------------------------- */

 /** @defgroup  QIDL data types
*  @{
*/
//! @cond Doxygen_Suppress
#ifndef __QIDL_WCHAR_T_DEFINED__
#define __QIDL_WCHAR_T_DEFINED__
typedef uint16 _wchar_t;
#endif


/* __STRING_OBJECT__ will be deprecated in the future */


#if !defined(__QIDL_STRING_OBJECT_DEFINED__) && !defined(__STRING_OBJECT__)
#define __QIDL_STRING_OBJECT_DEFINED__
#define __STRING_OBJECT__

/**
 * @brief This structure is used to represent an IDL string when used inside a
   sequence or union.
 */
typedef struct _cstring_s {
   char* data;
   int dataLen;
   int dataLenReq;
} _cstring_t;

/**
 * @brief This structure is used to represent an IDL wstring when used inside a
   sequence or union.
 */

typedef struct _wstring_s {
   _wchar_t* data;
   int dataLen;
   int dataLenReq;
} _wstring_t;
#endif /* __QIDL_STRING_OBJECT_DEFINED__ */
//! @endcond
/**
 * @}
 */
/*
=======================================================================
  DATA STRUCTURES DOCUMENTATION
=======================================================================

=======================================================================

AEEUID

Description:
   This is a BREW unique ID.  Used to express unique types, interfaces, classes
     groups and privileges.  The BREW ClassID Generator generates
     unique IDs that can be used anywhere you need a new AEEIID, AEECLSID,
     or AEEPRIVID.

Definition:
    typedef uint32             AEEUID

=======================================================================

AEEIID

Description:
   This is an interface ID type, used to denote a BREW interface. It is a special case
     of AEEUID.

Definition:
    typedef uint32             AEEIID

=======================================================================

AEECLSID

Description:
   This is a classe ID type, used to denote a BREW class. It is a special case
     of AEEUID.

Definition:
    typedef uint32             AEECLSID

=======================================================================

AEEPRIVID

Description:
   This is a privilege ID type, used to express a privilege.  It is a special case
     of AEEUID.

Definition:
    typedef uint32             AEEPRIVID

=======================================================================

AECHAR

Description:
   This is a 16-bit character type.

Definition:
   typedef uint16             AECHAR

=======================================================================

AEEResult

Description:
   This is the standard result type.

Definition:
   typedef int                AEEResult

=======================================================================

_wchar_t

Description:
   This is a 16-bit character type corresponding to the IDL 'wchar'
   type.

Definition:
   typedef uint16             _wchar_t

See Also:
   _cstring_t
   _wstring_t

=======================================================================

_cstring_t

Description:
   This structure is used to represent an IDL string when used inside a
   sequence or union.

Definition:
   typedef struct _cstring_s {
      char* data;
      int dataLen;
      int dataLenReq;
   } _cstring_t;

Members:
   data       : A pointer to the NULL-terminated string.
   dataLen    : The size, in chars, of the buffer pointed to by 'data',
                including the NULL terminator.  This member is only used
                when the structure is part of an rout or inrout
                parameter, but must be supplied by the caller as an
                input in these cases.
   dataLenReq : The size that would have been required to store the
                entire result string.  This member is only used when the
                structure is part of an rout or inrout parameter, when
                it is an output value set by the callee.  The length of
                the returned string (including the NULL terminator)
                after a call is the minimum of dataLen and dataLenReq.

See Also:
   _wchar_t
   _wstring_t

=======================================================================

_wstring_t

Description:
   This structure is used to represent an IDL wstring when used inside a
   sequence or union.

Definition:
   typedef struct _wstring_s {
      _wchar_t* data;
      int dataLen;
      int dataLenReq;
   } _wstring_t;

Members:
   data       : A pointer to the NULL-terminated wide string.
   dataLen    : The size, in 16-bit characters, of the buffer pointed to
                by 'data', including the NULL terminator.  This member
                is only used when the structure is part of an rout or
                inrout parameter, but must be supplied by the caller as
                an input in these cases.
   dataLenReq : The number of 16-bit characters that would have been
                required to store the entire result string.  This member
                is only used when the structure is part of an rout or
                inrout parameter, when it is an output value set by the
                callee.  The length of the returned wstring (including
                the NULL terminator) after a call is the minimum of
                dataLen and dataLenReq.

See Also:
   _cstring_t
   _wchar_t

=======================================================================
*/

#endif /* #ifndef AEESTDDEF_H */

