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
#include <stdbool.h>
#include <ctype.h>

#include "socklog.h"
#include "msg_que_scan.h"

#define MTEXTSIZE 512

bool g_flag = true;		//false: ��ʶ�Զ��ѹر�IPC����, true:��ʶ���˹ر�IPC����


//��Ϣ���нڵ�
typedef struct _msgbuf
{
    long mtype;		/* message type, must be > 0 */
    char mtext[MTEXTSIZE];	/* message data */
}myMsgBuf;


/*�� ��Ϣ����
 *@param: keyserver : ��Ϣ���еļ�ֵ
 *@param: srcMsqid ; ����Ϣ���еľ��
 *retval: 0 �ɹ�, -1 ʧ��
*/
static int open_msg_queue(key_t keyserver, int *srcMsqid);


char *unsigned_int_to_hex(unsigned int number, char *hex_str )
{
	char b[17] = { "0123456789ABCDEF" };
	int c[8], i = 0, j = 0, d, base = 16;
					 
	do{
		c[i] = number % base;
		i++;
		number = number / base;
	} while (number != 0);

	hex_str[j++] = '0';
	hex_str[j++] = 'X';

	for (--i; i >= 0; --i, j++)
	{
		d = c[i];		
		hex_str[j] = b[d];
	}
	hex_str[j] = '\0';

	return hex_str;
}


//��uint32 ����ת��Ϊ C�ַ���; Ĭ�ϲ���ʮ����
char* uitoa(unsigned int n, char *s)
{
    int i, j;
    i = 0;
	
	if(s == NULL)		return NULL;
	
    char buf[20];
    memset(buf, 0, sizeof(buf));
    do{
        buf[i++] = n % 10 + '0';//ȡ��һ������
    } while ((n /= 10)>0);//ɾ��������   
    i -= 1;
    for (j = 0; i >= 0; j++, i--)//���ɵ�����������ģ�����Ҫ�������
        s[j] = buf[i];
    s[j] = '\0';  
	
    return s;
}


unsigned int atoui(const char *str)
{
	unsigned int result = 0;

	while (*str)
	{
		result = result * 10 + *str - '0';
		str++;
	}

	return result;
}



//��������
listNode* create_list()
{
	listNode *head = NULL;

	if((head = malloc(sizeof(listNode))) == NULL)
	{
		printf("err : func create_list() malloc()\n");
		return NULL;
	}
	
	head->cmd = -1;
	head->func = NULL;
	memset(head->arg, 0, sizeof(head->arg));
	head->msqid = -1;
	memset(head->commomFileDir, 0, sizeof(head->commomFileDir));
	memset(head->TemplateFileDir, 0, sizeof(head->TemplateFileDir));
	head->next = NULL;
	head->shmPool = NULL;

	return head;
}


//��Ӧ�������ֺͻص����� ��������, Ĭ��ͷ�巨
int insert_node(listNode *handle, int cmd, recv_data_callback func)
{
	if (!handle || cmd < 0 || !func )
	{
		socket_log(SocketLevel[4], -1, "!handle || cmd < 0 || !func");
		return -1;
	}
		
	//1.�½ڵ㴴��
	listNode *newNode = malloc(sizeof(listNode));
	newNode->cmd = cmd;
	newNode->func = func;
	memset(newNode->arg, 0, sizeof(newNode->arg));
	newNode->msqid = handle->msqid;
	strcpy(newNode->commomFileDir, handle->commomFileDir);
	strcpy(newNode->TemplateFileDir, handle->TemplateFileDir);
	newNode->next = NULL;
	newNode->shmPool = handle->shmPool;

	
	//2.�ڵ����
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
		socket_log(SocketLevel[4], -1, "err : cmd : %d, handle : %p",cmd, handle);
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
int  scan_open_msgque(key_t key, char *commomFileDir, char *templateFileDir, listNode **handle)
{
	int ret = 0;
	int msqid = 0;		//��Ϣ���еľ��
	listNode *tmpList = NULL;


	//1.����Ϣ����
	if((ret = open_msg_queue(key, &msqid)) < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func open_msg_queue()");
		return ret;
	}

	//2.��������
	if((tmpList = create_list()) == NULL)
	{	
		socket_log(SocketLevel[4], -1, "err : func create_list()");
		return ret = -1;
	}
	
	tmpList->msqid = msqid;	
	memcpy(tmpList->commomFileDir, commomFileDir, MY_MIN(strlen(commomFileDir), sizeof(tmpList->commomFileDir)));
	memcpy(tmpList->TemplateFileDir, templateFileDir, MY_MIN(strlen(templateFileDir), sizeof(tmpList->TemplateFileDir)));

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

	if(g_flag)	//g_flagΪ��ʱ���Զ��ѹر�IPC,�����ٷ������ݰ��� ֱ���˳�����
	{
		//1.������ݣ�����֪�����̽��ر�IPCͨ��
		msgbuf.mtype = recvType;
		len = sprintf(msgbuf.mtext, "%s", "0~");
		if((ret = msgsnd(handle->msqid, &msgbuf, len, 0)) < 0)
		{
			socket_log(SocketLevel[4], ret, "err : func msgsnd()");
			return ret;
		}
		
		msgbuf.mtype = handle->msqid;	//������� ֪ͨ��һͨ�Ž��̣� IPCͨ�Ž��ر�
		len = sprintf(msgbuf.mtext, "%s", "1~");
		if((ret = msgsnd(handle->msqid, &msgbuf, len, 0)) < 0)
		{
			socket_log(SocketLevel[4], ret, "err : func msgsnd()");
			return ret;
		}
		sleep(1);
	}
	
	//2.�ر���Ϣ����
	if((ret = msgctl(handle->msqid, IPC_RMID, NULL)) < 0)
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
	if((ret = destroy_list(handle)) < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func destroy_list()");
		return ret;
	}

	return ret;
}


//��Ϣ���з�����Ϣ
int  scan_send_msgque(int cmd, char *datainfo, listNode *handle)
{
	int ret = 0, len = 0;
	myMsgBuf buf;
	
	if(datainfo == NULL)
	{
		socket_log(SocketLevel[4], -1, "err : datainfo == NULL");
		return ret   = -1;
	}
	
	buf.mtype = handle->msqid;
	len = sprintf(buf.mtext, "%d~%s~", cmd, datainfo);		
	
	if((ret = msgsnd(handle->msqid, &buf, len, 0)) < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func msgsnd() %s", strerror(errno));
		return ret;
	}

	return ret;
}


/*������Ϣ���е���Ϣ, ִ����ǰע��Ļص�����
 *@param: handle : ����ľ��
 *@retval: 0 �ɹ�, -1 ʧ��, -2 The IPC Queue closed; (else, The cmd callBack func return value )
*/
int  scan_recv_msgque(listNode *handle)
{
	int ret = 0, len = 0, cmd = 0;
	int recvType = 1;
	myMsgBuf	msgBuf;
	char cmdBuf[4] = { 0 };
	listNode *tmpNode = NULL;
		
	if(!handle)
	{
		socket_log(SocketLevel[4], -1, "err : handle == NULL ");
		return ret = -1;
	}


	//1.��ȡ���յ���Ϣ��������
	memset(&msgBuf, 0, sizeof(myMsgBuf));

	while(1)
	{
		if((len = msgrcv(handle->msqid, &msgBuf, MTEXTSIZE, recvType, 0)) < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				perror("msgrcv");
				socket_log(SocketLevel[4], len, "err : func msgrcv()");
				return len;
			}	
		}
		
		printf("recv news : %s, [%d],[%s]\n", msgBuf.mtext, __LINE__, __FILE__);
		break;
	}	
	
	
	//2.��ȡ��Ϣ�����е�������
	memset(cmdBuf, 0, sizeof(cmdBuf));
	sscanf(msgBuf.mtext, "%[^~]", cmdBuf);
	cmd = atoi(cmdBuf);

	//3.�����˳�ѭ��,�ر���Ϣ����; (�����յ���������Ϊ 0ʱ, ��ʶ���������ر���Ϣ����,����ѭ��)
	if(cmd == 0)
	{
		printf("will close msg queue, cmd = %d\n", cmd);
		return -2;
	}
	else if(cmd == 1)
	{
		g_flag = false;
		printf("The opposite site will close msg queue, cmd = %d\n", cmd);		
		return ret = -2;		
	}
	else if(cmd == 0x24)
	{
		if((ret = call_back_free_shmory_block(msgBuf.mtext, handle)) < 0)
		{
			socket_log(SocketLevel[4], -1, "err : func call_back_free_shmory_block()");
			return ret = -1;
		}
		return ret;
	}

	//4.���������ֶ�Ӧ�� �ص�����, δ�ҵ�ʱ�˳�ѭ��
	if((tmpNode = find_cmd_func(handle, cmd)) == NULL)
	{
		socket_log(SocketLevel[4], -1, "err : func find_cmd_func()");
		return ret = -1;
	}
	
	//5.��ձ���
	memset(tmpNode->arg, 0, sizeof(tmpNode->arg));
	
	//6.��ȡ��Ϣ�����е���Ϣ������ arg;
	memcpy(tmpNode->arg, msgBuf.mtext, sizeof(tmpNode->arg));		//ȥ������~

	//7.ִ�лص�����	// arg ����Ϊ: cmd~content
	ret = tmpNode->func(tmpNode->arg, tmpNode);
	socket_log(SocketLevel[2], ret, " The cmd %d callBack func return : %d ",tmpNode->cmd, ret );

	return ret;
}

extern int pox_system(const char * cmd_line);

//�����ļ�, д������, ����UI���̷����ļ�����·��
int  no_tmeplate_sendData(listNode *handle, void *fileName, void *picData, unsigned int picLen)
{
	int ret = 0, writeLen = 0, len = 0;
	FILE *fp = NULL;
	char filePath[256] = { 0 };
	int cmd = 160;			//��ʶ: �����ļ�·��������

	//1.�������ߴ��ļ�
	sprintf(filePath, "%s%s",handle->commomFileDir, (char *)fileName);

	if((fp = fopen(filePath, "wb")) == NULL)
	{
		socket_log(SocketLevel[4], ret, "err : func fopen(): %s", filePath);
		return ret = -1;
	}

	//2.д���ļ�����
	while(writeLen < picLen)
	{		
		if((len = fwrite(picData + writeLen, 1, picLen - writeLen, fp)) < 0)
		{
			socket_log(SocketLevel[4], ret, "err : func fwrite() %s", strerror(errno));
			fclose(fp);
			return ret = -1;
		}			
		writeLen += len;
	}

	
	//3.�ֶ�ˢ��,���ر��ļ�
	if((ret = fflush(fp)) < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func fflush()");
		fclose(fp);
		return ret;
	}

	fclose(fp);


	//4.���ʹ����ļ��ľ���·��
	if((ret = scan_send_msgque(cmd, filePath, handle)) < 0)
	{
		socket_log(SocketLevel[4], ret, "err : func scan_send_msgque()");
		return ret;
	}	

	
	return ret;
}

//��ָ����ֵ����Ϣ����;  0 �ɹ�; -1 ʧ��
int open_msg_queue(key_t keyserver, int *srcMsqid)
{
	int ret = 0, msqid = 0;
	
	if((msqid = msgget(keyserver, 0666 | IPC_CREAT | IPC_EXCL)) < 0)
	{
		if(errno == EEXIST)
		{
			//1.��ȡ�Ѵ�����Ϣ���е�ID 
			if((msqid = msgget(keyserver, 0666 )) < 0)
			{
				ret = -1;
				socket_log(SocketLevel[4], ret, "err : func msgget()");
				return ret;
			}
			//2.�ر���Ϣ����
			if((ret = msgctl(msqid, IPC_RMID, NULL)) < 0)
			{				
				socket_log(SocketLevel[4], ret, "err : func msgget()");
				return ret;
			}
			//3.�����µ���Ϣ����			
			if((msqid = msgget(keyserver, 0666 | IPC_CREAT )) < 0)
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

//��ȡx��y�η�; x^y
unsigned int power(int x, int y)
{
    unsigned int result = 1;
    while (y--)
        result *= x;
    return result;
}
//���ַ���������ת��Ϊ 16����
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


/*��Ϣ����ע�ắ��; ����: ��ʹ�ú�IPC�����ڴ�� �ڴ������ڴ����
*@param: arg : ���յ�����Ϣ������������. style : cmd~info~
*@param: shmHandle : ��������ڴ�ؾ��
*@retval: �ɹ� 0; ʧ�� -1
*/
int call_back_free_shmory_block(void *arg, void *srcShmHandle)
{	
	int ret = 0;
	shmory_pool *shmHandle = NULL;
	ipcHandle *handle = (ipcHandle *)srcShmHandle;
	unsigned int offset = 0;
	char *tmp = NULL;
	char buf[256] = { 0 };

	if(!srcShmHandle)
	{
		ret = -1;
		printf("Err : func call_back_free_shmory_block() srcShmHandle == NULL, [%d], [%s]\n", __LINE__, __FILE__);
		socket_log(SocketLevel[4], ret, "Err : func call_back_free_shmory_block() srcShmHandle == NULL"); 
		return ret ;
	}
	
	if((shmHandle = handle->shmPool) == NULL)		assert(0);
	
	tmp = strchr(arg, '~');
	memcpy(buf, tmp+1, strlen(tmp) - 2);
	offset = atoui(buf);
	
	if((ret = shmory_pool_free(shmHandle, offset)) < 0)
	{
		socket_log(SocketLevel[2], ret, "Err : func shmory_pool_free()"); 
		printf("Err : func shmory_pool_free() [%d], [%s]\n", __LINE__, __FILE__);
		return ret;
	}	
	
	return ret;
}

