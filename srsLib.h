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
