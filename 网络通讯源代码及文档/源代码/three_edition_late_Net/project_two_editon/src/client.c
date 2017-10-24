#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <net/if.h>
#include <net/if_arp.h>
#ifdef __GLIBC__
#include <net/if_packet.h>
#else
#include <linux/if_packet.h>
#endif
#include <net/route.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <setjmp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "client.h"
#include "setmsg.h"
#include "udpipgen.h"
#include "autoip.h"

extern	char *ProgramName;
extern	char *IfNameExt;
extern	int  IfNameExt_len;
extern	char *HostName;
extern	unsigned char *ClassID;
extern	int ClassID_len;
extern  unsigned char *ClientID;
extern  int ClientID_len;
extern	int DebugFlag;
extern	int BeRFC1541;
extern	unsigned LeaseTime;
extern	int SetHostName;
extern	int SendSecondDiscover;
extern	unsigned short ip_id;
extern  void *(*currState)();
extern  time_t TimeOut;
extern  unsigned nleaseTime;
extern  struct in_addr inform_ipaddr;
extern	int DoCheckSum;
extern	int InitialHostName_len;
extern	char *InitialHostName;
extern	int DownIfaceOnStop;
extern  int autoip_flag;
extern	int autoip_sleep;

int sendRelArp();
int dhcpConfig();
int dhcpSocket;
int udpFooSocket;
int prev_ip_addr;
time_t ReqSentTime;
dhcpOptions DhcpOptions;
dhcpInterface DhcpIface;
udpipMessage UdpIpMsgSend,UdpIpMsgRecv;
jmp_buf env;
unsigned char ClientHwAddr[ETH_ALEN];

const struct ip *ipSend=(struct ip *)((struct udpiphdr *)UdpIpMsgSend.udpipmsg)->ip;
const struct ip *ipRecv=(struct ip *)((struct udpiphdr *)UdpIpMsgRecv.udpipmsg)->ip;
const dhcpMessage *DhcpMsgSend = (dhcpMessage *)&UdpIpMsgSend.udpipmsg[sizeof(udpiphdr)];
      dhcpMessage *DhcpMsgRecv = (dhcpMessage *)&UdpIpMsgRecv.udpipmsg[sizeof(udpiphdr)];

static short int saved_if_flags = 0;

int parseDhcpMsg() 
{
  register u_char *p = DhcpMsgRecv->options+4;
  unsigned char *end = DhcpMsgRecv->options+sizeof(DhcpMsgRecv->options);
  /* Force T1 and T2 to 0: either new values will be in message, or they
     will need to be recalculated from lease time */
  if (DhcpOptions.val[dhcpT1value] && DhcpOptions.len[dhcpT1value] > 0)
    memset(DhcpOptions.val[dhcpT1value],0,DhcpOptions.len[dhcpT1value]);
  if (DhcpOptions.val[dhcpT2value] && DhcpOptions.len[dhcpT2value] > 0)
    memset(DhcpOptions.val[dhcpT2value],0,DhcpOptions.len[dhcpT2value]);
  while (p < end)
    switch (*p) {
        case endOption: 
	  goto swend;
        case padOption: 
	  p++; break;
       	default:
	  if (p[1]) {
	    if (p + 2 + p[1] >= end)
	      goto swend; /* corrupt packet */
	      if (DhcpOptions.len[*p] == p[1])
	        memcpy(DhcpOptions.val[*p],p+2,p[1]);
	      else {
		DhcpOptions.len[*p] = p[1];
	        if (DhcpOptions.val[*p])
	          free(DhcpOptions.val[*p]);
	      	else
		  DhcpOptions.num++;
	      	DhcpOptions.val[*p] = malloc(p[1]+1);
		memset(DhcpOptions.val[*p],0,p[1]+1);
	  	memcpy(DhcpOptions.val[*p],p+2,p[1]);
	      }
	    }
	  p+=p[1]+2;
    }
  swend:
    if (! DhcpMsgRecv->yiaddr) 
      DhcpMsgRecv->yiaddr=DhcpMsgSend->ciaddr;
    /* did not get dhcpServerIdentifier, make it the same as IP address of the sender */
    if (! DhcpOptions.val[dhcpServerIdentifier]) {	
      DhcpOptions.val[dhcpServerIdentifier] = malloc(4);
      memcpy(DhcpOptions.val[dhcpServerIdentifier],&ipRecv->ip_src.s_addr,4);
      DhcpOptions.len[dhcpServerIdentifier] = 4;
      DhcpOptions.num++;
      if (DebugFlag)
        syslog(LOG_DEBUG,
	"dhcpServerIdentifier option is missing in DHCP server response. Assuming %u.%u.%u.%u\n",
	((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[0],
	((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[1],
	((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[2],
	((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[3]);
    }
    /* did not get subnetMask */
    if (! DhcpOptions.val[subnetMask]) {
      DhcpOptions.val[subnetMask] = malloc(4);
      ((unsigned char *)DhcpOptions.val[subnetMask])[0] = 255;
      if (IN_CLASSA(ntohl(DhcpMsgRecv->yiaddr))) {
          ((unsigned char *)DhcpOptions.val[subnetMask])[1] = 0; /* class A */
          ((unsigned char *)DhcpOptions.val[subnetMask])[2] = 0;
          ((unsigned char *)DhcpOptions.val[subnetMask])[3] = 0;
      }
      else {
        ((unsigned char *)DhcpOptions.val[subnetMask])[1] = 255;
        if ( IN_CLASSB(ntohl(DhcpMsgRecv->yiaddr)) ) {
	  ((unsigned char *)DhcpOptions.val[subnetMask])[2] = 0;/* class B */
          ((unsigned char *)DhcpOptions.val[subnetMask])[3] = 0;
	}
	else {
	  ((unsigned char *)DhcpOptions.val[subnetMask])[2] = 255;
	  if (IN_CLASSC(ntohl(DhcpMsgRecv->yiaddr)))
            ((unsigned char *)DhcpOptions.val[subnetMask])[3] = 0; /* class C */
	  else
            ((unsigned char *)DhcpOptions.val[subnetMask])[3] = 255;
	}
      }
      DhcpOptions.len[subnetMask] = 4;
      DhcpOptions.num++;
      if (DebugFlag)
	printf("subnetMask option is missing in DHCP server response. Assuming %u.%u.%u.%u\n",
	((unsigned char *)DhcpOptions.val[subnetMask])[0],
	((unsigned char *)DhcpOptions.val[subnetMask])[1],
	((unsigned char *)DhcpOptions.val[subnetMask])[2],
	((unsigned char *)DhcpOptions.val[subnetMask])[3]);
    }
    /* did not get broadcastAddr */
    if (! DhcpOptions.val[broadcastAddr]) {
      int br = DhcpMsgRecv->yiaddr | ~*((int *)DhcpOptions.val[subnetMask]);
      DhcpOptions.val[broadcastAddr] = malloc(4);
      memcpy(DhcpOptions.val[broadcastAddr],&br,4);
      DhcpOptions.len[broadcastAddr] = 4;
      DhcpOptions.num++;
      if (DebugFlag)
	printf("broadcastAddr option is missing in DHCP server response. Assuming %u.%u.%u.%u\n",
	((unsigned char *)DhcpOptions.val[broadcastAddr])[0],
	((unsigned char *)DhcpOptions.val[broadcastAddr])[1],
	((unsigned char *)DhcpOptions.val[broadcastAddr])[2],
	((unsigned char *)DhcpOptions.val[broadcastAddr])[3]);
    }

    if (DhcpOptions.val[dhcpIPaddrLeaseTime] && DhcpOptions.len[dhcpIPaddrLeaseTime] == 4) {
      if (*(unsigned int *)DhcpOptions.val[dhcpIPaddrLeaseTime] == 0) {
        memcpy(DhcpOptions.val[dhcpIPaddrLeaseTime],&nleaseTime,4);
	if (DebugFlag)
	  printf("dhcpIPaddrLeaseTime=0 in DHCP server response. Assuming %u sec\n",LeaseTime);
      }
      else
        if (DebugFlag)
	  printf("dhcpIPaddrLeaseTime=%u in DHCP server response.\n",
	  ntohl(*(unsigned int *)DhcpOptions.val[dhcpIPaddrLeaseTime]));
    }
    /* did not get dhcpIPaddrLeaseTime */
    else {
      DhcpOptions.val[dhcpIPaddrLeaseTime] = malloc(4);
      memcpy(DhcpOptions.val[dhcpIPaddrLeaseTime],&nleaseTime,4);
      DhcpOptions.len[dhcpIPaddrLeaseTime] = 4;
      DhcpOptions.num++;
      if (DebugFlag)
	printf("dhcpIPaddrLeaseTime option is missing in DHCP server response. Assuming %u sec\n",LeaseTime);
    }
  if (DhcpOptions.val[dhcpT1value] && DhcpOptions.len[dhcpT1value] == 4) {
    if (*(unsigned int *)DhcpOptions.val[dhcpT1value] == 0) {
      unsigned t2 = 0.5*ntohl(*(unsigned int *)DhcpOptions.val[dhcpIPaddrLeaseTime]);
      int t1 = htonl(t2);
      memcpy(DhcpOptions.val[dhcpT1value],&t1,4);
      DhcpOptions.len[dhcpT1value] = 4;
      if (DebugFlag)
        printf("dhcpT1value is missing in DHCP server response. Assuming %u sec\n",t2);
    }
  }
  /* did not get T1 */
  else {
    unsigned t2 = 0.5*ntohl(*(unsigned int *)DhcpOptions.val[dhcpIPaddrLeaseTime]);
    int t1 = htonl(t2);
    DhcpOptions.val[dhcpT1value] = malloc(4);
    memcpy(DhcpOptions.val[dhcpT1value],&t1,4);
    DhcpOptions.len[dhcpT1value] = 4;
    DhcpOptions.num++;
    if (DebugFlag)
      printf("dhcpT1value is missing in DHCP server response. Assuming %u sec\n",t2);
  }
  if (DhcpOptions.val[dhcpT2value] && DhcpOptions.len[dhcpT2value] == 4) {
    if (*(unsigned int *)DhcpOptions.val[dhcpT2value] == 0) {
      unsigned t2 = 0.875*ntohl(*(unsigned int *)DhcpOptions.val[dhcpIPaddrLeaseTime]);
      int t1 = htonl(t2);
      memcpy(DhcpOptions.val[dhcpT2value],&t1,4);
      DhcpOptions.len[dhcpT2value] = 4;
      if (DebugFlag)
        printf("dhcpT2value is missing in DHCP server response. Assuming %u sec\n",t2);
    }
  }
  /* did not get T2 */
  else {
    unsigned t2 = 0.875*ntohl(*(unsigned int *)DhcpOptions.val[dhcpIPaddrLeaseTime]);
    int t1 = htonl(t2);
    DhcpOptions.val[dhcpT2value] = malloc(4);
    memcpy(DhcpOptions.val[dhcpT2value],&t1,4);
    DhcpOptions.len[dhcpT2value] = 4;
    DhcpOptions.num++;
    if (DebugFlag)
      printf("dhcpT2value is missing in DHCP server response. Assuming %u sec\n",t2);
  }
  if (DhcpOptions.val[dhcpFQDNHostName]) {
    printf("dhcpFQDNHostName response flags = %02X  rcode1 = %02X  rcode2 = %02X  name = \"%s\"\n",
	((unsigned char *)DhcpOptions.val[dhcpFQDNHostName])[0],
	((unsigned char *)DhcpOptions.val[dhcpFQDNHostName])[1],
	((unsigned char *)DhcpOptions.val[dhcpFQDNHostName])[2],
	((char *)DhcpOptions.val[dhcpFQDNHostName])+3);
  }
  if (DhcpOptions.val[dhcpMessageType])
    return *(unsigned char *)DhcpOptions.val[dhcpMessageType];
  return 0;
}


void classIDsetup()
{
  struct utsname sname;
  if ( uname(&sname) ) syslog(LOG_ERR,"classIDsetup: uname: %m\n");
  DhcpIface.class_len=snprintf(DhcpIface.class_id,CLASS_ID_MAX_LEN,
  "%s %s %s",sname.sysname,sname.release,sname.machine);
}


void clientIDsetup()
{
  unsigned char *c = DhcpIface.client_id;
  *c++ = dhcpClientIdentifier;
  if (ClientID) {
    *c++ = ClientID_len + 1;	/* 1 for the field below */
    *c++ = 0;			/* type: string */
    memcpy(c,ClientID,ClientID_len);
    DhcpIface.client_len = ClientID_len + 3;
    return;
  }
  *c++ = ETH_ALEN + 1;	        /* length: 6 (MAC Addr) + 1 (# field) */
  *c++ = ARPHRD_ETHER;	/* type: Ethernet address */
  memcpy(c,ClientHwAddr,ETH_ALEN);
  DhcpIface.client_len = ETH_ALEN + 3;
}


void releaseDhcpOptions()
{
  register int i;
  for (i=1;i<256;i++)
    if (DhcpOptions.val[i]) 
      free(DhcpOptions.val[i]);
  memset(&DhcpOptions,0,sizeof(dhcpOptions));
}


/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */
static int timeval_subtract(result,x,y)
struct timeval *result,*x,*y;
{
  /* Perform the carry for the later subtraction by updating Y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }
  /* Compute the time remaining to wait.
     `tv_usec' is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}


int dhcpSendAndRecv(xid,msg,buildUdpIpMsg)
unsigned xid,msg;
void (*buildUdpIpMsg)(unsigned);
{
  struct sockaddr addr;
  struct timeval begin, current, diff;
  int i,len,timeout=0;
  socklen_t addrLength;
  char foobuf[512];
  const struct udphdr *udpRecv;
  int j=DHCP_INITIAL_RTO/2;
  do {
    do {
      j+=j;
      if (j > DHCP_MAX_RTO) 
        j = DHCP_MAX_RTO;
      memset(&addr,0,sizeof(struct sockaddr));
      memcpy(addr.sa_data,IfNameExt,IfNameExt_len);
      buildUdpIpMsg(xid);
      len = sizeof(struct packed_ether_header)+sizeof(udpiphdr)+sizeof(dhcpMessage);
      if (sendto(dhcpSocket, &UdpIpMsgSend, len, 0, &addr,sizeof(struct sockaddr)) == -1) {
        perror("sendto: %m\n");
        return (-1);
      }
      gettimeofday(&begin, NULL);
      i=random();
    }
    while ( waitfd(dhcpSocket,j+i%200000) );
    do {
      struct ip ipRecv_local;
      char *tmp_ip;
      memset(&UdpIpMsgRecv,0,sizeof(udpipMessage));
      addrLength=sizeof(struct sockaddr);
      len=recvfrom(dhcpSocket,&UdpIpMsgRecv,sizeof(udpipMessage),0,(struct sockaddr *)&addr,&addrLength);
      if (len == -1) {
        perror("recvfrom: %m\n");
      	return (-1);
      }
      gettimeofday(&current, NULL);
      timeval_subtract(&diff, &current, &begin);
      timeout = j - diff.tv_sec*1000000 - diff.tv_usec + random()%200000;
      if (UdpIpMsgRecv.ethhdr.ether_type != htons(ETHERTYPE_IP))
        continue;
      tmp_ip = UdpIpMsgRecv.udpipmsg;
      for (i=0;i<sizeof(struct ip)-2;i+=2)
        if ((UdpIpMsgRecv.udpipmsg[i] == 0x45) && (UdpIpMsgRecv.udpipmsg[i+1] == 0x00)) {
          tmp_ip=&(UdpIpMsgRecv.udpipmsg[i]);
          break;
      }
      memcpy(&ipRecv_local,((struct udpiphdr *)tmp_ip)->ip,sizeof(struct ip));
      udpRecv=(struct udphdr *)((char*)(((struct udpiphdr*)tmp_ip)->ip)+sizeof(struct ip));
      if (ipRecv_local.ip_p != IPPROTO_UDP) 
        continue;
      len-=sizeof(struct packed_ether_header);
      i=(int )ntohs(ipRecv_local.ip_len);
      if (len < i) {
        if (DebugFlag) 
	  printf("corrupted IP packet of size=%d and ip_len=%d discarded\n", len, i);
	    continue;
      }
      len=i-(ipRecv_local.ip_hl<<2);
      i=(int )ntohs(udpRecv->uh_ulen);
      if (len < i) {
        if (DebugFlag) 
	  printf("corrupted UDP msg of size=%d and uh_ulen=%d discarded\n", len, i);
	    continue;
      }
      if (DoCheckSum) {
        len=udpipchk((udpiphdr *)tmp_ip);
        if (len) {
          if (DebugFlag)
            switch (len) {
	      case -1: syslog(LOG_DEBUG, "corrupted IP packet with ip_len=%d discarded\n", (int )ntohs(ipRecv_local.ip_len));
	        break;
	      case -2: syslog(LOG_DEBUG, "corrupted UDP msg with uh_ulen=%d discarded\n", (int )ntohs(udpRecv->uh_ulen));
		break;
            }
	  continue;
        }
      }
      DhcpMsgRecv=(dhcpMessage *)&tmp_ip[(ipRecv_local.ip_hl<<2)+sizeof(struct udphdr)];
      if (DhcpMsgRecv->xid != xid) 
        continue;
      if (DhcpMsgRecv->htype != ARPHRD_ETHER) {
        if (DebugFlag)
          printf("wrong msg htype 0x%X discarded\n",DhcpMsgRecv->htype);
        continue;
      }
      if (DhcpMsgRecv->op != DHCP_BOOTREPLY) 
        continue;
      while (udpFooSocket > 0 && recvfrom(udpFooSocket,(void *)foobuf,sizeof(foobuf),0,NULL,NULL) != -1); 
      if (parseDhcpMsg() == msg) 
        return 0;
      if (DhcpOptions.val[dhcpMessageType])
      if (*(unsigned char *)DhcpOptions.val[dhcpMessageType] == DHCP_NAK) {
        if (DhcpOptions.val[dhcpMsg])
          syslog(LOG_ERR, "DHCP_NAK server response received: %s\n", (char *)DhcpOptions.val[dhcpMsg]);
        else
	  syslog(LOG_ERR, "DHCP_NAK server response received\n");
        return 1;
      }
    }
    while ( timeout > 0 && waitfd(dhcpSocket, timeout) == 0 );
  }
  while ( 1 );
  return 1;
}


void *Start()
{
  int flags;
  int o = 1;
  unsigned i=0;
  struct ifreq	ifr;
  struct sockaddr_pkt sap;
  struct sockaddr_in clientAddr;
  memset(&ifr,0,sizeof(struct ifreq));
  memcpy(ifr.ifr_name,IfNameExt,IfNameExt_len);
  dhcpSocket = socket(PF_PACKET,SOCK_PACKET,htons(ETH_P_ALL));
  if (dhcpSocket == -1 || (flags = fcntl(dhcpSocket, F_GETFL, 0)) == -1 || fcntl(dhcpSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
    syslog(LOG_ERR,"Start: socket: %m\n");
    exit(1);
  }
  if (ioctl(dhcpSocket,SIOCGIFHWADDR,&ifr)) {
    syslog(LOG_ERR,"Start: ioctl SIOCGIFHWADDR: %m\n");
    exit(1);
  }
  if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
    syslog(LOG_ERR,"Start: interface %s is not Ethernet or 802.2 Token Ring\n",ifr.ifr_name);
    exit(1);
  }
  if (setsockopt(dhcpSocket,SOL_SOCKET,SO_BROADCAST,&o,sizeof(o)) == -1) {
    syslog(LOG_ERR,"Start: setsockopt: %m\n");
    exit(1);
  }
  if (ioctl(dhcpSocket,SIOCGIFFLAGS,&ifr)) {  
    syslog(LOG_ERR,"Start: ioctl SIOCGIFFLAGS: %m\n");  
    exit(1);  
  }  
  saved_if_flags = ifr.ifr_flags;  
  ifr.ifr_flags = saved_if_flags | IFF_UP | IFF_BROADCAST | IFF_NOTRAILERS | IFF_RUNNING;
  if (ioctl(dhcpSocket,SIOCSIFFLAGS,&ifr)) {
    syslog(LOG_ERR,"Start: ioctl SIOCSIFFLAGS: %m\n");
    exit(1);
  }
  memset(&sap,0,sizeof(sap));
  do {
    i++;
    if (i > 1)
      syslog(LOG_WARNING,"Start: retrying MAC address request "
	"(returned %02x:%02x:%02x:%02x:%02x:%02x)",
	ClientHwAddr[0],ClientHwAddr[1],ClientHwAddr[2],
	ClientHwAddr[3],ClientHwAddr[4],ClientHwAddr[5]);
      if (ioctl(dhcpSocket,SIOCGIFHWADDR,&ifr)) {
        syslog(LOG_ERR,"dhcpStart: ioctl SIOCGIFHWADDR: %m\n");
	exit(1);
      }
      if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
	syslog(LOG_ERR,"Start: interface %s is not Ethernet or 802.2 Token Ring\n",ifr.ifr_name);
	exit(1);
      }
      if (setsockopt(dhcpSocket,SOL_SOCKET,SO_BROADCAST,&o,sizeof(o)) == -1) {
	syslog(LOG_ERR,"Start: setsockopt: %m\n");
	exit(1);
      }
      ifr.ifr_flags = saved_if_flags | IFF_UP | IFF_BROADCAST | IFF_NOTRAILERS | IFF_RUNNING;
      if (ioctl(dhcpSocket,SIOCSIFFLAGS,&ifr)) {
        syslog(LOG_ERR,"Start: ioctl SIOCSIFFLAGS: %m\n");
	exit(1);
      }
      memset(&sap,0,sizeof(sap));
      sap.spkt_protocol = htons(ETH_P_ALL);
      memcpy(sap.spkt_device,IfNameExt,IfNameExt_len);
      sap.spkt_family = PF_PACKET;
      if (bind(dhcpSocket,(void*)&sap,sizeof(struct sockaddr)) == -1)
        syslog(LOG_ERR,"Start: bind: %m\n");
      memcpy(ClientHwAddr,ifr.ifr_hwaddr.sa_data,ETH_ALEN);
      if (DebugFlag)
	fprintf(stdout,"mydhcpcd: MAC address = %02x:%02x:%02x:%02x:%02x:%02x\n",
	ClientHwAddr[0], ClientHwAddr[1], ClientHwAddr[2],
	ClientHwAddr[3], ClientHwAddr[4], ClientHwAddr[5]);
  }
  while ( !ClientHwAddr[0] &&
	  !ClientHwAddr[1] &&
	  !ClientHwAddr[2] &&
	  !ClientHwAddr[3] &&
	  !ClientHwAddr[4] &&
	  !ClientHwAddr[5] &&
	   i<HWADDR_TRIES );
  i=time(NULL)+ClientHwAddr[5]+4*ClientHwAddr[4]+8*ClientHwAddr[3]+16*ClientHwAddr[2]+32*ClientHwAddr[1]+64*ClientHwAddr[0];
  srandom(i);
  ip_id=i&0xffff;
  udpFooSocket = socket(AF_INET,SOCK_DGRAM,0);
  if (udpFooSocket == -1) {
    syslog(LOG_ERR,"Start: socket: %m\n");
    exit(1);
  }
  if (setsockopt(udpFooSocket,SOL_SOCKET,SO_BROADCAST,&o,sizeof(o)))
    syslog(LOG_ERR,"Start: setsockopt: %m\n");
  memset(&clientAddr.sin_addr,0,sizeof(&clientAddr.sin_addr));
  clientAddr.sin_family = AF_INET;
  clientAddr.sin_port = htons(DHCP_CLIENT_PORT);
  if (bind(udpFooSocket,(struct sockaddr *)&clientAddr,sizeof(clientAddr))) {
    if (errno != EADDRINUSE)
      syslog(LOG_ERR,"Start: bind: %m\n");
    close(udpFooSocket);
    udpFooSocket = -1;
  }
  else
    if (fcntl(udpFooSocket,F_SETFL,O_NONBLOCK) == -1) {
      syslog(LOG_ERR,"Start: fcntl: %m\n");
      exit(1);
    }
  return &Init;
}


void classclientsetup()
{
  if (ClassID) {
      memcpy(DhcpIface.class_id,ClassID,ClassID_len);
      DhcpIface.class_len=ClassID_len;
  }
  else
    classIDsetup();
  clientIDsetup();
}

void *Reboot()
{
 
  if (sigsetjmp(env,0xffff)) {
      if (DebugFlag)
	printf("timed out waiting for DHCP_ACK response\n");
      if (TimeOut != 0)
	alarm(TimeOut);
      return &Init;
  }
  Start();
  memset(&DhcpOptions,0,sizeof(DhcpOptions));
  memset(&DhcpIface,0,sizeof(dhcpInterface));
  classclientsetup();
  return Request(random(),&setDhcpReboot);
}


void *Init()
{
  if (sigsetjmp(env,0xffff)) {
    if (DebugFlag) 
      printf("time out  at Init\n");
    if (autoip_flag) {
      sleep(autoip_sleep);
      printf("I have slept for %d secs\n",autoip_sleep);
    }
    else {
      autoip();
      autoip_flag = 1;
    }
    if(TimeOut != 0)
      alarm(TimeOut);
    return &Init;
  }
  releaseDhcpOptions();
  if (DebugFlag) 
    printf("broadcasting DHCP_DISCOVER\n");
  if (dhcpSendAndRecv(random(),DHCP_OFFER,&setDhcpDiscover)) {
    Stop();
    return 0;
  }
  if (SendSecondDiscover)
  {
    if (DebugFlag) 
      syslog(LOG_DEBUG,"broadcasting second DHCP_DISCOVER\n");
    dhcpSendAndRecv(DhcpMsgRecv->xid,DHCP_OFFER,&setDhcpDiscover);
  }
  prev_ip_addr = DhcpIface.ciaddr;
  DhcpIface.ciaddr = DhcpMsgRecv->yiaddr;
  memcpy(&DhcpIface.siaddr,DhcpOptions.val[dhcpServerIdentifier],4);
  memcpy(DhcpIface.shaddr,UdpIpMsgRecv.ethhdr.ether_shost,ETH_ALEN);
  DhcpIface.xid = DhcpMsgRecv->xid;
/* DHCP_OFFER received */
  if (DebugFlag)
    printf("DHCP_OFFER received from %s (%u.%u.%u.%u)\n",
    DhcpMsgRecv->sname,
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[0],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[1],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[2],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[3]);
  return Request(DhcpIface.xid,&setDhcpRequest);
}



void *Request(xid,buildDhcpMsg)
unsigned xid;
void (*buildDhcpMsg)(unsigned);
{
/* send the message and read and parse replies into DhcpOptions */
  if (DebugFlag)
    syslog(LOG_DEBUG,"broadcasting DHCP_REQUEST for %u.%u.%u.%u\n",
	   ((unsigned char *)&DhcpIface.ciaddr)[0],
	   ((unsigned char *)&DhcpIface.ciaddr)[1],
	   ((unsigned char *)&DhcpIface.ciaddr)[2],
	   ((unsigned char *)&DhcpIface.ciaddr)[3]);
  if (dhcpSendAndRecv(xid,DHCP_ACK,buildDhcpMsg)) 
    return &Init;
  ReqSentTime=time(NULL);
  if (DebugFlag) 
    syslog(LOG_DEBUG,
    "DHCP_ACK received from %s (%u.%u.%u.%u)\n",DhcpMsgRecv->sname,
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[0],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[1],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[2],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[3]);
/* check if the offered IP address already in use  */ 
/*  if (checkIP(DhcpIface.ciaddr)) {
    if (DebugFlag) printf(
	"requested %u.%u.%u.%u address is in use\n",
	((unsigned char *)&DhcpIface.ciaddr)[0],
	((unsigned char *)&DhcpIface.ciaddr)[1],
	((unsigned char *)&DhcpIface.ciaddr)[2],
	((unsigned char *)&DhcpIface.ciaddr)[3]);
    DhcpIface.ciaddr = 0;
    return &Decline;
  }*/
  if (DebugFlag) 
    printf("verified %u.%u.%u.%u address is not in use\n",
    ((unsigned char *)&DhcpIface.ciaddr)[0],
    ((unsigned char *)&DhcpIface.ciaddr)[1],
    ((unsigned char *)&DhcpIface.ciaddr)[2],
    ((unsigned char *)&DhcpIface.ciaddr)[3]);    
  if (dhcpConfig()) {
    Stop();
    return (0);
  }
  /* Successfull ACK: Use the fields obtained for future requests */
  memcpy(&DhcpIface.siaddr,DhcpOptions.val[dhcpServerIdentifier],4);
  memcpy(DhcpIface.shaddr,UdpIpMsgRecv.ethhdr.ether_shost,ETH_ALEN);
  return &Bound;
}


void *Bound()
{
  int i, maxfd;
  fd_set rset;
  char foobuf[512];
  if (sigsetjmp(env,0xffff)) 
    return &Renew;
  i=ReqSentTime+ntohl(*(unsigned int *)DhcpOptions.val[dhcpT1value])-time(NULL);
  if ( i > 0 )
    alarm(i);
  else
    return &Renew;
  while(1) {
    FD_ZERO(&rset);
    maxfd = dhcpSocket;
    FD_SET(dhcpSocket, &rset);
    if (udpFooSocket  != -1) {
      if (udpFooSocket > maxfd)
        maxfd = udpFooSocket;
	FD_SET(udpFooSocket, &rset);
    }
    if (select(maxfd+1, &rset, NULL, NULL, NULL) == -1) {
      if (errno != EINTR)
      return &Renew;
    }
    else {
      if (udpFooSocket != -1 && FD_ISSET(udpFooSocket, &rset))
        while (recvfrom(udpFooSocket,(void *)foobuf,sizeof(foobuf),0,NULL,NULL) != -1 );
	  if (FD_ISSET(dhcpSocket, &rset))
	    while (recvfrom(dhcpSocket,(void *)foobuf,sizeof(foobuf),0,NULL,NULL) != -1 );
	 }
    }
}


void *Renew()
{
  int i;
  if (sigsetjmp(env,0xffff)) 
    return &Rebind;
  i = ReqSentTime+ntohl(*(unsigned int *)DhcpOptions.val[dhcpT2value])-time(NULL);
  if (i > 0)
    alarm(i);
  else
    return &Rebind;
  if (DebugFlag)
    printf("sending DHCP_REQUEST for %u.%u.%u.%u to %u.%u.%u.%u\n",
	   ((unsigned char *)&DhcpIface.ciaddr)[0],
	   ((unsigned char *)&DhcpIface.ciaddr)[1],
	   ((unsigned char *)&DhcpIface.ciaddr)[2],
	   ((unsigned char *)&DhcpIface.ciaddr)[3],
	   ((unsigned char *)&DhcpIface.siaddr)[0],
	   ((unsigned char *)&DhcpIface.siaddr)[1],
	   ((unsigned char *)&DhcpIface.siaddr)[2],
	   ((unsigned char *)&DhcpIface.siaddr)[3]);
  if (dhcpSendAndRecv(random(),DHCP_ACK,&setDhcpRenew)) 
    return &Rebind;
  ReqSentTime=time(NULL);
  if (DebugFlag) 
    printf("DHCP_ACK received from %s (%u.%u.%u.%u)\n",DhcpMsgRecv->sname,
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[0],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[1],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[2],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[3]);
  return &Bound;
}


void *Rebind()
{
  int i;
  if (sigsetjmp(env,0xffff)) {
    if (TimeOut != 0) {
      alarm(2*TimeOut);
    }
    return &Stop;
  } 
  i = ReqSentTime+ntohl(*(unsigned int *)DhcpOptions.val[dhcpIPaddrLeaseTime])-time(NULL);
  if (i > 0)
    alarm(i);
  else
    return &Stop;
  if (DebugFlag)
    printf("broadcasting DHCP_REQUEST for %u.%u.%u.%u\n",
	   ((unsigned char *)&DhcpIface.ciaddr)[0],
	   ((unsigned char *)&DhcpIface.ciaddr)[1],
	   ((unsigned char *)&DhcpIface.ciaddr)[2],
	   ((unsigned char *)&DhcpIface.ciaddr)[3]);
  if (dhcpSendAndRecv(random(),DHCP_ACK,&setDhcpRebind)) 
    return &Stop;
  ReqSentTime=time(NULL);
  if (DebugFlag) 
    printf("DHCP_ACK received from %s (%u.%u.%u.%u)\n",DhcpMsgRecv->sname,
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[0],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[1],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[2],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[3]);
  /* Successfull ACK: Use the fields obtained for future requests */
  memcpy(&DhcpIface.siaddr,DhcpOptions.val[dhcpServerIdentifier],4);
  memcpy(DhcpIface.shaddr,UdpIpMsgRecv.ethhdr.ether_shost,ETH_ALEN);
  return &Bound;
}


void *Release()
{
  struct sockaddr addr;
  if (DhcpIface.ciaddr == 0) 
    return &Init;
  setDhcpRelease(random());
  memset(&addr,0,sizeof(struct sockaddr));
  memcpy(addr.sa_data,IfNameExt,IfNameExt_len);
  if (DebugFlag)
    printf("sending DHCP_RELEASE for %u.%u.%u.%u to %u.%u.%u.%u\n",
	   ((unsigned char *)&DhcpIface.ciaddr)[0],
	   ((unsigned char *)&DhcpIface.ciaddr)[1],
	   ((unsigned char *)&DhcpIface.ciaddr)[2],
	   ((unsigned char *)&DhcpIface.ciaddr)[3],
	   ((unsigned char *)&DhcpIface.siaddr)[0],
	   ((unsigned char *)&DhcpIface.siaddr)[1],
	   ((unsigned char *)&DhcpIface.siaddr)[2],
	   ((unsigned char *)&DhcpIface.siaddr)[3]);
  if (sendto(dhcpSocket,&UdpIpMsgSend,sizeof(struct packed_ether_header) + sizeof(udpiphdr)+sizeof(dhcpMessage),0,
	      &addr,sizeof(struct sockaddr)) == -1 )
    syslog(LOG_ERR,"Release: sendto: %m\n");
  sendRelArp(); /* clear ARP cache entries for client IP addr */
  if (SetHostName) {
    sethostname(InitialHostName,InitialHostName_len);
    if (DebugFlag)
      fprintf(stdout,"dhcpcd: your hostname = %s\n",InitialHostName);
  }
  DhcpIface.ciaddr=0;
  return &Init;
}



void *Stop()
{
  struct ifreq ifr;
  struct sockaddr_in *p = (struct sockaddr_in *)&(ifr.ifr_addr);
  releaseDhcpOptions();
  memset(&ifr,0,sizeof(struct ifreq));
  memcpy(ifr.ifr_name,IfNameExt,IfNameExt_len);
  p->sin_family = AF_INET;
  p->sin_addr.s_addr = 0;
  if (DownIfaceOnStop) {
    ifr.ifr_flags = saved_if_flags & ~IFF_UP;
    if (ioctl(dhcpSocket,SIOCSIFFLAGS,&ifr))
      syslog(LOG_ERR,"Stop: ioctl SIOCSIFFLAGS: %m\n");
  }

  close(dhcpSocket);
  return &Start;
}


void *Decline()
{
  struct sockaddr addr;
  memset(&UdpIpMsgSend,0,sizeof(udpipMessage));
  memcpy(UdpIpMsgSend.ethhdr.ether_dhost,MAC_BCAST_ADDR,ETH_ALEN);
  memcpy(UdpIpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  UdpIpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_IP);
  setDhcpDecline(random());
  udpipgen((udpiphdr *)&UdpIpMsgSend.udpipmsg,0,INADDR_BROADCAST,
  htons(DHCP_CLIENT_PORT),htons(DHCP_SERVER_PORT),sizeof(dhcpMessage));
  memset(&addr,0,sizeof(struct sockaddr));
  memcpy(addr.sa_data,IfNameExt,IfNameExt_len);
  if ( DebugFlag ) syslog(LOG_DEBUG,"broadcasting DHCP_DECLINE\n");
  if ( sendto(dhcpSocket,&UdpIpMsgSend,sizeof(struct packed_ether_header) + sizeof(udpiphdr)+sizeof(dhcpMessage),0,
	      &addr,sizeof(struct sockaddr)) == -1 )
    syslog(LOG_ERR,"Decline: sendto: %m\n");
  return &Init;
}


void *Inform()
{
  Start();
  memset(&DhcpOptions,0,sizeof(DhcpOptions));
  memset(&DhcpIface,0,sizeof(dhcpInterface));
  if (! inform_ipaddr.s_addr) {
    struct ifreq ifr;
    struct sockaddr_in *p = (struct sockaddr_in *)&(ifr.ifr_addr);
    memset(&ifr,0,sizeof(struct ifreq));
    memcpy(ifr.ifr_name,IfNameExt,IfNameExt_len);
    p->sin_family = AF_INET;
    if (ioctl(dhcpSocket,SIOCGIFADDR,&ifr) == 0)
      inform_ipaddr.s_addr=p->sin_addr.s_addr;
    if (! inform_ipaddr.s_addr) {
      inform_ipaddr.s_addr=DhcpIface.ciaddr;
    }
  }
  DhcpIface.ciaddr=inform_ipaddr.s_addr;
  if (! DhcpIface.class_len) { 
    if (ClassID) {
      memcpy(DhcpIface.class_id,ClassID,ClassID_len);
      DhcpIface.class_len=ClassID_len;
    }
    else
      classIDsetup();
  }
  if (! DhcpIface.client_len) 
    clientIDsetup();
  if (sigsetjmp(env,0xffff)) {
    if (DebugFlag)
      printf("timed out waiting for DHCP_ACK response\n");
    return 0;
  }
  if (DebugFlag)
    printf("broadcasting DHCP_INFORM for %u.%u.%u.%u\n",
	   ((unsigned char *)&DhcpIface.ciaddr)[0],
	   ((unsigned char *)&DhcpIface.ciaddr)[1],
	   ((unsigned char *)&DhcpIface.ciaddr)[2],
	   ((unsigned char *)&DhcpIface.ciaddr)[3]);
  if (dhcpSendAndRecv(random(),DHCP_ACK,setDhcpInform)) 
    return 0;
  if (DebugFlag) 
    printf("DHCP_ACK received from %s (%u.%u.%u.%u)\n",DhcpMsgRecv->sname,
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[0],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[1],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[2],
    ((unsigned char *)DhcpOptions.val[dhcpServerIdentifier])[3]);
/* check if the offered IP address already in use */
  if (checkIP(DhcpIface.ciaddr)) {
    if (DebugFlag) 
      printf("requested %u.%u.%u.%u address is in use\n",
	((unsigned char *)&DhcpIface.ciaddr)[0],
	((unsigned char *)&DhcpIface.ciaddr)[1],
	((unsigned char *)&DhcpIface.ciaddr)[2],
	((unsigned char *)&DhcpIface.ciaddr)[3]);
    return &Decline;
  }
  if (DebugFlag) 
    printf("verified %u.%u.%u.%u address is not in use\n",
    ((unsigned char *)&DhcpIface.ciaddr)[0],
    ((unsigned char *)&DhcpIface.ciaddr)[1],
    ((unsigned char *)&DhcpIface.ciaddr)[2],
    ((unsigned char *)&DhcpIface.ciaddr)[3]);
  if (dhcpConfig()) 
    return 0;
  exit(0);
}


