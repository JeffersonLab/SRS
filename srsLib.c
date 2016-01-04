/*----------------------------------------------------------------------------*/
/**
 * @mainpage
 * <pre>
 *  Copyright (c) 2015        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Authors: Bryan Moffit                                                   *
 *             moffit@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5660             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *     Library for readout of SRS
 *
 * </pre>
 *----------------------------------------------------------------------------*/

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h> 
#include <string.h>
#include <fcntl.h>
#include <errno.h>  
#include <sys/stat.h>  
#include <sys/un.h>  
#include <sys/time.h>    /* timeval{} for select() */
#include <time.h>        /* timespec{} for pselect() */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <netdb.h>
#include <sys/stat.h>    /* for S_xxx file mode constants */
#include <sys/uio.h>     /* for iovec{} and readv/writev */
#include <sys/un.h>      /* for Unix domain sockets */
#include <ctype.h>
#include <byteswap.h>
#include <pthread.h>
#include "srsLib.h"

static int srsDebugMode=0; 
static int srsSockFD[MAX_FEC];
unsigned long long before, after, diff=0;

static int srsSlowControl(char *ip, int port, 
			  unsigned int *req_fields, int nfields,
			  unsigned int *ret_data);
static int srsParseResponse(unsigned int *ibuf, unsigned int *obuf, int nwords);
static int srsPrepareSocket(char *ip_addr,int port_number);
static int srsSendData();
static int srsReceiveData(unsigned int *buffer, int nwords);
static int srsCloseSocket();
static int srsReadFile(char *path, char *ip, int *port, unsigned int *obuffer);
static unsigned long long int rdtsc(void);

/* Mutex to guard SRS read/writes */
pthread_mutex_t     srsMutex = PTHREAD_MUTEX_INITIALIZER;
#define SRSLOCK     if(pthread_mutex_lock(&srsMutex)<0) perror("pthread_mutex_lock");
#define SRSUNLOCK   if(pthread_mutex_unlock(&srsMutex)<0) perror("pthread_mutex_unlock");

int
srsConnect(int *sockfd) /* formally createAndBinSocket */
{
  struct sockaddr_in my_addr; 
  struct timeval socket_timeout={0,900000}; /* 900 microsecond time out */
  int stat=0;
  int PORT = 6006;

  *sockfd = socket(AF_INET, SOCK_DGRAM,0);
  if (*sockfd==-1)    
    {
      perror("socket");
      return -1;
    }
  else
    {
      if(srsDebugMode)
	printf("Server : Socket() successful\n");
    }

  stat = setsockopt(*sockfd,
		    SOL_SOCKET,
		    SO_RCVTIMEO,
		    &socket_timeout,
		    sizeof(socket_timeout));

  if(stat!=0)
    {
      perror("setsockopt");
      printf("%s: errno = %d\n",__FUNCTION__,errno);
    }

  memset(&my_addr, 0, sizeof(my_addr));

  my_addr.sin_family      = AF_INET;
  my_addr.sin_port        = htons(PORT);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  stat = bind(*sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr));

  if (stat==-1) 
    {
      perror("bind");
      return -1;
    }
  else
    {
      if(srsDebugMode)
	printf("Server : bind() successful\n");
    }

  before = rdtsc();
  sleep(1);
  after = rdtsc();
  diff = after-before;
  printf("%s: norm = %lld\n",__FUNCTION__,diff);

  return 0;
}

#define SIZE 65504


int 
srsReadBlock(int sockfd, volatile unsigned int* buf_in, int nwrds, int blocklevel)
{
  int channelNo=-1, trigNum=0, n; //cli for client?
  struct sockaddr_in cli_addr;  
  socklen_t slen=sizeof(cli_addr);  
  unsigned int *ptr; 
  int Index=0;  
  unsigned int rawdata=0;
  int nwords=0;
  unsigned long long total=0;
  int l_errno=0;
  unsigned int *buf;

  buf = malloc(nwrds*sizeof(unsigned int));

  ptr = (unsigned int*)buf; 

  while(1)
    {
      if(srsDebugMode)
	printf("%s: call recvfrom  buf = 0x%lx\n",__FUNCTION__,(unsigned long)&buf);

      before = rdtsc();
      n = recvfrom(sockfd, (void *)ptr, nwrds*sizeof(unsigned int), 
		   0, (struct sockaddr*)&cli_addr, &slen); 
      l_errno = errno;
      after = rdtsc();

      diff = after-before;
      total += diff;
      if(srsDebugMode)
	  printf("%s: recvfrom returned %d    time = %lld\n",__FUNCTION__,n,diff);
      
      if (n == -1)
	{ 
	  if(errno == EAGAIN)
	    printf("%s: timeout\n",__FUNCTION__);
	  else
	    {
	      printf("%s: errno = %d  sockfd = %d\n",__FUNCTION__,l_errno,sockfd);
	      perror("recvfrom");
	    }
	  return -1;
	}

      if (n>=4)
	{
	  if(srsDebugMode)
	    printf("Received packet from %s:%d of length %i\n",
		   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), n);

	  for ( Index=0;Index<n/4;Index++)
	    {
	      rawdata = *ptr++;
	      nwords++;
	      if(srsDebugMode)
		printf("%d  0x%x\n",Index,rawdata);

	      if(rawdata == 0xfafafafa) /* event-trailer frame */
		continue; 
	      else
		{
		  if (Index == 1)
		    {
		      unsigned int dh1 = ntohl(rawdata);
		      unsigned char c = (dh1 >>24) & 0xff;
		      c = (dh1 >>16) & 0xff;
		      c = (dh1 >>8) & 0xff;
		      unsigned int channel = dh1 & 0xff;
		      channelNo = (int)channel; 
		      if(channel==15) 
			trigNum++; //==0 is valid for the event-trailer frame too.
		    }
		}
	    }
	}

      if(trigNum==blocklevel) 
	break;


    }

/*   printf("%s: total = %lld\n",__FUNCTION__,total); */

  return nwords+1;
}


void
srsSetDebugMode(int enable)
{
  if(enable)
    srsDebugMode=1;
  else
    srsDebugMode=0;
}


int
srsReadList(char *ip, int port, 
	    unsigned int sub_addr, unsigned int *list, int nlist,
	    unsigned int *data, int maxwords)
{
  int maxregs=0;
  int iword=0, nwords=0, wCnt=0;
  unsigned int *read_command;

  switch(port)
    {
    case 6007:
      maxregs = 16;
      break;

    case 6039:
      maxregs = 32;
      break;

    case 6519:
      maxregs = 7;
      break;
      
    default:
      printf("%s: Reading All registers from port %d not supported\n",
	     __FUNCTION__,port);
      maxregs=0;
    }

  read_command = (unsigned int *)malloc((maxregs+4)*sizeof(unsigned int));
  if(read_command==NULL)
    {
      perror("malloc");
      printf("%s: Unable to allocate memory\n",__FUNCTION__);
      return -1;
    }

  read_command[wCnt++] = bswap_32(0x80000000);  /* Request ID */
  read_command[wCnt++] = bswap_32(sub_addr);    /* SubAddress */
  read_command[wCnt++] = bswap_32(0xBBAAffff);  /* Read List */
  read_command[wCnt++] = bswap_32(0x11223344);  /* CMD INFO (dont care) */
  for(iword=0; iword<nlist; iword++) /* Add in dummy fields */
    {
      read_command[wCnt++] = bswap_32(list[iword]);
    }

  nwords = srsSlowControl(ip, port, 
			  read_command, wCnt,
			  data);

  if(read_command)
    free(read_command);

  return nwords;
}

int
srsReadBurst(char *ip, int port, 
	     unsigned int sub_addr, unsigned int first_addr, int nregs,
	     unsigned int *data, int maxwords)
{
  int maxregs=0;
  int iword=0, nwords=0, wCnt=0;
  unsigned int *read_command;

  switch(port)
    {
    case 6007:
      maxregs = 16;
      break;

    case 6039:
      maxregs = 32;
      break;

    case 6519:
      maxregs = 7;
      break;
      
    default:
      printf("%s: Reading All registers from port %d not supported\n",
	     __FUNCTION__,port);
      nregs=0;
    }

  read_command = (unsigned int *)malloc((nregs+3)*sizeof(unsigned int));
  if(read_command==NULL)
    {
      perror("malloc");
      printf("%s: Unable to allocate memory\n",__FUNCTION__);
      return -1;
    }

  read_command[wCnt++] = bswap_32(0x80000000);  /* Request ID */
  read_command[wCnt++] = bswap_32(sub_addr);    /* SubAddress */
  read_command[wCnt++] = bswap_32(0xBBBBffff);  /* Read Burst */
  read_command[wCnt++] = bswap_32(first_addr);  /* CMD INFO (first address to read) */
  for(iword=0; iword<nregs-1; iword++) /* Add in dummy fields */
    {
      read_command[wCnt++] = bswap_32(first_addr+iword+1);
    }

  nwords = srsSlowControl(ip, port, 
			  read_command, wCnt,
			  data);

  if(read_command)
    free(read_command);

  return nwords;
}

int
srsWriteBurst(char *ip, int port, 
	      unsigned int sub_addr, unsigned int first_addr, int nregs,
	      unsigned int *wdata, int maxwords)
{
  int maxregs=0;
  int iword=0, nwords=0, wCnt=0;
  unsigned int *write_command;

  switch(port)
    {
    case 6007:
      maxregs = 16;
      break;

    case 6039:
      maxregs = 32;
      break;

    case 6519:
      maxregs = 7;
      break;
      
    default:
      printf("%s: Reading All registers from port %d not supported\n",
	     __FUNCTION__,port);
      nregs=0;
    }

  write_command = (unsigned int *)malloc((nregs+3)*sizeof(unsigned int));
  if(write_command==NULL)
    {
      perror("malloc");
      printf("%s: Unable to allocate memory\n",__FUNCTION__);
      return -1;
    }

  write_command[wCnt++] = bswap_32(0x80000000);  /* Request ID */
  write_command[wCnt++] = bswap_32(sub_addr);    /* SubAddress */
  write_command[wCnt++] = bswap_32(0xAABBffff);  /* Write Burst */
  write_command[wCnt++] = bswap_32(first_addr);  /* CMD INFO (first address to write) */

  for(iword=0; iword<(nregs-1); iword++) /* Add in dummy fields */
    {
      write_command[wCnt++] = bswap_32(wdata[iword]);
    }

  nwords = srsSlowControl(ip, port, 
			  write_command, wCnt,
			  NULL);

  if(write_command)
    free(write_command);

  return nwords;
}

int
srsWritePairs(char *ip, int port, 
	      unsigned int sub_addr, unsigned int *addr, int nregs,
	      unsigned int *wdata, int maxwords)
{
  int maxregs=0;
  int iword=0, nwords=0, wCnt=0;
  unsigned int *write_command;

  switch(port)
    {
    case 6007:
      maxregs = 16;
      break;

    case 6039:
      maxregs = 32;
      break;

    case 6519:
      maxregs = 7;
      break;
      
    default:
      printf("%s: Reading All registers from port %d not supported\n",
	     __FUNCTION__,port);
      nregs=0;
    }

  write_command = (unsigned int *)malloc((nregs+4)*sizeof(unsigned int));
  if(write_command==NULL)
    {
      perror("malloc");
      printf("%s: Unable to allocate memory\n",__FUNCTION__);
      return -1;
    }

  write_command[wCnt++] = bswap_32(0x80000000);  /* Request ID */
  write_command[wCnt++] = bswap_32(sub_addr);    /* SubAddress */
  write_command[wCnt++] = bswap_32(0xAAAAffff);  /* Write Pairs */
  write_command[wCnt++] = bswap_32(0x11223344);  /* CMD INFO (dont care) */
  for(iword=0; iword<nregs-1; iword++) /* Add in dummy fields */
    {
      write_command[wCnt++] = bswap_32(addr[iword]);
      write_command[wCnt++] = bswap_32(wdata[iword]);
    }

  nwords = srsSlowControl(ip, port, 
			  write_command, wCnt,
			  NULL);

  if(write_command)
    free(write_command);

  return nwords;
}

int
srsExecConfigFile(char *filename)
{
  char ip[100];
  int port = 0;
  unsigned int *buf;
  int nwords=0;

  buf = (unsigned int *)malloc(50*sizeof(unsigned int));

  nwords = srsReadFile(filename, (char *)&ip, (int *)&port, buf);

  srsSlowControl(ip, port,
		 buf, nwords,
		 NULL);

  free(buf);
  return 0;
}

static int
srsSlowControl(char *ip, int port, 
	       unsigned int *req_fields, int nfields,
	       unsigned int *ret_data)
{
  unsigned int *response;
  int iword, nwords=0, maxwords=0;

  maxwords      = (2*(nfields-4))+4;
  response      = (unsigned int *)malloc(maxwords*sizeof(unsigned int));

  if(srsDebugMode)
    {
      printf("%s: Writing to port %d  ip %s\n",
	     __FUNCTION__,port, ip);
      for(iword=0; iword<nfields; iword++)
	printf("%2d: 0x%08x\n",iword,bswap_32(req_fields[iword]));
    }


  SRSLOCK;
  if(srsPrepareSocket(ip, port)<0)
    {
      printf("%s: Unable to open/prepare socket\n",__FUNCTION__);
      srsCloseSocket();

      if(response)
	free(response);
      SRSUNLOCK;
      return -1;
    }

  if(srsSendData(req_fields,nfields)<0)
    {
      printf("%s: Unable to send data\n",__FUNCTION__);
      srsCloseSocket();

      if(response)
	free(response);
      SRSUNLOCK;
      return -1;
    }
  
  nwords = srsReceiveData(response, maxwords);
  if(nwords<0)
    {
      printf("%s: Failed to receive data\n",__FUNCTION__);
      srsCloseSocket();

      if(response)
	free(response);
      SRSUNLOCK;
      return -1;
    }

  if(srsDebugMode)
    {
      printf("%s: Response:\n",__FUNCTION__);
      for(iword=0; iword<nwords; iword++)
	printf("%2d: 0x%08x\n",iword,bswap_32(response[iword]));

      printf("\n");
    }

  srsCloseSocket();

  nwords = srsParseResponse(response, ret_data, nwords);
  SRSUNLOCK;

  if(response)
    free(response);

  return nwords;
}

static int
srsParseResponse(unsigned int *ibuf, unsigned int *obuf, int nwords)
{
  int iword=0, dCnt=0, nerr=0;

  /* First word */
  if((bswap_32(ibuf[0]) & (1<<31)) != 0)
    {
      printf("%s: 0x%08x: Invalid Reply Identifier\n",
	     __FUNCTION__,ibuf[0]);
    }

  for(iword=4; iword<nwords; iword+=2)
    {
      if(bswap_32(ibuf[iword])!=0) /* Error condition */
	nerr++;

      if(obuf)
	obuf[dCnt++] = bswap_32(ibuf[iword+1]);
    }
  
  if(nerr>0)
    printf("%s: %d Error(s) found in reply\n",__FUNCTION__,nerr);

  return dCnt;
}


/* UDP Send/Receive commands */
int g_sockfd;
struct sockaddr_in g_servaddr;


static struct addrinfo *
host_serv(const char *host, const char *serv, int family, int socktype) 
{
  struct addrinfo hints, *res;

  memset(&hints,0,sizeof(struct addrinfo));
  hints.ai_flags = AI_CANONNAME; // returns canonical name
  hints.ai_family = family; // AF_UNSPEC AF_INET AF_INET6 etc...
  hints.ai_socktype = socktype; // 0 SOCK_STREAM SOCK_DGRAM

  if (getaddrinfo(host,serv,&hints,&res) != 0) 
    {
      perror("getaddrinfo");
      return NULL;
    }
  
  return (res);    
}

static char* 
sock_ntop_host(const struct sockaddr *sa, socklen_t salen) 
{
  static char str[128];		/* Unix domain is largest */
  
  switch (sa->sa_family) 
    {
    case AF_INET: 
      {
	struct sockaddr_in	*sin = (struct sockaddr_in *) sa;
	
	if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
	  return(NULL);
	return(str);
      }

    default:
      {
	snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
		 sa->sa_family, salen);
	return(str);
      }
  }
  return (NULL);
}


//********************** create a socket ******************************************
static int
srsPrepareSocket(char *ip_addr,int port_number) 
{
  int rval=0;
  struct addrinfo *ai;
  char *h;
  struct timeval socket_timeout={0,900000}; /* 900 microsecond time out */
  int stat;

  g_sockfd = socket(AF_INET,SOCK_DGRAM,0);  
  if(g_sockfd<0)
    {
      perror("socket");
      return -1;
    }

  stat = setsockopt(g_sockfd,
		    SOL_SOCKET,
		    SO_RCVTIMEO,
		    &socket_timeout,sizeof(socket_timeout));
  if(stat!=0)
    {
      perror("setsockopt");
      rval=-1;
    }

  memset(&g_servaddr,0,sizeof(g_servaddr));
  g_servaddr.sin_family = AF_INET;

  //
  // set the source port
  //
  g_servaddr.sin_port = htons(6007);
  stat = bind(g_sockfd,(struct sockaddr *)& g_servaddr, sizeof(g_servaddr));
  if (stat==-1) 
    {
      perror("bind");
      return -1;
    }
  else
    {
      if(srsDebugMode)
	printf("%s: DEBUG: bind() successful\n",__FUNCTION__);
    }

  //
  // set the destination port
  //
  g_servaddr.sin_port = htons(port_number);
  ai = host_serv(ip_addr,NULL,AF_INET,SOCK_DGRAM);
  if(ai==NULL)
    {
      printf("%s: host_serv returned NULL\n",__FUNCTION__);
      return -1;
    }

  h = sock_ntop_host(ai->ai_addr,ai->ai_addrlen);
  stat = inet_pton(AF_INET,(ai->ai_canonname ? h : ai->ai_canonname),&g_servaddr.sin_addr);
  if(stat==0)
    {
      printf("%s: Invalid network address (%s)\n",
	     __FUNCTION__,(ai->ai_canonname ? h : ai->ai_canonname));
    }
  else if(stat<0)
    {
      perror("inet_pton");
      rval = -1;
    }

  freeaddrinfo(ai);  

  return rval;
}

//
// Start to send data
//
static int
srsSendData(unsigned int *buffer, int nwords) 
{
  int dSent=0;
  
  dSent = sendto(g_sockfd,buffer,(nwords<<2),0,
		 (struct sockaddr *)&g_servaddr,sizeof(g_servaddr));

  if ( dSent == -1) 
    {
      perror("sendto");
      return -1;
    }
  return (dSent>>2);
}

static int
srsReceiveData(unsigned int *buffer, int nwords) 
{
  int n, try=0;
  int l_errno;
  int dCnt;
  socklen_t g_len;
  struct sockaddr_in g_cliaddr;
      
  n = recvfrom(g_sockfd,(void *)buffer,nwords*sizeof(unsigned int),
	       0,(struct sockaddr *) &g_cliaddr,&g_len);
  while ((n<0) && (try<10))
    {
      try++;
      l_errno = errno;
      perror("recvfrom");
      n = recvfrom(g_sockfd,(void *)buffer,nwords*sizeof(unsigned int),
		   0,(struct sockaddr *) &g_cliaddr,&g_len);
    }
  dCnt = n>>2;

  if(try==10)
    {
      printf("%s: ERROR: Timeout\n",__FUNCTION__);
      return -1;
    }

  if(srsDebugMode)
    {
      int i;
      printf("%s: Received %d bytes\n",__FUNCTION__,n);
      for(i=0; i<dCnt; i++)
	{
	  printf("%4d: %08x  %08x\n",i,buffer[i],bswap_32(buffer[i]));
	}
	  
      printf("\n");
    }

  return dCnt;
}

static void 
charToHex(char *buffer,int *buf) 
{
  int i;
  int num = 0, tot = 0, base = 1;

  for (i=7;i>=0;i--)
    {
      buffer[i] = toupper(buffer[i]);
      if (buffer[i] < '0' || buffer[i] > '9')
	{
	  if (buffer[i] > 'F') buffer[i] = 'F';
	  num = buffer[i] - 'A';
	  num += 10;
	}
      else 
	num = buffer[i] - '0';
      
      tot += num*base;
      base *= 16;
    }
  *buf = tot;
}

static int
srsCloseSocket()
{
  if(g_sockfd)
    {
      if(close(g_sockfd)!=0)
	{
	  perror("close");
	  return -1;
	}
    }

  return 0;
}

static int
srsReadFile(char *path, char *ip, int *port, unsigned int *obuffer)
{
  FILE *pFile;
  int *buf;
  char buffer[100];
  int nwords=0;
  
  buf = (int*)obuffer;
  memset(buffer,0,100);
  pFile = fopen (path , "r");
  if (pFile == NULL) 
    {
      perror("fopen");
      printf("%s: Error opening file %s",__FUNCTION__,path);
      return -1;
    } 
  else 
    {
      //
      // IP
      //
      fgets (buffer,100,pFile);
      strncpy(ip,buffer,100);
      //
      // PORT
      //
      fgets (buffer,100,pFile);
      *port = atoi(buffer);
      //
      // INFO
      //
      while (fgets (buffer,100,pFile) != NULL) 
	{
	  if (buffer[0] == '#') continue;
	  charToHex(buffer,buf);
	  *buf = htonl(*buf);
	  buf++;
	  nwords++;
	}
      fclose (pFile);
    }

  return nwords;
}

static unsigned long long int 
rdtsc(void)
{
  /*    unsigned long long int x; */
  unsigned a, d;
   
  __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

  return ((unsigned long long)a) | (((unsigned long long)d) << 32);
}
