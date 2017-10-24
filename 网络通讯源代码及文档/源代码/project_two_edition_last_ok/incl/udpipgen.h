#ifndef UDPIPGEN_H
#define UDPIPGEN_H

#include <netinet/ip.h>

#ifndef IPDEFTTL
#define IPDEFTTL 64
#endif

struct ipovly
{
  int ih_next,ih_prev;
  u_char ih_x1;
  u_char ih_pr;
  u_short ih_len;
  struct in_addr ih_src;
  struct in_addr ih_dst;
} __attribute__((packed));

struct udphdr
{
  u_int16_t uh_sport;
  u_int16_t uh_dport;
  u_int16_t uh_ulen;
  u_int16_t uh_sum;
} __attribute__((packed));

typedef struct udpiphdr
{
  char ip[sizeof(struct ip)];
  char udp[sizeof(struct udphdr)];
} __attribute__((packed)) udpiphdr;

/**
 *Description: this functions generates the IP header of an UDP datagram
 *@param udpip: the header to be built
 *@param saddr: source ip addr
 *@param daddr: destination ip addr
 *@param sport: source port
 *@param dport: destination port
 *@param msglen: the length of the message
 */
void udpipgen(udpiphdr *udpip,unsigned int saddr,unsigned int daddr,
unsigned short sport,unsigned short dport,unsigned short msglen);

/**
 *Description: check the ip header of an UDP datagram
 *return 0 if no error occurs
 */
int udpipchk(udpiphdr *udpip);

#endif
