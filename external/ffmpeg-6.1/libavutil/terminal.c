/*
 * \file   terminal.c
 *
 * \brief  implement a simple key-event driven interface for embedded linux running
 *         on the console  terminal

 * \author zhouwg2000@gmail.com
 *
 * \date   2008-6-18,2008-6-19
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <assert.h>

#include "get_key.h"
#include "vkey.h"			/*! virtual key defines header file */
#include "terminal.h"

/**
 */
static unsigned long handle_standalone_key(int bBlock)
{
        int nread = 0;
        RMascii keybuf[5];
        unsigned long scancode = VK_UNKNOWN;

again:
        if (RMKeyAvailable())
        {
                nread =  read(STDIN_FILENO, keybuf, 5);
                if (4 == nread)
                {
                        if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (49 == keybuf[2]) && (126 == keybuf[3]) )
                        {
                                scancode = VK_HOME;
                        }
                        if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (52 == keybuf[2]) && (126 == keybuf[3]) )
                        {
                                if (1)
                                {
                                        scancode = VK_END;
                                }
                                else
                                {
                                        scancode = VK_UNKNOWN;
                                }
                        }
                }
                else if (3 == nread)
                {

                        if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (66 == keybuf[2]) )
                        {
                                scancode = VK_DOWN;
                        }

                        if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (65 == keybuf[2]) )
                        {
                                scancode = VK_UP;
                        }

                        if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (68 == keybuf[2]) )
                        {
                                scancode = VK_LEFT;
                        }

                        if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (67 == keybuf[2]) )
                        {
                                scancode = VK_RIGHT;
                        }

                }
                else if (1 == nread)
                {

                        scancode =  keybuf[0];
                }
        } //end if (RMKeyAvailable())
		else
		{
				if (bBlock)
						goto again;
		}
        return scancode;

}


unsigned long WaitKey()
{
     return handle_standalone_key(1);
}

unsigned long GetKey()
{
     return handle_standalone_key(0);
}


void TerminalInit()
{
    RMTermInit(1);
    RMSignalInit(NULL, NULL);
}


void TerminalQuit()
{
    RMTermExit();
}

#if 0
/**
 * \brief event-driven infinite loop here
 */
void fnEventLoop(void)
{

        unsigned long scancode = 0;
        int g_bExit = 0;

        while (1)
        {
                if (g_bExit)
                {
                        T_TRACE("exit process key thread\n");
                        break;
                }
                scancode = WaitKey();
                if (scancode <= 0)
                {
                        continue;
                }

                switch (scancode)
                {
                case VK_HOME:
                        T_TRACE("press VK_HOME\n");
                        break;
                case VK_END:
                        T_TRACE("press VK_END\n");
                        if (!isInvokedByXserver())
                        {
                                g_bExit = 1;
                        }
                        else
                        {
                                T_TRACE("xserver is running\n");
                        }
                        break;
                case VK_DOWN:
                        T_TRACE("press VK_DONW\n");
                        break;
                case VK_UP:
                        T_TRACE("press VK_UP\n");
                        break;
                case VK_LEFT:
                        T_TRACE("press VK_LEFT\n");
                        break;
                case VK_RIGHT:
                {
                        T_TRACE("press VK_RIGHT\n");
                }
                		break;

                case VK_ENTER:
                        T_TRACE("press VK_ENTER\n");
                default:
                        break;

                }// end switch (scancode)
        }// end while (1)

        return ;
}


int main(int argc, const char *argv[] )
{
	TerminalInit();
	atexit(TerminalQuit);
    fnEventLoop();

    return 0;
}
#endif

