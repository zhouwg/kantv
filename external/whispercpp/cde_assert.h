/*
 * Copyright (c) Project KanTV. 2021-2023. All rights reserved.
 *
 * Copyright (c) 2024- KanTV Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * The above statement and notice must be included in corresponding files
 * in derived project
 */

#ifndef __CDE_ASSERT__
#define __CDE_ASSERT__

#ifndef __KERNEL__
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#if  defined(ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

//borrow from AOSP
#if  1//defined(ANDROID) || defined(__ANDROID__)
#define CONDITION(cond)                         (__builtin_expect((cond) != 0, 0))
#define __android_second(dummy, second, ...)    second
#define __android_rest(first, ...)              , ## __VA_ARGS__

#define android_printAssert(cond, tag, fmt...)  __android_log_assert(cond, tag, __android_second(0, ## fmt, NULL) __android_rest(fmt))


#ifndef LOG_ALWAYS_FATAL_IF
#ifndef __IRSA_DEBUG__
#define LOG_ALWAYS_FATAL_IF(cond, ...) \
            ( (CONDITION(cond)) \
            ? ((void)LOGE(__VA_ARGS__)) \
            : (void)0 )
#else
#define LOG_ALWAYS_FATAL_IF(cond, ...) \
            ( (CONDITION(cond)) \
            ? ((void)android_printAssert(#cond, LOG_TAG, ## __VA_ARGS__)) \
            : (void)0 )
#endif
#endif

#ifndef LOG_ALWAYS_FATAL
#ifndef __IRSA_DEBUG__
#define LOG_ALWAYS_FATAL(...) \
            ((void)LOGE(__VA_ARGS__))
#else
#define LOG_ALWAYS_FATAL(...) \
        ( ((void)android_printAssert(NULL, LOG_TAG, ## __VA_ARGS__)) )
#endif
#endif


#ifndef __IRSA_DEBUG__
#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) ((void)0)
#endif

#ifndef LOG_FATAL
#define LOG_FATAL(...) ((void)0)
#endif

#else

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) LOG_ALWAYS_FATAL_IF(cond, ## __VA_ARGS__)
#endif

#ifndef LOG_FATAL
#define LOG_FATAL(...) LOG_ALWAYS_FATAL(__VA_ARGS__)
#endif

#endif

#ifndef LOG_ASSERT
#define LOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), ## __VA_ARGS__)
#endif

#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)



#ifndef __IRSA_DEBUG__
#define CHECK(condition) (condition)

#else

#define CHECK(condition)                            \
        LOG_ALWAYS_FATAL_IF(                            \
            !(condition),                               \
            "%s",                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)    \
            " CHECK(" #condition ") failed.")
#endif


#ifndef __IRSA_DEBUG__
#define TRESPASS() ((void)0)
#else
#define TRESPASS() \
        LOG_ALWAYS_FATAL(                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)            \
                " Should not be here.");
#endif

#endif // #if  defined(ANDROID) || defined(__ANDROID__)


#ifdef __cplusplus
}
#endif

#endif
