//
// $RCSfile: telinterp.cc,v $
// $Revision: 1.8 $
// $Date: 2010/10/12 14:37:27 $
// Telescope mount command interpreter
// Parses command lines (strings) and performs operations on
// telescope mount.
//
// The commands known to the interpreter are defined in the static array
// "cmdfunctab".
//
//****************************************************************************
// Copyright (c) 1995, California Institute of Technology.
// U.S. Government sponsorship is acknowledged.
//****************************************************************************
//
// Steve Groom, JPL
// 7/27/95

#define VERBOSE		// enable debugging messages

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#include <neatconf.h>
#include <netio.h>
#include <telmount.h>
#include <tm_angles.h>
#include <neattime.h>

#define HARRISMOUNT

// declarations of function table entries
//static int cmd_pospd(telmount *t, int fd_out, char *command);
static int cmd_posrd(telmount *t, int fd_out, char *command);
static int cmd_getfocus(telmount *t, int fd_out, char *command);
//static int cmd_pointpd(telmount *t, int fd_out, char *command);
static int cmd_pointrd(telmount *t, int fd_out, char *command);
static int cmd_track(telmount *t, int fd_out, char *command);
static int cmd_focus(telmount *t, int fd_out, char *command);
static int cmd_stow(telmount *t, int fd_out, char *command);
static int cmd_stopmount(telmount *t, int fd_out, char *command);
static int cmd_stop(telmount *t, int fd_out, char *command);
//static int cmd_dome(telmount *t, int fd_out, char *command);
static int cmd_status(telmount *t, int fd_out, char *command);
static int cmd_telescope_status(telmount *t, int fd_out, char *command);
//static int cmd_dump(telmount *t, int fd_out, char *command);

// Function table containing commands and routines to implement each.
static struct tabent {
    char	*name;
    int		(*func)(telmount *t, int fd_out, char *command);
} cmdfunctab[] = {
    {"posrd",	cmd_posrd},
    {"getfocus",cmd_getfocus},
    {"pointrd",	cmd_pointrd},
    {"track",	cmd_track},
    {"focus",	cmd_focus},
    {"stow",	cmd_stow},
    {"stopmount",cmd_stopmount}, // must be before "stop"
    {"stop",	cmd_stop},
    {"status",	cmd_status},
    {"telescope_status",	cmd_telescope_status},
};
static int cmdtabsize = sizeof(cmdfunctab)/sizeof(cmdfunctab[0]);

// pointrd
// Point telescope to specified RA/Dec (J2000) coordinates.
// RA is specified in fractional hours (0.0 - 24.0)
// Dec is specified in fractional degrees (-90.0 - +90.0)
// keep tracking off after move (use track command to
// turn tracking on after move )
static int
cmd_pointrd(telmount *t, int fd_out, char *command)
{
    char replybuf[NEAT_LINELEN];
    double ra;
    double dec;
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: %s\n",command);
#endif // VERBOSE
    if (sscanf(command,"pointrd%lf%lf",&ra,&dec) != 2)
    {
	sprintf(replybuf, "%s can't parse command \"%s\"\n",TELMOUNTCL_MSG_ERROR,command);
	fprintf(stderr,"telmount:interp: %s",replybuf);
    }
    else
    {
	int result = t->point(ra,dec,TELMOUNT_POINT);
#ifdef VERBOSE
	fprintf(stderr,"telmount:interp: command returned %d\n",result);
#endif // VERBOSE
	if (result == -1)
	{
	    sprintf(replybuf,"%s command \"%s\" failed\n",
		TELMOUNTCL_MSG_ERROR, command);
	    fprintf(stderr,"telmount:interp: %s",replybuf);
	}
	else if (result == -2)
	{
	    sprintf(replybuf,"%s unsafe command \"%s\"\n",
		TELMOUNTCL_MSG_BADVAL, command);
	    fprintf(stderr,"telmount:interp: %s",replybuf);
	}
	else
	    sprintf(replybuf,"%s\n",TELMOUNTCL_MSG_OK);
    }
    if (write_loop(fd_out,(u_char *) replybuf,strlen(replybuf)) == -1)
    {
	fprintf(stderr,
	    "telmount:interp: error writing reply to command \"%s\"\n",
	    command);
	return -1;
    }
    return 0;
}

// track
// Point telescope to specified RA/Dec (J2000) coordinates,
// and leave telescope in sidereal tracking mode (i.e. moving),
// tracking that point in the sky. 
// this duplicates the pointrd command
static int
cmd_track(telmount *t, int fd_out, char *command)
{
    char replybuf[NEAT_LINELEN];
    double ra;
    double dec;
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: %s\n",command);
#endif // VERBOSE
    if (sscanf(command,"track%lf%lf",&ra,&dec) != 2)
    {
	sprintf(replybuf, "%s can't parse command \"%s\"\n",
	    TELMOUNTCL_MSG_ERROR, command);
	fprintf(stderr,"telmount:interp: %s",replybuf);
    }
    else
    {
	int result = t->point(ra,dec,TELMOUNT_SIDTRACK);
#ifdef VERBOSE
	fprintf(stderr,"telmount:interp: command returned %d\n",result);
#endif // VERBOSE
	if (result == -1)
	{
	    sprintf(replybuf,"%s command \"%s\" failed\n",
		TELMOUNTCL_MSG_ERROR,command);
	    fprintf(stderr,"telmount:interp: %s",replybuf);
	}
	else if (result == -2)
	{
	    sprintf(replybuf,"%s unsafe command \"%s\"\n",
		TELMOUNTCL_MSG_BADVAL, command);
	    fprintf(stderr,"telmount:interp: %s",replybuf);
	}
	else
	    sprintf(replybuf,"%s\n",TELMOUNTCL_MSG_OK);
    }
    if (write_loop(fd_out,(u_char *) replybuf,strlen(replybuf)) == -1)
    {
	fprintf(stderr,
	    "telmount:interp: error writing reply to command \"%s\"\n",
	    command);
	return -1;
    }
    return 0;
}

// focus
// Set focus to specified position.
// GEODSS focus range is 0 - 2047.
static int
cmd_focus(telmount *t, int fd_out, char *command)
{
    char replybuf[NEAT_LINELEN];
    double fset;
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: %s\n",command);
#endif // VERBOSE
    if (sscanf(command,"focus%lf",&fset)!= 1)
    {
	sprintf(replybuf, "%s can't parse command \"%s\"\n",TELMOUNTCL_MSG_ERROR,command);
	fprintf(stderr,"telmount:interp: %s",replybuf);
    }
    else
    {
	int result = t->focus(fset);
// #ifdef VERBOSE
	fprintf(stderr,"telmount:interp: command returned %d\n",result);
// #endif // VERBOSE
	if (result == -1)
	{
	    sprintf(replybuf,"%s command \"%s\" failed\n",
		TELMOUNTCL_MSG_ERROR,command);
	    fprintf(stderr,"telmount:interp: %s",replybuf);
	}
	else if (result == -2)
	{
	    sprintf(replybuf,"%s value out of range, command \"%s\"\n",
		TELMOUNTCL_MSG_BADVAL, command);
	    fprintf(stderr,"telmount:interp: %s",replybuf);
	}
	else
	    sprintf(replybuf,"%s\n",TELMOUNTCL_MSG_OK);
    }
    if (write_loop(fd_out,(u_char *) replybuf,strlen(replybuf)) == -1)
    {
	fprintf(stderr,
	    "telmount:interp: error writing reply to command \"%s\"\n",
	    command);
	return -1;
    }
    return 0;
}

// posrd
// Return time and current mount position in ra/dec J2000 coords
static int
cmd_posrd(telmount *t, int fd_out, char *command)
{
    char replybuf[NEAT_LINELEN];
    double ra;
    double dec;
    double uxtime;
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: %s\n",command);
#endif // VERBOSE
    int result = t->getpos(&ra,&dec,&uxtime);
#ifdef VERBOSE
    fprintf(stderr,"ra = %f  dec = %f\n",ra,dec);
#endif
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: command returned %d\n",result);
#endif // VERBOSE
    if (result == -1)
    {
	sprintf(replybuf,"%s command \"%s\" failed\n",TELMOUNTCL_MSG_ERROR,command);
	fprintf(stderr,"telmount:interp: %s",replybuf);
    }
    else
	sprintf(replybuf,"%s %f %f %f\n",TELMOUNTCL_MSG_OK,ra,dec,uxtime);
    if (write_loop(fd_out,(u_char *) replybuf,strlen(replybuf)) == -1)
    {
	fprintf(stderr,
	    "telmount:interp: error writing reply to command \"%s\"\n",
	    command);
	return -1;
    }
    return 0;
}

// getfocus
// Retrieve current focus position
static int
cmd_getfocus(telmount *t, int fd_out, char *command)
{
    char replybuf[NEAT_LINELEN];
    double fset;
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: %s\n",command);
#endif // VERBOSE
    int result = t->getfocus(&fset);
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: command returned %d\n",result);
#endif // VERBOSE
    if (result == -1)
    {
	sprintf(replybuf,"%s command \"%s\" failed\n",TELMOUNTCL_MSG_ERROR,command);
	fprintf(stderr,"telmount:interp: %s",replybuf);
    }
    else
	sprintf(replybuf,"%s %f\n",TELMOUNTCL_MSG_OK,fset);
    if (write_loop(fd_out,(u_char *) replybuf,strlen(replybuf)) == -1)
    {
	fprintf(stderr,
	    "telmount:interp: error writing reply to command \"%s\"\n",
	    command);
	return -1;
    }
    return 0;
}

// stop
// Stop the telescope if tracking.  Leaves mount drivers turned on,
// and telescope pointed whereever it currently is.
static int
cmd_stop(telmount *t, int fd_out, char *command)
{
    char replybuf[NEAT_LINELEN];
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: %s\n",command);
#endif // VERBOSE
    int result = t->stop();
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: command returned %d\n",result);
#endif // VERBOSE
    if (result == -1)
    {
	sprintf(replybuf,"%s command \"%s\" failed\n",TELMOUNTCL_MSG_ERROR,command);
	fprintf(stderr,"telmount:interp: %s",replybuf);
    }
    else
	sprintf(replybuf,"%s\n",TELMOUNTCL_MSG_OK);
    if (write_loop(fd_out,(u_char *) replybuf,strlen(replybuf)) == -1)
    {
	fprintf(stderr,
	    "telmount:interp: error writing reply to command \"%s\"\n",
	    command);
	return -1;
    }
    return 0;
}

// stopmount
// Stop the telescope mount, disengage mount drivers.
// *** Caution, this allows mount to move freely,    ***
// *** (i.e. if it's off balance, it can tip over) . ***
static int
cmd_stopmount(telmount *t, int fd_out, char *command)
{
    char replybuf[NEAT_LINELEN];
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: %s\n",command);
#endif // VERBOSE
    int result = t->stopmount();
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: command returned %d\n",result);
#endif // VERBOSE
    if (result == -1)
    {
	sprintf(replybuf,"%s command \"%s\" failed\n",TELMOUNTCL_MSG_ERROR,command);
	fprintf(stderr,"telmount:interp: %s",replybuf);
    }
    else
	sprintf(replybuf,"%s\n",TELMOUNTCL_MSG_OK);
    if (write_loop(fd_out,(u_char *) replybuf,strlen(replybuf)) == -1)
    {
	fprintf(stderr,
	    "telmount:interp: error writing reply to command \"%s\"\n",
	    command);
	return -1;
    }
    return 0;
}


// stow
// Stow the telescope by moving it to the predetermined "stow" position.
static int
cmd_stow(telmount *t, int fd_out, char *command)
{
    char replybuf[NEAT_LINELEN];
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: %s\n",command);
#endif // VERBOSE
    int result = t->stow();
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: command returned %d\n",result);
#endif // VERBOSE
    if (result == -1)
    {
	sprintf(replybuf,"%s command \"%s\" failed\n",TELMOUNTCL_MSG_ERROR,command);
	fprintf(stderr,"telmount:interp: %s",replybuf);
    }
    else
	sprintf(replybuf,"%s\n",TELMOUNTCL_MSG_OK);

    if (write_loop(fd_out,(u_char *) replybuf,strlen(replybuf)) == -1)
    {
	fprintf(stderr,
	    "telmount:interp: error writing reply to command \"%s\"\n",
	    command);
	return -1;
    }
    return 0;
}

// status
static int
cmd_status(telmount *t, int fd_out, char *command)
{
    char replybuf[NEAT_LINELEN];
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: %s\n",command);
#endif // VERBOSE
    int result = t->tracking_status();
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: command returned %d\n",result);
#endif // VERBOSE
    if (result == -1)
    {
	sprintf(replybuf,"%s command \"%s\" failed\n",TELMOUNTCL_MSG_ERROR,command);
	fprintf(stderr,"telmount:interp: %s",replybuf);
    }
    else
	sprintf(replybuf,"%s\n",TELMOUNTCL_MSG_OK);
    if (write_loop(fd_out,(u_char *) replybuf,strlen(replybuf)) == -1)
    {
	fprintf(stderr,
	    "telmount:interp: error writing reply to command \"%s\"\n",
	    command);
	return -1;
    }
    return 0;
}

// telescope_status
static int
cmd_telescope_status(telmount *t, int fd_out, char *command)
{
    char replybuf[NEAT_LINELEN];
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: %s\n",command);
#endif // VERBOSE
    int result = t->telescope_status();
#ifdef VERBOSE
    fprintf(stderr,"telmount:interp: command returned %d\n",result);
#endif // VERBOSE
    if (result == -1)
    {
	sprintf(replybuf,"%s command \"%s\" failed\n",TELMOUNTCL_MSG_ERROR,command);
	fprintf(stderr,"telmount:interp: %s",replybuf);
    }
    else
	sprintf(replybuf,"%s\n",TELMOUNTCL_MSG_OK);
    if (write_loop(fd_out,(u_char *) replybuf,strlen(replybuf)) == -1)
    {
	fprintf(stderr,
	    "telmount:interp: error writing reply to command \"%s\"\n",
	    command);
	return -1;
    }
    return 0;
}

// telmount::interp - telescope mount command interpreter
// Process supplied command line and operate on mount.
// Any response to the command is written to the file
// descriptor fd_out.  Returns 0 on success,
// and -1 on mount error.
int
telmount::interp(int fd_out, char *command)
{
    if ((command == NULL) || (strlen(command) == 0))
	return 0;	// null command handled trivially

    int i;
    for (i=0;
	((i<cmdtabsize) &&
	    strncmp(command,cmdfunctab[i].name,strlen(cmdfunctab[i].name)));
	i++);
    if (i>=cmdtabsize)
    {
	fprintf(stderr,"telmount::interp unknown command %s\n",command);
	return -1;	// ??
    }
     return cmdfunctab[i].func(this,fd_out,command);
	}
