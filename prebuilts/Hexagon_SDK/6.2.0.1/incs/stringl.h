/*
 *  $Header: //components/rel/core.qdsp6/8.2/api/common/kernel/libstd/stringl/stringl.h#1 $ 
 *  $DateTime: 2023/05/10 09:48:16 $ 
 */

/*	$OpenBSD: string.h,v 1.17 2006/01/06 18:53:04 millert Exp $	*/
/*	$NetBSD: string.h,v 1.6 1994/10/26 00:56:30 cgd Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)string.h	5.10 (Berkeley) 3/9/91
 */

#ifndef _STRINGL_H_
#define	_STRINGL_H_

#include <stdio.h>
#include <string.h>
#include <wchar.h>

/** @defgroup error_codes Error Codes
 *  @{
 */
//
// AEEstd.h header error codes
//
#ifndef STD_NODIGITS
    #define STD_NODIGITS   1    /**< See std_scanul(). */
#endif

#ifndef STD_NEGATIVE
    #define STD_NEGATIVE   2    /**< See std_scanul(). */
#endif

#ifndef STD_OVERFLOW
    #define STD_OVERFLOW   3    /**< See std_scanul(). */
#endif

#ifndef STD_BADPARAM
    #define STD_BADPARAM   4    /**< See std_scanul(). */
#endif

/**
 * @}
 */

/**< UTF-16 2-byte wide char type */
typedef unsigned short wchar;

#ifdef __cplusplus
namespace std 
{
    extern "C"
    {
#endif //__cplusplus

/**
Added these macros for supporting compilation on Win based
software dev environments like VC, .Net etc.
*/
#ifdef _WIN32
   #define snprintf     _snprintf
   #define vsnprintf    _vsnprintf
#endif

/** @defgroup str_apis String Operation APIs
 *  @{
 */
 
/**
  strlcat - Size bounded string concatenation.
 
  Concatenates the source string to destination string.
 
  This function ensures that the destination string will
  not be improperly terminated and that there will be
  no concatenation beyond the size of the destination buffer.
          
  @param[in,out]  dst   Destination buffer.
  @param[in]      src   Source string.
  @param[in]      siz   Size of the destination buffer in bytes.
   
  @return 
  The length of the string that was attempted to be created,
  i.e. the sum of the source and destination strings.
 
  @dependencies 
  None.        
*/
size_t strlcat(char *dst, const char *src, size_t siz);

/**
 * @}
 */
 
 /** @defgroup wstr_apis Wide Char String Operation APIs
 *  @{
 */
/**
  wcslcat - Size bounded wide string concatenation using 
  C standard wide character data type wchar_t.
 
  Concatenates the source string to destination string.
 
  This function ensures that the destination string will
  not be improperly terminated and that there will be
  no concatenation beyond the size of the destination buffer.
          
  @param[in,out]  dst   Destination buffer.
  @param[in]      src   Source string.
  @param[in]      siz   Size of the destination buffer in units of wchar_t.
   
  @return 
  The length of the string that was attempted to be created,
  i.e. the sum of the source and destination strings.

  @note It has been observed that wchar_t on some platforms is
  2 bytes wide (UTF-16) and on others is 4 bytes wide (UTF-32). 
  So carefully consider this when using the data type wchar_t
  and this API in your application.

  @dependencies 
  None.        
*/

size_t wcslcat(wchar_t *dst, const wchar_t *src, size_t siz);

/**
  wstrlcat - Size bounded wide string concatenation using 2 byte
  wide (UTF-16) character data type wchar.
 
  Concatenates the source string to destination string.
 
  This function ensures that the destination string will
  not be improperly terminated and that there will be
  no concatenation beyond the size of the destination buffer.
          
  @param[in,out]  dst   Destination buffer.
  @param[in]      src   Source string.
  @param[in]      siz   Size of the destination buffer in units of wchar.
   
  @return 
  The length of the string that was attempted to be created,
  i.e. the sum of the source and destination strings.
 
  @dependencies 
  None.        
*/
size_t wstrlcat(wchar* dst, const wchar* src, size_t siz);

/**
 * @}
 */
 
 /** @addtogroup str_apis
  @{ */
    
/**
  strlcpy - Size bounded string copy. 
   
  Copies the source string to the destination buffer.
   
  This function ensures that the destination buffer will always 
  be NULL terminated and that there will not be a copy beyond 
  the size of the destination buffer. 
   
  @param[out] dst   Destination buffer.
  @param[in]  src   Source String.
  @param[in]  siz   Size of the destination buffer in bytes.
   
  @return 
  The length of the source string. 
   
  @dependencies 
  None. 
*/    
size_t strlcpy(char *dst, const char *src, size_t siz);

/** @}  */

 /** @addtogroup wstr_apis
  @{ */
/**
  wcslcpy - Size bounded wide string copy using
  C standard wide character data type wchar_t.

  Copies the source string to the destination buffer. 
   
  This function ensures that the destination buffer will always 
  be NULL terminated and that there will not be a copy beyond 
  the size of the destination buffer. 
   
  @param[out] dst   Destination buffer.
  @param[in]  src   Source String.
  @param[in]  siz   Size of the destination buffer in units of wchar_t.
   
  @return 
  The length of the source string. 

  @note It has been observed that wchar_t on some platforms is
  2 bytes wide (UTF-16) and on others is 4 bytes wide (UTF-32). 
  So carefully consider this when using the data type wchar_t
  and this API in your application.
   
  @dependencies 
  None. 
*/

size_t wcslcpy(wchar_t *dst, const wchar_t *src, size_t siz);

/**
  wstrlcpy - Size bounded wide string copy using 2 byte
  wide (UTF-16) character data type wchar.
   
  Copies the source string to the destination buffer. 
   
  This function ensures that the destination buffer will always 
  be NULL terminated and that there will not be a copy beyond 
  the size of the destination buffer. 
   
  @param[out] dst   Destination buffer.
  @param[in]  src   Source String.
  @param[in]  siz   Size of the destination buffer in units of wchar.
   
  @return 
  The length of the source string. 
   
  @dependencies 
  None. 
*/
size_t wstrlcpy(wchar* dst, const wchar* src, size_t siz);

/**
  wstrlen - Returns the number of characters in the source string.
  Used for strings based on wchar data type i.e. 2 byte wide (UTF-16)
  characters.
      
  @param[in]  src   Source String.
   
  @return 
  The number of characters in the source string.
   
  @dependencies 
  None. 
*/
size_t wstrlen(const wchar *src);

/**
  wstrcmp - Compares wchar (UTF-16) string s1 to the wchar string s2.

  This function starts comparing the first character of each string. 
  If they are equal to each other, it continues with the following 
  pairs until the characters differ or until a terminating 
  null-character is reached.

  @param[in]  s1   String to be compared.
  @param[in]  s2   String to be compared against.

  @return 
  0  - Indicates that the strings are equal.
  >0 - Indicates that the strings are not equal and a character in s1 is 
       greater than the corresponding character in s2.
  <0 - Indicates that the strings are not equal and a character in s1 is 
       lesser than the corresponding character in s2.
   
  @dependencies 
  None. 
*/
int wstrcmp(const wchar *s1, const wchar *s2);

/**
  wstrncmp - Compares upto n wchar (UTF-16) characters in string s1 
  to the wchar string s2.

  This function starts comparing the first character of each string. 
  If they are equal to each other, it continues with the following 
  pairs until the characters differ or until a terminating 
  null-character is reached or n comparisons have been performed.

  @param[in]  s1   String to be compared.
  @param[in]  s2   String to be compared against.
  @param[in]  n    Nmber of character to be compared.

  @return 
  0  - Indicates that the strings are equal.
  >0 - Indicates that the strings are not equal and a character in s1 is 
       greater than the corresponding character in s2.
  <0 - Indicates that the strings are not equal and a character in s1 is 
       lesser than the corresponding character in s2.
   
  @dependencies 
  None. 
*/
int wstrncmp(const wchar *s1, const wchar *s2, size_t n);

/** @}  */

 /** @addtogroup str_apis
  @{ */
 
/**
  strcasecmp - compare two strings ignoring case.
   
  @param[in] s1 First string.
  @param[in] s2 Second string.
   
  @return 
  The strcasecmp() and strncasecmp() functions return an integer 
  less than, equal to, or greater than zero if s1 (or the first 
  n bytes  thereof)  is found, respectively, to be less than, to
  match, or be greater than s2. 
   
  @dependencies 
  None. 
*/
int strcasecmp(const char * s1, const char * s2);

/**
  strncasecmp - compare two strings ignoring case (sized).
   
  @param[in] s1 First string.
  @param[in] s2 Second string.
  @param[in] n  The number of characters to compare (from the
                beginning).
   
  @return 
  The strcasecmp() and strncasecmp() functions return an integer 
  less than, equal to, or greater than zero if s1 (or the first 
  n bytes  thereof)  is found, respectively, to be less than, to
  match, or be greater than s2. 
   
  @dependencies 
  None. 
*/
int strncasecmp(const char * s1, const char * s2, size_t n);

/**
std_scanul()

Description:

  The std_scanul() converts an ASCII representation of a number
  to an unsigned long.  It expects strings that match the
  following pattern:

       spaces [+|-] digits

 
  'Spaces' is zero or more ASCII space or tab characters.
 
  'Digits' is any number of digits valid in the radix.  Letters
  'A' through 'Z' are treated as digits with values 10 through
  35. 'Digits' may begin with "0x" when a radix of 0 or 16 is
  specified.

  Upper and lower case letters can be used interchangeably.
 
  @param[in]  pchBuf    The start of the string to scan.

  @param[in]  nRadix    The numeric radix (or base) of the
                        number. Valid values are 2 through 36 or zero,
                        which implies auto-detection. Auto-detection
                        examines the digits field.  If it begins with
                        "0x", radix 16 is selected.  Otherwise, if it
                        begins with "0" radix 8 is selected.
                        Otherwise, radix 10 is selected.

  @param[out] ppchEnd   If ppchEnd is not NULL, *ppchEnd
                        points to the first character that did not
                        match the expected pattern shown above,
                        except on STD_BADPARAM and STD_OVERFLOW when
                        it is set to the start of the string.

  @param[out] pnError   If pnError is not NULL, *pnError
                        holds the error code, which is one of the
                        following:
 
        0            : Numeric value is from 0 to
                       MAX_UINT32.
        
        STD_NEGATIVE : The scanned value was negative and its absolute value was
                       from 1 to MAX_UINT32.  The result is the negated value
                       (cast to a uint32).
        
        STD_NODIGITS : No digits were found.  The result is zero.
        
        STD_OVERFLOW : The absolute value exceeded MAX_UINT32.  The result
                       is set to MAX_UINT32 and *ppchEnd is set to pchBuf.
    
        STD_BADPARAM : An improper value for nRadix was received.  The result
                       is set to zero, and *ppchEnd is set to pchBuf.
 
  @return
  The converted numeric result.
 
  @dependencies
  None.
 
*/
unsigned int std_scanul(const char * pchBuf, int nRadix, const char ** ppchEnd, int *pnError);

/** @}  */

/** @defgroup mem_ops Memory Operation APIs
 *  @{
 */
 
/**
  memscpy - Size bounded memory copy. 
   
  Copies bytes from the source buffer to the destination buffer.
   
  This function ensures that there will not be a copy beyond 
  the size of the destination buffer. 

  The result of calling this on overlapping source and destination
  buffers is undefined.
   
  @param[out] dst       Destination buffer.
  @param[in]  dst_size  Size of the destination buffer in bytes.
  @param[in]  src       Source buffer.
  @param[in]  src_size  Number of bytes to copy from source buffer.
   
  @return 
  The number of bytes copied to the destination buffer.  It is the
  caller's responsibility to check for trunction if it cares about it -
  truncation has occurred if the return value is less than src_size.
   
  @dependencies 
  None. 
*/    

size_t memscpy(void *dst, size_t dst_size, const void *src, size_t src_size);

/**
  memscpy_i - Inline function for size bounded memory copy.

  @see memscpy()
*/

static __inline size_t memscpy_i
(
  void *dst,
  size_t dst_size,
  const void *src,
  size_t src_size
)
{
  size_t  copy_size = (dst_size <= src_size)? dst_size : src_size;

  memcpy(dst, src, copy_size);

  return copy_size;
}

/**
  memsmove - Size bounded memory move.
   
  Moves bytes from the source buffer to the destination buffer.
   
  This function ensures that there will not be a copy beyond 
  the size of the destination buffer. 

  This function should be used in preference to memscpy() if there
  is the possiblity of source and destination buffers overlapping.
  The result of the operation is defined to be as if the copy were from
  the source to a temporary buffer that overlaps neither source nor
  destination, followed by a copy from that temporary buffer to the
  destination.
   
  @param[out] dst       Destination buffer.
  @param[in]  dst_size  Size of the destination buffer in bytes.
  @param[in]  src       Source buffer.
  @param[in]  src_size  Number of bytes to copy from source buffer.
   
  @return 
  The number of bytes copied to the destination buffer.  It is the
  caller's responsibility to check for trunction if it cares about it -
  truncation has occurred if the return value is less than src_size.
   
  @dependencies 
  None. 
*/    

size_t memsmove(void *dst, size_t dst_size, const void *src, size_t src_size);

/**
  memsmove_i - Inline function for size bounded memory move.

  @see memsmove()
*/

static __inline size_t memsmove_i
(
  void *dst,
  size_t dst_size,
  const void *src,
  size_t src_size
)
{
  size_t  copy_size = (dst_size <= src_size)? dst_size : src_size;

  memmove(dst, src, copy_size);

  return copy_size;
}

/**
  secure_memset - Memset functionality that won't be optimized by the compiler

  Memsets a memory location to a given value in a way that is unlikely to be
  removed by the compiler

  A classic compiler optimization is to remove references to instructions that
  assign a value to a variable where that variable is never used after the
  assignment.  However, this means that compilers will often remove memset 
  instructions which are used to "zero" sensitive information in stack or heap
  memory, which can cause a security risk.  This function performs a basic
  memset operation, but should always be instantiated in its own file, this
  will mean file optimizers will not be able to optimize its use and linkers
  do not have sufficient intelligence to optimize calls between files.

  This function should be used when clearing sensitive information in memory.

  @param[in]  ptr    Points to the memory area to be set.
  @param[in]  value  The value to be set.
  @param[in]  len    The number of bytes to be set.

  @return
  This function returns the pointer to the memory area ptr.

  @dependencies
  None.
*/

void* secure_memset(void* ptr, int value, size_t len);

/**
 * @}
 */


/** @defgroup time_safe_ops Time Safe Memory Operation APIs
 *  @{
 */
 
/**
  timesafe_memcmp - Constant-time memory comparison

  Compares bytes at two different sections in memory in constant time

  This function compares the len bytes starting at ptr1 with the len
  bytes starting at ptr2.  The function returns 1 if the two sections
  of memory are different and 0 if the two sections of memory are
  identical.  The function always scans the entire range of memory to
  ensure the function runs in constant time.

  This function should be used when comparing confidential information
  in memory as it prevents timing attacks.  A traditional memcmp() exits
  after finding non-equal bytes and this can be used to determine the value
  of confidential data in memory.  Examples uses include password checks,
  MACs checks, decryption checks, and checks on private user information.

  @param[in]  ptr1   Points to the first memory bytes to be compared.
  @param[in]  ptr2   Points to the second memory bytes to be compared.
  @param[in]  len    The number of bytes to be compared.

  @return
  This function returns 1 if the two buffers are different and
  0 if the two buffers are identical.

  @dependencies
  None.
*/

int timesafe_memcmp(const void* ptr1, const void* ptr2, size_t len);

/**
  timesafe_strncmp - Constant-time string comparison

  Compares bytes in two different string buffers in constant time

  This function compares the contents of the string buffer at ptr1 with
  the string buffer at ptr2 up to a maximum of len bytes.  The function
  does not compare bytes beyond the first occurrence of a NULL byte.  The
  function returns 1 if the two strings are different and 0 if the strings
  are identical.  The function always scans the entire range of memory to
  ensure the function runs in constant time.

  This function shuld be used when comparing strings that contain confidential
  information as it prevents timing attacks.  A traditional strncmp() exits 
  after finding non-equal bytes or a NULL byte and this can be used to
  determine the value of confidential data in memory.

  @param[in]  ptr1   Points to the first string to be compared.
  @param[in]  ptr2   Points to the second string to be compared.
  @param[in]  len    The number of bytes to be compared.

  @return
  This function returns 1 if the strings are different and
  0 if the strings are the same.

  @dependencies
  None.
*/

int timesafe_strncmp(const char* ptr1, const char* ptr2, size_t len);
/**
 * @}
 */
 
  /** @addtogroup str_apis
  @{ */
/**
  strnlen - Determine the length of a fixed size string

  This function takes a maxlen length parameter and stops looking for a NULL
  character if it passes the maxlen length.  It is considered safer than
  strlen because it will not enter an endless loop if the source string
  is a bad string without NULL termination.

  @param[in]  str     Points to the source string.
  @param[in]  maxlen  The maximum number of bytes to count.

  @return
  This function returns the number of bytes in string pointed to by str,
  excluding the terminating NULL byte ('\0'), but at most maxlen.

  @dependencies
  None.
*/
#ifndef _WIN32
size_t strnlen(const char *str, size_t maxlen);
#endif

/**
 * @}
 */
#ifdef __cplusplus
    } //extern "C"
} //namespace std
#endif //__cplusplus

//Explicit export of the libstd implemented functions
#ifdef __cplusplus
#ifdef _WIN32
    using std::strlcat;
    using std::strlcpy;
    using std::strcasecmp;
    using std::strncasecmp;
#endif
    using std::wcslcat;
    using std::wstrlcat;
    using std::wcslcpy;
    using std::wstrlcpy;
    using std::wstrcmp;
    using std::wstrncmp;
    using std::wstrlen;
    using std::memscpy;
    using std::memsmove;
    using std::secure_memset;
    using std::timesafe_memcmp;
    using std::timesafe_strncmp;
#ifndef _WIN32
    using std::strnlen;
#endif
#endif //__cplusplus

#endif /* _STRINGL_H_ */
