#ifndef __net_h__
#define __net_h__

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
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <sys/stat.h>    /* for S_xxx file mode constants */
#include        <sys/uio.h>             /* for iovec{} and readv/writev */
/*#include        <unistd.h>*/
#include        <sys/wait.h>
#include        <sys/un.h>              /* for Unix domain sockets */

#endif


/*
10/14/15:
The "#include        <unistd.h>" line is disabled to deal
with the following error that I got when I tried to compile vme_list.c


[pradrun@prad rol]$ pwd
/home/pradrun/coda/2.6.2/extensions/linuxvme/ti/rol
[pradrun@prad rol]$ make 
....
...

/usr/include/unistd.h: At top level:
/usr/include/unistd.h:405: error: conflicting types for 'sleep'
tiprimary_list.c:185: error: previous implicit declaration of 'sleep' was here
make: *** [vme_list.so] Error 1

*/
