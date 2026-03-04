/*
 * $RCSfile: stow_telescope.cc,v $
 * $Revision: 1.1 $
 * $Date: 2010/10/12 14:37:30 $
 * Test program for NEAT telescope mount.
 * Just checks to make sure we can take control
 *****************************************************************************
 * Copyright (c) 2000, California Institute of Technology.
 * U.S. Government sponsorship is acknowledged.
 *****************************************************************************
 *
 * Erik Hovland, JPL
 * $Date: 2010/10/12 14:37:30 $
 */

#include <iostream.h>
#include <strstream.h>
#include <fstream.h>
#include <iomanip.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#include <syslog.h>

#include <neatconf.h>
#include <telescope_controller.h>
#define COM_PORT 1 /* com_port that TCU used for externa commands */
#define POINT_TIMEOUT 300 /* timeout in seconds for pointing the telescope */
#define SERVER_NAME "ls4-workstn"

int main(int argc, char *argv[]) {
    telescope_controller* telescope = new telescope_controller(COM_PORT,POINT_TIMEOUT,SERVER_NAME);

    int status = telescope->take_control();
    switch (status) {
    case 0:
        cerr << "stow_telescope: telescope ready" << endl;
        break;
    case 1:
        cerr << "stow_telescope: could not take control of telescope" << endl;
        break;
    case -1:
        cerr << "stow_telescope: could not communicate with telescope" << endl;
        break;
    default:
        cerr << "stow_telescope: unknown response from telescope: " << status
	     << endl;
        break;
    }
    if (status != 0) {
        cout << "stow_telescope: could not take control " << status << endl;
        exit(1);
    }
    status = telescope->stow();
    switch (status) {
    case 1:
        break;
    case 0:
        cerr << "stow_telescope: could not stow telescope" << endl;
        break;
    case -1:
        cerr << "stow_telescope: could not communicate with telescope" << endl;
        break;
    default:
        cerr << "stow_telescope: unknown response from telescope: " << status
	     << endl;
        break;
    }

    if (status != 1) {
        exit(1);
    }

    cout << "stow_telescope: telescope now stowed" << endl;

    exit(0);
}

