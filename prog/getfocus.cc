/**************************************************************************
 * $RCSfile: getfocus.cc,v $
 * $Revision: 1.6 $
 * $Date: 2009/07/23 20:57:11 $
 * Author - Erik Hovland
 * Very simple idea, get the current focus offset and return it.
 **************************************************************************/

#include <iostream>
#include <stdlib.h>
#include <telescope_controller.h>
#define COM_PORT 1 /* com_port TCS uses to take remote commands */
#define POINT_TIMEOUT 300 /* timeout in seconds for pointing the telescope */
#define SERVER_NAME "ls4-workstn"


// Usage print out if you don't put in a focus offset at the command line
void usage(char *);

using namespace std;

int main(int argc, char *argv[]) {
	double offset;
	telescope_controller *tcu;

	// make sure the user put nothing on the command line
	if (argc > 1) {
		usage(argv[0]);
		exit(1);
	}

	tcu = new telescope_controller(COM_PORT,POINT_TIMEOUT,SERVER_NAME);
	offset = tcu->get_focus();

	cerr << "current focus setting is " << offset << endl;

	delete tcu;

	exit(0);
}

void usage(char *progname) {
	cerr << "Usage: " << progname << endl;
}
