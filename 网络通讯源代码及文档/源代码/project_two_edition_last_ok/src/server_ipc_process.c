#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<strings.h>
#include<ctype.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdint.h>
#include <sys/select.h>
#include <fcntl.h>
#include <setjmp.h>


#include "socklog.h"
#include "server_simple.h"
#include "msg_que_scan.h"
#include "server_ipc_process.h"
#include "func_compont.h"


sigjmp_buf g_sigEnv;
sigjmp_buf g_sigEnvEnd;
pid_t g_childPID = 0;
extern listNode *g_handle;


/*初始化： 完成功能： 1.开启服务器的端口监听，当有客户端连接时，开辟线程去执行业务，	2.开始IPC的初始化，创建链表成功，打开消息队列成功；
 *@param：ip ： 服务器IP地址；
 *@param：port：服务器监听端口
 *@param：key： 消息队列的键值；
 *@param：fileDir： 创建文件的目录。
 *@param：handle： 链表的句柄
 *@retval:   0 成功；  -1 服务器初始化失败； -2 IPC创建失败，服务器创建成功； -3  创建子线程失败 。
*/
int init_func(char *ip, int port, char *uploadDirParh, key_t key, char *fileDir, listNode **handle, pid_t *srcPid)
{
	int  ret = 0;
	pid_t pid ;
	
	memset(&g_sigEnv, 0, sizeof(sigjmp_buf));
	memset(&g_sigEnvEnd, 0, sizeof(g_sigEnvEnd));
	
	//1.创建子进程， 负责不同的业务
	pid = fork();
	if(pid < 0)
	{
		perror("fork");
		return ret = -3;
	}	
	else if(pid == 0)			//子进程, 初始化服务器的程序
	{
				
		ret = child_process_for_server(ip, port, uploadDirParh);
		if(ret < 0)
		{
			printf("err: func child_process_for_server() [%d], [%s]\n",__LINE__, __FILE__);
			return ret = -1;
		}
		
	}
	else if(pid > 0)			//父进程，初始化IPC的程序
	{			
		sleep(1);		//睡眠1秒钟， 保证子进程先运行完毕
		ret = scan_open_msgque(key, fileDir, handle );		
		if(ret < 0)
		{
			printf("err: func scan_open_msgque() [%d], [%s]\n",__LINE__, __FILE__);
			return ret = -2;
		}	
	}	
	
	*srcPid = pid;
	g_childPID = pid;
	
	
	//捕捉 SIGINT信号后, 关闭子进程和消息队列,然后退出.
	ret = sigsetjmp(g_sigEnv, 1);
    if(ret > 0)
    {
//    	myprint("The sigsetjmp() 关闭子进程和消息队列,然后退出. -------- begin------ : childPID : %d, ret : %d", g_childPID, ret);
    	//1.关闭子进程
       ret = close_server(g_childPID);
       if(ret < 0)       
       		myprint("Err : func close_server() %s", strerror(errno));
       else
       		myprint("OK child process closed!!!");
       
        //2.关闭消息队列  
		ret = scan_close_msgque(g_handle);
		if(ret < 0)
			myprint("Err : func scan_close_msgque() %s", strerror(errno));  
		else
			myprint("OK message queue closed!!!");
								
		exit(0);
		//3.调回主程序,关闭
		//siglongjmp(g_sigEnvEnd, 10);					
    }

	
	return ret;
}


//子进程执行逻辑，
int child_process_for_server(char *ip, int port, char *uploadDirParh)
{
	int ret = 0;
	
	//int init_server(char *ip, int port);  服务器初始化
	ret = init_server(ip, port, uploadDirParh);
	if(ret < 0)
	{
		printf("err: func pthread_create() [%d], [%s]\n",__LINE__, __FILE__);
		return ret;		
	}
	
	return ret;
}

int close_server(pid_t pid)
{
	int ret = 0;
	
	//1.杀死服务器进程
	if((ret = kill(pid, SIGKILL)) < 0)
	{
		printf("err: func kill() [%d], [%s]\n",__LINE__, __FILE__);
		return ret;		
	}
	
	//2.回收子进程， 负责服务器的进程
	wait(NULL);

	return ret;
}