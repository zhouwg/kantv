/*
 * \file   terminal.c
 *
 * \brief  a simple key-event driven interface for console terminal

 * \author zhouwg2000@gmail.com
 *
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
#include "vkey.h"
#include "terminal.h"


static unsigned long handle_standalone_key(int bBlock)
{
    int nread = 0;

    RMascii keybuf[5];
    unsigned long scancode = VK_UNKNOWN;

again:
    if (RMKeyAvailable()) {
        nread =  read(STDIN_FILENO, keybuf, 5);
        if (4 == nread) {
            if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (49 == keybuf[2]) && (126 == keybuf[3]) ) {
                scancode = VK_HOME;
            }

            if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (52 == keybuf[2]) && (126 == keybuf[3]) ) {
                if (1) {
                    scancode = VK_END;
                } else {
                    scancode = VK_UNKNOWN;
                }
            }

        } else if (3 == nread) {
            if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (66 == keybuf[2]) ) {
                scancode = VK_DOWN;
            }

            if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (65 == keybuf[2]) ) {
                scancode = VK_UP;
            }

            if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (68 == keybuf[2]) ) {
                scancode = VK_LEFT;
            }

            if ( (27 == keybuf[0]) && (91 == keybuf[1]) && (67 == keybuf[2]) ) {
                scancode = VK_RIGHT;
            }

        } else if (1 == nread) {
            scancode =  keybuf[0];
        }
    } else {
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


int TerminalInit()
{
    RMTermInit(1);
    RMSignalInit(NULL, NULL);

    return 0;
}


void TerminalQuit()
{
    RMTermExit();
}
