/*!
 * \file terminal.h
 *
 * \brief  implement a simple key-event driven interface to troubleshooting tool
 *
 * \author zhouwg2000@gmail.com
 *
 */

#ifndef __TERMINAL_H
#define __TERMINAL_H

#ifdef __cplusplus
    extern "C"{
#endif

int             TerminalInit(void);
void            TerminalQuit(void);
unsigned long   WaitKey(void);
unsigned long   GetKey(void);


#ifdef __cplusplus
}
#endif

#endif
