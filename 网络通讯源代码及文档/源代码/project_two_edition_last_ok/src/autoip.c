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
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include "myarp.h"
#include "autoip.h"

extern char *IfNameExt;	
extern int IfNameExt_len;	
extern int dhcpSocket;
extern unsigned char ClientHwAddr[ETH_ALEN];

static int set_flag(char *ifname, short flag, int skfd);
struct in_addr protectRange(struct in_addr ip);

static int set_flag(char *ifname, short flag, int skfd)
{
  struct ifreq ifr;
  strcpy(ifr.ifr_name, ifname);
  if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
    perror("get flag error\n");
    return (-1);
  }
  strcpy(ifr.ifr_name, ifname);
  ifr.ifr_flags |= flag;
  if (ioctl(skfd, SIOCSIFFLAGS, &ifr) < 0) {
    perror("set flags error\n");
    return (-1);
  }
  return (0);
}

struct in_addr protectRange(struct in_addr ip)
{
  unsigned long int host = inet_lnaof(ip) & IN_CLASSB_HOST;
  if (host >> 8 == 0)
    host = 0x100 | (0xff & host);
  else 
    if (host >> 8 == 255)
      host = 0xfe00 | (0xff & host);
  ip = inet_makeaddr((IN_CLASSB_NET & LINK_LOCAL_NETADDR), host);
  return ip;
}

int autoip(void)
{
  struct ifreq ifr;	/* interface request struct - if.h */
  struct sockaddr_in* addr;
  int result;
  u_long netmask;
  int skfd;
  int status;
  //char myHaddr[6];
  const char *ip2 = "255.255.0.0";
  struct in_addr ip;
  srand((unsigned)time(NULL));
  unsigned int r=rand();
  ip = inet_makeaddr((IN_CLASSB_NET & LINK_LOCAL_NETADDR), (IN_CLASSB_HOST & r));
  while ((status = checkIP(ip.s_addr))) {
    srand((unsigned)time(NULL) + 100);  //use time as the seed
    ip = inet_makeaddr((IN_CLASSB_NET & LINK_LOCAL_NETADDR), (IN_CLASSB_HOST & r));
    ip = protectRange(ip);  
  }
  if (status == -1) {
    return (-1);
  }
  netmask = inet_addr(ip2);
  skfd = socket(AF_INET, SOCK_DGRAM, 0);

   /* order is important. set IP address, and then the netmask */
   strcpy(ifr.ifr_name, IfNameExt);
   ifr.ifr_name[IfNameExt_len] = 0;
   addr = (struct sockaddr_in *) &ifr.ifr_addr; 
   bzero(addr, sizeof(struct sockaddr_in));
   addr->sin_family = AF_INET;
   addr->sin_addr.s_addr = ip.s_addr;
   if ((result=ioctl(skfd, SIOCSIFADDR, &ifr))==-1) {
     perror("set ip");
     exit(-1);
   }
   addr = (struct sockaddr_in *) & ifr.ifr_broadaddr;
   bzero(addr, sizeof(struct sockaddr_in));
   addr->sin_family = AF_INET;
   addr->sin_addr.s_addr = ip.s_addr & netmask | ~netmask;
   if (ioctl(skfd, SIOCSIFBRDADDR, &ifr) == -1) {
    perror("set broadcast");
    exit(-1);
   }
   addr = (struct sockaddr_in *) &ifr.ifr_netmask;
   bzero(addr, sizeof(struct sockaddr_in));
   addr->sin_family = AF_INET;
   addr->sin_addr.s_addr = netmask;
   if ((result=ioctl(skfd, SIOCSIFNETMASK, &ifr)) == -1) {
     perror("set netmask");
     exit(-1);
   }
   set_flag(IfNameExt, IFF_UP | IFF_BROADCAST | IFF_NOTRAILERS | IFF_RUNNING | IFF_MULTICAST , skfd);
   close(skfd);
   printf("Auto-IP started\n");
   sendGraArp(ip.s_addr);
   return(0);
}



