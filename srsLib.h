#ifndef __SRSLIB__
#define __SRSLIB__
/**************1/21/15: Krishna
 *  
 *
 *****************/

void  srsSlowControl(int enable);
int   srsConnect(int *sockfd);
void  receiveData(int sockfd, int *nn, char buf[]);
int   listenSRS4CODAv0(int sockfd, volatile unsigned int* buf, int nwrds);
void  srsSetDebugMode(int enable);

#endif /* __SRSLIB__ */
