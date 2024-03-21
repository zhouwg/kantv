/*!
 * \file terminal.h
 *
 * \brief  implement a simple key-event driven interface to application
 *
 * \author zhouwg2000@gmail.com
 *
 * \remark
 *
 * 	history:
 *
 * $Id: terminal.h,v 1.1 2008/08/05 02:48:53 zhouwg Exp $
 *
 */

#ifndef __TERMINAL_H
#define __TERMINAL_H

#ifdef __cplusplus
	extern "C"{
#endif

void TerminalInit(void);
void TerminalQuit(void);
unsigned long WaitKey(void);
unsigned long GetKey(void);


#ifdef __cplusplus
}
#endif

#endif
