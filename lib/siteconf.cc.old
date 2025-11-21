// $RCSfile: siteconf.cc,v $
// $Revision: 1.6 $
// $Date: 2009/07/23 20:57:10 $
//
// Routines for configuration file handling and "site" object definition
//
//****************************************************************************
// Copyright (c) 1995, California Institute of Technology.
// U.S. Government sponsorship is acknowledged.
//****************************************************************************
//
// Steve Groom, JPL
// 11/30/95

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>

#include <neatconf.h>
#include <mathconst.h>
#include <site.h>
#include <telescope_controller.h>

// Load a configuration file into a searchable "configfile" object
configfile::configfile(char *filename)
{
    fail = 0;
    headrec = NULL;

    FILE *configfp;
    configfp = fopen(filename,"r");
    if (configfp == NULL)
    {
	fprintf(stderr,"configfile: error opening config file %s\n",filename);
	fail = 1;
	return;
    }

    configrec *tailrec = NULL;
    configrec *rec = NULL;

    int linenum = 0;
    while (!feof(configfp))
    {
	linenum++;
	rec = new configrec;
	if (rec == NULL)
	{
	    fprintf(stderr,"configfile: memory allocation failure\n");
	    fail = 1;
	    (void) fclose(configfp);
	    return;
	}

	// read line from file
	if (fgets(rec->line,sizeof(rec->line),configfp) == NULL)
	{
	    break;	// EOF
	}

	char *nl = strchr(rec->line,'\n');
	if (nl == NULL)
	{
	    // line too long
	    fprintf(stderr,"configfile: line too long at line %d\n",linenum);
	    fail = 1;
	    delete rec;
	    (void) fclose(configfp);
	    return;
	}
	*nl = '\0';

	// truncate line at first comment character
	char *comment = strchr(rec->line,'#');
	if (comment != NULL)
	    *comment = '\0';
	
	// find start of keyword
	rec->keyword = rec->line;
	while (isspace(*rec->keyword))
	    rec->keyword++;

	// ignore blank lines
	if (*rec->keyword == '\0')
	{
	    delete rec;
	    continue;
	}

	// skip past keyword to find valstr
	rec->valstr = rec->keyword;
	while ((*rec->valstr != '\0') && (!isspace(*rec->valstr)))
	    rec->valstr++;
	if (*rec->valstr == '\0')
	{
	    // end of line without seeing a value
	    fprintf(stderr,"configfile: error at line %d\n",linenum);
	    fail = 1;
	    delete rec;
	    (void) fclose(configfp);
	    return;
	}

	*rec->valstr++ = '\0';	// terminate keyword

	// find start of valstr
	while (isspace(*rec->valstr))
	    rec->valstr++;
	if (*rec->valstr == '\0')
	{
	    // end of line without seeing a value string
	    fprintf(stderr,"configfile: error at line %d\n",linenum);
	    fail = 1;
	    delete rec;
	    (void) fclose(configfp);
	    return;
	}

	// if value is a quote-delimited string, extracted the quoted part
	if (*rec->valstr == '"')
	{
	    rec->valstr++;	// past the opening quote

	    char *endstr;
	    endstr = strchr(rec->valstr,'"');
	    if (endstr == NULL)
	    {
		// no closing quote
		fprintf(stderr,"configfile: missing close quote at line %d\n",
		    linenum);
		fail = 1;
		delete rec;
		(void) fclose(configfp);
		return;
	    }
	    *endstr = '\0';	// chop the closing quote
	}

	// check for duplicate keywords
	if (lookup(rec->keyword) != 0)
	{
	    fprintf(stderr,"configfile: duplicate keyword \"%s\" at line %d\n",
		rec->keyword,linenum);
	    fail = 1;
	    delete rec;
	    (void) fclose(configfp);
	    return;
	}

	// append to list
	rec->next = NULL;
	if (headrec == NULL)
	{
	    headrec = tailrec = rec;
	}
	else
	{
	    tailrec->next = rec;
	    tailrec = rec;
	}
    }
    (void) fclose(configfp);
    return;
}

// destructor for a loaded configuration file
configfile::~configfile()
{
    configrec *temprec;
    while(headrec != NULL)
    {
	temprec = headrec;
	headrec = headrec->next;
	temprec->next = NULL;
	delete temprec;
    }
}

// Lookup a keyword in a loaded configuration file.
// Return 1 if found, 0 if not found.  No error message is printed.
int
configfile::lookup(char *keyword)
{
    configrec *rec;
    for(rec = headrec;
	(rec != NULL) && (strcmp(rec->keyword,keyword));
	rec = rec->next);
    if (rec == NULL)
    {
	// not found
	return 0;
    }
    return 1;
}

// Lookup a keyword in a loaded configuration file.
// If found, copy the corresponding value into the supplied string
// and return 1.  If not found, print an error message and return 0.
int
configfile::lookup(char *keyword, char *val)
{
    configrec *rec;

//fprintf(stderr,"configfile::lookup looping to find keyword %s\n",keyword);

    for(rec = headrec;
	(rec != NULL) && (strcmp(rec->keyword,keyword));
	rec = rec->next){
//fprintf(stderr,"configfile::lookup checking %s\n",rec);
    }
    if (rec == NULL)
    {
	// not found
	fprintf(stderr,"configfile: configuration parameter %s not found\n",
	     keyword);
	return 0;
    }


    // found
    /***
    fprintf(stderr,"configfile: configuration parameter %s value \"%s\"\n",
	 rec->keyword,rec->valstr);
    ***/
    strcpy(val,rec->valstr);
    return 1;
}

// Lookup a keyword in a loaded configuration file.
// If found, copy the corresponding integer value into
// the supplied value and return 1.  If not found, or if an integer
// value cannot be parsed from that the value string, returns 0.
int
configfile::lookup(char *keyword, int *val)
{
    char tempstr[64];
    if (!lookup(keyword,tempstr)) return 0;
    if (sscanf(tempstr,"%d",val) != 1) return 0;
    return 1;
}

// Lookup a keyword in a loaded configuration file.
// If found, copy the corresponding floating point (double) value into
// the supplied value and return 1.  If not found, or if a floating
// point value cannot be parsed from that the value string, returns 0.
int
configfile::lookup(char *keyword, double *val)
{
    char tempstr[64];

//fprintf(stderr,"configfile::lookup looking for keyword %s\n",keyword);
    if (!lookup(keyword,tempstr)) return 0;
//fprintf(stderr,"configfile::lookup found keyword %s\n",keyword);
    if (sscanf(tempstr,"%lf",val) != 1) return 0;
    return 1;
}

// Dump a loaded configuration file.
void
configfile::print()
{
    configrec *rec;
    for(rec = headrec; rec != NULL; rec = rec->next)
    {
	printf("%30s ..... %s\n",rec->keyword, rec->valstr);
    }
}

// Configure "conf_location" object
int
conf_location::configure(configfile &conf)
{

    if (!conf.lookup(CONF_SITE_NAME,name)) return 0;
    if (!conf.lookup(CONF_SITE_LAT,&lat)) return 0;
    if (!conf.lookup(CONF_SITE_LON,&lon)) return 0;
    if (!conf.lookup(CONF_SITE_ELEV,&elev)) return 0;
    if (!conf.lookup(CONF_SITE_MIN_ELEVATION,&min_elevation)) return 0;
    if (!conf.lookup(CONF_SITE_SAFE_ELEVATION,&safe_elevation)) return 0;
    slat = sin(RPD*lat);
    clat = cos(RPD*lat);
    return 1;
}

// Configure "conf_mount" object
int
conf_mount::configure(configfile &conf)
{

    if (!conf.lookup(CONF_MOUNT_POINT_TIMEOUT, &mount_point_timeout)) return 0;

    return 1;
}

// Configure "conf_mount" object
int
conf_tcs::configure(configfile &conf)
{

    if (!conf.lookup(TCS_COM_PORT, &com_port) ) return 0;
    if(com_port<1 || com_port > 2){
        fprintf(stderr,"com port inconf file must be 1 or 2. It's not set to %d\n",com_port);
	return -1;
    }
    fprintf(stderr,"conf_tcs: configure: com_port = %d\n",com_port);
    if (!conf.lookup(TCS_COORD_EPOCH, &coordinate_epoch) ) return 0;
    if(coordinate_epoch<1900.0||coordinate_epoch>2100.0){
	coordinate_epoch=DEFAULT_COORDINATE_EPOCH;
        fprintf(stderr,
		"conf_tcs:configure: epoch out of range. Assuming default %f\n",
		coordinate_epoch);
    }
//fprintf(stderr,"conf_tcs::configure: found string %s\n",TCS_COORD_EPOCH);
    if (!conf.lookup(TCS_RA_BIAS_RATE, &ra_bias_rate)) return 0;
    if (!conf.lookup(TCS_DEC_BIAS_RATE, &dec_bias_rate)) return 0;
    if (!conf.lookup(TCS_FAST_SLEW_ON, &fast_slew_on)) return 0;
    if (!conf.lookup(TCS_BIAS_RATE_ON, &bias_rate_on)) return 0;
#ifdef TCS_DEMO
    if (!conf.lookup(TCS_ABERRATION_CORR_ON, &aberration_correction_on)) return 0;
    if (!conf.lookup(TCS_ABERRATION_DECONV_ON, &aberration_deconvolution_on)) return 0;
    if (!conf.lookup(TCS_FLEXTURE_CORR_ON, &flexture_correction_on)) return 0;
    if (!conf.lookup(TCS_FLEXTURE_DECONV_ON, &flexture_deconvolution_on)) return 0;
    if (!conf.lookup(TCS_REFRACTION_CORR_ON, &refraction_correction_on)) return 0;
    if (!conf.lookup(TCS_REFRACTION_DECONV_ON, &refraction_deconvolution_on)) return 0;
    if (!conf.lookup(TCS_NUTATION_CORR_ON, &nutation_correction_on)) return 0;
    if (!conf.lookup(TCS_PARALLAX_CORR_ON, &parallax_correction_on)) return 0;
    if (!conf.lookup(TCS_PRECESSION_CORR_ON, &precession_correction_on)) return 0;
    if (!conf.lookup(TCS_PROPER_MOTION_CORR_ON, &proper_motion_correction_on)) return 0;
    if (!conf.lookup(TCS_COSDEC_CORR_ON, &cos_dec_correction_on)) return 0;
#endif
    if (!conf.lookup(TCS_SYNC_TIME_ON, &sync_time_on)) return 0;

    return 1;
}


// Load configuration file and configure site parameters from it.
// Returns 1 on success, 0 on failure.
int
site::configure(char *filename)
{
    // load config file into array of searchable keyword/value string pairs
    configfile conf(filename);
    if (conf.fail) return 0;

    // grab any site specific configurations
    if (!conf.lookup(CONF_SITE_DEBUG, &site_debug)) return 0;

    // Configure each sub-category
    if (!location.configure(conf)) return 0;
    if (!mount.configure(conf)) return 0;
    if (!tcs.configure(conf)) return 0;

    conf.print();

    return 1;
}
