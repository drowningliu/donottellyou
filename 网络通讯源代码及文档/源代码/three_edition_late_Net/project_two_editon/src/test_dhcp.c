#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dhcp.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include "dhcp.h"

int main()
{
    int ret = 0;
    int flag = 0;    
    pid_t childPID;


    while(1)
    {
        flag = getchar();

        switch (flag)
        {
        case 49:        //1
            if((ret = dhcp_interface(&childPID)) < 0)
                printf("Error : func dhcp_interface() [%d], [%s]\n", __LINE__, __FILE__);
            break;
        case 50:        //2
            if((ret = set_local_ip_gate("192.168.1.156", "127.0.0.1")) < 0 )
                printf("Error : func set_local_ip_gate() [%d], [%s]\n", __LINE__, __FILE__);
            break;            
        }

        if(flag == 51)
        {
            ret = kill(childPID, SIGKILL);
            return ret;
        }

        getchar();
    }

    return ret;
}
