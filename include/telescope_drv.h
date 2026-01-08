/****************************************************************************
 * $RCSfile: telescope_drv.h,v $
 * $Revision: 1.8 $
 * $Date: 2010/10/12 14:37:27 $
 * Author: Erik Hovland (ehovland@huey.jpl.nasa.gov)
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

#include <observer.h>

#define NO_NOP /* don't need to send NOPs just to get status */

/* EXTERN int telescope_open(void); */
EXTERN int telescope_open(char*);
//EXTERN int telescope_io(char *cmd, char *response_string, char *server_name, int server_port);
EXTERN int telescope_io(char *cmd, char *response_string, char *tcs_prog_name, char *tcs_status_file);
EXTERN int telescope_close(int serial_fd);

/*  TCS communincation program */
// now defined as environment variable in .login file
// #define TCS_PROGRAM "/home/observer/bin/tcs_talk_client.pl"

/* ERROR code for bad communication with TCS */
#define TCS_ERROR_CODE "TCS_ERROR"

/* size of TCS telemetry string */
#define TCS_TELEMETRY_SIZE 150 

/* size of buffer used to return TCS response */
#define TEL_RESPONSE_SIZE 1024

/* TCP log file */
// now defined as environement varables  in .login fulke
//#define TCS_STATUS_FILE "/home/observer/logs/tcs.status"

// Mode selection for point commands: fixed point or sidereal rate track
enum pointmode {TELMOUNT_POINT,TELMOUNT_SIDTRACK};


/***************  telescope command definitions ***************
 *  commands are strings with specific headers and footers.
 * Important stuff inbetween ;).
 *******************************************************************/


/* expected TCS telemetry format */

typedef struct {
   char motion_status[1]; /* 0 = stable, 1 = dec motion, 2 = ra motion, 3 = ra and dec motion */
                          /* 3 bits, bit 0 is dec motion, bit 1 is ra motion, bit 3 is dome motion */
   char wobble_status[1]; /* blank = not on a beam, 1 = at beam 1, 2 = at beam 2 */
   char dummy1[1];        /* blank space filler */
   char ra[9];            /* current right ascension HHMMSS.ss */
   char dummy2[1];        /* blank space filler */
   char dec[9];           /* current declination +DDMMSS.s */
   char dummy3[2];        /* blank space filler */
   char ha[9];            /* current hour angle +HH:MM:SS */
   char dummy4[1];        /* blank space filler */
   char lst[8];           /* local sidereal time HH:MM:SS */
   char dummy5[1];        /* blank space filler */
   char alt[5];           /* telescope elevation in deg +DD.d */
   char dummy6[1];        /* blank space filler */
   char azim[6];          /* telescope azimuth in deg +DDD.d (- for west, + for east ) */
   char dummy7[1];        /* blank space filler */
   char secz[5];          /* secant of the zenith distance DD.dd */
   char dummy8[1];        /* blank space filler */
   char com1[1];          /* com1 command execution code: E,e (alternating) for executed, 
			     1 = bogus request, 2 = bogus data, 3 = unrecognized command  */
   char com2[1];          /* com2 execution code */
   char com3[1];          /* com3 execution code */
   char com4[1];          /* com4 execution code */
   char com5[1];          /* com5 execution code */
   char com6[1];          /* com6 execution code */
   char com7[1];          /* com7 execution code */
   char com8[1];          /* com8 execution code */
   char dummy9[1];        /* blank space filler */
   char ra_limit[1];      /* 1 = in ra limit, blank = not in limit */
   char dec_limit[1];     /* 1 = in dec limit, blank = not in limit */
   char horiz_limit[1];   /* 1 = in horizon limit, blank = not in limit */
   char drive_status[1];  /* 1 = drives disabled, blank = not disabled */
   char epoch[8];         /* display coordinate epoch YYYY.YYY */
   char dummy10[1];        /* blank space filler */
   char jd[9];           /* Julian Date JJJJJJJ.JJ */
#ifdef DEMO_TCS
   char reserved[5];      /* reserved */
#else
   char dummy11[1];        /* blank space filler */
   char channel[1];       /* Current com channel number. Integer */
   char dummy12[1];        /* blank space filler */
   char focus_pos[6];     /*signed focus position +nnnnn */
   char dummy13[1];        /* blank space filler */
   char dome_err_deg[6]; /* Dome Error in Deg. +DDD.d  west < 0, east > 0 */
   char dummy14[1];        /* blank space filler */
   char ut_time[10];     /* UT Time HH:MM:SS.s */
   char dummy[1];       /* 0 for closed, 1 for open */
   char dome_state[1];   /* 0 for closed, 1 for open */
   char reserved[26];     /* reserved */
#endif
   char cr[1];            /* carriage return */
   char lf[1];            /* linefeed */
} TCS_Telemetry;

/* return codes from telescope_controller::check_tel_response */

#define TCS_TELEMETRY_OK 0 /*telemetry string correct length and command successfully executed */
#define TCS_TELEMETRY_ERROR -1 /*full telemetry string not returned by TCS */
#define TCS_BAD_COMMAND_ERROR 1 /* badly formed or unexectutable command sent to TCS */
#define TCS_BAD_DATA_ERROR 2 /* command with badly formed arguments sent to TCS */
#define TCS_UNRECOGNIZED_COMMAND_ERROR 3 /* unrecognized command sent to TCS */
#define TCS_UNKNOWN_ERROR 4 /* unexpected error codes in telemetry string */
#define TCS_CHECKSUM_ERROR 5 /* unexpected error codes in telemetry string */


/* NOP command. Should not be needed for anything */
#define TELESCOPE_CMD_NOP_STR "NOPNOP"

/* take control. This is used to see if the TCS is taking
   a dummy command (NOP) and responding. If so, we 
   have control */
#define TELESCOPE_CMD_TAKE_CONTROL_STR "NOPNOP"

/*enable, disable drives */
#define TELESCOPE_CMD_UNKILL_STR "UNKILL"
#define TELESCOPE_CMD_KILL_STR "KILL"

/* stow and stop */
#define TELESCOPE_CMD_STOW_STR "MOVSTOW"
#define TELESCOPE_CMD_STOP_STR "CANCEL"

/* status */
#ifdef NO_NOP
#define TELESCOPE_CMD_STS_STR "STATUS"
#else
#define TELESCOPE_CMD_STS_STR "NOPNOP"
#endif

#define TELESCOPE_CMD_NOCMD_STR "NOCOMMAND"

/* next ra, dec*/
#define TELESCOPE_CMD_NEXTRA_STR "NEXTRA"
#define TELESCOPE_CMD_NEXTDEC_STR "NEXTDEC"

/* move to next position*/
#define TELESCOPE_CMD_MOVENEXT_STR "MOVNEXT"

/* set siderial tracking on/off*/
#define TELESCOPE_CMD_TRACKON_STR "TRACK ON"
#define TELESCOPE_CMD_TRACKOFF_STR "TRACK OFF"

/*set fast slew on/off */
#define TELESCOPE_CMD_SLEWON_STR "SLEWON"
#define TELESCOPE_CMD_SLEWOFF_STR "SLEWOFF"

/* turn bias rates on/off */
#define TELESCOPE_CMD_BIASON_STR "BIASON"
#define TELESCOPE_CMD_BIASOFF_STR "BIASOFF"

/* change epoch of coordinates input to TCS */
#define TELESCOPE_CMD_EPOCH_STR "EPOCH"

/* set ut time of TCS computer*/
#define TELESCOPE_CMD_SETTIME_STR "SETTIME"

/* set ut date of TCS computer*/
#define TELESCOPE_CMD_SETDATE_STR "SETDATE"

/* new  commands */
#define TELESCOPE_CMD_MOVERADEC_STR "MOVRADEC" /* parameters hh:mm:ss.s space sdd:mm:ss.s, moves to given ra and dec */
#define TELESCOPE_CMD_FOCUS_STR  "FOCUS" /* takes space, then parameter (long int) */
#define TELESCOPE_CMD_RELFOCUS_STR  "RELFOCUS" /* takes space, then parameter (long int) */
#define TELESCOPE_CMD_FOCUS_ZERO_STR "FOCUSZER0" /* sets current position to 0 */
#define TELESCOPE_CMD_MOVE_DOME_STR "MOVDOME" /* parameter floating point 0 to 359, moves to this azimuth, 0 is north */
#define TELESCOPE_CMD_AUTO_DOME_ON_STR "AUTODOME ON" /* sets dome following on  */
#define TELESCOPE_CMD_AUTO_DOME_OFF_STR "AUTODOME OFF" /* set dome following off and stops any ongoing motion of dome */
#define TELESCOPE_CMD_STOP_DOME_STR "AUTODOME OFF"
#define TELESCOPE_CMD_DOME_INIT_STR "DOMEINIT" /* forces the current position to be starting point */
#define TELESCOPE_CMD_STOW_DOME_STR "STOWDOME" /* moves the dome to its stow position */
#define TELESCOPE_CMD_DOME_OPEN_STR "DOME OPEN" /* open dome slit */
#define TELESCOPE_CMD_DOME_CLOSE_STR "DOME CLOSE" /* close dome slit */
#define TELESCOPE_CMD_ZENITH_STR  "ELAZ 90.0 000.0" /* put telescope at zenint, el = 90, az = 0*/
#define TELESCOPE_CMD_STOW20_STR  "ELAZ 20.0 000.0" /* put telescope at zenint, el = 90, az = 0*/

#ifdef TCS_DEMO 

   /* These commands no longer available. Only in demo version . */
/* turn aberration correction on/off */
#define TELESCOPE_CMD_ABERON_STR "ABERON"
#define TELESCOPE_CMD_ABEROFF_STR "ABEROFF"

/* turn aberration deconvolution  on/off */
#define TELESCOPE_CMD_UNABERON_STR "UNABERON"
#define TELESCOPE_CMD_UNABEROFF_STR "UNABEROFF"

/* turn flexture correction on/off */
#define TELESCOPE_CMD_FLEXON_STR "FLEXON"
#define TELESCOPE_CMD_FLEXOFF_STR "FLEXOFF"

/* turn flexture deconvolution on/off */
#define TELESCOPE_CMD_UNFLEXON_STR "UNFLEXON"
#define TELESCOPE_CMD_UNFLEXOFF_STR "UNFLEXOFF"

/* turn nutation correction on/off */
#define TELESCOPE_CMD_NUTON_STR "NUTON"
#define TELESCOPE_CMD_NUTOFF_STR "NUTOFF"

/* turn parallax correction on/off */
#define TELESCOPE_CMD_PARAON_STR "PARAON"
#define TELESCOPE_CMD_PARAOFF_STR "PARAOFF"

/* turn precession correction on/off */
#define TELESCOPE_CMD_PREON_STR "PREON"
#define TELESCOPE_CMD_PREOFF_STR "PREOFF"

/* turn proper motion correction on/off */
#define TELESCOPE_CMD_PROPERON_STR "PROPERON"
#define TELESCOPE_CMD_PROPEROFF_STR "PROPEROFF"

/* turn refraction correction on/off */
#define TELESCOPE_CMD_REFRACON_STR "REFRACON"
#define TELESCOPE_CMD_REFRACOFF_STR "REFRACOFF"

/* turn refraction deconvolution on/off */
#define TELESCOPE_CMD_UNREFRACON_STR "UNREFRACON"
#define TELESCOPE_CMD_UNREFRACOFF_STR "UNREFRACOFF"

#endif

#if 0

/* dome and windscreen commands */
#define TELESCOPE_CMD_SLAVE_DOME_STR "D1SLAVE\n"
#define TELESCOPE_CMD_SLAVE_WINDSCR_STR "D2SLAVE\n"
#define TELESCOPE_CMD_OPEN_DOME_STR "DSO\n"
#define TELESCOPE_CMD_CLOSE_DOME_STR "DSC\n"

#endif

/* Read & Write defines for io function */
#define TELESCOPE_WRITE 0
#define TELESCOPE_READ 1

#endif /* _TELESCOPE_SERIAL_H_ */
