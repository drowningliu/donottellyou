#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/sockios.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include "myarp.h"
#include "client.h"

extern char *IfNameExt;	
extern int IfNameExt_len;	
extern int dhcpSocket;
extern unsigned char ClientHwAddr[ETH_ALEN];
extern dhcpInterface DhcpIface;
extern int DebugFlag;
const int inaddr_broadcast = INADDR_BROADCAST;

int waitfd(int fd, time_t usec)
{
  fd_set fs;
  struct timeval t;
  int r;
	
  FD_ZERO(&fs);
  FD_SET(fd, &fs);
  t.tv_sec = usec/1000000;
  t.tv_usec = usec%1000000;
	
  if ((r = select(fd+1,&fs,NULL,NULL,&t)) == -1) 
    return -1;
  if (FD_ISSET(fd,&fs)) 
    return 0;
  return 1;
}

void setArpMsg(arpMessage *msg, u_short opcode, u_long tipaddr, u_char *thaddr,
	       u_long sipaddr, u_char *shaddr)
{
  memset(msg,0,sizeof(arpMessage));
  memcpy(msg->ethhdr.h_dest, MAC_BCAST_ADDR, 6);
  memcpy(msg->ethhdr.h_source, shaddr, ETH_ALEN);
  msg->ethhdr.h_proto = htons(ETHERTYPE_ARP);
  msg->htype = htons(ARPHRD_ETHER);
  msg->ptype = htons(ETHERTYPE_IP);
  msg->hlen = ETH_ALEN;
  msg->plen = 4;
  msg->operation = htons(opcode);
  memcpy(msg->sHaddr, shaddr, ETH_ALEN);
  msg->sInaddr[3] = sipaddr >> 24;      // note that sInaddr is stored in big endian form
  msg->sInaddr[2] = ( sipaddr & 0x00ff0000) >> 16;
  msg->sInaddr[1] = ( sipaddr & 0x0000ff00) >> 8;
  msg->sInaddr[0] = ( sipaddr & 0x000000ff) ;   
//printf("Source IP addr is %u.%u.%u.%u \n", msg->sInaddr[0], msg->sInaddr[1], msg->sInaddr[2], msg->sInaddr[3]);
  msg->tInaddr[3] = tipaddr >> 24;     // so does tInaddr
  msg->tInaddr[2] = ( tipaddr & 0x00ff0000) >> 16;
  msg->tInaddr[1] = ( tipaddr & 0x0000ff00) >> 8;
  msg->tInaddr[0] = ( tipaddr & 0x000000ff) ;    
//printf("Target IP addr is %u.%u.%u.%u \n", msg->tInaddr[0], msg->tInaddr[1], msg->tInaddr[2], msg->tInaddr[3]);
  if (opcode == ARPOP_REPLY) {
    memcpy(msg->tHaddr, thaddr, ETH_ALEN);
  }
}  
	
int checkIP(u_long taddr)
{	
  struct sockaddr addr;
  arpMessage ArpMsgSend,ArpMsgRecv;
  //struct ifreq ifr;
  int i = 0;
  int j;
  char tHaddr[ETH_ALEN];
  memset(tHaddr, 0, ETH_ALEN);
//	printf("target hardware address is: %02X.%02X.%02X.%02X.%02X.%02X \n",
//	       tHaddr[0], tHaddr[1], tHaddr[2], tHaddr[3], tHaddr[4], tHaddr[5]);
  int len;
  len = sizeof(arpMessage);
  u_long myaddr;
  myaddr = 0;
  memset(&ArpMsgSend, 0, sizeof(arpMessage));
  setArpMsg(&ArpMsgSend, ARPOP_REQUEST, taddr, (u_char *)tHaddr, myaddr, (u_char *)ClientHwAddr);
  memset(&addr,0,sizeof(struct sockaddr));
  memcpy(addr.sa_data,IfNameExt,IfNameExt_len);
  while (1) { 
    do {
      if (i++ > 4) {
        if (DebugFlag)
	      printf("have tried 4 times and have nothing to receive.\n");
	return(0);
      }
      if (sendto(dhcpSocket, &ArpMsgSend,len, 0, &addr, sizeof(struct sockaddr)) == -1) {
        perror("check IP sendto");
	exit(1);
      }
    }
    while (waitfd(dhcpSocket, 50000));
    if (DebugFlag == 1)
      printf("i have something to read\n");
    do {
      memset(&ArpMsgRecv, 0, sizeof(arpMessage));
      j = sizeof(struct sockaddr);
      if (recvfrom(dhcpSocket, &ArpMsgRecv, sizeof(arpMessage), 0, (struct sockaddr *)&addr, (socklen_t *)&j) == -1) {
        perror("recvfrom error\n");
	return(-1);
      }
      if (ArpMsgRecv.ethhdr.h_proto != htons(ETHERTYPE_ARP))
        continue;
      if (ArpMsgRecv.operation == htons(ARPOP_REPLY)) {
        printf("ARPOP_REPLY received from %u.%u.%u.%u for %u.%u.%u.%u\n",
				ArpMsgRecv.sInaddr[0],ArpMsgRecv.sInaddr[1],
				ArpMsgRecv.sInaddr[2],ArpMsgRecv.sInaddr[3],
				ArpMsgRecv.tInaddr[0],ArpMsgRecv.tInaddr[1],
				ArpMsgRecv.tInaddr[2],ArpMsgRecv.tInaddr[3]);
      }
      else {
        continue;
      }
      if (memcmp(ArpMsgRecv.tHaddr, ClientHwAddr, ETH_ALEN)) {
        printf("target hardware address mismatch: %02X.%02X.%02X.%02X.%02X.%02X received, %02X.%02X.%02X.%02X.%02X.%02X expected\n",
				ArpMsgRecv.tHaddr[0],ArpMsgRecv.tHaddr[1],ArpMsgRecv.tHaddr[2],
				ArpMsgRecv.tHaddr[3],ArpMsgRecv.tHaddr[4],ArpMsgRecv.tHaddr[5],
				ClientHwAddr[0],ClientHwAddr[1],
				ClientHwAddr[2],ClientHwAddr[3],
				ClientHwAddr[4],ClientHwAddr[5]);
	continue;
      }
      if (memcmp(&ArpMsgRecv.sInaddr, &taddr, 4)) {
        printf("sender IP address mismatch: %u.%u.%u.%u received, %u.%u.%u.%u expected\n",
				ArpMsgRecv.sInaddr[0],ArpMsgRecv.sInaddr[1],ArpMsgRecv.sInaddr[2],ArpMsgRecv.sInaddr[3],
				((unsigned char *)&taddr)[0],
				((unsigned char *)&taddr)[1],
				((unsigned char *)&taddr)[2],
				((unsigned char *)&taddr)[3]);
	continue;
      }
      return 1; //some has used this ip address
    }
    while (waitfd(dhcpSocket, 50000 == 0));
  }
}	

//send gratuitous arp to inform others
int sendGraArp(u_long addr)
{
  //send gratuitous arp 
  arpMessage graArpMsg;
  char tHaddr[6] = {'\xff','\xff','\xff','\xff','\xff','\xff'};
  int len;
  struct sockaddr saddr;
  setArpMsg(&graArpMsg, ARPOP_REPLY, addr, (u_char *)tHaddr, addr, (u_char *)ClientHwAddr);
  len = sizeof(arpMessage);
  memset(&saddr,0,sizeof(struct sockaddr));
  memcpy(saddr.sa_data,IfNameExt,IfNameExt_len);
  if ( sendto(dhcpSocket, &graArpMsg, len, 0, &saddr, sizeof(struct sockaddr)) == -1 ) {
    perror("send gratuitous arp");
    exit(1);
  }
  if (DebugFlag == 1)
    printf("i have already informed others my new IP\n");
  return(1);

}

int sendRelArp()  /* sends UNARP message, cf. RFC1868 */
{
  arpMessage ReleaseArp;
  struct sockaddr addr;
  int len;
  char tHaddr[6] = {'\xff','\xff','\xff','\xff','\xff','\xff'};
  	
  memset(&ReleaseArp,0,sizeof(arpMessage));
  setArpMsg(&ReleaseArp, ARPOP_REPLY, inaddr_broadcast, (u_char *)tHaddr, DhcpIface.ciaddr, (u_char *)ClientHwAddr);
  memset(&addr,0,sizeof(struct sockaddr));
  memcpy(addr.sa_data,IfNameExt,IfNameExt_len);
  len = sizeof(arpMessage);
  if (sendto(dhcpSocket, &ReleaseArp, len, 0, &addr, sizeof(struct sockaddr)) == -1) {
    printf("arpRelease: sendto: %m\n");
    return -1;
  }
  return 0;
}

