/*
 * File:
 *    srsLibTest.c
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
#include "srsLib.h"

int sockfd=0;
void sig_handler(int signo);
void closeup();

/**********************************************************************
 *
 * MAIN
 *
 **********************************************************************/

int 
main(int argc, char *argv[]) 
{

  int stat;
  unsigned int buf[32];
  unsigned int list[3] = {0x3, 0xa, 0xf};
  /* signal(SIGINT,sig_handler); */

  printf("\nJLAB SRS Tests\n");
  printf("----------------------------\n");

  /* srsSlowControl(1); */

  /* srsSlowControl(0); */

  srsSetDebugMode(1);

  /* srsExecConfigFile("config/set_IP10012.txt");sys */
  /* srsExecConfigFile("config/adc_IP10012.txt"); */
  /* srsExecConfigFile("config/fecCalPulse_IP10012.txt"); */
  /* srsExecConfigFile("config/apv_IP10012.txt"); */
  /* srsExecConfigFile("config/fecAPVreset_IP10012.txt"); */
  /* srsExecConfigFile("config/pll_IP10012.txt"); */

  srsStatus("10.0.7.2",0);

 CLOSE:
  closeup();

  exit(0);
}

void closeup()
{
/*   srsSlowControl(0); */
  if(sockfd)
    close(sockfd);
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
