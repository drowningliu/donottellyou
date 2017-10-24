#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
#include "client.h"
#include "myarp.h"

extern char *ProgramName;
extern char *IfNameExt;
extern int DebugFlag;
extern int Persistent;
extern jmp_buf env;
extern void *(*currState)();

void sigHandler(sig)
int sig;
{
  if (sig == SIGCHLD) {
    waitpid(-1,NULL,WNOHANG);
    return;
  }
  if (sig == SIGALRM) {
    if (currState == &Bound)
      siglongjmp(env,1); /* this timeout is T1 */
        else {
          if (currState == &Renew)
            siglongjmp(env,2); /* this timeout is T2 */
          else {
	    if (currState == &Rebind)
	        siglongjmp(env,3);  /* this timeout is dhcpIpLeaseTime */
	    else {
              if (currState == &Reboot)
	        siglongjmp(env,4);  /* failed to acquire the same IP address */
	      else {
		if (DebugFlag) {
		       printf("timed out waiting for a valid DHCP server response\n");
		       printf("let's jump back to Init\n");
		}
		siglongjmp(env,5);
	      }  
	    }
	  }
        }
    }
  else {
    if (sig == SIGHUP) {
      Release();
	  /* allow time for final packets to be transmitted before shutting down     */
	  /* otherwise 2.0 drops unsent packets. fixme: find a better way than sleep */
      sleep(1);
    }
    
    if (sig == SIGUSR1) {
      siglongjmp(env,6);
      if (DebugFlag)
        printf("let's have a break until the network is reachable\n");
    }
    
    if (sig == SIGUSR2) {
      if (DebugFlag)
        printf("Use dhcp policy\n");
      return;
    }      
    
    syslog(LOG_ERR,"terminating on signal %d\n",sig);
  }
  if (!Persistent || sig != SIGTERM)
    Stop();
  exit(sig);
}




void signalSetup()
{
  int i;
  struct sigaction action;
  sigaction(SIGHUP,NULL,&action);
  action.sa_handler= &sigHandler;
  action.sa_flags = 0;
  for (i=1;i<16;i++) sigaction(i,&action,NULL);
  sigaction(SIGCHLD,&action,NULL);
  sigaction(SIGUSR1,&action,NULL);
}
