#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <errno.h>


#include "socklog.h"
#include "msg_shm_ui.h"
#include "template_scan_element_type.h"


#define _COUPLE_
#define _NO_COUPLE_ 1
#define CAPACITY 100



int open_ipc_shm(key_t keyShm, int *shmid)
{
	int tmpShmid = 0;

#ifdef _COUPLE_
	//1. open The key point IPC shmory
	if((tmpShmid = shmget(keyShm, 0, 0)) < 0 )
	{
		socket_log(SocketLevel[4], -1, "Err: func shmget() %s", strerror(errno)); 
		return -1;
	}
#endif

#ifdef _NO_COUPLE_
	if((tmpShmid = shmget(keyShm, CAPACITY * sizeof(templateScanElement), 0666 | IPC_CREAT | IPC_EXCL)) < 0)
	{
		//2.1 程序启动后, 存在原来的共享内存,先删除,再创建新的
		if(errno == EEXIST)
		{
			tmpShmid = shmget(keyShm, 0, 0);
			if(tmpShmid < 0)
			{
				socket_log(SocketLevel[4], -1, "Err: func shmget() 0666 | IPC_CREAT"); 			
				return -1;
			}				
		}	
	} 
#endif

	*shmid = tmpShmid;


	return 0;
}


int link_shmid_process(uiIpcHandle *handle, void **shared_memory)
{
	
	if(!handle || !shared_memory)
	{
		socket_log(SocketLevel[4], -1, "Err : handle : %p, shared_memory : %p", handle, shared_memory);
		return  -1;
	}


	//1.link The shmid in The process 
	*shared_memory = shmat(handle->shmid, NULL, 0);
	if (*shared_memory == (void *)-1) 
	{
		socket_log(SocketLevel[4], -1, "Err: func shmat() %s ", strerror(errno));
		return -1;
	}

	return 0;
}



int destory_shmory(int shmid)
{
	int ret = 0;
	
	ret = shmctl(shmid,  IPC_RMID, NULL);
	if(ret < 0)
	{
		if(errno == EINVAL)
			return 0;
		else if(errno == EIDRM)
			return 0;
		else
		{
			printf("Err: func shmctl() IPC_RMID %s, [%d], [%s]\n", strerror(errno), __LINE__, __FILE__);
			socket_log(SocketLevel[2], ret, "Err: func shmctl() IPC_RMID  %s", strerror(errno)); 		
			return -1;
		}		
	}	

	return ret;
}


