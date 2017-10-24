#include <sys/stat.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "client.h"
#include "handler.h"
#include "udpipgen.h"

struct in_addr inform_ipaddr, default_router; 
char *ProgramName = NULL;
char **ProgramEnviron = NULL;
char *IfNameExt	= DEFAULT_IFNAME;
int IfNameExt_len = DEFAULT_IFNAME_LEN;
char *HostName = NULL;
int HostName_len = 0;
char *ClassID =	NULL;
int ClassID_len	= 0;
unsigned char *ClientID	= NULL;
int ClientID_len = 0;
void *(*currState)() = &Reboot;
int DebugFlag =	0;
int BeRFC1541 =	0;
unsigned LeaseTime = DEFAULT_LEASETIME;
int SetHostName	= 0;
int BroadcastResp = 0;
time_t TimeOut = DEFAULT_TIMEOUT;
int magic_cookie = 0;
unsigned short dhcpMsgSize = 0;
unsigned nleaseTime = 0;
int DoCheckSum = 0;
int SendSecondDiscover = 0;
int Persistent = 1;
int DownIfaceOnStop = 1;
int autoip_flag = 0;
int autoip_sleep = 300;
int SetDHCPDefaultRoutes= 1;
int RouteMetric	= 1;
int Window = 0;

typedef void (*sighandler_t)(int);


int entrance()
{
    int j;

    j=open("/dev/null",O_RDWR);
    while (j < 2 && j >= 0) 
        j = dup(j);
    if (j > 2) 
        close(j);

    umask(022);
    signalSetup();
    magic_cookie = htonl(MAGIC_COOKIE);   
    dhcpMsgSize = htons(sizeof(dhcpMessage)+sizeof(udpiphdr));
    nleaseTime = htonl(LeaseTime);
    if (TimeOut != 0)
        alarm(TimeOut);
    while ( (currState!= &Bound) && (autoip_flag == 0)){
        if ( (currState=(*currState)()) == NULL ) 
            exit(1);
    }


    if (autoip_flag == 1) {
        currState = &Init;
        while ( currState!= &Bound )
            if ( (currState=(*currState)()) == NULL ) 
                exit(1);
    }
    autoip_flag = 0;
    alarm(0);     //cancel the alarm set previously
    while ( currState )
        currState=(*currState)();
    exit(1);
}


/*
   void func_data(int arg)
   { 
   int mypid = 0; 
   while( ( mypid = waitpid(-1, NULL, WNOHANG ) ) > 0)	
   ;
   return ;	
   } 


   int pox_system(char *cmd_line)
   {
   int ret = 0;
   sighandler_t old_handler;
   old_handler = signal(SIGCHLD, SIG_DFL);
   ret = system(cmd_line);
   signal(SIGCHLD, old_handler);
   return ret;
   }

   int set_local_ip_gate(char *localIP, char *gate)
   {
   int ret = 0;
   char buf[1024] = { 0 };

   sprintf(buf, "ifconfig eth0 %s", localIP);
   ret = pox_system(buf);
   if(ret < 0)
   {
   perror("system ifconfig");
   return ret ;    
   }

   memset(buf, 0, sizeof(buf));
   sprintf(buf, "route add default gw %s", gate);
   ret = pox_system(buf);
   if(ret < 0)
   {
   perror("system route");
   return ret ;    
   }


   return ret;
   }
   */

int dhcp_interface(pid_t *pid)
{
    int ret = 0;

    //  signal(SIGCHLD, func_data);	
    pid_t grandPaPid;
    //1.回收子进程
    grandPaPid  = fork();
    if(grandPaPid > 0)		
    {
        *pid = grandPaPid;
        return 0;
    }	
    else if(grandPaPid == 0)
    {
    	 if((ret = entrance()) < 0)					
    	 	return ret;    	 	
    }   
    else if(grandPaPid < 0)	
    {
        perror("fork");
        ret = -1;
    }	
    return ret;
}

