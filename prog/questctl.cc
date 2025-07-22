//        
// questctl
// 
// Telescope mount manual control program.
// Allows user to interactively command the telescope mount using socket
// connection.
// Commands which are understood are all those supported by the
// telescope::telinterp() function.
// Auxillary commands also added.
//
// questctl -h lists commands option
//
// questctl -f starts up simulated operations
//
// questctl -d wait for dome to open before receiving commands
//             or exits if sun is up
//
// 
//
//****************************************************************************
// Copyright (c) 1995, CaliFornia Institute of Technology.
// U.S. Government sponsorship is acknowledged.
//****************************************************************************
//
// David Rabinowitz, July 3 2003
// 
 
#define LINUX

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "netio.h"
#include <time.h>
#include <signal.h>
#include <ctype.h>
#ifdef LINUX
#include <string.h>
#else
#include <strings.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
//#include <strstream.h>
//#include <fstream.h>
//#include <iomanip.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>

#include <telmount.h>
#include <neattime.h>
#include <dms.h>
//#include <neatstat.h>
#include <observer.h>
#include <site.h>
#include <neatconf.h>
#include <telescope_controller.h>
#include <riset.h>
#include <questctl.h>
#include <almanac.h>
#include <iostream>
//#include <tm_angles.h>

//#define POLLING_DELAY_USEC 5000000 // delay 5 seconds between polling for sockets command
#define POLLING_DELAY_USEC 100000 // delay 0.1 seconds between polling for sockets command
				// allows enough time for signal to close dome to get through
#define NO_DOME_OPEN // if defined, questctl can not open dome by command
                     // by signal, it can as long as sun is not up

#define PROGNAME "questctl"
#define PID_FILE "questctl.pid"

/* use command and response files to send  commands to questctl without using
   sockets. THis needs to be done when questctl is busy slewing or some other
   time intensive task, and it can not take sockets commands */

#define COMMAND_FILE "questctl.command"
#define RESPONSE_FILE "questctl.response"
#define FAKE_PID_FILE "fake_questctl.pid"
#define FAKE_RESPONSE_FILE "fake_questctl.response"
#define FAKE_STATUS_FILE "fake_tcs.status"
#define QUESTPORT 3911
#define STATUS_CHECK_INTERVAL 10.0 /*seconds between telescope status checks*/
#define COMMAND_LENGTH NEAT_LINELEN
#define RESPONSE_LENGTH NEAT_LINELEN

#define DEFAULT_FOCUS_MM 25.6 /* mm */ /* best setting for UBIR */

// special commands executed without teleinterp subroutines

#define LST_COMMAND "lst"
#define OPENDOME_COMMAND "opendome"
#define CLOSEDOME_COMMAND "closedome"
#define GETFOCUS_COMMAND "getfocus"
#define SETFOCUS_COMMAND "setfocus"
#define SLAVEDOME_COMMAND "slavedome"
#define DOMESTATUS_COMMAND "domestatus"
#define STATUS_COMMAND "status"
#define WEATHER_COMMAND "weather"
#define FILTER_COMMAND "filter"

/* commands understood by telinterp.cc */
#define POINT_COMMAND "pointrd"
#define TRACK_COMMAND "track"
#define POSRD_COMMAND "posrd"
#define STOW_COMMAND "stow"
#define STOPMOUNT_COMMAND "stopmount"
#define STOP_COMMAND "stop"
#define TELESCOPE_STATUS_COMMAND "telescope_status"


/* these are for the get_filter command. Filter indices were assigned by
   John Henning 2004 Jun 2 */

#define NUM_FILTER_INDICES 8
#define FILTER_NAME_LENGTH 16
#define FILTER1 "none"
#define FILTER2 "zzir"
#define FILTER3 "RIBU"
#define FILTER4 "RG610"
#define FILTER5 "unknown"
#define FILTER6 "unknown"
#define FILTER7 "unknown"
#define FILTER8 "clear"


/* these are the status values returned by t->dome_shutter_status*/

#define DOME_OPEN_STATUS ds_open
#define DOME_CLOSED_STATUS ds_closed
#define DOME_UNKNOWN_STATUS ds_stat_unknown
#define DOME_REQUEST_FAIL_STATUS d_comm_failed

/* stares returned by t->status */

#define TRACKING_ON_STATE TELESCOPE_TRACKING_ON_TARGET
#define TRACKING_IN_OFFTARGET_STATE TELESCOPE_TRACKING_OFF_TARGET
#define TRACKING_OFF_STATE TELESCOPE_PARKED

/* these are the state values returned by t->open_dome_shutter and
   t->close_dome_shutter. Note the OPEN and CLOSED states are
   0 and 1, whereas OPEN and CLOSEDD status value are 1 and 0 */

#define DOME_OPEN_STATE 0
#define DOME_CLOSED_STATE 1
#define DOME_UNKNOWN_STATE 2

#define COMMAND_ERROR_REPLY "error"
#define COMMAND_DONE_REPLY "ok"

// command codes
// define codes used to stop telescope if it is about to track into a limit
// or stow telescope and close dome is sun is up.

#define STOP_CODE -1 // stop the drive
#define RUN_CODE 0 // ok to track
#define SHUTDOWN_CODE 1 // stow telescope and close dome
#define CLOSE_CODE 2 // close the dome
#define STOW_CODE 3 // stow the telscope and close the dome
#define OPEN_CODE 4 // open the dome
#define EXIT_CODE 5 // just exit
#define NO_COMMAND_CODE 0

// commands that can be read from command file
#define SHUTDOWN_CODE_STRING "SHUTDOWN"
#define OPEN_CODE_STRING "OPEN"
#define CLOSE_CODE_STRING "CLOSE"
#define EXIT_CODE_STRING "EXIT"
#define STOW_CODE_STRING "STOW"
#define STOP_CODE_STRING "STOP"
#define NOCOMMAND_CODE_STRING "NOCOMMAND"

#define LST_SET_TOLERANCE 0.05 // if telescrope will hit limit in less than
			       // this time, stop the tracking.

// codes for sun up and sun down
#define SUNUP 1
#define SUNDOWN 0
/* variable for simulated operation */

#define DOME_DELAY 3

#define FAKE_LST_OFFSET  06.0 /* increment to add to LST in fake mode */

#define SIGTERM 15 /*kill*/

#define RA_FORMAT "%02d%02d%05.2f"
#define DEC_FORMAT "%1s%02d%02d%05.2f"
#define HA_FORMAT "%1s%02d:%02d:%02d"
#define LST_FORMAT "%02d:%02d:%02d"
#define UT_FORMAT "%02d:%02d:%04.1f"

/* define to make sun always down at startup */
//#define FAKE_SUNDOWN


int fake_site_flag=0;
double my_fake_focus=0;
double my_fake_ra=0.0;
double my_fake_ha=0.0;
double my_fake_dec=0.0;
double my_fake_ut=0.0;
double my_fake_stop_time=0.0;
int my_fake_status=0; /* not tracking */
int my_fake_dome_status=DOME_OPEN_STATUS;
char fake_com1_status[1];


int verbose=1;

void print_help();
int init_command_file();
int get_command_code();
int install_signal_handlers();
void sigterm_handler();
int init_site(site *mon_site);
int sunup(Almanac *almanac);
int stop_telescope();
int stow_telescope();
int shutdown_telescope();
int get_position(double *ra, double *dec, double *ut);
int get_tracking_status(int *tracking_status);
int do_exit(int code);
int process_command(char *command, char *reply, int socfd);
int check_status(Almanac *almanac,int *stop_code);
int domectl(telescope_controller *t, int state);
int getfocus(telescope_controller *t, double *f);
int setfocus(telescope_controller *t, double f);
int getlst(double *lst);
int get_domestatus(telescope_controller *t, char *dome_status_string);
int slavedome(telescope_controller *t, char *dome_status_string);
int get_weather(telescope_controller *t, char *weather_string);
int get_filter(telescope_controller *t, char *filter_string);
int fake_interp(char *command, char *reply);
int record_pid();
int record_response(char *string);
int record_fake_status (int dome_state);
int make_fake_status (int dome_state, TCS_Telemetry *t);
int hr_to_string(double hr, char *string, char *format);
int deg_to_string(double deg, char *string, char *format);

telmount *t;
telescope_controller *telescope;
site site_info;
int command_code=NO_COMMAND_CODE;  // command code is one of the STOP CODEs defined above
long int command_time = 0; // time stamp of last read command in command file

using namespace std;

int main(int argc, char *argv[])
{
    char command[COMMAND_LENGTH],reply[RESPONSE_LENGTH];
    int nfds,listenfd,socfd,count,done; 
    int stop_code;
    fd_set readset; 
    struct timeval timeout;
    struct timeval t1,t2;
    int wait_for_dome=0;
    Almanac almanac;
    double lst; 
//    double current_ra, current_dec;

    if(argc==2&&strstr(argv[1],"h")!=NULL){
       print_help();
       do_exit(0);
    }
    else if(argc==2&&strstr(argv[1],"f")!=NULL){
       fake_site_flag=1;
    }
    else if(argc==2&&strstr(argv[1],"d")!=NULL){
       wait_for_dome=1; 
    }

    if(install_signal_handlers()!=0){
        fprintf(stderr,"questctl: could not install signal handlers. Exitting\n");
        fflush(stderr);
        exit(-1);
    }


    if(init_site(&site_info)!=0){
       fprintf(stderr,"questctl: could not not initialize site_info site\n");
       do_exit(-1);
    }

    if(init_command_file()!=0){
       fprintf(stderr,"questctl: could not not initialize command file\n");
       do_exit(-1);
    }

    if(get_almanac(&almanac,site_info.name())!=0){
       fprintf(stderr,"questctl: could not not initialize almanac\n");
       do_exit(-1);
    }

    *command=0;
    *reply=0;
    if(record_pid()!=0){
       fprintf(stderr,"could not record pid\n");
       exit(-1);
    }
 
    struct  tm  tm;
    fprintf(stderr,"ut_offset is %12.6f\n", UT_OFFSET);
    fprintf(stderr,"current ut is %12.6f\n", get_date_time(&tm));
	     
    if(fake_site_flag){
	    fprintf(stderr,"%s: telescope operation will be simulated\n",argv[0]);
        fflush(stderr);
        getlst(&lst);
        my_fake_ha=0.0;
        my_fake_ra=lst-my_fake_ha;
        my_fake_stop_time=neat_gettime_utc();
        my_fake_status=TRACKING_OFF_STATE;
        strcpy(fake_com1_status,"E");   
        record_fake_status(my_fake_dome_status);
    }
    else{

      // get telescope_controller
      // first initialize telescope to null
      telescope = NULL;
      // and then get telescope controller
      telescope = new telescope_controller(site_info.com_port(),site_info.mount_point_timeout());
      if(telescope==NULL){
	fprintf(stderr,"%s: telescope initialization failed\n",argv[0]);
	if (telescope != NULL) delete telescope;
	do_exit(-1);
      }
      // if wait for dome option is set, wait for dome to open or for command to open dome.
      // If sun is up, however, exit
      if (wait_for_dome){
        while(telescope->dome_shutter_status()!=DOME_OPEN_STATUS){
          if(get_command_code()<0){
            fprintf(stderr,"error getting command code while waiting for dome to open\n");
            do_exit(-1);
          }
          if(sunup(&almanac)==SUNUP){
            fprintf(stderr,"%s: sun is up, shutting down\n",argv[0]);
            do_exit(-1);
          }
	  else if(command_code==EXIT_CODE){
              fprintf(stderr,"%s: signal code is EXIT CODE. Exiting\n",argv[0]);
	      record_response(EXIT_CODE_STRING);
	      do_exit(0);
              command_code=NO_COMMAND_CODE;
	  }
	  else if(command_code==OPEN_CODE){
              fprintf(stderr,"%s: signal code is OPEN CODE. Opening dome\n",argv[0]);
	      record_response(OPEN_CODE_STRING);
              if(domectl(telescope,DOME_OPEN_STATE)!=0){
                fprintf(stderr,"%s: could not open telescope\n",
                        argv[0]);
              }
              command_code=NO_COMMAND_CODE;
	  }
          else {
             fprintf(stderr,"%s: %9.6f Waiting for dome to open. \n",
		argv[0],neat_gettime_utc());
             sleep(60);
          }
        }
        fprintf(stderr,"%s: %9.6f Dome is now open.\n",
		argv[0],neat_gettime_utc());
      }

      // setup mount if possible

      fprintf(stderr,"%s: Initializing telescope mount.\n",argv[0]);

      // first initialize t to NULL
      t = NULL;

      // keep trying to initialize mount. Stop trying at sunrise */

      while(t==NULL){ 
        t = new telmount(fake_site_flag);
        if ((t == NULL) || t->fail)
        {
	   fprintf(stderr,"%s: %9.6f mount initialization failed\n",
		argv[0],neat_gettime_utc());
	   fprintf(stderr,"%s: Will keep trying until sunrise\n",argv[0]);
	   if (t != NULL) {
             delete t;
             t=NULL;
           }
        }
        if(sunup(&almanac)==SUNUP){
            fprintf(stderr,"%s: sun is up, shutting down\n",argv[0]);
            do_exit(-1);
        }
        if(t==NULL)sleep(120);
        if(get_command_code()<0){
            fprintf(stderr,"error getting command code while waiting for dome to open\n");
            do_exit(-1);
        }
	if(command_code==EXIT_CODE){
              fprintf(stderr,"%s: signal code is EXIT CODE. Exiting\n",argv[0]);
	      record_response(EXIT_CODE_STRING);
	      do_exit(0);
              command_code=NO_COMMAND_CODE;
	}
      }
      fprintf(stderr,"%s: %9.6f Mount Initialized.\n",
		argv[0],neat_gettime_utc());

    }

    // main processing loop
    done=0;
    while(!done)
    {
      if(fake_site_flag&&my_fake_status==TRACKING_ON_STATE){
          double now = neat_gettime_utc();
          my_fake_ha=my_fake_ha+
                        (now-my_fake_stop_time)/3600.0;
          my_fake_stop_time=now;
      }

      // initialize received command and reply to NULL */
      *command=0;
      *reply=0;

      // Set up socket for control connections.

      // initialize listenfd and socfd to -1

      listenfd=-1;
      socfd=-1;

      if(verbose){fprintf(stderr,"%s: setting up listen socket\n",argv[0]);}

      listenfd = network_listen(QUESTPORT);
      if (listenfd == -1) {
        fprintf(stderr,"%s: error creating listener socket\n",argv[0]);
        do_exit(-1);
      }

      if(verbose){fprintf(stderr,
	"%s: polling for activity on listener socket\n",argv[0]);}

      /* While there is no activity, poll the status of the telescope
         and stow or stop as neccessary.
      */
      timeout.tv_sec=0;
      timeout.tv_usec=0;
      FD_ZERO(&readset);
      FD_SET(listenfd,&readset);
      nfds = select(listenfd+1,&readset,NULL,NULL,&timeout);
      gettimeofday(&t1,NULL);
      gettimeofday(&t2,NULL);
      while(nfds==0){
        usleep (POLLING_DELAY_USEC);
        if((t2.tv_sec-t1.tv_sec)>STATUS_CHECK_INTERVAL){
           if(verbose){
             fprintf(stderr,"%s: checking telescope status %ld\n",argv[0],
			t2.tv_sec);
           }
           t1=t2;
           if(check_status(&almanac,&stop_code)!=0){
	     fprintf(stderr,"%s: unable to check telescope status %ld\n",argv[0],
			t2.tv_sec);
             do_exit(-1);
           }
           
           if(stop_code==STOP_CODE){
              fprintf(stderr,"%s: check_status returns stop_code STOP %ld\n",
			argv[0],t2.tv_sec);
              fprintf(stderr,"%s: stopping telescope %ld\n",argv[0],t2.tv_sec);
              if(stop_telescope()!=0){
                fprintf(stderr,"%s: could not stop telescope %ld\n",
                        argv[0],t2.tv_sec);
                //do_exit(-1);
              }
              stop_code=RUN_CODE;
           }
           else if (stop_code==SHUTDOWN_CODE){
              fprintf(stderr,"%s: check_status returns stop_code SHUTDOWN\n",argv[0]);
              fprintf(stderr,"%s: shutting down telescope %ld\n",
                       argv[0],t2.tv_sec);
              if(shutdown_telescope()!=0){
                fprintf(stderr,"%s: could not shutdown telescope %ld\n",
			argv[0],t2.tv_sec);
                do_exit(-1);
              }
           }

           if(get_command_code()<0){
               fprintf(stderr,"error getting command code while checking status\n");
               do_exit(-1);
           }

           if(command_code==CLOSE_CODE){
              record_response(CLOSE_CODE_STRING);
              fprintf(stderr,"%s: signal code has been set to CLOSE_CODE %ld\n",
			argv[0],t2.tv_sec);
              if(stow_telescope()!=0){
                fprintf(stderr,"%s: could not stow telescope %ld\n",
                        argv[0],t2.tv_sec);
              }
              command_code=NO_COMMAND_CODE;
           }
           else if(command_code==OPEN_CODE){
              record_response(OPEN_CODE_STRING);
              fprintf(stderr,"%s: signal code has been set to OPEN_CODE %ld\n",
			argv[0],t2.tv_sec);
              fprintf(stderr,"%s: opening telescope %ld\n",argv[0],t2.tv_sec);
              if(domectl(telescope,DOME_OPEN_STATE)!=0){
                fprintf(stderr,"%s: could not open telescope %ld\n",
                        argv[0],t2.tv_sec);
              }
              command_code=NO_COMMAND_CODE;
           }
           else if(command_code==STOW_CODE){
              record_response(STOW_CODE_STRING);
              fprintf(stderr,"%s: signal code has been set to STOW_CODE %ld\n",
			argv[0],t2.tv_sec);
              fprintf(stderr,"%s: stowing telescope %ld\n",argv[0],t2.tv_sec);
              if(stow_telescope()!=0){
                fprintf(stderr,"%s: could not stow telescope %ld\n",
                        argv[0],t2.tv_sec);
              }
              command_code=NO_COMMAND_CODE;
           }
           else if(command_code==EXIT_CODE){
              record_response(EXIT_CODE_STRING);
              fprintf(stderr,"%s: signal code has been set to EXIT_CODE %ld\n",
			argv[0],t2.tv_sec);
              fprintf(stderr,"%s: exiting now %ld\n",argv[0],t2.tv_sec);
              command_code=NO_COMMAND_CODE;
 	      exit(0);
           }
           else if(command_code==SHUTDOWN_CODE){
              record_response(SHUTDOWN_CODE_STRING);
              fprintf(stderr,"%s: signal code has been set to EXIT_CODE %ld\n",
			argv[0],t2.tv_sec);
              fprintf(stderr,"%s: exiting now %ld\n",argv[0],t2.tv_sec);
              if(shutdown_telescope()!=0){
                fprintf(stderr,"%s: could not shutdown telescope %ld\n",
			argv[0],t2.tv_sec);
                do_exit(-1);
              }
              command_code=NO_COMMAND_CODE;
           }
        }
        FD_SET(listenfd,&readset);
        nfds = select(listenfd+1,&readset,NULL,NULL,&timeout);
        gettimeofday(&t2,NULL);
     }

      // now there is activity (or else an error polling the listener socket*/
      gettimeofday(&t2,NULL);

      if ((nfds == -1) && (errno != EINTR)) {
	    fprintf(stderr,"%s: error in select %ld\n",argv[0],t2.tv_sec);
      }
      else if (nfds > 0) {

	  // there is activity   

          if(verbose){
             fprintf(stderr,"%s: activity sensed on listener socket %ld\n",
		argv[0],listenfd);
             fprintf(stderr,"%s: nfds = %d\n", argv[0],nfds);
          }

	  // Process the activity
	  // Test the connection channel first

	  if ((listenfd != -1) && FD_ISSET(listenfd,&readset)) {

             if(verbose){
                 fprintf(stderr,"%s: accepting connection %ld\n",
			argv[0],t2.tv_sec);
             }

             if ((socfd = network_accept(listenfd)) == -1){
		fprintf(stderr,"%s: error in accept %ld\n",
			argv[0],t2.tv_sec);
                do_exit(-1);
             }
             else{
		if(verbose){
                   fprintf(stderr,"%s: client connected %ld\n",
			argv[0],t2.tv_sec);
                }
		/*FD_SET(socfd,&readset);*/
		FD_CLR(listenfd,&readset);
		(void) close(listenfd);
		listenfd = -1;
             }
          }
      
          if(verbose){
             fprintf(stderr,"%s: reading command %ld\n",argv[0],t2.tv_sec);
          }
	  for (int i=0;i< COMMAND_LENGTH;i++){*(command+i)=0;}

          if ((count=read(socfd,(u_char *)command,COMMAND_LENGTH)) == -1) {
	     fprintf(stderr,"%s: socket read error %ld\n",argv[0],t2.tv_sec);
	     close(socfd);
	     do_exit(-1);
          }
          else if (count==0){
	     fprintf(stderr,"%s: EOF on socket read %ld\n",argv[0],t2.tv_sec);
	     close(socfd);
          }
          else{
            if(verbose){
              fprintf(stderr,"%s: received %s %ld\n",argv[0],command,t2.tv_sec);
              fprintf(stderr,"%s: processing command\n",argv[0]);
            }
            if(strcmp(command,"shutdown")!=0){
              if(process_command(command,reply,socfd)==-1){
                gettimeofday(&t2,NULL);
	        fprintf(stderr,"%s: error processing command %ld\n",
			argv[0],t2.tv_sec);
	      }
              gettimeofday(&t2,NULL);
              if(verbose){
                fprintf(stderr,"%s: done processing command %ld\n",
			argv[0],t2.tv_sec);
              }
            }
          }
          if(socfd!=-1)close(socfd);
      }

      if(strcmp(command,"shutdown")==0){
         if(verbose){fprintf(stderr,"%s: shutting down %ld\n",
		argv[0],t2.tv_sec);}
         done=1;
      }
    }
  
    if(socfd!=-1)close(socfd);
    if(listenfd!=-1)close(listenfd);

    do_exit(0);

}
/*****************************************************/

int init_site(site *mon_site)
{

    char confname[NEAT_FILENAMELEN];
    sprintf(confname,"%s/%s",NEAT_SYSDIR,NEAT_CONFIGFILE);

    if(verbose){
       fprintf(stderr,"init_site: configuring from file %s with fake_flag %d\n",confname, fake_site_flag);
    }

    if (!(mon_site->configure(confname,fake_site_flag)))
    {
        fprintf(stderr,"init_site: error loading configuration file %s with fake flag %d\n",
            confname, fake_site_flag);
        
        return(-1);
    }
    else {
        if(verbose){
           fprintf(stderr,"init_site:site configured successfully\n");
           fprintf(stderr,"site name: %s\n",mon_site->name());
           fprintf(stderr,"site longitude: %10.6f\n",mon_site->lon());
           fprintf(stderr,"site latitude: %10.6f\n",mon_site->lat());
           fprintf(stderr,"site min_elevation: %10.6f\n",mon_site->min_elevation());
           fprintf(stderr,"site safe_elevation: %10.6f\n",mon_site->safe_elevation());
        }
        return(0);
    }
}
/*****************************************************/

int do_exit(int code)
{
  
   if(!fake_site_flag){
       if(telescope!=NULL)delete telescope;
       if(t!=NULL)delete t;
   }
   exit(code);
}

/*****************************************************/

int get_tracking_status(int *tracking_status)
{
   fprintf(stderr,"get_tracking_status: reading telescope tracking status\n");

   if(fake_site_flag){
     *tracking_status=my_fake_status;
   }
   else { 
     *tracking_status=telescope->tracking_status();
     if(*tracking_status<0){
       fprintf(stderr,"get_tracking_status: could not read telescope position\n");
       return(-1);
     }
   }

   return(0);
}

/*****************************************************/

int get_position(double *ra, double *dec, double *ut)
{
   fprintf(stderr,"get_postion: reading telescope position\n");
   double lst;

   if(fake_site_flag){
      if(getlst(&lst)!=0){  
        fprintf(stderr,"check_status: could not read lst\n");
        return(-1);
      }
     *ra=lst-my_fake_ha;
     my_fake_ra=*ra;
     *dec=my_fake_dec;
     my_fake_ut=neat_gettime_utc();
     *ut=my_fake_ut;
   }
   else {
     if(t->getpos(ra,dec,ut)!=0){
       fprintf(stderr,"get_postion: could not read telescope position\n");
       return(-1);
     }
   }

   return(0);
}

/*****************************************************/

int stop_telescope()
{

   fprintf(stderr,"stop_telescope: stopping telescope\n");

   if(fake_site_flag){
     my_fake_status=0;
   }
   else {
     if(t->stop()!=0){
       fprintf(stderr,"stop_telescope: could not stop telescope\n");
       return(-1);
     }
   }


   return(0);
}

/*****************************************************/

int stow_telescope()
{

   int success=0;

   fprintf(stderr,"stow_telescope: stowing telescope\n");

   if(fake_site_flag){
     fprintf(stderr,"stow_telescope: fake stowing telescope. Sleeping 10 seconds...\n");
     sleep(10);
     fprintf(stderr,"stow_telescope: fake stowing telescope. Done sleeping 10 seconds.\n");
     my_fake_status=0;
     my_fake_dome_status=DOME_CLOSED_STATE;
     record_fake_status(0);
     success=1;
   }
   else {
     if(t->stow()!=1){
       fprintf(stderr,"stow_telescope: could not stow telescope\n");
       success=-1; 
     }
     if(domectl(telescope,DOME_CLOSED_STATE)!=0){
         fprintf(stderr,"stow_telescope: could not close dome\n");
         success=-1; 
     }
   }

   return(success);
}

/*****************************************************/

int shutdown_telescope()
/* call telmount stow function. This moves the telescope to zenith, waits for it
  to get there. Then moves the telescope to stow position, homes and closes the dome
  all at the same time. Then it waits for the stow to complete. THen it issues another
  close dome command and waits for the dome to close.
*/
{
   int status;

   fprintf(stderr,"shutdown_telescope: shutting down telescope\n");

   status=0;

   if(stow_telescope()!=0){  
       fprintf(stderr,"shutdown_telescope: could not stow telescope\n");
       status=-1; 
   }
   if(domectl(telescope,DOME_CLOSED_STATE)!=0){
       fprintf(stderr,"shutdown_telescope: could not close dome\n");
       status=-1; 
   }

   return(status);

}
/*****************************************************/

/* read the telescope state (tracking or no), dome state (open or closed),
   ra, dec, lst, and determine the lst when the current telescope position
   will set below min_elevation. If the telescope is just about to
   go below the minumum elevation (lst_set-lst<LST_SET_TOLERANCE) then
   return with stop_code=STOP_CODE to indicate that a stop should be issued.
*/

int check_status(Almanac *almanac, int *stop_code)
{
   double lst,ra,dec,ut;
   double lst_rise,lst_set, az_rise, az_set;
   double time_left=0.0; 
   int rs_status;
   int dome_status,tracking_status;
   char dome_status_string[1024];

   if(fake_site_flag){*stop_code=RUN_CODE;return(0);}
   // check if sun is up

   if(sunup(almanac)==SUNUP){
     fprintf(stderr,"check_status: sun is up, setting shutdown code\n");
     *stop_code=SHUTDOWN_CODE;
     return(0);
   }

   // get lst

   if(verbose){
     fprintf(stderr,"check_status: getting lst\n");
   }

   if(getlst(&lst)!=0){  
     fprintf(stderr,"check_status: could not read lst\n");
     return(-1);
   }

   if(verbose){
     fprintf(stderr,"checking telescope status\n");
   }

   // get dome status

   dome_status=get_domestatus(telescope,dome_status_string);

   // check two more times if dome_status is DOME_REQUEST_FAIL_STATUS
   int n=0;
   while(n++<3&&dome_status==DOME_REQUEST_FAIL_STATUS){
       sleep(3);
       dome_status=get_domestatus(telescope,dome_status_string);
   }

   if(dome_status==DOME_REQUEST_FAIL_STATUS){
      fprintf(stderr,"check_status: could not get dome status\n");
      return(-1);
   }

   // get ra,dec

   if(get_position(&ra,&dec,&ut)!=0){
     fprintf(stderr,"check_status: could not read ra,dec\n");
     return(-1);
   }

   // get tracking status

   if(get_tracking_status(&tracking_status)!=0){
     fprintf(stderr,"check_status: could not read tracking status\n");
     return(-1);
   }

   if(verbose){
     fprintf(stderr,
       "check_status:\nra=%9.6f\ndec=%9.6f\nlst=%9.6f\ntracking:%d\ndome:%d\n",
	 ra,dec,lst,tracking_status,dome_status);
   }

   // if tracking is off, return with stop_code = RUN_CODE

   if(tracking_status==TRACKING_OFF_STATE){
     *stop_code=RUN_CODE;
     return(0);
   }

   // tracking is on.
   // check when current telescope position rises and sets, if at all.
   // Set the stop code to STOP_CODE if the object is below the horizon
   // or if the position will set in less than LSET_SET_TOLERANCE hours.
   

   // get the lst when the current position rises and sets above/below
   // min_elevation

   double min_elevation = site_info.min_elevation();
   riset(ra, dec, site_info.lat(), -min_elevation, &lst_rise, &lst_set,
	&az_rise, &az_set, &rs_status);

   if(verbose){
     if (rs_status == 1)	 // never rises
     {
        fprintf(stderr,"check_status: this position never rises\n");
     }
     else if (rs_status == -1)	// never sets
     {
        fprintf(stderr,"check_status: this position never sets\n");
     }
     else if (rs_status == 2)	// internal math error
     {
        fprintf(stderr,"check_status: riset math error\n");
     }
     else
     {
	fprintf(stderr,"check_status: this position rises at lst %9.6f\n",
		lst_rise);
	fprintf(stderr,"check_status: this position sets at lst %9.6f\n",
		lst_set);
     }
   }

   // evaluate stop_code

   // default code is to run
   *stop_code=RUN_CODE;

   if (rs_status == 1)	 // never rises
   {
        *stop_code=STOP_CODE;
   }
   else if (rs_status == -1)	// never sets
   {
        *stop_code=RUN_CODE;
   }
   else if (rs_status == 2)	// internal math error
   {
        *stop_code=STOP_CODE;
   }
   else{

        // check if current position is below MINIMUM_ELEVATION

        if(lst_rise<lst_set&&lst<lst_rise){ 

          // since lst_rise<lst_set there is no 24-hour boundary to worry
          // about here. If lst < lst_rise then the the position has not yet
          // risen above MINUMUM_ELEVATION.

          fprintf(stderr,"check_status: position below minimum elevation\n");
          *stop_code=STOP_CODE;
        }
        else if(lst_rise<lst_set&&lst>lst_set){   

          // Again, no 24-hour boundary problem. If lst>lst_set then the
          // the position has already set below MINUMUM_ELEVATION.

          fprintf(stderr,"check_status: position below minimum elevation\n");
          *stop_code=STOP_CODE;
        }
        else if (lst_rise>lst_set&&lst<lst_rise&&lst>lst_set){

          // Since lst_rise > lst_set, there must be a 24-hour boundary 
          // between lst_rise and lst_set. If lst is both less than lst_rise
          // and greater than lst_set, then it is not betweem lst_rise and
          // lst_set and the position must be below MINUMUM_ELEVATION.

          fprintf(stderr,"check_status: position below minimum elevation\n");
          *stop_code=STOP_CODE;
        }
        else{ // get time left until position sets

          // If none of the above conditions apply, then the position is
          // currently above MINUMUM_ELEVATION. Now determine how much time
          // is left before the position sets.

          time_left=lst_set-lst;

          // take care of 24-hour boundary problem

          if(time_left<0.0)time_left=time_left+24.0;

          // set stop_code to STOP_CODE if time_left < LST_SET_TOLERANCE

          if(time_left<LST_SET_TOLERANCE){
            fprintf(stderr,
              "check_status: position will set in less than %f hours\n",
               LST_SET_TOLERANCE);
            *stop_code=STOP_CODE;
          }
       }
   }

   if(*stop_code==STOP_CODE){
     fprintf(stderr,"check_status: setting stop code to STOP\n");
   }

   return(0);
}
/*****************************************************/

// check is sun is up

int sunup(Almanac *almanac)
{
double lst,lst_sunrise,lst_sunset;
int sun_pos;

   if(verbose){
     fprintf(stderr,"sunup: checking if sun is up\n");
   }

   // get lst_sunrise and lst_sunset from almanac structure

   lst_sunrise=almanac->lst_sunrise;
   lst_sunset=almanac->lst_sunset;


   // get lst

   if(verbose){
     fprintf(stderr,"sunup: getting LST\n");
   }

   if(getlst(&lst)!=0){  
     fprintf(stderr,"sunup: could not read lst\n");
     return(-1);
   }

   // assume sun is below horizon

   sun_pos=SUNDOWN;

   // now check if  sun has not yet set or has risen 

   if ( lst_sunrise > lst_sunset ){ // no 24-hour boundary
     if ( lst > lst_sunrise || lst < lst_sunset ) { // it's daylight
        sun_pos=SUNUP;
     }
   }
   else{ // 24-hour boundary between lst_sunset and lst_sunrise
     if (lst > lst_sunrise && lst < lst_sunset) { // it's daylight
        sun_pos=SUNUP;
     }
   }

   if(verbose){
     if(sun_pos==SUNUP){
       fprintf(stderr,"sunup: sun is up\n");
     }
     else{
       fprintf(stderr,"sunup: sun is down\n");
     }
   }

#ifdef FAKE_SUNDOWN
   fprintf(stderr,"sunup but allowing observations anyhow\n");
   sun_pos=SUNDOWN;
#endif
   return(sun_pos);

}

/*****************************************************/

int process_command(char *command, char *reply, int socfd)
{
    double f_mm=DEFAULT_FOCUS_MM;
    int f_steps=telescope->convert_focus_to_steps(f_mm);
    double lst=0.0;
    char dummy_string[COMMAND_LENGTH];
    int dome_status=DOME_CLOSED_STATUS;
    int tracking_status=TRACKING_OFF_STATE;  
    char dome_status_string[1024];
    char weather_string[1024];
    char filter_string[1024];
    
      if (strlen(command)>1) {
        if(strstr(command,CLOSEDOME_COMMAND)!=NULL){
            if(domectl(telescope,DOME_CLOSED_STATE)==0){
                sprintf(reply,"%s\n",COMMAND_DONE_REPLY);
            }
            else{
                sprintf(reply,"%s\n",COMMAND_ERROR_REPLY);
            }
            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
        }
        else if(strstr(command,OPENDOME_COMMAND)!=NULL){
            if(domectl(telescope,DOME_OPEN_STATE)==0){
                sprintf(reply,"%s\n",COMMAND_DONE_REPLY);
            }
            else{
                sprintf(reply,"%s\n",COMMAND_ERROR_REPLY);
            }
            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
        }
        else if(strstr(command,GETFOCUS_COMMAND)!=NULL){
            if(getfocus(telescope,&f_mm)==0){  
                sprintf(reply,"%s %8.4f\n",COMMAND_DONE_REPLY,f_mm);
            }
            else{
                sprintf(reply,"%s\n",COMMAND_ERROR_REPLY);
            }
            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
        }
        else if(strstr(command,SETFOCUS_COMMAND)!=NULL){
            sscanf(command,"%s %lf",dummy_string,&f_mm);
            f_steps=telescope->convert_focus_to_steps(f_mm);
            if(f_steps<MIN_FCS||f_steps>MAX_FCS){
               fprintf(stderr,
                "process_command: requested focus %8.3f ( %d steps) out of range: %12.0f to %12.0f\n",
					f_mm,f_steps,MIN_FCS,MAX_FCS);
               sprintf(reply,"%s\n",COMMAND_ERROR_REPLY);
            }
            else {
               fprintf(stderr,"process_command: setting focus to  %8.3f (%d steps)\n",f_mm,f_steps);
              if(setfocus(telescope,f_mm)==0){  
                sprintf(reply,"%s\n",COMMAND_DONE_REPLY);
              }
              else{
                sprintf(reply,"%s\n",COMMAND_ERROR_REPLY);
              }
            }
            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
        }
        else if(strstr(command,LST_COMMAND)!=NULL){
            if(getlst(&lst)==0){  
                sprintf(reply,"%s %8.4f\n",COMMAND_DONE_REPLY,lst);
            }
            else{
                sprintf(reply,"%s\n",COMMAND_ERROR_REPLY);
            }
            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
        }
        else if(strstr(command,DOMESTATUS_COMMAND)!=NULL){
            dome_status=get_domestatus(telescope, dome_status_string);
            if(dome_status<0){
                sprintf(reply,"%s %s\n",COMMAND_ERROR_REPLY,dome_status_string);
            }
            else {
                sprintf(reply,"%s %s\n",COMMAND_DONE_REPLY,dome_status_string);
            }
            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
        }
        else if(strstr(command,STATUS_COMMAND)!=NULL){
            if(get_tracking_status(&tracking_status)!=0){
                fprintf(stderr,"process_command: can't get tracking status\n");
                tracking_status=-1;
            }

            if(fake_site_flag){
              if(tracking_status<0){
                sprintf(reply,"%s %d fake operations\n",COMMAND_ERROR_REPLY,tracking_status);
              }
              else {
                sprintf(reply,"%s %d fake_operations\n",COMMAND_DONE_REPLY,tracking_status);
              }
            }
            else{
              if(tracking_status<0){
                sprintf(reply,"%s %d \n",COMMAND_ERROR_REPLY,tracking_status);
              }
              else {
                sprintf(reply,"%s %d \n",COMMAND_DONE_REPLY,tracking_status);
              }
            }

            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
        }
        else if(strstr(command,SLAVEDOME_COMMAND)!=NULL){
            if(slavedome(telescope, dome_status_string)!=0){
                sprintf(reply,"%s %s\n",COMMAND_ERROR_REPLY,dome_status_string);
            }
            else {
                sprintf(reply,"%s %s\n",COMMAND_DONE_REPLY,dome_status_string);
            }
            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
        }
        else if(strstr(command,WEATHER_COMMAND)!=NULL){
            if(get_weather(telescope, weather_string)!=0){
                sprintf(reply,"%s %s\n",COMMAND_ERROR_REPLY,weather_string);
            }
            else {
                sprintf(reply,"%s %s\n",COMMAND_DONE_REPLY,weather_string);
            }
            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
        }
        else if(strstr(command,FILTER_COMMAND)!=NULL){
            if(get_filter(telescope, filter_string)!=0){
                sprintf(reply,"%s %s\n",COMMAND_ERROR_REPLY,filter_string);
            }
            else {
                sprintf(reply,"%s %s\n",COMMAND_DONE_REPLY,filter_string);
            }
            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
        }
        else{
          if(fake_site_flag){
            fake_interp(command,reply);
            if(write_loop(socfd,(u_char *)reply,strlen(reply))==-1){
                fprintf(stderr,
		 "process_command: error sending reply to socket\n");
                return(-1);
            }
          }
          else{  
            //command[sizeof(command)-1] = '\0';
	    t->interp(socfd,command);
	  }
        }
      }

      return(0);
}


/*********************************************/

void print_help()
{
   printf("usage: questctl [-f for simulation] [-d to wait for dome open]\n");
   printf("telescope commands:\n");
   printf("  %s\n",LST_COMMAND);
   printf("  %s\n",OPENDOME_COMMAND);
   printf("  %s\n",CLOSEDOME_COMMAND);
   printf("  %s\n",GETFOCUS_COMMAND);
   printf("  %s\n",SETFOCUS_COMMAND);
   printf("  %s\n",DOMESTATUS_COMMAND);
   printf("  %s\n",SLAVEDOME_COMMAND);
   printf("  pointrd\n");
   printf("  track\n");
   printf("  posrd\n");
   printf("  stow\n");
   printf("  stopmount\n");
   printf("  stop\n");
   printf("  status\n");

   return;

}
/*********************************************/

int fake_interp(char *command, char *reply)
{
    char string[COMMAND_LENGTH];
    double ut_time,lst;

    ut_time=999.000;

    if(strstr(command,POINT_COMMAND)!=NULL){
       cerr << "fake point command" << endl;
       sleep(10);
       sscanf(command,"%s %lf %lf",string,&my_fake_ra,&my_fake_dec);
       getlst(&lst);
       my_fake_ha=lst-my_fake_ra;
       my_fake_status=TRACKING_ON_STATE;
       sprintf(reply,COMMAND_DONE_REPLY);
    }
    else if(strstr(command,TRACK_COMMAND)!=NULL){
       cerr << "fake track command" << endl;
       sleep(10);
       sscanf(command,"%s %lf %lf",string,&my_fake_ra,&my_fake_dec);
       getlst(&lst);
       my_fake_ha=lst-my_fake_ra;
       my_fake_status=TRACKING_ON_STATE;
       sprintf(reply,COMMAND_DONE_REPLY);
    }
    else if(strstr(command,POSRD_COMMAND)!=NULL){
       cerr << "fake posrd command" << endl;
       getlst(&lst);
       my_fake_ra=lst-my_fake_ha;
       sprintf(reply,"%s %9.6f %9.6f %9.6f",COMMAND_DONE_REPLY,my_fake_ra,my_fake_dec,neat_gettime_utc());
    }
    else if(strstr(command,TELESCOPE_STATUS_COMMAND)!=NULL){
       cerr << "fake telescope_status command" << endl;
       getlst(&lst);
       my_fake_ra=lst-my_fake_ha;
       sprintf(reply,"%s %s",COMMAND_DONE_REPLY,"FAKE MODE : NO STATUS AVAILABLE");
    }
    else if(strstr(command,STOW_COMMAND)!=NULL){
       cerr << "fake stow command" << endl;
       sleep(30);
       my_fake_status=TRACKING_OFF_STATE;
       getlst(&lst);
       my_fake_ha=0.0;
       my_fake_ra=lst;
       my_fake_dec=+47.0;
       my_fake_dome_status = DOME_CLOSED_STATUS;
       record_fake_status(0);
       sprintf(reply,COMMAND_DONE_REPLY);
    }
    else if(strstr(command,STOPMOUNT_COMMAND)!=NULL){
       sleep(1);
       my_fake_status=TRACKING_OFF_STATE;
       my_fake_stop_time=neat_gettime_utc();
       sprintf(reply,COMMAND_DONE_REPLY);
    }
    else if(strstr(command,STOP_COMMAND)!=NULL){
       cerr << "fake stop command" << endl;
       sleep(1);
       my_fake_stop_time=neat_gettime_utc();
       my_fake_status=TRACKING_OFF_STATE;
       sprintf(reply,COMMAND_DONE_REPLY);
    }
#if  0
    else if(strstr(command,STATUS_COMMAND)!=NULL){
       cerr << "fake status command" << endl;
       sleep(1);
       sprintf(reply,"%s %d fake operations",COMMAND_DONE_REPLY,my_fake_status);
    }
#endif
    else{
       sprintf(reply,"%s unrecognized command",COMMAND_ERROR_REPLY);
    }
    
    return(0);
}


/*********************************************/

int get_filter(telescope_controller *telescope, char *filter_string)
{

        int filter_id,index;
        unsigned short i;
        char filter[NUM_FILTER_INDICES*FILTER_NAME_LENGTH];

        if (fake_site_flag){
           sprintf(filter_string,"filter: FAKE");
           return(0);
        }

        strcpy(filter+0,FILTER1);
        strcpy(filter+(FILTER_NAME_LENGTH*1),FILTER2);
        strcpy(filter+(FILTER_NAME_LENGTH*2),FILTER3);
        strcpy(filter+(FILTER_NAME_LENGTH*3),FILTER4);
        strcpy(filter+(FILTER_NAME_LENGTH*4),FILTER5);
        strcpy(filter+(FILTER_NAME_LENGTH*5),FILTER6);
        strcpy(filter+(FILTER_NAME_LENGTH*6),FILTER7);
        strcpy(filter+(FILTER_NAME_LENGTH*7),FILTER8);
        

        filter_id=telescope->get_filter();
        if (filter_id==-1){
            printf("%s: couldn't get filter id", "getfocus");
            return(-1); 
        }

        index=-1;
        i=filter_id;
        while(i>0){
	   index++;
	   i=i>>1;
        }

        if (index<=0||index>=NUM_FILTER_INDICES){
           sprintf(filter_string,"filter: error\n");
        }
        else{
           sprintf("filter_string,filter: %s\n",filter+(FILTER_NAME_LENGTH*index));
        }

	return(0);
}

/*********************************************/

int get_weather(telescope_controller *telescope, char *weather_string)
{

    struct weather_data cur_wea_stat;

    if(fake_site_flag){
      cur_wea_stat.temp=0.0;
      cur_wea_stat.humidity=0.0;
      cur_wea_stat.wind_speed=0.0,
      cur_wea_stat.wind_direction=0.0;
      cur_wea_stat.dew_point=0.0;
    }
    else{
      cur_wea_stat = telescope->get_weather_data();
      if (cur_wea_stat.wind_direction > 360) {
	cur_wea_stat.wind_direction -= 360;
      }
    }
    sprintf(weather_string,
      "temp: %5.1f humidity: %6.2f wind speed: %5.1f  wind dir: %5.1f dew point %5.1f",
      cur_wea_stat.temp,cur_wea_stat.humidity,cur_wea_stat.wind_speed,
      cur_wea_stat.wind_direction, cur_wea_stat.dew_point);

    cout << "temp: " << cur_wea_stat.temp
	 << " humidity: " << cur_wea_stat.humidity << " wind speed: "
	 << cur_wea_stat.wind_speed << " wind dir: "
	 << cur_wea_stat.wind_direction << " dew point: "
	 << cur_wea_stat.dew_point << endl;

    return(0);
}

/*********************************************/

int slavedome(telescope_controller *telescope, char *dome_status_string)
{
    int status;

    if(fake_site_flag){
      cerr << "fake slavedome command" << endl;
      status=0;
    }
    else {
      status = telescope->slave_dome();
    }

    switch (status) {
    case 0:
        sprintf(dome_status_string,"dome slaved to telescope");
        break;
    case -1:
        sprintf(dome_status_string,"could not communicate with telescope");
        break;
    default:
        sprintf(dome_status_string,"unknown response from telescope");
        break;
    }

    return(status);

}
/*********************************************/

int get_domestatus(telescope_controller *telescope, char *status_str)
{
    int status;
    
    if(fake_site_flag){
      cerr << "fake domestatus command" << endl;
      status=my_fake_dome_status;
    }
    else {
      status = telescope->dome_shutter_status();
    }

    switch(status) {
	case DOME_OPEN_STATUS:
	    strcpy(status_str, "dome open");
	    break;
	case DOME_CLOSED_STATUS:
	    strcpy(status_str, "dome closed");
	    break;
	case DOME_UNKNOWN_STATUS:
	    strcpy(status_str, "dome status unknown");
	    break;
	case DOME_REQUEST_FAIL_STATUS:
	    strcpy(status_str, "dome status request failed");
	    break;
	default:
	    char value[128];
	    sprintf(value, "%d", status);
	    strcpy(status_str, value);
	    break;
    }

    cerr << "current dome status is: " << status_str << endl;

    return(status);
}

/*********************************************/

int getlst(double *lst)
{
    
    char confname[NEAT_FILENAMELEN];
    site mon_site;

    mon_site=site_info;
    (void) sprintf(confname,"%s/%s",NEAT_SYSDIR,NEAT_CONFIGFILE);
    double now = neat_gettime_utc();
    *lst = uxt_lst(now,mon_site.lon());
    
    if(fake_site_flag){
      cerr << "fake lst command" << endl;
      fflush(stderr);
      *lst=*lst+FAKE_LST_OFFSET;
      if(*lst>24.0){*lst=(*lst)-24.0;}
    }
    
    fprintf(stderr,"getlst: lst is %10.7f\n",*lst);
    fflush(stderr);

    return(0);
}

/*********************************************/
/* set telescope focus in steps */
int setfocus(telescope_controller *telescope,double f)
{

    if(fake_site_flag){
       my_fake_focus=f;
       cerr << "fake setfocus command" << endl;
       cerr << "focus offset changed to " << f << "mm" << endl;
    }
    else{
       if (telescope->set_focus(f) == -1) {
	  cerr << "setting focus failed" << endl;
	  return(-1);
       }

      f = telescope->get_focus();
      cerr << "focus offset changed to " << f << "mm" << endl;
    }
    return(0);
}

/*********************************************/
/* get telescope focus in steps */
int getfocus(telescope_controller *telescope, double *f)
{

   if(fake_site_flag){
     *f=my_fake_focus;
     cerr << "fake getfocus command" << endl;
     cerr << "current focus setting is " << *f << endl;
   }
   else{
	*f = telescope->get_focus();
	cerr << "current focus setting is " << *f << endl;
   }

   return(0);
}

/*********************************************/

int domectl(telescope_controller *t,int state){

    int status;

    if(state!=DOME_OPEN_STATE&&state!=DOME_CLOSED_STATE){
       cout << "domectl: unrecognized dome state" << endl;
       return(1);
    }

#ifdef NO_DOME_OPEN
    // can't open dome unless command came from command_code
    if(!fake_site_flag&&state==DOME_OPEN_STATE&&command_code!=OPEN_CODE){
       cerr << "domectl: dome open not allowed " << endl;
       return(1);
    }
#endif

    if(fake_site_flag){
      cerr << "fake domectl command" << endl;
      sleep(DOME_DELAY);
      status=0;
    }
    else{
      status = t->take_control();
    }
    
    switch (status) {
    case 0:
        cout << "domectl: telescope ready" << endl;
        break;
    case 1:
        cout << "domectl: could not take control of telescope" << endl;
        break;
    case -1:
        cout << "domectl: could not communicate with telescope" << endl;
        break;
    default:
        cout << "domectl: unknown response from telescope: " << status
	     << endl;
        break;
    }
    if (status != 0) {
        return(1);
    }

    if(state==DOME_OPEN_STATE){
       if(fake_site_flag){
         cerr << "fake open_dome command" << endl;
         status=0;
         my_fake_dome_status=DOME_OPEN_STATUS;
         record_fake_status(my_fake_dome_status);
       }
       else{
         status = t->open_dome_shutter();
       }
    }
    else if (state==DOME_CLOSED_STATE){
       if(fake_site_flag){
         cerr << "fake open dome command" << endl;
         status=0;
         my_fake_dome_status=DOME_CLOSED_STATUS;
         record_fake_status(my_fake_dome_status);
       }
       else{
         status = t->close_dome_shutter();
       }
    }

    if(status==0){
        if(state==DOME_CLOSED_STATE){
            cout << "domectl: dome shutter closed" << endl;
        }
        else if(state==DOME_OPEN_STATE){
            cout << "domectl: dome shutter opened" << endl;
        }
        return(0);
    }
    else{
        if (status==-1){
            cout << "domectl: could not communicate with telescope" << endl;
        }
        else{
            cout << "domectl: unknown response from telescope: " << status
	         << endl;
        }
        if(state==DOME_CLOSED_STATE){
            cout << "domectl: could not close dome" << endl;
        }
        else if(state==DOME_OPEN_STATE){
            cout << "domectl: could not open dome" << endl;
        }
        return(1);
        
    }

}

/***************************************************/

int install_signal_handlers()
{

    if(signal(SIGTERM,(sighandler_t)sigterm_handler)==SIG_ERR){
      fprintf(stderr,
          "install_signal_handler: ERROR installing sigterm_handler\n");
      fflush(stderr);
      return(-1);
    }

    return(0);
}

/*********************************************/


void sigterm_handler()
{
   fflush(stdout);
   fflush(stderr);

   fprintf(stderr,"sigterm_handler: terminate signal received\n");
   fflush(stderr);

   command_code=EXIT_CODE;

}

/*********************************************/

int record_pid()
{
   FILE *output;
   
   if(fake_site_flag){
     output=fopen(FAKE_PID_FILE,"w");
     if(output==NULL){
        fprintf(stderr,"record_pid: could not open file %s for writing\n",FAKE_PID_FILE);
        return(-1);
     }
   }
   else{
     output=fopen(PID_FILE,"w");
     if(output==NULL){
        fprintf(stderr,"record_pid: could not open file %s for writing\n",PID_FILE);
        return(-1);
     }
   }

   fprintf(output,"%d",getpid());
   fclose(output);

   return(0);
}
/*********************************************/

int init_command_file()
{
  FILE *output;
  char string[1024];

  output=fopen(COMMAND_FILE,"w");
  if(output==NULL){
    fprintf(stderr,"init_command_file: error opening file %s\n",COMMAND_FILE);
    return(-1);
  }

  fprintf(output,NOCOMMAND_CODE_STRING);

  fclose(output);

  return(0);
}
/*********************************************/

/* read latest command from COMMAND_FILE */
int get_command_code()
{
  FILE *input;
  char string[1024],command_string[256];
  long int time;

  if(verbose){
    fprintf(stderr,"getting command code\n");
    fflush(stderr);
  }

  input=fopen(COMMAND_FILE,"r");
  if(input==NULL){
    fprintf(stderr,"get_command_code: error opening file %s\n",COMMAND_FILE);
    command_code=NO_COMMAND_CODE;
    /*return(-1);*/
  }

  else if(fgets(string,1024,input)==NULL){
    fprintf(stderr,"get_command_code: error reading file %s\n",COMMAND_FILE);
    command_code=NO_COMMAND_CODE;
    fclose(input);
  }
  else{
    sscanf(string,"%s %ld",command_string,&time);
    if(strstr(command_string,NOCOMMAND_CODE_STRING)!=NULL){
          command_code=NO_COMMAND_CODE;
    }
    else if (time > command_time){
       if(strstr(command_string,STOP_CODE_STRING)!=NULL){
          command_code=STOP_CODE;
       }
       else if (strstr(command_string,SHUTDOWN_CODE_STRING)!=NULL){
          command_code=SHUTDOWN_CODE;
       }
       else if (strstr(command_string,STOW_CODE_STRING)!=NULL){
          command_code=STOW_CODE;
       }
       else if (strstr(command_string,CLOSE_CODE_STRING)!=NULL){
          command_code=CLOSE_CODE;
       }
       else if (strstr(command_string,OPEN_CODE_STRING)!=NULL){
          command_code=OPEN_CODE;
       }
       else if (strstr(command_string,EXIT_CODE_STRING)!=NULL){
          command_code=EXIT_CODE;
       }
       else{
          fprintf(stderr,
		"get_command_code: unrecognized command in command file: %s\n",
		command_string);
          command_code=NO_COMMAND_CODE;
       }
       command_time=time;
    }
    fclose(input);
  }

  if(verbose){
    fprintf(stderr,"command code is %d\n",command_code);
    fflush(stderr);
  }

  return(0);
}

/*********************************************/

int record_response(char *string)
{
   FILE *output;
   struct timeval t;
   
   if(fake_site_flag){
     output=fopen(FAKE_RESPONSE_FILE,"w");
     if(output==NULL){
         fprintf(stderr,"record_response: could not open file %s for writing\n",FAKE_RESPONSE_FILE);
         return(-1);
     }
   }
   else{
     output=fopen(RESPONSE_FILE,"w");
     if(output==NULL){
         fprintf(stderr,"record_response: could not open file %s for writing\n",RESPONSE_FILE);
         return(-1);
     }
   }
   gettimeofday(&t,NULL);
   fprintf(output,"%s %d\n",string,t.tv_sec);
   fclose(output);

   return(0);
}
/*********************************************/

int record_fake_status (int dome_state)
{
   FILE *output;
   struct timeval t;
  
   output=fopen(FAKE_STATUS_FILE,"w");
   if(output==NULL){
      fprintf(stderr,"record_fake_status: could not open file %s for writing\n",FAKE_STATUS_FILE);
      return(-1);
   }
   gettimeofday(&t,NULL);
   TCS_Telemetry tel;
   make_fake_status(dome_state,&tel);
   fprintf(output,"%s",(char *)&tel);
   fclose(output);

   return(0);
}
/*********************************************/

int make_fake_status (int dome_state, TCS_Telemetry *t)
{
   for (int i=0;i<sizeof(TCS_Telemetry);i++){
      strcpy(((char *)t)+i," ");
   }
   sprintf(t->motion_status,"%1d",my_fake_status);
   strcpy(t->wobble_status," ");
   strcpy(t->dummy1," ");
   hr_to_string(my_fake_ra,t->ra,RA_FORMAT);
   strcpy(t->dummy2," ");
   deg_to_string(my_fake_dec,t->dec,DEC_FORMAT);
   strcpy(t->dummy3,"  ");
   hr_to_string(my_fake_ha,t->ha,HA_FORMAT);
   strcpy(t->dummy4," ");
   float my_fake_lst = my_fake_ra + my_fake_ha;
   hr_to_string(my_fake_lst,t->lst,LST_FORMAT);
   strcpy(t->dummy5," ");
   strcpy(t->alt,"+90.0");
   strcpy(t->dummy6," ");
   strcpy(t->azim,"+000.0");
   strcpy(t->dummy7," ");
   strcpy(t->secz,"01.00");
   strcpy(t->dummy8," ");
   if (strcmp(fake_com1_status,"E")==0)
	  strcpy(fake_com1_status,"e");
   else if (strcmp(fake_com1_status,"e")==0)
	  strcpy(fake_com1_status,"E");
   strcpy(t->com1,fake_com1_status); /* alternates E,e for each command successfully executed */
   strcpy(t->com2," "); 
   strcpy(t->com3," "); 
   strcpy(t->com4," "); 
   strcpy(t->com5," "); 
   strcpy(t->com6," "); 
   strcpy(t->com7," "); 
   strcpy(t->com8," "); 
   strcpy(t->dummy9," ");
   strcpy(t->ra_limit," "); /*1 when in limit */
   strcpy(t->dec_limit," "); /*1 when in limit */
   strcpy(t->horiz_limit," ");/* 1 when in limit */
   strcpy(t->drive_status,"1"); /* 1 when enabled, blank otherwise */
   strcpy(t->epoch,"2000.000");
   strcpy(t->dummy10," ");
   strcpy(t->jd,"2450000.00");
#ifdef DEMO_TCS
   strcpy(t->reserved,"     ");
#else
   strcpy(t->dummy11," ");
   strcpy(t->channel,"1");
   strcpy(t->dummy12," ");
   sprintf(t->focus_pos,"%+05d",my_fake_focus);
   strcpy(t->dummy13," ");
   strcpy(t->dome_err_deg,"+000.0");
   strcpy(t->dummy14," ");
   hr_to_string(my_fake_ut,t->ut_time,UT_FORMAT); 
   strcpy(t->dummy," ");
   sprintf(t->dome_state,"%1d",dome_state);
   strcpy(t->reserved,"                          ");
#endif
   sprintf(t->cr,"\r");
   sprintf(t->lf,"\n");
   return (0);
}

int hr_to_string(double hr, char *string, char *format)
{
   double hr_s;
   int hr_h,hr_m;
   int sign;
   if (hr < 0 ){
      hr = -hr;
      sign = -1;
   }
   else{
      sign = 1;
   }
   hr_h = hr;
   hr_m = (hr - hr_h)*60.0;
   hr_s = (hr - hr_h - (hr_m/60.0))*3600.0;
   if(strstr(format,"%1s")== NULL)
        sprintf(string,format,hr_h,hr_m,hr_s);
   else if (sign<0)
        sprintf(string,format,"-",hr_h,hr_m,hr_s);
   else
      sprintf(string,format,"+",hr_h,hr_m,hr_s);
    
   return (0);
}

int deg_to_string(double deg, char *string, char *format)
{
   double deg_s;
   int deg_d,deg_m;
   int sign;
   if (deg < 0 ){
      deg = -deg;
      sign = -1;
   }
   else{
      sign = 1;
   }
   deg_d = deg;
   deg_m = (deg - deg_d)*60.0;
   deg_s = (deg - deg_d - (deg_m/60.0))*3600.0;
   if(strstr(format,"%1s")== NULL)
       sprintf(string,format,deg_d,deg_m,deg_s);
   else if (sign<0)
       sprintf(string,format,"-",deg_d,deg_m,deg_s);
   else
       sprintf(string,format,"+",deg_d,deg_m,deg_s);

   return (0);
}


