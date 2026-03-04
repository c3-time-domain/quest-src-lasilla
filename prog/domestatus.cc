/**************************************************************************
 * $RCSfile: domestatus.cc,v $
 * $Revision: 1.6 $
 * $Date: 2009/07/23 20:57:10 $
 * Author - Erik Hovland
 * Very simple idea, get the current focus offset and return it.
 **************************************************************************/

#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <telescope_controller.h>
#include <unistd.h>

#define COM_PORT 1 /* com port used by TCS for external io */
#define POINT_TIMEOUT 300 /* timeout in seconds for pointing the telescope */
#define SERVER_NAME "ls4-workstn"

//EXTERN int tel_ctlr_debug;
//EXTERN int tel_drv_debug;

// Usage print out if you don't put in a focus offset at the command line
void usage(char *);

int main(int argc, char *argv[]) {
    openlog("domestatus", LOG_PID, LOG_LOCAL0);
    int status;
    telescope_controller *tcu;

    // make sure the user put nothing on the command line
    if (argc > 2) {
		usage(argv[0]);
		exit(1);
    }

    if (argc == 2) {
	// check for verbosity flag
	int c;
	while ((c = getopt(argc, argv, "v")) != EOF) {
	    switch (c) {
		case 'v':
		    //tel_ctlr_debug = 1;
		    //tel_drv_debug = 1;
		    break;
		default:
		    usage(argv[0]);
		    exit(1);
	    }
	}
    }

    char server_name[1024];
    strcpy(server_name,SERVER_NAME);
    tcu = NULL;
    tcu = new telescope_controller(COM_PORT,POINT_TIMEOUT,server_name);

    fprintf(stderr,"domestatus: tcu->tcs_prog_name: %s\n",tcu->tcs_prog_name);

    status = tcu->dome_shutter_status();
    char* status_str = new char[128];
    switch(status) {
	case 1:
	    strcpy(status_str, "dome open");
	    break;
	case 0:
	    strcpy(status_str, "dome closed");
	    break;
	case -1:
	    strcpy(status_str, "dome/telescope mismatch");
	    break;
	case -2:
	    strcpy(status_str, "dome status unknown");
	    break;
	case -8:
	    strcpy(status_str, "dome status request failed");
	    break;
	default:
	    char value[128];
	    sprintf(value, "%d", status);
	    strcpy(status_str, value);
	    break;
    }

    cerr << "current dome status is: " << status_str << endl;
    cout <<  status_str << endl;

    delete tcu;
    delete status_str;

    exit(0);
}

void usage(char *progname) {
	cerr << "Usage: " << progname << " [-v]" << endl;
}
