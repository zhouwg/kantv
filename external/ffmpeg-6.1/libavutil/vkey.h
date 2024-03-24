/*!
 * \file    vkey.h
 *
 * \brief   virtual keycode
 *
 * \author  zhouwg2000@gmail.com
 *
 *
 * \remark
 *
 */

#ifndef _VK_H
#define _VK_H

#ifdef __cplusplus
    extern "C"{
#endif

typedef enum {
    VK_UNKNOWN      = 0,        /* invalid keycode  */

    VK_FIRST        = 0,

    VK_ESCAPE       = 27,

    VK_OK           = 0xd,
    VK_RETURN       = 27,
    VK_HOME         = 278,
    VK_END          = 271,
    VK_MENU         = 101,      /*  'e'  */
    VK_POWER        = 628,
    VK_VOSWITCH     = 107,      /*  'k'  */
    VK_INFO         = 119,


    VK_PAGEUP       = 280,
    VK_PAGEDOWN     = 281,

    VK_UP           = 273,
    VK_DOWN         = 274,
    VK_LEFT         = 276,
    VK_RIGHT        = 275,

    VK_DECVOLUME    = 91,       /* volume decrease ,    '['     */
    VK_INCVOLUME    = 93,       /* volume increase ,    ']'     */
    VK_MUTE         = 106,      /* volume mute     ,    'j'     */
    VK_PLAY         = 112,      /* play/pause,          'p'     */
    VK_PAUSE        = 112,      /* play/pause,          'p'     */
    VK_STOP         = 116,      /* stop,                't'     */

    VK_FORWARD      = 46,       /* fast forward,        '<'     */
    VK_BACKWARD     = 44,       /* fast backward,       '>'     */

    VK_SPEED_FAST_1x= 0x3d,     /* 1x,                  '='     */
    VK_SPEED_SLOW_1x= 0x2d,     /* 1x,                  '-'     */

    VK_PREV         =109,       /* previous,            'm'     */
    VK_NEXT         =110,       /* next,                'n'     */
    VK_ZOOMIN       =119,       /* zoomin,              'w'     */
    VK_ZOOMOUT      =103,       /* zoomout,             'g'     */

    VK_0            = 48,       /*  0 */
    VK_1            = 49,       /*  1 */
    VK_2            = 50,       /*  2 */
    VK_3            = 51,       /*  3 */
    VK_4            = 52,       /*  4 */
    VK_5            = 53,       /*  5 */
    VK_6            = 54,       /*  6 */
    VK_7            = 55,       /*  7 */
    VK_8            = 56,       /*  8 */
    VK_9            = 57,       /*  9 */
    VK_z            = 122,
    VK_AR           = 115,      /* AR switch */

    VK_HELP         = 104,      /* help, 'h' */

    VK_LAST
} VKey;

#include "terminal.h"

#ifdef __cplusplus
}
#endif

#endif /* _VK_H */
