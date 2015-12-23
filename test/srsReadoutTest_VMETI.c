/*
 * File:
 *    srsReadoutTest.c
 *
 * Description:
 *    Program to test the readout of the SRS using a Pipeline TI as
 *    the trigger source
 *
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/signal.h>
#include "jvme.h"
#include "tiLib.h"
#include "srsLib.h"
/* #include "remexLib.h" */

DMA_MEM_ID vmeIN,vmeOUT;
extern DMANODE *the_event;
extern unsigned int *dma_dabufp;
extern int tiA32Base;
int sockfd;
FILE *outfile;
unsigned long long calib=0;


void  mytiISR(int arg);
void sig_handler(int signo);
void closeup();


#define BLOCKLEVEL 1

#define PERIOD 32000

#define DO_READOUT

/**********************************************************************
 *
 * MAIN
 *
 **********************************************************************/

int 
main(int argc, char *argv[]) 
{

  int stat;

  signal(SIGINT,sig_handler);

  printf("\nJLAB SRS Tests\n");
  printf("----------------------------\n");

  unsigned long long before,after,diff=0;

/*   outfile = fopen("out.txt","rw"); */

  before = rdtsc();
  sleep(1);
  after = rdtsc();
  calib = after-before;

  srsSetDebugMode(1);

  vmeOpenDefaultWindows();

  vmeCheckMutexHealth(0);
  /* Setup Address and data modes for DMA transfers
   *   
   *  vmeDmaConfig(addrType, dataType, sstMode);
   *
   *  addrType = 0 (A16)    1 (A24)    2 (A32)
   *  dataType = 0 (D16)    1 (D32)    2 (BLK32) 3 (MBLK) 4 (2eVME) 5 (2eSST)
   *  sstMode  = 0 (SST160) 1 (SST267) 2 (SST320)
   */
  vmeDmaConfig(2,5,1);

  /* INIT dmaPList */

  dmaPFreeAll();
  vmeIN  = dmaPCreate("vmeIN",60*1024,1,0);
  vmeOUT = dmaPCreate("vmeOUT",0,0,0);
    
  dmaPStatsAll();

  dmaPReInitAll();

  if(srsConnect((int*)&sockfd)==0) 
    printf("socket created. ..\n");
  else
    printf("%s: ERROR: Socket to SRS not open\n",
	   __FUNCTION__);

  srsSlowControl(1);

  tiA32Base=0x09000000;
  tiSetFiberLatencyOffset_preInit(0x20);
  tiInit(0,TI_READOUT_EXT_POLL,TI_INIT_SKIP_FIRMWARE_CHECK);
  tiCheckAddresses();

  tiDefinePulserEventType(0xAA,0xCD);

  tiSetSyncEventInterval(0);

  tiSetEventFormat(3);

#ifndef DO_READOUT
  tiDisableDataReadout();
  tiDisableA32();
#endif

  tiLoadTriggerTable(0);
    
  tiSetTriggerHoldoff(1,5,1);
  tiSetTriggerHoldoff(2,0,0);

  tiSetPrescale(0);
  tiSetBlockLevel(BLOCKLEVEL);

  stat = tiIntConnect(TI_INT_VEC, mytiISR, 0);
  if (stat != OK) 
    {
      printf("ERROR: tiIntConnect failed \n");
      goto CLOSE;
    } 
  else 
    {
      printf("INFO: Attached TI Interrupt\n");
    }

  /*     tiSetTriggerSource(TI_TRIGGER_TSINPUTS); */
  tiSetTriggerSource(TI_TRIGGER_PULSER);
  tiEnableTSInput(0x1);

  /*     tiSetFPInput(0x0); */
  /*     tiSetGenInput(0xffff); */
  /*     tiSetGTPInput(0x0); */

  tiSetBusySource(TI_BUSY_LOOPBACK,1);

  tiSetBlockBufferLevel(1);

  tiSetFiberDelay(1,2);
  tiSetSyncDelayWidth(1,0x3f,1);
    
  tiSetBlockLimit(0);

  printf("Hit enter to reset stuff\n");
  getchar();

  tiClockReset();
  taskDelay(1);
  tiTrigLinkReset();
  taskDelay(1);
  tiEnableVXSSignals();

  int again=0;
 AGAIN:
  taskDelay(1);
  tiSyncReset(1);

  taskDelay(1);
    
  tiStatus(1);

  printf("Hit enter to start triggers\n");
  getchar();

  tiIntEnable(0);
  tiStatus(1);
#define SOFTTRIG
#ifdef SOFTTRIG
/*   tiSetRandomTrigger(1,0xf); */
/*   taskDelay(10); */
  tiSoftTrig(1,2,PERIOD,1);
#endif

  printf("Hit any key to Disable TID and exit.\n");
  getchar();
  tiStatus(1);

#ifdef SOFTTRIG
  /* No more soft triggers */
  /*     tidSoftTrig(0x0,0x8888,0); */
  tiSoftTrig(1,0,0x700,0);
  tiDisableRandomTrigger();
#endif

  tiIntDisable();

  tiIntDisconnect();

  if(again==1)
    {
      again=0;
      goto AGAIN;
    }


 CLOSE:
  closeup();

  exit(0);
}

void closeup()
{
  srsSlowControl(0);

/*   fclose(outfile); */
  
  dmaPFreeAll();
  vmeCloseDefaultWindows();

  close(sockfd);
}

/**********************************************************************
 *
 * Interrupt Service routine
 *
 **********************************************************************/

void
mytiISR(int arg)
{
  volatile unsigned short reg;
  int dCnt, len=0,idata;
  DMANODE *outEvent;
  int tibready=0, timeout=0;
  int printout = 10;
  int dataCheck=0;
  unsigned long long before,after,diff=0;
  int ievent=0;

  before = rdtsc();
  tiSetOutputPort(1,0,0,0);
  unsigned int tiIntCount = tiGetIntCount();

#ifdef DO_READOUT
  GETEVENT(vmeIN,tiIntCount);

  dCnt = tiReadTriggerBlock(dma_dabufp);

  if(dCnt<=0)
    {
      printf("No data or error.  dCnt = %d\n",dCnt);
      dataCheck=ERROR;
    }
  else
    {
/*       dataCheck = tiCheckTriggerBlock(dma_dabufp); */
      dma_dabufp += dCnt;
      /*       printf("dCnt = %d\n",dCnt); */
    
    }

  before = rdtsc();
  for(ievent=0; ievent<BLOCKLEVEL; ievent++)
    {
      dCnt = listenSRS4CODAv0(sockfd, dma_dabufp);
      
      if(dCnt<=0)
	{
	  printf("No data or error.  dCnt = %d\n",dCnt);
	  dataCheck=ERROR;
	}
      else
	{
	  printf("SRS dCnt = %d\n",dCnt);
/* 	  dma_dabufp += dCnt; */
	}      
    }
  after = rdtsc();
  diff = after-before;
  PUTEVENT(vmeOUT);

  outEvent = dmaPGetItem(vmeOUT);

#define READOUT
#ifdef READOUT
  if(tiIntCount%printout==0)
    {
      printf("Received %d triggers...\n",
	     tiIntCount);

      len = outEvent->length;
      
      for(idata=0;idata<len;idata++)
	{
	  if((idata%5)==0) printf("\n\t");
	  printf("  0x%08x ",(unsigned int)LSWAP(outEvent->data[idata]));
	}
      printf("\n\n");
      printf(" Time = %lld / %lld = %.2f\n",diff,calib,1000000.*(float)diff/(float)calib);
    }
#endif

  dmaPFreeItem(outEvent);

/* #else /\* DO_READOUT *\/ */
/*     tiResetBlockReadout(); */
#endif /* DO_READOUT */
  if(tiIntCount%printout==0)
    printf("intCount = %d\n",tiIntCount );
/*     sleep(1); */

  if(dataCheck!=OK)
    {
      printf("stop this crazy thing\n");
      tiSetBlockLimit(0);
    }

  tiSoftTrig(1,2,PERIOD,1);
 
}


void sig_handler(int signo)
{
  int i, status;

  switch (signo) {
  case SIGINT:
    printf("\n\n");
    closeup();
    exit(1);  /* exit if CRTL/C is issued */
  }
  return;
}
