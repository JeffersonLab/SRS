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
#include "srsLib.h"

static int srsDebugMode=0; 
char command[500], cDir[200]="";
unsigned long long before, after, diff=0;
static void ReadFile(char *path, char **ip, int *port);
void PrepareSocket(char *ip_addr,int port_number);
void SendData();
void ReceiveData();
unsigned long long int rdtsc(void);

void 
srsSlowControl(int enable)//1 or 0 for start or stop
{
  char *commandDirectory;   
  char ToDo[2][20]={"stop","start"};

  if(enable)
    enable=1;
  else
    enable=0;

  commandDirectory = getenv("SRS_SLOWCONTROL_DIR");
  if(commandDirectory==NULL)
    sprintf(commandDirectory,"./SRSconfig");
  
  if(enable)
    {
      sprintf(command,"source %s/configureFEC.sh",commandDirectory);
      system(command);
    }

  sprintf(command,"source %s/%s_pRad.sh",commandDirectory,ToDo[enable]);  
  system(command);
}

void 
srsTest()
{
  char *ip;
  int port = 0;

  ReadFile("SRSconfig/slow_control/ipSet.txt", (char **)&ip, (int *)&port);
  PrepareSocket(ip, port);
  SendData();
  
  ReceiveData();
}

int
srsConnect(int *sockfd) /* formally createAndBinSocket */
{
  struct sockaddr_in my_addr; 
  struct timeval socket_timeout={0,1000000}; /* 1 millisecond time out */
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

int 
listenSRS4CODAv0(int sockfd, volatile unsigned int* buf)
{
  int HowManyEvents=1,channelNo=-1, trigNum=0, n; //cli for client?

/*   srsConnect(&sockfd); */

  struct sockaddr_in cli_addr;  
  socklen_t slen=sizeof(cli_addr);  
  int *ptr; 
  int Index=0;  
  unsigned int rawdata=0;
  int nwords=0;
  unsigned long long total=0;

  ptr = (int*)buf; 
  while(1)
    {
      if(srsDebugMode)
	printf("%s: call recvfrom\n",__FUNCTION__);

      before = rdtsc();
      n = recvfrom(sockfd, (void *)ptr, 20*1024*sizeof(unsigned int), 
		   0, (struct sockaddr*)&cli_addr, &slen); 
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
	    perror("recvfrom");
	  return -1;
	}

      if(trigNum==HowManyEvents) 
	break;

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

/* UDP Send/Receive commands */

#define SIZE 65504

int g_sockfd,g_sockfd2,g_n,g_fd;
socklen_t g_len,g_servlen;
struct sockaddr_in g_servaddr,g_servaddr2;
struct sockaddr_in g_cliaddr;
struct sockaddr *g_replyaddr;

int g_broadcast = 1;
pid_t g_pid;
char g_buffer[SIZE],g_buf[SIZE];
int g_byte = 0;
char g_ip[100];
int g_port = 0;


struct addrinfo *
host_serv(const char *host, const char *serv, int family, int socktype) 
{
  struct addrinfo hints, *res;
  
  memset(&hints,0,sizeof(struct addrinfo));
  hints.ai_flags = AI_CANONNAME; // returns canonical name
  hints.ai_family = family; // AF_UNSPEC AF_INET AF_INET6 etc...
  hints.ai_socktype = socktype; // 0 SOCK_STREAM SOCK_DGRAM

  if ((g_n = getaddrinfo(host,serv,&hints,&res)) != 0) 
    return NULL;
  
  return (res);    
}

char* 
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
void 
PrepareSocket(char *ip_addr,int port_number) 
{
  struct addrinfo *ai;
  char *h;

  g_sockfd = socket(AF_INET,SOCK_DGRAM,0);  

  if (g_broadcast == 1) 
    {
      const int on = 1;
      setsockopt(g_sockfd,SOL_SOCKET,SO_BROADCAST,&on,sizeof(on));
    }

  memset(&g_servaddr,0,sizeof(g_servaddr));
  g_servaddr.sin_family = AF_INET;

  //
  // set the source port
  //
  g_servaddr.sin_port = htons(6007);
  bind(g_sockfd,(struct sockaddr *)& g_servaddr, sizeof(g_servaddr));

  //
  // set the destination port
  //
  g_servaddr.sin_port = htons(port_number);
  ai = host_serv(ip_addr,NULL,AF_INET,SOCK_DGRAM);
  h = sock_ntop_host(ai->ai_addr,ai->ai_addrlen);
  inet_pton(AF_INET,(ai->ai_canonname ? h : ai->ai_canonname),&g_servaddr.sin_addr);
  g_servlen = sizeof(g_servaddr);
  g_replyaddr = malloc(g_servlen);
  freeaddrinfo(ai);  
}

//
// Start to send data
//
void
SendData() 
{
  int num;
  
  num = sendto(g_sockfd,g_buffer,g_byte,0,(struct sockaddr *)&g_servaddr,sizeof(g_servaddr));

  if ( num == -1) 
    {
      perror ("error sending buffer using sendto");
      return;
    }
}

void
ReceiveData() 
{
  if ((g_pid = fork()) == 0) 
    {
      int n;
      char buffer[SIZE];
      unsigned int *ptr;
      
      ptr = (unsigned int*)buffer;
      memset(buffer,0,SIZE);
      n = recvfrom(g_sockfd,(void *)ptr,sizeof(buffer),0,(struct sockaddr *) &g_cliaddr,&g_len);
      ptr+=5;

      printf("%s: Received %d bytes\n",__FUNCTION__,n);

      int i=0;
      for(i=0; i<n; i++)
	{
	  if((i%4)==0) printf("\n%4d: ",i);
	  printf(" %02x",buffer[i] &0xff);
	}

      for(i=0; i<n/4; i++)
	{
	  printf("%4d: %08x\n",i,*ptr);
	}
      
      printf("\n");
      return;
    }
}

void 
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

static void 
ReadFile(char *path, char **ip, int *port) 
{
  FILE *pFile;
  int *buf;;
  char buffer[100];
  
  memset(g_buffer,0,SIZE);
  buf = (int*)g_buffer;
  memset(buffer,0,100);
  pFile = fopen (path , "r");
  if (pFile == NULL) 
    {
      printf ("Error opening file %s",path);
      exit(0);
    } 
  else 
    {
      //
      // IP
      //
      fgets (buffer,100,pFile);
      snprintf(g_ip,sizeof(g_ip),"%s",buffer);
      **ip = g_ip;
      //
      // PORT
      //
      fgets (buffer,100,pFile);
      g_port = atoi(buffer);
      *port = g_port;
      //
      // INFO
      //
      while (fgets (buffer,100,pFile) != NULL) 
	{
	  if (buffer[0] == '#') continue;
	  charToHex(buffer,buf);
	  *buf = htonl(*buf);
	  buf++;
	  g_byte+=4;
	}
      fclose (pFile);
    }
}

unsigned long long int rdtsc(void)
{
  /*    unsigned long long int x; */
  unsigned a, d;
   
  __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

  return ((unsigned long long)a) | (((unsigned long long)d) << 32);
}
