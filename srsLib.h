#ifndef __SRSLIB__
#define __SRSLIB__
/**************1/21/15: Krishna
 *  
 *
 *****************/

#define MAX_FEC 8

void  srsSlowControl(int enable);
int   srsConnect(int *sockfd);
void  receiveData(int sockfd, int *nn, char buf[]);
int   srsReadBlock(int sockfd, volatile unsigned int* buf_in, int nwrds, int blocklevel);
void  srsSetDebugMode(int enable);

#endif /* __SRSLIB__ */
