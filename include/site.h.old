/* 
 * $RCSfile: site.h,v $
 * $Revision: 1.6 $
 * $Date: 2009/07/23 20:57:10 $
 *
 * Information about the NEAT observation site
 * 
 * Steve Groom, JPL
 * July, 1995
 */
#ifndef _SITE_H_
#define _SITE_H_

#include <neatconf.h>
#include <point.h>
#include <mathconst.h>

// Configuration file keyword strings
#define CONF_SITE_NAME			"SITE_NAME"
#define CONF_SITE_LAT			"SITE_LAT"
#define CONF_SITE_LON			"SITE_LON"
#define CONF_SITE_ELEV			"SITE_ELEV"
#define CONF_SITE_DEBUG			"SITE_DEBUG"
#define CONF_MOUNT_POINT_TIMEOUT	"MOUNT_POINT_TIMEOUT"
#define CONF_SITE_MIN_ELEVATION         "SITE_MIN_ELEVATION"
#define CONF_SITE_SAFE_ELEVATION         "SITE_SAFE_ELEVATION"
#define TCS_COM_PORT                     "TCS_COM_PORT"
#define TCS_FAST_SLEW_ON 		"TCS_FAST_SLEW_ON"
#define TCS_BIAS_RATE_ON		"TCS_BIAS_RATE_ON"

#ifdef TCS_DEMO
#define TCS_ABERRATION_CORR_ON		"TCS_ABERRATION_CORR_ON" 
#define TCS_ABERRATION_DECONV_ON	"TCS_ABERRATION_DECONV_ON"
#define TCS_FLEXTURE_CORR_ON 		"TCS_FLEXTURE_CORR_ON"
#define TCS_FLEXTURE_DECONV_ON		"TCS_FLEXTURE_DECONV_ON"
#define TCS_REFRACTION_CORR_ON 		"TCS_REFRACTION_CORR_ON"
#define TCS_REFRACTION_DECONV_ON	"TCS_REFRACTION_DECONV_ON"
#define TCS_NUTATION_CORR_ON 		"TCS_NUTATION_CORR_ON"
#define TCS_PARALLAX_CORR_ON 		"TCS_PARALLAX_CORR_ON"
#define TCS_REFRACTION_CORR_ON 		"TCS_REFRACTION_CORR_ON"
#define TCS_PRECESSION_CORR_ON 		"TCS_PRECESSION_CORR_ON"
#define TCS_PROPER_MOTION_CORR_ON 	"TCS_PROPER_MOTION_CORR_ON"
#define TCS_COSDEC_CORR_ON 		"TCS_COSDEC_CORR_ON"
#endif

#define TCS_COORD_EPOCH 		"TCS_COORD_EPOCH"
#define TCS_RA_BIAS_RATE 		"TCS_RA_BIAS_RATE"
#define TCS_DEC_BIAS_RATE 		"TCS_DEC_BIAS_RATE"
#define TCS_SYNC_TIME_ON 		"TCS_SYNC_TIME_ON"



struct configrec
{
    char line[128];
    char *keyword;
    char *valstr;
    configrec *next;
};

class configfile {
private:
    configrec *headrec;
    
public:
    int fail;

    // constructor
    configfile(char *filename);
    // destructor
    ~configfile();

    int lookup(char *keyword);
    int lookup(char *keyword, char *val);
    int lookup(char *keyword, int *val);
    int lookup(char *keyword, double *val);
    void print();
};

// Information about the telescope location
class conf_location {
friend class site;
private:
    char name[64];		// name of site
    double lat;		// latitude of site, degrees north
    double lon;		// longitude of site, degrees EAST longitude
    double elev;	// elevation of site, meters
    double slat;	// sin of latitude
    double clat;	// cos of latitude
    double min_elevation; // minimum elevation of pointings
    double safe_elevation; // safe elevation for  pointings without going to zenith first

public:
    // member functions
    int configure(configfile &conf);
};

// Information about the telescope mount
class conf_mount {
friend class site;
private:

    // timeout associated to a single move of the telescope in seconds
    int mount_point_timeout;

public:
    // member functions
    int configure(configfile &conf);
};

// Information about the telescope control software
class conf_tcs {
friend class site;
private:

   // member functions
    int configure(configfile &conf);
    int com_port;
    int fast_slew_on;
    int bias_rate_on;
    int aberration_correction_on;
    int aberration_deconvolution_on;
    int flexture_correction_on;
    int flexture_deconvolution_on;
    int nutation_correction_on;
    int parallax_correction_on;
    int precession_correction_on;
    int proper_motion_correction_on;
    int refraction_correction_on;
    int refraction_deconvolution_on;
    int cos_dec_correction_on; // cos dec correction to dec bias rate
    int sync_time_on; // sync TCS computer time to host computer
    double coordinate_epoch;
    double ra_bias_rate;
    double dec_bias_rate;

public:
 
};


class site {
private:
    conf_location location;	// Information about the telescope location
    conf_mount mount;		// Information about the telescope mount
    conf_tcs tcs;		// Information about the telescope mount
    int site_debug;		// Information about whether debugging info
				// should be on or not
public:

    int configure(char *filename);	// load config file

    // accessors
    const int debug()		{ return site_debug; }
    char *name()			{ return location.name; }
    double lat()			{ return location.lat; }
    double lon()			{ return location.lon; }
    double elev()			{ return location.elev; }
    double min_elevation()              { return location.min_elevation;}
    double safe_elevation()             { return location.safe_elevation;}
    double slat()			{ return location.slat; }
    double clat()			{ return location.clat; }
    const int mount_point_timeout()	{ return mount.mount_point_timeout; }
    int com_port()			{ return tcs.com_port; }
    double coordinate_epoch()		{ return tcs.coordinate_epoch; }
    double ra_bias_rate()			{ return tcs.ra_bias_rate; }
    double dec_bias_rate()			{ return tcs.dec_bias_rate; }
    int bias_rate_on()			{ return tcs.bias_rate_on; }
    int fast_slew_on()			{ return tcs.fast_slew_on; }
    int aberration_correction_on()	{ return tcs.aberration_correction_on; }
    int aberration_deconvolution_on()	{ return tcs.aberration_deconvolution_on; }
    int flexture_correction_on()	{ return tcs.flexture_correction_on; }
    int flexture_deconvolution_on()	{ return tcs.flexture_deconvolution_on; }
    int refraction_correction_on()	{ return tcs.refraction_correction_on; }
    int refraction_deconvolution_on()	{ return tcs.refraction_deconvolution_on; }
    int nutation_correction_on()	{ return tcs.nutation_correction_on; }
    int parallax_correction_on()	{ return tcs.parallax_correction_on; }
    int precession_correction_on()	{ return tcs.precession_correction_on; }
    int proper_motion_correction_on()	{ return tcs.proper_motion_correction_on; }
    int cos_dec_correction_on()		{ return tcs.cos_dec_correction_on; }
    int sync_time_on()			{ return tcs.sync_time_on; }

};
#endif // _SITE_H_
