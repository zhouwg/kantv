/*!
 *
 * \author zhouwg2000@gmail.com
 *
 * \brief  implement a simple frambuffer interface
 *
 * \file fb.c
 *
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

extern FILE  *FrameBufferOpen(void);
extern void  GetFrameBufferInfo(FILE *fp, unsigned char **ptr, int *w, int *h, int *bpp, int *memlen);
extern void  FrameBufferClose(FILE *fp);

#ifdef __cplusplus
}
#endif
