#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "queue_sendAddr_dataLenth.h"
#include "socklog.h"

static int g_queue_capacity = -1;
static pthread_mutex_t *g_mutex;

//初始化队列
queue_send_dataBLock queue_init_send_block(int capacity, pthread_mutex_t *mutex)
{		
	int ret = 0;	 	
	sendDataBlock *myqueue = NULL;
	socket_log( SocketLevel[2], ret, "func queue_init_send_block()  begin");
	
	if(capacity <= 0 || mutex == NULL)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: capacity <= 0 || mutex == NULL");
		goto End;
	}
	
	g_queue_capacity = capacity;
	g_mutex = mutex;
	
	//1.队列的初始化
	myqueue = (sendDataBlock*)malloc(sizeof(sendDataBlock));
	myqueue->head = 0;
	myqueue->rear = 0;	
	myqueue->capacity = capacity;	
	myqueue->buffer = (dataBlock *)malloc(capacity * sizeof(dataBlock));	

	
End:
	socket_log( SocketLevel[2], ret, "func queue_init_send_block()  end");
	return (queue_send_dataBLock )myqueue;
}

//队列中插入元素
int push_queue_send_block(queue_send_dataBLock myqueue, void *sendDataAddr, int sendDataLenth)
{
	int ret = 0;
	pthread_mutex_lock(g_mutex);
	
	if( !myqueue || !sendDataAddr || sendDataLenth < 0)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: !myqueue || !sendDataAddr || sendDataLenth < 0");
		goto End;
	}
	
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;
	
	
	//判断队列是否 已满
	if(is_full_send_block(myqueue))
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: the queue no have memory");
		goto End;
	}
	

	sendNews->buffer[sendNews->rear].sendDataAddr = sendDataAddr;
	sendNews->buffer[sendNews->rear++].sendDataLenth = sendDataLenth;
	sendNews->rear %= sendNews->capacity;
	
End:
	pthread_mutex_unlock(g_mutex);
	return ret;
}	

//出队
int pop_queue_send_block(queue_send_dataBLock myqueue, char **addr, int *sendDataLenth)
{
	int ret = 0;
	pthread_mutex_lock(g_mutex);
	
	if(!myqueue || !addr || !sendDataLenth)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: !myqueue || !addr || !sendDataLenth");
		goto End;
	}
	
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;
	
	//判断队列是否为空
	if(is_empty_send_block(myqueue))
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: The queue have no member");
		goto End;
	}
	
	*addr = sendNews->buffer[sendNews->head].sendDataAddr;
	*sendDataLenth = sendNews->buffer[sendNews->head++].sendDataLenth; 
	//sendNews->head++;
	sendNews->head %= sendNews->capacity;
	
End:	
	pthread_mutex_unlock(g_mutex);
	return ret;
}


/*获取当前队列的对头元素， 不删除*/
int get_queue_send_block(queue_send_dataBLock myqueue, char **addr, int *sendDataLenth)
{
	int ret = 0;
	
	if(!myqueue || !addr || !sendDataLenth)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: !myqueue || !addr || !sendDataLenth");
		goto End;
	}
		
	sendDataBlock *sendNews = (sendDataBlock *)myqueue;
	
	//判断队列是否为空
	if(is_empty_send_block(myqueue))
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: The queue have no member");
		goto End;
	}
	
	*addr = sendNews->buffer[sendNews->head].sendDataAddr;
	*sendDataLenth = sendNews->buffer[sendNews->head].sendDataLenth; 
		
End:

	return ret;
}


//判断队列是否为空
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

//队列是否满队
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

//队列中元素的个数
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

//得到空闲队列的元素个数
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

//销毁队列
int destroy_queue_send_block(queue_send_dataBLock myqueue)
{
	int ret = 0;
	pthread_mutex_lock(g_mutex);
	
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
	pthread_mutex_unlock(g_mutex);
	return ret;
}


//清空队列
int clear_queue_send_block(queue_send_dataBLock myqueue)
{
	int ret = 0;
	pthread_mutex_lock(g_mutex);
	
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
	pthread_mutex_unlock(g_mutex);
	return ret;
}












