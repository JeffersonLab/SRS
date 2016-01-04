#ifndef __SRSLIB__
#define __SRSLIB__
/**************1/21/15: Krishna
 *  
 *
 *****************/

#define MAX_FEC 8


int   srsConnect(int *sockfd);
int   srsReadBlock(int sockfd, volatile unsigned int* buf_in, int nwrds, int blocklevel);
void  srsSetDebugMode(int enable);

int   srsSetDAQIP(char *ip, char *daq_ip);
int   srsSetDTCC(char *ip, int dataOverEth, int noFlowCtrl, int paddingType,
		 int trgIDEnable, int trgIDAll, int trailerCnt, int paddingByte, int trailerByte);
int   srsConfigADC(char *ip,
		  int reset_mask, int ch0_down_mask, int ch1_down_mask,
		  int eq_level0_mask, int eq_level1_mask, int trgout_enable_mask,
		  int bclk_enable_mask);
int   srsSetApvTriggerControl(char *ip,
			     int mode, int trgburst, int freq,
			     int trgdelay, int tpdelay, int rosync);
int   srsSetEventBuild(char *ip,
		       int chEnable, int dataLength, int mode, 
		       int eventInfoType, unsigned int eventInfoData);
int   srsTrigEnable(char *ip);
int   srsTrigDisable(char *ip);
int   srsAPVReset(char *ip);

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
