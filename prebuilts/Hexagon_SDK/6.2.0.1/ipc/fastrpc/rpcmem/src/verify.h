/**
 * Copyright (c) 2012-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VERIFY_H
#define VERIFY_H


//#define VERIFY_PRINT_ERROR
//#define VERIFY_PRINT_INFO


#ifndef _WIN32
#define C_ASSERT(test) \
    switch(0) {\
      case 0:\
      case test:;\
    }
#endif // _WIN32

#ifndef __V_STR__
	#define __V_STR__(x) #x ":"
#endif //__STR__
#ifndef __V_TOSTR__
	#define __V_TOSTR__(x) __V_STR__(x)
#endif // __TOSTR__
#ifndef __V_FILE_LINE__
	#define __V_FILE_LINE__ __FILE__ ":" __V_TOSTR__(__LINE__)
#endif /*__FILE_LINE__*/


// TODO:sunny - enabled extra prints
#define VERIFY_PRINT_INFO
#define VERIFY_PRINT_ERROR


#ifdef __ANDROID__
/*android */
#if (defined VERIFY_PRINT_INFO) || (defined VERIFY_PRINT_ERROR)
#include <android/log.h>
#endif

extern const char* __progname;
#ifdef VERIFY_PRINT_INFO
#ifdef __ANDROID__
#define VERIFY_IPRINTF(format, ...) __android_log_print(ANDROID_LOG_DEBUG , __progname, __V_FILE_LINE__ format, ##__VA_ARGS__)
#else /* !__ANDROID__ */
#define VERIFY_IPRINTF(format, ...) fprintf(stderr,"%s:%d " _fmt "\n", __func__, __LINE__ , ##__VA_ARGS__)
#endif /* __ANDROID__ */
#endif

#ifdef VERIFY_PRINT_ERROR
#ifdef __ANDROID__
#define VERIFY_EPRINTF(format, ...) __android_log_print(ANDROID_LOG_ERROR , __progname, __V_FILE_LINE__ format, ##__VA_ARGS__)
#else /* !__ANDROID__ */
#define VERIFY_EPRINTF(format, ...) fprintf(stderr,"%s:%d " _fmt "\n", __func__, __LINE__ , ##__VA_ARGS__)
#endif /* __ANDROID__ */
#endif

/* end android */
#elif (defined __hexagon__) || (defined __qdsp6__)
/* q6 */

#ifdef VERIFY_PRINT_INFO
   #define FARF_VERIFY_LOW  1
   #define FARF_VERIFY_LOW_LEVEL HAP_LEVEL_LOW
   #define VERIFY_IPRINTF(args...) FARF(VERIFY_LOW, args)
#endif

#ifdef VERIFY_PRINT_ERROR
   #define FARF_VERIFY_ERROR         1
   #define FARF_VERIFY_ERROR_LEVEL HAP_LEVEL_ERROR
   #define VERIFY_EPRINTF(args...) FARF(VERIFY_ERROR, args)
#endif

#if (defined VERIFY_PRINT_INFO) || (defined VERIFY_PRINT_ERROR)
   #include "HAP_farf.h"
#endif

/* end q6 */
#else
/* generic */

#if (defined VERIFY_PRINT_INFO) || (defined VERIFY_PRINT_ERROR)
#include <stdio.h>
#endif

#ifdef VERIFY_PRINT_INFO
#define VERIFY_IPRINTF(format, ...) printf(__V_FILE_LINE__ format, ##__VA_ARGS__)
#endif

#ifdef VERIFY_PRINT_ERROR
#define VERIFY_EPRINTF(format, ...) printf(__V_FILE_LINE__ format, ##__VA_ARGS__)
#endif

/* end generic */
#endif

#ifndef VERIFY_PRINT_INFO
#define VERIFY_IPRINTF(format, ...) (void)0
#endif

#ifndef VERIFY_PRINT_ERROR
#define VERIFY_EPRINTF(format, ...) (void)0
#endif

#ifndef VERIFY
	#define VERIFY(val) \
	   do {\
		  VERIFY_IPRINTF(":info: calling: " #val "\n");\
		  if(0 == (val)) {\
			 nErr = nErr == 0 ? -1 : nErr;\
			 VERIFY_EPRINTF(":error: %d: " #val "\n", nErr);\
			 goto bail;\
		  } else {\
			 VERIFY_IPRINTF(":info: passed: " #val "\n");\
		  }\
	   } while(0)
#endif //VERIFY

#ifndef VERIFYC
        #define VERIFYC(val,err_code) \
           do {\
                  VERIFY_IPRINTF(":info: calling: " #val "\n");\
                  if(0 == (val)) {\
                         nErr = err_code;\
                         VERIFY_EPRINTF(":Error: %x: " #val "\n", nErr);\
                         goto bail;\
                  } else {\
                         VERIFY_IPRINTF(":info: passed: " #val "\n");\
                  }\
           } while(0)
#endif //VERIFYC


#endif /* VERIFY_H */


