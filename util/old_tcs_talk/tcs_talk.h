#include 	<stdio.h>
#include        <string.h>
#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>                /* timespec{} for pselect() */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>               /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdlib.h>
#include        <sys/stat.h>    /* for S_xxx file mode constants */
#include        <sys/uio.h>             /* for iovec{} and readv/writev */
#include        <unistd.h>
#include        <sys/wait.h>
#include        <sys/un.h>              /* for Unix domain sockets */
#include        <sys/uio.h>
#ifdef  __bsdi__
#include        <machine/endian.h>      /* required before tcp.h, for BYTE_ORDER */
#endif
#include        <netinet/tcp.h>         /* TCP_NODELAY */
#include        <netdb.h>               /* getservbyname(), gethostbyname() */
#include        <termios.h>
#include	<sys/poll.h>


