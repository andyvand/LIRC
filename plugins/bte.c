/****************************************************************************
 ** hw_bte.c ****************************************************************
 ****************************************************************************
 *
 *  routines for Bluetooth for Ericsson mobile phone receiver (BTE)
 *
 *  Copyright (C) 2003 Vadim Shliakhov <svadim@nm.ru>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 *  20-02-05 VSS filter out 'e' (cancel) code triggered by other keys at t630
 *  21-01-05 VSS Pause after BTE menu aborted by user to allow turn off bluetooth at t630
 *  12-04-04 VSS t630 2 char key codes handling
 *               changes in connection reestablishing
 *  02-02-04 VSS read loop opened to make use of main select() inside lircd
 *               connection reestablishing dropped for a while
 *  16-01-04 VSS workaround for "NO" button, some cleanups
 *  20-11-03 VSS try to reestablish connection if lost
 *  14-07-03 VSS log flood fixed
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <limits.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "lirc_driver.h"
#include "lirc/serial.h"

struct timeval start, end, last;
lirc_t gap, signal_length;
ir_code pre, code;

// Forwards:
int bte_decode(struct ir_remote *remote, struct decode_ctx_t* ctx);
int bte_init(void);
int bte_deinit(void);
char *bte_rec(struct ir_remote *remotes);


#define BTE_CAN_SEND 0
const struct driver hw_bte = {
	.name		=	"bte",
	.device		=	LIRC_DRIVER_DEVICE,
#if BTE_CAN_SEND
	.features	=	LIRC_CAN_REC_LIRCCODE | LIRC_CAN_SEND_LIRCCODE,	/* features */
	.send_mode	=	LIRC_MODE_LIRCCODE,
#else
	.features	=	LIRC_CAN_REC_LIRCCODE,
	.send_mode	=	0,
#endif

	.rec_mode	=	LIRC_MODE_LIRCCODE,
	.code_length	=	16,
	.init_func	=	bte_init,
	.deinit_func	=	bte_deinit,
	.open_func	=	default_open,
	.close_func	=	default_close,
#if BTE_CAN_SEND
	.send_func	=	bte_send,
#else
	.send_func	=	NULL,
#endif
	.rec_func	=	bte_rec,
	.decode_func	=	bte_decode,
	.drvctl_func	=	NULL,
	.readdata	=	NULL,
	.api_version	=	2,
	.driver_version = 	"0.9.2",
	.info		=  	"No info available."
};

const struct driver* hardwares[] = { &hw_bte, (const struct driver*)NULL };

enum bte_state {
	BTE_NONE = 0, BTE_INIT, BTE_SET_ECHO, BTE_CHARSET, BTE_SET_ACCESSORY,
	BTE_START_EVENTS, BTE_STOP_EVENTS, BTE_CREATE_DIALOG, BTE_JUMP_ASIDE
};

static int pending = 0;
static int memo_mode = 0;
static int filter_cancel = 0;
static char prev_cmd[PACKET_SIZE + 1];
static int io_failed = 0;

int bte_connect(void);

int bte_sendcmd(char *str, int next_state)
{

	if (io_failed && !bte_connect())	// try to reestablish connection
		return 0;

	pending = next_state;
	sprintf(prev_cmd, "AT%s\r", str);

	LOGPRINTF(1, "bte_sendcmd: \"%s\"", str);
	if (write(drv.fd, prev_cmd, strlen(prev_cmd)) <= 0) {
		io_failed = 1;
		pending = 0;
		logprintf(LIRC_ERROR, "bte_sendcmd: write failed  - %d: %s", errno, strerror(errno));
		return 0;
	}
	LOGPRINTF(1, "bte_sendcmd: done");
	return 1;
}

int bte_connect(void)
{
	struct termios tattr;

	LOGPRINTF(3, "bte_connect called");

	if (drv.fd >= 0)
		close(drv.fd);

	do			//try block
	{
		errno = 0;
		if ((drv.fd = open(drv.device, O_RDWR | O_NOCTTY)) == -1) {
			LOGPRINTF(1, "could not open %s", drv.device);
			LOGPERROR(1, "bte_connect");
			break;
		}
		if (tcgetattr(drv.fd, &tattr) == -1) {
			LOGPRINTF(1, "bte_connect: tcgetattr() failed");
			LOGPERROR(1, "bte_connect");
			break;
		}
		LOGPRINTF(1, "opened %s", drv.device);
		LOGPERROR(1, "bte_connect");
		cfmakeraw(&tattr);
		tattr.c_cc[VMIN] = 1;
		tattr.c_cc[VTIME] = 0;
		if (tcsetattr(drv.fd, TCSAFLUSH, &tattr) == -1) {
			LOGPRINTF(1, "bte_connect: tcsetattr() failed");
			LOGPERROR(1, "bte_connect");
			break;
		}
		if (!tty_setbaud(drv.fd, 115200)) {
			LOGPRINTF(1, "bte_connect: could not set baud rate %s", drv.device);
			LOGPERROR(1, "bte_connect");
			break;
		}
		logprintf(LIRC_ERROR, "bte_connect: connection established");
		io_failed = 0;

		if (bte_sendcmd("E?", BTE_INIT))	// Ask for echo state just to syncronise
		{
			return (1);
		} else {
			LOGPRINTF(1, "bte_connect: device did not respond");
		}
	} while (0);

	//try block failed
	io_failed = 1;
	if (drv.fd >= 0)
		close(drv.fd);
	if ((drv.fd = open("/dev/zero", O_RDONLY)) == -1) {
		logprintf(LIRC_ERROR, "could not open /dev/zero/");
		logperror(LIRC_ERROR, "bte_init()");
	}
	sleep(1);
	return 0;
}

int bte_init(void)
{
	LOGPRINTF(3, "bte_init called, device %s", drv.device);

	if (!tty_create_lock(drv.device)) {
		logprintf(LIRC_ERROR, "bte_init: could not create lock file");
		return 0;
	}
	if (!bte_connect()) {
		// return 0;
	}
	return 1;
}

int bte_deinit(void)
{
	// stop events forwarding
	bte_sendcmd("+CMER=0,0,0,0,0", 0);
	close(drv.fd);
	tty_delete_lock();
	LOGPRINTF(1, "bte_deinit: OK");
	return (1);
}

char *bte_readline()
{
	static char msg[PACKET_SIZE + 1];
	char c;
	static int n = 0;
	int ok = 1;

	LOGPRINTF(6, "bte_readline called");

	if (io_failed && !bte_connect())	// try to reestablish connection
		return (NULL);

	if ((ok = read(drv.fd, &c, 1)) <= 0) {
		io_failed = 1;
		logprintf(LIRC_ERROR, "bte_readline: read failed - %d: %s", errno, strerror(errno));
		return (NULL);
	}
	if (ok == 0 || c == '\r')
		return NULL;
	if (c == '\n') {
		if (n == 0)
			return NULL;
		msg[n] = 0;
		n = 0;
		LOGPRINTF(1, "bte_readline: %s", msg);
		return (msg);
	}
	msg[n++] = c;
	if (n >= PACKET_SIZE - 1)
		msg[--n] = '!';
	return NULL;
}

char *bte_automaton()
{
	char *msg;
	int key = 0;
	int key0 = 0;
	int key_release = 0;
	int i;

	LOGPRINTF(5, "bte_automaton called");

	code = 0;

	while (1) {
		if ((msg = bte_readline()) == NULL)	// read failed
			return (NULL);
		if (pending != BTE_INIT)
			break;
		// tty_reset() seems to leave some garbage in a buffer so skip it
		if (strncmp(msg, "E: ", 3) == 0)
			pending = BTE_SET_ECHO;
	}
	if (strcmp(msg, "ERROR") == 0)	// "ERROR" received
	{
		pending = 0;
		logprintf(LIRC_ERROR, "bte_automaton: 'ERROR' received! Previous command: %s", prev_cmd);
		return (NULL);
	} else if (strcmp(msg, "OK") == 0)	// Check for next cmd to send
	{
		switch (pending) {
		case BTE_SET_ECHO:
			bte_sendcmd("E1", BTE_CHARSET);
			break;
		case BTE_CHARSET:	// set ISO-8859-1 charset
			bte_sendcmd("+CSCS=\"8859-1\"", BTE_SET_ACCESSORY);
			break;
		case BTE_SET_ACCESSORY:	// Set accessory menu item
			bte_sendcmd("*EAM=\"BTE remote\"", 0);
			break;
		case BTE_START_EVENTS:	// start events forwarding
			bte_sendcmd("+CMER=3,2,0,0,0", 0);
			break;
		case BTE_CREATE_DIALOG:	// create input dialog
			bte_sendcmd("*EAID=13,2,\"BTE Remote\"", BTE_START_EVENTS);
			break;
		case BTE_JUMP_ASIDE:
			// release device temporarily; chance for a
			// user to switch off mobile's bluetooth (t630)
			close(drv.fd);
			LOGPRINTF(3, "bte_automaton: device closed; sleeping");
			sleep(30);
			break;
		}
	} else if (strcmp(msg, "*EAAI") == 0)	// Accessory menu activated
	{
		// send empty command, trigger creating input dialog
		bte_sendcmd("", BTE_CREATE_DIALOG);
	} else if (strcmp(msg, "*EAII: 0") == 0)	// Input dialog rejected ("NO" pressed)
	{
		// issued even if "*EAID=13,2,xxxx"
		// stop events forwarding & re-create dialog
		bte_sendcmd("+CMER=0,0,0,0,0", BTE_CREATE_DIALOG);
	} else if (strcmp(msg, "*EAII") == 0)	// Input dialog aborted
	{
		// accesory menu left
		// stop events forwarding, no further actions
		// bte_sendcmd("+CMER=0,0,0,0,0", 0);
		bte_sendcmd("+CMER=0,0,0,0,0", BTE_JUMP_ASIDE);
	} else if (strncmp(msg, "+CKEV:", 6) == 0)	// Key-code event
	{
		i = 7;		// parse key-code string
		code = key = msg[i++];
		if (msg[i] != ',')	// 2 char code?
		{
			key0 = key;
			key = msg[i++];
			code = code << 8 | key;
		}
		key_release = msg[i + 1] == '0';
		code |= key_release << 15;

		LOGPRINTF(1, "bte_automaton: code 0x%llx", (__u64) code);

		if (key_release) {
			code = 0;	// block key release events
		} else
			switch (key)	// check key pressed for extra conditions
			{
			case 'e':
				if (filter_cancel) {
					code = 0;
					filter_cancel = 0;
					LOGPRINTF(1, "bte_automaton: 'e' filtered");
					break;
				}
				if (memo_mode)	// MEMO mode exited
				{
					memo_mode = 0;
					LOGPRINTF(1, "bte_automaton: MEMO mode exited");
				}
				break;
			case 'G':	// MEMO mode entered
				memo_mode = 1;
				LOGPRINTF(1, "bte_automaton: MEMO key");
				break;
				// testing for 'e' triggers
			case 'J':
			case 'R':
				if (key0 != ':')
					break;	// not ':J' or ':R'
			case ']':
				filter_cancel = 1;
				break;
			}
	} else			// Unknown reply
	{
		LOGPRINTF(1, "bte_automaton: Unknown reply");
	}
	strcat(msg, "\n");	// pad with newline
	return (msg);
}

char *bte_rec(struct ir_remote *remotes)
{
	LOGPRINTF(4, "bte_rec called");

	if (bte_automaton())
		return decode_all(remotes);
	else
		return NULL;
}

int bte_decode(struct ir_remote *remote, struct decode_ctx_t* ctx)
{
	LOGPRINTF(3, "bte_decode called");
	ctx->pre = pre;
	ctx->code = code;
	ctx->post = 0;

	LOGPRINTF(1, "bte_decode: %llx", (__u64) ctx->code);
	return (1);
}
