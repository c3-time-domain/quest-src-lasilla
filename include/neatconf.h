/*
 * $RCSfile: neatconf.h,v $
 * $Revision: 1.6 $
 * $Date: 2009/07/23 20:57:10 $
 * 
 * system configuration information for the NEAT ops system
 *
 * Steve Groom, JPL
 * July 19, 1995
 *
 * change NEAT-SLIPSEC from 1 hour to 10 sec for satellite observations
 * David Rabinowitz, JPL
 * Sept 29, 1997
 *
 */
#ifndef _NEATCONF_H_
#define _NEATCONF_H_

#include <syslog.h>
#include <observer.h>

#define NEAT_VIC_TASKNAME	"NEATCCD"	/* data collection task name */
#define NEAT_VIC_USERNAME	"NEAT"		/* data collection user name */

#define NEAT_CAM_RETRIES	6   /* times to retry failed exposures */

/************** Definitions for Ops Software Config ***********************/
#define NEAT_SYSDIR	"/home/observer/questlib" /* NEAT system files kept here */

/* The system configuration file.
 * This file contains many required parameters describing
 * the location (lat, lon, elevation), telescope calibration,
 * camera (rows, cols, etc), and data labelling.
 * The file is expected to reside in NEAT_SYSDIR.
 */
#define NEAT_CONFIGFILE	"quest.conf"	/* NEAT config file */

/* Filenames and directories related to data collection.
 * All of these (except for NEAT_IMGRAW) must be on the filesystem,
 * probably best to have them in the same directory.
 * This is because the image files are are first written to temporary
 * locations, then rename(2)'d to their final names.  If not on
 * the same filesystem, the rename() will fail!
 */
#define NEAT_IMGDIR	"" /* NEAT obs data directory */
// #define NEAT_IMGDIR	"/home/observer/obsdata/" /* NEAT obs data directory */
#define NEAT_IMGRAW	"/home/observer/obsdata/img.raw" /* camera raw data file */
#define NEAT_IMGTMP	"/home/observer/obsdata/img.tmp" /* temp file for readout */
#define NEAT_IMGCUR	"/home/observer/obsdata/img.current" /* latest readout */

/* some numbers for sizing temporary string variables */
#define NEAT_FILENAMELEN	256	/* max length of filename */
#define NEAT_LINELEN		1024	/* max length of line in a text file */

/* Logging configuration */
#define NEAT_LOG_FACILITY	LOG_LOCAL0	/* syslog facility to use */

/* Dark image tracking */
#define NEAT_DARKCATNAME "lastdark" 	/* catalog of recent dark images */
#define NEAT_MAXDARKTIMES	64	/* track up to 64 distinct exp times */
#define NEAT_DARKDELTA		0.01	/* match exp time to 0.01 sec */

/**** Definitions allowing readout out of camera in background ****/
 
#define NEAT_CHILD_DARKCATNAME "lastdark.child"  /* catalog of recent dark 
                               images kept by child of forked process*/
#define NEAT_CHILDLOGNAME "questcam.log"  /* questcam log*/

/**** Definitions for the observation manager ****/
#define NEAT_OBSMGRPROG		"obsmgr" /* name of observation mgr program */
#define NEAT_OBSMGR_CHECKTIME	5	/* idle schedule chk intvl, secs */

#define NEAT_DISPLAYPROG	"disprun" /* name of NEAT R/T display */
#define NEAT_DISPLAYPIDFILE	"disprun.pid" /* display process PID file */

/**** Definitions for the schedule manager ****/
#define NEAT_SCHEDPROG	"schedmgr" 	/* name of scheduler program */
#define NEAT_SCHEDFILE	"sequence" 	/* scheduler sequence file */
#define NEAT_SCHEDCKPT	"checkpoint" 	/* scheduler checkpoint file */

/* Allowed schedule variation, expressed as a fraction of the specified
 * observation interval.
 */
#define NEAT_SCHED_SLIPFACTOR	.2	/* allowed schedule variation */

/* Max allowed slippage for "time-critical" observations.
 */
/*#define NEAT_SCHED_TCSLIPSEC	(60*60)*/	/* 1 hour */
#define NEAT_SCHED_TCSLIPSEC	(10)	/* 10  sec */     

/* observation log files, located in NEAT_SYSDIR */
#define NEAT_SCHED_OBSLOG "log.obs"	/* log of individual observations */
#define NEAT_SCHED_GROUPLOG "log.group"	/* completed requests (triplets) */
#define NEAT_SCHED_REJECTLOG "log.reject"	/* failed/rejected requests */

/**** Definitions for the analysis manager ****/
#define NEAT_ANLQFILE	"anlqueue"	/* The analysis queue file */
#define NEAT_ANLREJECT	"anlqueue.reject"	/* The analysis reject file */
#define NEAT_ANLSLEEP	10		/* secs between checks of queue file */
//#define NEAT_ANLPROG	"adarun.pl"	/* Auto. Data Anal. run script */
#define NEAT_ANLPROG	"analrun.pl"	/* Auto. Data Anal. run script */
#define NEAT_MANLPROG	"mountmodrun.pl"	/* Auto Data Anal. for mount */
						/* offset */
#define NEAT_ANLDIR	"/home/observer/analysis" /* the analysis working dir */

/**** Definitions for the telescope manager ****/
#define NEAT_TELMGRPROG	"telmgr"	/* name of mount manager program */
#define NEAT_TELMGR_CHECKTIME	10 	/* mount status check interval */

/**** Definitions for the focus utility ****/
/* NEAT_FOCUS_SCRIPT is the VICAR script created by the "questfocus"
 * utility.  The script performs automated focus analysis, and outputs
 * a focus data file containing the calculated best focus.
 * The focus data file is then used by the observation manager to
 * set the telescope focus at the beginning of an observation session.
 * Both of the files below are located in NEAT_SYSDIR.
 */
#define NEAT_FOCUS_SCRIPT "questfocus.pdf" /* VICAR script from autofocus */
#define NEAT_FOCUS_OUT 	"focus.dat" /* focus data file output from script */

#endif /* _NEATCONF_H_ */
