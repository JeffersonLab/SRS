/*
 * File:
 *    srsReadoutTest_TIpcie
 *
 * Description:
 *    Test SRS readout with Linux Driver
 *    and TIpcie library
 *
 *
 */


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/signal.h>
#include "TIpcieLib.h"
#include "srsLib.h"
#include "byteswap.h"
/* #include "remexLib.h" */

#define BLOCKLEVEL 0x2
#define PERIOD 16
#define DO_READOUT

int srsFD;
unsigned long long calib=0;
#define TI_DATA_SIZE  120
#define SRS_DATA_SIZE (20*1024)
volatile unsigned int TIdata[TI_DATA_SIZE], SRSdata[SRS_DATA_SIZE];



/* ISR Prototype */
void mytiISR(int arg);
void sig_handler(int signo);
void closeup();
extern unsigned long long int rdtsc(void);


int 
main(int argc, char *argv[]) 
{
  int stat;
  unsigned long long before,after,diff=0;

  before = rdtsc();
  sleep(1);
  after = rdtsc();
  calib = after-before;
  signal(SIGINT,sig_handler);

  printf("\nJLAB TI Tests\n");
  printf("----------------------------\n");

/*   remexSetCmsgServer("dafarm28"); */
/*   remexInit(NULL,1); */
  

/**********************************************************************
 * TI SETUP
 **********************************************************************/
  tipOpen();

  /* Set the TI structure pointer */
  tipInit(TIP_READOUT_EXT_POLL,0);
  tipCheckAddresses();

  tipDefinePulserEventType(0xAA,0xCD);

#ifndef DO_READOUT
  tipDisableDataReadout(0);
#endif

  tipLoadTriggerTable(0);
    
  /* tipSetTriggerHoldoff(1,4,0); */
  /* tipSetTriggerHoldoff(2,4,0); */

  tipSetPrescale(0);
  tipSetBlockLevel(BLOCKLEVEL);

  stat = tipIntConnect(TIP_INT_VEC, mytiISR, 0);
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
  tipSetTriggerSource(TIP_TRIGGER_PULSER);
  tipEnableTSInput(0x1);

  /*     tiSetFPInput(0x0); */
  /*     tiSetGenInput(0xffff); */
  /*     tiSetGTPInput(0x0); */

  tipSetBusySource(TIP_BUSY_LOOPBACK ,1);

  tipSetBlockBufferLevel(1);

/*   tiSetFiberDelay(1,2); */
/*   tiSetSyncDelayWidth(1,0x3f,1); */
    
  tipSetBlockLimit(0);

/**********************************************************************
 * SRS Setup
 **********************************************************************/

  if(srsConnect((int*)&srsFD)==0) 
    printf("socket created. .. (%d)\n",srsFD);
  else
    printf("%s: ERROR: Socket to SRS not open\n",
	   __FUNCTION__);

  srsExecConfigFile("config/set_IP10012.txt");
  srsExecConfigFile("config/adc_IP10012.txt");
  srsExecConfigFile("config/fecCalPulse_IP10012.txt");
  srsExecConfigFile("config/apv_IP10012.txt");
  srsExecConfigFile("config/fecAPVreset_IP10012.txt");
  srsExecConfigFile("config/pll_IP10012.txt");

  printf("Hit enter to reset stuff\n");
  getchar();

  /* tipClockReset(); */
  /* usleep(10000); */
  tipTrigLinkReset();
  usleep(10000);

  usleep(10000);
  tipSyncReset(1);

  usleep(10000);
    
  tipStatus(1);
  tipPCIEStatus(1);

  srsExecConfigFile("config/start_IP10012.txt");

  srsStatus("10.0.1.2",0);


  printf("Hit enter to start triggers\n");
  getchar();


  tipIntEnable(0);
  tipStatus(1);
  /* tipPCIEStatus(1); */

#define SOFTTRIG
#ifdef SOFTTRIG
  /* tipSetRandomTrigger(1,0x7); */
  /* taskDelay(10); */
  tipSoftTrig(1,2,PERIOD,1);
#endif

  printf("Hit any key to Disable TID and exit.\n");
  getchar();
  tipStatus(1);
  /* tipPCIEStatus(1); */

#ifdef SOFTTRIG
  /* No more soft triggers */
  /*     tidSoftTrig(0x0,0x8888,0); */
  tipSoftTrig(1,0,0x700,0);
  tipDisableRandomTrigger();
#endif

  tipIntDisable();

  tipIntDisconnect();

  srsExecConfigFile("config/stop_IP10012.txt");
  srsStatus("10.0.1.2",0);

 CLOSE:
  closeup();
  exit(0);
}

void closeup()
{
  srsExecConfigFile("config/stop_IP10012.txt");
  sleep(1);
  close(srsFD);

  tipClose();

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

/* Interrupt Service routine */
void
mytiISR(int arg)
{
  int dCnt_ti=0, dCnt_srs=0, len=0,idata;
  int printout = 1;
  int dataCheck=0;
  unsigned long long before,after,diff=0;
  int ievent=0;
  unsigned int tiIntCount = tipGetIntCount();

  dCnt_ti = tipReadBlock((volatile unsigned int *)&TIdata,
			 TI_DATA_SIZE,0);
  /* dCnt_ti = tipReadTriggerBlock((volatile unsigned int *)&data); */

  if(dCnt_ti<=0)
    {
      printf("**************************************************\n");
      printf("TI: No data or error.  dCnt = %d\n",dCnt_ti);
      printf("**************************************************\n");
      dataCheck=ERROR;
    }
  else
    {
      /* dataCheck = tiCheckTriggerBlock(data); */
    }

  before = rdtsc();
  /* for(ievent=0; ievent<BLOCKLEVEL; ievent++) */
    {
      dCnt_srs = srsReadBlock(srsFD, 
			      (volatile unsigned int *)&SRSdata,
			      SRS_DATA_SIZE,BLOCKLEVEL);
      if(dCnt_srs<=0)
	{
	  printf("**************************************************\n");
	  printf("SRS: No data or error.  dCnt = %d\n",dCnt_srs);
	  printf("**************************************************\n");
	  dataCheck=ERROR;
	}
      else
	{
	  printf("SRS dCnt = %d\n",dCnt_srs);
	}      
    }
  after = rdtsc();
  diff = after-before;

  if(tiIntCount%printout==0)
    {
      printf("Received %d triggers...\n",
	     tiIntCount);

#define PRINTOUT_TI
#ifdef PRINTOUT_TI
      len = dCnt_ti;
      
      for(idata=0;idata<(len);idata++)
	{
	  if((idata%4)==0) printf("\n\t");
	  printf("  0x%08x ",(unsigned int)(TIdata[idata]));
	}
      printf("\n\n");
#endif /* PRINTOUT_TI */

#define PRINTOUT_SRS
#ifdef PRINTOUT_SRS
      len = dCnt_srs;
      
      printf("srs len = %d\n",len);
      for(idata=0;idata<(len);idata++)
	{
	  if((idata%4)==0) printf("\n\t");
	  printf("  0x%08x ",(unsigned int)(SRSdata[idata]));
	}
      printf("\n\n");
#endif /* PRINTOUT_TI */
    }

  if(dataCheck!=OK)
    {
      getchar();
    }

  getchar();
  tipSoftTrig(1,2,PERIOD,1);

}

