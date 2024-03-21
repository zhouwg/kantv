/*
 * Copyright 1995-2019 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "crypto/cryptlib.h"
#include "internal/err.h"
#include "crypto/err.h"
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <openssl/bio.h>
#include <openssl/opensslconf.h>
#include "internal/thread_once.h"
#include "crypto/ctype.h"
#include "internal/constant_time.h"
#include "e_os.h"

#ifdef OP_TEE
#include <sys/types.h>

#include <dirent.h>
#endif

static int err_load_strings(const ERR_STRING_DATA *str);

static void ERR_STATE_free(ERR_STATE *s);
#ifndef OPENSSL_NO_ERR
static ERR_STRING_DATA ERR_str_libraries[] = {
    {ERR_PACK(ERR_LIB_NONE, 0, 0), "unknown library"},
    {ERR_PACK(ERR_LIB_SYS, 0, 0), "system library"},
    {ERR_PACK(ERR_LIB_BN, 0, 0), "bignum routines"},
    {ERR_PACK(ERR_LIB_RSA, 0, 0), "rsa routines"},
    {ERR_PACK(ERR_LIB_DH, 0, 0), "Diffie-Hellman routines"},
    {ERR_PACK(ERR_LIB_EVP, 0, 0), "digital envelope routines"},
    {ERR_PACK(ERR_LIB_BUF, 0, 0), "memory buffer routines"},
    {ERR_PACK(ERR_LIB_OBJ, 0, 0), "object identifier routines"},
    {ERR_PACK(ERR_LIB_PEM, 0, 0), "PEM routines"},
    {ERR_PACK(ERR_LIB_DSA, 0, 0), "dsa routines"},
    {ERR_PACK(ERR_LIB_X509, 0, 0), "x509 certificate routines"},
    {ERR_PACK(ERR_LIB_ASN1, 0, 0), "asn1 encoding routines"},
    {ERR_PACK(ERR_LIB_CONF, 0, 0), "configuration file routines"},
    {ERR_PACK(ERR_LIB_CRYPTO, 0, 0), "common libcrypto routines"},
    {ERR_PACK(ERR_LIB_EC, 0, 0), "elliptic curve routines"},
    {ERR_PACK(ERR_LIB_ECDSA, 0, 0), "ECDSA routines"},
    {ERR_PACK(ERR_LIB_ECDH, 0, 0), "ECDH routines"},
    {ERR_PACK(ERR_LIB_SSL, 0, 0), "SSL routines"},
    {ERR_PACK(ERR_LIB_BIO, 0, 0), "BIO routines"},
    {ERR_PACK(ERR_LIB_PKCS7, 0, 0), "PKCS7 routines"},
    {ERR_PACK(ERR_LIB_X509V3, 0, 0), "X509 V3 routines"},
    {ERR_PACK(ERR_LIB_PKCS12, 0, 0), "PKCS12 routines"},
    {ERR_PACK(ERR_LIB_RAND, 0, 0), "random number generator"},
    {ERR_PACK(ERR_LIB_DSO, 0, 0), "DSO support routines"},
    {ERR_PACK(ERR_LIB_TS, 0, 0), "time stamp routines"},
    {ERR_PACK(ERR_LIB_ENGINE, 0, 0), "engine routines"},
    {ERR_PACK(ERR_LIB_OCSP, 0, 0), "OCSP routines"},
    {ERR_PACK(ERR_LIB_UI, 0, 0), "UI routines"},
    {ERR_PACK(ERR_LIB_FIPS, 0, 0), "FIPS routines"},
    {ERR_PACK(ERR_LIB_CMS, 0, 0), "CMS routines"},
    {ERR_PACK(ERR_LIB_HMAC, 0, 0), "HMAC routines"},
    {ERR_PACK(ERR_LIB_CT, 0, 0), "CT routines"},
    {ERR_PACK(ERR_LIB_ASYNC, 0, 0), "ASYNC routines"},
    {ERR_PACK(ERR_LIB_KDF, 0, 0), "KDF routines"},
    {ERR_PACK(ERR_LIB_OSSL_STORE, 0, 0), "STORE routines"},
    {ERR_PACK(ERR_LIB_SM2, 0, 0), "SM2 routines"},
    {0, NULL},
};

static ERR_STRING_DATA ERR_str_functs[] = {
    {ERR_PACK(0, SYS_F_FOPEN, 0), "fopen"},
    {ERR_PACK(0, SYS_F_CONNECT, 0), "connect"},
    {ERR_PACK(0, SYS_F_GETSERVBYNAME, 0), "getservbyname"},
    {ERR_PACK(0, SYS_F_SOCKET, 0), "socket"},
    {ERR_PACK(0, SYS_F_IOCTLSOCKET, 0), "ioctlsocket"},
    {ERR_PACK(0, SYS_F_BIND, 0), "bind"},
    {ERR_PACK(0, SYS_F_LISTEN, 0), "listen"},
    {ERR_PACK(0, SYS_F_ACCEPT, 0), "accept"},
# ifdef OPENSSL_SYS_WINDOWS
    {ERR_PACK(0, SYS_F_WSASTARTUP, 0), "WSAstartup"},
# endif
    {ERR_PACK(0, SYS_F_OPENDIR, 0), "opendir"},
    {ERR_PACK(0, SYS_F_FREAD, 0), "fread"},
    {ERR_PACK(0, SYS_F_GETADDRINFO, 0), "getaddrinfo"},
    {ERR_PACK(0, SYS_F_GETNAMEINFO, 0), "getnameinfo"},
    {ERR_PACK(0, SYS_F_SETSOCKOPT, 0), "setsockopt"},
    {ERR_PACK(0, SYS_F_GETSOCKOPT, 0), "getsockopt"},
    {ERR_PACK(0, SYS_F_GETSOCKNAME, 0), "getsockname"},
    {ERR_PACK(0, SYS_F_GETHOSTBYNAME, 0), "gethostbyname"},
    {ERR_PACK(0, SYS_F_FFLUSH, 0), "fflush"},
    {ERR_PACK(0, SYS_F_OPEN, 0), "open"},
    {ERR_PACK(0, SYS_F_CLOSE, 0), "close"},
    {ERR_PACK(0, SYS_F_IOCTL, 0), "ioctl"},
    {ERR_PACK(0, SYS_F_STAT, 0), "stat"},
    {ERR_PACK(0, SYS_F_FCNTL, 0), "fcntl"},
    {ERR_PACK(0, SYS_F_FSTAT, 0), "fstat"},
    {0, NULL},
};

static ERR_STRING_DATA ERR_str_reasons[] = {
    {ERR_R_SYS_LIB, "system lib"},
    {ERR_R_BN_LIB, "BN lib"},
    {ERR_R_RSA_LIB, "RSA lib"},
    {ERR_R_DH_LIB, "DH lib"},
    {ERR_R_EVP_LIB, "EVP lib"},
    {ERR_R_BUF_LIB, "BUF lib"},
    {ERR_R_OBJ_LIB, "OBJ lib"},
    {ERR_R_PEM_LIB, "PEM lib"},
    {ERR_R_DSA_LIB, "DSA lib"},
    {ERR_R_X509_LIB, "X509 lib"},
    {ERR_R_ASN1_LIB, "ASN1 lib"},
    {ERR_R_EC_LIB, "EC lib"},
    {ERR_R_BIO_LIB, "BIO lib"},
    {ERR_R_PKCS7_LIB, "PKCS7 lib"},
    {ERR_R_X509V3_LIB, "X509V3 lib"},
    {ERR_R_ENGINE_LIB, "ENGINE lib"},
    {ERR_R_UI_LIB, "UI lib"},
    {ERR_R_OSSL_STORE_LIB, "STORE lib"},
    {ERR_R_ECDSA_LIB, "ECDSA lib"},

    {ERR_R_NESTED_ASN1_ERROR, "nested asn1 error"},
    {ERR_R_MISSING_ASN1_EOS, "missing asn1 eos"},

    {ERR_R_FATAL, "fatal"},
    {ERR_R_MALLOC_FAILURE, "malloc failure"},
    {ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED,
     "called a function you should not call"},
    {ERR_R_PASSED_NULL_PARAMETER, "passed a null parameter"},
    {ERR_R_INTERNAL_ERROR, "internal error"},
    {ERR_R_DISABLED, "called a function that was disabled at compile-time"},
    {ERR_R_INIT_FAIL, "init fail"},
    {ERR_R_OPERATION_FAIL, "operation fail"},

    {0, NULL},
};
#endif

static CRYPTO_ONCE err_init = CRYPTO_ONCE_STATIC_INIT;
static int set_err_thread_local;
static CRYPTO_THREAD_LOCAL err_thread_local;

static CRYPTO_ONCE err_string_init = CRYPTO_ONCE_STATIC_INIT;
static CRYPTO_RWLOCK *err_string_lock;

static ERR_STRING_DATA *int_err_get_item(const ERR_STRING_DATA *);

/*
 * The internal state
 */

static LHASH_OF(ERR_STRING_DATA) *int_error_hash = NULL;
static int int_err_library_number = ERR_LIB_USER;

static unsigned long get_error_values(int inc, int top, const char **file,
                                      int *line, const char **data,
                                      int *flags);

static unsigned long err_string_data_hash(const ERR_STRING_DATA *a)
{
    unsigned long ret, l;

    l = a->error;
    ret = l ^ ERR_GET_LIB(l) ^ ERR_GET_FUNC(l);
    return (ret ^ ret % 19 * 13);
}

static int err_string_data_cmp(const ERR_STRING_DATA *a,
                               const ERR_STRING_DATA *b)
{
    if (a->error == b->error)
        return 0;
    return a->error > b->error ? 1 : -1;
}

static ERR_STRING_DATA *int_err_get_item(const ERR_STRING_DATA *d)
{
    ERR_STRING_DATA *p = NULL;

    CRYPTO_THREAD_read_lock(err_string_lock);
    p = lh_ERR_STRING_DATA_retrieve(int_error_hash, d);
    CRYPTO_THREAD_unlock(err_string_lock);

    return p;
}

#ifndef OPENSSL_NO_ERR
/* 2019-05-21: Russian and Ukrainian locales on Linux require more than 6,5 kB */
# define SPACE_SYS_STR_REASONS 8 * 1024
# define NUM_SYS_STR_REASONS 127

static ERR_STRING_DATA SYS_str_reasons[NUM_SYS_STR_REASONS + 1];
/*
 * SYS_str_reasons is filled with copies of strerror() results at
 * initialization. 'errno' values up to 127 should cover all usual errors,
 * others will be displayed numerically by ERR_error_string. It is crucial
 * that we have something for each reason code that occurs in
 * ERR_str_reasons, or bogus reason strings will be returned for SYSerr(),
 * which always gets an errno value and never one of those 'standard' reason
 * codes.
 */

static void build_SYS_str_reasons(void)
{
    /* OPENSSL_malloc cannot be used here, use static storage instead */
    static char strerror_pool[SPACE_SYS_STR_REASONS];
    char *cur = strerror_pool;
    size_t cnt = 0;
    static int init = 1;
    int i;
    int saveerrno = get_last_sys_error();

    CRYPTO_THREAD_write_lock(err_string_lock);
    if (!init) {
        CRYPTO_THREAD_unlock(err_string_lock);
        return;
    }

    for (i = 1; i <= NUM_SYS_STR_REASONS; i++) {
        ERR_STRING_DATA *str = &SYS_str_reasons[i - 1];

        str->error = ERR_PACK(ERR_LIB_SYS, 0, i);
        /*
         * If we have used up all the space in strerror_pool,
         * there's no point in calling openssl_strerror_r()
         */
        if (str->string == NULL && cnt < sizeof(strerror_pool)) {
            if (openssl_strerror_r(i, cur, sizeof(strerror_pool) - cnt)) {
                size_t l = strlen(cur);

                str->string = cur;
                cnt += l;
                cur += l;

                /*
                 * VMS has an unusual quirk of adding spaces at the end of
                 * some (most? all?) messages. Lets trim them off.
                 */
                while (cur > strerror_pool && ossl_isspace(cur[-1])) {
                    cur--;
                    cnt--;
                }
                *cur++ = '\0';
                cnt++;
            }
        }
        if (str->string == NULL)
            str->string = "unknown";
    }

    /*
     * Now we still have SYS_str_reasons[NUM_SYS_STR_REASONS] = {0, NULL}, as
     * required by ERR_load_strings.
     */

    init = 0;

    CRYPTO_THREAD_unlock(err_string_lock);
    /* openssl_strerror_r could change errno, but we want to preserve it */
    set_sys_error(saveerrno);
    err_load_strings(SYS_str_reasons);
}
#endif

#define err_clear_data(p, i) \
        do { \
            if ((p)->err_data_flags[i] & ERR_TXT_MALLOCED) {\
                OPENSSL_free((p)->err_data[i]); \
                (p)->err_data[i] = NULL; \
            } \
            (p)->err_data_flags[i] = 0; \
        } while (0)

#define err_clear(p, i) \
        do { \
            err_clear_data(p, i); \
            (p)->err_flags[i] = 0; \
            (p)->err_buffer[i] = 0; \
            (p)->err_file[i] = NULL; \
            (p)->err_line[i] = -1; \
        } while (0)

static void ERR_STATE_free(ERR_STATE *s)
{
    int i;

    if (s == NULL)
        return;
    for (i = 0; i < ERR_NUM_ERRORS; i++) {
        err_clear_data(s, i);
    }
    OPENSSL_free(s);
}

DEFINE_RUN_ONCE_STATIC(do_err_strings_init)
{
    if (!OPENSSL_init_crypto(0, NULL))
        return 0;
    err_string_lock = CRYPTO_THREAD_lock_new();
    if (err_string_lock == NULL)
        return 0;
    int_error_hash = lh_ERR_STRING_DATA_new(err_string_data_hash,
                                            err_string_data_cmp);
    if (int_error_hash == NULL) {
        CRYPTO_THREAD_lock_free(err_string_lock);
        err_string_lock = NULL;
        return 0;
    }
    return 1;
}

void err_cleanup(void)
{
    if (set_err_thread_local != 0)
        CRYPTO_THREAD_cleanup_local(&err_thread_local);
    CRYPTO_THREAD_lock_free(err_string_lock);
    err_string_lock = NULL;
    lh_ERR_STRING_DATA_free(int_error_hash);
    int_error_hash = NULL;
}

/*
 * Legacy; pack in the library.
 */
static void err_patch(int lib, ERR_STRING_DATA *str)
{
    unsigned long plib = ERR_PACK(lib, 0, 0);

    for (; str->error != 0; str++)
        str->error |= plib;
}

/*
 * Hash in |str| error strings. Assumes the URN_ONCE was done.
 */
static int err_load_strings(const ERR_STRING_DATA *str)
{
    CRYPTO_THREAD_write_lock(err_string_lock);
    for (; str->error; str++)
        (void)lh_ERR_STRING_DATA_insert(int_error_hash,
                                       (ERR_STRING_DATA *)str);
    CRYPTO_THREAD_unlock(err_string_lock);
    return 1;
}

int ERR_load_ERR_strings(void)
{
#ifndef OPENSSL_NO_ERR
    if (!RUN_ONCE(&err_string_init, do_err_strings_init))
        return 0;

    err_load_strings(ERR_str_libraries);
    err_load_strings(ERR_str_reasons);
    err_patch(ERR_LIB_SYS, ERR_str_functs);
    err_load_strings(ERR_str_functs);
    build_SYS_str_reasons();
#endif
    return 1;
}

int ERR_load_strings(int lib, ERR_STRING_DATA *str)
{
    if (ERR_load_ERR_strings() == 0)
        return 0;

    err_patch(lib, str);
    err_load_strings(str);
    return 1;
}

int ERR_load_strings_const(const ERR_STRING_DATA *str)
{
    if (ERR_load_ERR_strings() == 0)
        return 0;
    err_load_strings(str);
    return 1;
}

int ERR_unload_strings(int lib, ERR_STRING_DATA *str)
{
    if (!RUN_ONCE(&err_string_init, do_err_strings_init))
        return 0;

    CRYPTO_THREAD_write_lock(err_string_lock);
    /*
     * We don't need to ERR_PACK the lib, since that was done (to
     * the table) when it was loaded.
     */
    for (; str->error; str++)
        (void)lh_ERR_STRING_DATA_delete(int_error_hash, str);
    CRYPTO_THREAD_unlock(err_string_lock);

    return 1;
}

void err_free_strings_int(void)
{
    if (!RUN_ONCE(&err_string_init, do_err_strings_init))
        return;
}

/********************************************************/

void ERR_put_error(int lib, int func, int reason, const char *file, int line)
{
    ERR_STATE *es;

#ifdef _OSD_POSIX
    /*
     * In the BS2000-OSD POSIX subsystem, the compiler generates path names
     * in the form "*POSIX(/etc/passwd)". This dirty hack strips them to
     * something sensible. @@@ We shouldn't modify a const string, though.
     */
    if (strncmp(file, "*POSIX(", sizeof("*POSIX(") - 1) == 0) {
        char *end;

        /* Skip the "*POSIX(" prefix */
        file += sizeof("*POSIX(") - 1;
        end = &file[strlen(file) - 1];
        if (*end == ')')
            *end = '\0';
        /* Optional: use the basename of the path only. */
        if ((end = ijk_strrchr(file, '/')) != NULL)
            file = &end[1];
    }
#endif
    es = ERR_get_state();
    if (es == NULL)
        return;

    es->top = (es->top + 1) % ERR_NUM_ERRORS;
    if (es->top == es->bottom)
        es->bottom = (es->bottom + 1) % ERR_NUM_ERRORS;
    es->err_flags[es->top] = 0;
    es->err_buffer[es->top] = ERR_PACK(lib, func, reason);
    es->err_file[es->top] = file;
    es->err_line[es->top] = line;
    err_clear_data(es, es->top);
}

void ERR_clear_error(void)
{
    int i;
    ERR_STATE *es;

    es = ERR_get_state();
    if (es == NULL)
        return;

    for (i = 0; i < ERR_NUM_ERRORS; i++) {
        err_clear(es, i);
    }
    es->top = es->bottom = 0;
}

unsigned long ERR_get_error(void)
{
    return get_error_values(1, 0, NULL, NULL, NULL, NULL);
}

unsigned long ERR_get_error_line(const char **file, int *line)
{
    return get_error_values(1, 0, file, line, NULL, NULL);
}

unsigned long ERR_get_error_line_data(const char **file, int *line,
                                      const char **data, int *flags)
{
    return get_error_values(1, 0, file, line, data, flags);
}

unsigned long ERR_peek_error(void)
{
    return get_error_values(0, 0, NULL, NULL, NULL, NULL);
}

unsigned long ERR_peek_error_line(const char **file, int *line)
{
    return get_error_values(0, 0, file, line, NULL, NULL);
}

unsigned long ERR_peek_error_line_data(const char **file, int *line,
                                       const char **data, int *flags)
{
    return get_error_values(0, 0, file, line, data, flags);
}

unsigned long ERR_peek_last_error(void)
{
    return get_error_values(0, 1, NULL, NULL, NULL, NULL);
}

unsigned long ERR_peek_last_error_line(const char **file, int *line)
{
    return get_error_values(0, 1, file, line, NULL, NULL);
}

unsigned long ERR_peek_last_error_line_data(const char **file, int *line,
                                            const char **data, int *flags)
{
    return get_error_values(0, 1, file, line, data, flags);
}

static unsigned long get_error_values(int inc, int top, const char **file,
                                      int *line, const char **data,
                                      int *flags)
{
    int i = 0;
    ERR_STATE *es;
    unsigned long ret;

    es = ERR_get_state();
    if (es == NULL)
        return 0;

    if (inc && top) {
        if (file)
            *file = "";
        if (line)
            *line = 0;
        if (data)
            *data = "";
        if (flags)
            *flags = 0;

        return ERR_R_INTERNAL_ERROR;
    }

    while (es->bottom != es->top) {
        if (es->err_flags[es->top] & ERR_FLAG_CLEAR) {
            err_clear(es, es->top);
            es->top = es->top > 0 ? es->top - 1 : ERR_NUM_ERRORS - 1;
            continue;
        }
        i = (es->bottom + 1) % ERR_NUM_ERRORS;
        if (es->err_flags[i] & ERR_FLAG_CLEAR) {
            es->bottom = i;
            err_clear(es, es->bottom);
            continue;
        }
        break;
    }

    if (es->bottom == es->top)
        return 0;

    if (top)
        i = es->top;            /* last error */
    else
        i = (es->bottom + 1) % ERR_NUM_ERRORS; /* first error */

    ret = es->err_buffer[i];
    if (inc) {
        es->bottom = i;
        es->err_buffer[i] = 0;
    }

    if (file != NULL && line != NULL) {
        if (es->err_file[i] == NULL) {
            *file = "NA";
            *line = 0;
        } else {
            *file = es->err_file[i];
            *line = es->err_line[i];
        }
    }

    if (data == NULL) {
        if (inc) {
            err_clear_data(es, i);
        }
    } else {
        if (es->err_data[i] == NULL) {
            *data = "";
            if (flags != NULL)
                *flags = 0;
        } else {
            *data = es->err_data[i];
            if (flags != NULL)
                *flags = es->err_data_flags[i];
        }
    }
    return ret;
}

void ERR_error_string_n(unsigned long e, char *buf, size_t len)
{
    char lsbuf[64], fsbuf[64], rsbuf[64];
    const char *ls, *fs, *rs;
    unsigned long l, f, r;

    if (len == 0)
        return;

    l = ERR_GET_LIB(e);
    ls = ERR_lib_error_string(e);
    if (ls == NULL) {
        BIO_snprintf(lsbuf, sizeof(lsbuf), "lib(%lu)", l);
        ls = lsbuf;
    }

    fs = ERR_func_error_string(e);
    f = ERR_GET_FUNC(e);
    if (fs == NULL) {
        BIO_snprintf(fsbuf, sizeof(fsbuf), "func(%lu)", f);
        fs = fsbuf;
    }

    rs = ERR_reason_error_string(e);
    r = ERR_GET_REASON(e);
    if (rs == NULL) {
        BIO_snprintf(rsbuf, sizeof(rsbuf), "reason(%lu)", r);
        rs = rsbuf;
    }

    BIO_snprintf(buf, len, "error:%08lX:%s:%s:%s", e, ls, fs, rs);
    if (strlen(buf) == len - 1) {
        /* Didn't fit; use a minimal format. */
        BIO_snprintf(buf, len, "err:%lx:%lx:%lx:%lx", e, l, f, r);
    }
}

/*
 * ERR_error_string_n should be used instead for ret != NULL as
 * ERR_error_string cannot know how large the buffer is
 */
char *ERR_error_string(unsigned long e, char *ret)
{
    static char buf[256];

    if (ret == NULL)
        ret = buf;
    ERR_error_string_n(e, ret, (int)sizeof(buf));
    return ret;
}

const char *ERR_lib_error_string(unsigned long e)
{
    ERR_STRING_DATA d, *p;
    unsigned long l;

    if (!RUN_ONCE(&err_string_init, do_err_strings_init)) {
        return NULL;
    }

    l = ERR_GET_LIB(e);
    d.error = ERR_PACK(l, 0, 0);
    p = int_err_get_item(&d);
    return ((p == NULL) ? NULL : p->string);
}

const char *ERR_func_error_string(unsigned long e)
{
    ERR_STRING_DATA d, *p;
    unsigned long l, f;

    if (!RUN_ONCE(&err_string_init, do_err_strings_init)) {
        return NULL;
    }

    l = ERR_GET_LIB(e);
    f = ERR_GET_FUNC(e);
    d.error = ERR_PACK(l, f, 0);
    p = int_err_get_item(&d);
    return ((p == NULL) ? NULL : p->string);
}

const char *ERR_reason_error_string(unsigned long e)
{
    ERR_STRING_DATA d, *p = NULL;
    unsigned long l, r;

    if (!RUN_ONCE(&err_string_init, do_err_strings_init)) {
        return NULL;
    }

    l = ERR_GET_LIB(e);
    r = ERR_GET_REASON(e);
    d.error = ERR_PACK(l, 0, r);
    p = int_err_get_item(&d);
    if (!p) {
        d.error = ERR_PACK(0, 0, r);
        p = int_err_get_item(&d);
    }
    return ((p == NULL) ? NULL : p->string);
}

void err_delete_thread_state(void)
{
    ERR_STATE *state = CRYPTO_THREAD_get_local(&err_thread_local);
    if (state == NULL)
        return;

    CRYPTO_THREAD_set_local(&err_thread_local, NULL);
    ERR_STATE_free(state);
}

#if OPENSSL_API_COMPAT < 0x10100000L
void ERR_remove_thread_state(void *dummy)
{
}
#endif

#if OPENSSL_API_COMPAT < 0x10000000L
void ERR_remove_state(unsigned long pid)
{
}
#endif

DEFINE_RUN_ONCE_STATIC(err_do_init)
{
    set_err_thread_local = 1;
    return CRYPTO_THREAD_init_local(&err_thread_local, NULL);
}

ERR_STATE *ERR_get_state(void)
{
    ERR_STATE *state;
    int saveerrno = get_last_sys_error();

    if (!OPENSSL_init_crypto(OPENSSL_INIT_BASE_ONLY, NULL))
        return NULL;

    if (!RUN_ONCE(&err_init, err_do_init))
        return NULL;

    state = CRYPTO_THREAD_get_local(&err_thread_local);
    if (state == (ERR_STATE*)-1)
        return NULL;

    if (state == NULL) {
        if (!CRYPTO_THREAD_set_local(&err_thread_local, (ERR_STATE*)-1))
            return NULL;

        if ((state = OPENSSL_zalloc(sizeof(*state))) == NULL) {
            CRYPTO_THREAD_set_local(&err_thread_local, NULL);
            return NULL;
        }

        if (!ossl_init_thread_start(OPENSSL_INIT_THREAD_ERR_STATE)
                || !CRYPTO_THREAD_set_local(&err_thread_local, state)) {
            ERR_STATE_free(state);
            CRYPTO_THREAD_set_local(&err_thread_local, NULL);
            return NULL;
        }

        /* Ignore failures from these */
        OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    }

    set_sys_error(saveerrno);
    return state;
}

/*
 * err_shelve_state returns the current thread local error state
 * and freezes the error module until err_unshelve_state is called.
 */
int err_shelve_state(void **state)
{
    int saveerrno = get_last_sys_error();

    /*
     * Note, at present our only caller is OPENSSL_init_crypto(), indirectly
     * via ossl_init_load_crypto_nodelete(), by which point the requested
     * "base" initialization has already been performed, so the below call is a
     * NOOP, that re-enters OPENSSL_init_crypto() only to quickly return.
     *
     * If are no other valid callers of this function, the call below can be
     * removed, avoiding the re-entry into OPENSSL_init_crypto().  If there are
     * potential uses that are not from inside OPENSSL_init_crypto(), then this
     * call is needed, but some care is required to make sure that the re-entry
     * remains a NOOP.
     */
    if (!OPENSSL_init_crypto(OPENSSL_INIT_BASE_ONLY, NULL))
        return 0;

    if (!RUN_ONCE(&err_init, err_do_init))
        return 0;

    *state = CRYPTO_THREAD_get_local(&err_thread_local);
    if (!CRYPTO_THREAD_set_local(&err_thread_local, (ERR_STATE*)-1))
        return 0;

    set_sys_error(saveerrno);
    return 1;
}

/*
 * err_unshelve_state restores the error state that was returned
 * by err_shelve_state previously.
 */
void err_unshelve_state(void* state)
{
    if (state != (void*)-1)
        CRYPTO_THREAD_set_local(&err_thread_local, (ERR_STATE*)state);
}

int ERR_get_next_error_library(void)
{
    int ret;

    if (!RUN_ONCE(&err_string_init, do_err_strings_init))
        return 0;

    CRYPTO_THREAD_write_lock(err_string_lock);
    ret = int_err_library_number++;
    CRYPTO_THREAD_unlock(err_string_lock);
    return ret;
}

static int err_set_error_data_int(char *data, int flags)
{
    ERR_STATE *es;
    int i;

    es = ERR_get_state();
    if (es == NULL)
        return 0;

    i = es->top;

    err_clear_data(es, i);
    es->err_data[i] = data;
    es->err_data_flags[i] = flags;

    return 1;
}

void ERR_set_error_data(char *data, int flags)
{
    /*
     * This function is void so we cannot propagate the error return. Since it
     * is also in the public API we can't change the return type.
     */
    err_set_error_data_int(data, flags);
}

void ERR_add_error_data(int num, ...)
{
    va_list args;
    va_start(args, num);
    ERR_add_error_vdata(num, args);
    va_end(args);
}

void ERR_add_error_vdata(int num, va_list args)
{
    int i, n, s;
    char *str, *p, *a;

    s = 80;
    if ((str = OPENSSL_malloc(s + 1)) == NULL) {
        /* ERRerr(ERR_F_ERR_ADD_ERROR_VDATA, ERR_R_MALLOC_FAILURE); */
        return;
    }
    str[0] = '\0';

    n = 0;
    for (i = 0; i < num; i++) {
        a = va_arg(args, char *);
        if (a == NULL)
            a = "<NULL>";
        n += strlen(a);
        if (n > s) {
            s = n + 20;
            p = OPENSSL_realloc(str, s + 1);
            if (p == NULL) {
                OPENSSL_free(str);
                return;
            }
            str = p;
        }
        OPENSSL_strlcat(str, a, (size_t)s + 1);
    }
    if (!err_set_error_data_int(str, ERR_TXT_MALLOCED | ERR_TXT_STRING))
        OPENSSL_free(str);
}

int ERR_set_mark(void)
{
    ERR_STATE *es;

    es = ERR_get_state();
    if (es == NULL)
        return 0;

    if (es->bottom == es->top)
        return 0;
    es->err_flags[es->top] |= ERR_FLAG_MARK;
    return 1;
}

int ERR_pop_to_mark(void)
{
    ERR_STATE *es;

    es = ERR_get_state();
    if (es == NULL)
        return 0;

    while (es->bottom != es->top
           && (es->err_flags[es->top] & ERR_FLAG_MARK) == 0) {
        err_clear(es, es->top);
        es->top = es->top > 0 ? es->top - 1 : ERR_NUM_ERRORS - 1;
    }

    if (es->bottom == es->top)
        return 0;
    es->err_flags[es->top] &= ~ERR_FLAG_MARK;
    return 1;
}

int ERR_clear_last_mark(void)
{
    ERR_STATE *es;
    int top;

    es = ERR_get_state();
    if (es == NULL)
        return 0;

    top = es->top;
    while (es->bottom != top
           && (es->err_flags[top] & ERR_FLAG_MARK) == 0) {
        top = top > 0 ? top - 1 : ERR_NUM_ERRORS - 1;
    }

    if (es->bottom == top)
        return 0;
    es->err_flags[top] &= ~ERR_FLAG_MARK;
    return 1;
}

void err_clear_last_constant_time(int clear)
{
    ERR_STATE *es;
    int top;

    es = ERR_get_state();
    if (es == NULL)
        return;

    top = es->top;

    /*
     * Flag error as cleared but remove it elsewhere to avoid two errors
     * accessing the same error stack location, revealing timing information.
     */
    clear = constant_time_select_int(constant_time_eq_int(clear, 0),
                                     0, ERR_FLAG_CLEAR);
    es->err_flags[top] |= clear;
}

#ifdef OP_TEE
char *ijk_strrchr(const  char *s, char c)
{
	const char *p;

	p = NULL;
	do {
		if (*s == (char) c) {
			p = s;
		}
	} while (*s++);

	return (char *) p;			/* silence the warning */
}

char *ijk_strdup(const char *s1)
{
	char *s;
	size_t l = (strlen(s1) + 1) * sizeof(char);

	if ((s = malloc(l)) != NULL) {
		memcpy(s, s1, l);
	}

	return s;
}

size_t ijk_strspn(const char *str, const char *accept)
{
    if (accept[0] == '\0')
        return 0;
    if ((accept[1] == '\0'))
    {
        const char *a = str;
        for (; *str == *accept; str++)
            ;
        return str - a;
    }

    return 0;
}


char *
ijk_strchrnul(const char *s, int c_in)
{
    const unsigned char *char_ptr;
    unsigned char c;

    c = (unsigned char)c_in;

    /* Handle the first few characters by reading one character at a time.
     Do this until CHAR_PTR is aligned on a longword boundary.  */
    for (char_ptr = (const unsigned char *)s;
         ((unsigned long int)char_ptr & (sizeof(long) - 1)) != 0;
         ++char_ptr)
        if (*char_ptr == c || *char_ptr == '\0')
            return (void *)char_ptr;

    return 0;
}


size_t ijk_strcspn(const char *str, const char *reject)
{
    if ((reject[0] == '\0') || (reject[1] == '\0'))
        return ijk_strchrnul(str, reject[0]) - str;

    return 0;
}

int
__xstat (int vers, const char *name, struct stat *buf)
{
    return -1;
}

int
__fxstat (int vers, int fd, struct stat *buf)
{
    return -1;
}
int __ctype_b_loc = 0;

      int stat(const char *pathname, struct stat *statbuf)
      {
          return -1;
      }
       int fstat(int fd, struct stat *statbuf)
       {
           return -1;
       }
       int lstat(const char *pathname, struct stat *statbuf)
       {
           return -1;
       }

int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout)
                  {
                      return 0;
                  }
int shmdt(const void *shmaddr)
{
    return -1;
}
int shmget(key_t key, size_t size, int shmflg)
{
    return -1;
}

void *shmat(int shmid, const void *shmaddr, int shmflg)
{
    return -1;
}

struct dirent *readdir(DIR *dirp)
{
    return NULL;
}

int closedir(DIR *dirp)
{
    return -1;
}
ssize_t read(int fd, void *buf, size_t count)
{
    return 0;
}
int uname(struct utsname *buf)
{
    return -1;
}
int open(const char *pathname, int flags)
{
    return -1;
}
long syscall(long number, ...)
{
    return 1;
}
char *secure_getenv(const char *name)
{
    return NULL;
}
DIR *opendir(const char *name)
{
    return NULL;
}

int close(int fd)
{
    return  0;
}
pid_t getpid(void)
{
    return 0x333;
}

int isSpace(int c)
{
    int ret = 0;

    switch (c) {
    case ' ': /* (0x20)    space (SPC) */
    case '\t': /* (0x09)    horizontal tab (TAB) */
    case '\n': /* (0x0a)    newline (LF) */
    case '\v': /* (0x0b)    vertical tab (VT) */
    case '\f': /* (0x0c)    feed (FF) */
    case '\r': /* (0x0d)    carriage return (CR) */
        ret = 1;
        break;
    }

    return ret;
}

static int gErrorCode = 0;
int * __errno_location()
{
    return &gErrorCode;
}

static void (*globalExitFunction)() = NULL;
int atexit(void (*function)(void))
{
    globalExitFunction = function;
    return 0;
}



typedef struct {
	uint32_t seconds;
	uint32_t millis;
} TEE_Time;
void TEE_GetREETime(TEE_Time *time);

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    TEE_Time teeTime;
    TEE_GetREETime(&teeTime);

    //printf("SDS-uniform-drmclient in clock_gettime");

    tp->tv_sec = teeTime.seconds;
    tp->tv_nsec = teeTime.millis*1000;
    
    return 0;
}

char *strcat(char *dest, const char *src)
{
  strcpy (dest + strlen (dest), src);
  return dest;
}

char *strncat(char *s1, const char *s2, size_t n)
{
  char *s = s1;

  /* Find the end of S1.  */
  s1 += strlen (s1);

  size_t ss = strnlen (s2, n);

  s1[ss] = '\0';
  memcpy (s1, s2, ss);

  return s;
}

static int toLower(int c)
{

    int t = c;
    if (t >= 'A' && t <= 'Z')
    {
        t -= 'A';
        t += 'a';
    }
    return t;
}

int isAlpha(int c)
{
    if(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'))){
        return 1;
    }

    return 0;
}
static int toUpper(int c)
{
    int t = c;

    if (t >= 'a' && t <= 'z')
    {
        t -= 'a';
        t += 'A';
    }
    return t;
}

int strcasecmp(const char *s1, const char *s2)
{
  const unsigned char *p1 = (const unsigned char *) s1;
  const unsigned char *p2 = (const unsigned char *) s2;
  int result;

  if (p1 == p2)
    return 0;

  while ((result = toLower (*p1) - toLower (*p2++)) == 0)
    if (*p1++ == '\0')
      break;

  return result;
}


int strncasecmp(const char *s1, const char *s2, size_t n)
{
  const unsigned char *p1 = (const unsigned char *) s1;
  const unsigned char *p2 = (const unsigned char *) s2;
  int result;

  if (p1 == p2 || n == 0)
    return 0;

  while ((result = toLower (*p1) - toLower (*p2++)) == 0)
    if (*p1++ == '\0' || --n == 0)
      break;

  return result;
}

int isSpace(int c);
long int strtol(const char *nptr, char **endptr, int base)
{
    int negative;
    unsigned long int i;
    const char *s;
    char c;
    const char *save, *end;

    if (base < 0 || base == 1 || base > 36)
    {
        return 0;
    }

    save = s = nptr;

    /* Skip white space.  */
    while (isSpace(*s))
        ++s;

    /* Check for a sign.  */
    negative = 0;
    if (*s == '-')
    {
        negative = 1;
        ++s;
    }
    else if (*s == '+')
        ++s;

    /* Recognize number prefix and if BASE is zero, figure it out ourselves.  */
    if (*s == '0')
    {
        if ((base == 0 || base == 16) && toUpper(s[1]) == 'X')
        {
            s += 2;
            base = 16;
        }
        else if (base == 0)
            base = 8;
    }
    else if (base == 0)
        base = 10;

    /* Save the pointer so we can check later if anything happened.  */
    save = s;
    end = NULL;

    i = 0;
    c = *s;
    for (; c != '\0'; c = *++s)
    {
        if (s == end)
            break;
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (isAlpha(c))
            c = toUpper(c) - 'A' + 10;
        else
            break;
        if ((int)c >= base)
            break;

        i *= (unsigned long int)base;
        i += c;
    }

    /* Check if anything actually happened.  */
    if (s == save)
        goto noconv;

    /* Store in ENDPTR the address of one character
     past the last character we converted.  */
    if (endptr != NULL)
        *endptr = (char *)s;

    /* Return the result of the appropriate sign.  */
    return negative ? -i : i;

noconv:
    /* We must handle a special case here: the base is 0 or 16 and the
     first two characters are '0' and 'x', but the rest are no
     hexadecimal digits.  This is no error case.  We return 0 and
     ENDPTR points to the `x`.  */
    *endptr = (char *)nptr;
    return 0L;
}





char *stpcpy(char *dest, const char *src)
{
  size_t len = strlen (src);

  return memcpy (dest, src, len + 1) + len;
}

time_t time(time_t *tloc)
{
    TEE_Time teeTime;
    TEE_GetREETime(&teeTime);

    if(tloc)
    *tloc = teeTime.seconds;
    
    return teeTime.seconds;
}

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    TEE_Time teeTime;
    TEE_GetREETime(&teeTime);

    //printf("SDS-uniform-drmclient in gettimeofday");
    tv->tv_sec = teeTime.seconds;
    tv->tv_usec = teeTime.millis*1000;

    return 0;
}

char *__tzname[2] = { (char *) "GMT", (char *) "GMT" };
/* This structure contains all the information about a
   timezone given in the POSIX standard TZ envariable.  */
typedef struct
  {
    const char *name;

    /* When to change.  */
    enum { J0, J1, M } type;	/* Interpretation of:  */
    unsigned short int m, n, d;	/* Month, week, day.  */
    int secs;			/* Time of day.  */

    int offset;			/* Seconds east of GMT (west if < 0).  */

    /* We cache the computed time of change for a
       given year so we don't have to recompute it.  */
    time_t change;	/* When to change to this zone.  */
    int computed_for;	/* Year above is computed for.  */
  } tz_rule;

static const unsigned short int __mon_yday[2][13] =
  {
    /* Normal years.  */
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    /* Leap years.  */
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
  };

#define SECS_PER_HOUR (60 * 60)
#define SECS_PER_DAY (SECS_PER_HOUR * 24)

/* Compute the `struct tm' representation of T,
   offset OFFSET seconds east of UTC,
   and store year, yday, mon, mday, wday, hour, min, sec into *TP.
   Return nonzero if successful.  */
int __offtime(time_t t, long int offset, struct tm *tp)
{
    time_t days, rem, y;
    const unsigned short int *ip;


    days = t / SECS_PER_DAY;
    rem = t % SECS_PER_DAY;
    rem += offset;
    while (rem < 0)
    {
        rem += SECS_PER_DAY;
        --days;
    }
    while (rem >= SECS_PER_DAY)
    {
        rem -= SECS_PER_DAY;
        ++days;
    }
    tp->tm_hour = rem / SECS_PER_HOUR;
    rem %= SECS_PER_HOUR;
    tp->tm_min = rem / 60;
    tp->tm_sec = rem % 60;
    /* January 1, 1970 was a Thursday.  */
    tp->tm_wday = (4 + days) % 7;
    if (tp->tm_wday < 0)
        tp->tm_wday += 7;
    y = 1970;

#define DIV(a, b) ((a) / (b) - ((a) % (b) < 0))
#define LEAPS_THRU_END_OF(y) (DIV(y, 4) - DIV(y, 100) + DIV(y, 400))

    while (days < 0 || days >= (__isleap(y) ? 366 : 365))
    {
        /* Guess a corrected year, assuming 365 days per year.  */
        time_t yg = y + days / 365 - (days % 365 < 0);

        /* Adjust DAYS and Y to match the guessed year.  */
        days -= ((yg - y) * 365 + LEAPS_THRU_END_OF(yg - 1) - LEAPS_THRU_END_OF(y - 1));
        y = yg;
    }
    tp->tm_year = y - 1900;
    if (tp->tm_year != y - 1900)
    {
        /* The year cannot be represented due to overflow.  */
        return 0;
    }
    tp->tm_yday = days;
    ip = __mon_yday[__isleap(y)];
    for (y = 11; days < (long int)ip[y]; --y)
        continue;
    days -= ip[y];
    tp->tm_mon = y;
    tp->tm_mday = days + 1;
    return 1;
}

# define __builtin_expect(expr, val) (expr)

#define SECSPERDAY ((time_t) 86400)
/* Figure out the exact time (as a time_t) in YEAR
   when the change described by RULE will occur and
   put it in RULE->change, saving YEAR in RULE->computed_for.  */
static void
compute_change (tz_rule *rule, int year)
{
  time_t t;

  if (year != -1 && rule->computed_for == year)
    /* Operations on times in 2 BC will be slower.  Oh well.  */
    return;

  /* First set T to January 1st, 0:00:00 GMT in YEAR.  */
  if (year > 1970)
    t = ((year - 1970) * 365
	 + /* Compute the number of leapdays between 1970 and YEAR
	      (exclusive).  There is a leapday every 4th year ...  */
	 + ((year - 1) / 4 - 1970 / 4)
	 /* ... except every 100th year ... */
	 - ((year - 1) / 100 - 1970 / 100)
	 /* ... but still every 400th year.  */
	 + ((year - 1) / 400 - 1970 / 400)) * SECSPERDAY;
  else
    t = 0;

  switch (rule->type)
    {
    case J1:
      /* Jn - Julian day, 1 == January 1, 60 == March 1 even in leap years.
	 In non-leap years, or if the day number is 59 or less, just
	 add SECSPERDAY times the day number-1 to the time of
	 January 1, midnight, to get the day.  */
      t += (rule->d - 1) * SECSPERDAY;
      if (rule->d >= 60 && __isleap (year))
	t += SECSPERDAY;
      break;

    case J0:
      /* n - Day of year.
	 Just add SECSPERDAY times the day number to the time of Jan 1st.  */
      t += rule->d * SECSPERDAY;
      break;

    case M:
      /* Mm.n.d - Nth "Dth day" of month M.  */
      {
	unsigned int i;
	int d, m1, yy0, yy1, yy2, dow;
	const unsigned short int *myday =
	  &__mon_yday[__isleap (year)][rule->m];

	/* First add SECSPERDAY for each day in months before M.  */
	t += myday[-1] * SECSPERDAY;

	/* Use Zeller's Congruence to get day-of-week of first day of month. */
	m1 = (rule->m + 9) % 12 + 1;
	yy0 = (rule->m <= 2) ? (year - 1) : year;
	yy1 = yy0 / 100;
	yy2 = yy0 % 100;
	dow = ((26 * m1 - 2) / 10 + 1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
	if (dow < 0)
	  dow += 7;

	/* DOW is the day-of-week of the first day of the month.  Get the
	   day-of-month (zero-origin) of the first DOW day of the month.  */
	d = rule->d - dow;
	if (d < 0)
	  d += 7;
	for (i = 1; i < rule->n; ++i)
	  {
	    if (d + 7 >= (int) myday[0] - myday[-1])
	      break;
	    d += 7;
	  }

	/* D is the day-of-month (zero-origin) of the day we want.  */
	t += d * SECSPERDAY;
      }
      break;
    }

  /* T is now the Epoch-relative time of 0:00:00 GMT on the day we want.
     Just add the time of day and local offset from GMT, and we're done.  */

  rule->change = t - rule->offset + rule->secs;
  rule->computed_for = year;
}

/* tz_rules[0] is standard, tz_rules[1] is daylight.  */
static tz_rule tz_rules[2];
void
__tz_compute (time_t timer, struct tm *tm, int use_localtime)
{
  compute_change (&tz_rules[0], 1900 + tm->tm_year);
  compute_change (&tz_rules[1], 1900 + tm->tm_year);

  if (use_localtime)
    {
      int isdst;

      /* We have to distinguish between northern and southern
	 hemisphere.  For the latter the daylight saving time
	 ends in the next year.  */
      if (__builtin_expect (tz_rules[0].change
			    > tz_rules[1].change, 0))
	isdst = (timer < tz_rules[1].change
		 || timer >= tz_rules[0].change);
      else
	isdst = (timer >= tz_rules[0].change
		 && timer < tz_rules[1].change);
      tm->tm_isdst = isdst;
      tm->tm_zone = __tzname[isdst];
      tm->tm_gmtoff = tz_rules[isdst].offset;
    }
}

struct tm *
__tz_convert(time_t timer, int use_localtime, struct tm *tp)
{
    long int leap_correction;
    int leap_extra_secs;

    if (!__offtime(timer, 0, tp))
        tp = NULL;
    else
        __tz_compute(timer, tp, use_localtime);
    leap_correction = 0L;
    leap_extra_secs = 0;

    if (tp)
    {
        if (!use_localtime)
        {
            tp->tm_isdst = 0;
            tp->tm_zone = "GMT";
            tp->tm_gmtoff = 0L;
        }

        if (__offtime(timer, tp->tm_gmtoff - leap_correction, tp))
            tp->tm_sec += leap_extra_secs;
        else
            tp = NULL;
    }

    return tp;
}

static struct tm globalTm;
struct tm *gmtime(const time_t *time)
{
    return __tz_convert(*time, 0, &globalTm);
}


#endif
