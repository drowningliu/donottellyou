/***************************************************************************
 * Author     : yuanliixjtu@gmail.com			_________________  *
 * Date       : 06/30/2006 14:26:29			|    | This file | *
 * File name  : netstatus.c	   			| vi |  powered  | *
 * Description: the implementation of get the net status|____|___________| *
 *									   *
 ***************************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/wireless.h>
#include "netstatus.h"

#define IFNAMESIZE	16
static char wiredName[IFNAMESIZE];
static char wirelessName[IFNAMESIZE];

int IsWiredAvailable(void)
{
#define BUF_LEN	1024
	int ret = 0;
	int i, fd, num;
	char buf[BUF_LEN];
	struct ifreq *ifr;
	struct iwreq wrq;
	struct ifconf conf;

	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	conf.ifc_len = BUF_LEN;
	conf.ifc_buf = buf;
	/* get all If configuration */
	if (ioctl(fd, SIOCGIFCONF, &conf) < 0) {
		perror("ioctl(SIOCGIFCONF)");
		close(fd);
		return -1;
	}

	num = conf.ifc_len / sizeof(struct ifreq);

	/* check each If */
	for (i = 0, ifr = conf.ifc_req; i < num; i++, ifr++) {
		if (ioctl(fd, SIOCGIFFLAGS, ifr) < 0) {
			perror("ioctl(SIOCGIFFLAGS)");
			continue;
		}

		if (ifr->ifr_flags & IFF_LOOPBACK)
			continue;

		/* skip the wireless If */
		sprintf(wrq.ifr_name, ifr->ifr_name, IFNAMESIZE);
		if (ioctl(fd, SIOCGIWNAME, &wrq) >= 0)
			continue;
		
		strncpy(wiredName, ifr->ifr_name, IFNAMESIZE);
		if (ifr->ifr_flags & IFF_RUNNING) {
			ret = 1;
			break;
		}
	}

	close(fd);
	return ret;
}

char *GetWiredIfname(void)
{
	if ('\0' == wiredName[0]) {
		/* check the status first, get a valid wired If name */
		IsWiredAvailable();
		if ('\0' == wiredName[0])
			/* If still can't get a valid If name, return NULL */
			return NULL;
		else
			return wiredName;
	} else
		return wiredName;
}

int IsWirelessAvailable(void)
{
	int ret = 0;
	int i, fd, num;
	char buf[BUF_LEN];
	char essid[IW_ESSID_MAX_SIZE + 1];
	struct ifreq *ifr;
	struct iwreq wrq;
	struct ifconf conf;
	int tmp;

	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	conf.ifc_len = BUF_LEN;
	conf.ifc_buf = buf;
	/* get all If configuration */
	if (ioctl(fd, SIOCGIFCONF, &conf) < 0) {
		perror("ioctl(SIOCGIFCONF)");
		close(fd);
		return -1;
	}

	num = conf.ifc_len / sizeof(struct ifreq);

	/* check each If */
	for (i = 0, ifr = conf.ifc_req; i < num; i++, ifr++) {
		if (ioctl(fd, SIOCGIFFLAGS, ifr) < 0) {
			perror("ioctl(SIOCGIFFLAGS)");
			continue;
		}

		if (ifr->ifr_flags & IFF_LOOPBACK)
			continue;

		/* skip the wired If */
		sprintf(wrq.ifr_name, ifr->ifr_name, IFNAMESIZE);
		if (ioctl(fd, SIOCGIWNAME, &wrq) < 0)
			continue;
		
		strncpy(wirelessName, ifr->ifr_name, IFNAMESIZE);
		wrq.u.essid.pointer = (caddr_t)essid;
		wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
		wrq.u.essid.flags = 0;
		tmp = ioctl(fd, SIOCGIWESSID, &wrq);
		printf("tmp=%d", tmp);
		printf("essid=%s\n", essid);
	}

	close(fd);
	return ret;
}

char *GetWirelessIfname(void)
{
	if ('\0' == wirelessName[0]) {
		/* check the status first, get a valid wireless If name */
		IsWirelessAvailable();
		if ('\0' == wirelessName[0])
			/* If still can't get a valid If name, return NULL */
			return NULL;
		else
			return wirelessName;
	} else
		return wirelessName;
}
