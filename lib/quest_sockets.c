/* quest_socket.c

   routines for opening, connecting, sending, and receiving commands
   with unix sockets
 
   DLR 2007 Mar 5
   DLR Revised 2009 Nov 10
*/

#include "quest_sockets.h"
#include "telescope_drv.h"
#define VERBOSE1 1

extern double neat_gettime_utc();


/************************************************************/

int send_socket_command(char *command, char *reply, char *machine, int port, int timeout_sec)
{
  int i,s;
  u_short p;
  double ut;

  for(i=0;i<MAXBUFSIZE;i++)reply[i]=0;

  if(strlen(command)>MAXBUFSIZE){
     fprintf(stderr,"send_socket_command: command size too long : %s\n",
               command);
     return(-1);
  }

  if(VERBOSE1){
     ut=neat_gettime_utc();
     fprintf(stderr,
        "send_socket_command: %9.6lf calling socket with machine %s port %d\n",
         ut,machine,port);
     fflush(stderr);
  }

  p = port;
  if ((s= call_socket(machine,p)) < 0) { 
        fprintf(stderr,"send_socket_command: could not open socket with machine %s port %d\n",
             machine,port);
        fflush(stderr);
        perror("send_socket_command: call_socket");
        return(-1);
  }

  if(VERBOSE1){
  	    ut=neat_gettime_utc();
            fprintf(stderr,"send_socket_command: %9.6lf writing command : %s\n",
		ut,command);
            fflush(stderr);
  }

  //DEBUG

  char *buf;
  buf = (char *)malloc((strlen(command)+1)*sizeof(char));
  strcpy(buf,command);
  sprintf(buf+strlen(command),"\n");

  //if (write_socket_data(s, (char *)command, strlen(command))
  //        != strlen(command)) {
  if (write_socket_data(s, (char *)buf, strlen(command)+1)
            != strlen(command)+1) {
  //if (write_socket_data(s, (char *)command, strlen(command)+1)
  //        != strlen(command)+1) {
          fprintf(stderr,"send_socket_command: can't write data to socket\n");
          return(-1);
  }

  free(buf);

  if(VERBOSE1){
  	  ut=neat_gettime_utc();
          fprintf(stderr,"send_socket_command: %9.6lf reading reply \n",ut);
          fflush(stderr);
  }

  if(read_socket_data(s,reply,MAXBUFSIZE)<=0){
          fprintf(stderr,"send_socket_command: can't read command reply\n");
          return(-1);
  }

  if(VERBOSE1){
  	  ut=neat_gettime_utc();
          fprintf(stderr,"send_socket_command: %9.6lf reply is %s",ut,reply);
          fflush(stderr);
  }

  close(s);

  return(0);
}

/************************************************************/

int read_socket_data(int s,        /* connected socket */
	      char *buf,    /* pointer to the buffer */
	      int n)          /* number of characters (bytes) we want */
{ 
    int bcount, /* counts bytes read */
    br; /* bytes read this pass */

    bcount= 0;
    br= 0;
    while (bcount < n) { /* loop until full buffer */
 	if ((br= read(s,buf,n-bcount)) > 0) {
 	    bcount += br; /* increment byte counter */
 	    buf += br; /* move buffer ptr for next read */
 	}
 	if (br < 0) /* signal an error to the caller */
 	    return(-1);
        else if (*buf==0||br==0)
           return(bcount);
    }
    
    return(bcount);
 }

/************************************************************/
int write_socket_data(int s,         /* connected socket */
	       char *buf,     /* pointer to the buffer */
	       int n)         /* number of characters (bytes) we want */
{ 
    int bcount, /* counts bytes read */
    br; /* bytes read this pass */

    bcount= 0;
    br= 0;
    while (bcount < n) { /* loop until full buffer */
 	if ((br= write(s,buf,n-bcount)) > 0) {
 	    bcount += br; /* increment byte counter */
 	    buf += br; /* move buffer ptr for next read */
 	}
 	if (br < 0) /* signal an error to the caller */
 	    return(-1);
    }
    
    return(bcount);
 }


/************************************************************/
 
int call_socket(char *hostname, u_short portnum)
 { 
     struct sockaddr_in sa;
     struct hostent *hp;
     int s;

     if ((hp= gethostbyname(hostname)) == NULL) { /* do we know the host's */
 	errno= ECONNREFUSED; /* address? */
 	return(-1); /* no */
     }

     bzero(&sa,sizeof(sa));
     bcopy(hp->h_addr,(char *)&sa.sin_addr,hp->h_length); /* set address */
     sa.sin_family= hp->h_addrtype;
     sa.sin_port= htons((u_short)portnum);
     if ((s= socket(hp->h_addrtype,SOCK_STREAM,0)) < 0) /* get socket */
 	return(-1);
 	
     if (connect(s,(const  struct sockaddr* )&sa,sizeof sa) < 0) { /* connect */
 	close(s);
 	return(-1);
     }
     
     return(s);
 }
/************************************************************/

