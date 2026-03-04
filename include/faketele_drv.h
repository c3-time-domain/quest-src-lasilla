/****************************************************************************
 *
 * only serial commands for demo telescope
 *
 * include file to help with interfacing the
 * serial port. The serial port is the main interface to the TCU
 * which controls the telescope. This file augments the typical
 * read() and write() commands to be used specifically with the TCU set
 * of commands (mainly strings).
 * 
 *
 * Copyright held by the California Institute of Technology, Pasadena, CA.
 *
 ***************************************************************************/

#ifndef _TELESCOPE_SERIAL_H_
#define _TELESCOPE_SERIAL_H_

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN extern
#endif

/* EXTERN int telescope_open(void); */
EXTERN int telescope_open(char*);
EXTERN int telescope_io(int fd, int cmd, char *cmd_string, char *servername, int server_port);
EXTERN int telescope_close(int serial_fd);

/*  Serial port device file name */
//#define TELESCOPE_DEVICE "/dev/ttya"
#define TELESCOPE_DEVICE "/dev/ttyS0"

/***************  telescope command definitions ***************
 *  commands are strings with specific headers and footers.
 * Important stuff inbetween ;).
 *******************************************************************/

/* Command footer */
#define TELESCOPE_CMD_FTR "\n"
#define TELESCOPE_RSP_FTR "\n\n"

/* ACK and NACK responses */
#define TELESCOPE_RSP_ACK "A\n\n"
#define TELESCOPE_RSP_NACK "N\n\n"
#define TELESCOPE_RSP_NIC "N NIC\n\n"

/* Take/Release control command */
#define TELESCOPE_CMD_TAKE_CTRL "CTAKE\n"
#define TELESCOPE_CMD_REL_CTRL "CREL\n"

/* Track Star command */
#define TELESCOPE_CMD_STAR_TRACK "STRTK 9\n"

/* Load Star position */
#define TELESCOPE_CMD_STAR_LOAD "STAR"
#define TELESCOPE_CMD_STAR_CHK "STAR?\n"

/* dome and windscreen commands */

/* stow and stop */
#define TELESCOPE_CMD_STOW_STR "STOW\n"
#define TELESCOPE_CMD_STOP_STR "STOP\n"

/* Status command, Link command */
#define TELESCOPE_CMD_STS_STR "STAT\n"
#define TELESCOPE_CMD_BIN_STS_STR "FB\n"
#define TELESCOPE_CMD_LINK_TIMEOUT "LINK"
#define TELESCOPE_CMD_LINK_CHECK "LINK?\n"

/* status faults command */
#define TELESCOPE_CMD_STS_MSG_REQ_STR "STM\n"
#define TELESCOPE_CMD_FLT_REQ_STR "M\n"
#define TELESCOPE_CMD_ACK_FLT_STR "ACKF\n"

/* focus commands */

/* weather status command */

/* weather triggers commands */

/* Read & Write defines for io function */
#define TELESCOPE_WRITE 0
#define TELESCOPE_READ 1

#endif /* _TELESCOPE_SERIAL_H_ */
