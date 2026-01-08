/*
 * $RCSfile: telmount.cc,v $
 * $Revision: 1.8 $
 * $Date: 2010/10/12 14:37:27 $ 
 *
 * The telescope mount object.
 * The functions in this file provide a means to command the
 * mount to move to given positions, stop moving, set dome position,
 * start tracking, change the telescope focus, etc.
 * These operations are implemented using the command set defined
 * by the GEODSS telescope mount.  Communication with the mount
 * is handled by the driver interface routines found in the separate
 * file gtm_drv.c, which is called here through the gtm_*() functions.
 *
 * This version interacts directly with the mount device.
 *
 *
 * Notes about La Silla installation:
 * telmount::stow calls telescope_controller::stow
 * where telescope controller goes to zenith, wait for zenith move to complete,
 * the stows the telescope without checking if the dome is open or closed,
 * homes the dome, and then closes the dome.
 *
 * After the tescope controller completes the stow, the telmount stow waits
 * for the telescope move to complete without checking if dome is open or closed.
 *
 * If the telescope fails to go to zenith or fails to move during the stow by the
 * telescope controller, it still tries to home and close the dome
 *
 *****************************************************************************
 * Copyright (c) 1995, California Institute of Technology.
  U.S. Government sponsorship is acknowledged.
 *****************************************************************************
 *
 * Steve Groom, JPL
 * 7/22/95
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <tm_angles.h>
#include <tm_rate.h>
#include <site.h>
#include <tm_dome.h>
#include <tm_focus.h>
#include <telmount.h>
#include <neattime.h>
#include <precess.h>

#define POS_READ_RETRIES 3
#define VERBOSE 0

//XXXX debugging
//XXXX
// fake mount is not needed if questctl.c is called with "-f" argument.
// However, other code importing telmout might required FAKEMOUNT for
// testing
/* #define FAKEMOUNT */

#ifdef FAKEMOUNT
polgdec_enc	FAKEMOUNT_pde_curr;
#define FAKEMOUNT_ra_curr "   0.0000000"
#define FAKEMOUNT_dec_curr "   0.0000000"
#endif	// FAKEMOUNT
// debugging vars for tcu interface and tcu object program
EXTERN int tel_drv_debug;
EXTERN int tel_ctlr_debug;
using namespace std;

// Constructor
telmount::telmount(){
  //telmount(0);
  init_telmount(0,0,0.0);
}

telmount::telmount(int fake_flag, double fake_ut_offset) {
    int dome_flag = 0; // Do not override dome. No ops when dome is closed
    init_telmount(fake_flag,dome_flag, fake_ut_offset);
}

telmount::telmount(int fake_flag, int dome_flag, double fake_ut_offset) {
    init_telmount(fake_flag,dome_flag, fake_ut_offset);
}
   
void telmount::init_telmount(int fake_flag, int dome_flag, double fake_ut_offset) {
    fail = 0;

    if(VERBOSE){
      fprintf(stderr,"telmount: initializing  with fake_flag %d, dome_flag %d, and ut_offset %7.3f\n",fake_flag,dome_flag,fake_ut_offset);
      fflush(stderr);
    }

    ut_offset = fake_ut_offset;
    override_dome = dome_flag;
    // load configuration file
    char confname[NEAT_FILENAMELEN];
    (void) sprintf(confname,"%s/%s",NEAT_SYSDIR,NEAT_CONFIGFILE);
    if(VERBOSE){cerr << "telmount: loading file " << confname << endl;fflush(stderr);}
    fprintf(stderr,"telmount: configuring from file %s with fake_flag %d\n",confname,fake_flag);
    if (!tm_site.configure(confname,fake_flag)) {
        fprintf(stderr, "telmount: error loading configuration file %s\n",
               confname);
        fail = 1;
        return;
    }

    // figure out if we are in debugging mode
		//    debug = tm_site.debug();
		//
		debug=0;
		//
		//
    point_timeout = tm_site.mount_point_timeout();
    if (debug >= 1) {
        cerr << "telmount: debugging output is turned on" << endl;
        cerr << "telmount: point_timeout: " << point_timeout << " seconds"
	     << endl;
    }
    if (debug >= 2) {
	cerr << "telmount: debugging output of tcu controlling object on"
	     << endl;
	tel_ctlr_debug = 1;
    }
    // turn on debugging output of rcc/tcu communication
    if (debug >=3 ) {
	cerr << "telmount: debugging output of rcc/tcu communication on"
	     << endl;
	tel_drv_debug = 1;
    }

#ifdef FAKEMOUNT
    cerr << "telmount: MOUNT NOT AVAILABLE, WILL BE SIMULATED" << endl;
#else // FAKEMOUNT
// connect pointer to actual TCU (telescope control unit)
    if(VERBOSE){
         cerr << "telmount: initializing telescope_controller " << endl;
         fflush(stderr);
    }
    tcu = new telescope_controller(tm_site.com_port(),tm_site.mount_point_timeout(),tm_site.server_name(), ut_offset);
    if (tcu == NULL) {
        cerr << "telmount constructor FAILED" << endl;
        fprintf(stderr, "telmount constructor FAILED\n");
        fail = 1;
        return;
    }

    if(VERBOSE){
         cerr << "telmount: enabling servos " << endl;
         fflush(stderr);
    }

   if(tcu->enable_servos()!=0){
         cerr << "telmount: could not enable servos" << endl;
         fflush(stderr);
        fail = 1;
        return;
   }


    if(VERBOSE){
         cerr << "telmount: checking dome status " << endl;
         fflush(stderr);
    }
    // check a few things to make sure we are ready to go
    // is the dome shutter open?
    int ds_status;
    if ((ds_status = tcu->dome_shutter_status()) != ds_open) {
	if (ds_status == ds_stat_unknown) {
	    sleep(30);
	    if ((ds_status = tcu->dome_shutter_status()) != ds_open) {
		fprintf(stderr, "telmount constructor failed, dome is not open\n");
		fail = 1;
		return;
	    }
	}
    }
    // are there any faults?
    if(VERBOSE){
         cerr << "telmount: checking faults " << endl;
         fflush(stderr);
    }
#if 1
   if (tcu->fault_status() != no_fault) {
	fprintf(stderr, "telmount constructor failed, fatal fault detected\n");
        fail = 1;
	return;
    }
#endif
    // always issue stop command when starting operations
    if(VERBOSE){
         cerr << "telmount: issuing stop command " << endl;
         fflush(stderr);
    }
    if (tcu->stop() != 0) {
	fprintf(stderr, "telmount constructor failed issuing stop command\n");
	fail = 1;
	return;
    }

    // initialize telescope
    if(VERBOSE){
         cerr << "telmount: initializing " << endl;
         fflush(stderr);
    }
    if (tcu->initialize(&tm_site) != 0) {
	fprintf(stderr, "telmount constructor failed initializing telescope\n");
	fail = 1;
	return;
    }
   
    if(VERBOSE){
         cerr << "telmount: slaving dome " << endl;
         fflush(stderr);
    }
    if (tcu->slave_dome() != 0) {
	fprintf(stderr, "telmount constructor failed slaving dome\n");
	fail = 1;
	return;
    }

    if(VERBOSE){
         cerr << "telmount: slaving wind screen " << endl;
         fflush(stderr);
    }

    if (tcu->slave_windscr() != 0) {
	fprintf(stderr, "telmount constructor failed slaving windscr\n");
	fail = 1;
	return;
    }

    if(VERBOSE){
         fprintf(stderr,"telmount: setting site coords to %lf %lf\n",
           tm_site.lon(),tm_site.lat());
         fflush(stderr);
    }
    tcu->setcoords(tm_site.lon(), tm_site.lat());
#endif	// FAKEMOUNT

    fprintf(stderr,
            "telmount: Configured for site %s (lat %f,  lon %f,  elev %fm)\n",
           tm_site.name(),tm_site.lat(),tm_site.lon(),tm_site.elev());

    if(VERBOSE){
         cerr << "telmount: getting focus setting" << endl;
         fflush(stderr);
    }
    double focus;
    if (getfocus(&focus) == -1) {
	fprintf(stderr, "telmount: Failed getting focus value\n"); 
	fail = 1;
    }

    if(VERBOSE){
         cerr << "telmount: getting dome status" << endl;
         fflush(stderr);
    }
    struct d_status dome_stat;
    if (tcu->dome_status(dome_stat) == -1) {
	fprintf(stderr, "telmount: Failed getting dome status\n");
	fail = 1;
    }
    tm_dome_az = dome_stat.domep;
    tm_focus = focus;

    if(VERBOSE){
      fprintf(stderr,"moving telescope to zenith\n");
    }

/*
    if(move_zenith()!=0){
        fprintf(stderr,"could not move the telescope to zenith. Aborting..\n");
        fail = 1;
    }
*/

    if(VERBOSE){
         cerr << "telmount: done initializing telmount" << endl;
         fflush(stderr);
    }
}

// Destructor

telmount::~telmount() {
    // put things into a safe state and shut down
    if (fail) return;
    // stop telescope
    tcu->stop();
    tcu->stow();
    delete tcu;
}

// full status of telescope
int 
telmount::telescope_status(){

#ifdef FAKEMOUNT
    cerr << "telmount::tracking_status: mount not available" << endl;
    return 0;
#else // FAKEMOUNT
   int status=tcu->tracking_status();
   return 0;
#endif
}

// request status from tcu and report that status in integer form
int
telmount::tracking_status() {
    fprintf(stderr, "telmount::tracking_status\n");

    if (fail) {
        return -1;
    }
#ifdef FAKEMOUNT
    cerr << "telmount::tracking_status: mount not available" << endl;
    return 0;
#else // FAKEMOUNT


    int status = tcu->tracking_status();
    switch (status) {
	case TELESCOPE_PARKED:
	    fprintf(stderr, "telmount::tracking_status: telescope parked\n");
	    break;
	case TELESCOPE_SLEWING:
	    fprintf(stderr, "telmount::tracking_status: telescope slewing\n");
	    break;
	case TELESCOPE_TRACKING_ON_TARGET:
	    fprintf(stderr,
		   "telmount::tracking_status: TCU in star track mode and on target\n");
	    //status = 0;
	    break;
	case TELESCOPE_TRACKING_OFF_TARGET:
	    fprintf(stderr,
		   "telmount::tracking_status: TCU in star track mode, off target\n");
 	    //status = 0;
	    break;
	case TELESCOPE_DOME_MOVING:
	    fprintf(stderr, "telmount::tracking_status: dome still moving\n");
	    break;
	case TELESCOPE_STATUS_UKNOWN:
	    fprintf(stderr, "telmount::tracking_status: error getting tracking_status\n");
	    break;
	default:
	    fprintf(stderr, "telmount::tracking_status: unknown response from TCU, %d\n",
		   status);
	    //status = -1;
	break;
    }
    return status;
#endif // FAKEMOUNT
}

int
telmount::dump() {
    fprintf(stderr, "telmount::dump\n");
    if (fail) {
        return -1;
    }
    fprintf(stderr,
           "telmount::dump: dump command not available for this mount\n");

    return 0;
}

int
telmount::stopmount() {
    fprintf(stderr,"telmount::stopmount\n");
    if (fail) {
        return -1;
    }
#ifdef FAKEMOUNT
    fprintf(stderr,"telmount::stopmount: mount not available\n");

#else // FAKEMOUNT
    int status = tcu->stop();
    if (status == -1) {
        fprintf(stderr, "telmount::stopmount: error communicating with TCU\n");
        return -1;
    } else if (status != 0) {
        fprintf(stderr, "telmount::stopmount: stop request rejected by TCU\n");
        return -1;
    }
#endif // FAKEMOUNT

    return 0;
}

// Move to stow position, tracking off
int
telmount::stow() {
    if (fail) {
        return -1;
    }
    // Point at stow position and hold there.
    int status = tcu->stow();
    if (status == -1) {
        fprintf(stderr, "telmount::stow: error communicating with TCU\n");
        return -1;
    } else if (status != 1) {
        fprintf(stderr, "telmount::stow: stow request rejected by TCU\n");
        return -1;
    }

    fprintf(stderr, "telmount::stow: waiting for stow to complete\n");

    // wait for stow to complete.Don't check for open dome during wait

    int targ_stat;
    targ_stat =  wait_ontarget(0);
    fprintf(stderr, "telmount::stow: wait_ontarget returned %d\n",
	   targ_stat);

    if (targ_stat == -1) {
       fprintf(stderr,
           "telmount::stow: telescope not stowing\n");
       return -2;
    } else if (targ_stat == -2) {
       fprintf(stderr, "telmount::stow: failed waiting for stow\n");
       return -1;
    } else if (targ_stat != TELESCOPE_PARKED){
       fprintf(stderr, "telmount::stow: ERROR: telescope did not park during stow\n");
       return -1;
    }

    return 0;
}

// Move to zenith position, tracking off
// Wait for move to zenith to complete before returning
// Return -1 on failure
// Return -2 on wait_ontarget fault
// Return 0 on success
int
telmount::move_zenith() {
    if (fail) {
        return -1;
    }
    // Point at zenith position and hold there.
    int status = tcu->zenith();
    if (status == -1) {
        fprintf(stderr, "telmount::move_zenith: error communicating with TCU\n");
        return -1;
    } else if (status != 0) {
        fprintf(stderr, "telmount::move_zenith: stow request rejected by TCU\n");
        return -1;
    }

    fprintf(stderr, "telmount::move_zenith: waiting for move to zenith to complete\n");

    // check for dome open during wait on target

    int targ_stat;
    targ_stat = wait_ontarget(1);
    fprintf(stderr, "telmount::move_zenith: wait_ontarget returned %d\n",
	   targ_stat);

    if (targ_stat == -1) {
       fprintf(stderr,
           "telmount::move_zenith: telescope not stowing\n");
       return -2;
    } else if (targ_stat == -2) {
       fprintf(stderr, "telmount::move_zenith: failed waiting for stow\n");
       return -1;
    }

    fprintf(stderr, "telmount::move_zenith: reached zenith, now stopping drive.\n");

    status = tcu->stop();
    if (status == -1) {
        fprintf(stderr, "telmount::move_zenith: error communicating with TCU\n");
        return -1;
    } else if (status != 0) {
        fprintf(stderr, "telmount::move_zenith: stoRprequest rejected by TCU\n");
        return -1;
    }

    fprintf(stderr, "telmount::move_zenith: telescope now stopped at zenith.\n");

    return 0;
}


// Gets current position of telescope. Returns RA/DEC in
// hours and degrees.
int
telmount::getpos(double *ra_p, double *dec_p, double *uxtime_p) {
    if (fail) {
        return (-1);
    }

    double epoch, lst, ha;
    struct point current_position;

#ifdef FAKEMOUNT
    // faked for testing without mount hardware
    *uxtime_p=neat_gettime_utc(ut_offset);
    strcpy(ra_c,FAKEMOUNT_ra_curr);
    strcpy(dec_c, FAKEMOUNT_dec_curr);
    sscanf(ra_c,"%lf",ra_p);
    sscanf(dec_c,"%lf",dec_p);
#else // FAKEMOUNT

    if (VERBOSE) {
        cerr << "telmount::getpos_rd: checking status" << endl;
    }
    tcu->get_position(current_position, &epoch, &lst, &ha);
    if (VERBOSE) {
        cerr << "telmount::getpos_rd: TCU returned ra: "
        << current_position.ra << " dec: " << current_position.dec
        << endl;
    }
    if (current_position.ra == -999.9) {
        fprintf(stderr,
               "telmount::getpos_rd: unable to get point status from TCU\n");
        return (-1);
    }
    *uxtime_p=neat_gettime_utc(ut_offset);
    *ra_p=current_position.ra;
    *dec_p=current_position.dec;

    if (debug) {
        cerr << "telmount::getpos_rd: current position ra: " << *ra_p
        << " dec: " << *dec_p << " time: " << *uxtime_p << endl;
    }
#endif
    return 0;
}

#if 0
// Read the current mount position, return RA/DEC J2000 coordinates
// Uses getpos_rd() to read out pol/gdec position, then converts to ra/dec.
// Returns 0 on success, -1 on failure.
int
telmount::getpos(double *ra_p, double *dec_p, double *uxtime_p) {
    if (fail) {
        return -1;
    }

    double ra, dec, mjd, uxtime;
    if (this->getpos_rd(&ra,&dec,&uxtime) == -1) {
        return -1;
    }

    if(VERBOSE){
        cerr << "getpos: ra = " << ra << " dec = " << dec << endl;
    }

    mjd = uxt_mjd(uxtime);
    precess(mjd,MJ2000,&ra,&dec);
    if(VERBOSE){
        cerr << "getpos: after precession :\n ra = " << ra << " dec = " << dec << endl;
    }
    if (ra_p != NULL) {
        *ra_p = ra;
    }
    if (dec_p != NULL) {
        *dec_p = dec;
    }
    if (uxtime_p != NULL) {
        *uxtime_p = uxtime;
    }
    return 0;
}
#endif

// Return the current focus position.
// We can't read this from mount so we use the last known
// focus setting.
// Returns 0 or -1 on either success or failure.
int
telmount::getfocus(double *focus_p) {
    if (focus_p != NULL) {
	if ((*focus_p = tcu->get_focus()) < MIN_FCS) {
	    fprintf(stderr,
		   "telmount::getfocus: failed retrieving focus from telescope\n");
	    return -1;
	}
    }
    fprintf(stderr, "telmount::getfocus: current focus: %f\n", *focus_p);
    return 0;
}

// Returns the current weather data.
// The struct, weather_data is defined in telescope_controller.h
// 0 on OK, -1 on failure
int
telmount::getweather(struct weather_data* current_weather) {
    if (current_weather != NULL) {
        *current_weather = tcu->get_weather_data();
    }
    if (current_weather->temp == -999.9) {
        return -1;
    }
    return 0;
}

// returns last commanded dome az position
int
telmount::getdomeaz(double& dome_az) {
    dome_az = tm_dome_az;
    return 0;
}

// Point in RA/DEC (hours/degrees coordinates)
// Returns -1 on some mount error,
// Returns -2 if pointing is rejected as unsafe.
int
telmount::point_rd(double ra, double dec, pointmode mode) {
    fprintf(stderr,"telmount::point_rd: ra %f dec %f\n",ra,dec);
    if (fail) {
        return -1;
    }

    // Move to position.
    fprintf(stderr, "telmount::point_rd: moving to ra %f, dec %f\n", ra, dec);

    int move_result = do_move_rd(ra,dec,mode);
    return move_result;

}


// Point in RA/DEC (J2000) coordinates
// Returns -1 on some mount error,
// Returns -2 if pointing is rejected as unsafe.
int
telmount::point(double ra, double dec, pointmode mode) {
    if (debug) {
        cerr << "telmount::point" << endl;
    }
    if (fail) {
        return -1;
    }

    //double uxtime = neat_gettime_utc(ut_offset);
    //double mjd = uxt_mjd(uxtime);

    // precess RA/DEC to epoch of date
    //precess(MJ2000,mjd,&ra,&dec);

    // send off point
    return point_rd(ra,dec,mode);
}

// Turns off mount tracking, leaving mount pointed where it is.
// Returns 0 on success, -1 on error.
int
telmount::stop() {
    if (debug) {
        cerr << "telmount::stop" << endl;
    }
#ifdef FAKEMOUNT
    fprintf(stderr, "telmount::stop: mount not available\n");
#else // FAKEMOUNT
    int status = tcu-> stop();
    if (status == -1) {
        fprintf(stderr, "telmount::stop: unable to communicate with TCU\n");
        return status;
    } else if (status != 0) {
        fprintf(stderr, "telmount::stop: stop request rejected by TCU\n");
        return -1;
    }
#endif // FAKEMOUNT
    return 0;
}

// set focus position
// Returns 0 on success, -1 on mount error, -2 if bad focus value is detected.
int
telmount::focus(double fset) {
    if (debug) {
        cerr << "telmount::focus" << endl;
    }

// DLR DEBUG
// ffprintf(stderr,"telmount focus routine\n");fflush(stderr);

    if (fail) {
        return -1;
    }

    double t1, t2;
    t1 = neat_gettime_utc(ut_offset);

    fprintf(stderr, "telmount::focus: asked to set focus to %f\n", fset);
    if (fset < MIN_FCS || fset > MAX_FCS) {
	fprintf(stderr, "telmount::focus: setting %f out of allowed range: %lf to %lf\n",
	       fset,MIN_FCS,MAX_FCS);
        return -2;
    }

    // set new focus and wait for it to completed
    fprintf(stderr, "telmount::focus: sending command to set new focus\n");

    int cmd_sts;
    cmd_sts = tcu->set_focus(fset);
    if (cmd_sts != 1) {
	cerr << "telmount::focus: unable to set focus" << endl;
 	return -1;
    }

    if ((tm_focus = tcu->get_focus()) < MIN_FCS) {
	fprintf(stderr, "telmount::focus: failed getting focus position\n");
	return -2;
    }
    else{
        fprintf(stderr, "telmount::focus: final focus is %lf\n",tm_focus);
    }

    fprintf(stderr, "telmount::focus: done\n");

    return 0;
}

// target - wait for TCU to report "on target"
// To accomplish this the telescope is polled for status
// and that status is checked to see if it says it is on target.
// Return -2 on failure to get on any fault
// Return -1 on timeout
// Return 0 if on target
//
// set dome_check_flag to 1 to check for dome open during wait
// set dome_check_flat to 0 to skip dome check

int
telmount::wait_ontarget(int dome_check_flag) {
    double t1,t2;

    if (debug) {
        cerr << "telmount::wait_ontarget" << endl;
    }
    if (fail) {
        return -2;
    }

    char my_name[24] = "telmount::wait_ontarget";

    int status = TELESCOPE_SLEWING;
    t1 = neat_gettime_utc(ut_offset);
 
    while (status == TELESCOPE_SLEWING || status == TELESCOPE_DOME_MOVING) {
	int fault_stat = 0, dome_shutter_stat = 0;
	
	// first off check for any faults
	if ((fault_stat = tcu->fault_status()) != 0) {
	    // telescope faulted, we can't operate
	    fprintf(stderr, "%s: telescope faulted, %d, dome: %d\n", my_name,
		   fault_stat, dome_shutter_stat);
	    // report information to log.operator
	    report_fault(fault_stat, dome_shutter_stat);
	    return -2;
	}


	// check if dome shutter is open or closed (better be open)
	if (dome_check_flag){
	  if ((dome_shutter_stat = tcu->dome_shutter_status()) != ds_open){
             if(dome_shutter_stat == ds_comm_failed){
	         fprintf(stderr, "%s: tcu comm failed during dome shutter check\n",
		   my_name);
	         return -2;
	     }
	
	     else if (dome_shutter_stat == ds_closed) {
	        fprintf(stderr, "%s: dome shutter closed, %d\n", my_name,
		   dome_shutter_stat);
	      //return -2;
	     }
	     else {
	        fprintf(stderr, "%s: dome shutter status unknown, %d\n", my_name,
		   dome_shutter_stat);
             }
	   }
	   fprintf(stderr, "%s: fault status: %d dome status %d\n", my_name,
	       fault_stat, dome_shutter_stat);
        }

        status = tcu->tracking_status();
        if (status == -1) {
            fprintf(stderr, "%s: unable to communicate with TCU\n", my_name);
	        report_fault(fault_stat, dome_shutter_stat);
            return -2;
        }

        // check to see if point timeout has been reached and we have not
        // yet reach our point
        t2=neat_gettime_utc(ut_offset);
        if (t2-t1 > point_timeout) {
            fprintf(stderr, "%s: telescope not reaching position\n", my_name);
	        report_fault(fault_stat, dome_shutter_stat);
            return -1;
        }
	sleep(1);
    }

    fprintf(stderr, "%s: waited %f seconds\n", my_name, t2-t1);

    //return 0;
    // Change made 2010 03 15 to report error when telescope is parked and should be tracking
    return status;
}

// execute a telescope position move to RA/DEC in degrees.
// mode specifices sidereal tracking or fixed pointing.
// Return -1 on some type of device error.
// Returns -2 if point is rejected as unsafe.
// Current implementation always accepts any point as safe.
int
telmount::do_move_rd(double ra, double dec, pointmode mode) {
    fprintf(stderr, "telmount::do_move_rd\n");
    char my_name[21] = "telmount::do_move_rd";

    if (fail) {
	    report_fault(no_fault, ds_stat_unknown);
        return -1;
    }
    
    // first check for any faults that may have happened.
    int fault_stat = 0;
    if ((fault_stat = tcu->fault_status()) < 0) {
        fprintf(stderr, "%s: tcu faulted with value %d\n", my_name, fault_stat);
        report_fault(fault_stat, ds_stat_unknown);
        return -1;
    }
    fprintf(stderr, "%s: tcu fault status is %d\n", my_name, fault_stat);

    // now check to make sure the dome is open or mismatched (closed is bad
    // unknown is bad)
    int dome_stat = tcu->dome_shutter_status();
    if (dome_stat <= 0 && override_dome ==0 ) {
	  fprintf(stderr, "%s: dome fault, status is %d\n", my_name, dome_stat);
	  report_fault(fault_stat, dome_stat);
	  return -1;
    }
    fprintf(stderr, "%s: dome status: %d\n", my_name, dome_stat);

    double zenith_ra, zenith_dec;
    int check_status=check_pointing(ra,dec,&zenith_ra,&zenith_dec);
    if(check_status==-1){
      fprintf(stderr,"telmount::do_move_rd: new pointing below horizon. Rejecting.\n");
      return -1;
    }
    else if (check_status==-2){
     fprintf(stderr,"telmount::do_move_rd: could not check elevation of new pointing\n");
     return(-1);
    }
    else if(check_status==1){
      fprintf(stderr,"telmount::do_move_rd: new elevation close to dec horizon. Moving to zenith first\n");
    }

    double move_start_time = neat_gettime_utc(ut_offset);


    if(check_status==1){

      fprintf(stderr, "telmount::do_move_rd: pointing telescope to zenith\n");
      int status = move_zenith();
      if (status != 0) {
          fprintf(stderr, "telmount::do_move_rd: unable to point telescope to zenith\n");
          report_fault(fault_stat, dome_stat);
          return -1;
      } else {
        fprintf(stderr, "telmount::do_move_rd: telescope at zenith\n");
      }

      fprintf(stderr,"telmount:do_move_rd: now moving to requested pointing\n");  
    }

    struct point rda;
    rda.ra = ra;
    rda.dec = dec;

    fprintf(stderr, "telmount::do_move_rd: sending ra %f dec %f\n", rda.ra,
           rda.dec);

    // send position command.
    int status = tcu->point(rda,mode);
    if (status != 0) {
        fprintf(stderr, "telmount::do_move_rd: unable to point telescope\n");
	    report_fault(fault_stat, dome_stat);
        return -1;
    } else {
        fprintf(stderr, "telmount::do_move_rd: point accepted\n");
    }

    fprintf(stderr, "telmount::do_move_rd: elapsed time: %f\n",
           (neat_gettime_utc(ut_offset) - move_start_time));

    
    fprintf(stderr, "telmount::do_move_rd: waiting for target acquisition\n");

    // check for dome open during wait on target
    int targ_stat;
    targ_stat = wait_ontarget(1);
    fprintf(stderr, "telmount::do_move_rd: wait_ontarget returned %d\n",targ_stat);

    if (targ_stat == -1) {
       fprintf(stderr,
           "telmount::do_move_rd: telescope can't acquire target\n");
       return -2;
    } else if (targ_stat == -2) {
       fprintf(stderr, "telmount::do_move_rd: failed waiting on target\n");
       return -1;
    } else if (targ_stat != TELESCOPE_TRACKING_ON_TARGET){
       fprintf(stderr, "telmount::do_move_rd: ERROR: telescope not tracking on target\n");
       return -1;
    }
  
    // get dome az so tm_dome_az is correct
    struct d_status dome_pos_stat;
    if (tcu->dome_status(dome_pos_stat) == -1) {
	fprintf(stderr, "telmount::do_move_rd: failed getting dome status\n");
	cerr << "telmount::do_move_rd: failed getting dome status" << endl;
	return -1;
    }
    fprintf(stderr, "telmount::do_move_rd: dome position %f\n",
	   dome_pos_stat.domep);
    tm_dome_az = dome_pos_stat.domep;

    double now = neat_gettime_utc(ut_offset);
    double movetime = now - move_start_time;
    fprintf(stderr, "telmount::do_move_rd: move completed in %f seconds\n",
           movetime);

    if(mode==TELMOUNT_POINT){
       fprintf(stderr, "telmount:do_move_rd: point mode is TELMOUNT_POINT. Stopping telescope\n");
       if (tcu->stop()!=0){
          fprintf(stderr,"telmount:do_move_rd: could not stop telescope\n");
          return -1;
       }
    }

    return 0;
}

// check that new position is above horizon limit, and if a slew to zenith
// should be made first
// Return -1 if below horizon limit
// Return 1  if slew to zenith required first
// Return 0 if pointing OK
// Return -2 if unable to check pointing
int telmount::check_pointing(double ra, double dec, double *zenith_ra, double *zenith_dec){

  double epoch, lst, current_ha;
  struct point current_position;
  
  tcu->get_position(current_position, &epoch, &lst, &current_ha);
  *zenith_ra = lst;

  if(epoch=0.0){
     fprintf(stderr,"telmount::check_pointing: couldn't get currrent position\n");
     return(-2);
  }

  double lat = tm_site.lat();
  fprintf(stderr,"telmount::get_position: site latitude is %lf\n",lat);
  *zenith_dec = lat;

  fprintf(stderr,"telmount::check_pointing: current position is %lf %lf\n",
	current_position.ra, current_position.dec );

  // calculate azimuth and elevation of next pointing
  // check if it below the elevation limit. Return with -2 if o

  radec_ang rapoint(ra, dec);
  hadec_ang hapoint(rapoint, lst);
  azel_ang  aepoint(lat, hapoint);
  double az = aepoint.az;
  double el = aepoint.el;

  fprintf(stderr,"telmount:check_pointing: new az and el will be %lf %lf\n",az,el);
  if (el < tm_site.min_elevation()) {
     fprintf(stderr,"telmount:check_pointing: new pointing below elevation limit at %lf\n",
		tm_site.min_elevation());
     return -1;
  }


  // check if new pointing is with safe_elevation of the dec position of the horizon
  // behind the telescope peer.
  // if so, return with 1 to indicate slew to zenith before slew to new position

  double dec_horizon;
  if(lat>0.0) dec_horizon = lat - 90.0;
  else dec_horizon = 90.0 + lat;
  
  fprintf(stderr,"telmount: check_pointing: dec at horizon behind peer is %lf\n",dec_horizon);
  if(fabs(dec-dec_horizon)<tm_site.safe_elevation()){
    fprintf(stderr,"telmount:get_position: new dec is close to dec at horizon. Slew to zenith first\n");
    return(1);
  }

  fprintf(stderr,"telmount:check_pointing: new pointing is safe. No precautions\n");

  return 0;
}
double
dtor(double degrees) {
    double radians;
    radians = (degrees*PI)/180;
    return radians;
}

double
telmount::rtod(double radians) {
    double degrees;
    degrees = (radians*180)/PI;

    return degrees;
}

double
htor(double hours) {
    double radians;
    radians = (hours*15*PI)/180;
    return radians;
}

double
telmount::rtoh(double radians) {
    double hours;
    hours = ((radians*180)/PI)/15;
    return hours;
}

double calc_time_move(double ra1, double dec1,
                      double ra2, double dec2) {

    double move_error;
    double delay_seconds;
    ra1 = ra1*15;
    ra2 = ra2*15;
    move_error = fabs(ra2 - ra1) +
                 fabs(dec2 - dec1);

    delay_seconds = move_error/3;

    return delay_seconds;
}

/**************************************************************************
 * METHOD: telmount::report_fault(int, int)
 *
 * DESCRIPTION:
 *   If we take a big ol' dump we want to report the event to log.operator.
 *   This function does that along with some smarts to figure out
 *   what information we can report. We should report everything if we can.
 *   This means weather, focus, dome and anything else.
 **************************************************************************/
void telmount::report_fault(int fault_status, int dome_status) {
    char my_name[23] = "telmount::report_fault";
    char fault_name[27] = "critical telescope failure";

    // check either of the provided fault values to see if
    // they indicate that the communication between us and the
    // the telescope control computer has died.

    if (fault_status == comm_fault || dome_status == d_comm_failed) {
	// come failed or something else drastic, chances are we
	// can't communicate with the tcu/acu so just report
	// what we can
        fprintf(stderr,
               "%s: %s, fault status: %d, dome_status: %d\n", my_name,
               fault_name, fault_status, dome_status);
        return;
    }

    // otherwise comm is fine and we can provide a full report
    char fault_word[127];
    switch (fault_status) {
	case no_fault:
	    strcpy(fault_word, "No Fault");
	    break;
	case drive_fault:
	    strcpy(fault_word, "Drives disabled");
	    break;
	case ra_limit_fault:
	   strcpy(fault_word, "RA limit");
	    break;
	case dec_limit_fault:
	   strcpy(fault_word, "Dec limit");
	   break;
	case horiz_limit_fault:
	   strcpy(fault_word, "Horizon limit");
	   break;
	default:
	   strcpy(fault_word, "Unknown Fault");
	   break;
    }

    char dome_word[127];
    switch (dome_status) {
	case ds_open:
	    strcpy(dome_word, "Open");
	    break;
	case ds_closed:
	    strcpy(dome_word, "Closed");
	    break;
	case ds_stat_unknown:
	    strcpy(dome_word, "Status Unknown");
	    break;
	 default:
	    strcpy(dome_word, "Unknown Fault");
	    break;
    }

    // grab telescope, weather, acu and ccu status data
    struct tcu_status cur_tel_stat;
    struct weather_data cur_wea_data;
    if (tcu->stat_req(0,cur_tel_stat) < 0) {
        fprintf(stderr, "%s: failed retrieving telescope status\n",
               my_name);
        return;
    }

    cur_wea_data = tcu->get_weather_data();
    if (cur_wea_data.temp <= -999.0) {
        fprintf(stderr, "%s: failed retrieving weather data\n",
               my_name);
        return;
    }
    
    // build strings for mode, acu and ccu status
    char mode_string[255] = "";

    int i = 0;
    char tmp_str[16];
    for (i=0;i<tot_modes;i++) {
        sprintf(tmp_str, "%d:%d ", i, cur_tel_stat.mode[i]);
        strcat(mode_string, tmp_str);
    }


    // ok, we should have all the status, now to print it.
    fprintf(stderr, "%s: Ops shutdown due to fault\n", my_name);
    fprintf(stderr, "%s:\tTel Fault status: %s, Dome status: %s\n",
	   my_name, fault_word, dome_word);
    fprintf(stderr,
	   "%s:\tTel status: telp ra: %f dec: %f, focusp: %d, domep: %f,",
	   my_name, cur_tel_stat.hap, cur_tel_stat.decp, cur_tel_stat.focusp,
	   cur_tel_stat.domep);
    fprintf(stderr,
	   "%s:\t\twindscrp: %f, mode: %s\n", my_name, cur_tel_stat.windscreenp,
	   mode_string);
    fprintf(stderr,
	   "%s:\tWeather Data: temp: %f, humidity: %f, wind speed: %f,",
	   my_name, cur_wea_data.temp, cur_wea_data.humidity,
	   cur_wea_data.wind_speed);
    fprintf(stderr, "%s:\t\twind direction: %f, dew point: %f\n",
	   my_name, cur_wea_data.wind_direction, cur_wea_data.dew_point);

}

