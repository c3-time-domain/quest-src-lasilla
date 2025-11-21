/*
 * $RCSfile: opendome.cc,v $
 * $Revision: 1.8 $
 * $Date: 2010/10/12 14:37:29 $
 * Test program for NEAT telescope mount.
 * Just checks to make sure we can take control
 *****************************************************************************
 * Copyright (c) 2000, California Institute of Technology.
 * U.S. Government sponsorship is acknowledged.
 *****************************************************************************
 *
 * Erik Hovland, JPL
 * $Date: 2010/10/12 14:37:29 $
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
#define COM_PORT 1 /* com_port TCS uses to take remote commands */
#define SERVER_NAME "ls4-workstn"


#define POINT_TIMEOUT 300 /* timeout in seconds for pointing the telescope */

using namespace std;

int main(int argc, char *argv[]) {
    telescope_controller* telescope = new telescope_controller(COM_PORT,POINT_TIMEOUT,SERVER_NAME);

    int status = telescope->take_control();
    switch (status) {
    case 0:
        cerr << "opendome: telescope ready" << endl;
        break;
    case 1:
        cerr << "opendome: could not take control of telescope" << endl;
        break;
    case -1:
        cerr << "opendome: could not communicate with telescope" << endl;
        break;
    default:
        cerr << "opendome: unknown response from telescope: " << status
	     << endl;
        break;
    }
    if (status != 0) {
        cout << "opendome: error taking control " << status << endl;
        exit(1);
    }
    status = telescope->open_dome_shutter();
    switch (status) {
    case 0:
        break;
    case -1:
        cerr << "opendome: could not communicate with telescope" << endl;
        break;
    default:
        cerr << "opendome: unknown response from telescope: " << status
	     << endl;
        break;
    }

    if (status != 0) {
        cout << "opendome: error opening dome " << status << endl;
        exit(1);
    }

    cout << "opendome: dome shutter opened" << endl;

    exit(0);
}

