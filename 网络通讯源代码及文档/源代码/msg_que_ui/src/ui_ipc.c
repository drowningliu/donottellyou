#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <errno.h>

#include "ui_ipc.h"
#include "socklog.h"
#include "msg_que_ui.h"
#include "msg_shm_ui.h"

uiIpcHandle *g_srcHandle = NULL;

#define LINESIZE  1024


int init_ipc_ui_include(key_t keyMsg, key_t keyShm, uiIpcHandle **srcHandle)
{
	int  ret = 0;
	int  queid = 0;
	int  shmid = 0;	
	uiIpcHandle *handle = NULL;

	if(!srcHandle)
	{
		ret = -1;
		myprint("err : handle : %p", srcHandle);
		socket_log(SocketLevel[4], ret, "err : handle : %p", srcHandle); 	
		return ret ;
	}

	//1.create list
	if((handle = create_list()) == NULL)
	{
		ret = -1;
		myprint("err : func create_list()");
		socket_log(SocketLevel[4], ret, "err : func create_list()");		
		return ret;
	}
	
	//1.open The IPC Queue
	ret = ui_open_fun_mesque(keyMsg, &queid);
	if(ret < 0)
	{
		myprint("err : func ui_open_fun_mesque()");
		socket_log(SocketLevel[4], ret, "err : func ui_open_fun_mesque()");		
		return ret ;
	}
	handle->msqid = queid;


	//2.open The IPC Shmory
	ret = open_ipc_shm(keyShm, &shmid);
	if(ret < 0)
	{
		myprint("err : func open_ipc_shm()");
		socket_log(SocketLevel[4], ret, "Err: func open_ipc_shm() "); 		
		return ret = -1;
	}
	handle->shmid = shmid;

	
	//3.link The shmid in The process 	
	ret = link_shmid_process(handle, (void **)&(handle->share_pool));
	if(ret < 0)
	{
		myprint("err : func link_shmid_process()");
		socket_log(SocketLevel[4], ret, "Err: func link_shmid_process() ");		
		return ret = -1;
	}


	//3. assignment value
	*srcHandle = handle;
	
	return ret;
}



int destroy_ipc_Que_Shm_include(uiIpcHandle *handle)
{
	int ret = 0;

	if(!handle)
	{
		socket_log(SocketLevel[4], ret, "Err: handle : %p ", handle);				
		return ret = -1;
	}
	
	if((ret = ui_close_fun_mesque(handle->msqid)) < 0)	
	{
		socket_log(SocketLevel[4], ret, "Err: func ui_close_fun_mesque() ");				
		return ret = -1;
	}	

	if((ret = shmdt(handle->share_pool)) < 0)
	{
		socket_log(SocketLevel[4], ret, "Err: func shmdt() "); 			
		return ret = -1;
	}
	
	if((ret = destory_shmory(handle->shmid)) < 0)
	{
		socket_log(SocketLevel[4], ret, "Err: func destory_shmory() ");				
		return ret = -1;
	}
	
	if((ret = destroy_list(handle)) < 0)
	{
		socket_log(SocketLevel[4], ret, "Err: func destroy_list() "); 			
		return ret = -1;
	}

	
	return ret;
}


int init_ipc_ui(key_t keyMsg, key_t keyShm)
{
	int ret = 0;
	
	if((ret = init_ipc_ui_include(keyMsg, keyShm, &g_srcHandle)) < 0)
	{
		ret = -1;
		myprint("Err: func init_ipc_ui_include() ");
		socket_log(SocketLevel[4], ret, "Err: func init_ipc_ui_include() ");			
		return ret;
	}

	return ret;
}

int ui_send_fun_mesque( const char *str)
{
	int ret = 0;
	
	if((ret = ui_send_fun_mesque_include(g_srcHandle, str)) < 0)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Err: func ui_send_fun_mesque_include() ");			
		return ret;
	}
	
	return ret;
}

int ui_recv_fun_mesque(char **str)
{
	int ret = 0;	
	char *recvBuf = NULL;

	if((recvBuf = malloc(LINESIZE)) == NULL)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Err: func malloc() "); 		
		return ret;
	}
	memset(recvBuf, 0, LINESIZE);
	
	if((ret = ui_recv_fun_mesque_include(g_srcHandle, recvBuf)) < 0)
	{
		if(ret == -2)
		{			
			//return ret;			//这种情况 : UI需要通知,使用返回值 -2 进行通知
			*str = recvBuf;
			return 0;				//这种情况 : UI不需要通知, 返回0;
		}
		ret = -1;
		socket_log(SocketLevel[4], ret, "Err: func ui_recv_fun_mesque_include() ");			
		return ret;
	}

	*str = recvBuf;
	
	return ret;
}

int destroy_ipc_Que_Shm()
{
	int ret = 0;
	
	if((ret = destroy_ipc_Que_Shm_include(g_srcHandle)) < 0)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Err: func destroy_ipc_Que_Shm_include() ");			
		return ret;
	}
	
	return ret;

}

int ui_free_fun_mesque(char *str)
{
	if(!str)
	{
		printf("Err : func ui_free_fun_mesque() str : %p", str);
		return -1;
	}

	free(str);
	return 0;
}


