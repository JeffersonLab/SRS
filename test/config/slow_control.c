#include "net.h"

#define SIZE 65504

int g_sockfd,g_sockfd2,g_n,g_fd;
socklen_t g_len,g_servlen;
struct sockaddr_in g_servaddr,g_servaddr2;
struct sockaddr_in g_cliaddr;
struct sockaddr *g_replyaddr;

int g_broadcast = 1;
pid_t g_pid;
char g_buffer[SIZE],g_buf[SIZE];
int g_byte = 0;
char g_ip[100];
int g_port = 0;

/**** These two functions are used to handle the name or the IP of the machine ****/
struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype) {
  int n;
  struct addrinfo hints, *res;
  
  bzero(&hints,sizeof(struct addrinfo));
  hints.ai_flags = AI_CANONNAME; // returns canonical name
  hints.ai_family = family; // AF_UNSPEC AF_INET AF_INET6 etc...
  hints.ai_socktype = socktype; // 0 SOCK_STREAM SOCK_DGRAM

  if ((g_n = getaddrinfo(host,serv,&hints,&res)) != 0) return NULL;
  
  return (res);    
}

char* sock_ntop_host(const struct sockaddr *sa, socklen_t salen) {
  static char str[128];		/* Unix domain is largest */
  
  switch (sa->sa_family) {
  case AF_INET: {
    struct sockaddr_in	*sin = (struct sockaddr_in *) sa;
    
    if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
      return(NULL);
    return(str);
  }
  default:
    snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
	     sa->sa_family, salen);
    return(str);
  }
  return (NULL);
}


//********************** create a socket ******************************************
void PrepareSocket (char *ip_addr,int port_number) {
  struct addrinfo *ai;
  char *h;
  g_sockfd = socket(AF_INET,SOCK_DGRAM,0);  
  if (g_broadcast == 1) {
    const int on = 1;
    setsockopt(g_sockfd,SOL_SOCKET,SO_BROADCAST,&on,sizeof(on));
  }
  bzero(&g_servaddr,sizeof(g_servaddr));
  g_servaddr.sin_family = AF_INET;
  //
  // set the source port
  //
  g_servaddr.sin_port = htons(6007);
  bind(g_sockfd,(struct sockaddr *)& g_servaddr, sizeof(g_servaddr));
  //
  // set the destination port
  //
  g_servaddr.sin_port = htons(port_number);
  ai = host_serv(ip_addr,NULL,AF_INET,SOCK_DGRAM);
  h = sock_ntop_host(ai->ai_addr,ai->ai_addrlen);
  inet_pton(AF_INET,(ai->ai_canonname ? h : ai->ai_canonname),&g_servaddr.sin_addr);
  g_servlen = sizeof(g_servaddr);
  g_replyaddr = malloc(g_servlen);
  freeaddrinfo(ai);  
}

//
// Start to send data
//
SendData() {
  int num;
  
  num = sendto(g_sockfd,g_buffer,g_byte,0,(struct sockaddr *)&g_servaddr,sizeof(g_servaddr));
      if ( num == -1) {
	perror ("error sending buffer using sendto");
	return;
      }
}

ReceiveData(int code) {
  if ((g_pid = fork()) == 0) {
    int n;
    char buffer[SIZE];
    int *ptr;
    
    ptr = (unsigned int*)buffer;
    bzero(buffer,SIZE);
    n = recvfrom(g_sockfd,(void *)ptr,sizeof(buffer),0,(struct sockaddr *) &g_cliaddr,&g_len);
    ptr+=5;
    if (code == 0) printf("BCLK_MODE: received %d bytes 0x%X \n",n,*ptr);
    else if (code == 1) printf("BCLK_TRGBUST: received %d bytes 0x%X \n",n,*ptr);
    else if (code == 2) printf("BCLK_FREQ: received %d bytes 0x%X \n",n,*ptr);
    else if (code == 3) printf("BCLK_TRGDELAY: received %d bytes 0x%X \n",n,*ptr);
    else if (code == 4) printf("BCLK_TPDELAY: received %d bytes 0x%X \n",n,*ptr);
    else if (code == 5) printf("BCLK_ROSYNC: received %d bytes 0x%X \n",n,*ptr);
    else if (code == 6) printf("EVBLD_CHMASK: received %d bytes 0x%X \n",n,*ptr);
    else if (code == 7) printf("EVBLD_DATALENGTH: received %d bytes 0x%X \n",n,*ptr);
    else if (code == 8) printf("EVBLD_MODE: received %d bytes 0x%X \n",n,*ptr);
    else if (code == 9) printf("EVBLD_EVENTINFOTYPE: received %d bytes 0x%X \n",n,*ptr);
    else if (code == 10) printf("BCLK_EVENTINFODATA: received %d bytes 0x%X \n",n,*ptr);

    int i=0;
    for(i=0; i<n; i++)
      {
	if((i%4)==0) printf("\n%4d: ",i);
	printf(" %02x",buffer[i] &0xff);
      }

    printf("\n");
    exit(0);
  }
}

void charToHex(char *buffer,int *buf) {
  int i;
  int num = 0, tot = 0, base = 1;

  for (i=7;i>=0;i--){
    buffer[i] = toupper(buffer[i]);
    if (buffer[i] < '0' || buffer[i] > '9'){      
      if (buffer[i] > 'F') buffer[i] = 'F';
      num = buffer[i] - 'A';
      num += 10;
    }
    else num = buffer[i] - '0';
    
    tot += num*base;
    base *= 16;
  }
  *buf = tot;
}

void ReadFile(char *path) {
  FILE *pFile;
  int *buf,byte = 0;
  char buffer[100];
  
  bzero(g_buffer,SIZE);
  buf = (int*)g_buffer;
  bzero(buffer,100);
  pFile = fopen (path , "r");
  if (pFile == NULL) {
    printf ("Error opening file %s",path);
    exit(0);
  } else {
    //
    // IP
    //
    fgets (buffer,100,pFile);
    snprintf(g_ip,sizeof(g_ip),"%s",buffer);
    //
    // PORT
    //
    fgets (buffer,100,pFile);
    g_port = atoi(buffer);
    //
    // INFO
    //
    while (fgets (buffer,100,pFile) != NULL) {
      if (buffer[0] == '#') continue;
      charToHex(buffer,buf);
      *buf = htonl(*buf);
      buf++;
      g_byte+=4;
    }
    fclose (pFile);
  }
}

//
// MAIN
//
int main(int argc, char **argv) {
  int port_numberR = 0,port_numberS = 0;
  int i;
  //
  //SOCKET creation
  //
  ReadFile(argv[1]);
  PrepareSocket(g_ip,g_port);
  SendData();

  return 0;
}
