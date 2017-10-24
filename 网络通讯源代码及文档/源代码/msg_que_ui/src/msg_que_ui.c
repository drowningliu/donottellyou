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
static bool 	g_flag = true;	//g_flagΪ��ʱ���Զ��ѹر�IPC,�����ٷ������ݰ��� ֱ���˳�����
								//Ϊ��,�򼺷������ر�IPC,��Ҫ�������ݰ�, ֪ͨ��һ������.

typedef struct msgbuf
{
    long mtype;		/* message type, must be > 0 */
    char mtext[MTEXTSIZE];	/* message data */
}myMsgBuf;



static int open_msg_queue(key_t keyserver, int *srcMsqid);
//static int recv_data(char *dataInfor, int  msqid, int type);


/*��ָ������Ϣ����, �����ڴ��
 *@param: key : ��Ϣ���еļ�ֵ
 *@retval: 0 �ɹ�, -1 ʧ��
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
	
	//1.����Ϣ����
	ret = open_msg_queue(key, queid);
	if(ret < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func open_msg_queue()");
		return ret;
	}

#if 0
	//���������ڴ�
	pool = mem_pool_init(500, MTEXTSIZE);
	if(pool == NULL)
	{
		socket_log(SocketLevel[4], ret, "err : func mem_pool_init()");
		return ret;
	}
#endif

	
	return ret;
}

/*�ر���Ϣ����, �����ڴ��
 *@retval: 0 �ɹ�, -1 ʧ��
*/
int ui_close_fun_mesque(int msqid)	
{
	int ret = 0, len = 0, sendType = 1;
	myMsgBuf msgbuf;

	
	if(g_flag)		//g_flagΪ��ʱ���Զ��ѹر�IPC,�����ٷ������ݰ��� ֱ���˳�����
	{

		//1.����պ������͹ر����ݰ���������Ϊ 1;�����ݰ���ʹ��;
		msgbuf.mtype = msqid;		//ע����������Ƿ����Լ��� ʹ�Լ��˳���
		len = sprintf(msgbuf.mtext, "%s", "1~");
		ret = msgsnd(msqid, &msgbuf, len, 0);
		if(ret < 0)
		{
			socket_log(SocketLevel[4], ret, "err : func msgsnd()");
			return ret;
		}

		msgbuf.mtype = sendType;	//���������֪ͨ��һ���̣� �����̽��ر�IPCͨ��
		len = sprintf(msgbuf.mtext, "%s", "1~");
		ret = msgsnd(msqid, &msgbuf, len, 0);
		if(ret < 0)
		{
			socket_log(SocketLevel[4], ret, "err : func msgsnd()");
			return ret;
		}

		sleep(1);		//�ȴ����н��յ����һ�����ݣ��˳�

	}
	
	//2.�ر���Ϣ����
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

/*��Ϣ���з�����Ϣ, ��Ϣ���е�����Ϊ ��Ϣ���ID
 *@param: dataInfor : ��Ҫ���͵���Ϣ
 *@retval: 0 �ɹ�, -1 ʧ��
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

	//1.��ȡ������
	memset(cmdBuf, 0, sizeof(cmdBuf));
	sscanf(dataInfor, "%[^~]", cmdBuf);
	cmd = atoi(cmdBuf);

	//2.�������ڴ��б����̵ĵ�ַת��Ϊ ƫ����, ���͹�ȥ
	if(cmd == 0x24)
	{
		tmp = strchr(dataInfor, '~');
		memcpy(tmpOffsetBuf, tmp+1, strlen(tmp)-2);		
		tmp = hex_conver_dec(tmpOffsetBuf) + NULL;
		offset = tmp - handle->share_pool;		//��ȡƫ����, �޷�������
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



//��ȡx��y�η�; x^y
unsigned int power(int x, int y)
{
    unsigned int result = 1;
    while (y--)
        result *= x;
    return result;
}
//���ַ�����16��������ת��Ϊ �޷�������
unsigned int hex_conver_dec(char *src)
{
    char *tmp = src;
    int len = 0;
    unsigned int hexad = 0;
    char sub = 0;

    while (1)
    {
        if (*tmp == '0')            //ȥ���ַ��� ��λ0
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


/*������Ϣ���е���Ϣ
 *@param: str : ���յ�����Ϣ, ���ݸ���������
 *@retval: 0 �ɹ�, -1 ʧ��, -2 The IPC Queue closed; (else, The cmd callBack func return value )
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

	
	//3.��ȡ��Ϣ�����е�������
	memset(cmdBuf, 0, sizeof(cmdBuf));
	sscanf(msgBuf.mtext, "%[^~]", cmdBuf);
	cmd = atoi(cmdBuf);

//	myprint("UI recvBuf : %s", msgBuf.mtext);
	//4.��һͨ�ŷ��ر���Ϣ����; (�����յ���������Ϊ 1ʱ������-2��֪ͨ�ϲ�Ӧ��)
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

/*������Ϣ���е���Ϣ
 *@param: str : ���յ�����Ϣ, ���ݸ���������
 *@retval: 0 �ɹ�, -1 ʧ��, -2 The IPC Queue closed; (else, The cmd callBack func return value )
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

	
	//3.��ȡ��Ϣ�����е�������
	memset(cmdBuf, 0, sizeof(cmdBuf));
	sscanf(msgBuf.mtext, "%[^~]", cmdBuf);
	cmd = atoi(cmdBuf);
	
	//4.��һͨ�ŷ��ر���Ϣ����; (�����յ���������Ϊ 1ʱ������-2��֪ͨ�ϲ�Ӧ��)
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
	
	//4.���������ֶ�Ӧ�� �ص�����, δ�ҵ�ʱ�˳�ѭ��
	tmpNode = find_cmd_func(handle, cmd);
	if(tmpNode == NULL)
	{
		socket_log(SocketLevel[4], -1, "err : func find_cmd_func()");
		return ret = -1;
	}
	
	//5.��ձ���
	memset(tmpNode->arg, 0, sizeof(tmpNode->arg));
	
	//6.��ȡ��Ϣ�����е���Ϣ������ arg;
	memcpy(tmpNode->arg, msgBuf.mtext, sizeof(tmpNode->arg));		//ȥ������~

	//7.ִ�лص�����	// arg ����Ϊ: cmd~content
	ret = tmpNode->func(tmpNode->arg);
	socket_log(SocketLevel[2], ret, " The cmd %d callBack func return : %d ",tmpNode->cmd, ret );

		
	return ret;
}
#if 0

/*�ͷ��ڴ�
 *@param: str : ������Ϣ���ڴ�
 *@retval: 0 �ɹ�, -1 ʧ��
*/
int ui_free_fun_mesque(uiIpcHandle *handle, char *str);


/*�ͷ��ڴ�
 *@param: str : ������Ϣ���ڴ�
 *@retval: 0 �ɹ�, -1 ʧ��
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

/*�������ֵ����Ϣ����
 *@param: keyserver : ��Ϣ���еļ�ֵ 
 *@param: srcMsqid : ��Ϣ���еľ��
 *@retval: 0 �ɹ�, -1 ʧ��
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
/*������Ϣ, ����Ϣ���������ڴ����
 *@param: dataInfor : ������Ϣ���ݵ��ڴ�
 *@param: g_msqid : ��Ϣ���еľ��
 *@param: type : ���յ���Ϣ����
 *@retval: 0 �ɹ�, -1 ʧ��
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

//��������
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




//��Ӧ�������ֺͻص����� ��������, Ĭ��ͷ�巨
int insert_node_include(uiIpcHandle *handle, int cmd, cmd_callback func)
{
	if (!handle || cmd < 0 || !func )
	{
		socket_log(SocketLevel[4], -1, "!handle || cmd < 0 || !func");
		return -1;
	}
		
	//1.�½ڵ㴴��
	uiIpcHandle *newNode = malloc(sizeof(uiIpcHandle));
	newNode->cmd = cmd;
	newNode->msqid = handle->msqid;
	newNode->shmid = handle->shmid;
	newNode->func = func;
	memset(newNode->arg, 0, sizeof(newNode->arg));	
	newNode->share_pool = handle->share_pool;
	newNode->next = NULL;

	
	//2.�ڵ����
	newNode->next = handle->next;
	handle->next = newNode;

	return 0;
}



//��������, 0 �ɹ�,  -1 ʧ��
int destroy_list(uiIpcHandle *list)
{
	int ret = 0;
	
	if(!list)
	{
		socket_log(SocketLevel[4], -1, "err : list == NULL");
		return ret = -1;
	}
	
	uiIpcHandle *tmp = NULL;

	//���������������
	while (list)
	{
		tmp = list->next;
		free(list);
		list = tmp;
	}
	printf("The destroy_list() end, [%d], [%s]\n", __LINE__, __FILE__);
	
	return 0;
}



//���Ҷ�Ӧ�������ֵĽڵ�, ������������ڵ�;
//�ҵ� ��Ӧ��������, ����ֵ ��NULL;  δ�ҵ� NULL;
uiIpcHandle	*find_cmd_func(uiIpcHandle *handle, int cmd)
{
	if ( cmd < 0 || !handle)
	{
		socket_log(SocketLevel[4], -1, "err : cmd < 0 || !handle");
		return NULL;
	}
	
	uiIpcHandle  *tmpNode = handle->next;

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

