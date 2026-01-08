/**************************************************************************
 * telescope_status.cc
 * return full telescope status parsed from TCU response line
 **************************************************************************/

#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <telescope_controller.h>
#define COM_PORT 1 /* com port used by TCS for external io */
#define POINT_TIMEOUT 300 /* timeout in seconds for pointing the telescope */
#define SERVER_NAME "ls4-workstn"

//EXTERN int tel_ctlr_debug;
//EXTERN int tel_drv_debug;

// Usage print out if you don't put in a focus offset at the command line
void usage(char *);

int main(int argc, char *argv[]) {
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

    tcu = new telescope_controller(COM_PORT,POINT_TIMEOUT,SERVER_NAME);
    status = tcu->telescope_status();

    if (status!=0){
      fprintf(stderr,"telescope_status: status request failed\n");
    }

    delete tcu;

    exit(0);
}

void usage(char *progname) {
	cerr << "Usage: " << progname << " [-v]" << endl;
}
