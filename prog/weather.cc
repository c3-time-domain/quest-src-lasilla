//
// $RCSfile: weather.cc,v $
// $Revision: 1.6 $
// $Date: 2009/07/23 20:57:12 $
//

#include <iostream>
#include <stdlib.h>
#include <telescope_controller.h>
#define COM_PORT 1 /* com_port TCS uses to take remote commands */
#define POINT_TIMEOUT 300 /* timeout in seconds for pointing the telescope */
#define SERVER_NAME "ls4-workstn"

void usage(char *);
EXTERN int tel_drv_debug;

using namespace std;

int main(int argc, char* argv[]) {
    if (argc > 1) {
	usage(argv[0]);
	exit(1);
    }
    struct weather_data cur_wea_stat;
    telescope_controller* tcu = new telescope_controller(COM_PORT,POINT_TIMEOUT,SERVER_NAME);
    cur_wea_stat = tcu->get_weather_data();
    if (cur_wea_stat.wind_direction > 360) {
	cur_wea_stat.wind_direction -= 360;
    }
    cout << "temperature: " << cur_wea_stat.temp
	 << " humidity: " << cur_wea_stat.humidity << " wind speed: "
	 << cur_wea_stat.wind_speed << " wind dir: "
	 << cur_wea_stat.wind_direction << " dew point: "
	 << cur_wea_stat.dew_point << " pressure : " 
	 << cur_wea_stat.pressure << " dome states: " 
         << cur_wea_stat.dome_states << endl;
    delete tcu;
    exit(0);
}

void usage(char* progname) {
    cerr << "Usage: " << progname << endl;
}

