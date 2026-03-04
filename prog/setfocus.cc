/**************************************************************************
 * $RCSfile: setfocus.cc,v $
 * $Revision: 1.6 $
 * $Date: 2009/07/23 20:57:11 $
 *
 * Author - Erik Hovland
 * This utility is the main utility for dealing with the focus controller
 * on the MSSS telescope. It uses a very simple interface. Run the command
 * with one argument which is a decimal value of how much you want to move
 * the focus in millimeters and the command will attempt to move it that
 * far.
 **************************************************************************/

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <telescope_controller.h>
#define COM_PORT 1 /* com_port TCS uses to take remote commands */
#define POINT_TIMEOUT 300 /* timeout in seconds for pointing the telescope */
#define SERVER_NAME "ls4-workstn"

// Usage print out if you don't put in a focus setting at the command line
void usage(char *);

using namespace std;

int main(int argc, char *argv[]) {
    double f_mm; 
    int f_steps;
    telescope_controller *tcu;

    // make sure the user put the new f_mm on the command line
    if (argc < 2) {
	usage(argv[0]);
	exit(1);
    }

    f_mm = atof(argv[1]);
    cerr << "changing focus to: " << f_mm << endl;
    tcu = new telescope_controller(COM_PORT,POINT_TIMEOUT,SERVER_NAME);
    f_steps=tcu->convert_focus_to_steps(f_mm);
    if(f_steps<MIN_FCS||f_steps>MAX_FCS){
        fprintf(stderr,
                "setfocus: requested focus %8.3f ( %d steps) out of range: %12.0f to %12.0f\n",
                            f_mm,f_steps,MIN_FCS,MAX_FCS);
    }
    else {
        fprintf(stderr,"setfocus: setting focus to  %8.3f (%d steps)\n",f_mm,f_steps);
        if (tcu->set_focus(f_mm) == -1) {
   	    cerr << "setting focus failed" << endl;
        }
        else{
            f_mm = tcu->get_focus();
            cerr << "focus f_mm changed to " << f_mm << "mm" << endl;
        }
    }


    delete tcu;

    exit(0);
}

void usage(char *progname) {
    cerr << "Usage: " << progname << " f_mm_value" << endl;
}
