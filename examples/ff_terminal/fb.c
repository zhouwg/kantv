/*!
 *
 * \author zhouwg2000@gmail.com
 *
 * \brief  implement a simple frambuffer interface
 *
 * \file fb.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/fb.h>

#include "vkey.h"
#include "terminal.h"


#ifdef PLATFORM_CDELINUX

static int fb                   = -1;
static char *mapped_mem         = NULL;
static long mapped_memlen       = -1;
static unsigned char *g_pOSDFrameBuffer = NULL;
static const char *s_pMagicPointer = (char*)0x1a2b3c4d;
static const char *FBDEV = "/dev/fb0";
static unsigned char *getFB();


FILE  *FrameBufferOpen(void)
{
    return (FILE*)s_pMagicPointer;
}


static void print_vinfo(struct fb_var_screeninfo *vinfo)
{
#ifdef  __DEBUG_FB
    fprintf(stderr, "Printing vinfo:\n");
    fprintf(stderr, "\txres: %d\n", vinfo->xres);
    fprintf(stderr, "\tyres: %d\n", vinfo->yres);
    fprintf(stderr, "\txres_virtual: %d\n", vinfo->xres_virtual);
    fprintf(stderr, "\tyres_virtual: %d\n", vinfo->yres_virtual);
    fprintf(stderr, "\txoffset: %d\n", vinfo->xoffset);
    fprintf(stderr, "\tyoffset: %d\n", vinfo->yoffset);
    fprintf(stderr, "\tbits_per_pixel: %d\n", vinfo->bits_per_pixel);
    fprintf(stderr, "\tgrayscale: %d\n", vinfo->grayscale);
    fprintf(stderr, "\tnonstd: %d\n", vinfo->nonstd);
    fprintf(stderr, "\tactivate: %d\n", vinfo->activate);
    fprintf(stderr, "\theight: %d\n", vinfo->height);
    fprintf(stderr, "\twidth: %d\n", vinfo->width);
    fprintf(stderr, "\taccel_flags: %d\n", vinfo->accel_flags);
    fprintf(stderr, "\tpixclock: %d\n", vinfo->pixclock);
    fprintf(stderr, "\tleft_margin: %d\n", vinfo->left_margin);
    fprintf(stderr, "\tright_margin: %d\n", vinfo->right_margin);
    fprintf(stderr, "\tupper_margin: %d\n", vinfo->upper_margin);
    fprintf(stderr, "\tlower_margin: %d\n", vinfo->lower_margin);
    fprintf(stderr, "\thsync_len: %d\n", vinfo->hsync_len);
    fprintf(stderr, "\tvsync_len: %d\n", vinfo->vsync_len);
    fprintf(stderr, "\tsync: %d\n", vinfo->sync);
    fprintf(stderr, "\tvmode: %d\n", vinfo->vmode);
    fprintf(stderr, "\tred: %d/%d\n", vinfo->red.length, vinfo->red.offset);
    fprintf(stderr, "\tgreen: %d/%d\n", vinfo->green.length, vinfo->green.offset);
    fprintf(stderr, "\tblue: %d/%d\n", vinfo->blue.length, vinfo->blue.offset);
    fprintf(stderr, "\talpha: %d/%d\n", vinfo->transp.length, vinfo->transp.offset);
#endif
}


static void print_finfo(struct fb_fix_screeninfo *finfo)
{
#ifdef  __DEBUG_FB
    fprintf(stderr, "Printing finfo:\n");
    fprintf(stderr, "\tsmem_start = %p\n", (char *)finfo->smem_start);
    fprintf(stderr, "\tsmem_len = %d\n", finfo->smem_len);
    fprintf(stderr, "\tsmem_len = %d kb\n", finfo->smem_len >> 10);
    fprintf(stderr, "\tsmem_len = %d mb\n", finfo->smem_len >> 20);
    fprintf(stderr, "\ttype = %d\n", finfo->type);
    fprintf(stderr, "\ttype_aux = %d\n", finfo->type_aux);
    fprintf(stderr, "\tvisual = %d\n", finfo->visual);
    fprintf(stderr, "\txpanstep = %d\n", finfo->xpanstep);
    fprintf(stderr, "\typanstep = %d\n", finfo->ypanstep);
    fprintf(stderr, "\tywrapstep = %d\n", finfo->ywrapstep);
    fprintf(stderr, "\tline_length = %d\n", finfo->line_length);
    fprintf(stderr, "\tmmio_start = %p\n", (char *)finfo->mmio_start);
    fprintf(stderr, "\tmmio_len = %d\n", finfo->mmio_len);
    fprintf(stderr, "\taccel = %d\n", finfo->accel);
#endif
}


int GetFrameBufferInfo(FILE *fp, unsigned char **ptr, int *w, int *h, int *bpp, int *memlen)
{
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    char *filename1[2];
    int    i                    = 0;

    unsigned long *pLong    = NULL;
    unsigned char *pTmp        = NULL;
    unsigned int nRow        = 0;
    unsigned int nCol        = 0;
    long screensize;

    fb = open("/dev/fb0", O_RDWR);
    if (fb <= 0)
    {
        printf("can not open framebuffer device.\n");
        perror("open");
        return -1;
    }
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo))
    {
        printf("ioctl error with %s.\n", FBDEV);
        return -1;
    }
    print_vinfo(&vinfo);
    if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo))
    {
        printf("ioctl error with %s.\n", FBDEV);
        return -1;
    }
    print_finfo(&finfo);


    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

#define PAGE_SIZE 4096
    long mapped_offset = (((long)finfo.smem_start) -
                          (((long)finfo.smem_start)&~(PAGE_SIZE-1)));
    mapped_memlen = finfo.smem_len+mapped_offset;

    printf("offset is %ld\n", mapped_offset);

    mapped_mem = mmap(NULL, mapped_memlen,
                      PROT_READ|PROT_WRITE, MAP_SHARED, fb, 0);

    if ( mapped_mem == (char *)-1 )
    {
        printf("Unable to memory map the video hardware");
        mapped_mem = NULL;
        return(-1);

    }
    g_pOSDFrameBuffer = mapped_mem+mapped_offset;

    printf("g_pOSDFrameBuffer is %p\n", g_pOSDFrameBuffer);

    *ptr    = g_pOSDFrameBuffer;
    *w         = vinfo.xres;
    *h         = vinfo.yres;
    *bpp     = vinfo.bits_per_pixel;
    *memlen = finfo.smem_len;


    return 0;
}


static unsigned char *getFB()
{
    return g_pOSDFrameBuffer;
}


void  FrameBufferClose(FILE *fp)
{
    if ( mapped_mem != NULL )
    {
        munmap(mapped_mem, mapped_memlen);
        mapped_mem = NULL;
    }
    close(fb);
    return;
}
#endif /* PLATFORM_CDELINUX */
