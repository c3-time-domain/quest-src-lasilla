/**************************************************************************
 * $RCSfile: telescope_controller.h,v $
 * $Revision: 1.8 $
 * $Date: 2010/10/12 14:37:27 $
 * Author - Erik Hovland
 *
 * Class declaration for using the new telescope controller at palomar
 **************************************************************************/

#ifndef INC_TELESCOPE_CONTROLLER_H
#define INC_TELESCOPE_CONTROLLER_H

#include <observer.h>
#include <telescope_drv.h>
#include <point.h>
#include <tm_angles.h>
#include <dms.h>
#include <neattime.h>
typedef unsigned int useconds_t;

#define DEFAULT_COORDINATE_EPOCH 2000.0 /* epoch for telescope coordinates */
#define WAIT_TIME 3.0 /* seconds to wait after move command to see if telescope has settled */
#define MAX_RA_DRIFT 1.0 /* 1.0 arcsec drift in RA per WAIT_TIME seconds is allowed after position slew */
#define MAX_DEC_DRIFT 1.0 /* 1.0 arcsec drift in DEC per WAIT_TIME seconds is allowed after position slew*/
#define POINTING_TOLERANCE 60.0 /* arcsecs allowed between intended and returned pointing */

/* parameters to convert focus setting from steps (as understood by telescope controller)
   to mm (as understood by scheduler) */
#define FOCUS_MM0 26.65
#define FOCUS_STEPS0 -136000
#define FOCUS_MM_TO_STEPS 84100.0

/* perl script for getting weather info */
#define WEATHER_CLIENT "/home/observer/quest-src-lasilla/util/weather_client.pl"
/* temporary file for storing weather info */
#define WEATHER_FILE "/tmp/weather.tmp"

/* ACU_STATUS_FILTER_ID_BYTE
   This is the  byte of the ACU status string that gives the filter index, as set up
   by John Henning 2004 Jun 02:

    This is to advise that filter encoding is now implemented at the Samuel
    Oschin Telescope.  The "FB 1" command returns a numeric filter ID in byte
    36 (counting from byte 1), with the translations to filter type listed
    below:

    01: no_filter
    02: Gunn_rizz
    04: Johnson_UBRI
    08: RG610
    10: not_assigned
    20: not_assigned
    40: not_assigned
    80: Clear

    The ASCII name of the installed filter is returned from the "STM 1"
    command in a string of the form "FILTER=xxxxxx" (note: other items may be
    returned also).

    For those on site, the filter name is displayed in the TCS status window
    as a line of the form "FILTER=xxxxxx".  This may appear anyplace among the
    status items displayed.

*/

/* telescope tracking status values */
  
#define TELESCOPE_STATUS_UKNOWN -1
#define TELESCOPE_SLEWING 0
#define TELESCOPE_TRACKING_ON_TARGET 1
#define TELESCOPE_TRACKING_OFF_TARGET 2
#define TELESCOPE_PARKED  3 
#define TELESCOPE_DISABLED 4
#define TELESCOPE_HORIZON_LIMIT 5
#define TELESCOPE_RA_LIMIT 6
#define TELESCOPE_DEC_LIMIT 7
#define TELESCOPE_DOME_MOVING 8

/* dome status values */

#define DOME_STOPPED 0



enum TELESCOPE_CMDS {
/* command name				value */
/* ------------				----- */
   TELESCOPE_CMD_NOP=			0,
   TELESCOPE_CMD_STOW=			1,
   TELESCOPE_CMD_STOP=			2,
   TELESCOPE_CMD_STS=			3,
   TELESCOPE_CMD_NEXTRA=		4,
   TELESCOPE_CMD_NEXTDEC=		5,
   TELESCOPE_CMD_MOVENEXT=		6,
   TELESCOPE_CMD_TRACKON=		7,
   TELESCOPE_CMD_TRACKOFF=		8,
   TELESCOPE_CMD_SLEWON=		9,
   TELESCOPE_CMD_SLEWOFF=		10,
   TELESCOPE_CMD_KILL=			11,
   TELESCOPE_CMD_UNKILL=		12,
   TELESCOPE_CMD_BIASON=		13,
   TELESCOPE_CMD_BIASOFF=		14,
   TELESCOPE_CMD_EPOCH=			15,
   TELESCOPE_CMD_SETTIME=		16,
   TELESCOPE_CMD_SETDATE=		17,
   TELESCOPE_CMD_MOVRADEC=		18,
   TELESCOPE_CMD_FOCUS=			19,
   TELESCOPE_CMD_RELFOCUS=		20,
   TELESCOPE_CMD_FOCUS_ZERO=		21,
   TELESCOPE_CMD_MOVE_DOME= 		22,
   TELESCOPE_CMD_AUTO_DOME_ON=		23,
   TELESCOPE_CMD_AUTO_DOME_OFF=		24,
   TELESCOPE_CMD_STOP_DOME=		25,
   TELESCOPE_CMD_DOME_INIT=		26,
   TELESCOPE_CMD_STOW_DOME=		27,
   TELESCOPE_CMD_DOME_OPEN=		28,
   TELESCOPE_CMD_DOME_CLOSE=		29,
   TELESCOPE_CMD_ZENITH=		30,
   TELESCOPE_CMD_TAKE_CONTROL=          31,
   TELESCOPE_CMD_STOW20=                32,
   TELESCOPE_CMD_NOCMD=			33

#ifdef TCS_DEMO
   TELESCOPE_CMD_ABERON=		15,
   TELESCOPE_CMD_ABEROFF=		16,
   TELESCOPE_CMD_UNABERON=		17,
   TELESCOPE_CMD_UNABEROFF=		18,
   TELESCOPE_CMD_FLEXON=		19,
   TELESCOPE_CMD_FLEXOFF=		20,
   TELESCOPE_CMD_UNFLEXON=		21,
   TELESCOPE_CMD_UNFLEXOFF=		22,
   TELESCOPE_CMD_NUTON=			23,
   TELESCOPE_CMD_NUTOFF=		24,
   TELESCOPE_CMD_PARAON=		25,
   TELESCOPE_CMD_PARAOFF=		26,
   TELESCOPE_CMD_PREON=			27,
   TELESCOPE_CMD_PREOFF=		28,
   TELESCOPE_CMD_PROPERON=		29,
   TELESCOPE_CMD_PROPEROFF=		30,
   TELESCOPE_CMD_REFRACON=		31,
   TELESCOPE_CMD_REFRACOFF=		32,
   TELESCOPE_CMD_UNREFRACON=		33,
   TELESCOPE_CMD_UNREFRACOFF=		34,
#endif
 };

/* old focus limits
enum FCS_LIMITS { MIN_FCS = 0, MAX_FCS = 1000000 };
*/

enum FCS_LIMITS { MIN_FCS = -250000, MAX_FCS = -100500 };

// Enums follow the Telescope Interface Spec, see that document for more
// information. Specifically the STATUS REQUEST (sec 8.7) and BINARY
// STATUS REQUEST (sec. 8.6) commands.

// tcu_modes correspond to each mode available in the mode string returned
// by the STAT command
enum mode_index {
/* mode name		array entry */
/* ---------		----------- */
   motion_mode =		0,
   wobble_mode =		1,
   ra_limit_mode =		2,
   dec_limit_mode =		3,
   horiz_limit_mode =		4,
   drive_status_mode =		5,
   tot_modes =			6
};

enum motion_status_bits {null_bit,dec_bit,ra_bit,dome_bit};
enum motion_status_value { stable_motion };
enum wobble_status_value {not_on_beam, at_beam1, at_beam2 };
enum limit_status_value {not_in_limit, in_limit };
enum drive_status_value {not_disabled, disabled };

enum telescope_fault_code {
   no_fault,ra_limit_fault,dec_limit_fault,horiz_limit_fault,drive_fault, comm_fault 
};

struct tcu_status {
    double rap; /* ra position in hours */
    double hap; /* ha position in hours */
    double decp; /* dec position in degrees */
    double lst; /* Local Sidereal time in hours */
    int focusp; /* focus position in steppor motor steps */
    double domep; /* position error of dome  (<0 west, >0 east)*/
    double windscreenp;
    double jd;
    double epoch;
    int mode[8];
    int dome_shutter_state;
    double az; /* telescope azimuth */
    double el; /* telescope elevation */
    double secz ;/* telescope elevation */
    double ut; /* telescope elevation */
};

enum dmodes {
/* dome mode name	value */
/* --------------	----- */
   dstopm =		70,
   dposdm =		71,
   dstowm =		72,
   dslavem =		73,
   dmaintm =		53
};

enum wmodes {
/* wind screen modes	value */
/* -----------------	----- */
   wstopm =		70,
   wposdm =		71,
   wstowm =		72,
   wslavem =		73,
   wmaintm =		53
};


// the return values for dome_status()
enum d_stat_vals {
/* dome stat name		value */
/* --------------		----- */
   d_success =			0,
   d_comm_failed =		-8
};

// the return vaules for dome_shutter_status()
enum ds_stat_vals {
/* dome shutter stat name	value */
/* ----------------------	----- */
   ds_open =			1,
   ds_closed =			0,
   ds_stat_unknown =		-1,
   ds_comm_failed =		-2
};

const unsigned int acu_stat_size = 40;
const unsigned int ccu_stat_size = 30;
const int dome_shutter_timeout = 300;
const int focus_timeout = 600;
const int FOCUS_TOLERANCE_STEPS = 10;

class telescope_command; // forward declaration

struct weather_data {
	double temp;
	double humidity;
	double wind_speed;
	double wind_direction;
	double dew_point;
	double pressure; 
	int dome_states;
};

struct weather_triggers {
    double max_wind_speed;
    double dew_point_tolerance;
};


struct d_status {
    unsigned short dome_mode;
    unsigned short dome_submode;
    double domep;
    int dome_mismatch_flag;
};

struct ws_status {
    unsigned short windscr_mode;
    unsigned short windscr_submode;
    double windscrp;
};
/**************************************************************************
 * CLASS: telescope_controller
 *
 * DESCRIPTION:
 *   This class encapsulates all the commands that should be necessary to
 *   control the telescope. The telescope is interfaced through the
 *   serial port to another computer which actually does all of the
 *   control. The operation should contain all commands that exist in the
 *   telmount class so that we can use the telmount interface for external
 *   commanding of the telescope (telmgr and obsmgr)
 **************************************************************************/
class telescope_controller {
	public:
		enum TC_RESPONSES { TC_NACK = 0, TC_ACK = 1, TC_FAIL = -1};
		telescope_controller();
		telescope_controller(char*);
		telescope_controller(int,int);
		~telescope_controller();
		int take_control();
		int fault_status();
		int point(struct point&, pointmode mode);
		void get_position(struct point& current_position, 
				double *epoch, double *lst, double *ha);
		int initialize(site *tm_site);
		int stop();
		int stow();
		int zenith(); /* move telescope to zenith. Track there */
		int tracking_status();
		int telescope_status();
		char* faults();

		int wait_move();
		int set_nextra(double ra);
		int set_nextdec(double dec);
		int set_epoch(double epoch);
		double get_focus();
                int get_filter();
		int set_focus(const double);
		int focus_status();
		int stat_req(int passive_flag, struct tcu_status&);
		int stop_focus();
		int open_dome_shutter();
		int close_dome_shutter();
		int slave_dome();
		int unslave_dome();
		int slave_windscr();
		int dome_status(struct d_status&);
		int windscr_status(struct ws_status&);
		int dome_shutter_status();
		struct weather_data get_weather_data();
		int get_weather_triggers(struct weather_triggers*);
		int set_link_timeout(const double);
		int set_telescope_time(struct tm *tm);
		int disable_servos();
		int enable_servos();
		//double get_link_timeout();
		char* sendcmd(telescope_command*);
		char* sendcmd_loop(telescope_command*);
		int check_tel_response(char*);
		void setcoords(const double&, const double&);
		int com_port;
		int point_timeout;
		int convert_focus_to_steps(double f_mm);
		double convert_focus_to_mm(int f_steps);

	private:
		char* class_name;
		bool has_control;
		void dummy_load_weather(struct weather_data*);
		struct point current_position;
		struct point target_position;
		double pos_lon;
		double pos_lat;
		azel_ang cur_azel;
//		double rate;
};

class telescope_command {
	public:
		telescope_command();
		//telescope_command(struct point&);
		//telescope_command(const double);
		telescope_command(int);
		~telescope_command();

		char* get_command_string();
		void set_command_string(int cmd_code);
		//void set_command_type(int);
		int set_point(struct point&);
		//int set_timeout(const double);
		void setup_focus(int focus_steps);
		void setup_nextra(double ra);
		void setup_nextdec(double dec);
		void setup_epoch(double epoch);
		void setup_date(struct tm *tm);
		void setup_time(struct tm *tm);

	private:
		char* command_string;
		int command_type;
};

#include <string.h>
#include <unistd.h>

using namespace std;

#endif //INC_TELESCOPE_CONTROLLER_H
