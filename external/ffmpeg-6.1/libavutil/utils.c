/*
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

#include "config.h"
#include "avutil.h"
#include "avassert.h"
#include <ctype.h>

/**
 * @file
 * various utility functions
 */

const char *av_get_media_type_string(enum AVMediaType media_type)
{
    switch (media_type) {
    case AVMEDIA_TYPE_VIDEO:      return "video";
    case AVMEDIA_TYPE_AUDIO:      return "audio";
    case AVMEDIA_TYPE_DATA:       return "data";
    case AVMEDIA_TYPE_SUBTITLE:   return "subtitle";
    case AVMEDIA_TYPE_ATTACHMENT: return "attachment";
    default:                      return NULL;
    }
}

char av_get_picture_type_char(enum AVPictureType pict_type)
{
    switch (pict_type) {
    case AV_PICTURE_TYPE_I:  return 'I';
    case AV_PICTURE_TYPE_P:  return 'P';
    case AV_PICTURE_TYPE_B:  return 'B';
    case AV_PICTURE_TYPE_S:  return 'S';
    case AV_PICTURE_TYPE_SI: return 'i';
    case AV_PICTURE_TYPE_SP: return 'p';
    case AV_PICTURE_TYPE_BI: return 'b';
    default:                 return '?';
    }
}

unsigned av_int_list_length_for_size(unsigned elsize,
                                     const void *list, uint64_t term)
{
    unsigned i;

    if (!list)
        return 0;
#define LIST_LENGTH(type) \
    { type t = term, *l = (type *)list; for (i = 0; l[i] != t; i++); }
    switch (elsize) {
    case 1: LIST_LENGTH(uint8_t);  break;
    case 2: LIST_LENGTH(uint16_t); break;
    case 4: LIST_LENGTH(uint32_t); break;
    case 8: LIST_LENGTH(uint64_t); break;
    default: av_assert0(!"valid element size");
    }
    return i;
}

char *av_fourcc_make_string(char *buf, uint32_t fourcc)
{
    int i;
    char *orig_buf = buf;
    size_t buf_size = AV_FOURCC_MAX_STRING_SIZE;

    for (i = 0; i < 4; i++) {
        const int c = fourcc & 0xff;
        const int print_chr = (c >= '0' && c <= '9') ||
                              (c >= 'a' && c <= 'z') ||
                              (c >= 'A' && c <= 'Z') ||
                              (c && strchr(". -_", c));
        const int len = snprintf(buf, buf_size, print_chr ? "%c" : "[%d]", c);
        if (len < 0)
            break;
        buf += len;
        buf_size = buf_size > len ? buf_size - len : 0;
        fourcc >>= 8;
    }

    return orig_buf;
}

AVRational av_get_time_base_q(void)
{
    return (AVRational){1, AV_TIME_BASE};
}

void av_assert0_fpu(void) {
#if HAVE_MMX_INLINE
    uint16_t state[14];
     __asm__ volatile (
        "fstenv %0 \n\t"
        : "+m" (state)
        :
        : "memory"
    );
    av_assert0((state[4] & 3) == 3);
#endif
}



double av_atof(const char *str) {
    const char *p = str;
    int sign = 1;
    while (*p == ' ')++p;//忽略前置空格
    if (*p == '-')//考虑是否有符号位
    {
        sign = -1;
        ++p;
    }
    else if (*p == '+')
        ++p;
    int hasDot = 0,hasE = 0;
    double integerPart = 0.0,decimalPart = 0.0;
    //遇到'e'或'.'字符则退出循环,设置hasE和hasDot。
    for (; *p; ++p){
        if (isdigit(*p)) //若p指向的字符为数字则计算当前整数部分的值
            integerPart = 10 * integerPart + *p - '0';
        else if (*p == '.'){
            hasDot = 1;
            p++;
            break;
        }
        else if (*p == 'e' || *p == 'E'){
            hasE = 1;
            p++;
            break;
        }
        else  //如果遇到非法字符,则截取合法字符得到的数值,返回结果。
            return integerPart;
    }

//上一部分循环中断有三种情况,一是遍历完成,这种情况下一部分的循环会自动跳过；其次便是是遇到'.'或'e',两种hasE和hasDot只可能一个为真,若hasDot为真则计算小数部分,若hasE为真则计算指数部分。
    int decimalDigits = 1;
    int exponential = 0;
    for (; *p; p++){
        if (hasDot && isdigit(*p))
            decimalPart += (*p - '0') / pow(10, decimalDigits++);
        else if (hasDot && (*p == 'e' || *p == 'E')) {
            integerPart += decimalPart;
            decimalPart = 0.0;
            hasE = 1;
            ++p;
            break;
        }
        else if (hasE && isdigit(*p))
            exponential = 10 * exponential + *p - '0';
        else break;
    }
//上一部分较难理解的就是else if (hasDot && (*p == 'e' || *p == 'E')) 这一特殊情况,对于合法的浮点数,出现'.'字符后,仍然有可能是科学计数法表示,但是出现'e'之后,指数部分不能为小数(这符合<string.h>对atof()的定义)。这种情况变量IntegerPart和decimalPart都是科学计数法的基数,因此有integerPart += decimalPart(这使得IntergerPart的命名可能欠妥,BasePart可能是一种好的选择)。
//上一部分循环结束一般情况下就能返回结果了,除非遇到前文所述的特殊情况，对于特殊情况需要继续计算指数。
    if (hasE && hasDot)
        for (; *p; p++)
            if (isdigit(*p))
                exponential = 10 * exponential + *p - '0';
    return sign * (integerPart * pow(10, exponential) + decimalPart);

}


#if 1 //weiguo 
double av_strtof(const char *str)
{
    double a;           /* the a value in a*10^b */
    double decplace;    /* number to divide by if decimal point is seen */
    double b;           /* The b value (exponent) in a*10^b */

    int sign = 1;       /* stores the sign of a */
    int bsign = 1;      /* stores the sign of b */

    while (*str && isspace(*str))
        ++str;

    if (*str == '+')
        ++str;
    if (*str == '-') {
        sign = -1;
        ++str;
    }
    if ((*str == 'n' || *str == 'N') &&
       (str[1] == 'a' || str[1] == 'A')
       && (str[2] == 'n' || str[2] == 'N'))
            return NAN * sign;
    if ((*str == 'i' || *str == 'I') && (str[1] == 'n' || str[1] == 'N') &&
        (str[2] == 'f' || str[2] == 'F'))
              return INFINITY * sign;

    for (a = 0; *str && isdigit(*str); ++str)
        a = a * 10 + (*str - '0');

    if (*str == '.')
        ++str;
    for (decplace = 1.; *str && isdigit(*str); ++str, decplace *= 10.)
        a = a * 10. + (*str - '0');

    if (*str == 'e' || *str == 'E') {
        /* if the user types a string starting from e, make the base be 1 */
        if (a == 0)
            a = 1;
        ++str;
        if (*str == '-') {
            bsign = -1;
            ++str;
        }
        if (*str == '+')
            ++str;
        for (b = 0; *str && isdigit(*str); ++str)
            b = b * 10 + (*str - '0');

        b *= bsign;
    }
    else
        b = 0;

    return (a * sign / decplace) * pow(10, b);
}
#endif
