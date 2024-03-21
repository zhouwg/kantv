/*
 * Apple HTTP Live Streaming demuxer
 * Copyright (c) 2021, Zhou Weiguo <zhouwg2000@gmail.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "log.h"
#include "cde_log.h"
#include "hlsencryptinfo.h"

void dump_key_info(struct KeyInfo *info)
{
    if (NULL == info)
        return;

    if ((NULL != info->encryption_keyuri) && (0 != info->encryption_keyuri[0]))
    av_log(NULL, AV_LOG_INFO, "\tinfo->encryption_keyuir      : %s", info->encryption_keyuri);

    if ((NULL != info->encryption_keyrealuri) && (0 != info->encryption_keyrealuri[0]))
    av_log(NULL, AV_LOG_INFO, "\tinfo->encryption_keyrealuri  : %s", info->encryption_keyrealuri);

    if ((NULL != info->encryption_method) && (0 != info->encryption_method[0]))
    av_log(NULL, AV_LOG_INFO, "\tinfo->encryption_method      : %s", info->encryption_method);

    if ((NULL != info->encryption_ivstring) && (0 != info->encryption_ivstring[0]))
    av_log(NULL, AV_LOG_INFO, "\tinfo->encryption_ivstring    : %s", info->encryption_ivstring);

    if ((NULL != info->encryption_videoformat) && (0 != info->encryption_videoformat[0]))
    av_log(NULL, AV_LOG_INFO, "\tinfo->encryption_videoformat : %s", info->encryption_videoformat);

    if ((NULL != info->encryption_keyidstring) && (0 != info->encryption_keyidstring[0]))
    av_log(NULL, AV_LOG_INFO, "\tinfo->encryption_keyidstring : %s", info->encryption_keyidstring);

    if ((NULL != info->encryption_keyid) && (0 != info->encryption_keyid[0]))
    av_log(NULL, AV_LOG_INFO, "\tinfo->encryption_keyid       : %s", info->encryption_keyid);

    if ((NULL != info->encryption_keyformat) && (0 != info->encryption_keyformat[0]))
    av_log(NULL, AV_LOG_INFO, "\tinfo->encryption_keyformat   : %s", info->encryption_keyformat);

    if ((NULL != info->encryption_keyformatversions) && (0 != info->encryption_keyformatversions[0]))
    av_log(NULL, AV_LOG_INFO, "\tinfo->encryption_keyformatversions: %s", info->encryption_keyformatversions);

    if ((NULL != info->encryption_keystring) && (0 != info->encryption_keystring[0]))
    av_log(NULL, AV_LOG_INFO, "\tinfo->encryption_keystring   : %s", info->encryption_keystring);

    av_log(NULL, AV_LOG_INFO, "\tinfo->is_encrypted           : %d", info->is_encrypted);
    av_log(NULL, AV_LOG_INFO, "\tinfo->es_type                : %d", info->es_type);
    av_log(NULL, AV_LOG_INFO, "\tinfo->drm_sessionhandle      : %p", info->drm_sessionhandle);
    av_log(NULL, AV_LOG_INFO, "\tinfo->is_provisioned         : %d", info->is_provisioned);
}

#if (defined __ANDROID__) || (defined ANDROID)
extern  int __android_log_print(int prio, const char *tag,  const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__ ((format(printf, 3, 4)))
#endif
;
#endif


#ifdef __KERNEL__
    #define PRINT(format, arg) \
        printk(KERN_INFO format, arg)
#else
    #if (defined __ANDROID__) || (defined ANDROID)
        #define PRINT(format, arg) \
             __android_log_print(priority, tag, format , arg)
    #else
#ifndef TEE_TA_SIDE
        #define PRINT(format, arg) \
            printf(format, arg)
#else
        #define PRINT(format, arg)  (void)0
#endif
    #endif
#endif


void DUMP_KEY_INFO_ORIG_IMPL(const char *file, const char *func, unsigned int line, struct KeyInfo *info)
{
    if (NULL == info)
        return;

#if (defined __ANDROID__) || (defined ANDROID)
    int priority = CDE_LOG_DEBUG;
    const char *tag = NULL;

    if (NULL == tag) {
        tag = "SVEMEDIA";
    }
#endif

    char prefix[512];

    char *pfile = strrchr(file, '/');
    if (NULL == pfile)
        pfile = (char*)file;
    else
        pfile++;

    file = pfile;

    snprintf(prefix, 512, "[%s, %s, %d]  dump_key_info: ",  file, func, line);
#ifndef __KERNEL__
    #if (defined __ANDROID__) || (defined ANDROID)
        __android_log_print(priority, tag, "%s\n", prefix);
     #else
        printf("%s\n", prefix);
     #endif
#else
     printk(KERN_INFO "%s\n", prefix);
#endif

    if ((NULL != info->encryption_keyuri) && (0 != info->encryption_keyuri[0])) {
        PRINT("\tinfo->encryptionKeyUri      : %s", info->encryption_keyuri);
    }

    if ((NULL != info->encryption_keyrealuri) && (0 != info->encryption_keyrealuri[0])) {
        PRINT("\tinfo->encryptionKeyRealUri  : %s", info->encryption_keyrealuri);
    }

    if ((NULL != info->encryption_method) && (0 != info->encryption_method[0])) {
        PRINT("\tinfo->encryptionMethod      : %s", info->encryption_method);
    }

    if ((NULL != info->encryption_ivstring) && (0 != info->encryption_ivstring[0])) {
        PRINT("\tinfo->encryptionIvString    : %s", info->encryption_ivstring);
    }

    if ((NULL != info->encryption_videoformat) && (0 != info->encryption_videoformat[0])) {
        PRINT("\tinfo->encryptionVideoFormat : %s", info->encryption_videoformat);
    }

    if ((NULL != info->encryption_keyidstring) && (0 != info->encryption_keyidstring[0])) {
        PRINT("\tinfo->encryptionKeyIdString : %s", info->encryption_keyidstring);
    }

    if ((NULL != info->encryption_keyformat) && (0 != info->encryption_keyformat[0])) {
        PRINT("\tinfo->encryptionKeyFormat   : %s", info->encryption_keyformat);
    }

    if ((NULL != info->encryption_keyformatversions) && (0 != info->encryption_keyformatversions[0])) {
        PRINT("\tinfo->encryptionKeyFormatVer: %s", info->encryption_keyformatversions);
    }

    if ((NULL != info->encryption_keystring) && (0 != info->encryption_keystring[0])) {
        PRINT("\tinfo->encryptionKeyString   : %s", info->encryption_keystring);
    }

    PRINT("\tinfo->isEncrypted           : %d", info->is_encrypted);
    PRINT("\tinfo->is_provisioned        : %d", info->is_provisioned);
    PRINT("\tinfo->drmSessionHandle      : %p", info->drm_sessionhandle);
    PRINT("\tinfo->encryptionKey         : %s\n", " ");
    HEXDUMPJ_NOPREFIX(info->encryption_key, 16);
    PRINT("\tinfo->encryptionKeyId       : %s\n", " ");
    HEXDUMPJ_NOPREFIX(info->encryption_keyid, 16);
    PRINT("\tinfo->encryptionIv          : %s\n", " ");
    //HEXDUMP(info->encryptionKey, 16);
    //HEXDUMP(info->encryptionIv,  16);
    HEXDUMPJ_NOPREFIX(info->encryption_iv,  16);
}
