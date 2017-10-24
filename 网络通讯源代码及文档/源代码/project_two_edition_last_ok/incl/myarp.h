#ifndef MY_ARP_H_
#define MY_ARP_H_

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

#define ETHERTYPE_ARP	0x0806
#define ETHERTYPE_IP	0x0800

typedef struct arpMessage
{
  struct ethhdr ethhdr;
  u_short htype;	/* hardware type (must be ARPHRD_ETHER) */
  u_short ptype;	/* protocol type (must be ETHERTYPE_IP) */
  u_char  hlen;		/* hardware address length (must be 6) */
  u_char  plen;		/* protocol address length (must be 4) */
  u_short operation;	/* ARP opcode */
  u_char  sHaddr[ETH_ALEN];	/* sender's hardware address */
  u_char  sInaddr[4];	/* sender's IP address */
  u_char  tHaddr[ETH_ALEN];	/* target's hardware address */
  u_char  tInaddr[4];	/* target's IP address */
  u_char  pad[18];	/* pad for min. Ethernet payload (60 bytes) */
}arpMessage;

/**
 *Description: blocks a set of socket descriptions for a certain time
               until they are ready. 
 *@param fd: a socket description
 *@param usec: waiting time
 *Return 1 when timeout occurs, return 0 if any device is ready.
 */
int waitfd(int fd, time_t usec);

/**
 *Description: builds different kinds of arp messages
 *@param msg: the msg to be built
 *@param opcode: indicates the kind of operation
 *@param tipaddr: target ip address
 *@param thaddr: target hardware address
 *@param sipaddr: source ip address
 *@param shaddr: source hardware address
 */
void setArpMsg(arpMessage *msg, u_short opcode, u_long tipaddr, u_char *thaddr, u_long sipaddr, u_char *shaddr);

/**
 *Description: checks whether an ip address is in use or not
 *@param taddr: target ip address to be checked
 *return 1 if this ip address is in use, otherwise return 0
 */ 
int checkIP(u_long taddr);  

/**
 *Description: sends Gratuitous Arp messages to inform others of this ip address
 *@param addr: the ip address to be informed
 *return -1 when an error occurs, otherwise return 0
 */
int sendGraArp(u_long addr);


#endif
