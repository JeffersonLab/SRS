/*************************************************************************
 *
 *  vme_list.c - Library of routines for readout and buffering of 
 *                events using a JLAB Trigger Interface V3 (TI) with 
 *                a Linux VME controller.
 *
 *
 *  When the trigger for SRS went directly from the generator rather than from TI, 
 *    the event rate went and stayed stable upto about 2.4 kHz, but for a trigger
 *    rate above 2.4 kHz, the system didn't crash but the event rate went down 
 *    to as low as about 1.4 kHz.
 *
 */

/* Event Buffer definitions */
#define MAX_EVENT_POOL     300
//#define MAX_EVENT_LENGTH   1024*60      /* Size in Bytes */
#define MAX_EVENT_LENGTH  81920  /*81920*1000 */ /*ka: our SRS data packet is very big*/

/* Define Interrupt source and address */
#define TI_MASTER
#define TI_READOUT TI_READOUT_EXT_POLL /*Polling mode, external triggers*/
#define TI_ADDR    0x100000  /* slot 2 */

/* Decision on whether or not to readout the TI for each block 
   - Comment out to disable readout 
*/
//#define TI_DATA_READOUT
#define  SRS_DATA_READOUT
//#define  SRS_DATA_PRINTOUT  
//#define SRS_DEBUG


//TSINPUTS as trigger, define USE_PULSER to use internal pulser
//#define USE_PULSER

#define FIBER_LATENCY_OFFSET 0x4A  /* measured longest fiber length */

/* #include "dmaBankTools.h" */
#include "tipcieprimary_list.c" /* source required for CODA */


/* #include "libSRS.h" */
/*const SRS_bank   = 0x201;*/

/* Default block level */
unsigned int BLOCKLEVEL=1;
#define BUFFERLEVEL 1

/* Redefine tsCrate according to TI_MASTER or TI_SLAVE */
#ifdef TI_SLAVE
int tsCrate=0;
#else
#ifdef TI_MASTER
int tsCrate=1;
#endif
#endif



#ifdef SRS_DATA_READOUT
int sockfd; /*For use below in createAndBinSocket(&sockfd) and recvfrom(sockfd, buf,..)*/
//ka: ======== 11/16/15 (want to do it globally and only once) (Moved here from rocTrigger())
struct sockaddr_in cli_addr;  socklen_t slen=sizeof(cli_addr);  
int BUFFLEN = 16384; char buf[BUFLEN];  int *ptr=(int*)buf; //int *ptr; 
//ka: ======== 11/16/15 (want to do it globally and only once) (Moved here from rocTrigger())
#endif

/* function prototype */
void rocTrigger(int arg);

/****************************************
 *  DOWNLOAD
 ****************************************/
void
rocDownload()
{

  /* Setup Address and data modes for DMA transfers
   *   
   *  vmeDmaConfig(addrType, dataType, sstMode);
   *
   *  addrType = 0 (A16)    1 (A24)    2 (A32)
   *  dataType = 0 (D16)    1 (D32)    2 (BLK32) 3 (MBLK) 4 (2eVME) 5 (2eSST)
   *  sstMode  = 0 (SST160) 1 (SST267) 2 (SST320)
   */
  vmeDmaConfig(1,5,1); 

  /*****************
   *   TI SETUP
   *****************/
  int overall_offset=0x80;

#ifndef TI_DATA_READOUT
  /* Disable data readout */
  tiDisableDataReadout();
  /* Disable A32... where that data would have been stored on the TI */
  tiDisableA32();
#endif

  /* Set crate ID */
  tiSetCrateID(0x01); /* ROC 1 */
  
#ifdef USE_PULSER
    tiSetTriggerSource(TI_TRIGGER_PULSER);
#else
    tiSetTriggerSource(TI_TRIGGER_TSINPUTS);
#endif
  /* Set needed TS input bits */
  tiEnableTSInput( TI_TSINPUT_1 | TI_TSINPUT_2 );

  /* Load the trigger table that associates 
     pins 21/22 | 23/24 | 25/26 : trigger1
     pins 29/30 | 31/32 | 33/34 : trigger2
  */
  tiLoadTriggerTable(0);

  tiSetTriggerHoldoff(1,10,0);
  tiSetTriggerHoldoff(2,10,0);

/*   /\* Set the sync delay width to 0x40*32 = 2.048us *\/ */
  tiSetSyncDelayWidth(0x54, 0x40, 1);

  /* Set the busy source to non-default value (no Switch Slot B busy) */
  tiSetBusySource(TI_BUSY_LOOPBACK,1);

/*   tiSetFiberDelay(10,0xcf); */

#ifdef TI_MASTER
  /* Set number of events per block */
  tiSetBlockLevel(BLOCKLEVEL);
#endif

  tiSetEventFormat(1);

  tiSetBlockBufferLevel(BUFFERLEVEL);

  tiStatus(0);

  printf("rocDownload: User Download Executed\n");

}

/****************************************
 *  PRESTART
 ****************************************/
void
rocPrestart()
{
  unsigned short iflag;
  int stat;
  int islot;

  tiStatus(0);

  printf("rocPrestart: User Prestart Executed\n");

#ifdef SRS_DATA_READOUT
  StartOrStopSlowControl(1);//1 or 0 for start or stop SRS
  printf("SRS FEC configuration script also executed..\n");

  //Create the socket for UDP connection with SRS (sockfd is defined globally above)
  if(srsConnect(&sockfd)==0) 
    printf("socket created. ..\n");
  else
    printf("%s: ERROR: Socket to SRS not open\n",
	   __FUNCTION__);
#endif
}

/****************************************
 *  GO
 ****************************************/
void
rocGo()
{
  int islot;
  /* Enable modules, if needed, here */

  /* Get the current block level */
  BLOCKLEVEL = tiGetCurrentBlockLevel();
  printf("%s: Current Block Level = %d\n",
	 __FUNCTION__,BLOCKLEVEL);

  /* Use this info to change block level is all modules */

#ifdef USE_PULSER // Start software trigger
  //  tiSetRandomTrigger(1,0x7);
    tiSoftTrig(1,3000,0x1123,1);
#endif

}

/****************************************
 *  END
 ****************************************/
void
rocEnd()
{

  int islot;
  
  tiStatus(0);
  
#ifdef USE_PULSER // Stop the internal pulser
    tiSoftTrig(1,0,0x1123,1);
   // tiDisableRandomTrigger();
#endif

#ifdef SRS_DATA_READOUT
  close(sockfd); 
  StartOrStopSlowControl(0);
  printf("rocEnd: Ended after %d blocks\n",tiGetIntCount());
#endif

}

/****************************************
 *  TRIGGER
 ****************************************/
void
rocTrigger(int arg)
{
  int ii, islot;
  int stat, dCnt, len=0, idata;
#ifdef SRS_DATA_READOUT
  int HowManyEvents=1,channelNo=-1, trigNum=0, bytes_received; //sockfd, //cli for client?
  //createAndBinSocket(&sockfd);
  //ka: ======== 11/16/15 (want to do it globally and only once)
  //struct sockaddr_in cli_addr;  socklen_t slen=sizeof(cli_addr);  
  //int BUFFLEN = 16384; char buf[BUFLEN];  int *ptr=(int*)buf; //int *ptr; 
  //ka: ======== 11/16/15 (want to do it globally and only once)
  int Index=0;  unsigned int rawdata=0;
#ifdef SRS_DATA_PRINTOUT 
  printf("ka: Received a trigger 10/16/15 \n");
#endif
#endif  //===================ka: 10/14/15


  //tiSetOutputPort(1,0,0,0); //BM

#ifdef TI_DATA_READOUT
  BANKOPEN(4,BT_UI4,0);

  vmeDmaConfig(1,5,1); 
  //tiSetOutputPort(1,1,0,0);//BM
  dCnt = tiReadBlock(dma_dabufp,8+(3*BLOCKLEVEL),1);
  if(dCnt<=0)
    {
      printf("No data or error.  dCnt = %d\n",dCnt);
    }
  else
    {
      dma_dabufp += dCnt;
    }
  //tiSetOutputPort(1,0,0,0);//BM

  BANKCLOSE;
#endif

#ifdef SRS_DATA_READOUT  
  BANKOPEN(6,BT_UI4,0);  //ka  open SRS_bank
  *dma_dabufp++ = LSWAP(0xda000022);  //output hex da000022
  tiSetOutputPort(1,0,1,0);

  while(1)
    {
      rawdata=0;	
      //ptr = (int*)buf;  //ka: 11/16/15 (want to do it globally and only once)
      //bzero(buf,BUFLEN);                //????????????????????????
      bytes_received = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&cli_addr, &slen); 
#ifdef SRS_DATA_PRINTOUT  
      printf("n=%d   ",bytes_received);
#endif      
      //if (bytes_received == -1) return; //????????????????????????

#ifdef SRS_DEBUG
      if(trigNum==HowManyEvents) break;
#endif

      if (bytes_received>=4)
	{
	  for ( Index=0;Index<bytes_received/4;Index++)
	    {
	      rawdata = ptr[Index];  
#ifdef SRS_DATA_PRINTOUT  
	      printf("%d  rawdata=0x%08x\n",Index,rawdata);
#endif
	      //*rol->dabufp++ = rawdata;
	      *dma_dabufp++ = rawdata;
	      //*dma_dabufp++ = ptr[Index];

#ifdef SRS_DEBUG	      
	      if ( rawdata == 0xfafafafa) break; //continue; //event-trailer frame
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
#endif
	    }
	}
      if ( rawdata == 0xfafafafa) break; //????????????????????????
    }
  //close(sockfd); 
  //StartOrStopSlowControl(0);//1 or 0 for start or stop
  BANKCLOSE;                    //ka: close SRS_bank
#endif

  tiSetOutputPort(0,0,0,0);

}

void
rocCleanup()
{
  srsDisconnect(sockfd);
  StartOrStopSlowControl(0);
}
