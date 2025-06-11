#ifndef AEEVALIST_H
#define AEEVALIST_H
/*
=======================================================================
    Copyright 2006-2007,2012-2013,2020 QUALCOMM Technologies Inc.
    All Rights Reserved.
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
=======================================================================
*/

#if !defined(__clang__) && (defined(__ARMCC_VERSION) || (defined(__GNUC__) && defined(__arm__)))

#if (defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 200000 && !defined(__APCS_ADSABI)) || \
    (defined(__GNUC__) && defined(__arm__) && defined(__ARM_EABI__))

# define __AEEVA_ATPCS 0

#else

# define __AEEVA_ATPCS 1

#endif

typedef void* AEEVaList;

#define __AEEVA_ARGALIGN(t)   (((char*)(&((struct{char c;t x;}*)1)->x))-((char*)1))
#define __AEEVA_ARGSIZE(t)    ((sizeof(t)+sizeof(int)-1) & ~(sizeof(int)-1))

static __inline void __cpy(char*d, const char*s, int len)
{
   while (len-- > 0) *d++ = *s++;
}

static __inline AEEVaList __AEEVa_Arg(AEEVaList args, void* pv, int nVSize,
                                      int nArgSize, int nArgAlign)
{
   int   nArgs = (int)args & ~1;
   char* pcArgs = (char*)args;
   int   bATPCS = (int)args & 1;
   int   nArgsOffset = 0;
   int   nVOffset = 0;

   if (!bATPCS) { /* caller was compiled with AAPCS */

      if (nArgAlign > (int)sizeof(int)) {
         nArgAlign--; /* make a mask */
         pcArgs += ((nArgs + nArgAlign) & (int)~(unsigned)nArgAlign) - nArgs;
         /* move pv to next alignment */
      }
   }

#if defined(AEE_BIGENDIAN)
   if (nArgSize < (int)sizeof(int)) {
      nArgsOffset = (int)sizeof(int) - nArgSize;
   }
   nVOffset = nVSize - nArgSize;
#else
   (void)nVSize;
#endif /* AEE_BIGENDIAN */

   __cpy((char*)pv + nVOffset, (pcArgs - bATPCS) + nArgsOffset, nArgSize);

   /* round up */
   nArgSize = (nArgSize+(int)sizeof(int)-1) & ~((int)sizeof(int)-1);

   return pcArgs + nArgSize; /* increment va */
}

#define AEEVA_START(va,v)     ((va) = (char*)&(v) + __AEEVA_ARGSIZE(v) + __AEEVA_ATPCS)
#define AEEVA_ARG(va,v,t)     ((void)((va) = __AEEVa_Arg(va,&v,sizeof(v),sizeof(t),__AEEVA_ARGALIGN(t))))
#define AEEVA_END(va)         ((va) = (AEEVaList)0)
#define AEEVA_COPY(dest, src) ((void)((dest) = (src)))

#else /* !defined(__clang__) && (defined(__ARMCC_VERSION) || (defined(__GNUC__) && defined(__arm__))) */

#include <stdarg.h>

typedef va_list AEEVaList;

#define AEEVA_START(va,v)     (va_start((va), (v)))
#define AEEVA_ARG(va,v,t)     ((v) = va_arg((va),t))
#define AEEVA_END(va)         (va_end((va)))
#define AEEVA_COPY(dest, src) (va_copy((dest),(src)))

#endif/* !defined(__clang__) && (defined(__ARMCC_VERSION) || (defined(__GNUC__) && defined(__arm__))) */

#endif /* #ifndef AEEVALIST_H */

