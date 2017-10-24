#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include "queue_sendAddr_dataLenth.h"
#include "socklog.h"

static int g_queue_capacity = -1;

//��ʼ������
queue_send_dataBLock queue_init_send_block(int capacity)
{		
	int ret = 0;	 	
	sendDataBlock *myqueue = NULL;
	socket_log( SocketLevel[2], ret, "func queue_init_send_block()  begin");
	
	if(capacity <= 0 )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: capacity <= 0 || mutex == NULL");
		goto End;
	}
	
	g_queue_capacity = capacity;
	
	
	//1.���еĳ�ʼ��
	myqueue = (sendDataBlock*)malloc(sizeof(sendDataBlock));
	myqueue->head = 0;
	myqueue->rear = 0;	
	myqueue->capacity = capacity;	
	myqueue->buffer = (dataBlock *)malloc(capacity * sizeof(dataBlock));	

	
End:
	socket_log( SocketLevel[2], ret, "func queue_init_send_block()  end");
	return (queue_send_dataBLock )myqueue;
}

//�����в���Ԫ��
int push_queue_send_block(queue_send_dataBLock myqueue, void *sendDataAddr, int sendDataLenth, int cmd)
{
	int ret = 0;
	
	
	if( !myqueue || !sendDataAddr || sendDataLenth < 0)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: !myqueue || !sendDataAddr || sendDataLenth < 0");
		goto End;
	}
	
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;
	
	
	//�ж϶����Ƿ� ����
	if(is_full_send_block(myqueue))
	{
		ret = -1;
		printf("\nThe elemet Number : %d ... [%d],[%s]\n", get_element_count_send_block(myqueue), __LINE__, __FILE__ );		
		socket_log( SocketLevel[4], ret, "Error: the queue no have memory, The elemet Number : %d", get_element_count_send_block(myqueue));
		assert(0);		
	}
	

	sendNews->buffer[sendNews->rear].sendDataAddr = sendDataAddr;
	sendNews->buffer[sendNews->rear].cmd = cmd;
	sendNews->buffer[sendNews->rear++].sendDataLenth = sendDataLenth;
	sendNews->rear %= sendNews->capacity;
	
End:
	
	return ret;
}	

//����
int pop_queue_send_block(queue_send_dataBLock myqueue, char **addr, int *sendDataLenth, int *cmd)
{
	int ret = 0;
	
	
	if(!myqueue )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL ");
		goto End;
	}
	
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;
	
	//�ж϶����Ƿ�Ϊ��
	if(is_empty_send_block(myqueue))
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: The queue have no member");
		goto End;
	}
	
	if(addr)			*addr = sendNews->buffer[sendNews->head].sendDataAddr;
	if(cmd)				*cmd = sendNews->buffer[sendNews->head].cmd;
	if(sendDataLenth)	*sendDataLenth = sendNews->buffer[sendNews->head++].sendDataLenth; 

	sendNews->head %= sendNews->capacity;
	
End:	
	
	return ret;
}


/*��ȡ��ǰ���еĶ�ͷԪ�أ� ��ɾ��*/
int get_queue_send_block(queue_send_dataBLock myqueue, char **addr, int *sendDataLenth, int *cmd)
{
	int ret = 0;
	
	if(!myqueue)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		goto End;
	}
		
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;
	
	//�ж϶����Ƿ�Ϊ��
	if(is_empty_send_block(myqueue))
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: The queue have no member");
		goto End;
	}
	
	if(addr)				*addr = sendNews->buffer[sendNews->head].sendDataAddr;
	if(cmd)					*cmd = sendNews->buffer[sendNews->head].cmd;
	if(sendDataLenth)		*sendDataLenth = sendNews->buffer[sendNews->head].sendDataLenth; 
		
End:

	return ret;
}


//�ж϶����Ƿ�Ϊ��
bool is_empty_send_block(queue_send_dataBLock myqueue)
{
	int ret = 0;
	if(!myqueue )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		return 0;
	}
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;
	
	return sendNews->head == sendNews->rear;
}

//�����Ƿ�����
bool is_full_send_block(queue_send_dataBLock myqueue)
{
	int ret = 0;
	if(!myqueue )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		return 0;
	}
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;
	
	return sendNews->head == (sendNews->rear + 1) % sendNews->capacity;
}

//������Ԫ�صĸ���
int get_element_count_send_block(queue_send_dataBLock myqueue)
{
	int ret = 0;
	if(!myqueue )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		return ret;
	}
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;
	
	return (sendNews->rear - sendNews->head + sendNews->capacity) % sendNews->capacity;
}

//�õ����ж��е�Ԫ�ظ���
int get_free_count_send_block(queue_send_dataBLock myqueue)
{
	int ret = 0;
	if(!myqueue )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		return ret;
	}
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;		
		
	return sendNews->capacity - get_element_count_send_block(myqueue) -1;
}

//���ٶ���
int destroy_queue_send_block(queue_send_dataBLock myqueue)
{
	int ret = 0;
	
	
	socket_log( SocketLevel[2], ret, "func destroy_queue()  begin");
	if(!myqueue)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		goto End;
	}	
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;	
	
	if(sendNews->buffer)		free(sendNews->buffer);
	free(sendNews);
	myqueue = NULL;
	
End:
	socket_log( SocketLevel[2], ret, "func destroy_queue()  end");
	
	return ret;
}


//��ն���
int clear_queue_send_block(queue_send_dataBLock myqueue)
{
	int ret = 0;
	
	
	if(!myqueue)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		goto End;
	}	
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;	
	
	sendNews->head = 0;
	sendNews->rear = 0;	
	memset(sendNews->buffer, 0, g_queue_capacity * sizeof(sendDataBlock ));
		
End:	
	
	return ret;
}












