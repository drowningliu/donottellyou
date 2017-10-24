#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>

#include "socklog.h"
#include "msg_que_ui.h"

 

#define 		MTEXTSIZE 512
static bool 	g_flag = true;	//g_flag为假时，对端已关闭IPC,不能再发送数据包， 直接退出即可
								//为真,则己方主动关闭IPC,需要发送数据包, 通知另一个进程.

typedef struct msgbuf
{
    long mtype;		/* message type, must be > 0 */
    char mtext[MTEXTSIZE];	/* message data */
}myMsgBuf;



static int open_msg_queue(key_t keyserver, int *srcMsqid);
//static int recv_data(char *dataInfor, int  msqid, int type);


/*打开指定的消息队列, 创建内存池
 *@param: key : 消息队列的键值
 *@retval: 0 成功, -1 失败
*/
int ui_open_fun_mesque(key_t key, int *queid)
{
	int ret = 0;
//	mem_pool_t *pool = NULL;

	if(!queid )
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "err : queid : %p", queid);
		return ret;
	}
	
	//1.打开消息队列
	ret = open_msg_queue(key, queid);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func open_msg_queue()");
		return ret;
	}

#if 0
	//创建共享内存
	pool = mem_pool_init(500, MTEXTSIZE);
	if(pool == NULL)
	{
		socket_log(SocketLevel[4], ret, "err : func mem_pool_init()");
		return ret;
	}
#endif

	
	return ret;
}

/*关闭消息队列, 销毁内存池
 *@retval: 0 成功, -1 失败
*/
int ui_close_fun_mesque(int msqid)	
{
	int ret = 0, len = 0, sendType = 1;
	myMsgBuf msgbuf;

	
	if(g_flag)		//g_flag为假时，对端已关闭IPC,不能再发送数据包， 直接退出即可
	{

		//1.向接收函数发送关闭数据包，命令字为 1;该数据包不使用;
		msgbuf.mtype = msqid;		//注意这包数据是发给自己， 使自己退出；
		len = sprintf(msgbuf.mtext, "%s", "1~");
		ret = msgsnd(msqid, &msgbuf, len, 0);
		if(ret < 0)
		{
			socket_log(SocketLevel[4], ret, "err : func msgsnd()");
			return ret;
		}

		msgbuf.mtype = sendType;	//这包数据是通知另一进程， 本进程将关闭IPC通信
		len = sprintf(msgbuf.mtext, "%s", "1~");
		ret = msgsnd(msqid, &msgbuf, len, 0);
		if(ret < 0)
		{
			socket_log(SocketLevel[4], ret, "err : func msgsnd()");
			return ret;
		}

		sleep(1);		//等待队列接收到最后一包数据，退出

	}
	
	//2.关闭消息队列
	ret = msgctl(msqid, IPC_RMID, NULL);
	if(ret < 0)
	{
		if(errno == EIDRM)
		{
			socket_log(SocketLevel[2], 1, "INFO : The message queue is errno == EIDRM");
			return 0;
		}
		else if(errno == EPERM)
		{
			socket_log(SocketLevel[4], ret, "err : func msgctl() errno == EPERM");
			return ret;
		}
		else if(errno == EINVAL)
		{
			socket_log(SocketLevel[2], 1, "INFO : The message queue is errno == EINVAL");
			return 0;
		}
		else
		{
			socket_log(SocketLevel[4], ret, "err : func msgctl()");
			return ret;
		}

	}

	
	return ret;
}

/*消息队列发送消息, 消息队列的类型为 消息句柄ID
 *@param: dataInfor : 将要发送的消息
 *@retval: 0 成功, -1 失败
*/
int ui_send_fun_mesque_include(uiIpcHandle *handle, const char *dataInfor)	
{
	int ret = 0,  len = 0;
	myMsgBuf buf;
	int sendType = 1;
	char cmdBuf[10] = { 0 };
	int cmd = 0;
	char *tmp = NULL;
	char tmpOffsetBuf[64] = { 0 };
	unsigned int offset = 0;
	
	if(!dataInfor || !handle)
	{
		socket_log(SocketLevel[4], -1, "err : dataInfor : %p, handle : %p", dataInfor, handle);
		return ret = -1;
	}

	//1.获取命令字
	memset(cmdBuf, 0, sizeof(cmdBuf));
	sscanf(dataInfor, "%[^~]", cmdBuf);
	cmd = atoi(cmdBuf);

	//2.将共享内存中本进程的地址转换为 偏移量, 发送过去
	if(cmd == 0x24)
	{
		tmp = strchr(dataInfor, '~');
		memcpy(tmpOffsetBuf, tmp+1, strlen(tmp)-2);		
		tmp = hex_conver_dec(tmpOffsetBuf) + NULL;
		offset = tmp - handle->share_pool;		//获取偏移量, 无符号整形
		sprintf(buf.mtext, "%d~%u~", 0x24, offset);
		buf.mtype = sendType;	
		len = strlen(buf.mtext);
	}
	else
	{
		buf.mtype = sendType;
		len = strlen(dataInfor);
		memcpy(buf.mtext, dataInfor, len);
	}


	while(1)
	{		
		ret = msgsnd(handle->msqid, &buf, len, 0);
		if(ret < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				socket_log(SocketLevel[4], ret, "err : func msgsnd()");
				return ret;
			}
		}
		break;
	}
	//myprint("********   UI sendBuf : %s  *******", buf.mtext);
	return ret;
}



//获取x的y次方; x^y
unsigned int power(int x, int y)
{
    unsigned int result = 1;
    while (y--)
        result *= x;
    return result;
}
//将字符串的16进制内容转换为 无符号整形
unsigned int hex_conver_dec(char *src)
{
    char *tmp = src;
    int len = 0;
    unsigned int hexad = 0;
    char sub = 0;

    while (1)
    {
        if (*tmp == '0')            //去除字符串 首位0
        {
            tmp++;
            continue;
        }           
        else if (*tmp == 'X')
        {
            tmp++;
            continue;
        }
        else if (*tmp == 'x')
        {
            tmp++;
            continue;
        }
        else
            break;

    }

    len = strlen(tmp);
    for (; len > 0; len--)
    {        
        sub = toupper(*tmp++) - '0';
        if (sub > 9)
            sub -= 7;
        hexad += sub * power(16, len - 1);      
    }

    return hexad;
}


/*接收消息队列的消息
 *@param: str : 接收到的消息, 传递给主调函数
 *@retval: 0 成功, -1 失败, -2 The IPC Queue closed; (else, The cmd callBack func return value )
*/
int ui_recv_fun_mesque_include(uiIpcHandle *handle, char *str)	
{
	int ret = 0, cmd = -1, len = 0;
	char cmdBuf[4] = { 0 };
	myMsgBuf msgBuf;
	char *shmory_addr = NULL, *tmp = NULL;
	char tmpOffsetBuf[64] = { 0 };
	unsigned int offset = 0;
	
	if(!handle || !str )
	{
		socket_log(SocketLevel[4], -1, "err : handle : %p, str : %p", handle, str);
		return ret = -1;
	}

	//1. recv The IPC Queue message
	memset(&msgBuf, 0, sizeof(myMsgBuf));
	while(1)
	{
		len = msgrcv(handle->msqid, &msgBuf, MTEXTSIZE, handle->msqid, 0);
		if(len == -1)
		{
			if(errno == EINTR)				
				continue;		
			else	
			{
				socket_log(SocketLevel[4], len, "err : func msgrcv()");
				return len;
			}
		}
		break;
	}

	
	//3.获取消息队列中的命令字
	memset(cmdBuf, 0, sizeof(cmdBuf));
	sscanf(msgBuf.mtext, "%[^~]", cmdBuf);
	cmd = atoi(cmdBuf);

//	myprint("UI recvBuf : %s", msgBuf.mtext);
	//4.另一通信方关闭消息队列; (当接收到的命令字为 1时，返回-2，通知上层应用)
	if(cmd == 0xA1)
	{
		tmp = strchr(msgBuf.mtext, '~');
		memcpy(tmpOffsetBuf, tmp+1, strlen(tmp)-2);
		offset = hex_conver_dec(tmpOffsetBuf);
		shmory_addr = handle->share_pool + offset;		
		sprintf(str, "%d~%p~", 0xA1, shmory_addr );
	}
	else if(cmd == 1)
	{
		g_flag = false;
		printf("The IPC Queue will close cmd = %d\n", cmd);	
		strcpy(str, msgBuf.mtext);
		return ret = -2;
	}	
	else
	{
		strcpy(str, msgBuf.mtext);
	}
	
	return ret;
}

/*接收消息队列的消息
 *@param: str : 接收到的消息, 传递给主调函数
 *@retval: 0 成功, -1 失败, -2 The IPC Queue closed; (else, The cmd callBack func return value )
*/
int ui_recv_fun_mesque_insert_func(uiIpcHandle *handle)	
{
	int ret = 0, cmd = -1, len = 0;
	char cmdBuf[4] = { 0 };
	uiIpcHandle *tmpNode = NULL;
	myMsgBuf msgBuf;
	char *shmory_addr = NULL, *tmp = NULL;
	char tmpOffsetBuf[64] = { 0 };
	unsigned int offset = 0;
	
	if(!handle )
	{
		socket_log(SocketLevel[4], -1, "err : handle : %p", handle);
		return ret = -1;
	}

	//1. recv The IPC Queue message
	memset(&msgBuf, 0, sizeof(myMsgBuf));
	while(1)
	{
		len = msgrcv(handle->msqid, &msgBuf, MTEXTSIZE, handle->msqid, 0);
		if(len == -1)
		{
			if(errno == EINTR)				
				continue;		
			else	
			{
				socket_log(SocketLevel[4], len, "err : func msgrcv()");
				return len;
			}
		}
		break;
	}

	
	//3.获取消息队列中的命令字
	memset(cmdBuf, 0, sizeof(cmdBuf));
	sscanf(msgBuf.mtext, "%[^~]", cmdBuf);
	cmd = atoi(cmdBuf);
	
	//4.另一通信方关闭消息队列; (当接收到的命令字为 1时，返回-2，通知上层应用)
	if(cmd == 0xA1)
	{
		tmp = strchr(msgBuf.mtext, '~');
		memcpy(tmpOffsetBuf, tmp+1, strlen(tmp)-2);
		offset = hex_conver_dec(tmpOffsetBuf);;
		shmory_addr = handle->share_pool + offset;
		memset(msgBuf.mtext, 0, MTEXTSIZE);
		sprintf(msgBuf.mtext, "%d~%p~", 0xA1, shmory_addr );
	}
	else if(cmd == 1)
	{
		g_flag = false;
		printf("The IPC Queue will close cmd = %d\n", cmd);		
		return ret = -2;
	}
	
	//4.查找命令字对应的 回调函数, 未找到时退出循环
	tmpNode = find_cmd_func(handle, cmd);
	if(tmpNode == NULL)
	{
		socket_log(SocketLevel[4], -1, "err : func find_cmd_func()");
		return ret = -1;
	}
	
	//5.清空变量
	memset(tmpNode->arg, 0, sizeof(tmpNode->arg));
	
	//6.提取消息队列中的消息至参数 arg;
	memcpy(tmpNode->arg, msgBuf.mtext, sizeof(tmpNode->arg));		//去除两个~

	//7.执行回调函数	// arg 内容为: cmd~content
	ret = tmpNode->func(tmpNode->arg);
	socket_log(SocketLevel[2], ret, " The cmd %d callBack func return : %d ",tmpNode->cmd, ret );

		
	return ret;
}
#if 0

/*释放内存
 *@param: str : 接收消息的内存
 *@retval: 0 成功, -1 失败
*/
int ui_free_fun_mesque(uiIpcHandle *handle, char *str);


/*释放内存
 *@param: str : 接收消息的内存
 *@retval: 0 成功, -1 失败
*/
int ui_free_fun_mesque(uiIpcHandle *handle, char *str)
{
	int ret = 0;

	if(!str || !handle)
	{
		socket_log(SocketLevel[4], -1, "err : handle : %p, str : %p", handle, str);
		return ret = -1;
	}
	
	ret = mem_pool_free(handle->pool, str);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func mem_pool_free()");
		return ret ;
	}
	
	return ret;
}
#endif

/*打开输入键值的消息队列
 *@param: keyserver : 消息队列的键值 
 *@param: srcMsqid : 消息队列的句柄
 *@retval: 0 成功, -1 失败
*/
int open_msg_queue(key_t keyserver, int *srcMsqid)
{
	int ret = 0, msqid = 0;
	
	msqid = msgget(keyserver, 0666 | IPC_CREAT | IPC_EXCL);
	if(msqid == -1)
	{
		if(errno == EEXIST)
		{
			msqid = msgget(keyserver, 0666 );
			if(msqid < 0)
			{
				ret = -1;
				socket_log(SocketLevel[4], ret, "err : func msgget()");
				return ret;
			}
		}
		else
		{
			ret = -1;
			socket_log(SocketLevel[4], ret, "err : func msgget()");
			return ret;
		}			
	}

	*srcMsqid = msqid;

	return ret;
}


#if 0
/*接收消息, 将信息拷贝至该内存块中
 *@param: dataInfor : 拷贝消息内容的内存
 *@param: g_msqid : 消息队列的句柄
 *@param: type : 接收的消息类型
 *@retval: 0 成功, -1 失败
*/
int recv_data(char *dataInfor, int  msqid, int type)
{
	int ret = 0, len = 0;

	
	myMsgBuf msgBuf;
	memset(&msgBuf, 0, sizeof(myMsgBuf));
	while(1)
	{
		len = msgrcv(msqid, &msgBuf, MTEXTSIZE, type, 0);
		if(len == -1)
		{
			if(errno == EINTR)	
			{
				continue;
			}
			else	
			{
				socket_log(SocketLevel[4], len, "err : func msgrcv()");
				return len;
			}
		}
		break;
	}
	memcpy(dataInfor, msgBuf.mtext, len);
	
	return ret;
}
#endif

//创建链表
uiIpcHandle* create_list()
{
	uiIpcHandle *head = malloc(sizeof(uiIpcHandle));
	if (!head)
	{
		printf("err : func create_list() malloc()\n");
		return NULL;
	}
	
	head->cmd = -1;
	head->msqid = 0;
	head->shmid = 0;
	head->func = NULL;
	memset(head->arg, 0, sizeof(head->arg));
	head->share_pool = NULL;
	head->next = NULL;

	return head;
}




//对应的命令字和回调函数 插入链表, 默认头插法
int insert_node_include(uiIpcHandle *handle, int cmd, cmd_callback func)
{
	if (!handle || cmd < 0 || !func )
	{
		socket_log(SocketLevel[4], -1, "!handle || cmd < 0 || !func");
		return -1;
	}
		
	//1.新节点创建
	uiIpcHandle *newNode = malloc(sizeof(uiIpcHandle));
	newNode->cmd = cmd;
	newNode->msqid = handle->msqid;
	newNode->shmid = handle->shmid;
	newNode->func = func;
	memset(newNode->arg, 0, sizeof(newNode->arg));	
	newNode->share_pool = handle->share_pool;
	newNode->next = NULL;

	
	//2.节点插入
	newNode->next = handle->next;
	handle->next = newNode;

	return 0;
}



//销毁链表, 0 成功,  -1 失败
int destroy_list(uiIpcHandle *list)
{
	int ret = 0;
	
	if(!list)
	{
		socket_log(SocketLevel[4], -1, "err : list == NULL");
		return ret = -1;
	}
	
	uiIpcHandle *tmp = NULL;

	//遍历链表进行销毁
	while (list)
	{
		tmp = list->next;
		free(list);
		list = tmp;
	}
	printf("The destroy_list() end, [%d], [%s]\n", __LINE__, __FILE__);
	
	return 0;
}



//查找对应的命令字的节点, 返回它的链表节点;
//找到 对应的命令字, 返回值 非NULL;  未找到 NULL;
uiIpcHandle	*find_cmd_func(uiIpcHandle *handle, int cmd)
{
	if ( cmd < 0 || !handle)
	{
		socket_log(SocketLevel[4], -1, "err : cmd < 0 || !handle");
		return NULL;
	}
	
	uiIpcHandle  *tmpNode = handle->next;

	//遍历链表查找
	while (tmpNode)
	{
		if (tmpNode->cmd == cmd)
		{
			break;
		}
		tmpNode = tmpNode->next;
	}

	if (tmpNode)
	{
		return tmpNode;
	}
	
		
	return NULL;
}

#if 0

/*insert The cmd And cmd_call_Back in the list
*@param :  handle		for The sigal list 
*@param :  cmd			insert The list Node cmd
*@param :  func 		The call func for cmd 
*@retval:  success 0, fail -1;
*/
int insert_node(int cmd, cmd_callback func);


int insert_node( int cmd, cmd_callback func)
{
	int ret = 0;
	
	if((ret = insert_node_include(g_srcHandle, cmd, func)) < 0)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Err: func insert_node_include() ");			
		return ret;
	}
	
	return ret;

}
#endif

