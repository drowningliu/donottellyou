#ifndef BUILDMSG_H
#define BUILDMSG_H

/**
 *Description: these functions are used to generate different types of Dhcp Messages
 */
void setDhcpDiscover(unsigned);
void setDhcpRequest(unsigned);
void setDhcpRenew(unsigned);
void setDhcpRebind(unsigned);
void setDhcpReboot(unsigned);
void setDhcpRelease(unsigned);
void setDhcpDecline(unsigned);
void setDhcpInform(unsigned);

#endif
