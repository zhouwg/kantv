/*
 *  decryptor for HLS encrypted content
 *
 * Copyright (c) 2011-2017, John Chen
 *
 * Copyright (c) 2021, Zhou Weiguo<zhouwg2000@gmail.com>, porting to FFmpeg and add HLS Sample China-SM4-CBC and HLS China-SM4-CBC support
 *
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

#ifndef AVUTIL_HLSDECRYPTOR_H
#define AVUTIL_HLSDECRYPTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <pthread.h>
#include <inttypes.h>

#include "hlsencryptinfo.h"
#include "libavcodec/avcodec.h"

#ifdef __cplusplus
    extern "C" {
#endif


#define AES_BLOCK_LENGTH_BYTES  16
#define SM4_BLOCK_LENGTH_BYTES  16

enum DrmCryptoLevel {
   DRM_CRYPTO_ENCRYPT_UNKNOWN,
   DRM_CRYPTO_ENCRYPT_NONE,
   DRM_CRYPTO_ENCRYPT_CONTAINER_BASED,
   DRM_CRYPTO_ENCRYPT_ES_BASED
};

typedef enum DrmCryptoInfoType {
    DRM_CRYPTO_IV = 0,
    DRM_CRYPTO_KEY,
    DRM_CRYPTO_KEYID,
    DRM_CRYPTO_ESMODE,
    DRM_CRYPTO_CRYPTOMODE,
    DRM_CRYPTO_ENCRYPT_LEVEL,
    DRM_CRYPTO_NEED_DEPADDING
}DrmCryptoInfoType;

typedef enum DrmCryptoMode {
     CRYPTO_MODE_NONE = 0,
     CRYPTO_MODE_AES_CTR  = 1,
     CRYPTO_MODE_AES_CBC  = 2,
     CRYPTO_MODE_SM4_ECB  = 3,
     CRYPTO_MODE_SM4_CBC  = 4,
     CRYPTO_MODE_SM4_CFB  = 5,
     CRYPTO_MODE_AES_CHINADRM_CBC  = 0x100
}DrmCryptoMode;

typedef struct {
    void *value;
    uint32_t value_size;
} DrmCryptoInfo;

typedef struct {
     uint32_t *num_cleardata;
     uint32_t *num_encrypteddata;
     uint32_t num_subsamples;
} DrmBufferInfo;

typedef struct HLSDecryptor{
    int32_t encrypt_level;  //DRM_CRYPTO_ENCRYPT_ES_BASED / DRM_CRYPTO_ENCRYPT_CONTAINER_BASED
    DrmCryptoMode crypto_mode;
    ESType    es_type;
    int32_t need_depadding;

    uint8_t key[AES_BLOCK_LENGTH_BYTES];
    uint8_t keyid[AES_BLOCK_LENGTH_BYTES];
    uint8_t iv[AES_BLOCK_LENGTH_BYTES];
    pthread_mutex_t lock_decryptor;

    AVCodecParserContext *apc;

    int32_t (*decrypt)(struct HLSDecryptor *ad, uint8_t* buffer, uint32_t *buffer_size);
    int32_t (*set_cryptoinfo)(struct HLSDecryptor *ad, DrmCryptoInfoType info_type, DrmCryptoInfo *pinfo);
    int32_t (*get_cryptoinfo)(struct HLSDecryptor *ad, DrmCryptoInfoType info_type, DrmCryptoInfo *pinfo);
    int32_t (*parse_cei)(struct HLSDecryptor *ad, uint8_t *buf, int buf_size, uint32_t *out_keyframe_index, uint8_t *out_encryption_flag);
} HLSDecryptor;

HLSDecryptor* hls_decryptor_init(void);
HLSDecryptor* hls_decryptor_init2(AVCodecParserContext *apc, ESType es_type);
void          hls_decryptor_destroy(HLSDecryptor *ad);
void          hls_decryptor_strip03(uint8_t *data, int *size);

#ifdef __cplusplus
    }
#endif
#endif /* AVUTIL_HLSDECRYPTOR_H */
