/*
  Test program for sending commands to COMSOFT TCS demo over serial port, and for monitoring return telemetry stream

   D. Rabinowitz 3-04-08
*/


#include "tcs_talk.h"
#include "telescope_drv.h"

/* IO permissions when opening the serial port for reading and writing */
#define READ_OPS O_RDONLY
#define WRITE_OPS O_WRONLY

/* Baud Rate to serial IO */
#define BAUD_RATE /*B9600*/B9600

#define STRING_LENGTH 10240 /* length of read buffer */

/* value of MMC read timeout in seconds and usec */
#define TIMEOUT_SEC 0  
/*#define TIMEOUT_USEC 1000000*/
#define TIMEOUT_USEC 5000000
/*#define SERIAL_PORT "/dev/cua1"*/
#define SERIAL_PORT "/dev/ttyS0"
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
        int buf_size;
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
        }
        else{
          skip_write = 0;
        }
#endif
     

        if(skip_write!=1){
            chksum(cmd);
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
            write_serial(cmd);
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
            chksum(cmd);
  	    if(init_serial_output(SERIAL_PORT)<0){
		printf("dropmgr_main: could not initialize serial output\n");
		exit(-1);
	    }
            write_serial(cmd);
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
   printf("init_serial_input: simulating serial IO\n");
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
   printf("init_serial_output: simulating serial IO\n");
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
	int n,result,length,l1,l2;
	char done_reply[3];
	char *s;
	fd_set readfds;
	struct timeval timeout;
	int done,sync;

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
	       s=s+n;
               length=length+n;
	       *s=0;
               if(verbose1){printf("%d: %s\n",n,s-n);fflush(stdout);}
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

	return(n);
}


/*************************************************************************/

write_serial(string)
char *string;
{
	if(write(fd_serial_out,string,strlen(string))!=strlen(string)){
	     printf("fingmgr_serial: can't write to serial port\n");
	     return(-1);
	}
	
	tcdrain(fd_serial_out);
	
	return(0);
} 

/*************************************************************************/

int chksum(char *cmd)
{
    int sum=0;
    int len=0;
    int i, subchar;
    char s[2];


    for(i=0;i<=strlen(cmd); i++){
       len=i;
       subchar=cmd[i];

       if(subchar != 10 ){
          sum += subchar;
       }
       else{
          break;
       }
    }

    cmd[len]=(sum % 64) + 0x20;
    cmd[len+1]=13;

    return;
}


/*************************************************************************/
