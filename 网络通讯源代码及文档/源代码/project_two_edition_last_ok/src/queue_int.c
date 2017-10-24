#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "queue_int.h"
#include "socklog.h"

static int g_queue_capacity = -1;

//初始化队列
queue_int *queue_int_init(int capacity)
{		
	int ret = 0;
	queue_int *myqueue = NULL;
	socket_log( SocketLevel[2], ret, "func queue_int_init()  begin");
	
	if(capacity <= 0 )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: capacity <= 0 || !mutex");
		goto End;
	}
	
	g_queue_capacity = capacity;
	
	//1.队列的初始化
	myqueue = (queue_int*)malloc(sizeof(queue_int));
	myqueue->head = 0;
	myqueue->rear = 0;	
	myqueue->capacity = capacity;	
	myqueue->bufferPtr = (int *)malloc(capacity * sizeof(int));	
	
	
End:
	socket_log( SocketLevel[2], ret, "func queue_int_init()  end");
	return myqueue;
}

//队列中插入元素
int push_int_queue(queue_int *myqueue, int lenth)
{
	int ret = 0;
	
	
	
	if( !myqueue )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: count > get_free_count(myqueue) || !myqueue ");
		goto End;
	}
	//判断队列是否 已满
	if(is_int_full(myqueue))
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: the queue no have memory");
		goto End;
	}
	

	myqueue->bufferPtr[myqueue->rear++] = lenth;
	myqueue->rear %= myqueue->capacity;
	
End:
	
	
	return ret;
}	

//出队
int pop_int_queue(queue_int *myqueue , int *lenth)
{
	int ret = 0;
		
	
	if(!myqueue )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: maxNum > get_free_count(myqueue) || !myqueue || !buffer");
		goto End;
	}
	//判断队列是否为空
	if(is_int_empty(myqueue))
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: The queue have no member");
		goto End;
	}
	
	if(lenth != NULL)		
		*lenth = myqueue->bufferPtr[myqueue->head];	
	myqueue->head++;
	myqueue->head %= myqueue->capacity;
	
End:	
	
	return ret;
}


/*获取当前队列的对头元素， 不删除*/
int get_int_queue(queue_int *myqueue, int *buffer)
{
	int ret = 0;

	
	if(!myqueue || !buffer)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: maxNum > get_free_count(myqueue) || !myqueue || !buffer");
		goto End;
	}
	//判断队列是否为空
	if(is_int_empty(myqueue))
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: The queue have no member");
		goto End;
	}
	
	*buffer = myqueue->bufferPtr[myqueue->head];
	
End:	

	return ret;
}

//判断队列是否为空
bool is_int_empty(queue_int *myqueue)
{
		
	int ret = 0;
	if(!myqueue )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");\

		return false;
	}
	

	
	return myqueue->head == myqueue->rear;
}

//队列是否满队
bool is_int_full(queue_int *myqueue)
{
	int ret = 0;
		
	if(!myqueue )
	{
		ret = -1;
	
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		return false;
	}

	
	return myqueue->head == (myqueue->rear + 1) % myqueue->capacity;
}

//队列中元素的个数
int get_int_element_count(queue_int *myqueue)
{
	int ret = 0;

	
	if(!myqueue )
	{
		ret = -1;
		
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		return ret;
	}
	
	
	return (myqueue->rear - myqueue->head + myqueue->capacity) % myqueue->capacity;
}


//得到空闲队列的元素个数
int get_int_free_count(queue_int *myqueue)
{
	int ret = 0;
	
	
	if(!myqueue )
	{
		ret = -1;
		
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		return ret;
	}
	
	
	return myqueue->capacity - get_int_element_count(myqueue) -1;
}

//销毁队列
int destroy_int_queue(queue_int *myqueue)
{
	int ret = 0;
	
	
	socket_log( SocketLevel[2], ret, "func destroy_queue()  begin");
	if(!myqueue)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		goto End;
	}	
	
	if(myqueue->bufferPtr)		free(myqueue->bufferPtr);
	free(myqueue);
	myqueue = NULL;
	
End:
	socket_log( SocketLevel[2], ret, "func destroy_queue()  end");
	
	return ret;
}

//清空队列
int clear_int_queue(queue_int *myqueue)
{
	int ret = 0;
	
	
	if(!myqueue)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		goto End;
	}	

	myqueue->head = 0;
	myqueue->rear = 0;	
	memset(myqueue->bufferPtr, 0, g_queue_capacity * sizeof(int));
		
End:
	
	return ret;
}
