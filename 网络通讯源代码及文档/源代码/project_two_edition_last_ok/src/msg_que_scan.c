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

//��Ϣ���нڵ�
typedef struct _msgbuf
{
    long mtype;		/* message type, must be > 0 */
    char mtext[MTEXTSIZE];	/* message data */
}myMsgBuf;


/*ͨ����Ϣ���� ������Ϣ
 *@param: dataInfor : Ҫ���͵��ļ���
 *@param: handle : ������
 *retval: 0 �ɹ�, -1 ʧ��
*/
int  scan_send_msgque(char *dataInfor, listNode *handle);

/*�� ��Ϣ����
 *@param: keyserver : ��Ϣ���еļ�ֵ
 *@param: srcMsqid ; ����Ϣ���еľ��
 *retval: 0 �ɹ�, -1 ʧ��
*/
static int open_msg_queue(key_t keyserver, int *srcMsqid);

//��������
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


//��Ӧ�������ֺͻص����� ��������, Ĭ��ͷ�巨
int insert_node(listNode *handle, int cmd, recv_data_callback func, void *arg, int argLen)
{
	if (!handle || cmd < 0 || !func )
	{
		socket_log(SocketLevel[4], -1, "!handle || cmd < 0 || !func");
		return -1;
	}

	//�½ڵ㴴��
	listNode *newNode = malloc(sizeof(listNode));
	newNode->cmd = cmd;
	newNode->func = func;
	newNode->arg = arg;
	newNode->argLen = argLen;
	newNode->msqid = handle->msqid;
	strcpy(newNode->fileDir, handle->fileDir);
	newNode->next = NULL;

	//�ڵ����
	newNode->next = handle->next;
	handle->next = newNode;

	return 0;
}

//��������, 0 �ɹ�,  -1 ʧ��
int destroy_list(listNode *list)
{
	int ret = 0;
	
	if(!list)
	{
		socket_log(SocketLevel[4], -1, "err : list == NULL");
		return ret = -1;
	}
	
	listNode *tmp = NULL;

	//���������������
	while (list)
	{
		tmp = list->next;
		free(list);
		list = tmp;
	}

	return 0;
}


//���Ҷ�Ӧ�������ֵĽڵ�, ������������ڵ�;
//�ҵ� ��Ӧ��������, ����ֵ ��NULL;  δ�ҵ� NULL;
listNode* find_cmd_func(listNode *handle, int cmd)
{
	if ( cmd < 0 || !handle)
	{
		socket_log(SocketLevel[4], -1, "err : cmd < 0 || !handle");
		return NULL;
	}
	
	listNode  *tmpNode = handle->next;

	//�����������
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

//����Ϣ����, ��������
int  scan_open_msgque(key_t key, char *fileDir, listNode **handle)
{
	int ret = 0;
	int msqid = 0;		//��Ϣ���еľ��
	listNode *tmpList = NULL;


	//1.����Ϣ����
	ret = open_msg_queue(key, &msqid);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func open_msg_queue()");
		return ret;
	}

	//2.��������
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

//�ر���Ϣ����,��������
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
	
	//1.����Ϣ, ʹ���պ����˳�ѭ��;(������Ϊ 0����Ϣ)
	msgbuf.mtype = recvType;
	len = sprintf(msgbuf.mtext, "%s", "0~");
	ret = msgsnd(handle->msqid, &msgbuf, len, 0);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func msgsnd()");
		return ret;
	}
	sleep(1);
	
	//2.�ر���Ϣ����
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
	
	//3.���ȫ�ֱ���
	ret = destroy_list(handle);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func destroy_list()");
		return ret;
	}

	return ret;
}


//��Ϣ���з�����Ϣ : ������ͼƬ·��
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

//��Ϣ���н�����Ϣ : ��Ӧ�������� ��Ӧ��Ӧ�Ļص�����
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
		//1.��ȡ���յ���Ϣ��������
		memset(&msgBuf, 0, sizeof(myMsgBuf));	
		len = msgrcv(handle->msqid, &msgBuf, MTEXTSIZE, recvType, 0);
		if(len == -1)
		{
			perror("msgrcv");
			socket_log(SocketLevel[4], len, "err : func msgrcv()");
			return len;
		}

		printf("recv news : %s\n", msgBuf.mtext);
		
		//2.��ȡ��Ϣ�����е�������
		memset(cmdBuf, 0, sizeof(cmdBuf));
		sscanf(msgBuf.mtext, "%[^~]", cmdBuf);
		cmd = atoi(cmdBuf);

		//3.�����˳�ѭ��,�ر���Ϣ����; (�����յ���������Ϊ 0ʱ, ��ʶ���������ر���Ϣ����,����ѭ��)
		if(cmd == 0)
		{
			printf("will close msg queue, cmd = %d\n", cmd);
			break;
		}
		
		//4.���������ֶ�Ӧ�� �ص�����, δ�ҵ�ʱ�˳�ѭ��
		tmpNode = find_cmd_func(handle, cmd);
		if(tmpNode == NULL)
		{
			socket_log(SocketLevel[4], -1, "err : func find_cmd_func()");
			return ret = -1;
		}
		
		//5.��ȡ��Ϣ�����е���Ϣ������ arg;
		tmp = strchr(msgBuf.mtext, '~');
		memcpy(tmpNode->arg, tmp + 1, strlen(tmp) - 2);		//ȥ������~

		//6.ִ�лص�����
		tmpNode->func(tmpNode->arg);

		//7.��ձ���
		memset(tmpNode->arg, 0, tmpNode->argLen);
		
	}

	return ret;
}

//�����ļ�, д���ļ�����
int  create_file_send_filepath(listNode *handle, void *addr, void *picData, unsigned int picLen)
{
	int ret = 0, writeLen = 0;
	FILE *fp = NULL;
	char filePath[256] = { 0 };

	//1.�������ߴ��ļ�
	sprintf(filePath, "%s/%s",handle->fileDir, (char *)addr);
	fp = fopen(filePath, "a+");
	if(fp == NULL)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "err : func fopen()");
		return ret;
	}

	//2.д���ļ�����
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
	
	//3.�ֶ�ˢ��,���ر��ļ�

	fclose(fp);

	//4.�����ļ�·��
	ret = scan_send_msgque(addr, handle);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func scan_send_msgque()");
		fclose(fp);
		return ret;
	}	

	
	return ret;
}

//��ָ����ֵ����Ϣ����;  0 �ɹ�; -1 ʧ��
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








