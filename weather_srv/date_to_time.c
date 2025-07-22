/* date_to_time.c

    convert "broken down time" (year, month, day, hour, minute, sec) to
time since 00:00:00  Jan 1 1970 UTC. Assume date is UT date 

  DLR 2009 Jul 8
*/

#include <stdio.h>
#include <time.h>


main(int argc, char **argv)
{
    char *year,*month,*day,*hour,*min,*sec;
    time_t t;
    struct tm my_tm;

    if(argc!=7){
        fprintf(stderr,"syntax: date_to_time yyyy mm dd hh mm ss\n");
        exit(-1);
     }

     year=argv[1];
     month=argv[2];
     day=argv[3];
     hour=argv[4];
     min=argv[5];
     sec=argv[6];


     sscanf(sec,"%d", &(my_tm.tm_sec));
     sscanf(min,"%d", &(my_tm.tm_min));
     sscanf(hour,"%d", &(my_tm.tm_hour));
     sscanf(day,"%d", &(my_tm.tm_mday));
     sscanf(month,"%d", &(my_tm.tm_mon));
     my_tm.tm_mon=my_tm.tm_mon-1;
     sscanf(year,"%d", &(my_tm.tm_year));
     my_tm.tm_year=my_tm.tm_year-1900;


     if(my_tm.tm_sec<0||my_tm.tm_sec>60||
        my_tm.tm_min<0||my_tm.tm_min>60||
        my_tm.tm_hour<0||my_tm.tm_hour>24||
        my_tm.tm_mday<1||my_tm.tm_mday>31||
        my_tm.tm_mon<0||my_tm.tm_mon>11||
        my_tm.tm_year<0||my_tm.tm_year>1000){
        fprintf(stderr,"bad date %s %s %s %s %s %s\n",
        year,month,day,hour,min,sec);
        exit(-1);
      }


      t = mktime(&my_tm);

      if(t==-1){
         fprintf(stderr,"error computing calendar time\n");
         exit(-1);
      }

      fprintf(stdout,"%ld\n",t);

      exit(0);
}

