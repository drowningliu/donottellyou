#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "fifo.h"
#include "socklog.h"



static int g_fifo_capacity = -1;

//初始化队列
fifo_type *fifo_init(int capacity)
{		
	int ret = 0;
	fifo_type *myqueue = NULL;
	//socket_log( SocketLevel[2], ret, "func fifo_init()  begin");
	
	if(capacity <= 0)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: capacity <= 0");
		goto End;
	}
	g_fifo_capacity = capacity;
	
	//1.队列的初始化
	myqueue = (fifo_type*)malloc(sizeof(fifo_type));
	myqueue->head = 0;
	myqueue->rear = 0;	
	myqueue->capacity = capacity;	
	myqueue->bufferPtr = (char *)malloc(capacity);			
	
End:
	
	return myqueue;
}

//队列中插入元素
int push_fifo(fifo_type *myqueue, const char *buffer, const int count)
{
	int ret = 0;
	int i = 0;
	
	
	if(count > get_fifo_free_count(myqueue) )
	{
		ret = -2;
		socket_log( SocketLevel[4], ret, "Error: count > get_free_count(myqueue)");
		goto End;
	}
	
	if( !myqueue || !buffer)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error:  || !myqueue || !buffer");
		goto End;
	}
	
	for(i = 0; i < count; i++)
	{
		myqueue->bufferPtr[myqueue->rear++] = buffer[i];
		myqueue->rear %= myqueue->capacity;
	}
	
End:
	
	return ret;
}	

//出队
int pop_fifo(fifo_type *myqueue, char *buffer, const int maxNum)
{
	int ret = 0;
	int num = 0;
	
	
	if(maxNum > get_fifo_element_count(myqueue) || !myqueue || !buffer)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: maxNum > get_free_count(myqueue) || !myqueue || !buffer");
		goto End;
	}
	
	while(num < maxNum)
	{
		if(is_empty_fifo(myqueue))
			break;
		*(buffer + num++) = myqueue->bufferPtr[myqueue->head++];
		myqueue->head %= myqueue->capacity;
	}	
	
End:	
	
	return ret;
}

//判断队列是否为空
bool is_empty_fifo(fifo_type *myqueue)
{
	int ret = 0;
			
	if(!myqueue )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
	
		return false;
	}
		
	return myqueue->head == myqueue->rear;
}

//队列是否满队
bool is_full_fifo(fifo_type *myqueue)
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
int get_fifo_element_count(fifo_type *myqueue)
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

//得到空闲队列的元素
int get_fifo_free_count(fifo_type *myqueue)
{
	int ret = 0;
	if(!myqueue )
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: myqueue == NULL");
		return ret;
	}
			
	return myqueue->capacity - get_fifo_element_count(myqueue) -1;
}

//销毁队列
int destroy_fifo_queue(fifo_type *myqueue)
{
	int ret = 0;
		
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
	
	return ret;
}


//清空队列
int clear_fifo(fifo_type *myqueue)
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
	memset(myqueue->bufferPtr, 0, g_fifo_capacity );
	
End:	
	
	return  ret;
}












