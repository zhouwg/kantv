/*
 *
 * Copyright (c) Sigma Designs, Inc. 2003. All rights reserved.
 *
 */

/**
	@file get_key.c
	@brief non-blocking character read from terminal
	
	@author Christian Wolff

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#include "get_key.h"


#define FALSE	0
#define TRUE	1
static struct termios tio_save;
static RMuint32 init_count = 0;
static RMuint32 exit_count = 0;
static void (*terminate)(void *param) = NULL;
void *g_param;

void RMTermExit(void);
static void sig_handler(int signo)
{
	if (exit_count++) {
		printf("Signal %d during cleanup, exiting unclean!\n", signo);
		exit(1);
	}
	printf("Signal %d, exiting...\n", signo);
	RMTermExit();
	if (terminate) (*terminate)(g_param);
	exit(0);
}

void RMTermInit(RMbool block_int)
{
	struct termios tio;
	
	if (init_count++) return;
	if (tcgetattr(STDIN_FILENO, &tio_save) < 0) {
		init_count--;
		return;
	}
	tio = tio_save;
	if (block_int) {
		tio.c_lflag &= ~(ECHO | ICANON | ISIG);
		tio.c_iflag &= ~(BRKINT);
	} else {
		tio.c_lflag |= (ISIG);
		tio.c_lflag &= ~(ECHO | ICANON);
		tio.c_iflag |= (BRKINT);
		tio.c_iflag &= ~(IGNBRK);
	}
	
	/* 
	   arrows or F1 characters generate a sequence of 3 characters in a row:
	   like ESC [ A (up arrow).
	   
	   To avoid reading [ or A as an individual character, we set
	   an inter character interval of 1/10 second. If characters
	   are received in less of 1/10 second then it is treated as
	   an individual character and is discard.
	*/
	
	tio.c_cc[VMIN] = 3;   /* up to 3 chars at a time */
	tio.c_cc[VTIME] = 1;  /* after receiving 1 char and after a delay of 1/10 second, 'read' returns */
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio);
}

void RMTermExit(void)
{
	if (--init_count) return;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio_save);
}

void RMSignalInit(void (*cleanup)(void *param), void *param)
{
	terminate = cleanup;
	g_param = param;
	
	if (signal(SIGBUS, sig_handler) == SIG_ERR) printf("Can't catch SIGBUS!\n");
#ifdef SIGEMT
	if (signal(SIGEMT, sig_handler) == SIG_ERR) printf("Can't catch SIGEMT!\n");
#endif
	if (signal(SIGFPE, sig_handler) == SIG_ERR) printf("Can't catch SIGFPE!\n");
	if (signal(SIGHUP, sig_handler) == SIG_ERR) printf("Can't catch SIGHUP!\n");
	if (signal(SIGILL, sig_handler) == SIG_ERR) printf("Can't catch SIGILL!\n");
	if (signal(SIGINT, sig_handler) == SIG_ERR) printf("Can't catch SIGINT!\n");
	if (signal(SIGIOT, sig_handler) == SIG_ERR) printf("Can't catch SIGIOT!\n");
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) printf("Can't catch SIGPIPE!\n");
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) printf("Can't catch SIGQUIT!\n");
	if (signal(SIGSEGV, sig_handler) == SIG_ERR) printf("Can't catch SIGSEGV!\n");
	if (signal(SIGSYS, sig_handler) == SIG_ERR) printf("Can't catch SIGSYS!\n");
	if (signal(SIGTERM, sig_handler) == SIG_ERR) printf("Can't catch SIGTERM!\n");
	if (signal(SIGTRAP, sig_handler) == SIG_ERR) printf("Can't catch SIGTRAP!\n");
	if (signal(SIGUSR1, sig_handler) == SIG_ERR) printf("Can't catch SIGUSR1!\n");
	if (signal(SIGUSR2, sig_handler) == SIG_ERR) printf("Can't catch SIGUSR2!\n");
}

RMbool RMKeyAvailable(void)
{
	struct timeval tv;
	fd_set readfds;
	
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	return (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0);
}

RMascii RMGetKey(void)
{
	RMascii Key[3];
	if (read(STDIN_FILENO, Key, 3) == 1) return Key[0];
	return '\0';
}

RMbool RMGetKeyNoWait(RMascii *pKey)
{
	RMascii Key[3];

	if (! RMKeyAvailable()) return FALSE;
	if (pKey == NULL) return FALSE;
	if (read(STDIN_FILENO, Key, 3) == 1) {
		*pKey = Key[0];
		return TRUE;
	}
	
	return FALSE;
}

static void tRMTermEnableEcho()
{
	static struct termios tio;
	if (tcgetattr(STDIN_FILENO, &tio) < 0) {
		return;
	}
	tio.c_lflag |= ECHO | ICANON;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio);
}

void RMTermDisableEcho()
{
	static struct termios tio;
	if (tcgetattr(STDIN_FILENO, &tio) < 0) {
		return;
	}
	tio.c_lflag &=  ~( ECHO | ICANON );
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio);
}

void RMTermGetUint32(RMuint32 *data)
{
/* 	static struct termios tio; */
 	RMbool handle_echo = FALSE; 
	
	RMuint32 i = 0;
	RMascii buffer[31];
	RMascii key;

	if (data == NULL)
		return;

/* 	if ( tcgetattr(STDIN_FILENO, &tio) == 0 && (tio.c_lflag & ( ECHO | ICANON )) == 0 ) */
/* 		handle_echo = TRUE; */

 	if (handle_echo)	 
 		tRMTermEnableEcho(); 

 try_again:
	buffer[0] = '\0';
	while (i < 31) {
		if (RMKeyAvailable()) {
			key = RMGetKey();
			printf("%c", key); fflush(stdout); //echo
			if ((key >= '0') && (key <= '9')) {
				buffer[i++] = key;
				buffer[i] = '\0';
			}
			if (key == '\n') {
				buffer[i] = '\0';
				break;
			}
		}
	}
	sscanf(buffer, "%lu", data);
	
	if (i < 1) {
		fprintf(stderr, "you must enter a number\n");
		goto try_again;
	}

	if (handle_echo)
		RMTermDisableEcho();
	
	
	return;

}

#undef RMuint32 
#undef RMbool  
#undef RMascii
#undef FALSE
#undef TRUE
