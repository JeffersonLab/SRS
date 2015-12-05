/**************1/21/15: Krishna
 *  
 *
 *****************/

#define DISABLE_COMMENTS
//#define DISABLE_C_CODE
#define DISABLE_CPP_CODE
//#define DISABLE_WHILE_LOOP
//#define DISABLE_SLOWCONTROL

#include "net.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
/*#include <unistd.h>*/
#include <stdlib.h> 
#include <string.h>
#include <fcntl.h>
#include <errno.h>  
#include <sys/stat.h>  
#include <sys/un.h>  



#ifndef DISABLE_CPP_CODE
#include <iostream>
#include <iomanip>
using namespace std;
#endif


#define BUFLEN 16384
#define PORT 6006

char command[500], //cDir[200]="/home/mitra/Desktop/Krishna/KrishnaDocs/JessieTwiggerStuff/MakeHistos/MakeLib";
//cDir[200]="/home/adaq/Krishna/SRS";
//cDir[200]="/home/coda/KrishnaStuff/AdaqBackUp/coda/2.6.2/examples/crl";
  cDir[200]="/home/pradrun/coda/2.6.2/extensions/linuxvme/ti/rol";

void StartOrStopSlowControl(int StartOrStop)//1 or 0 for start or stop
{
  int z =0; char buf[32];   z = gethostname(buf,sizeof buf);  
  //kp: 3/20/15 =============== This block here just for backup, you can remove it if you want
  if(strcmp (buf,"srsdaq2") == 0) 
    sprintf(cDir,"/home/adaq/Krishna/SRS");
  if(strcmp (buf,"templehalla1") == 0) 
    sprintf(cDir,"/home/coda/SRS");
  else if(strcmp (buf,"hallavme9") == 0) 
    sprintf(cDir,"/home/coda/KrishnaStuff/AdaqBackUp/coda/2.6.2/examples/crl");
  else if(strcmp (buf,"srsdaq") == 0) 
    sprintf(cDir,"/home/mitra/Desktop/Krishna/KrishnaDocs/JessieTwiggerStuff/MakeHistos/MakeLib");
  //kp: 3/20/15 ===============
  else if(strcmp (buf,"prad.jlab.org") == 0)
    sprintf(cDir,"/home/pradrun/SRS/TemplehallaBkUp/KrishnaStuff/AdaqBackUp/Krishna/SRS");
  else if(strcmp (buf,"pgem.jlab.org") == 0)
    sprintf(cDir,"/home/daq/SRS/TemplehallaBkUp/KrishnaStuff/AdaqBackUp/Krishna/SRS");

  char ToDo[2][20]={"stop","start"};
  if(StartOrStop==1) 
    {
      //sprintf(command,"source %s/ConfigUVA/configSRUpRadUva.sh",cDir);  system(command);
      sprintf(command,"source %s/configureFEC.sh",cDir);  system(command);
    }
  //sprintf(command,"%s/slow_control %s/startKrishna4.txt",cDir,cDir);  system(command);
  //sprintf(command,"source %s/ConfigUVA/%s_pRadUVa.sh",cDir,ToDo[StartOrStop]);  system(command);
  sprintf(command,"source %s/FECconfig/%s_pRad.sh",cDir,ToDo[StartOrStop]);  system(command);
}

#ifndef DISABLE_SLOWCONTROL
#include <signal.h>

void signal_callback_handler(int signum)//http://www.yolinux.com/TUTORIALS/C++Signals.html 
{
  //printf("Caught signal %d\n",signum);
  // Cleanup and close up stuff here
  //PrintColoredWarning("Hi, I think you just pressed 'Ctr+C' command to kill this program.");
  //system("./slow_control stopKrishna4.txt");
  //char command[500]; sprintf(command,"%s/slow_control %s/stopKrishna4.txt",cDir,cDir);  system(command);
  StartOrStopSlowControl(0);//1 or 0 for start or stop
  //PrintColoredWarning("Therefore, for your safety, I executed 'slow_control stopKrishna4.txt'.");
  exit(signum);  // Terminate program
}
#endif

void err(char *str){  //perror(str);  
  exit(1);}

const int ChNum=16; char GemOn[2][20]={"","GemOn"}; 

void createAndBinSocket(int *sockfdArg)
{
  struct sockaddr_in my_addr; int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM,0))==-1)    err("socket");
#ifndef DISABLE_COMMENTS
  else                      printf("Server : Socket() successful\n");
#endif
  bzero(&my_addr, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(PORT);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr))==-1) err("bind");
#ifndef DISABLE_COMMENTS
  else                                      printf("Server : bind() successful\n");
#endif
  //sockfdArg = &sockfd; //address of socfd is sent to sockfdArg pointer
  *sockfdArg = sockfd;   //Equivalently socfd is stored at the address pointed by sockfdArg
}

void receiveData(int sockfd, int *nn, char buf[]) //Didn't work (2/11/15)
{
  //char buf[BUFLEN]; bufPtr=(int*)buf;
  struct sockaddr_in cli_addr;  socklen_t slen=sizeof(cli_addr);  int n;
#ifndef DISABLE_COMMENTS
  printf("f: sizeof(buf)=%d\n",sizeof(buf));
#endif
  bzero(buf,BUFLEN);
  n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&cli_addr, &slen);  *nn = n; 
#ifndef DISABLE_COMMENTS
  if (n>=4) printf("f: Received packet from %s:%d of length %i\n",
		   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), n); 
#endif
}


int listenSRS4CODAv0()
{
  int HowManyEvents=1,channelNo=-1, trigNum=0, sockfd, n; //cli for client?
  createAndBinSocket(&sockfd);

  struct sockaddr_in cli_addr;  socklen_t slen=sizeof(cli_addr);  
  char buf[BUFLEN];  int *ptr; int Index=0;  unsigned int rawdata=0;
 
  while(1)
    {
      ptr = (int*)buf; 
      bzero(buf,BUFLEN); n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&cli_addr, &slen); 
      
      if (n == -1){ //printf("Error"); 
	return 0;}
      if(trigNum==HowManyEvents) break;

      if (n>=4)
	{
#ifndef DISABLE_COMMENTS
	  printf("Received packet from %s:%d of length %i\n",inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), n);
#endif
	  for ( Index=0;Index<n/4;Index++)
	    {
	      rawdata = ptr[Index]; 
#ifndef DISABLE_COMMENTS
	      printf("%d  0x%x\n",Index,rawdata);
#endif
	      if ( rawdata == 0xfafafafa) continue; //event-trailer frame
	      else
		{
		  if (Index == 1)
		    {
		      unsigned int dh1 = ntohl(rawdata);
		      unsigned char c = (dh1 >>24) & 0xff;
		      c = (dh1 >>16) & 0xff;		    c = (dh1 >>8) & 0xff;
		      unsigned int channel = dh1 & 0xff;	    channelNo = (int)channel; 
		      if(channel==15) trigNum++; //==0 is valid for the event-trailer frame too.
		    }
		}
	    }
	}
    }
  close(sockfd);

  return 0;
}


