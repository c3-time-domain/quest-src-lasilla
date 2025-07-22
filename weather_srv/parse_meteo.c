/* parse_meteo.c

   read the meteo.last file obtained by ntt_dome_status and print
   temperature, humidity, wind_sp speed, wind_sp direction,  dewpoint, and pressure

   DLR 2009 Jul 17
*/

#include <stdio.h>

double get_median(double x1, double x2, double x3);

main(int argc, char **argv)
{
    double temp=0.0,hum=0.0,press=0.0,wind_sp=0.0,wind_dir=0.0,dewp=0.0;
    double t1=0.0,t2=0.0,t3=0.0,w1=0.0,w2=0.0,w3=0.0,wd1=0.0,wd2=0.0,wd3=0.0;
    char string[1024],s[256];
    FILE *input;

    if (argc!=2){
        fprintf(stderr,"syntax: parse_meteo meteo_file\n");
        exit(-1);
    }

    input=fopen(argv[1],"r");
    if(input==NULL){
       fprintf(stderr,"parse_meteo: unable to open file %s\n",argv[0]);
       exit(-1);
    }

    while(fgets(string,1024,input)!=NULL){
       if(strstr(string,"T 1")!=NULL){
          sscanf(string,"%s %s %s %s %lf",s,s,s,s,&t1);
       }
       else if (strstr(string,"T 2")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&t2);
       }
       else if (strstr(string,"T 3")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&t3);
       }
       else if (strstr(string,"RH  %")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&hum);
       } 
       else if (strstr(string,"TD  C")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&dewp);
       } 
       else if (strstr(string,"P   MB")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&press);
       } 
       else if (strstr(string,"WS1 M/S")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&w1);
       } 
       else if (strstr(string,"WS2 M/S")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&w2);
       } 
       else if (strstr(string,"WS3 M/S")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&w3);
       } 
       else if (strstr(string,"WD1 DEG")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&wd1);
       } 
       else if (strstr(string,"WD2 DEG")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&wd2);
       } 
       else if (strstr(string,"WD3 DEG")!=NULL){
         sscanf(string,"%s %s %s %s %lf",s,s,s,s,&wd3);
       } 
    }

    
    temp=get_median(t1,t2,t3);
    wind_sp=get_median(w1,w2,w3);
    wind_dir=get_median(wd1,wd2,wd3);

    fprintf(stdout,"%5.1f %5.1f %5.1f %5.1f %5.1f %5.1f\n",
		temp,hum,wind_sp,wind_dir,dewp,press);

    fclose(input);
    
    exit(0);
}

/*************************************************/

double get_median(double x1, double x2, double x3)
{
    double temp;
    int done;

    /* sort x1,x2,x3 */

    done=0;
    while (!done){

      done=1;

      if(x1>x2){
         temp=x2;
         x2=x1;
         x1=temp;
         done=0;
      }

      if(x2>x3){
         temp=x3;
         x3=x2;
         x2=temp;
         done=0;
      }

    }
 
    return(x2);
}
     

    
    
