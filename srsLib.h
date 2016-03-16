#ifndef __SRSLIB__
#define __SRSLIB__
/*----------------------------------------------------------------------------*/
/**
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
 *     Header file for SRS Library
 *
 *----------------------------------------------------------------------------*/

#define MAX_FEC 8

#define SRS_SYS_PORT     6007
#define SRS_APVAPP_PORT  6039
#define SRS_APV_PORT     6263
#define SRS_ADCCARD_PORT 6519

#define SRS_APV_ALLAPV_MASK 0xFF03
#define SRS_APV_ALLPLL_MASK 0xFF00


int   srsInit(char *fecip, char *hostip);
int   srsClose(int id);
int   srsConnect(int *sockfd, char *ip, int port);
int   srsStatus(int id, int pflag);
int   srsReadBlock(int sockfd, volatile unsigned int* buf_in, int nwrds, int blocklevel,
		   int *frameCnt);

int   srsSetDAQIP(int id, char *daq_ip, int port);
int   srsSetDTCClk(int id, int dtcclk_inh, int dtctrg_inh, 
		   int dtc_swapports, int dtc_swaplanes, int dtctrg_invert);
int   srsSetDTCC(int id, int dataOverEth, int noFlowCtrl, int paddingType,
		 int trgIDEnable, int trgIDAll, int trailerCnt, int paddingByte, int trailerByte);
int   srsConfigADC(int id,
		  int reset_mask, int ch0_down_mask, int ch1_down_mask,
		  int eq_level0_mask, int eq_level1_mask, int trgout_enable_mask,
		  int bclk_enable_mask);
int   srsSetApvTriggerControl(int id,
			     int mode, int trgburst, int freq,
			     int trgdelay, int tpdelay, int rosync);
int   srsSetEventBuild(int id,
		       int chEnable, int dataLength, int mode, 
		       int eventInfoType, unsigned int eventInfoData);
int   srsTrigEnable(int id);
int   srsTrigDisable(int id);
int   srsAPVReset(int id);
int   srsAPVConfig(int id, int channel_mask, int device_mask,
		   int mode, int latency, int mux_gain, 
		   int ipre, int ipcasc, int ipsf, 
		   int isha, int issf, int ipsp, 
		   int imuxin, int ical, int vsps,
		   int vfs, int vfp, int cdrv, int csel);
int   srsPLLConfig(int id, int channel_mask,
		   int fine_delay, int trg_delay);
void  srsSetDebugMode(int enable);

int   srsReadList(char *ip, int port, 
		  unsigned int sub_addr, unsigned int *list, int nlist,
		  unsigned int *data, int maxwords);
int   srsReadBurst(char *ip, int port, 
		   unsigned int sub_addr, unsigned int first_addr, int nregs,
		   unsigned int *data, int maxwords);
int   srsWriteBurst(char *ip, int port, 
		    unsigned int sub_addr, unsigned int first_addr, int nregs,
		    unsigned int *wdata, int maxwords);
int   srsWritePairs(char *ip, int port, 
		    unsigned int sub_addr, unsigned int *addr, int nregs,
		    unsigned int *wdata, int maxwords);
int   srsExecConfigFile(char *filename);
#endif /* __SRSLIB__ */
