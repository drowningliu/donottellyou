#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "socklog.h"
#include "msg_que_scan.h"

#define MTEXTSIZE 512

//消息队列节点
typedef struct _msgbuf
{
    long mtype;		/* message type, must be > 0 */
    char mtext[MTEXTSIZE];	/* message data */
}myMsgBuf;


/*通过消息队列 发送消息
 *@param: dataInfor : 要发送的文件名
 *@param: handle : 链表句柄
 *retval: 0 成功, -1 失败
*/
int  scan_send_msgque(char *dataInfor, listNode *handle);

/*打开 消息队列
 *@param: keyserver : 消息队列的键值
 *@param: srcMsqid ; 打开消息队列的句柄
 *retval: 0 成功, -1 失败
*/
static int open_msg_queue(key_t keyserver, int *srcMsqid);

//创建链表
listNode* create_list()
{
	listNode *head = malloc(sizeof(listNode));
	if (!head)
	{
		printf("err : func create_list() malloc()\n");
		return NULL;
	}
	
	head->cmd = -1;
	head->func = NULL;
	head->arg = NULL;
	head->argLen = 0;
	head->msqid = -1;
	memset(head->fileDir, 0, sizeof(head->fileDir));
	head->next = NULL;

	return head;
}


//对应的命令字和回调函数 插入链表, 默认头插法
int insert_node(listNode *handle, int cmd, recv_data_callback func, void *arg, int argLen)
{
	if (!handle || cmd < 0 || !func )
	{
		socket_log(SocketLevel[4], -1, "!handle || cmd < 0 || !func");
		return -1;
	}

	//新节点创建
	listNode *newNode = malloc(sizeof(listNode));
	newNode->cmd = cmd;
	newNode->func = func;
	newNode->arg = arg;
	newNode->argLen = argLen;
	newNode->msqid = handle->msqid;
	strcpy(newNode->fileDir, handle->fileDir);
	newNode->next = NULL;

	//节点插入
	newNode->next = handle->next;
	handle->next = newNode;

	return 0;
}

//销毁链表, 0 成功,  -1 失败
int destroy_list(listNode *list)
{
	int ret = 0;
	
	if(!list)
	{
		socket_log(SocketLevel[4], -1, "err : list == NULL");
		return ret = -1;
	}
	
	listNode *tmp = NULL;

	//遍历链表进行销毁
	while (list)
	{
		tmp = list->next;
		free(list);
		list = tmp;
	}

	return 0;
}


//查找对应的命令字的节点, 返回它的链表节点;
//找到 对应的命令字, 返回值 非NULL;  未找到 NULL;
listNode* find_cmd_func(listNode *handle, int cmd)
{
	if ( cmd < 0 || !handle)
	{
		socket_log(SocketLevel[4], -1, "err : cmd < 0 || !handle");
		return NULL;
	}
	
	listNode  *tmpNode = handle->next;

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

//打开消息队列, 创建链表
int  scan_open_msgque(key_t key, char *fileDir, listNode **handle)
{
	int ret = 0;
	int msqid = 0;		//消息队列的句柄
	listNode *tmpList = NULL;


	//1.打开消息队列
	ret = open_msg_queue(key, &msqid);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func open_msg_queue()");
		return ret;
	}

	//2.创建链表
	tmpList = create_list();
	if(!tmpList)
	{	
		socket_log(SocketLevel[4], -1, "err : func create_list()");
		return ret = -1;
	}
	
	tmpList->msqid = msqid;
	memcpy(tmpList->fileDir, fileDir, sizeof(tmpList->fileDir));

	*handle = tmpList;
	
	return ret;
}

//关闭消息队列,销毁链表
int  scan_close_msgque(listNode *handle)
{
	int ret = 0, len = 0;
	int recvType = 1;	
	myMsgBuf	msgbuf;

	if(!handle)
	{
		socket_log(SocketLevel[4], ret, "err : handle == NULL");
		return ret = -1;	
	}
	
	//1.发消息, 使接收函数退出循环;(命令字为 0的消息)
	msgbuf.mtype = recvType;
	len = sprintf(msgbuf.mtext, "%s", "0~");
	ret = msgsnd(handle->msqid, &msgbuf, len, 0);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func msgsnd()");
		return ret;
	}
	sleep(1);
	
	//2.关闭消息队列
	ret = msgctl(handle->msqid, IPC_RMID, NULL);
	if(ret < 0)
	{
		if(errno == EIDRM)
		{
			socket_log(SocketLevel[2], 0, "INFO : The message queue id has removed");
			return 0;
		}
		else if(errno == EPERM)
		{
			socket_log(SocketLevel[4], ret, "err : func msgctl() errno == EPERM");
			return ret;
		}
		else if(errno == EINVAL)
		{
			socket_log(SocketLevel[2], 0, "INFO : The message queue id has removed");
			return 0;
		}
		else
		{
			socket_log(SocketLevel[4], ret, "err : func msgctl()");
			return ret;
		}
	}
	
	//3.清空全局变量
	ret = destroy_list(handle);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func destroy_list()");
		return ret;
	}

	return ret;
}


//消息队列发送消息 : 完整的图片路径
int  scan_send_msgque(char *datainfo, listNode *handle)
{
	int ret = 0, len = 0;
	myMsgBuf buf;
	
	if(datainfo == NULL)
	{
		socket_log(SocketLevel[4], -1, "err : datainfo == NULL");
		return ret   = -1;
	}
	
	buf.mtype = handle->msqid;
	len = sprintf(buf.mtext, "%d~%s/%s~", 160, handle->fileDir, datainfo);	
	
	ret = msgsnd(handle->msqid, &buf, len, 0);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func msgsnd()");
		return ret;
	}

	return ret;
}

//消息队列接收消息 : 对应的命令字 对应相应的回调函数
int  scan_recv_msgque(listNode *handle)
{
	int ret = 0, len = 0, cmd = 0;
	int recvType = 1;
	myMsgBuf	msgBuf;
	char *tmp = NULL;
	char cmdBuf[4] = { 0 };
	listNode *tmpNode = NULL;
		
	if(!handle)
	{
		socket_log(SocketLevel[4], -1, "err : handle == NULL ");
		return ret = -1;
	}

	while(1)
	{
		//1.获取接收到信息的命令字
		memset(&msgBuf, 0, sizeof(myMsgBuf));	
		len = msgrcv(handle->msqid, &msgBuf, MTEXTSIZE, recvType, 0);
		if(len == -1)
		{
			perror("msgrcv");
			socket_log(SocketLevel[4], len, "err : func msgrcv()");
			return len;
		}

		printf("recv news : %s\n", msgBuf.mtext);
		
		//2.获取消息队列中的命令字
		memset(cmdBuf, 0, sizeof(cmdBuf));
		sscanf(msgBuf.mtext, "%[^~]", cmdBuf);
		cmd = atoi(cmdBuf);

		//3.主动退出循环,关闭消息队列; (当接收到的命令字为 0时, 标识进程主动关闭消息队列,跳出循环)
		if(cmd == 0)
		{
			printf("will close msg queue, cmd = %d\n", cmd);
			break;
		}
		
		//4.查找命令字对应的 回调函数, 未找到时退出循环
		tmpNode = find_cmd_func(handle, cmd);
		if(tmpNode == NULL)
		{
			socket_log(SocketLevel[4], -1, "err : func find_cmd_func()");
			return ret = -1;
		}
		
		//5.提取消息队列中的消息至参数 arg;
		tmp = strchr(msgBuf.mtext, '~');
		memcpy(tmpNode->arg, tmp + 1, strlen(tmp) - 2);		//去除两个~

		//6.执行回调函数
		tmpNode->func(tmpNode->arg);

		//7.清空变量
		memset(tmpNode->arg, 0, tmpNode->argLen);
		
	}

	return ret;
}

//创建文件, 写入文件内容
int  create_file_send_filepath(listNode *handle, void *addr, void *picData, unsigned int picLen)
{
	int ret = 0, writeLen = 0;
	FILE *fp = NULL;
	char filePath[256] = { 0 };

	//1.创建或者打开文件
	sprintf(filePath, "%s/%s",handle->fileDir, (char *)addr);
	fp = fopen(filePath, "a+");
	if(fp == NULL)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "err : func fopen()");
		return ret;
	}

	//2.写入文件内容
	while(writeLen < picLen)
	{		
		writeLen = fwrite(picData + writeLen, 1, picLen - writeLen, fp);
		if(writeLen < 0)
		{
			socket_log(SocketLevel[4], ret, "err : func fopen()");
			fclose(fp);
			return ret;
		}
		
		ret = fflush(fp);
		if(ret < 0)
		{
			socket_log(SocketLevel[4], ret, "err : func fflush()");
			fclose(fp);
			return ret;
		}
		writeLen += writeLen;
	}
	
	//3.手动刷新,并关闭文件

	fclose(fp);

	//4.发送文件路径
	ret = scan_send_msgque(addr, handle);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func scan_send_msgque()");
		fclose(fp);
		return ret;
	}	

	
	return ret;
}

//打开指定键值的消息队列;  0 成功; -1 失败
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








