/*
  Test program for sending commands to COMSOFT TCS demo over serial port, and for monitoring return telemetry stream

   D. Rabinowitz 3-04-08
*/


#include "tcs_talk.h"
#include "telescope_drv.h"
#include "errno.h"

//#define FAKE_CONTROLLER

/* IO permissions when opening the serial port for reading and writing */
#define READ_OPS O_RDONLY
#define WRITE_OPS O_WRONLY

/* Baud Rate to serial IO */
#define BAUD_RATE B9600

#define STRING_LENGTH 10240 /* length of read buffer */

/* value of MMC read timeout in seconds and usec */
#define TIMEOUT_SEC 0  
/*#define TIMEOUT_USEC 1000000*/
#define TIMEOUT_USEC 5000000
/*#define SERIAL_PORT "/dev/cua1"*/
/*#define SERIAL_PORT "/dev/ttyS0"*/
#define SERIAL_PORT "/dev/ttyUSB0"
/*#define SERIAL_PORT "/dev/cu.usbserial-FTDD9UGB"*/
#define DONE_REPLY "done"
#define ERROR_REPLY "error"

/* Message to send back when there is an error reading data from the MMC controller */
#define READ_ERROR_REPLY "LOGFLEX_READ_ERROR"

/* number of times to read buffer before accepting string */
#define NUM_READ_TRIES 10

/* Local Globals */

char serial_port[256]; /* serial port device name */

int fd_serial_in,fd_serial_out; /* file descriptors for the serial port, one for
                                   writing (fd_serial_out) by the parent process, 
                                   and one for reading (fd_serial_in) by the
                                   child process */
                                             
unsigned short *serial_buffer; /* buffer for reading and writing to the serial port */

int verbose=0;
int verbose1=0;


/*************************************************************************/
main(argc,argv)
int argc;
char **argv;
{
        int i,status,n;
        char *string,cmd[1024],s[256],s1[256];
        TCS_Telemetry telem_buf;
        int buf_size,cmd_len;
        pid_t  pid;
        int skip_write;



        string = (char *)(& telem_buf);
        buf_size=sizeof(TCS_Telemetry);
        sprintf(string,"nothing\n");
        cmd[0]=0;

        fgets(cmd,1024,stdin);

#if 0
        if(strstr(cmd,TELESCOPE_CMD_NOCMD_STR)!=NULL){
          exit(0);
        }
        if(strstr(cmd,TELESCOPE_CMD_STS_STR)!=NULL){
          skip_write = 1;
        }
        else{
          skip_write = 0;
        }
#else
        if(strstr(cmd,TELESCOPE_CMD_STS_STR)!=NULL ||
                strstr(cmd,TELESCOPE_CMD_NOCMD_STR)!=NULL){
          skip_write = 1;
          //skip_write = 0;
        }
        else{
          skip_write = 0;
        }
#endif

        if(skip_write!=1){
            cmd_len=chksum(cmd);
            if(verbose){
                fprintf(stderr,"\nsending command: %s\n",cmd);
                fflush(stderr);
            }
              if(init_serial_output(SERIAL_PORT)<0){
                printf("dropmgr_main: could not initialize serial output\n");
                exit(-1);
            }
        }
        if(init_serial_input(SERIAL_PORT)<0){
            printf("dropmgr_main: could not initialize serial input\n");
            exit(-1);
        }

        if(skip_write!=1){
            write_serial(cmd,cmd_len);
        }
        for(i=0;i<buf_size;i++)string[i]=0;

        if(verbose){
           fprintf(stderr,"reading telemetry\n");
           fflush(stderr);
        }

        read_serial(string);
        n=1;
        while(strlen(string)!=buf_size&&n++<NUM_READ_TRIES){
                for(i=0;i<buf_size;i++)string[i]=0;
                read_serial(string);
                fprintf(stdout,"reply is [%s]\n",string);
                usleep(100000);
        }
if(n>1){
  fprintf(stderr,"%d iterations trial 1\n",n-1);
}

        /* If the cmd was a STS_CMD, and the telemetry read back 
           indicates a bad command, then send a NOP string.
           This should clead the bad command status.*/
        if (skip_write&&strncmp(telem_buf.com1,"e",1)!=0&&
          strncmp(telem_buf.com1,"E",1)!=0&&
          strncmp(telem_buf.com2,"e",1)!=0&&
          strncmp(telem_buf.com2,"E",1)!=0){
            sprintf(cmd,"%s",TELESCOPE_CMD_NOP_STR);
            cmd_len=chksum(cmd);
              if(init_serial_output(SERIAL_PORT)<0){
                printf("dropmgr_main: could not initialize serial output\n");
                exit(-1);
            }
            write_serial(cmd,cmd_len);
            for(i=0;i<buf_size;i++)string[i]=0;
            read_serial(string);
            n=1;
            while(strlen(string)!=buf_size&&n++<NUM_READ_TRIES){
                for(i=0;i<buf_size;i++)string[i]=0;
                read_serial(string);
                usleep(100000);
           }
if(n>1){
  fprintf(stderr,"%d iterations trial 2\n",n-1);
}
        }


 
        if(verbose){
            fprintf(stderr,"writing telemetry\n");
            fflush(stderr);
        }
        printf("%s",string);

        if(verbose){
            fprintf(stderr,"done writing telemetry\n");
            fflush(stderr);
        }
        if (skip_write!=1){
             terminate_serial_output();
        }
        terminate_serial_input();

        exit(0);
}
        

/*************************************************************************/

init_serial_input(serial_port)
char *serial_port;
{

#ifdef FAKE_CONTROLLER
   if (verbose) printf("init_serial_input: simulating serial IO\n");
#else
        fd_serial_in = open(serial_port,READ_OPS);
        if (fd_serial_in < 0) {
                printf("fingmgr_serial: can't open %s for input\n",serial_port);
                perror(serial_port);
                return(-1);
        }

        if(init_port(fd_serial_in,READ_OPS)!=0){
                printf("fingmgr_serial: can't set serial port input parameters\n");
                return(-1);
        }
#endif

        return(0);
}

/*************************************************************************/

init_serial_output(serial_port)
char *serial_port;
{
#ifdef FAKE_CONTROLLER
   if(verbose)printf("init_serial_output: simulating serial IO\n");
#else
        fd_serial_out = open(serial_port,WRITE_OPS);
        if (fd_serial_out < 0) {
                printf("fingmgr_serial: can't open %s for output\n",serial_port);
                perror(serial_port);
                return(-1);
        }

        if(init_port(fd_serial_out,WRITE_OPS)!=0){
                printf("fingmgr_serial: can't set serial port output parameters\n");
                return(-1);
        }
#endif
        return(0);
}

/*************************************************************************/

terminate_serial_input()
{

#ifdef FAKE_CONTROLLER
#else
        close(fd_serial_in);
#endif
        return(0);
}

/*************************************************************************/

terminate_serial_output()
{

#ifdef FAKE_CONTROLLER
#else
        close(fd_serial_out);
#endif
        return(0);
}
/****************************************************************/

init_port(fd,op)
int fd;
int op;
{
    speed_t speed;
    struct termios term1,*term;
    
    term=&term1;

    if((int)tcgetattr(fd,term)!=0){
            printf("can't get termios structure\n");
        perror("tcsgetattr error");
            return(-1);
    }
    
    term->c_cflag=0;
    term->c_oflag=0;
    term->c_iflag=0;
    term->c_lflag=0;
    
    if(op==READ_OPS){
           cfsetispeed(term,BAUD_RATE);
        term->c_cflag=term->c_cflag|CREAD|CS8;
        term->c_lflag=term->c_lflag&(~ECHO);   

#if 1
        term->c_cflag=term->c_cflag&(~CRTSCTS);
#else
        term->c_cflag=term->c_cflag|CRTSCTS;
#endif

    }
    else if(op==WRITE_OPS){
        cfsetospeed(term,BAUD_RATE);
        term->c_cflag=term->c_cflag|CS8|CLOCAL|CREAD;
         term->c_lflag=term->c_lflag&(~ECHO);   
#if 1
        term->c_cflag=term->c_cflag&(~CRTSCTS);
#else
        term->c_cflag=term->c_cflag|CRTSCTS;
#endif

    }
    else{
           printf("port ops not recognized\n");
           return(-1);
    }     

    if(tcsetattr(fd,TCSANOW,term)!=0){
            printf("can't set termios structure\n");
            return(-1);
    }

    return(0);

}

/*************************************************************************/

read_serial(string)
char *string;
{
        int i,n,result,length,l1,l2;
        char done_reply[3];
        char *s;
        fd_set readfds;
        struct timeval timeout;
        int done,sync;

        
#ifdef FAKE_CONTROLLER
        TCS_Telemetry *t;
        t=(TCS_Telemetry *)malloc(sizeof(TCS_Telemetry));
        for (i=0;i<sizeof(TCS_Telemetry);i++){
           strcpy((char *)(t+i)," ");
        }
        strcpy(t->motion_status,"1");
        strcpy(t->wobble_status," ");
        strcpy(t->dummy1," ");
        strcpy(t->ra,"000000.00");
        strcpy(t->dummy2," ");
        strcpy(t->dec,"+320000.00");
        strcpy(t->dummy3,"  ");
        strcpy(t->ha,"+00:00:00");
        strcpy(t->dummy4," ");
        strcpy(t->lst,"00:00:00");
        strcpy(t->dummy5," ");
        strcpy(t->alt,"+90.0");
        strcpy(t->dummy6," ");
        strcpy(t->azim,"+000.0");
        strcpy(t->dummy7," ");
        strcpy(t->secz,"01.00");
        strcpy(t->dummy8," ");
        strcpy(t->com1,"E"); /* alternates E,e for each command successfully executed */
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
        strcpy(t->drive_status," "); /* 1 when enabled, blank otherwise */
        strcpy(t->epoch,"2000.000");
        strcpy(t->dummy10," ");
        strcpy(t->jd,"2450000.00");
#ifdef DEMO_TCS
        strcpy(t->reserved,"     ");
#else
        strcpy(t->dummy11," ");
        strcpy(t->channel,"1");
        strcpy(t->dummy12," ");
        strcpy(t->focus_pos,"+00000");
        strcpy(t->dummy13," ");
        strcpy(t->dome_err_deg,"+000.0");
        strcpy(t->dummy14," ");
        strcpy(t->ut_time,"00:00:00.0");
        strcpy(t->dummy," ");
        strcpy(t->dome_state,"0"); /*0 for closed, 1 for open */
        strcpy(t->reserved,"                          ");
#endif
        sprintf(t->cr,"\r");
        sprintf(t->lf,"\n");
        strncpy(string,(char *)t,sizeof(TCS_Telemetry));

        n=strlen(string);
#else
        /* set  timeout value for polling port for data */
        
        timeout.tv_sec=TIMEOUT_SEC;
        timeout.tv_usec=TIMEOUT_USEC;

        /* get lengths of DONE_REPLY and ERROR_REPLY */

        sprintf(done_reply,"\r\n");
        l1=strlen(done_reply)+2;
        l2=strlen(ERROR_REPLY)+2;

          
        sprintf(string,"");
        s=string;
        length=0;
        done=0;
        sync=0;
        while(done==0){

            /* clear the set of read file descriptors that will be polled. THen
                add fd_serial_in to the set */

             FD_ZERO(&readfds);
            FD_SET(fd_serial_in,&readfds);
            result=select(fd_serial_in+1,&readfds,NULL,NULL,&timeout);

            if(result==-1){
              fprintf(stderr,"read_serial: error waiting for serial reply\n");
              fflush(stderr);
              *s=0;
              done=1;
            }
            else if (result==0){
                fprintf(stderr,"read_serial: timeout waiting for data\n");
                fflush(stderr);
                /* timeout waiting for reply. There is either no reply or
                   the MMC has hung. */
                *s=0;
                done=1;
            }
            else if(result>0){
               /*n=read(fd_serial_in,s,STRING_LENGTH-length);*/
               n=read(fd_serial_in,s,1);
               //printf("s: %s\n",s);
               s=s+n;
               length=length+n;
               *s=0;
               /*if(verbose1){printf("read %d: %s\n",n,s-n);fflush(stdout);}*/
               if(verbose1){printf("read %d of %d: %s\n",n,length,s);fflush(stdout);}
               if(length>=l1&&strstr(s-l1,done_reply)!=NULL){
                 if(sync){
                   if(verbose1){fprintf(stderr,"read_serial: synced\n");fflush(stdout);}
                   done=1;
                 }
                 else {
                   sync=1;
                   s=string;
                   length=0;
                 }

               }
            }
        }

        if(verbose){printf("read_serial: done reading serial\n");fflush(stdout);}
        
        /*tcdrain(fd_serial_in);*/
#endif
        return(n);
}


/*************************************************************************/

write_serial(string,cmd_len)
char *string;
int cmd_len;
{
#ifdef FAKE_CONTROLLER
           
        if (verbose){
            fprintf(stdout,"fake_controller. skip writing command %s\n",string);fflush(stdout);
        }

#else
        if (verbose){
            fprintf(stdout,"writing command %s\n",string);fflush(stdout);
        }

        //if(write(fd_serial_out,string,strlen(string))!=strlen(string)){
        if(write(fd_serial_out,string,cmd_len)!= cmd_len){
             printf("fingmgr_serial: can't write to serial port\n");
             return(-1);
        }
        
        tcdrain(fd_serial_out);
#endif
        return(0);
} 

/*************************************************************************/

int chksum(char *cmd)
{
    int sum=0;
    int len=0;
    int i, subchar;
    char s[2];
    int cmd_len=0;

    for(i=0;i<strlen(cmd); i++){
       len=i;
       subchar=cmd[i];

       if(subchar != 10 && subchar != 0 && subchar != 13){
          sum += subchar;
       }
       else{
          break;
       }
    }

    cmd[len]=(sum % 64) + 0x20;
    cmd[len+1]=13;
    cmd[len+2]=0; 
    cmd_len = strlen(cmd);

    // need extra zero-byte at end to keep junk out of buffer sent to TCS

    return (cmd_len);
}


/*************************************************************************/
