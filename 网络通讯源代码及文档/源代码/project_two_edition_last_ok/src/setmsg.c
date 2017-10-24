#include <string.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include "client.h"
#include "udpipgen.h"

extern dhcpMessage *DhcpMsgSend;
extern dhcpOptions DhcpOptions;
extern dhcpInterface DhcpIface;
extern char *HostName;
extern int HostName_len;
extern int DebugFlag;
extern int BeRFC1541;
extern unsigned	LeaseTime;
extern unsigned char ClientHwAddr[6];
extern udpipMessage UdpIpMsgSend;
extern int magic_cookie;
extern unsigned short dhcpMsgSize;
extern unsigned nleaseTime;
extern int BroadcastResp;
extern struct in_addr inform_ipaddr;


void setDhcpDiscover(xid)
unsigned xid;
{
  register unsigned char *p = DhcpMsgSend->options + 4;
/* build Ethernet header */
  memset(&UdpIpMsgSend,0,sizeof(udpipMessage));
  memcpy(UdpIpMsgSend.ethhdr.ether_dhost,MAC_BCAST_ADDR,ETH_ALEN);
  memcpy(UdpIpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  UdpIpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_IP);
  DhcpMsgSend->op = DHCP_BOOTREQUEST;
  DhcpMsgSend->htype = ARPHRD_ETHER;
  DhcpMsgSend->hlen = ETH_ALEN;
  DhcpMsgSend->xid = xid;
  DhcpMsgSend->secs = htons(10);
  if ( BroadcastResp )
    DhcpMsgSend->flags = htons(BROADCAST_FLAG);
  memcpy(DhcpMsgSend->chaddr,ClientHwAddr,ETH_ALEN);
  memcpy(DhcpMsgSend->options,&magic_cookie,4);
  *p++ = dhcpMessageType;
  *p++ = 1;
  *p++ = DHCP_DISCOVER;
  *p++ = dhcpMaxMsgSize;
  *p++ = 2;
  memcpy(p,&dhcpMsgSize,2);
  p += 2;
  if (DhcpIface.ciaddr) {
    if (BeRFC1541)
      DhcpMsgSend->ciaddr = DhcpIface.ciaddr;
    else {
      *p++ = dhcpRequestedIPaddr;
      *p++ = 4;
      memcpy(p,&DhcpIface.ciaddr,4);
      p += 4; 
    }
  }
  *p++ = dhcpIPaddrLeaseTime;
  *p++ = 4;
  memcpy(p,&nleaseTime,4);
  p += 4;
  *p++ = dhcpParamRequest;
  *p++ = 15;
  *p++ = subnetMask;
  *p++ = routersOnSubnet;
  *p++ = dns;
  *p++ = hostName;
  *p++ = domainName;
  *p++ = rootPath;
  *p++ = defaultIPTTL;
  *p++ = broadcastAddr;
  *p++ = performMaskDiscovery;
  *p++ = performRouterDiscovery;
  *p++ = staticRoute;
  *p++ = nisDomainName;
  *p++ = nisServers;
  *p++ = ntpServers;
  *p++ = dnsSearchPath;
  if ( HostName ) {
    *p++ = hostName;
    *p++ = HostName_len;
    memcpy(p,HostName,HostName_len);
    p += HostName_len;
  }
  *p++ = dhcpClassIdentifier;
  *p++ = DhcpIface.class_len;
  memcpy(p,DhcpIface.class_id,DhcpIface.class_len);
  p += DhcpIface.class_len;
  memcpy(p,DhcpIface.client_id,DhcpIface.client_len);
  p += DhcpIface.client_len;
/* build UDP/IP header */
  udpipgen((udpiphdr *)UdpIpMsgSend.udpipmsg,0,INADDR_BROADCAST,
  htons(DHCP_CLIENT_PORT),htons(DHCP_SERVER_PORT),sizeof(dhcpMessage));
}


void setDhcpRequest(xid)
unsigned xid;
{
  register unsigned char *p = DhcpMsgSend->options + 4;
 /* build Ethernet header */
  memset(&UdpIpMsgSend,0,sizeof(udpipMessage));
  memcpy(UdpIpMsgSend.ethhdr.ether_dhost,MAC_BCAST_ADDR,ETH_ALEN);
  memcpy(UdpIpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  UdpIpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_IP);
  DhcpMsgSend->op = DHCP_BOOTREQUEST;
  DhcpMsgSend->htype = ARPHRD_ETHER;
  DhcpMsgSend->hlen = ETH_ALEN;
  DhcpMsgSend->xid = xid;
  DhcpMsgSend->secs = htons(10);
  if (BroadcastResp)
    DhcpMsgSend->flags = htons(BROADCAST_FLAG);
  memcpy(DhcpMsgSend->chaddr,ClientHwAddr,ETH_ALEN);
  memcpy(DhcpMsgSend->options,&magic_cookie,4);
  *p++ = dhcpMessageType;
  *p++ = 1;
  *p++ = DHCP_REQUEST;
  *p++ = dhcpMaxMsgSize;
  *p++ = 2;
  memcpy(p,&dhcpMsgSize,2);
  p += 2;
  *p++ = dhcpServerIdentifier;
  *p++ = 4;
  memcpy(p,DhcpOptions.val[dhcpServerIdentifier],4);
  p += 4;
  if (BeRFC1541)
    DhcpMsgSend->ciaddr = DhcpIface.ciaddr;
  else {
    *p++ = dhcpRequestedIPaddr;
    *p++ = 4;
    memcpy(p,&DhcpIface.ciaddr,4);
    p += 4;
  }
  if (DhcpOptions.val[dhcpIPaddrLeaseTime]) {
    *p++ = dhcpIPaddrLeaseTime;
    *p++ = 4;
    memcpy(p,DhcpOptions.val[dhcpIPaddrLeaseTime],4);
    p += 4;
  }
  *p++ = dhcpParamRequest;
  *p++ = 15;
  *p++ = subnetMask;
  *p++ = routersOnSubnet;
  *p++ = dns;
  *p++ = hostName;
  *p++ = domainName;
  *p++ = rootPath;
  *p++ = defaultIPTTL;
  *p++ = broadcastAddr;
  *p++ = performMaskDiscovery;
  *p++ = performRouterDiscovery;
  *p++ = staticRoute;
  *p++ = nisDomainName;
  *p++ = nisServers;
  *p++ = ntpServers;
  *p++ = dnsSearchPath;
  if (HostName) {
    *p++ = hostName;
    *p++ = HostName_len;
    memcpy(p,HostName,HostName_len);
    p += HostName_len;
  }
  *p++ = dhcpClassIdentifier;
  *p++ = DhcpIface.class_len;
  memcpy(p,DhcpIface.class_id,DhcpIface.class_len);
  p += DhcpIface.class_len;
  memcpy(p,DhcpIface.client_id,DhcpIface.client_len);
  p += DhcpIface.client_len;
  *p = endOption;
/* build UDP/IP header */
  udpipgen((udpiphdr *)UdpIpMsgSend.udpipmsg,0,INADDR_BROADCAST,
  htons(DHCP_CLIENT_PORT),htons(DHCP_SERVER_PORT),sizeof(dhcpMessage));
}


void setDhcpRenew(xid)
unsigned xid;
{
  register unsigned char *p = DhcpMsgSend->options + 4;
  memset(&UdpIpMsgSend,0,sizeof(udpipMessage));
  memcpy(UdpIpMsgSend.ethhdr.ether_dhost,DhcpIface.shaddr,ETH_ALEN);
  memcpy(UdpIpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  UdpIpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_IP);
  
  DhcpMsgSend->op = DHCP_BOOTREQUEST;
  DhcpMsgSend->htype = ARPHRD_ETHER;
  DhcpMsgSend->hlen = ETH_ALEN;
  DhcpMsgSend->xid = xid;
  DhcpMsgSend->secs = htons(10);
  if ( BroadcastResp )
    DhcpMsgSend->flags = htons(BROADCAST_FLAG);
  DhcpMsgSend->ciaddr = DhcpIface.ciaddr;
  memcpy(DhcpMsgSend->chaddr,ClientHwAddr,ETH_ALEN);
  memcpy(DhcpMsgSend->options,&magic_cookie,4);
  *p++ = dhcpMessageType;
  *p++ = 1;
  *p++ = DHCP_REQUEST;
  *p++ = dhcpMaxMsgSize;
  *p++ = 2;
  memcpy(p,&dhcpMsgSize,2);
  p += 2;
  *p++ = dhcpParamRequest;
  *p++ = 15;
  *p++ = subnetMask;
  *p++ = routersOnSubnet;
  *p++ = dns;
  *p++ = hostName;
  *p++ = domainName;
  *p++ = rootPath;
  *p++ = defaultIPTTL;
  *p++ = broadcastAddr;
  *p++ = performMaskDiscovery;
  *p++ = performRouterDiscovery;
  *p++ = staticRoute;
  *p++ = nisDomainName;
  *p++ = nisServers;
  *p++ = ntpServers;
  *p++ = dnsSearchPath;
  if (HostName) {
    *p++ = hostName;
    *p++ = HostName_len;
    memcpy(p,HostName,HostName_len);
    p += HostName_len;
  }
  *p++ = dhcpClassIdentifier;
  *p++ = DhcpIface.class_len;
  memcpy(p,DhcpIface.class_id,DhcpIface.class_len);
  p += DhcpIface.class_len;
  memcpy(p,DhcpIface.client_id,DhcpIface.client_len);
  p += DhcpIface.client_len;
  *p = endOption;
  udpipgen((udpiphdr *)UdpIpMsgSend.udpipmsg,
  DhcpIface.ciaddr,DhcpIface.siaddr,
  htons(DHCP_CLIENT_PORT),htons(DHCP_SERVER_PORT),sizeof(dhcpMessage));
}


void setDhcpRebind(xid)
unsigned xid;
{
  register unsigned char *p = DhcpMsgSend->options + 4;
  memset(&UdpIpMsgSend,0,sizeof(udpipMessage));
  memcpy(UdpIpMsgSend.ethhdr.ether_dhost,MAC_BCAST_ADDR,ETH_ALEN);
  memcpy(UdpIpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  UdpIpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_IP);

  DhcpMsgSend->op = DHCP_BOOTREQUEST;
  DhcpMsgSend->htype = ARPHRD_ETHER;
  DhcpMsgSend->hlen = ETH_ALEN;
  DhcpMsgSend->xid = xid;
  DhcpMsgSend->secs = htons(10);
  if ( BroadcastResp )
    DhcpMsgSend->flags = htons(BROADCAST_FLAG);
  DhcpMsgSend->ciaddr = DhcpIface.ciaddr;
  memcpy(DhcpMsgSend->chaddr,ClientHwAddr,ETH_ALEN);
  memcpy(DhcpMsgSend->options,&magic_cookie,4);
  *p++ = dhcpMessageType;
  *p++ = 1;
  *p++ = DHCP_REQUEST;
  *p++ = dhcpMaxMsgSize;
  *p++ = 2;
  memcpy(p,&dhcpMsgSize,2);
  p += 2;
  if (DhcpOptions.val[dhcpIPaddrLeaseTime]) {
    *p++ = dhcpIPaddrLeaseTime;
    *p++ = 4;
    memcpy(p,DhcpOptions.val[dhcpIPaddrLeaseTime],4);
    p += 4;
  }
  *p++ = dhcpParamRequest;
  *p++ = 15;
  *p++ = subnetMask;
  *p++ = routersOnSubnet;
  *p++ = dns;
  *p++ = hostName;
  *p++ = domainName;
  *p++ = rootPath;
  *p++ = defaultIPTTL;
  *p++ = broadcastAddr;
  *p++ = performMaskDiscovery;
  *p++ = performRouterDiscovery;
  *p++ = staticRoute;
  *p++ = nisDomainName;
  *p++ = nisServers;
  *p++ = ntpServers;
  *p++ = dnsSearchPath;
  if (HostName) {
    *p++ = hostName;
    *p++ = HostName_len;
    memcpy(p,HostName,HostName_len);
    p += HostName_len;
  }
  *p++ = dhcpClassIdentifier;
  *p++ = DhcpIface.class_len;
  memcpy(p,DhcpIface.class_id,DhcpIface.class_len);
  p += DhcpIface.class_len;
  memcpy(p,DhcpIface.client_id,DhcpIface.client_len);
  p += DhcpIface.client_len;
  *p = endOption;
  udpipgen((udpiphdr *)UdpIpMsgSend.udpipmsg,
  DhcpIface.ciaddr,INADDR_BROADCAST,
  htons(DHCP_CLIENT_PORT),htons(DHCP_SERVER_PORT),sizeof(dhcpMessage));
}


void setDhcpReboot(xid)
unsigned xid;
{
  register unsigned char *p = DhcpMsgSend->options + 4;
 /* build Ethernet header */
  memset(&UdpIpMsgSend,0,sizeof(udpipMessage));
  memcpy(UdpIpMsgSend.ethhdr.ether_dhost,MAC_BCAST_ADDR,ETH_ALEN);
  memcpy(UdpIpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  UdpIpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_IP);
  DhcpMsgSend->op = DHCP_BOOTREQUEST;
  DhcpMsgSend->htype = ARPHRD_ETHER;
  DhcpMsgSend->hlen = ETH_ALEN;
  DhcpMsgSend->xid = xid;
  DhcpMsgSend->secs = htons(10);
  if ( BroadcastResp )
    DhcpMsgSend->flags = htons(BROADCAST_FLAG);
  memcpy(DhcpMsgSend->chaddr,ClientHwAddr,ETH_ALEN);
  memcpy(DhcpMsgSend->options,&magic_cookie,4);
  *p++ = dhcpMessageType;
  *p++ = 1;
  *p++ = DHCP_REQUEST;
  *p++ = dhcpMaxMsgSize;
  *p++ = 2;
  memcpy(p,&dhcpMsgSize,2);
  p += 2;
  if ( BeRFC1541 )
    DhcpMsgSend->ciaddr = DhcpIface.ciaddr;
  else {
    *p++ = dhcpRequestedIPaddr;
    *p++ = 4;
    memcpy(p,&DhcpIface.ciaddr,4);
    p += 4;
  }
  *p++ = dhcpIPaddrLeaseTime;
  *p++ = 4;
  memcpy(p,&nleaseTime,4);
  p += 4;
  *p++ = dhcpParamRequest;
  *p++ = 15;
  *p++ = subnetMask;
  *p++ = routersOnSubnet;
  *p++ = dns;
  *p++ = hostName;
  *p++ = domainName;
  *p++ = rootPath;
  *p++ = defaultIPTTL;
  *p++ = broadcastAddr;
  *p++ = performMaskDiscovery;
  *p++ = performRouterDiscovery;
  *p++ = staticRoute;
  *p++ = nisDomainName;
  *p++ = nisServers;
  *p++ = ntpServers;
  *p++ = dnsSearchPath;
  if (HostName) {
      *p++ = hostName;
      *p++ = HostName_len;
      memcpy(p,HostName,HostName_len);
      p += HostName_len;
  }
  *p++ = dhcpClassIdentifier;
  *p++ = DhcpIface.class_len;
  memcpy(p,DhcpIface.class_id,DhcpIface.class_len);
  p += DhcpIface.class_len;
  memcpy(p,DhcpIface.client_id,DhcpIface.client_len);
  p += DhcpIface.client_len;
  *p = endOption;
  udpipgen((udpiphdr *)UdpIpMsgSend.udpipmsg,0,INADDR_BROADCAST,
  htons(DHCP_CLIENT_PORT),htons(DHCP_SERVER_PORT),sizeof(dhcpMessage));
}


void setDhcpRelease(xid)
unsigned xid;
{
  register unsigned char *p = DhcpMsgSend->options + 4;
  memset(&UdpIpMsgSend,0,sizeof(udpipMessage));
  memcpy(UdpIpMsgSend.ethhdr.ether_dhost,DhcpIface.shaddr,ETH_ALEN);
  memcpy(UdpIpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  UdpIpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_IP);

  DhcpMsgSend->op = DHCP_BOOTREQUEST;
  DhcpMsgSend->htype = ARPHRD_ETHER;
  DhcpMsgSend->hlen = ETH_ALEN;
  DhcpMsgSend->xid = xid;
  DhcpMsgSend->ciaddr =	DhcpIface.ciaddr;
  memcpy(DhcpMsgSend->chaddr,ClientHwAddr,ETH_ALEN);
  memcpy(DhcpMsgSend->options,&magic_cookie,4);
  *p++ = dhcpMessageType;
  *p++ = 1;
  *p++ = DHCP_RELEASE;
  *p++ = dhcpServerIdentifier;
  *p++ = 4;
  memcpy(p,DhcpOptions.val[dhcpServerIdentifier],4);
  p += 4;
  memcpy(p,DhcpIface.client_id,DhcpIface.client_len);
  p += DhcpIface.client_len;
  *p = endOption;
  udpipgen((udpiphdr *)UdpIpMsgSend.udpipmsg,DhcpIface.ciaddr,
  DhcpIface.siaddr,htons(DHCP_CLIENT_PORT),htons(DHCP_SERVER_PORT),
  sizeof(dhcpMessage));
}


void setDhcpDecline(xid)
unsigned xid;
{
  register unsigned char *p = DhcpMsgSend->options + 4;
  memset(&UdpIpMsgSend,0,sizeof(udpipMessage));
  memcpy(UdpIpMsgSend.ethhdr.ether_dhost,DhcpIface.shaddr,ETH_ALEN);
  memcpy(UdpIpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  UdpIpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_IP);

  DhcpMsgSend->op = DHCP_BOOTREQUEST;
  DhcpMsgSend->htype = ARPHRD_ETHER;
  DhcpMsgSend->hlen = ETH_ALEN;
  DhcpMsgSend->xid = xid;
  memcpy(DhcpMsgSend->chaddr,ClientHwAddr,ETH_ALEN);
  memcpy(DhcpMsgSend->options,&magic_cookie,4);
  *p++ = dhcpMessageType;
  *p++ = 1;
  *p++ = DHCP_DECLINE;
  *p++ = dhcpServerIdentifier;
  *p++ = 4;
  memcpy(p,DhcpOptions.val[dhcpServerIdentifier],4);
  p += 4;
  if ( BeRFC1541 )
    DhcpMsgSend->ciaddr = DhcpIface.ciaddr;
  else {
      *p++ = dhcpRequestedIPaddr;
      *p++ = 4;
      memcpy(p,&DhcpIface.ciaddr,4);
      p += 4;
  }
  memcpy(p,DhcpIface.client_id,DhcpIface.client_len);
  p += DhcpIface.client_len;
  *p = endOption;
  udpipgen((udpiphdr *)UdpIpMsgSend.udpipmsg,0,
  DhcpIface.siaddr,htons(DHCP_CLIENT_PORT),htons(DHCP_SERVER_PORT),
  sizeof(dhcpMessage));
}



void setDhcpInform(xid)
unsigned xid;
{
  register unsigned char *p = DhcpMsgSend->options + 4;

  memset(&UdpIpMsgSend,0,sizeof(udpipMessage));
  memcpy(UdpIpMsgSend.ethhdr.ether_dhost,MAC_BCAST_ADDR,ETH_ALEN);
  memcpy(UdpIpMsgSend.ethhdr.ether_shost,ClientHwAddr,ETH_ALEN);
  UdpIpMsgSend.ethhdr.ether_type = htons(ETHERTYPE_IP);

  DhcpMsgSend->op = DHCP_BOOTREQUEST;
  DhcpMsgSend->htype = ARPHRD_ETHER;
  DhcpMsgSend->hlen = ETH_ALEN;
  DhcpMsgSend->xid = xid;
  DhcpMsgSend->secs = htons(10);
  if ( BroadcastResp )
    DhcpMsgSend->flags = htons(BROADCAST_FLAG);
  memcpy(DhcpMsgSend->chaddr,ClientHwAddr,ETH_ALEN);
  DhcpMsgSend->ciaddr = inform_ipaddr.s_addr;
  memcpy(DhcpMsgSend->options,&magic_cookie,4);
  *p++ = dhcpMessageType;
  *p++ = 1;
  *p++ = DHCP_INFORM;
  *p++ = dhcpMaxMsgSize;
  *p++ = 2;
  memcpy(p,&dhcpMsgSize,2);
  p += 2;
  *p++ = dhcpParamRequest;
  *p++ = 15;
  *p++ = subnetMask;
  *p++ = routersOnSubnet;
  *p++ = dns;
  *p++ = hostName;
  *p++ = domainName;
  *p++ = rootPath;
  *p++ = defaultIPTTL;
  *p++ = broadcastAddr;
  *p++ = performMaskDiscovery;
  *p++ = performRouterDiscovery;
  *p++ = staticRoute;
  *p++ = nisDomainName;
  *p++ = nisServers;
  *p++ = ntpServers;
  *p++ = dnsSearchPath;
  if (HostName) {
      *p++ = hostName;
      *p++ = HostName_len;
      memcpy(p,HostName,HostName_len);
      p += HostName_len;
  }
  *p++ = dhcpClassIdentifier;
  *p++ = DhcpIface.class_len;
  memcpy(p,DhcpIface.class_id,DhcpIface.class_len);
  p += DhcpIface.class_len;
  memcpy(p,DhcpIface.client_id,DhcpIface.client_len);
  p += DhcpIface.client_len;
  udpipgen((udpiphdr *)UdpIpMsgSend.udpipmsg,0,INADDR_BROADCAST,
  htons(DHCP_CLIENT_PORT),htons(DHCP_SERVER_PORT),sizeof(dhcpMessage));
}


