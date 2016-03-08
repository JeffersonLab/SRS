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

#define BLOCKLEVEL  1
#define BUFFERLEVEL 0
#define DO_READOUT
#define SOFTTRIG
#define USEDMA 0

unsigned long long calib=0;
#define TI_DATA_SIZE  120
#define SRS_DATA_SIZE (1024*80*2)
volatile unsigned int TIdata[TI_DATA_SIZE], SRSdata[SRS_DATA_SIZE];
int srsFD[MAX_FEC];
char FEC[MAX_FEC][100];
int nfec=1;

#define DO(x) {					\
  if(x==-1)					\
    printf("%s: ERROR returned from\n  %s\n",	\
	   __FUNCTION__,#x);			\
  }


/* ISR Prototype */
void mytiISR(int arg);
void sig_handler(int signo);
void closeup();
extern unsigned long long int rdtsc(void);
int triggerholdoff = 1;
int PERIOD=1;

unsigned int chEnable = 9;

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
  tipInit(TIP_READOUT_EXT_POLL,USEDMA?TIP_INIT_USE_DMA:0);
  tipCheckAddresses();

  tipDefinePulserEventType(0xAA,0xCD);

#ifndef DO_READOUT
  tipDisableDataReadout(0);
#endif

  tipLoadTriggerTable(0);
    
  tipSetTriggerHoldoff(1,triggerholdoff,2);
  tipSetTriggerHoldoff(2,0,2);
  tipSetTriggerHoldoff(3,0,2);
  tipSetTriggerHoldoff(4,0,2);

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
      printf("INFO: Attached TI Interrupt\a\n");
    }

#ifdef SOFTTRIG
  tipSetTriggerSource(TIP_TRIGGER_PULSER);
#else
  tipSetTriggerSource(TIP_TRIGGER_TSINPUTS);
#endif
  tipEnableTSInput(0xf);

  /*     tiSetFPInput(0x0); */
  /*     tiSetGenInput(0xffff); */
  /*     tiSetGTPInput(0x0); */

  tipSetBusySource(TIP_BUSY_LOOPBACK|TIP_BUSY_FP ,1);

  tipSetBlockBufferLevel(BUFFERLEVEL);

/*   tiSetFiberDelay(1,2); */
/*   tiSetSyncDelayWidth(1,0x3f,1); */
    
  tipSetBlockLimit(0);

/**********************************************************************
 * SRS Setup
 **********************************************************************/
  /* srsSetDebugMode(1); */


  nfec=0;
  strncpy(FEC[nfec++], "10.0.0.2",100);
  strncpy(FEC[nfec++], "10.0.1.2",100);
  /* strncpy(FEC[nfec++], "10.0.3.2",100); */
  strncpy(FEC[nfec++], "10.0.4.2",100);
  /* strncpy(FEC[nfec++], "10.0.5.2",100); */
  /* strncpy(FEC[nfec++], "10.0.7.2",100); */

  char hosts[MAX_FEC][100];
  int ifec=0;
  int starting_port = 7000;

  memset(&srsFD, 0, sizeof(srsFD));

  for(ifec=0; ifec<nfec; ifec++)
    {
      sprintf(hosts[ifec],"10.0.0.%d",ifec+3);
      DO(srsSetDAQIP(FEC[ifec], hosts[ifec], starting_port+ifec));
      DO(srsConnect((int*)&srsFD[ifec], hosts[ifec], starting_port+ifec));
    }


  for(ifec=0; ifec<nfec; ifec++)
    {

      /* Same as call to 
	 srsExecConfigFile("config/set_IP10012.txt"); */
      DO(srsSetDTCC(FEC[ifec], 
		 1, // int dataOverEth
		 0, // int noFlowCtrl 
		 2, // int paddingType
		 0, // int trgIDEnable
		 0, // int trgIDAll
		 4, // int trailerCnt
		 0xaa, // int paddingByte
		 0xdd  // int trailerByte
		    ));

      /* Same as call to 
	 srsExecConfigFile("config/adc_IP10012.txt"); */
      DO(srsConfigADC(FEC[ifec],
		   0xffff, // int reset_mask
		   0, // int ch0_down_mask
		   0, // int ch1_down_mask
		   0, // int eq_level0_mask
		   0, // int eq_level1_mask
		   0, // int trgout_enable_mask
		   0xffff // int bclk_enable_mask
		      ));

      /* Same as call to 
	 srsExecConfigFile("config/fecCalPulse_IP10012.txt"); */
      DO(srsSetApvTriggerControl(FEC[ifec],
			      4+2, // int mode
			      4, // int trgburst (x+1)*3 time bins
			      0x4, // int freq
			      0x100, // int trgdelay
			      0x7f, // int tpdelay
			      0x12e // int rosync
				 ));
      DO(srsSetEventBuild(FEC[ifec],
			  (1<<chEnable)-1, // int chEnable
			  2300, // int dataLength
			  /* 500, // int dataLength */
			  2, // int mode
			  0, // int eventInfoType
			  0xaa000bb8 | (ifec<<16) // unsigned int eventInfoData
			  ));
	
      /* Same as call to 
	 srsExecConfigFile("config/apv_IP10012.txt"); */
      DO(srsAPVConfig(FEC[ifec], 
		   0xff, // int channel_mask, 
		   0x03, // int device_mask,
		   0x19, // int mode, 
		   0x80, // int latency, 
		   0x2, // int mux_gain, 
		   0x62, // int ipre, 
		   0x34, // int ipcasc, 
		   0x22, // int ipsf, 
		   0x22, // int isha, 
		   0x22, // int issf, 
		   0x37, // int ipsp, 
		   0x10, // int imuxin, 
		   0x64, // int ical, 
		   0x28, // int vsps,
		   0x3c, // int vfs, 
		   0x1e, // int vfp, 
		   0xef, // int cdrv, 
		   0xf7 // int csel
		      ));

      /* DO(srsExecConfigFile("config/apv_IP10012.txt")); */


      /* Same as call to 
	 srsExecConfigFile("/daqfs/home/moffit/work/SRS/test/config/fecAPVreset_IP10012.txt"); */
      DO(srsAPVReset(FEC[ifec]));

      /* Same as call to 
	 srsExecConfigFile("config/pll_IP10012.txt"); */
      DO(srsPLLConfig(FEC[ifec], 
		   0xff, // int channel_mask,
		   0, // int fine_delay, 
		   0 // int trg_delay
		      ));
      
    }

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

  for(ifec=0; ifec<nfec; ifec++)
    {
      srsTrigEnable(FEC[ifec]);
      srsStatus(FEC[ifec],0);
    }


  printf("Hit enter to start triggers\n");
  getchar();


  tipIntEnable(0);
  tipStatus(1);
  /* tipPCIEStatus(1); */

#ifdef SOFTTRIG
  tipSetRandomTrigger(1,0x7);
  /* taskDelay(10); */
  /* tipSoftTrig(1,0xffff,PERIOD,0); */
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

  for(ifec=0; ifec<nfec; ifec++)
    {
      srsTrigDisable(FEC[ifec]);
      srsStatus(FEC[ifec],0);
    }

 CLOSE:
  closeup();
  exit(0);
}

void closeup()
{
  int ifec=0;
  for(ifec=0; ifec<nfec; ifec++)
    {
      srsStatus(FEC[ifec],0);
    }

  sleep(1);
  for(ifec=0; ifec<nfec; ifec++)
    {
      if(srsFD[ifec])
	close(srsFD[ifec]);
    }

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
  int dCnt_ti=0, dCnt_srs=0, dCnt_srs_total=0, frameCnt=0, len=0,idata;
  int printout = 100;
  int dataCheck=0;
  unsigned long long before,after,diff=0;
  int ifec=0;
  unsigned int tiIntCount = tipGetIntCount();

  dCnt_ti = tipReadBlock((volatile unsigned int *)&TIdata,
			 TI_DATA_SIZE,USEDMA?1:0);
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
  for(ifec=0; ifec<nfec; ifec++)
    {
      /* srsSetDebugMode(1); */
      dCnt_srs = srsReadBlock(srsFD[ifec], 
			      (volatile unsigned int *)&SRSdata[dCnt_srs_total],
			      SRS_DATA_SIZE,BLOCKLEVEL,&frameCnt);
      srsSetDebugMode(0);
      if(dCnt_srs<=0)
	{
	  printf("**************************************************\n");
	  printf("SRS: No data or error.  dCnt = %d\a\n",dCnt_srs);
	  printf("**************************************************\n");
	  dataCheck=ERROR;
	}
      else
	{
	  /* if(dCnt_srs!=(3481*BLOCKLEVEL)) */
	  if(frameCnt!=((chEnable+1)*BLOCKLEVEL))
	    {
	      dataCheck=ERROR;
	      printf("SRS frameCnt = %2d dCnt = %6d ***************************\n",
		     frameCnt, dCnt_srs);
	    }

	  dCnt_srs_total += dCnt_srs;
	}      
    }
  after = rdtsc();
  diff = after-before;

  if(tiIntCount%printout==0)
    {
      printf("Received %d triggers...\n",
	     tiIntCount);
      /* tipSoftTrig(1,0xffff,PERIOD,0); */

/* #define PRINTOUT_TI */
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
      len = dCnt_srs_total;
      int icounter=0;
      int samples=0;
      unsigned int tmpData=0;
      
      if(dataCheck!=OK)
	{
	  printf("srs len = %d\n",len);
	  for(idata=0;idata<(len);idata++)
	    {

	      if ((SRSdata[idata]&0xffffff)==0x434441)
		icounter=0;

	      if((icounter%8)==0) 
		printf("\n%4d\t",icounter);

	      if(icounter==1)
		samples = (LSWAP(SRSdata[idata])) & 0xffff;

	      if (((icounter-2)*2)==samples)
		{
		  icounter=0;
		  printf("\n");
		}

	      if(icounter<2)
		printf("  0x%08x ",(unsigned int)LSWAP(SRSdata[idata]));
	      else
		{
		  tmpData = (unsigned int)LSWAP(SRSdata[idata]);
		  printf("   %04x %04x ",
			 (tmpData&0xFF000000)>>24 | (tmpData&0x00FF0000)>>8, 
			 (tmpData&0x0000FF00)>>8 | (tmpData&0x000000FF)<<8
			 );
		}
	      icounter++;
	    }
	  printf("\n\n");
	  tipSetBlockLimit(1);
	}
#endif /* PRINTOUT_TI */
    }


  return;

  if(dataCheck!=OK)
    {
      /* Change the trigger rule */
      tipDisableTriggerSource(0);

      /* tipSetTriggerHoldoff(1,++triggerholdoff,2); */
      /* printf(" Changed triggerholdoff to %d (%.1f usec)\n", */
      /* 	     triggerholdoff, (float)triggerholdoff*5.120); */

      return;
      PERIOD += 1;
      printf(" Changed period to %d (%.1f usec)\n",
      	     PERIOD, (32.0+8.0*(float)PERIOD)/1000.);
      
      usleep(100000);
      tipEnableTriggerSource();
    }


  extern int tipDoAck;
  
  tipIntAck();
  tipSoftTrig(1,BLOCKLEVEL,PERIOD,1);
  tipDoAck=0;

}

