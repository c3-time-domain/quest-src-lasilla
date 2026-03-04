/**************************************************************************
 * $RCSfile: telescope_controller.cc,v $
 * $Revision: 1.8 $
 * $Date: 2010/10/12 14:37:27 $
 * Author - Erik Hovland
 *
 * telescope_controller - provides a C++ class interface to the Palomar
 * telescope controller unit (TCU/ACU). This class is used by the telmount
 * class to control the telescope.
 **************************************************************************/

#include <telescope_controller.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <neattime.h>
#define VERBOSE 1 

// parameters for sendcmd_loop
#define SEND_LOOP_TRIALS 10
#define SENDCMD_LOOP_DELAY 1

//#define STOW20 // define to stow at elevation 20 degrees 

/* swap open and close bits of dome state so the control
software thinks the dome is open when it is really closed */
//#define TRICK_DOME_OPEN

/* multiply focus value by 10 to fix bug in TCS output stream. Only
works if upper limit to focus fixed at -100000 */
//#define FOCUS_FIX 

/* if declared, don't send auto dome commands */
//#define DOME_OFF 

// debug global var
//int tel_ctlr_debug = 1 ;

using namespace std;
/**************************************************************************
 * METHOD: telescope_controller::telescope_controller
 *
 * DESCRIPTION:
 *   Default constructor for the telescope controller. Just opens up the
 *   serial port to ready it for commanding. Then makes a quick request
 *   to grab the control of the TCU/ACU.
 **************************************************************************/
telescope_controller::telescope_controller(void) {
    class_name = new char[21];
    strcpy(class_name, "telescope_controller");
    current_position.ra = current_position.dec = 0.0;
    pos_lon = pos_lat = 0.0;
    ut_offset=0;

}

/**************************************************************************/
//
// port is the comport number (1 or 2) used by the TCS computer for communication
// timout is the timeout in seconds for a telescope move.
// srv_name is the name or ip number of the machine hosting the telescope server.
telescope_controller::telescope_controller(int port, int timeout, char *srv_name) {

    double ut_offset = 0.0;
    init_telescope_controller(port,timeout,srv_name,ut_offset);
}


/**************************************************************************/
//
// port is the comport number (1 or 2) used by the TCS computer for communication
// timout is the timeout in seconds for a telescope move.
// srv_name is the name or ip number of the machine hosting the telescope server.
// ut_offset is add to the current UT for simulated observations

telescope_controller::telescope_controller(int port, int timeout, char *srv_name,
double ut_offset) {

    init_telescope_controller(port,timeout,srv_name,ut_offset);
}

/**************************************************************************/
//
// port is the comport number (1 or 2) used by the TCS computer for communication
// timout is the timeout in seconds for a telescope move.
// srv_name is the name or ip number of the machine hosting the telescope server.
// ut_offset is add to the current UT for simulated observations

void telescope_controller::init_telescope_controller(int port, int timeout, char *srv_name, double fake_ut_offset) {
    class_name = new char[21];
    server_name = new char[1024];
    strcpy(class_name, "telescope_controller");
    strcpy(server_name,srv_name);
    current_position.ra = current_position.dec = 0.0;
    pos_lon = pos_lat = 0.0;
    ut_offset = fake_ut_offset;
    com_port=port;
    point_timeout = timeout;
    weather_client =  new char[1024];
    weather_file = new char[1024];
    tcs_prog_name = new char[1024];
    tcs_status_file = new char[1024];
   
    if  (getenv(WEATHER_FILE) != NULL) 
      strcpy(weather_file,getenv(WEATHER_FILE));
    else
      strcpy(weather_file,DEFAULT_WEATHER_FILE);
   
    if  (getenv(WEATHER_CLIENT) != NULL) 
      strcpy(weather_client,getenv(WEATHER_CLIENT));
    else
      strcpy(weather_client,DEFAULT_WEATHER_CLIENT);
   
    if  (getenv(TCS_PROG_NAME) != NULL) 
      strcpy(tcs_prog_name,getenv(TCS_PROG_NAME));

    else
      strcpy(tcs_prog_name,DEFAULT_TCS_PROG_NAME);

    fprintf(stderr,"telescope_controller: tcs_prog_name: %s\n",tcs_prog_name);
   
    if  (getenv(TCS_STATUS_FILE) != NULL) 
      strcpy(tcs_status_file,getenv(TCS_STATUS_FILE));
    else
      strcpy(tcs_status_file,DEFAULT_TCS_STATUS_FILE);
   
    fprintf(stderr,"telescope_controller: tcs_status_file: %s\n",tcs_status_file);
    /*fprintf(stderr,"telescope_controller::telescope_controller: com_port = %d\n",com_port);*/

}

/**************************************************************************
 * METHOD: telescope_controller::~telescope_controller
 *
 * DESCRIPTION:
 *   Destructor. Nothing fancy. Just deletes some pointers.
 **************************************************************************/
telescope_controller::~telescope_controller() {
     delete class_name;
}

/**************************************************************************
 * METHOD: telescope_controller::setcoords(const double&, const double&)
 *
 * DESCRIPTION:
 *   Since the controller needs to know about the site longitude and
 *   latitude we just pass those on from a higher power
 **************************************************************************/
void telescope_controller::setcoords(const double& lon,
				     const double& lat) {
    pos_lon = lon;
    pos_lat = lat;
}
    
/**************************************************************************
 * METHOD: telescope_controller::take_control()
 *
 * under modification by DLR 2008 Mar 7
 *
 * DESCRIPTION:
 *   Send three NOP instructions to the TCS and check the telemtry stream to
 *   see if we get a response. The response code should alternate from "e" to 
 *   "E" with each command. If sucessful, return 0 with our_response = 0.
 *   Otherwise return with our_response = -1.
 **************************************************************************/
int telescope_controller::take_control() {
 
     int i,n_trials=10;
    int cmd_response = -1;

    // construct command
    telescope_command *tel_cmd = new telescope_command(TELESCOPE_CMD_TAKE_CONTROL);
    char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 

#if 0
    i=0;	
    while(cmd_response!=0&&i<n_trials){			      

       tel_cmd_response=sendcmd(tel_cmd);

       /* check the response for valid format and  acknowledgement. This
          returns 0 if the command is valid and the response is good. Otherwise,
          it returns a non-zero integer corresponding to the response problem */

       cmd_response=check_tel_response(tel_cmd_response);
       i++;
 
    }
#else
    tel_cmd_response = sendcmd_loop(tel_cmd);
    cmd_response=check_tel_response(tel_cmd_response);
#endif

    delete tel_cmd;
    delete tel_cmd_response;

    if(cmd_response!=0){
        fprintf(stderr, "%s: unable to take control of TCU\fter three attempts\n", class_name);
        fflush(stderr);
	return cmd_response;
    }

    //now check if telescope is disabled

    return cmd_response;
}


/**************************************************************************
 * METHOD: telescope_controller::stop()
 *
 * DESCRIPTION:
 *   Does an immediate emergency stop of the telescope.
 *   0 on success
 *   1 on failure
 *   -1 on unknown
 **************************************************************************/
int telescope_controller::stop() {
    int cmd_response = -1; 
    char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
    telescope_command *tel_cmd;
    int success= 0; /*assume we will succeed */

    if (take_control() != 0) {
        fprintf(stderr, "%s: unable to take control of TCU\n", class_name);
        cmd_response = -1;
        success=1;
    }

    tel_cmd = new telescope_command(TELESCOPE_CMD_STOP);
    tel_cmd_response = sendcmd(tel_cmd);
    cmd_response=check_tel_response(tel_cmd_response);

    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to stop the telescope\n", class_name);
        cmd_response = 0;
        success=1;
    }
    delete tel_cmd;

    tel_cmd = new telescope_command(TELESCOPE_CMD_STOP_DOME);
    tel_cmd_response = sendcmd(tel_cmd);
    cmd_response=check_tel_response(tel_cmd_response);

    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to stop the dome\n", class_name);
        cmd_response = 0;
        success=1;
    }
    delete tel_cmd;

    tel_cmd = new telescope_command(TELESCOPE_CMD_TRACKOFF);
    tel_cmd_response = sendcmd(tel_cmd);
    cmd_response=check_tel_response(tel_cmd_response);

    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to stop  telescope tracking\n", class_name);
        cmd_response = 0;
        success=1;
    }
    delete tel_cmd;

    delete tel_cmd_response;
    return success;
}
/**************************************************************************
 * METHOD: telescope_controller::disable_servos()
 * DESCRIPTION:
 *   Disable Telescope Servos to make sure telescope does not move
 *   Normally used only at last shutdown, when telescope is in stow position
 *   0 on success
 *   1 on failure
 *   -1 on unknown
 **************************************************************************/
int telescope_controller::disable_servos() {
    int cmd_response = -1; 
    char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
    telescope_command *tel_cmd;
    int success= 0; /*assume we will succeed */

    if (take_control() != 0) {
        fprintf(stderr, "%s: unable to take control of TCU\n", class_name);
        cmd_response = -1;
        success=1;
    }

    //  kill the servos
    tel_cmd = new telescope_command(TELESCOPE_CMD_KILL);
    tel_cmd_response = sendcmd(tel_cmd);
    cmd_response=check_tel_response(tel_cmd_response);

    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to disable telescope servos\n", class_name);
        cmd_response = 0;
        success=1;
    }
    delete tel_cmd;

    delete tel_cmd_response;
    return success;
}
/**************************************************************************
 * METHOD: telescope_controller::enable_servos()
 * DESCRIPTION:
 *   Disable Telescope Servos to make sure telescope does not move
 *   Normally used only at last shutdown, when telescope is in stow position
 *   0 on success
 *   1 on failure
 *   -1 on unknown
 **************************************************************************/
int telescope_controller::enable_servos() {
    int cmd_response = -1; 
    char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
    telescope_command *tel_cmd;
    int success= 0; /*assume we will succeed */

    if (take_control() != 0) {
        fprintf(stderr, "%s: unable to take control of TCU\n", class_name);
        cmd_response = -1;
        success=1;
    }

    //  unkill the servos
    tel_cmd = new telescope_command(TELESCOPE_CMD_UNKILL);
    tel_cmd_response = sendcmd(tel_cmd);
    cmd_response=check_tel_response(tel_cmd_response);

    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to enable telescope servos\n", class_name);
        cmd_response = 0;
        success=1;
    }
    delete tel_cmd;

    delete tel_cmd_response;
    return success;
}
/**************************************************************************
 * METHOD: telescope_controller::initialize()
 *
 * DESCRIPTION:
 *   presets telescope corrections, biases, slewrates, etc.
 *   1 on success
 *   0 on failure
 *   -1 on unknown
 **************************************************************************/
int telescope_controller::initialize(site *tm_site) {
    int cmd_response = -1;
    char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
    telescope_command *tel_cmd;

    target_position.ra=0.0;
    target_position.dec=0.0;

//fprintf(stderr,"tm_site.coordinate_epoch : %lf\n",tm_site->coordinate_epoch());

    com_port=tm_site->com_port();
    if(com_port<1||com_port>2){
       fprintf(stderr,"telescope_controller:initialize: com_port out of range. Value is %d\n",com_port);
       return 0;
    }
    fprintf(stderr,"telescope_controller:initialize: com_port is %d\n",com_port);
    strcpy(server_name,tm_site->server_name());
    fprintf(stderr,"telescope_controller:initialize: server name is %s\n",server_name);

    // take control of telescope
    if (take_control() != 0) {
        fprintf(stderr, "%s: unable to take control of TCU\n", class_name);
        cmd_response = -1;
	return cmd_response;
    }

    cmd_response=enable_servos();
    if(cmd_response!=0){
        fprintf(stderr, "%s: unable to enable telescope drives\n", class_name);
	return cmd_response;
    }

    // sync TCS computer time to host computer time
    if(tm_site->sync_time_on()){
       struct tm tm;
       get_date_time(&tm, ut_offset);
       if (set_telescope_time(&tm) != 0) {
           fprintf(stderr, "%s: unable to set telescope time\n", class_name);
           cmd_response = -1;
	   return cmd_response;
       }
       if(VERBOSE){
          fprintf(stderr,
       "telescope_controller:initialize setting telescope date/time to %d %d %d %d %d %d\n",
	     tm.tm_mon+1, tm.tm_mday, 1900+tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
       }
    }

    // stop any commanded motions
    tel_cmd = new telescope_command(TELESCOPE_CMD_STOP);
    tel_cmd_response = sendcmd(tel_cmd);
    cmd_response=check_tel_response(tel_cmd_response);
    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to stop the telescope\n", class_name);
	return cmd_response;
    }
    delete tel_cmd;

    // turn siderealt tracking off
    tel_cmd = new telescope_command(TELESCOPE_CMD_TRACKOFF);
    tel_cmd_response = sendcmd(tel_cmd);
    cmd_response=check_tel_response(tel_cmd_response);
    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to stop  telescope tracking\n", class_name);
	return cmd_response;
    }
    delete tel_cmd;

    // turn fast slews on
    if(tm_site->fast_slew_on()!=0){
       tel_cmd = new telescope_command(TELESCOPE_CMD_SLEWON);
       tel_cmd_response = sendcmd(tel_cmd);
       cmd_response=check_tel_response(tel_cmd_response);
       if (cmd_response != 0) {
           fprintf(stderr, "%s: unable to enable fast slews\n", class_name);
	   return cmd_response;
       }
    }
    else{
       tel_cmd = new telescope_command(TELESCOPE_CMD_SLEWOFF);
       tel_cmd_response = sendcmd(tel_cmd);
       cmd_response=check_tel_response(tel_cmd_response);
       if (cmd_response != 0) {
           fprintf(stderr, "%s: unable to disable fast slews\n", class_name);
	   return cmd_response;
       }
    }
    delete tel_cmd;

    // set epoch for coordinates
    if (set_epoch(tm_site->coordinate_epoch()) != 0) {
        fprintf(stderr, "%s: unable to set coordinate epoch\n", class_name);
        cmd_response = -1;
	return cmd_response;
    }


    /* turn bias rates on/off */
    if(tm_site->bias_rate_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_BIASON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr, "%s: unable to turn bias rates on\n", class_name);
	    return cmd_response;
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_BIASOFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr, "%s: unable to turn bias rates off\n", class_name);
	    return cmd_response;
        }
    }
    delete tel_cmd;

#ifdef TCS_DEMO
    /* these commands no longer offered. TCS_DEMO only */

    /* turn aberration correction om/off */
    if(tm_site->aberration_correction_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_ABERON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn aberration correction on\n", class_name);
   	    return cmd_response;
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_ABEROFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn aberration correction off\n", class_name);
   	    return cmd_response;
        }
    }
    delete tel_cmd;

    /* turn aberration deconvolution  om/off */
    if(tm_site->aberration_deconvolution_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_UNABERON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn aberration deconvolution on\n", class_name);
   	    return cmd_response;
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_UNABEROFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn aberration deconvolution off\n", class_name);
   	    return cmd_response;
        }
    }
    delete tel_cmd;

    /* turn flexture correction om/off */
    if(tm_site->flexture_correction_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_FLEXON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn flexture correction on\n", class_name);
	    return cmd_response;
        }
    }
    else{
       tel_cmd = new telescope_command(TELESCOPE_CMD_FLEXOFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn flexture correction off\n", class_name);
	    return cmd_response;
        }
    }
    delete tel_cmd;

    /* turn flexture deconvolution off */
    if(tm_site->flexture_deconvolution_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_UNFLEXON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn flexture deconvolution on\n", class_name);
	   return cmd_response;
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_UNFLEXOFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn flexture deconvolution off\n", class_name);
	    return cmd_response;
        }
    }
    delete tel_cmd;

    /* turn nutation correction on/off */
    if(tm_site->nutation_correction_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_NUTON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn nutation correction on\n", class_name);
	    return cmd_response;
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_NUTOFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn nutation correction off\n", class_name);
	    return cmd_response;
        }
    }
    delete tel_cmd;

    /* turn parallax correction on/off */
    if(tm_site->parallax_correction_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_PARAON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn parallax correction on\n", class_name);
	    return cmd_response;
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_PARAOFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn parallax correction off\n", class_name);
	    return cmd_response;
        }
    }
    delete tel_cmd;

    /* turn precession correction on/off */
    if(tm_site->precession_correction_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_PREON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn precession correction on\n", class_name);
	    return cmd_response;
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_PREOFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn precession correction off\n", class_name);
	    return cmd_response;
        }
    }
    delete tel_cmd;

    /* turn proper motion correction on/off */
    if(tm_site->proper_motion_correction_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_PROPERON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn proper motion correction on\n", class_name);
	    return cmd_response;
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_PROPEROFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn proper motion correction off\n", class_name);
	    return cmd_response;
        }    
    }
    delete tel_cmd;

    /* turn refraction correction on/off */
    if(tm_site->refraction_correction_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_REFRACON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn refraction correction on\n", class_name);
	    return cmd_response;
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_REFRACOFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn refraction correction off\n", class_name);
	    return cmd_response;
        }
    }
    delete tel_cmd;

    /* turn refraction deconvolution off */
    if(tm_site->refraction_deconvolution_on()!=0){
        tel_cmd = new telescope_command(TELESCOPE_CMD_UNREFRACON);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn refraction deconvolution on\n", class_name);
	    return cmd_response;
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_UNREFRACOFF);
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        if (cmd_response != 0) {
            fprintf(stderr,
		 "%s: unable to turn refraction deconvolution off\n", class_name);
	    return cmd_response;
        }
    }
    delete tel_cmd;
#endif

    delete tel_cmd_response;
    return cmd_response;
}

/**************************************************************************
 * METHOD: telescope_controller::stow()
 *
 * go to zenith first and wait for move to complete. Then go to stow position
 *
 * DESCRIPTION:
 *   Does an immediate emergency stow of the telescope.
 *   1 on success
 *   0 on failure
 *   -1 on unknown
 **************************************************************************/
int telescope_controller::stow() {
    int cmd_response = -1;
    telescope_command* tel_cmd ;
    char *tel_cmd_response;
    struct tcu_status cur_stat;
    int success=1;
    int zenith_success=1;

    if (take_control() != 0) {
        fprintf(stderr, "%s: unable to take control of TCU\n", class_name);
        success=0;
	return -1;
    }

    if (enable_servos() != 0 ) {
       fprintf(stderr,"%s: unable to enable servos\n", class_name);
    }

    /* check current hour angle. If it is already close to
    zero, then there is no need to go to zenith before stowing.
    Set ok_to_stow_flag to 1.
    */

    if(VERBOSE){
       cerr << "status: issuing first stat request " << endl;
       fflush(stderr);
    }

    if (stat_req(0,cur_stat) == -1) {
	fprintf(stderr, "%s: status request failed\n", class_name);
        success=0;
	return -1;
    }

    /* if currently more than 0.1 hours from the meridian,
       go to zenith before stowing. THis is to make sure
       the telescope doesn't hit the platform */

    if(fabs(cur_stat.hap)>0.1){ /* greater than 0.1 hours */
    
       fprintf(stderr,"%s: currently %4.1f hours from the meridian. Going to zenith before stowing\n",
       class_name,fabs(cur_stat.hap));

       // go to zenith 
 
       tel_cmd = new telescope_command(TELESCOPE_CMD_ZENITH);
       tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
       tel_cmd_response = sendcmd(tel_cmd);
       cmd_response=check_tel_response(tel_cmd_response);
       delete tel_cmd;
       delete tel_cmd_response;

      if (cmd_response != 0) {
           fprintf(stderr, "%s: unable to move telescope to zenith\n", class_name);
           success=0;
           zenith_success=0;
       }
 
       // otherwise wait for move to complete
       else if(wait_move()!=0){
          fprintf(stderr,"%s: error waiting for telescope to go to zenith\n",class_name);
          fprintf(stderr,"%s: skipping stow of telescope because could not get to zenith\n",class_name);
          success=0;
          zenith_success=0;
          return -1;
       }
    }
 
 
    // if successfully at zenith (or no need to go to zenith)  stow

    if(zenith_success){
#ifdef STOW20
        // stow at az=0, el=20 deg 
        tel_cmd = new telescope_command(TELESCOPE_CMD_STOW20);
#else
        // stow at default stow
        tel_cmd = new telescope_command(TELESCOPE_CMD_STOW);
#endif
        tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
        tel_cmd_response = sendcmd(tel_cmd);
        cmd_response=check_tel_response(tel_cmd_response);
        delete tel_cmd;
        delete tel_cmd_response;

        // check for failure to stow
       if (cmd_response != 0) {
            success=0;
            fprintf(stderr, "%s: unable to stow the telescope\n", class_name);
        }

        // otherwise wait for stow to complete
        else if(wait_move()!=0){
            success=0;
            fprintf(stderr,"%s: error waiting for telescope to stow\n",class_name);
            // return -1;
        }   
    }
 
    // next stow the dome

    tel_cmd = new telescope_command(TELESCOPE_CMD_STOW_DOME);
    tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
    tel_cmd_response = sendcmd(tel_cmd);
    cmd_response=check_tel_response(tel_cmd_response);
    delete tel_cmd;
    delete tel_cmd_response;

    // check for failure to stow the dome
    if (cmd_response != 0) {
        success=0;
        fprintf(stderr, "%s: unable to stow the dome\n", class_name);
    }

    // otherwise wait for stow to complete
    else {
       sleep(3); // give time for telescope to start moving */
       if(wait_move()!=0){
           success=0;
           fprintf(stderr,"%s: error waiting for dome to stow\n",class_name);
      //   return -1;
       }
    }   

    // next close the dome

    if(close_dome_shutter()!=0){
        success=0;
        fprintf(stderr, "%s: unable to close the dome\n", class_name);
    }

#if 0
   // don't do it. Telescope will droop
   // finally disable servos

    if(disable_servos()!=0){
        success=0;
        fprintf(stderr, "%s: unable to disable servos\n", class_name);
    }
#endif
    return success;
}

/**************************************************************************
 * METHOD: telescope_controller::zenith()
 *
 * DESCRIPTION:
 *   move telescope to zenith.
 *   1 on success
 *   0 on failure
 *   -1 on unknown
 **************************************************************************/
int telescope_controller::zenith() {
    int cmd_response = -1;
    telescope_command* tel_cmd ;
    char *tel_cmd_response;


    if (take_control() != 0) {
        fprintf(stderr, "%s: unable to take control of TCU\n", class_name);
        cmd_response = -1;
	return 0;
    }

    // now move the telescope to zenith

    tel_cmd = new telescope_command(TELESCOPE_CMD_ZENITH);
    tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
    tel_cmd_response = sendcmd(tel_cmd);
    cmd_response=check_tel_response(tel_cmd_response);

    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to move the telescope to zenith\n", class_name);
    }

    delete tel_cmd;
    delete tel_cmd_response;

    return cmd_response;
}

/**************************************************************************
 * METHOD: telescope_controller::stat_req(int passive_flag, struct tcu_status&)
 *
 * DESCRIPTION:
 *   Since we use the status string for all sorts of stuff this method gets
 *   the current status and jacks that into a structure that the calling
 *   program is free to use at there leisure.
 *   Returns:
 *   0 - on success
 *   -1 - on failure
 * 
 * set passive_flag to 1 to skip write to TCS serial port. This is needed
 * when questctl is running, and  you don't wont to have colliding processes
 * reading the TCS port. Use passive_flag=1 for status checks, assuming another
 * process (such as questctl) is running at the same time and making frequent
 * status checks
 *
 **************************************************************************/
int telescope_controller::stat_req(int passive_flag ,struct tcu_status& current_status) {
    char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
    int cmd_response = -1;
    mode_index index;
    telescope_command *tel_cmd;

    if( passive_flag == 1 ) {
        tel_cmd = new telescope_command(TELESCOPE_CMD_NOCMD);
        if(VERBOSE){
           cerr << "stat_req: sending TELESCOPE_CMD_NOCMD command " << endl;
           fflush(stderr);
        }
    }
    else{
        tel_cmd = new telescope_command(TELESCOPE_CMD_STS);
        if(VERBOSE){
           cerr << "stat_req: sending TELESCOPE_CMD_STS command " << endl;
           fflush(stderr);
        }
    }

    tel_cmd_response = sendcmd(tel_cmd);

    if(VERBOSE){
       cerr << "stat_req: checking telescope response " << endl;
       fflush(stderr);
    }
    cmd_response=check_tel_response(tel_cmd_response);
    if(VERBOSE){
       cerr << "stat_req: command response code is " << cmd_response << endl;
       fflush(stderr);
    }

    // when TCS is first connected for remote, it requires
    // a NOP to currectly respond. The error is 
    // TCS_UNKNOWN_ERROR. In this case, send a NOP
    if(cmd_response==TCS_UNKNOWN_ERROR){

       if(VERBOSE){
          cerr << "stat_req: TCS unknown error. Sending NOP " << endl;
          fflush(stderr);
       }

       delete tel_cmd;
       telescope_command* tel_cmd = new telescope_command    (TELESCOPE_CMD_NOP);
       tel_cmd_response = sendcmd(tel_cmd);

       cmd_response=check_tel_response(tel_cmd_response);
       if(VERBOSE){
          cerr << "stat_req: command response code is " << cmd_response << endl;
          fflush(stderr);
       }
    }


    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to get telescope status\n", class_name);
        
    }
    else{
        /* convert telemetry response to status */
        TCS_Telemetry *t;	
        t=(TCS_Telemetry *)tel_cmd_response;

        char string[2];
        string[0]=t->dome_state[0];
        string[1]=0;

        if(VERBOSE){
           fprintf(stderr,"dome status bit is %s\n",string);
        }
#ifdef TRICK_DOME_OPEN
        if(strncmp(t->dome_state,"0",1)==0){
            current_status.dome_shutter_state=ds_open;
            if(VERBOSE)fprintf(stderr,"setting dome shutter state to open\n");
        }
        else if(strncmp(t->dome_state,"1",1)==0){
            current_status.dome_shutter_state=ds_closed;
            if(VERBOSE)fprintf(stderr,"setting dome shutter state to closed\n");
        }
#else
        if(strncmp(t->dome_state,"1",1)==0){
            current_status.dome_shutter_state=ds_open;
            if(VERBOSE)fprintf(stderr,"setting dome shutter state to open\n");
        }
        else if(strncmp(t->dome_state,"0",1)==0){
            current_status.dome_shutter_state=ds_closed;
            if(VERBOSE)fprintf(stderr,"setting dome shutter state to closed\n");
        }
#endif
        else{
            current_status.dome_shutter_state=ds_stat_unknown;
            if(VERBOSE)fprintf(stderr,"setting dome shutter state to unknown\n");
        }

        /* ra HHMMSS.ss Use hms_nocolon conversion  */

        if(hms_to_h_nocolon(t->ra,&current_status.rap)!=0){
           fprintf(stderr, "%s: unable to get telescope ra\n", class_name);
           return(-1);
        }
        //current_status.rap=current_status.rap*15.0;

        /* hour angle is +HH:MM:SS. Use dms conversion which accounts for sign */
        if(dms_to_d(t->ha,&current_status.hap)!=0){
           fprintf(stderr, "%s: unable to get telescope ha\n", class_name);
           return(-1);
        }

        /* UT is HH:MM:SS.S Use hms conversion */
        if(hms_to_h(t->ut_time,&current_status.ut)!=0){
           fprintf(stderr, "%s: unable to get telescope ut \n", class_name);
           return(-1);
        }

        /* LST angle is HH:MM:SS. Use hms conversion */
        if(hms_to_h(t->lst,&current_status.lst)!=0){
           fprintf(stderr, "%s: unable to get telescope lst\n", class_name);
           return(-1);
        }

        /* dec is +DDMMSS.s. Use dms with no colon to convert */
        if(dms_to_d_nocolon(t->dec,&current_status.decp)!=0){
           fprintf(stderr, "%s: unable to get telescope dec\n", class_name);
           return(-1);
        }

	sscanf(t->jd,"%lf",&(current_status.jd));
	sscanf(t->epoch,"%lf",&(current_status.epoch));
	sscanf(t->alt,"%lf",&(current_status.el));
	sscanf(t->azim,"%lf",&(current_status.az));
	sscanf(t->dome_err_deg,"%lf",&(current_status.domep));
	sscanf(t->secz,"%lf",&(current_status.secz));


        sscanf(t->focus_pos,"%d",&current_status.focusp);
#ifdef FOCUS_FIX
        /* this corrects for bug in the TCS output stream. Only the the first 5 digits
           of the focus position are given. Normal focus is at -150,000 (RG610 filter).
           SO a value of -15,000 is really -150,000. But this assumes the focus is never
           set at a position betwen -99999 and +99999. For now, set the TCS to that the
           upper limit to the focus is -100,000.
        */
      
        current_status.focusp=current_status.focusp*10; 
#endif
        /* dome and and windscreen currently not part of telemetry string */
        current_status.domep = 0.0;
        current_status.windscreenp = 0.0;

        /* get motion, wobble, ra_limit, dec_limit, horiz_limit, and drive_status modes */

        index=motion_mode;
       	sscanf(t->motion_status,"%d",&(current_status.mode[index]));

        index=wobble_mode;
        if(strncmp(t->wobble_status," ",1)==0){
            current_status.mode[index]=0;
        }
        else{
	    cerr << "stat_req:  unknown wobble mode : " << t->wobble_status << endl;
       	    sscanf(t->wobble_status,"%d",&(current_status.mode[index]));
        }

#if 1

        index=ra_limit_mode;
        if(strncmp(t->ra_limit," ",1)==0){
            current_status.mode[index]=0;
        }
        else{
           if(strncmp(t->ra_limit,"R",1)==0){
		current_status.mode[index]=1;
	   }
           else{
	        cerr << "stat_req:  unknown ra limit mode : " << t->ra_limit  << endl;
		current_status.mode[index]=-1;
 	   }
        }

        index=dec_limit_mode;
        if(strncmp(t->dec_limit," ",1)==0){
            current_status.mode[index]=0;
        }
        else{
           if(strncmp(t->dec_limit,"D",1)==0){
		current_status.mode[index]=1;
	   }
           else{
	        cerr << "stat_req:  unknown dec limit mode : " << t->dec_limit  << endl;
		current_status.mode[index]=-1;
 	   }
        }

        index=horiz_limit_mode;
        if(strncmp(t->horiz_limit," ",1)==0){
            current_status.mode[index]=0;
        }
        else{
           if(strncmp(t->horiz_limit,"H",1)==0){
		current_status.mode[index]=1;
	   }
           else{
	        cerr << "stat_req:  unknown horizon limit mode : " << t->horiz_limit << endl;
		current_status.mode[index]=-1;
 	   }
        }

        index=drive_status_mode;
        if(strncmp(t->drive_status," ",1)==0){
            current_status.mode[index]=0;
        }
        else{
           if(strncmp(t->drive_status,"D",1)==0){
		current_status.mode[index]=1;
	   }
           else{
	        cerr << "stat_req:  unknown drive status mode : " << t->drive_status << endl;
		current_status.mode[index]=-1;
 	   }
        }
#endif

    }
    delete tel_cmd;
    delete tel_cmd_response;
    return cmd_response;
}

/**************************************************************************
 * METHOD: telescope_controller::tracking_status()
 *
 * DESCRIPTION:
 *   Returns a  representation of the status of the TCU response
 *   from the STAT command
 *   TELESCOPE_DOME_MOVING
 *   TELESCOPE_PARKED  3 
 *   TELESCOPE_DISABLED 4
 *   TELESCOPE_HORIZON_LIMIT
 *   TELESCOPE_RA_LIMIT
 *   TELESCOPE_DEC_LIMIT
 *   TELESCOPE_TRACKING_OFF_TARGET 2
 *   TELESCOPE_TRACKING_ON_TARGET 1
 *   TELESCOPE_SLEWING 0
 *   TELESCOPE_STATUS_UKNOWN -1
 *   3 - parked
 *   2 - in star track mode off target
 *   1 - in star track mode on target
 *   0 - not in star track mode
 *   -1 - unkown status
 *
 *   Telescope tracking status is either DOME_MOVING,
 *   one of ( TELESCOPE_PARKED,TELESCOPE_DISABLED,TELESCOPE_HORIZON_LIMIT,
 *        TELESCOPE_RA_LIMIT, or TELESCOPE_DEC_LIMIT),
 *   TELESCOPE_SLEWING, TELESCOPE_TRACKING_ON_TARGET, or TELESCOPE_TRACKING_OFF_TARGET
 * 
 **************************************************************************/
int telescope_controller::tracking_status() {
    /* There is no way to know if we are tracking except to see that RA and Dec are constant
       and that the motion_status_mode is stable. So make two calls to get status, separated
       by short delay (3 sec) and see if ra or dec has changed in the mean time */
    double dra, ddec, dha;
    struct tcu_status cur_stat_prev, cur_stat;
    mode_index index;
    motion_status_value value;

    if(VERBOSE){
       cerr << "status: issuing first stat request " << endl;
       fflush(stderr);
    }

    if (stat_req(0,cur_stat_prev) == -1) {
	fprintf(stderr, "%s: first status request failed\n", class_name);
	return -1;
    }

    /* check if dome is moving */

    index=motion_mode;
    value=(motion_status_value)((1<<dome_bit)&cur_stat_prev.mode[index]);

    if(VERBOSE){
       cerr << "tracking_status: dome motion bit  is " << dome_bit << endl;
       cerr << "tracking_status: dome motion status is " << value << endl;
       fflush(stderr);
    }

    if(value!=DOME_STOPPED){ //  dome still moving
       if(VERBOSE){
          cerr << "tracking_status: dome is still moving:  " << value << endl;
          fflush(stderr);
       }
       return TELESCOPE_DOME_MOVING;
    }

    

    /* check if telescope is still slewing */
#if 0
    index=motion_mode;
    value=(motion_status_value)cur_stat_prev.mode[index];

    if(VERBOSE){
       cerr << "tracking_status: motion status is " << value << endl;
       fflush(stderr);
    }

    if(value!=stable_motion){ // still slewing or dome still moving
       if(VERBOSE){
          cerr << "tracking_status: motion not stable " << value << endl;
          if(value&(1<<dec_bit)){cerr << "dec still in motion " << endl;}
          if(value&(1<<ra_bit)){cerr << "ra still in motion " << endl;}
          if(value&(1<<dome_bit)){cerr << "dome  still in motion " << endl;}
          fflush(stderr);
       }
       return TELESCOPE_SLEWING;
    }
#endif

    if(VERBOSE){
       fprintf(stderr,"tracking_status: waiting %f seconds\n",WAIT_TIME);
       fflush(stderr);
    }
    usleep((useconds_t)(WAIT_TIME*1000000.0));

    if(VERBOSE){
       cerr << "tracking_status: issuing second stat request " << endl;
       fflush(stderr);
    }

    if (stat_req(0,cur_stat) == -1) {
	fprintf(stderr, "%s: second status request failed\n", class_name);
	return -1;
    }

    /* calculate ha, ra and dec drift in previous WAIT_TIME seconds */
    dha=cur_stat.hap-cur_stat_prev.hap;
    dra=cur_stat.rap-cur_stat_prev.rap;
    /* add the following 2010 08 01 to handle case when ra set to 0 */
    if(dha<-12.0)dha=dha+24.0;
    if(dha>12.0)dha=dha-24.0;
    if(dra<-12.0)dra=dra+24.0;
    if(dra>12.0)dra=dra-24.0;
    ddec=cur_stat.decp-cur_stat_prev.decp;
    dha=15.0*dha*3600.0*cos(cur_stat.decp*RPD);
    dra=15.0*dra*3600.0*cos(cur_stat.decp*RPD);
    ddec=ddec*3600.0;

    /* if dha and ddec are small, telescope is parked (tracking off), disabled, or in a limit.
       Check which one and return the state */

    if(fabs(dha)<MAX_RA_DRIFT&&fabs(ddec)<MAX_DEC_DRIFT){
       // telescope not moving. See if the servos are disbaled
       if(cur_stat.mode[drive_status_mode]!=0){
           if(VERBOSE){
              fprintf(stderr,"tracking_status: telescope in drives disabled\n");
              fflush(stderr);
           }
	   return TELESCOPE_DISABLED;
       }
       else if (cur_stat.mode[horiz_limit_mode]!=0){
           if(VERBOSE){
              fprintf(stderr,"tracking_status: telescope in horizon limit\n");
              fflush(stderr);
           }
           return TELESCOPE_HORIZON_LIMIT;
       }
       else if (cur_stat.mode[ra_limit_mode]!=0){
           if(VERBOSE){
              fprintf(stderr,"tracking_status: telescope in ra limit\n");
              fflush(stderr);
           }
           return TELESCOPE_RA_LIMIT;
       }
       else if (cur_stat.mode[dec_limit_mode]!=0){
           if(VERBOSE){
              fprintf(stderr,"tracking_status: telescope in dec limit\n");
              fflush(stderr);
           }
           return TELESCOPE_DEC_LIMIT;
       }
       else{
           if(VERBOSE){
              fprintf(stderr,"tracking_status: telescope is parked: %7.3f %7.3f\n",dra,ddec);
              fflush(stderr);
           }
           return TELESCOPE_PARKED;
       }
    }

    /* otherwise if ra or dec drift is too large, telescope is slewing */
    if(fabs(dra)>MAX_RA_DRIFT||fabs(ddec)>MAX_DEC_DRIFT){
       if(VERBOSE){
          fprintf(stderr,"tracking_status: telescope still settling: %7.3f %7.3f\n",dra,ddec);
          fflush(stderr);
       }
       return TELESCOPE_SLEWING;
    }

    /* check if telescope position is not on target */

    dra=cur_stat.rap-target_position.ra;
    ddec=cur_stat.decp-target_position.dec;
    /* add the following 2010 10 01 to handle case when ra set to 0 */
    if(dra<-12.0)dra=dra+24.0;
    if(dra>12.0)dra=dra-24.0;
    dra=15.0*dra*3600.0*cos(cur_stat.decp*RPD);
    ddec=ddec*3600.0;
    if(fabs(dra)>POINTING_TOLERANCE||fabs(ddec)>POINTING_TOLERANCE){
       if(VERBOSE){
          fprintf(stderr,"tracking_status: telescope not on target: %7.3f %7.3f\n",dra,ddec);
          fflush(stderr);
       }
       return TELESCOPE_TRACKING_OFF_TARGET;
    }

#if 0 /* dome position check not yet implemeted */

    /* check not needed, motion status includes dome motion */
    /* else if dome has not caught up */

    else if ("dome not caught up yet"){
       return 0;
    }
#endif

    /* otherwise we are on target, tracking, and dome is assumed to be caught up too. */

    else {
       if(VERBOSE){
           fprintf(stderr,"tracking_status: telescope on target: %7.3f %7.3f\n",dra,ddec);
          fflush(stderr);
       }
       return TELESCOPE_TRACKING_ON_TARGET;
    }

    return -1;
}

/**************************************************************************
 * METHOD: telescope_controller::telescope_status()
 *
 * DESCRIPTION:
 *   Returns a complete status report for the tescope using all the
 *   bits of information in the TCU response
 * 
 **************************************************************************/
int telescope_controller::telescope_status() {
    /* There is no way to know if we are tracking except to see that RA and Dec are constant
       and that the motion_status_mode is stable. So make two calls to get status, separated
       by short delay (3 sec) and see if ra or dec has changed in the mean time */
    struct tcu_status cur_stat;

#if 0
    switch(tracking_status()){
    case TELESCOPE_SLEWING:
    case  TELESCOPE_DOME_MOVING:
     	fprintf(stdout, "Tracking Status: %s  ", "MOVING");
	break;
    case TELESCOPE_PARKED:
     	fprintf(stdout, "Tracking Status: %s  ", "PARKED");
	break;
    case TELESCOPE_DISABLED: 
     	fprintf(stdout, "Tracking Status: %s  ", "DISABLED");
	break;
    case TELESCOPE_HORIZON_LIMIT:
    case TELESCOPE_RA_LIMIT:
    case TELESCOPE_DEC_LIMIT:
     	fprintf(stdout, "Tracking Status: %s  ", "LIMIT");
	break;
    case TELESCOPE_TRACKING_OFF_TARGET:
     	fprintf(stdout, "Tracking Status: %s  ", "OFF_TARGET");
	break;
    case TELESCOPE_TRACKING_ON_TARGET:
     	fprintf(stdout, "Tracking Status: %s  ", "ON_TARGET");
	break;
    case TELESCOPE_STATUS_UKNOWN:  
    default:
     	fprintf(stdout, "Tracking Status: %s  ", "UNKNOWN");
	break;
    }
#endif

    if (stat_req(1,cur_stat) == -1) {
	fprintf(stderr, "%s: status request failed\n", class_name);
	return -1;
    }


    switch(cur_stat.dome_shutter_state){
    case ds_open:
     	fprintf(stdout, "Dome Status: %s  ", "OPEN");
	break;
    case ds_closed:
     	fprintf(stdout, "Dome Status: %s  ", "CLOSED");
	break;
    case ds_stat_unknown:
     	fprintf(stdout, "Dome Status: %s  ", "UNKNOWN");
	break;
    default:
     	fprintf(stdout, "Dome Status: %s  ", "UNKNOWN");
    }

    fprintf(stdout,"Telescope RA: %10.6f  ",cur_stat.rap);
    fprintf(stdout,"Telescope Dec: %10.6f  ",cur_stat.decp);
    fprintf(stdout,"Telescope HA: %10.6f  ",cur_stat.hap);
    fprintf(stdout,"Telescope LST: %10.6f  ",cur_stat.lst);
    fprintf(stdout,"Telescope JD: %10.1f  ",cur_stat.jd);
    fprintf(stdout,"Telescope UT: %10.6f  ",cur_stat.ut);
    fprintf(stdout,"Telescope Epoch: %6.1f  ",cur_stat.epoch);
    fprintf(stdout,"Telescope Azimuth: %6.1f  ",cur_stat.az);
    fprintf(stdout,"Telescope Elevation: %6.1f  ",cur_stat.el);
    fprintf(stdout,"Telescope Secz: %6.1f  ",cur_stat.secz);

    double f_mm = convert_focus_to_mm(cur_stat.focusp);
    fprintf(stdout,"Focus: %6.3f  ",f_mm);
    fprintf(stdout,"Dome Position: %5.1f  ",cur_stat.domep);

    switch(cur_stat.mode[wobble_mode]){
    case 0:  
        fprintf(stdout,"Wobble Status: %s  ","OFF_BEAM");
        break;
    case 1:  
        fprintf(stdout,"Wobble Status: %s  ","ON_BEAM1");
        break;
    case 2:  
        fprintf(stdout,"Wobble Status: %s  ","ON_BEAM2");
        break;
    default:
        fprintf(stdout,"Wobble Status: %s  ","UNKNOWN");
    }

    switch(cur_stat.mode[horiz_limit_mode]){
    case 0:  
        fprintf(stdout,"Horizon Limit: %s  ","OUT_OF_LIMIT");
        break;
    case 1:  
        fprintf(stdout,"Horizon Limit: %s  ","IN_LIMIT");
        break;
    default:
        fprintf(stdout,"Horizon Limit: %s  ","UNKNOWN");
    }

    switch(cur_stat.mode[ra_limit_mode]){
    case 0:  
        fprintf(stdout,"RA Limit: %s  ","OUT_OF_LIMIT");
        break;
    case 1:  
        fprintf(stdout,"RA Limit: %s  ","IN_LIMIT");
        break;
    default:
        fprintf(stdout,"RA Limit: %s  ","UNKNOWN");
    }

    switch(cur_stat.mode[dec_limit_mode]){
    case 0:  
        fprintf(stdout,"Dec Limit: %s  ","OUT_OF_LIMIT");
        break;
    case 1:  
        fprintf(stdout,"Dec Limit: %s  ","IN_LIMIT");
        break;
    default:
        fprintf(stdout,"Dec Limit: %s  ","UNKNOWN");
    }

    switch(cur_stat.mode[drive_status_mode]){
    case 0:  
        fprintf(stdout,"Servos: %s  ","ENABLED");
        break;
    case 1:  
        fprintf(stdout,"Servos: %s  ","DISABLED");
        break;
    default:
        fprintf(stdout,"Servos: %s  ","UNKNOWN");
    }

    if((motion_status_value)((1<<dome_bit)&cur_stat.mode[motion_mode])){
       fprintf(stdout,"Dome Motion: %s  ","MOVING");
    }
    else{
       fprintf(stdout,"Dome Motion: %s  ","FIXED");
    }

    if((motion_status_value)((1<<ra_bit)&cur_stat.mode[motion_mode])){
       fprintf(stdout,"RA Motion: %s  ","MOVING");
    }
    else{
       fprintf(stdout,"RA Motion: %s  ","FIXED");
    }

    if((motion_status_value)((1<<dec_bit)&cur_stat.mode[motion_mode])){
       fprintf(stdout,"Dec Motion: %s  ","MOVING");
    }
    else{
       fprintf(stdout,"Dec Motion: %s  ","FIXED");
    }

    fprintf(stdout,"\n");
    fflush(stdout);

    return(0);

}
/******************************************************************************/
// wait_move - wait for TCU to report "on target"
// To accomplish this the telescope is polled for status
// and that status is checked to see if it says it is on target.
// Return -2 on failure to get on any fault
// Return -1 on timeout
// Return 0 if on target

int
telescope_controller::wait_move() {
    double t1,t2;

    if (VERBOSE) {
        cerr << "telescope_controller::wait_move" << endl;
    }

    char my_name[1024] = "telescope_controller::wait_move";

    int my_status = TELESCOPE_SLEWING;
    t1 = neat_gettime_utc(ut_offset);
 
    while (my_status == TELESCOPE_SLEWING || my_status == TELESCOPE_DOME_MOVING) {

        my_status = tracking_status();
        if (my_status == -1) {
            fprintf(stderr, "%s: unable to communicate with TCU\n", my_name);
            return -2;
        }

        // check to see if point timeout has been reached and we have not
        // yet reach our point
        t2=neat_gettime_utc(ut_offset);
        if (t2-t1 > point_timeout) {
            fprintf(stderr, "%s: telescope not reaching position\n", my_name);
            return -1;
        }
	sleep(1);
    }


    fprintf(stderr, "%s: waited %f seconds\n", my_name, t2-t1);

    return 0;
}

/**************************************************************************
 * METHOD: telescope_controller::get_position()
 *
 * DESCRIPTION:
 *   Returns the current position of the telescope.
 **************************************************************************/
void telescope_controller::get_position(struct point& position, double *epoch, double *lst, double *ha) {

    if(VERBOSE){
       cerr << "get_position: getting position from telescope status" << endl;
       fflush(stderr);
    }

    struct tcu_status cur_stat;
    if (stat_req(0,cur_stat) == -1) {
	// darn we bombed
	fprintf(stderr, "%s: unable to retrieve position from TCU\n",
	       class_name);
	position.ra = -999.9;
	position.dec = -999.9;
	*ha=0.0;
        *epoch=0.0;
        *lst=0.0;
	return ;
    }
    position.ra = cur_stat.rap;
    position.dec = cur_stat.decp;
    *epoch=cur_stat.epoch;
    *lst=cur_stat.lst;
    *ha=cur_stat.hap;

    if(VERBOSE){
       cerr << "get_position: ra: " << position.ra << endl;
       cerr << "get_position: dec: " << position.dec << endl;
       cerr << "get_position: ha: " << *ha << endl;
       cerr << "get_position: lst: " << *lst << endl;
       cerr << "get_position: epoch: " << *epoch << endl;
       cerr << "get_position: lat: " << pos_lat << endl;
       fflush(stderr);
    }
 
    // calc az/el so we can use it later in status function
    radec_ang rapoint(position.ra, position.dec);
    hadec_ang hapoint(rapoint, cur_stat.lst);
    azel_ang  aepoint(pos_lat, hapoint);
    cur_azel.az = aepoint.az;
    cur_azel.el = aepoint.el;

    current_position.ra = position.ra;
    current_position.dec = position.dec;
    fprintf(stderr,
	   "%s: set cur pos to ra: %lf dec: %lf\n", class_name,
	   current_position.ra, current_position.dec);
    fprintf(stderr,
	   "%s: set cur az/el pos to az: %lf el: %lf\n", class_name,
	   cur_azel.az, cur_azel.el);


    return;
}



/**************************************************************************
 * METHOD: telescope_controller::get_filter()
 *
 *
 * DESCRIPTION:
 *   Returns the current filter ID. 
 * 
 *  Filter id is byte ACU_STATUS_FILTER_ID_BYTE  = 36 of the acu status word, as set up by John Henning
 *  2004 June 2
 *
 **************************************************************************/
int telescope_controller::get_filter() {

    /* not yet implemented */

    int filter_id=0;
    
    return filter_id;
}

/**************************************************************************
 * METHOD: telescope_controller::convert_focus_to_steps()
 *
 * DESCRIPTION:
 *  Converts focus value from mm to steps, as required by Telescope Controller.
 *  Returns steps as an integer
 **************************************************************************/

int telescope_controller::convert_focus_to_steps(double f_mm){
   double f;
   int f_i;
   f=(f_mm-FOCUS_MM0)*FOCUS_MM_TO_STEPS + FOCUS_STEPS0;
   f_i=(int)(f+0.5);
   if(VERBOSE){
     fprintf(stderr,"telescope_controller: convert_focus_to_steps: f_mm = %12.6f  f_i= %d\n",
	f_mm,f_i);
     fflush(stdout);
   }
   return(f_i);
}

/**************************************************************************
 * METHOD: telescope_controller::convert_focus_to_mm()
 *
 * DESCRIPTION:
 *  Converts focus value from steps to mm.
 *  Returns focus as a double
 **************************************************************************/

double telescope_controller::convert_focus_to_mm(int f_steps){
   double f;
   f=(f_steps-FOCUS_STEPS0)/FOCUS_MM_TO_STEPS + FOCUS_MM0;
   return(f);
}

/**************************************************************************
 * METHOD: telescope_controller::get_focus()
 *
 * DESCRIPTION:
 *   Returns the current focus value. If value is out of range (currently
 *   -?? to ?? then it is bogus and is considered an error message)
 **************************************************************************/
double telescope_controller::get_focus() {

    struct tcu_status cur_stat;
    double f_mm;

    if (stat_req(0,cur_stat) == -1) {
	// darn we bombed
	fprintf(stderr, "%s: unable to retrieve status from TCU\n",
	       class_name);
        f_mm = convert_focus_to_mm(MIN_FCS)-1000.0; 
    }
    else{
       f_mm = convert_focus_to_mm(cur_stat.focusp);
       if(VERBOSE){
          fprintf(stderr,"%s: current focus in steps (mm) is %d (%12.6f)\n",
		class_name,cur_stat.focusp,f_mm);
       }
    }
    return f_mm;

}

/**************************************************************************
 * METHOD: telescope_controller::set_focus(const double)
 *
 * DESCRIPTION:
 *   sets the focus value (should already be in encoder steps)
 * Returns 0 on sucess
 * Returns -1 on failure
 * Returns -2 on timeout
 **************************************************************************/
int telescope_controller::set_focus(const double new_focus_position) {

    int cmd_response;
    char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
    telescope_command *tel_cmd;
    double min_fcs_mm, max_fcs_mm;

    min_fcs_mm=convert_focus_to_mm(MIN_FCS);
    max_fcs_mm=convert_focus_to_mm(MAX_FCS);
    if(VERBOSE){
        fprintf(stderr,"%s: min/max focus values  are %d %d steps\n",class_name,MIN_FCS,MAX_FCS);
        fprintf(stderr,"%s: min/max focus values  are %12.6f %12.6f mm\n",class_name,min_fcs_mm, max_fcs_mm);
    }

    // always try to take control (because you may not have it)
    cmd_response = take_control();
    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to take control\n", class_name);
	return cmd_response;
    }

    // first get current focus position
    double current_focus=get_focus();
    /*if(current_focus<MIN_FCS){*/
    if(current_focus<min_fcs_mm){
        fprintf(stderr,"%s: could not get current focus\n",class_name);
        return -1;
    }
    if(VERBOSE){
        fprintf(stderr,"%s: current focus is %12.6f mm\n",class_name,current_focus);
    }

    // set focus position
    /// convert new focus position to integer
    int focus_steps=convert_focus_to_steps(new_focus_position);

    if(VERBOSE){
        fprintf(stderr,"%s: new focus steps will be %d\n",class_name,focus_steps);
        double f_mm;
        f_mm=convert_focus_to_mm(focus_steps);
        fprintf(stderr,"%s: which corresponds to %12.6f mm\n",class_name,f_mm);
    }

    tel_cmd = new telescope_command(TELESCOPE_CMD_FOCUS);
    tel_cmd->setup_focus(focus_steps);

    // send command
    tel_cmd_response = sendcmd(tel_cmd);
    if(cmd_response=check_tel_response(tel_cmd_response)!=0){
        fprintf(stderr, "%s: unable to set focus at %d\n",class_name,focus_steps);
    }

    delete tel_cmd;
    delete tel_cmd_response;

    // wait for focus to go to new setting
    int  i = 0;
    double foc_pos_mm = get_focus();
    int foc_pos=convert_focus_to_steps(foc_pos_mm);
#ifdef FOCUS_FIX
    while (foc_pos-focus_steps>FOCUS_TOLERANCE_STEPS){
#else
    while (foc_pos-focus_steps>10*FOCUS_TOLERANCE_STEPS){
#endif
	fprintf(stderr,"telescope_controller::set_focus: focus at %d. Waiting to  reach %d\n",foc_pos,focus_steps);
	sleep(10); // long 10 sec wait because the shutter takes a while
	i += 10;
	if (i >= focus_timeout) {
	    fprintf(stderr, "%s: timed out waiting for focus to go to %lf\n",
		class_name,new_focus_position);
	    return -2;
	}
        foc_pos_mm=get_focus();
        foc_pos=convert_focus_to_steps(foc_pos_mm);
    }
    // if we have reached here then new focus position is set

    return 0;
}

/**************************************************************************
 * METHOD: telescope_controller::stop_focus()
 *
 * DESCRIPTION:
 *   Causes focus encoder/motor to stop in it's tracks. Important to do
 *   if you want to immediately stop everything
 **************************************************************************/
int telescope_controller::stop_focus() {
     /* not implemented */
    fprintf(stderr,"stop_focus: not implemented\n");
    return 1;
}

/**************************************************************************
 * METHOD: telescope_controller::fault_status()
 *
 * check for various faults (ra, dec, or horizon limits, and drive disabled )
 *
 **************************************************************************/
int telescope_controller::fault_status() {

    struct tcu_status cur_stat;
    mode_index index;
    limit_status_value ra_status;
    limit_status_value dec_status;
    limit_status_value horiz_status;
    drive_status_value drive_status;
    telescope_fault_code fault_code;

    if(VERBOSE){
        cerr << "fault_status: getting telescope status " << endl;
        fflush(stderr);
    }
       
    if (stat_req(0,cur_stat) == -1) {
        fprintf(stderr, "%s:  status request failed\n", class_name);
        fault_code = comm_fault;
        return fault_code;
    }

    if(VERBOSE){
        cerr << "fault_status: checking ra limit " << endl;
        fflush(stderr);
    }

    index=ra_limit_mode;
    ra_status=(limit_status_value)cur_stat.mode[index];
    if(ra_status==in_limit) {
      cerr << "fault_status: inra limit " << endl;
      fault_code=ra_limit_fault;
      return fault_code;
    }

    index=dec_limit_mode;
    dec_status=(limit_status_value)cur_stat.mode[index];
    if(dec_status==in_limit) {
      cerr << "fault_status: in dec limit " << endl;
      fault_code=dec_limit_fault;
      return fault_code;
    }

    index=horiz_limit_mode;
    horiz_status=(limit_status_value)cur_stat.mode[index];
    if(horiz_status==in_limit) {
      cerr << "fault_status: in horiz limit " << endl;
      fault_code=horiz_limit_fault;
      return fault_code;
    }

    index=drive_status_mode;
    drive_status=(drive_status_value)cur_stat.mode[index];
    if(drive_status==disabled) {
      cerr << "fault_status: drive disabled " << endl;
      fault_code=drive_fault;
      return fault_code;
    }

    fault_code=no_fault;
    return fault_code;
}

/**************************************************************************
 * METHOD: telescope_controller::dome_shutter_status()
 *
 * DESCRIPTION:
 *   This returns only the status of the dome shutter. It is meant to
 *   reduce the load on the dome_status() command as well as take care
 *   of a design error with that method. Specifically that shutter status
 *   is not the same as dome status and as such makes it hard to report
 *   status accurately.
 *   return values:
 *   1 - dome shutter open
 *   0 - dome shutter closed
 *   -1 - dome shutter status unknown (neither open or close flags are set)
 *   -8 - unknown failure (probably a comm fault)
 **************************************************************************/
int telescope_controller::dome_shutter_status() {

    struct tcu_status cur_stat;
    if (stat_req(0,cur_stat) == -1) {
	fprintf(stderr, "%s: unable to retrieve status from TCU\n",
	       class_name);
	return d_comm_failed ;
    }
    else{
        return cur_stat.dome_shutter_state;
    }
}

/**************************************************************************
 * METHOD: telescope_controller::dome_status(struct dome_status)
 *
 * DESCRIPTION:
 *   The TCU returns dome status in the form of the return from the STAT
 *   request. This command gets that status and returns a value associated
 *   with that status.
 *   return values:
 *   0 - operations worked
 *   -1 - operation failed
 **************************************************************************/
int telescope_controller::dome_status(struct d_status& dome_stat) {
    /* not implemented */
    fprintf(stderr,"dome_status: not implemented\n");

    // put dome mode information into struct
    dome_stat.dome_mode = 0;
    dome_stat.dome_submode = 0;
    dome_stat.domep = 0;
    dome_stat.dome_mismatch_flag = 0;
    
    return 0;
}

/**************************************************************************
 * METHOD: telescope_controller::windscr_status(struct windscr_status)
 *
 * DESCRIPTION:
 *   The TCU returns windscr status in the form of the return from the STAT
 *   request. This command gets that status and returns a value associated
 *   with that status.
 *   return values:
 *   0 - operations worked
 *   -1 - operation failed
 **************************************************************************/
int telescope_controller::windscr_status(struct ws_status& windscr_stat) {
    /* not implemented */
    fprintf(stderr,"windscr_status: not implemented\n");
    windscr_stat.windscr_mode = 0;
    windscr_stat.windscr_submode = 0;
    windscr_stat.windscrp = 0;

    return 0;
}

/**************************************************************************
 * METHOD: telescope_controller::close_dome_shutter()
 *
 * DESCRIPTION:
 *   Provides command to close the dome shutter. This is a blocking command,
 *   it does not return until the dome shutter is closed (or times out).
 *   Returns:
 *   0 - success
 *   -1 - failure
 *   -2 - timed out waiting for shutter
 *   -3 - comm failed
 *   Use dome_shutter_status to figure out where the shutter is at if this
 *   command fails
 **************************************************************************/
int telescope_controller::close_dome_shutter() {

    // first see if the dome is already closed
    int domes_stat;
    if ((domes_stat = dome_shutter_status()) == ds_comm_failed) {
	fprintf(stderr, "%s: dome status failed\n", class_name);
	return -3;
    }

    // ok, figure out dome status
    switch (domes_stat) {
	case ds_open:
	    break;
	case ds_closed:
	    fprintf(stderr, "%s: dome shutter already closed\n",
		   class_name);
	    return 0;
	    break;
	case ds_stat_unknown:
	    fprintf(stderr, "%s: dome shutter state unknown\n", class_name);
	    break;
	default:
	    fprintf(stderr, "%s: dome shutter status bizarre, %d\n",
		   class_name, domes_stat);
	    return -3;
	    break;
    }
    // ok, got this far close the shutter
    if (take_control() != 0) {
	fprintf(stderr, "%s: unable to take control of tcu\n", class_name);
	return -1;
    }

    /* construct and send dome close command. Check for error */
    telescope_command* ods_cmd =
	new telescope_command(TELESCOPE_CMD_DOME_CLOSE);
    char* tcu_response = new char[TEL_RESPONSE_SIZE];
    tcu_response = sendcmd(ods_cmd);
    delete ods_cmd;
    int cmd_response;
    if(cmd_response=check_tel_response(tcu_response)!=0){
	fprintf(stderr, "telescope_controller: close_dome_shutter: failed sending dome shutter close command\n");
	delete tcu_response;
	return -1;
    }
        
    // now wait until open limit switch is tripped
    int cur_ds_stat, i = 0;
    while ((cur_ds_stat = dome_shutter_status()) != ds_closed) {
	if (cur_ds_stat == ds_comm_failed) {
	    fprintf(stderr, "%s: unable to get dome shutter status while waiting for closure\n",
		   class_name);
	    return -3;
	}
	sleep(10); // long 10 sec wait because the shutter takes a while
	i += 10;
	if (i >= dome_shutter_timeout) {
	    fprintf(stderr, "%s: timed out waiting for dome shutter to close\n",
		   class_name);
	    return -2;
	}
    }
    // if we have reached here then shutter is closed
    fprintf(stderr, "%s: done closing dome shutter\n", class_name);
    return 0;
}

/**************************************************************************
 * METHOD: telescope_controller::open_dome_shutter()
 *
 * DESCRIPTION:
 *   Provides command to open shutter. This is a blocking command, in that
 *   it does not return until the dome shutter is open.
 *   Returns:
 *   0 - success
 *   -1 - on failure
 *   -2 - timed out waiting for shutter
 *   -3 - comm failed
 *   The implementor is expected to get their own status if dome opening
 *   fails.
 **************************************************************************/
int telescope_controller::open_dome_shutter() {

    // first see if the dome is open
    int domes_stat;
    if ((domes_stat = dome_shutter_status()) == d_comm_failed) {
	fprintf(stderr, "%s: dome status failed\n", class_name);
	return -3;
    }

    // ok, figure out dome status
    switch (domes_stat) {
	case ds_open:
	    fprintf(stderr, "%s: dome shutter already opened\n",
		   class_name);
	    return 0;
	    break;
	case ds_closed:
	    break;
	case ds_stat_unknown:
	    fprintf(stderr, "%s: dome shutter state unknown\n", class_name);
	    break;
	default:
	    fprintf(stderr, "%s: dome shutter status bizarre\n, %d",
		   class_name, domes_stat);
	    return -3;
	    break;
    }
    // ok, got this far open the shutter
    if (take_control() != 0) {
	fprintf(stderr, "%s: unable to take control of tcu\n", class_name);
	return -1;
    }

    telescope_command* ods_cmd =
	new telescope_command(TELESCOPE_CMD_DOME_OPEN);
    char* tcu_response = new char[255];
    if (ods_cmd == NULL || tcu_response == NULL) {
	fprintf(stderr, "%s: failed initializing objects\n", class_name);
	return -3;
    }
    tcu_response = sendcmd(ods_cmd);
    delete ods_cmd;
    int cmd_response;
    if(cmd_response=check_tel_response(tcu_response)!=0){
	fprintf(stderr, "telescope_controller:dome_shutter_open: failed sending dome shutter open command\n");
	delete tcu_response;
	return -1;
    }


    // now wait until open limit switch is tripped
    int cur_ds_stat, i = 0;
    while ((cur_ds_stat = dome_shutter_status()) != ds_open) {
	if (cur_ds_stat == ds_comm_failed) {
	    fprintf(stderr, "%s: unable to get dome shutter status\n",
		   class_name);
	    return -3;
	}
	sleep(10); // long 10 sec wait because the shutter takes a while
	i += 10;
	if (i >= dome_shutter_timeout) {
	    fprintf(stderr, "%s: timed out waiting for shutter to open\n",
		   class_name);
	    return -2;
	}
    }
    // if we have reached here then shutter is open
    fprintf(stderr, "%s: done opening dome shutter\n", class_name);
    return 0;

}

/**************************************************************************
 * METHOD: telescope_controller::dummy_load_weather(struct weather_data)
 *
 * DESCRIPTION:
 *   Loads a weather_data structure with bogus values, very useful if you
 *   want to return bogus values as a way of reporting error in
 *   processing
 **************************************************************************/
void telescope_controller::dummy_load_weather(struct weather_data* sucker) {
    sucker->temp = -999.9;
    sucker->humidity = -999.9;
    sucker->wind_speed = -999.9;
    sucker->wind_direction = -999.9;
    sucker->dew_point = -999.9;
    sucker->dome_states = -1;  /* -1 means unknown */
}


/**************************************************************************
 * METHOD: telescope_controller::get_weather_data()
 *
 * DESCRIPTION:
 *   Extracts weather data from weather status message and returns the
 *   data to the caller in the struct weather_data. The caller is
 *   depended on to know anything about the data.
 **************************************************************************/
struct weather_data telescope_controller::get_weather_data() {

    struct weather_data current_data;
    FILE *input;
    char string[1024],s[256];

    // stuff it with known bad values
    dummy_load_weather(&current_data);
    current_data.temp=0.0;
// DEBUG
// skip weather check
#if 0
    // run system command to get La Silla dome status and write result to temporary file
    sprintf (string,"echo ERROR >& %s",weather_file);
    fprintf(stderr,"get_weather_data: executing [%s]\n",string);
    system (string);

    // create system string for getting weather info. This is a command
    // to echo an "s" character to the weather_client perl script and pipe
    // the output to a temporary file. The temporary files has the
    // weather info (currently only the dome status of the NTT and 2.2 m
    // telescopes at La Silla)

    sprintf(string,"echo s | %s >& %s",weather_client,weather_file);
    fprintf(stderr,"get_weather_data: executing [%s]\n",string);
    system (string);

    // read the result from the temporary file
    fprintf(stderr,"get_weather_data: opening [%s]\n",weather_file);
    input=fopen(weather_file,"r");
    if(input==NULL){
       fprintf(stderr,"%s: could not open file %s\n",class_name,weather_file);
       current_data.humidity = -99.0;   
    }
    else if(fgets(string,1024,input)==NULL){
       fprintf(stderr,"%s: could not read file %s\n",class_name,weather_file);
       current_data.humidity = -99.0;
    }
    else{
      fprintf(stderr,"get_weather_data: weathe file line is [%s]\n",string);
       sscanf(string,"%s %lf %lf %lf %lf %lf %lf",
        s,
	&(current_data.temp),
	&(current_data.humidity),
	&(current_data.wind_speed),
	&(current_data.wind_direction),
	&(current_data.dew_point), 
	&(current_data.pressure));


       if(strstr(string,"OPEN")!=NULL){
           current_data.dome_states = 1;
       }
       else if (strstr(string,"CLOSED")!=NULL){
           current_data.dome_states = 0;
       }
       else{
           fprintf(stderr,"%s:  La Silla dome status is unknown\n",class_name);
           fprintf(stderr,"%s:  status is %s\n",class_name,string);
       }
    }
    if(input!=NULL)fclose(input);
#endif
    return current_data;

}

/**************************************************************************
 * METHOD: telescope_controller::sendcmd_loop(const telescope_command)
 *
 * DESCRIPTION:
 *   Sends command to the TCU and receives the response. Checks the
 *   response for errors. If there are errors, resends SEND_LOOP_TRIALS
 *   or until there are nor errors. Returns with the response. Delays
 *   SENDCMD_LOOP_DELAY seconds between iterations.
 *
 **************************************************************************/
char* telescope_controller::sendcmd_loop(telescope_command* command_req) {
    int response_code;
    char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; // unreasonably large
    int i,n_trials=SEND_LOOP_TRIALS;

    i=0;	
    response_code=-1;
    while(response_code!=0&&i<n_trials){			      

       tel_cmd_response=sendcmd(command_req);

       response_code = check_tel_response(tel_cmd_response);
       i++;

       if(response_code!=0)sleep(SENDCMD_LOOP_DELAY);
    }

    return tel_cmd_response;
}

/**************************************************************************
 * METHOD: telescope_controller::sendcmd(const telescope_command)
 *
 * DESCRIPTION:
 *   Sends command to the TCU and receives the response. Sends that
 *   response back. The response is the telemetry string grabbed
 *   from the continous telemetry stream.

 *
 **************************************************************************/
char* telescope_controller::sendcmd(telescope_command* command_req) {
    char* cmd_response;
    cmd_response = new char[TEL_RESPONSE_SIZE]; // unreasonably large

    if(VERBOSE){
        cerr << "sendcmd: sending command " << command_req->get_command_string() << endl;
        fflush(stderr);
    }
    //telescope_io(command_req->get_command_string(),cmd_response, server_name, com_port);
    fprintf(stderr,"telescope_controller: calling telescop_io with tcs_prog_name [%s] and tcs_status_file[%s]\n",tcs_prog_name, tcs_status_file);

    telescope_io(command_req->get_command_string(),cmd_response, tcs_prog_name, tcs_status_file);
    if(VERBOSE){
        cerr << "sendcmd: done sending command " << command_req->get_command_string() << endl;
        fflush(stderr);
    }
    
    return cmd_response;
}

/**************************************************************************
 * METHOD: telescope_controller::check_tel_response(char *response)
 *
 * DESCRIPTION:
 *   Check for correct length, format, and acknowledgement code of
 *   command response. Should be sizeof(TCS_Telemetry) characters long and have either
 *   and "e" or an "E" for byte 63 (com1), 64 (com2), .. 70 (com8).
 *   We only use one of the com1 or com2 ports, so just check for an "e" or and "E"
 *   in any of these two places. On error conditions, this value will be
 *   "1" for bogus command, "2" for bogus data, "3" for unrecognized command.
 *
 **************************************************************************/
int telescope_controller::check_tel_response(char *response) {
 
    int n,buf_size;
    TCS_Telemetry *t;

    /* check for proper length of response string */
    buf_size=TCS_TELEMETRY_SIZE;
    n=strlen(response);
    if(n!=buf_size){
        fprintf(stderr,"check_tel_response: unexpected length for telemetry string: %d %s\n",n,response);
        return(TCS_TELEMETRY_ERROR);
    }
	
    t=(TCS_Telemetry *)response;
    if(VERBOSE){fprintf(stderr,"check_tel_response: TCS response is:\n%s\n",(char *)t);fflush(stderr);}

    /*check for an "e" or an "E" in the com1 and com2 characters of the telemetry string, or else a "C" for nops */

    char status_char[2];
    status_char[1]=0;
    if(com_port==1){
        strncpy(status_char,t->com1,1);
    }
    else if (com_port==2){
        strncpy(status_char,t->com2,1);
    }
    else{
        fprintf(stderr,"telescope_controller::check_tel_response: unexpected value for com_port: %d\n",
		com_port);
        return(TCS_TELEMETRY_ERROR);
    }

    if(strncmp(status_char,"e",1)==0||strncmp(status_char,"E",1)==0){
        if(VERBOSE){
             fprintf(stderr,"check_tel_response: response is ok\n");
             fprintf(stderr,"status_char: %s\n",status_char);
             fflush(stderr);
        }
        return(TCS_TELEMETRY_OK);
    }

   /*Otherwise check for an "1", "2", "3", or "C" in the com1 and com2 characters of the telemetry string */
    else if(strncmp(status_char,"1",1)==0){
        fprintf(stderr,"check_tel_response: badly formed command: %s\n",response);
        fprintf(stderr,"check_tel_response: status character is : %s\n",status_char);
        return(TCS_BAD_COMMAND_ERROR);
    }
    else if(strncmp(status_char,"2",1)==0){
        fprintf(stderr,"check_tel_response: bad command arguments: %s\n",response);
        return(TCS_BAD_DATA_ERROR);
    }
    else if(strncmp(status_char,"3",1)==0){
        fprintf(stderr,"check_tel_response: unrecognized command: %s\n",response);
        return(TCS_UNRECOGNIZED_COMMAND_ERROR);
    }
    else if(strncmp(status_char,"C",1)==0){
        fprintf(stderr,"check_tel_response: checksum error: %s\n",response);
        return(TCS_CHECKSUM_ERROR);
    }
    else{
        fprintf(stderr,"check_tel_response: unknown error: %s\n",response);
        return (TCS_UNKNOWN_ERROR);
    }

}

/**************************************************************************
 * METHOD: telescope_controller::point(const point&, pointmode mode)
 *
 * DESCRIPTION:
 *   Commands the telescope to a given point. Returns 1 on success, 0 on
 *   failure and -1 on unknown response. To get the telescope to move we
 *   actually first load a point into an preset memory space and then
 *   ask the telescope to use that preset to move.
 *   Tracking after move is on or off according to mode.
 **************************************************************************/
int telescope_controller::point(struct point& point_request, pointmode mode) {
    int cmd_response;
    char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
    telescope_command *tel_cmd;

    target_position = point_request;

    // always try to take control (because you may not have it)
    cmd_response = take_control();
    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to take control\n", class_name);
	return cmd_response;
    }

    // enable servos. Sometimes a noisy limit sensor disables the servos.
    // If the telescope is in an actually limit, enable servos will only
    // allow small motion.

    cmd_response = enable_servos();
    if (cmd_response != 0) {
	fprintf(stderr, "%s: unable to enable servos\n",
	       class_name);
	return cmd_response;
    }

    // stop first and turn tracking off
    if ((cmd_response = stop()) != 0) {
	fprintf(stderr, "%s: failed sending stop command\n",
	       class_name);
	return cmd_response;
    }

    // get current telescope position
    struct point pos;
    double epoch, lst, ha;
    get_position(pos, &epoch, &lst, &ha);
    if(epoch==0.0){
       fprintf(stderr," %s: unable to get current telescope position\n",class_name);
       return -1;
    }

    fprintf(stderr,"current ra,dec = %lf %lf\n",pos.ra, pos.dec);


    // turn on sidereal tracking on if mode = TELMOUNT_SIDTRACK

    if(mode==TELMOUNT_SIDTRACK){
#if 0
       /* this is unnessary. MOVNEXT turns tracking on by default */
       // send command to start sidereal tracking
       tel_cmd = new telescope_command(TELESCOPE_CMD_TRACKON);
       tel_cmd_response = sendcmd(tel_cmd);
       if(cmd_response=check_tel_response(tel_cmd_response) !=0){
           fprintf(stderr, 
		"%s: unable to turn tracking on\n", class_name);
	  return cmd_response;
       }
       delete tel_cmd;
#endif
    }
    else{
      fprintf(stderr,"mode is not TELMOUNT_SIDTRACK, but point command is being used\n");
      return(-1);
    }


    // set next ra 
    if ((cmd_response = set_nextra(point_request.ra)) != 0) {
	fprintf(stderr, "%s: failed sending nextra command\n",
	       class_name);
	return cmd_response;
    }

    // set next dec 
    if ((cmd_response = set_nextdec(point_request.dec)) != 0) {
	fprintf(stderr, "%s: failed sending nextdec command\n",
	       class_name);
	return cmd_response;
    }
 
    /* This works, but prefered method is to use MOVRADEC instead */

    // send command to actuate to that position
    tel_cmd = new telescope_command(TELESCOPE_CMD_MOVENEXT);
    tel_cmd_response = sendcmd(tel_cmd);
    if(cmd_response=check_tel_response(tel_cmd_response) !=0){
        fprintf(stderr, 
		"%s: unable to move the telescope to the next position\n", class_name);
	return cmd_response;
    }
    delete tel_cmd;
    delete tel_cmd_response;

    // turn dome following back on */
    tel_cmd = new telescope_command(TELESCOPE_CMD_AUTO_DOME_ON);
    tel_cmd_response = sendcmd(tel_cmd);
    if(cmd_response=check_tel_response(tel_cmd_response) !=0){
        fprintf(stderr, 
		"%s: unable to set dome tracking on\n", class_name);
	return cmd_response;
    }
    delete tel_cmd;
    delete tel_cmd_response;

#if 0
    struct tcu_status cur_stat;
    if (stat_req(0,cur_stat) == -1) {
	// darn we bombed
	fprintf(stderr, "%s: unable to retrieve position from TCU\n",
	       class_name);
	return -1;
    }

    current_position.ra = cur_stat.rap;
    current_position.dec = cur_stat.decp;

 
    // calc az/el so we can use it later in status function
    double lst = cur_stat.lst;
    radec_ang rapoint(current_position.ra, current_position.dec);
    hadec_ang hapoint(rapoint, lst);
    azel_ang  aepoint(pos_lat, hapoint);
    cur_azel.az = aepoint.az;
    cur_azel.el = aepoint.el;

    fprintf(stderr,
	   "%s: set cur pos to ra: %f dec: %f\n", class_name,
	   current_position.ra, current_position.dec);
    fprintf(stderr,
	   "%s: set cur az/el pos to az: %f el: %f\n", class_name,
	   cur_azel.az, cur_azel.el);
#endif

    return 0;
}
/**************************************************************************
 * METHOD: telescope_controller::set_nextra(double)
 *
 * DESCRIPTION:
 *   Set the RA position for the next telescope move
 **************************************************************************/
int telescope_controller::set_nextra(double ra) {

    int cmd_response = -1;

    if (take_control() != 0) {
        fprintf(stderr, "%s: unable to take control of TCU\n", class_name);
        cmd_response = -1;
    }
    else{
       /* construct command string */
       telescope_command* tel_cmd = new telescope_command(TELESCOPE_CMD_NEXTRA);
       tel_cmd->setup_nextra(ra); 
       char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 
       tel_cmd_response = sendcmd(tel_cmd);
       cmd_response=check_tel_response(tel_cmd_response);

       if (cmd_response != 0) {
          fprintf(stderr, "%s: unable to set next ra for the telescope\n",
					 class_name);
       }
       delete tel_cmd;
       delete tel_cmd_response;
    }

    return cmd_response;

}

/**************************************************************************
 * METHOD: telescope_controller::set_nextdec(double)
 *
 * DESCRIPTION:
 *   Set the Dec position for the next telescope move
 **************************************************************************/
int telescope_controller::set_nextdec(double dec) {

    int cmd_response = -1;

    if (take_control() != 0) {
        fprintf(stderr, "%s: unable to take control of TCU\n", class_name);
        cmd_response = -1;
    }
    else{
       /* construct command string */
       telescope_command* tel_cmd = new telescope_command(TELESCOPE_CMD_NEXTDEC);
       tel_cmd->setup_nextdec(dec);

       char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 

       tel_cmd_response = sendcmd(tel_cmd);
       cmd_response=check_tel_response(tel_cmd_response);

       if (cmd_response != 0) {
          fprintf(stderr, "%s: unable to set next dec for the telescope\n",
					 class_name);
       }
       delete tel_cmd;
       delete tel_cmd_response;
    }

    return cmd_response;

}


/**************************************************************************
 * METHOD: telescope_controller::set_epoch(double)
 *
 * DESCRIPTION:
 *   Set the epoch for telescope RA and Dec positions
 **************************************************************************/
int telescope_controller::set_epoch(double epoch) {

    int cmd_response = -1;

    if (take_control() != 0) {
        fprintf(stderr, "%s: unable to take control of TCU\n", class_name);
        cmd_response = -1;
    }
    else if (epoch < 0.0 | epoch > 9999.99 ) {
        fprintf(stderr, "%s: requested epoch out of range : %f\n", class_name, epoch);
        cmd_response = -1;
    }
    else{
       /* construct command string */
       telescope_command* tel_cmd = new telescope_command(TELESCOPE_CMD_EPOCH);
       tel_cmd->setup_epoch(epoch);

       char* tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 

       tel_cmd_response = sendcmd(tel_cmd);
       cmd_response=check_tel_response(tel_cmd_response);

       if (cmd_response != 0) {
          fprintf(stderr, "%s: unable to set epoch for telescope coords\n",
					 class_name);
       }
       delete tel_cmd;
       delete tel_cmd_response;
    }

    return cmd_response;

}
/**************************************************************************
 * METHOD: telescope_controller::set_telescope_time(struct tm *tm)
 *
 * DESCRIPTION:
 *   Set the epoch for telescope RA and Dec positions
 **************************************************************************/
int telescope_controller::set_telescope_time(struct tm *tm) {
    telescope_command* tel_cmd;
    char* tel_cmd_response ; 
    int cmd_response = -1;

    if (take_control() != 0) {
        fprintf(stderr, "%s: unable to take control of TCU\n", class_name);
        cmd_response = -1;
    }
    else if (tm->tm_year<0 | tm->tm_year > 200 ) { /* year-1900 must be between 0 and 2100 */ 
        fprintf(stderr, "%s: requested year out of range : %f\n", class_name, tm->tm_year);
        cmd_response = -1;
    }
    else if (tm->tm_mon<0 | tm->tm_mon > 111 ) { /* month must be between 0 and 11 */ 
        fprintf(stderr, "%s: requested month out of range : %f\n", class_name, tm->tm_mon);
        cmd_response = -1;
    }
    else if (tm->tm_mday<1 | tm->tm_mday > 31 ) { /* day must be between 1 and 31 */ 
        fprintf(stderr, "%s: requested day out of range : %f\n", class_name, tm->tm_mday);
        cmd_response = -1;
    }
    else if (tm->tm_hour<0 | tm->tm_hour > 24 ) { /* hour must be between 0 and 24*/ 
        fprintf(stderr, "%s: requested hour out of range : %f\n", class_name, tm->tm_hour);
        cmd_response = -1;
    }
    else if (tm->tm_min<0 | tm->tm_min > 60 ) { /* hour must be between 0 and 60*/ 
        fprintf(stderr,"%s: requested minute out of range : %f\n", class_name, tm->tm_min);
        cmd_response = -1;
    }
    else if (tm->tm_sec<0 | tm->tm_sec > 60 ) { /* sec must be between 0 and 60*/ 
        fprintf(stderr,"%s: requested second out of range : %f\n", class_name, tm->tm_sec);
        cmd_response = -1;
    }
    else{

//DEBUG : errors setting date. skip this for now
#if 0
      /* construct setdate command string */
       tel_cmd = new telescope_command(TELESCOPE_CMD_SETDATE);
       tel_cmd->setup_date(tm);

       tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 

       tel_cmd_response = sendcmd(tel_cmd);
       cmd_response=check_tel_response(tel_cmd_response);

       if (cmd_response != 0) {
          fprintf(stderr, "%s: unable to set date for telescope\n",
					 class_name);
     	  return cmd_response;
       }
       delete tel_cmd;
       delete tel_cmd_response;
#endif
       /* construct settime command string */
       tel_cmd = new telescope_command(TELESCOPE_CMD_SETTIME);
       tel_cmd->setup_time(tm);

       tel_cmd_response = new char[TEL_RESPONSE_SIZE]; 

       tel_cmd_response = sendcmd(tel_cmd);
       cmd_response=check_tel_response(tel_cmd_response);

       if (cmd_response != 0) {
          fprintf(stderr, "%s: unable to set time for telescope\n",
					 class_name);
          return cmd_response;
       }
       delete tel_cmd;
       delete tel_cmd_response;
    }

    return cmd_response;

}
/**************************************************************************
 * METHOD: telescope_controller::slave_dome()
 *
 * DESCRIPTION:
 *   This makes sure the dome is slaved to the telescope. This is necessary
 *   for operation.
 **************************************************************************/
int telescope_controller::slave_dome() {

    char *tel_cmd_response = new char[TEL_RESPONSE_SIZE];
    telescope_command *tel_cmd;
    int cmd_response;

    // always try to take control (because you may not have it)
    cmd_response = take_control();
    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to take control\n", class_name);
        return cmd_response;
    }

    tel_cmd = new telescope_command(TELESCOPE_CMD_AUTO_DOME_ON);
    tel_cmd_response = sendcmd(tel_cmd);
    if(cmd_response=check_tel_response(tel_cmd_response) !=0){
        fprintf(stderr, 
		"%s: unable to set dome tracking on\n", class_name);
	return cmd_response;
    }
    delete tel_cmd;
    delete tel_cmd_response;
    return cmd_response;
}
    
/**************************************************************************
 * METHOD: telescope_controller::unslave_dome()
 *
 * DESCRIPTION:
 *   This stops the slaving of the dome, and stops any ongoing motion of the dome
 **************************************************************************/
int telescope_controller::unslave_dome() {

    char *tel_cmd_response = new char[TEL_RESPONSE_SIZE];
    telescope_command *tel_cmd;
    int cmd_response;

    // always try to take control (because you may not have it)
    cmd_response = take_control();
    if (cmd_response != 0) {
        fprintf(stderr, "%s: unable to take control\n", class_name);
        return cmd_response;
    }

    tel_cmd = new telescope_command(TELESCOPE_CMD_AUTO_DOME_OFF);
    tel_cmd_response = sendcmd(tel_cmd);
    if(cmd_response=check_tel_response(tel_cmd_response) !=0){
        fprintf(stderr, 
		"%s: unable to set dome tracking off\n", class_name);
	return cmd_response;
    }
    delete tel_cmd;
    delete tel_cmd_response;
    return cmd_response;
}
    
/**************************************************************************
 * METHOD: telescope_controller::slave_windscr()
 *
 * DESCRIPTION:
 *   This makes sure the windscr is slaved to the telescope. This is necessary
 *   for operation.
 **************************************************************************/
int telescope_controller::slave_windscr() {
    /* not implemented */
    fprintf(stderr,"slave_windscr: not implemented\n");

    return 0;
}
    
/**************************************************************************
 * METHOD: telescope_command::telescope_command()
 *
 * DESCRIPTION:
 *   Default constructor. I bet it never gets used. Since it is less
 *   usable then the other constructor
 **************************************************************************/
telescope_command::telescope_command() {
    command_string = new char[255];
}

/**************************************************************************
 * METHOD: telescope_command::telescope_command(int)
 *
 * DESCRIPTION:
 *   Constructor which allows the caller to set the command type to build
 **************************************************************************/
telescope_command::telescope_command(int cmd_code) {
    command_string = new char[255];
    set_command_string(cmd_code);
}

/**************************************************************************
 * METHOD: telescope_command::~telescope_command()
 *
 * DESCRIPTION:
 *   No big deal, just deletes the command_string memory.
 **************************************************************************/
telescope_command::~telescope_command() {
    delete command_string;
}

/**************************************************************************
 * METHOD: telescope_command::get_command_string()
 *
 * DESCRIPTION:
 *   Returns command_string. There better be a command there
 **************************************************************************/
char* telescope_command::get_command_string() {
    return command_string;
}

/**************************************************************************
 * METHOD: telescope_command::set_command_string(int)
 *
 * DESCRIPTION:
 *   Much like the constructor which takes an integer value
 *   describing the command type this method sets up the command string
 *   if it can and sets the command_type.
 **************************************************************************/
void telescope_command::set_command_string(int cmd_code) {
    switch (cmd_code) {
    case TELESCOPE_CMD_NOP:
        strcpy(command_string, TELESCOPE_CMD_NOP_STR);
        break;
    case TELESCOPE_CMD_STS:
        strcpy(command_string, TELESCOPE_CMD_STS_STR);
        break;
    case TELESCOPE_CMD_NOCMD:
        strcpy(command_string, TELESCOPE_CMD_NOCMD_STR);
        break;
    case TELESCOPE_CMD_STOW:
        strcpy(command_string, TELESCOPE_CMD_STOW_STR);
        break;
    case TELESCOPE_CMD_STOP:
        strcpy(command_string, TELESCOPE_CMD_STOP_STR);
        break;
    case TELESCOPE_CMD_NEXTRA:
        strcpy(command_string, TELESCOPE_CMD_NEXTRA_STR);
        break;
    case TELESCOPE_CMD_NEXTDEC:
        strcpy(command_string, TELESCOPE_CMD_NEXTDEC_STR);
        break;
    case TELESCOPE_CMD_MOVENEXT:
        strcpy(command_string, TELESCOPE_CMD_MOVENEXT_STR);
        break;
    case TELESCOPE_CMD_TRACKON:
	strcpy(command_string, TELESCOPE_CMD_TRACKON_STR);
	break;
    case TELESCOPE_CMD_TRACKOFF:
	strcpy(command_string, TELESCOPE_CMD_TRACKOFF_STR);
	break;
    case TELESCOPE_CMD_SLEWON:
	strcpy(command_string, TELESCOPE_CMD_SLEWON_STR);
	break;
    case TELESCOPE_CMD_SLEWOFF:
	strcpy(command_string, TELESCOPE_CMD_SLEWOFF_STR);
	break;
    case TELESCOPE_CMD_KILL:
	strcpy(command_string, TELESCOPE_CMD_KILL_STR);
	break;
    case TELESCOPE_CMD_UNKILL:
	strcpy(command_string, TELESCOPE_CMD_UNKILL_STR);
	break;
    case TELESCOPE_CMD_BIASON:
	strcpy(command_string, TELESCOPE_CMD_BIASON_STR);
	break;
    case TELESCOPE_CMD_BIASOFF:
	strcpy(command_string, TELESCOPE_CMD_BIASOFF_STR);
	break;
    case TELESCOPE_CMD_EPOCH:
	strcpy(command_string, TELESCOPE_CMD_EPOCH_STR);
	break;
    case TELESCOPE_CMD_SETDATE:
	strcpy(command_string, TELESCOPE_CMD_SETDATE_STR);
	break;
    case TELESCOPE_CMD_SETTIME:
	strcpy(command_string, TELESCOPE_CMD_SETTIME_STR);
	break;
    case TELESCOPE_CMD_MOVRADEC:
	strcpy(command_string, TELESCOPE_CMD_MOVERADEC_STR);
	break;
    case TELESCOPE_CMD_FOCUS:
	strcpy(command_string, TELESCOPE_CMD_FOCUS_STR);
	break;
    case TELESCOPE_CMD_RELFOCUS:
	strcpy(command_string, TELESCOPE_CMD_RELFOCUS_STR);
	break;
    case TELESCOPE_CMD_FOCUS_ZERO:
	strcpy(command_string, TELESCOPE_CMD_FOCUS_ZERO_STR);
	break;
   case TELESCOPE_CMD_TAKE_CONTROL:
        strcpy(command_string, TELESCOPE_CMD_TAKE_CONTROL_STR);
        break;
#ifdef DOME_OFF
    // just send NOP instead of DOME on
    case TELESCOPE_CMD_MOVE_DOME:
	strcpy(command_string, TELESCOPE_CMD_NOP_STR);
	break;
    case TELESCOPE_CMD_AUTO_DOME_ON:
	strcpy(command_string, TELESCOPE_CMD_NOP_STR);
	break;
    case TELESCOPE_CMD_AUTO_DOME_OFF:
	strcpy(command_string, TELESCOPE_CMD_NOP_STR);
	break;
    case TELESCOPE_CMD_STOP_DOME:
	strcpy(command_string, TELESCOPE_CMD_NOP_STR);
	break;
    case TELESCOPE_CMD_DOME_INIT:
	strcpy(command_string, TELESCOPE_CMD_NOP_STR);
	break;
    case TELESCOPE_CMD_STOW_DOME:
	strcpy(command_string, TELESCOPE_CMD_NOP_STR);
	break;
    case TELESCOPE_CMD_DOME_OPEN:
	strcpy(command_string, TELESCOPE_CMD_NOP_STR);
	break;
    case TELESCOPE_CMD_DOME_CLOSE:
	strcpy(command_string, TELESCOPE_CMD_NOP_STR);
	break;
#else         
    case TELESCOPE_CMD_MOVE_DOME:
	strcpy(command_string, TELESCOPE_CMD_MOVE_DOME_STR);
	break;
    case TELESCOPE_CMD_AUTO_DOME_ON:
	strcpy(command_string, TELESCOPE_CMD_AUTO_DOME_ON_STR);
	break;
    case TELESCOPE_CMD_AUTO_DOME_OFF:
	strcpy(command_string, TELESCOPE_CMD_AUTO_DOME_OFF_STR);
	break;
    case TELESCOPE_CMD_STOP_DOME:
	strcpy(command_string, TELESCOPE_CMD_STOP_DOME_STR);
	break;
    case TELESCOPE_CMD_DOME_INIT:
	strcpy(command_string, TELESCOPE_CMD_DOME_INIT_STR);
	break;
    case TELESCOPE_CMD_STOW_DOME:
	strcpy(command_string, TELESCOPE_CMD_STOW_DOME_STR);
	break;
    case TELESCOPE_CMD_DOME_OPEN:
	strcpy(command_string, TELESCOPE_CMD_DOME_OPEN_STR);
	break;
    case TELESCOPE_CMD_DOME_CLOSE:
	strcpy(command_string, TELESCOPE_CMD_DOME_CLOSE_STR);
	break;
#endif
    case TELESCOPE_CMD_ZENITH:
	strcpy(command_string, TELESCOPE_CMD_ZENITH_STR);
	break;
    case TELESCOPE_CMD_STOW20:
	strcpy(command_string, TELESCOPE_CMD_STOW20_STR);
	break;
 
#ifdef TCS_DEMO

    /* these commands no longer available. TCS demo only */
    case TELESCOPE_CMD_ABERON:
	strcpy(command_string, TELESCOPE_CMD_ABERON_STR);
	break;
    case TELESCOPE_CMD_ABEROFF:
	strcpy(command_string, TELESCOPE_CMD_ABEROFF_STR);
	break;
    case TELESCOPE_CMD_UNABERON:
	strcpy(command_string, TELESCOPE_CMD_UNABERON_STR);
	break;
    case TELESCOPE_CMD_UNABEROFF:
	strcpy(command_string, TELESCOPE_CMD_UNABEROFF_STR);
	break;
    case TELESCOPE_CMD_FLEXON:
	strcpy(command_string, TELESCOPE_CMD_FLEXON_STR);
	break;
    case TELESCOPE_CMD_FLEXOFF:
	strcpy(command_string, TELESCOPE_CMD_FLEXOFF_STR);
	break;
    case TELESCOPE_CMD_UNFLEXON:
	strcpy(command_string, TELESCOPE_CMD_UNFLEXON_STR);
	break;
    case TELESCOPE_CMD_UNFLEXOFF:
	strcpy(command_string, TELESCOPE_CMD_UNFLEXOFF_STR);
	break;
    case TELESCOPE_CMD_NUTON:
	strcpy(command_string, TELESCOPE_CMD_NUTON_STR);
	break;
    case TELESCOPE_CMD_NUTOFF:
	strcpy(command_string, TELESCOPE_CMD_NUTOFF_STR);
	break;
    case TELESCOPE_CMD_PARAON:
	strcpy(command_string, TELESCOPE_CMD_PARAON_STR);
	break;
    case TELESCOPE_CMD_PARAOFF:
	strcpy(command_string, TELESCOPE_CMD_PARAOFF_STR);
	break;
    case TELESCOPE_CMD_PREON:
	strcpy(command_string, TELESCOPE_CMD_PREON_STR);
	break;
    case TELESCOPE_CMD_PREOFF:
	strcpy(command_string, TELESCOPE_CMD_PREOFF_STR);
	break;
    case TELESCOPE_CMD_PROPERON:
	strcpy(command_string, TELESCOPE_CMD_PROPERON_STR);
	break;
    case TELESCOPE_CMD_PROPEROFF:
	strcpy(command_string, TELESCOPE_CMD_PROPEROFF_STR);
	break;
    case TELESCOPE_CMD_REFRACON:
	strcpy(command_string, TELESCOPE_CMD_REFRACON_STR);
	break;
    case TELESCOPE_CMD_REFRACOFF:
	strcpy(command_string, TELESCOPE_CMD_REFRACOFF_STR);
	break;
    case TELESCOPE_CMD_UNREFRACON:
	strcpy(command_string, TELESCOPE_CMD_UNREFRACON_STR);
	break;
    case TELESCOPE_CMD_UNREFRACOFF:
	strcpy(command_string, TELESCOPE_CMD_UNREFRACOFF_STR);
	break;
#endif
#if 0
    case TELESCOPE_CMD_WTHR_STS:
        strcpy(command_string, TELESCOPE_CMD_WTHR_REQ);
        break;
    case TELESCOPE_CMD_OPEN_DOME:
        strcpy(command_string, TELESCOPE_CMD_OPEN_DOME_STR);
        break;
    case TELESCOPE_CMD_CLOSE_DOME:
        strcpy(command_string, TELESCOPE_CMD_CLOSE_DOME_STR);
        break;
#endif
    default:
        fprintf(stderr,"bad telescope command code: %d\n",
	       cmd_code);
        strcpy(command_string, TELESCOPE_CMD_NOP_STR);
    }
}



/**************************************************************************
 * METHOD: telescope_command::setup_focus(int)
 *
 * DESCRIPTION:
 *   Setup the string to set the focus position of the telescope
 **************************************************************************/
void telescope_command::setup_focus(int focus_steps) {

    char string[256];

    sprintf(string,"%d",focus_steps);
    strcat(command_string, " ");
    strcat(command_string, string);

    return ;

}
/**************************************************************************
 * METHOD: telescope_command::setup_nextra(double)
 *
 * DESCRIPTION:
 *   Setup the string to set the RA position for the next telescope move
 **************************************************************************/
void telescope_command::setup_nextra(double ra) {

    int dd,mm;
    double ss;
    char string[256];

    degdms(ra,&dd,&mm,&ss);
    sprintf(string,"%02d%02d%04.1f",dd,mm,ss);
    strcat(command_string, " ");
    strcat(command_string, string);

    return ;

}
/**************************************************************************
 * METHOD: telescope_command::setup_nextdec(double)
 *
 * DESCRIPTION:
 *   Setup the string to set the Dec position for the next telescope move
 **************************************************************************/
void telescope_command::setup_nextdec(double dec) {

    int dd,mm;
    double ss;
    char string[256];

    degdms(dec,&dd,&mm,&ss);

       
    if( dec > 0.0 || dec <= -1.0 ) {
      sprintf(string,"%+02d%02d%04.1f",dd,mm,ss);
    }
    else {
      if(mm<0)mm=-mm;
      if(ss<0.0)ss=-ss;
      sprintf(string,"-00%02d%04.1f",mm,ss);
    }

 
    strcat(command_string, " ");
    strcat(command_string, string);

 
    return ;

}
/**************************************************************************
 * METHOD: telescope_command::setup_epoch(double)
 *
 * DESCRIPTION:
 *   Setup the string to change the epoch for telescope coords
 **************************************************************************/
void telescope_command::setup_epoch(double epoch) {

    char string[256];

    sprintf(string,"%+07.2f",epoch);
    strcat(command_string, " ");
    strcat(command_string, string);

 
    return ;

}
/**************************************************************************
 * METHOD: telescope_command::setup_date(struct tm *tm)
 *
 * DESCRIPTION:
 *   setup the string to set the date for the telescope controller
 **************************************************************************/
void telescope_command::setup_date(struct tm *tm) {

    char string[256];

    sprintf(string,"%02d/%02d/%04d",tm->tm_mon+1,tm->tm_mday,tm->tm_year+1900);
    strcat(command_string, " ");
    strcat(command_string, string);
 
    return ;

}
/**************************************************************************
 * METHOD: telescope_command::setup_time(struct tm *tm)
 *
 * DESCRIPTION:
 *   setup the string to set the time for the telescope controller
 **************************************************************************/
void telescope_command::setup_time(struct tm *tm) {

    char string[256];
    double s;
    s=tm->tm_sec;
    sprintf(string,"%02d%02d%04.1f",tm->tm_hour,tm->tm_min,s);
    strcat(command_string, " ");
    strcat(command_string, string);
 
    return ;

}


