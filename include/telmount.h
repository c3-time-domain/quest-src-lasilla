/*
 * $RCSfile: telmount.h,v $
 * $Revision: 1.8 $
 * $Date: 2010/10/12 14:37:27 $
 *
 * telescope mount - application interface
 *
 * Steve Groom, JPL
 * July, 1995
 *
 * Modified by
 * Erik Hovland, JPL
 * Jan-Mar, 2000
 */
#ifndef _TELMOUNT_H_
#define _TELMOUNT_H_

#include <stdio.h>
#include <site.h>
#include <tm_angles.h>
#include <telescope_controller.h>


#define TELMOUNT_CMD_RETRIES	5	/* max retries of command */

#define TELCMD_LIST_SIZE	32	/* max commands in a command list */

/* This is the station address word used by the GEODSS telescope mount
 * Bus Interface Unit (BIU).  Normally these are always configured
 * at this address, but if it ever needs to be changed, here's where to do it.
 */
#define TELCMD_STATION_ADDRESS	0x7	/* mount station address bits */

#define TELMOUNT_POS_UNKNOWN	-999	/* value for unknown positions */

/* dome moves smaller than this amount are ignored. */
#define TELMOUNT_DOME_MIN_MOVE	2.0	/* min dome azm change, deg */

/**** Telescope mount motion limits ****/
#define TELMOUNT_EL_MIN		16.0	/* min safe elev angle (deg) */
#define TELMOUNT_POL_MIN	0.0	/* min safe pol position (deg) */
#define TELMOUNT_POL_MAX	360.0	/* max safe pol position (deg) */
#define TELMOUNT_POL_CENTER	180.0	/* center of the sky around pol axis */
/* The min and max gdec appear to be backwards here, since the gdec value wraps
 * at 360.  The acceptable ranges are MIN_GDEC to 360, and 0 to MAX_GDEC.
 * This is a continuous range from MIN_GDEC, through 360==0, to MAX_GDEC.
 */
#define TELMOUNT_GDEC_MIN	330.0	/* min safe gdec position (deg) */
#define TELMOUNT_GDEC_MAX	130.0	/* max safe gdec position (deg) */

#define TELMOUNT_POL_STOW	180.0	/* POL of stow position */
#define TELMOUNT_GDEC_STOW	0.0	/* GDEC of stow position */
#define TELMOUNT_BIG_POL_MOVE	165.0	/* be careful when pol move is big */

#define TELMOUNT_POS_ERR_FAST  0.005   /* max pos readout delta, fast track */
#define TELMOUNT_POS_ERR_SLOW  0.0008  /* max pos readout delta, slow track */

//#define TELMOUNT_FOCUS_MIN          0       /* low end of focus travel */
//#define TELMOUNT_FOCUS_MAX          2047    /* high end of focus travel */

/* strings used by telmount_client and telmount::interp() */
#define TELMOUNTCL_MSG_OK	"ok"		/* command OK */
#define TELMOUNTCL_MSG_ERROR	"error"		/* command returned error */
#define TELMOUNTCL_MSG_BADVAL	"badval"	/* bad or unsafe value */

/* values used by did_move to determine tolerance */
#define HARRIS_TOLERANCE	0.05

enum errcode {
    TELMOUNT_OK,
    TELMOUNT_ERROR,
    TELMOUNT_BADVAL
};


// some utility functions that are related, but not member functions
// since we want other code to be able to call them.
// OK to move to ra/dec ?
int safepos(site &s, double ra, double dec, double uxtime);
int safepos_pd(site &s, double pol, double dec); // OK to move to pol/gdec ?
int safepos_rd(int harris_fd, double ra, double dec);
//char *harris_build_rd_test(char *cmd_string, double ra, double dec);
double dtor(double degrees);
double htor(double hours);
double calc_time_move(double ra1, double dec1, double ra2, double dec2);

class telmount;		// forward declaration

struct telcmd {
    int tc_result;	/* result code from command */
    u_int tc_cmd;	/* command word */
    u_int *tc_data;	/* pointer to user data word */
};

struct harris_ops_mode {
	int track_mode;
	int neat_mode;
};

struct harris_radec {
	double ra;
	double dec;
};

class telcmdlist {
    friend class telmount;
    int ncmds;
    struct telcmd tc[TELCMD_LIST_SIZE];
public:
    telcmdlist() {ncmds = 0;}
    void reset() {ncmds = 0;}
    int add(u_int cmd, u_int *data_p)
	{
	    if (ncmds >= TELCMD_LIST_SIZE)
	    {
		fprintf(stderr,"tclcmdlist::add: command list overflow\n");
		return -1;
	    }
	    // Use command in low byte OR'd with station address
	    // in bits 9-11
	    tc[ncmds].tc_cmd = cmd;
	    tc[ncmds].tc_data = data_p;
	    ncmds++;
	    return 0;
	}
};

class telmount
{
private:
    site tm_site;
    polgdec_enc tm_pde;		/* last known position of mount, bcd words */
    polgdec_ang tm_pda;		/* same, as angles */
    double tm_rate_pol;		/* last commanded rate, pol axis */
    double tm_rate_gdec;	/* last commanded rate, dec axis */
    double tm_dome_az;		/* last commanded dome azimuth */
    double tm_focus;		/* last command focus position */
    telescope_controller* tcu;
    int debug;
    int point_timeout;		/* timeout for moving to a single position */

    int do_move_rd(double ra, double dec, pointmode mode);
    				/* make a position move */
    void report_fault(int, int);	/* reports system status to log */
    double rtod(double radians);
    double rtoh(double radians);

public:
    // constructor error indicator
    int fail;
    int harris_fd;
    double current_ra, current_dec;

    // constructors & destructors
    telmount();
    telmount(int fake_flag);
    ~telmount();

    // accessors
    site tmsite()	{return tm_site;}
    char *sitename()	{return tm_site.name();}
    double sitelat()	{return tm_site.lat();}
    double sitelon()	{return tm_site.lon();}

    // member functions
    int tracking_status();		// return tracking status of telescope
    int telescope_status();		// return all telescope status info 
    int dump();			// display all mount registers
				// read current ra/dec
    int getpos(double *ra, double *dec, double *uxtime);
    //int getpos_rd(double *ra_p, double *dec_p, double *uxtime);
    int getfocus(double *focus_p);
    int getweather(struct weather_data*);
    int getfaults(char*);
    int getdomeaz(double&);
    int stopmount();		// stop mount (mount drivers off)
    int stop();			// turn off tracking, leave pointing
    int stow();			// move dome and mount to stow position
    int move_zenith();	       // move telescope to zenith. stop tracking.
 // check that position is above horizon limit
    int check_pointing(double ra, double dec, double *zenith_ra, double *zenith_dec);
    int point(double ra, double dec, enum pointmode mode); // point to ra/dec
    int point_rd(double ra, double dec, enum pointmode mode); // pt to ra/dec
    int focus(double fset);		// set focus
    int wait_ontarget(int dome_check_flag); // wait for telescope to settle
    int interp(int fd_out, char *command);	// client command interpreter
};

// telmount client class, used to interact with telmgr.
class telmount_client
{
private:
    int tm_pipe[2];
    site tm_site;

public:
    // constructor error indicator
    int fail;
		int trkstate;

    // constructors & destructors
    telmount_client();
    ~telmount_client();

    // accessors
    site tmsite()	{return tm_site;}
    char *sitename()	{return tm_site.name();}
    double sitelat()	{return tm_site.lat();}
    double sitelon()	{return tm_site.lon();}

    // member functions
    int tracking_status();		// display all dr11-w registers
    int telescope_status();		// display all dr11-w registers
    int dump();			// display all mount registers
				// read current ra/dec
    int getpos(double *ra, double *dec, double *uxtime);
				// read current pol/gdec
    int getpos_pd(double *pol_p, double *gdec_p, double *uxtime);
    int getfocus(double *focus_p);

    int stopmount();		// stop mount (mount drivers off)
    int stop();			// turn off tracking, leave pointing
    int stow();				// move dome and mount to stow position
    int point(double ra, double dec, enum pointmode mode); // point to ra/dec
    int point_pd(double pol, double dec, enum pointmode mode); // pt to pol/gdec
    int dome(double azimuth);		// rotate dome to azimuth
    int focus(double fset);		// set focus
};

#endif /* _TELMOUNT_H_ */
