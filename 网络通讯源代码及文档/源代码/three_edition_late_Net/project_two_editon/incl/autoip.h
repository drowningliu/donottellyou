#ifndef AUTOIP_H_
#define AUTOIP_H_

#define LINK_LOCAL_NETADDR	0xA9FE0000	/* 169.254.0.0 */
#define ETHERTYPE_ARP	0x0806
#define ETHERTYPE_IP	0x0800

/**
 *Description: this function is used to automatically generate an ip address 
               when dhcp service is not accessible.
 *return -1 when an error occurs, otherwise return 0	     
 */  
int autoip(void);


#endif
