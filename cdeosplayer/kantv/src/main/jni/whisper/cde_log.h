/* Copyright (C) zhou.weiguo, 2015-2021  All rights reserved.
 *
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
 */

#ifndef CDE_LOG_H
#define CDE_LOG_H

#if (!defined(__KERNEL__)) && (!defined(TEE_TA_SIDE))
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#endif /* ifndef __KERNEL__ */


#ifdef __cplusplus
extern "C" {
#endif

/*
 *fetch from AOSP, Android log priority values, in ascending priority order.
 */
typedef enum cde_android_LogPriority {
    CDE_LOG_UNKNOWN = 0,
    CDE_LOG_DEFAULT,    /* only for setGlobalLogLevel */
    CDE_LOG_VERBOSE,
    CDE_LOG_DEBUG,
    CDE_LOG_INFO,
    CDE_LOG_WARN,
    CDE_LOG_ERROR,
    CDE_LOG_FATAL,
    CDE_LOG_SILENT,     /* only for setGlobalLogLevel() */
} cde_android_LogPriority;


#ifndef LOG_NDEBUG
    #ifdef NDEBUG
        #define LOG_NDEBUG 1
    #else
        #define LOG_NDEBUG 0
    #endif
#endif

#ifndef LOG_TAG
    #define LOG_TAG NULL
#endif


#ifdef __GNUC__
    #define DO_NOT_INSTRUMENT __attribute__ ((no_instrument_function))
#else
    #define DO_NOT_INSTRUMENT
#endif


#if LOG_NDEBUG

    #define CDE_LOG_PRI(file, func, line, priority, tag, ...) ((void)0)

    #define HEXDUMP_PRI(file, func, line, index_name, index, data_name, data, size, prefix, skip) ((void)0)

#else

    #define CDE_LOG_PRI(file, func, line, priority, tag, ... ) \
           LOG_PRI_ORIG_IMPL(file, func, line, priority, tag, __VA_ARGS__)

    #define HEXDUMP_PRI(file, func, line, index_name, index, data_name, data, size, prefix, skip) \
           HEXDUMP_PRI_ORIG_IMPL(file, func, line, index_name, index, data_name, data, size, prefix, skip)

#endif /*LOG_NDEBUG*/

#if LOG_NDEBUG

#define CDE_LOG_JNI(file, func, line, priority, tag, ...) ((void)0)

#else

#define CDE_LOG_JNI(file, func, line, priority, tag, ... ) \
           LOG_PRI_ORIG_IMPL(file, func, line, priority, tag, __VA_ARGS__)


#endif

#define CDE_LOG_ZWG(file, func, line, priority, tag, ... ) \
           LOG_PRI_ZWG_IMPL(file, func, line, priority, tag, __VA_ARGS__)


#if LOG_NDEBUG

#define CDE_HEXDUMP_JNI(file, func, line, index_name, index, data_name, data, size, prefix, skip) ((void)0)

#else

#define CDE_HEXDUMP_JNI(file, func, line, index_name, index, data_name, data, size, prefix, skip) \
           HEXDUMP_PRI_ORIG_IMPL(file, func, line, index_name, index, data_name, data, size, prefix, skip)

#endif


#ifdef __KERNEL__
    asmlinkage  void  LOG_PRI_ORIG_IMPL(const char *file, const char *func, unsigned int line,  int priority, const char *tag, const char *format,...);
#else
    void  LOG_PRI_ORIG_IMPL(const char *file, const char *func, unsigned int line,  int priority, const char *tag, const char *format,...);
#endif

#ifdef __KERNEL__
    asmlinkage  void  LOG_PRI_ZWG_IMPL(const char *file, const char *func, unsigned int line,  int priority, const char *tag, const char *format,...);
#else
    void  LOG_PRI_ZWG_IMPL(const char *file, const char *func, unsigned int line,  int priority, const char *tag, const char *format,...);
#endif

#ifdef __KERNEL__
    asmlinkage  void  HEXDUMP_PRI_ORIG_IMPL(const char *file, const char *func, unsigned int line, const char *index_name, unsigned int index, const char *data_name, const void *data, size_t size, int prefix, int skip);
#else
    void  HEXDUMP_PRI_ORIG_IMPL(const char *file, const char *func, unsigned int line, const char *index_name, unsigned int index, const char *data_name, const void *data, unsigned int size, int prefix, int skip);
#endif

void cdelog_setGlobalDumpCounts(int64_t maxDumpCounts);
void cdelog_setGlobalLogLevel(int);
int  cdelog_validateLogLevel(int);
void cdelog_setGlobalLogEnabled(int bEnable);
void cdelog_setGlobalLogModule(const char *moduleName, int bEnabled);




#ifndef LOGV
#define LOGV(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOG_VERBOSE(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#endif

#ifndef LOGD
#define LOGD(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_DEBUG(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#endif

#ifndef LOGI
#define LOGI(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOG_INFO(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_INFO, LOG_TAG, __VA_ARGS__)
#endif

#ifndef LOGW
#define LOGW(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOG_WARN(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_WARN, LOG_TAG, __VA_ARGS__)
#endif

#ifndef LOGE
#define LOGE(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOG_ERROR(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif

#ifndef LOGF
#define LOGF(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_FATAL, LOG_TAG, __VA_ARGS__)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(...) CDE_LOG_PRI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_FATAL, LOG_TAG, __VA_ARGS__)
#endif


#ifndef LOGJ
#define LOGJ(...) CDE_LOG_JNI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_JNI(...) CDE_LOG_JNI(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#endif


#ifndef LOGG
#define LOGGV(...) CDE_LOG_ZWG(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGGD(...) CDE_LOG_ZWG(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_DEBUG,   LOG_TAG, __VA_ARGS__)
#define LOGGI(...) CDE_LOG_ZWG(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_INFO,    LOG_TAG, __VA_ARGS__)
#define LOGGW(...) CDE_LOG_ZWG(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_WARN,    LOG_TAG, __VA_ARGS__)
#define LOGGE(...) CDE_LOG_ZWG(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_ERROR,   LOG_TAG, __VA_ARGS__)
#define LOGGF(...) CDE_LOG_ZWG(__FILE__, __FUNCTION__, __LINE__, CDE_LOG_FATAL,   LOG_TAG, __VA_ARGS__)
#endif


#ifndef HEXDUMPJ
#define HEXDUMPJ(data, size) CDE_HEXDUMP_JNI(__FILE__, __FUNCTION__, __LINE__, NULL, 0, #data, data, size, 1, 1)
#endif

#ifndef HEXDUMPJ_NOPREFIX
#define HEXDUMPJ_NOPREFIX(data, size) CDE_HEXDUMP_JNI(__FILE__, __FUNCTION__, __LINE__, NULL, 0, #data, data, size, 0, 1)
#endif

#ifndef HEXFULLDUMPJ
#define HEXFULLDUMPJ(data, size) CDE_HEXDUMP_JNI(__FILE__, __FUNCTION__, __LINE__, NULL, 0, #data, data, size, 1, 0)
#endif

#ifndef HEXFULLDUMPJ_NOPREFIX
#define HEXFULLDUMPJ_NOPREFIX(data, size) CDE_HEXDUMP_JNI(__FILE__, __FUNCTION__, __LINE__, NULL, 0, #data, data, size, 0, 0)
#endif

#ifndef HEXDUMP
#define HEXDUMP(data, size) HEXDUMP_PRI(__FILE__, __FUNCTION__, __LINE__, NULL, 0, #data, data, size, 1, 1)
#endif

#ifndef HEXDUMP3
#define HEXDUMP3(index, data, size) HEXDUMP_PRI(__FILE__, __FUNCTION__, __LINE__, #index, index,  #data, data, size, 1, 1)
#endif

#ifndef HEXFULLDUMP
#define HEXFULLDUMP(data, size) HEXDUMP_PRI(__FILE__, __FUNCTION__, __LINE__, NULL, 0, #data, data, size, 1, 0)
#endif

#ifndef HEXDUMP_NOPREFIX
#define HEXDUMP_NOPREFIX(data, size) HEXDUMP_PRI(__FILE__, __FUNCTION__, __LINE__, NULL, 0, #data, data, size, 0, 0)
#endif


#define ENTER_FUNC() LOGGD("enter %s {", __FUNCTION__)

#define LEAVE_FUNC() LOGGD("leave %s }", __FUNCTION__)

#define ENTER_JNI_FUNC() LOGGV("enter %s {", __FUNCTION__)

#define LEAVE_JNI_FUNC() LOGGV("leave %s }", __FUNCTION__)


#ifdef __cplusplus
}
#endif

#endif /* CDE_LOG_H */
