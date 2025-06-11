/*
 * Copyright (C) zhouwg, 2015-2021  All rights reserved.
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
#include "cde_log.h"

#if (defined(TEE_TA_SIDE))
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <string.h>
#include <tee_api.h>
#include <stdbool.h>
#include "trace.h"
#endif

#include <string.h>

//http://www.cnblogs.com/clover-toeic/p/4031618.html

#define NONE                 "\e[0m"
#define BLACK                "\e[0;30m"
#define L_BLACK              "\e[1;30m"
#define RED                  "\e[0;31m"
#define L_RED                "\e[1;31m"
#define GREEN                "\e[0;32m"
#define L_GREEN              "\e[1;32m"
#define BROWN                "\e[0;33m"
#define L_YELLOW             "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define L_BLUE               "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define L_PURPLE             "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define L_CYAN               "\e[1;36m"
#define WHITE                "\e[0;37m"
#define L_WHITE              "\e[1;37m"
#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K" //or "\e[1K\r"

#ifndef __cplusplus
#define true 1
#define false 0
#endif

#define LOG_BUF_LEN 4096

#if (defined __ANDROID__) || (defined ANDROID)
extern  int __android_log_print(int prio, const char *tag,  const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__ ((format(printf, 3, 4)))
#endif
;
#endif



static char logBuf[LOG_BUF_LEN];
static int gAllowedLogEnabled       = 1;
static int gAllowedLogLevel         = CDE_LOG_DEFAULT;
static int64_t gMaxDumpCounts       = 0; //no limit
static int64_t gLogDumpCounts       = 0;


#if 1// (defined(TEE_TA_SIDE))
static char* cde_strcat(char *dst, const char *src)
{
    char *temp = dst;
    if ((NULL == dst) || (NULL == src))
        return NULL;

    while (*temp != '\0')
        temp++;
    while ((*temp++ = *src++) != '\0');

    return dst;
}

static void *cde_memset(void *s, int c, size_t n)
{
    size_t index = 0;
    char *temp = (char*)s;
    while (index < n) {
        *temp++ = c;
        index++;
    }
    return s;
}

static char *cde_strrchr(const  char *s, char c)
{
    const char *p = NULL;

    do {
        if (*s == (char)c) {
            p = s;
        }
    } while (*s++);

    return (char *)p;
}

static int cde_isprint(int c)
{
    return (unsigned int)(c - ' ') < 127u - ' ';
}

#else
#define cde_strcat strcat
#endif

#if 1
void cdelog_setGlobalLogEnabled(int bEnable) {
    if (bEnable) {
        gAllowedLogEnabled = 1;
    } else {
        gAllowedLogEnabled = 0;
    }
    gMaxDumpCounts   = 0; //reset
}



void cdelog_setGlobalLogLevel(int level) {
    gAllowedLogLevel = (level >= 0 ? level : CDE_LOG_SILENT);
    gMaxDumpCounts   = 0; //reset
    gLogDumpCounts   = 0; //reset
}


int cdelog_validateLogLevel(int prio) {
    return (prio >= gAllowedLogLevel);
}

void cdelog_setGlobalDumpCounts(int64_t dumpCounts) {
    gMaxDumpCounts = dumpCounts;
    gLogDumpCounts = 0; //reset
}


#ifdef __KERNEL__
asmlinkage void  LOG_PRI_ORIG_IMPL(const char *file, const char *func, unsigned int line,  int priority, const char *tag, const char *format,  ...) {
#else
#if (!defined(__KERNEL__)) && (!defined(TEE_TA_SIDE))
pthread_mutex_t cde_log_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
void  LOG_PRI_ORIG_IMPL(const char *file, const char *func, unsigned int line,  int priority, const char *tag, const char *format,  ...) {
#endif

#ifdef NDEBUG
    return;
#endif

#if KANTV_DEBUG == 0
    return;
#endif

    if (0 == gAllowedLogEnabled) {
        return;
    }

    if (0 == gMaxDumpCounts) {
        //no limit
    } else {
        if (gLogDumpCounts >= gMaxDumpCounts)
        {
            return;
        }
    }

    tag = "KANTV";

    if (!cdelog_validateLogLevel(priority)) {
        return;
    }


#if (!defined(__KERNEL__)) && (!defined(TEE_TA_SIDE))
    pthread_mutex_lock(&cde_log_mutex);
#endif
    {
        va_list va;
        char *pfile     = NULL;
        int len_prefix  = 0;
        int len_content = 0;
        const char *color = L_WHITE;

        cde_memset(logBuf, 0, LOG_BUF_LEN);
        va_start(va, format);

        switch (priority) {
            case CDE_LOG_VERBOSE:
                color = L_PURPLE;
                break;
            case CDE_LOG_DEBUG:
                color = L_YELLOW;
                break;
            case CDE_LOG_INFO:
                color = L_GREEN;
                break;
            case CDE_LOG_WARN:
                color = RED;
                break;
            case CDE_LOG_ERROR:
                color = L_RED;
                break;
            default:
                color = L_WHITE;
                break;
        }

        if (NULL == tag) {
            tag = " ";
        }

        pfile = cde_strrchr(file, '/');
        if (NULL == pfile)
            pfile = (char*)file;
        else
            pfile++;

        file = pfile;

#ifndef __KERNEL__
    #if (defined(TARGET_WASM))
        len_prefix = snprintf(logBuf, LOG_BUF_LEN, "[%s, %s, %d]: ",  file, func, line);
    #else
        #ifndef TEE_TA_SIDE
            len_prefix = snprintf(logBuf, LOG_BUF_LEN, "%s[%s, %s, %d]: ", color, file, func, line);
        #else
            //color output not supported with TA on Amlogic S905
            len_prefix = snprintf(logBuf, LOG_BUF_LEN, "[%s, %s, %d]: ", file, func, line);
        #endif
    #endif
#else
        len_prefix = snprintf(logBuf, LOG_BUF_LEN, "[%s, %s, %d]: ",  file, func, line);
#endif
        len_content = vsnprintf(logBuf + len_prefix, LOG_BUF_LEN - len_prefix, format, va);
        snprintf(logBuf + len_prefix + len_content, LOG_BUF_LEN - len_prefix - len_content, "\n");

#ifndef __KERNEL__
        #if (defined __ANDROID__) || (defined ANDROID)
            __android_log_print(priority, tag, "%s", logBuf);
            __android_log_print(priority, tag, NONE);
        #else
            #ifndef TEE_TA_SIDE
                #if (defined(TARGET_WASM))
                    printf("%s", logBuf);
                #else
                    printf("%s%s", logBuf, NONE);
                #endif
            #else
                //color output not supported with TA on Amlogic S905
                //trace_printf(NULL, 0, TRACE_DEBUG, true, "%s%s", logBuf, NONE);
                trace_printf(NULL, 0, TRACE_DEBUG, true, "%s", logBuf);
            #endif
        #endif
#else
        printk(KERN_INFO "%s", logBuf);
#endif
        va_end(va);
    }
    gLogDumpCounts++;
#if (!defined(__KERNEL__)) && (!defined(TEE_TA_SIDE))
    pthread_mutex_unlock(&cde_log_mutex);
#endif
}


#ifdef __KERNEL__
EXPORT_SYMBOL_GPL(LOG_PRI_ORIG_IMPL);
#endif


#ifdef __KERNEL__
asmlinkage void  LOG_PRI_ZWG_IMPL(const char *file, const char *func, unsigned int line,  int priority, const char *tag, const char *format,  ...) {
#else
#if (!defined(__KERNEL__)) && (!defined(TEE_TA_SIDE))
pthread_mutex_t cde_logzwg_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
void  LOG_PRI_ZWG_IMPL(const char *file, const char *func, unsigned int line,  int priority, const char *tag, const char *format,  ...) {
#endif


    {
        tag = "KANTV";
    }

#if (!defined(__KERNEL__)) && (!defined(TEE_TA_SIDE))
    pthread_mutex_lock(&cde_logzwg_mutex);
#endif
    if (1) {
        va_list va;
        char *pfile     = NULL;
        int len_prefix  = 0;
        int len_content = 0;
        const char *color = L_WHITE;

        cde_memset(logBuf, 0, LOG_BUF_LEN);
        va_start(va, format);

        switch (priority) {
            case CDE_LOG_VERBOSE:
                color = L_PURPLE;
                break;
            case CDE_LOG_DEBUG:
                color = L_YELLOW;
                break;
            case CDE_LOG_INFO:
                color = L_GREEN;
                break;
            case CDE_LOG_WARN:
                color = RED;
                break;
            case CDE_LOG_ERROR:
                color = L_RED;
                break;
            default:
                color = L_WHITE;
                break;
        }

        if (NULL == tag) {
            tag = " ";
        }

        pfile = cde_strrchr(file, '/');
        if (NULL == pfile)
            pfile = (char*)file;
        else
            pfile++;

        file = pfile;

#ifndef __KERNEL__
    #if (defined(TARGET_WASM))
        len_prefix = snprintf(logBuf, LOG_BUF_LEN, "[%s, %s, %d]: ",  file, func, line);
    #else
        #ifndef TEE_TA_SIDE
            len_prefix = snprintf(logBuf, LOG_BUF_LEN, "%s[%s, %s, %d]: ", color, file, func, line);
        #else
            //color output not supported with TA on Amlogic S905
            len_prefix = snprintf(logBuf, LOG_BUF_LEN, "[%s, %s, %d]: ", file, func, line);
        #endif
    #endif
#else
        len_prefix = snprintf(logBuf, LOG_BUF_LEN, "[%s, %s, %d]: ",  file, func, line);
#endif
        len_content = vsnprintf(logBuf + len_prefix, LOG_BUF_LEN - len_prefix, format, va);
        snprintf(logBuf + len_prefix + len_content, LOG_BUF_LEN - len_prefix - len_content, "\n");

#ifndef __KERNEL__
        #if (defined __ANDROID__) || (defined ANDROID)
            __android_log_print(priority, tag, "%s", logBuf);
            __android_log_print(priority, tag, NONE);
#ifdef GGML_USE_QNN
            printf("%s%s", logBuf, NONE);
#endif
        #else
            #ifndef TEE_TA_SIDE
                #if (defined(TARGET_WASM))
                    printf("%s", logBuf);
                #else
                    printf("%s%s", logBuf, NONE);
                #endif
            #else
                //color output not supported with TA on Amlogic S905
                //trace_printf(NULL, 0, TRACE_DEBUG, true, "%s%s", logBuf, NONE);
                trace_printf(NULL, 0, TRACE_DEBUG, true, "%s", logBuf);
            #endif
        #endif
#else
        printk(KERN_INFO "%s", logBuf);
#endif
        va_end(va);
    }
#if (!defined(__KERNEL__)) && (!defined(TEE_TA_SIDE))
    pthread_mutex_unlock(&cde_logzwg_mutex);
#endif
}

#ifdef __KERNEL__
EXPORT_SYMBOL_GPL(LOG_PRI_ZWG_IMPL);
#endif

#endif

#ifdef __KERNEL__
asmlinkage void HEXDUMP_PRI_ORIG_IMPL(const char *file, const char *func, unsigned int line, const char *name, const void *_data, unsigned int size, int have_prefix, int should_skip) {
#else
void HEXDUMP_PRI_ORIG_IMPL(const char *file, const char *func, unsigned int line, const char *index_name, unsigned int index, const char *data_name, const void *_data, unsigned int size, int have_prefix, int should_skip) {
#endif
    const unsigned char *data = (const unsigned char *)_data;
    size_t offset = 0;


#if (defined __ANDROID__) || (defined ANDROID)
    int priority = CDE_LOG_DEBUG;
    const char *tag = NULL;

    {
        tag = "KANTV";
    }
#endif

    if (0 == gMaxDumpCounts) {
        //no limit
    } else {
        //filtering step-2
        if (gLogDumpCounts >= gMaxDumpCounts)
        {
            return;
        }
    }

    if (1 == have_prefix) {
        char prefix[512];
        cde_memset(prefix, 0, 512);

        char *pfile = cde_strrchr(file, '/');
        if (NULL == pfile)
            pfile = (char*)file;
        else
            pfile++;

        file = pfile;

        if (NULL != index_name)
            snprintf(prefix, 512, "[%s, %s, %d] %s-%-3d, dump %s(len=%d): ",  file, func, line, index_name, index, data_name, (int)size);
        else
            snprintf(prefix, 512, "[%s, %s, %d] dump %s(len=%d): ",  file, func, line,  data_name, (int)size);
#ifndef __KERNEL__
        #if (defined __ANDROID__) || (defined ANDROID)
            __android_log_print(priority, tag, "%s\n", prefix);
         #else
#ifndef TEE_TA_SIDE
            printf("%s\n", prefix);
#else
            trace_printf(NULL, 0, TRACE_DEBUG, true, "%s\n", prefix);
#endif
         #endif
#else
         printk(KERN_INFO "%s\n", prefix);
#endif
    }

    while (offset < size) {
        char sline[128];
        char tmp[32];

        cde_memset(sline, 0, 128);
        snprintf(tmp, 32,  "%08lx:  ", (unsigned long)offset);
        cde_strcat(sline, tmp);

        for (size_t i = 0; i < 16; ++i) {
            if (i == 8) {
                snprintf(tmp, 32, "%c", ' ');
                cde_strcat(sline, tmp);
            }
            if (offset + i >= size) {
                cde_strcat(sline,  "   ");
            } else {
                snprintf(tmp, 32, "%02x ", data[offset + i]);
                cde_strcat(sline, tmp);
            }
        }

        snprintf(tmp, 32, "%c", ' ');
        cde_strcat(sline, tmp);

        for (size_t i = 0; i < 16; ++i) {
            if (offset + i >= size) {
                break;
            }

            if (cde_isprint(data[offset + i])) {
                snprintf(tmp, 32, "%c", (char)data[offset + i]);
                cde_strcat(sline, tmp);
            } else {
                snprintf(tmp, 32, "%c", '.');
                cde_strcat(sline, tmp);
            }
        }

#ifndef __KERNEL__
        #if (defined __ANDROID__) || (defined ANDROID)
        __android_log_print(priority, tag, "%s", sline);
        #else
#ifndef TEE_TA_SIDE
        printf("%s\n", sline);
#else
        trace_printf(NULL, 0, TRACE_DEBUG, true, "%s\n", sline);
#endif
        #endif
#else
        printk(KERN_INFO "%s\n", sline);
#endif

        offset += 16;

        if ((size > 160 )  && (1 == should_skip)) {
            if (offset >= 80) {
                int n = size / 16;
                offset = ( n - 2) * 16;
                should_skip = 0;
#ifndef __KERNEL__
                #if (defined __ANDROID__) || (defined ANDROID)
                __android_log_print(priority, tag, "........");
                #else
#ifndef TEE_TA_SIDE
                printf("........\n");
#else
                trace_printf(NULL, 0, TRACE_DEBUG, true, "........\n");
#endif
                #endif
#else
                printk(KERN_INFO "........\n");
#endif
            }
        }
    }

    return;
#ifndef __KERNEL__
    #if (defined __ANDROID__) || (defined ANDROID)
        __android_log_print(priority, tag, "\n");
     #else
#ifndef TEE_TA_SIDE
        printf("\n");
#else
        trace_printf(NULL, 0, TRACE_DEBUG, true, "\n");
#endif
     #endif
#else
     printk(KERN_INFO "\n");
#endif
}


#ifdef __KERNEL__
EXPORT_SYMBOL_GPL(HEXDUMP_PRI_ORIG_IMPL);
#endif
