/* includes generally required for socket operations */
#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN extern
#endif

#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>               /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdlib.h>
#include        <string.h>
#include        <sys/stat.h>    /* for S_xxx file mode constants */
#include        <sys/uio.h>             /* for iovec{} and readv/writev */
#include	<sys/time.h>
#include        <unistd.h>
#include        <sys/wait.h>
#include        <sys/un.h>              /* for Unix domain sockets */
#include        <sys/uio.h>
#ifdef  __bsdi__
#include        <machine/endian.h>      /* required before tcp.h, for BYTE_ORDER */
#endif
#include        <netinet/tcp.h>         /* TCP_NODELAY */
#include        <netdb.h>               /* getservbyname(), gethostbyname() */



#include <time.h>
#include <sys/types.h>
#include <stdio.h>

#define MAXBUFSIZE 1024
#define TCS_STATUS_SRV_HOST "pco-nuc"
#define TCS_STATUS_SRV_PORT 3913
#define TCS_STATUS_SRV_TIMEOUT_SEC 10
   
EXTERN int send_socket_command(char *command, char *reply, char *machine,
                        int port, int timeout_sec);
EXTERN int read_socket_data(int s, char *buf, int n);
EXTERN int write_socket_data(int s, char *buf, int n);
EXTERN int call_socket(char *hostname, u_short portnum);



