#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <net/route.h>
#include <net/if.h>
#include <arpa/nameser.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <resolv.h>
#include <netdb.h>
#include <net/route.h>
#include "client.h"
#include "autoip.h"

extern	int dhcpSocket;
extern  int udpFooSocket;
extern	int prev_ip_addr;
extern	int Window;
extern  int SetDHCPDefaultRoutes;
extern	struct in_addr default_router;
extern  int RouteMetric;
extern	int DebugFlag;
extern	int SetHostName;
extern	int IfNameExt_len;
extern	char *IfNameExt; 
extern	char **ProgramEnviron;
extern	unsigned char ClientHwAddr[ETH_ALEN],*ClientID;
extern	dhcpInterface DhcpIface;
extern	dhcpOptions DhcpOptions;
extern	const dhcpMessage *DhcpMsgRecv;
char hostinfo_file[128];
int have_info=0; /* set once we have written the hostinfo file */

/* Note: Legths initialised to negative to allow us to distinguish between "empty" and "not set" */
char InitialHostName[HOSTNAME_MAX_LEN];
int InitialHostName_len=-1;

//int sendGraArp(u_long);
/*****************************************************************************/
int setDefaultRoute(route_addr)
char *route_addr;
{
struct	rtentry		rtent;
struct	sockaddr_in	*p;

memset(&rtent,0,sizeof(struct rtentry));
p = (struct sockaddr_in *)&rtent.rt_dst;
p->sin_family =	AF_INET;
p->sin_addr.s_addr = 0;
p = (struct sockaddr_in *)&rtent.rt_gateway;
p->sin_family =	AF_INET;
memcpy(&p->sin_addr.s_addr,route_addr,4);
p = (struct sockaddr_in *)&rtent.rt_genmask;
p->sin_family =	AF_INET;
p->sin_addr.s_addr = 0;
rtent.rt_dev = IfNameExt;
rtent.rt_metric	= RouteMetric;
rtent.rt_window	= Window;
rtent.rt_flags = RTF_UP|RTF_GATEWAY|(Window ? RTF_WINDOW : 0);
if ( ioctl(dhcpSocket,SIOCADDRT,&rtent) == -1 )
  {
    if ( errno == ENETUNREACH )    /* possibly gateway is over the bridge */
      {                            /* try adding a route to gateway first */
	memset(&rtent,0,sizeof(struct rtentry));
	p = (struct sockaddr_in *)&rtent.rt_dst;
	p->sin_family =	AF_INET;
	memcpy(&p->sin_addr.s_addr,route_addr,4);
	p = (struct sockaddr_in *)&rtent.rt_gateway;
	p->sin_family =	AF_INET;
	p->sin_addr.s_addr = 0;
	p = (struct sockaddr_in *)&rtent.rt_genmask;
	p->sin_family =	AF_INET;
	p->sin_addr.s_addr = 0xffffffff;
	rtent.rt_dev		=	IfNameExt;
	rtent.rt_metric     =	  RouteMetric;
	rtent.rt_flags      =	  RTF_UP|RTF_HOST;
	if ( ioctl(dhcpSocket,SIOCADDRT,&rtent) == 0 )
	  {
	    memset(&rtent,0,sizeof(struct rtentry));
	    p			=	(struct sockaddr_in *)&rtent.rt_dst;
	    p->sin_family	=	AF_INET;
	    p->sin_addr.s_addr	=	0;
	    p			=	(struct sockaddr_in *)&rtent.rt_gateway;
	    p->sin_family	=	AF_INET;
	    memcpy(&p->sin_addr.s_addr,route_addr,4);
	    p			=	(struct sockaddr_in *)&rtent.rt_genmask;
	    p->sin_family	=	AF_INET;
	    p->sin_addr.s_addr	=	0;
	    rtent.rt_dev	=	IfNameExt;
	    rtent.rt_metric	=	RouteMetric;
	    rtent.rt_window	=	Window;
	    rtent.rt_flags	=	RTF_UP|RTF_GATEWAY|(Window ? RTF_WINDOW : 0);
	    if ( ioctl(dhcpSocket,SIOCADDRT,&rtent) == -1 )
	      {
		syslog(LOG_ERR,"dhcpConfig: ioctl SIOCADDRT: %m\n");
		return -1;
	      }
	  }
      }
    else
      {
	syslog(LOG_ERR,"dhcpConfig: ioctl SIOCADDRT: %m\n");
	return -1;
      }
  }
return 0;
}

int dhcpConfig()
{
  int i;
 // FILE *f;
//  char hostinfo_file_old[128];
  struct ifreq ifr;
 // struct rtentry rtent;
  struct sockaddr_in *p = (struct sockaddr_in *)&(ifr.ifr_addr);
  struct hostent *hp=NULL;
  char *dname=NULL;
  int dname_len=0;

  memset(&ifr,0,sizeof(struct ifreq));
  memcpy(ifr.ifr_name,IfNameExt,IfNameExt_len);
  p->sin_family = AF_INET;
  p->sin_addr.s_addr = DhcpIface.ciaddr;
  /* setting IP address */
  if (ioctl(dhcpSocket,SIOCSIFADDR,&ifr) == -1) {
    perror("dhcpConfig: ioctl SIOCSIFADDR\n");
    return (-1);
  }
  memcpy(&p->sin_addr.s_addr,DhcpOptions.val[subnetMask],4);
  /* setting netmask */
  if ( ioctl(dhcpSocket,SIOCSIFNETMASK,&ifr) == -1 ) {
    p->sin_addr.s_addr = 0xffffffff; /* try 255.255.255.255 */
    if (ioctl(dhcpSocket,SIOCSIFNETMASK,&ifr) == -1) {
      perror("dhcpConfig: ioctl SIOCSIFNETMASK\n");
      return -1;
    }
  }
  memcpy(&p->sin_addr.s_addr,DhcpOptions.val[broadcastAddr],4);
  if ( ioctl(dhcpSocket,SIOCSIFBRDADDR,&ifr) == -1 ) /* setting broadcast address */
    perror("dhcpConfig: ioctl SIOCSIFBRDADDR\n");
  sendGraArp(DhcpIface.ciaddr);
  if (DebugFlag)
    fprintf(stdout,"mydhcpcd: your IP address = %u.%u.%u.%u\n",
    ((unsigned char *)&DhcpIface.ciaddr)[0],
    ((unsigned char *)&DhcpIface.ciaddr)[1],
    ((unsigned char *)&DhcpIface.ciaddr)[2],
    ((unsigned char *)&DhcpIface.ciaddr)[3]);

  if ( SetDHCPDefaultRoutes ) {
    if ( DhcpOptions.len[routersOnSubnet] > 3 )
      for (i=0;i<DhcpOptions.len[routersOnSubnet];i+=4)
        setDefaultRoute(DhcpOptions.val[routersOnSubnet]);
  }
  else
    if ( default_router.s_addr > 0 ) 
      setDefaultRoute((char *)&(default_router.s_addr));
  
  if ( SetHostName ) {
    if ( ! DhcpOptions.len[hostName] ) {
      hp=gethostbyaddr((char *)&DhcpIface.ciaddr,
      sizeof(DhcpIface.ciaddr),AF_INET);
      if (hp) {
        dname=hp->h_name;
	while (*dname > 32)
          dname++;
	dname_len=dname-hp->h_name;
	DhcpOptions.val[hostName]=(char *)malloc(dname_len+1);
	DhcpOptions.len[hostName]=dname_len;
	memcpy((char *)DhcpOptions.val[hostName],
	hp->h_name,dname_len);
	((char *)DhcpOptions.val[hostName])[dname_len]=0;
	DhcpOptions.num++;
      }
    }
    if (InitialHostName_len<0 && gethostname(InitialHostName,sizeof(InitialHostName))==0) {
      InitialHostName_len=strlen(InitialHostName);
      if (DebugFlag)
        fprintf(stdout,"dhcpcd: orig hostname = %s\n",InitialHostName);
    }
    if (DhcpOptions.len[hostName]) {
      sethostname(DhcpOptions.val[hostName],DhcpOptions.len[hostName]);
      if (DebugFlag)
        fprintf(stdout,"dhcpcd: your hostname = %s\n", (char *)DhcpOptions.val[hostName]);
    }
  }
  if (*(unsigned int *)DhcpOptions.val[dhcpIPaddrLeaseTime] == 0xffffffff) {
    perror("infinite IP address lease time. Exiting\n");
    exit(0);
  }
  return 0;
}
