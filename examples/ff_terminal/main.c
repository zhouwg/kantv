/*
 * \file
 *
 * \brief  implement a simple key-event driven example running on console terminal for embedded linux/android

 * \author zhouwg2000@gmail.com
 *
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <linux/fb.h>

#include "vkey.h"

#include "fb.h"

#include "libavutil/cde_log.h"

static void cleanup();
static void doRender();


#ifdef PLATFORM_CDELINUX
static int fb_w = 0;
static int fb_h = 0;
static int fb_bpp = 0;
static int fb_memlen = 0;
static unsigned char *fb_ptr = NULL;
#endif

static int b_exit = 0;


/*
 * \brief emulate event-driven via key-event
 */
static void handle_vk(unsigned long scancode)
{
    if (scancode <= 0)
    {
        return;
    }

    switch (scancode)
    {
    case VK_ESCAPE:
        b_exit = 1;
        printf("key ESCAPE\n");
        break;

    default:
        printf("unknown key\n");
        break;
    }
}



static void  doRender()
{
}


static void fnEventLoop(void)
{
    unsigned long scancode = 0;

    while (1)
    {
        if (1 == b_exit)
            break;

        usleep(5);

        scancode = GetKey();
        handle_vk(scancode);

        doRender();
    }
}


static void cleanup()
{
    static int bCleanup = 0;

    if (!bCleanup)
    {
        bCleanup = 1;
        TerminalQuit();
#ifdef PLATFORM_CDELINUX
        FrameBufferClose(NULL);
#endif
    }
}



int main(int argc, char *argv[])
{
#ifdef PLATFORM_CDELINUX
    FrameBufferOpen();
    GetFrameBufferInfo(NULL, &fb_ptr, &fb_w, &fb_h, &fb_bpp, &fb_memlen);
    LOGGV("osd ptr is %p,w is %d,h is %d,bpp is %d,memlen is %d\n", fb_ptr, fb_w, fb_h, fb_bpp, fb_memlen);
#endif

    TerminalInit();
    atexit(cleanup);

    fnEventLoop();

    cleanup();

    return 0;
}
