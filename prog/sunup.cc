//
// sunup
// 
// returns 1 is sun is up, 0 if sun is down, -1 on error.
//
// David Rabinowitz, July 16 2003
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
#include <strstream.h>
//#include <fstream.h>
//#include <iomanip.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>

#include <neattime.h>
#include <dms.h>
#include <site.h>
#include <neatconf.h>
#include <riset.h>
#include <questctl.h>
#include <almanac.h>

#define PROGNAME "sunup" 

// codes for sun up and sun down
#define SUNUP 1
#define SUNDOWN 0
#define ERROR -1


int verbose=0;
int fake_flag = 0;
int init_site(site *mon_site);
int sunup(Almanac *almanac);
int getlst(double *lst);

site site_info;


int main(int argc, char *argv[])
{
    Almanac almanac;
    int sun_pos;  

    if(argc>1 && strcmp(argv[1],"-f") == 0){
      fake_flag = 1;
    }

    if(init_site(&site_info)!=0){
       fprintf(stderr,"could not initialize site_info \n");
       printf("%d\n",ERROR);
       exit(-1);
    }

    if(get_almanac(&almanac,site_info.name())!=0){
       fprintf(stderr,"could not initialize almanac\n");
       printf("%d\n",ERROR);
       exit(-1);
    }

    sun_pos=sunup(&almanac);

    if(sun_pos==-1){
       fprintf(stderr,"could not get sun position\n");
       printf("%d\n",ERROR);
       exit(-1);
    }

    printf("%d\n",sun_pos);

    exit(0);

}
/*****************************************************/

int init_site(site *mon_site)
{

    char confname[NEAT_FILENAMELEN];
    (void) sprintf(confname,"%s/%s",NEAT_SYSDIR,NEAT_CONFIGFILE);
    if (!(mon_site->configure(confname,fake_flag)))
    {
        fprintf(stderr,"init_site: error loading configuration file %s\n",
            confname);
        
        return(-1);
    }
    else {
        if(verbose){
           fprintf(stderr,"init_site:site configured successfully\n");
           fprintf(stderr,"site name: %s\n",mon_site->name());
           fprintf(stderr,"site longitude: %10.6f\n",mon_site->lon());
           fprintf(stderr,"site latitude: %10.6f\n",mon_site->lat());
        }
        return(0);
    }
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

   return(sun_pos);

}

/*********************************************************/

int getlst(double *lst)
{
    
    site mon_site;
    char confname[NEAT_FILENAMELEN];
    (void) sprintf(confname,"%s/%s",NEAT_SYSDIR,NEAT_CONFIGFILE);
    if (!mon_site.configure(confname,fake_flag))
    {
        fprintf(stderr,"getlst: error loading configuration file %s\n",
            confname);
        
        return(-1);
    }
    double now = neat_gettime_utc();
    *lst = uxt_lst(now,mon_site.lon());
    
    if(verbose)fprintf(stderr,"getlst: lst is %10.7f\n",*lst);

    return(0);
}

/*********************************************/

