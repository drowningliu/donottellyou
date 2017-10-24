#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#include "queue_buffer.h"
#include "socklog.h"

static int g_block_size = -1;

//初始化队列
queue_buffer *queue_buffer_init(int capacity, int size)
{
	int ret = 0, i = 0;
	queue_buffer *myqueue = NULL;
	

	if (capacity <= 0 || size <= 0)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: capacity <= 0 || size <= 0");
		goto End;
	}

	g_block_size = size;

	//1.队列的初始化
	myqueue = (queue_buffer*)malloc(sizeof(queue_buffer));
	myqueue->head = 0;
	myqueue->rear = 0;
	myqueue->capacity = capacity;
	//先分配以及地址， 再给一地址分配空间
	myqueue->bufferPtr = (char **)malloc(capacity * sizeof(char *));
	for (i = 0; i < capacity; i++)
	{
		(myqueue->bufferPtr)[i] = malloc(g_block_size);
		memset((myqueue->bufferPtr)[i], 0, g_block_size);
	}
	
	
End:
	
	return myqueue;
}

//队列中插入元素
int push_buffer_queue(queue_buffer *myqueue, char *buffer)
{
	int ret = 0;
	
	if (!myqueue || !buffer)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue || !buffer");
		goto End;
	}

	//判断队列是否 已满
	if (is_buffer_full(myqueue))
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: func is_buffer_full()");
		goto End;
	}
	memset(myqueue->bufferPtr[myqueue->rear], 0, g_block_size);
	memcpy(myqueue->bufferPtr[myqueue->rear++], buffer, strlen(buffer));
	myqueue->rear %= myqueue->capacity;

End:	
	
	return ret;
}

//出队
int pop_buffer_queue(queue_buffer *myqueue, char *buffer)
{
	int ret = 0;
	
	
	if (!myqueue || !buffer)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue || !buffer");
		goto End;
	}
	//判断队列是否为空
	if (is_buffer_empty(myqueue))
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: func is_buffer_empty()");
		goto End;
	}

	memcpy(buffer, myqueue->bufferPtr[myqueue->head++], g_block_size);
	myqueue->head %= myqueue->capacity;

End:
	
	return ret;
}


/*获取当前队列的对头元素， 不删除*/
int get_buffer_queue(queue_buffer *myqueue, char *buffer)
{
	int ret = 0;

	
	if (!myqueue || !buffer)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue || !buffer");
		goto End;
	}
	//判断队列是否为空
	if (is_buffer_empty(myqueue))
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue || !buffer");
		goto End;
	}

	memcpy(buffer, myqueue->bufferPtr[myqueue->head], g_block_size);

End:

	return ret;
}


//判断队列是否为空
bool is_buffer_empty(queue_buffer *myqueue)
{
	int ret = 0;

	
	if (!myqueue)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue");
		return false;
	}
	

	return myqueue->head == myqueue->rear;
}

//队列是否满队
bool is_buffer_full(queue_buffer *myqueue)
{
	int ret = 0;
	
	if (!myqueue)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue");
		return false;
	}

	return myqueue->head == (myqueue->rear + 1) % myqueue->capacity;
}

//队列中元素的个数
int get_element_buffer_count(queue_buffer *myqueue)
{
	int ret = 0;
	
	if (!myqueue)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue");
		return ret;
	}

	return (myqueue->rear - myqueue->head + myqueue->capacity) % myqueue->capacity;
}

//得到空闲队列的元素个数
int get_free_buffer_count(queue_buffer *myqueue)
{
	int ret = 0;	
	
	if (!myqueue)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue");	
		return ret;
	}
	
	
	return myqueue->capacity - get_element_buffer_count(myqueue) - 1;
}

//销毁队列
int destroy_buffer_queue(queue_buffer *myqueue)
{
	int ret = 0, i = 0;
	
	
	if (!myqueue)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue");
		goto End;
	}

	if (myqueue->bufferPtr)
	{
		for (i = 0; i < myqueue->capacity; i++)
		{
			if (myqueue->bufferPtr[i])		free(myqueue->bufferPtr[i]);
		}
		free(myqueue->bufferPtr);
	}
	free(myqueue);
	myqueue = NULL;

End:
	

	return ret;
}


//清空队列
int clear_buffer_queue(queue_buffer *myqueue)
{
	int ret = 0, i = 0;
	
	
	if (!myqueue)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue");
		goto End;
	}

	myqueue->head = 0;
	myqueue->rear = 0;

	//memset(myqueue->bufferPtr, 0, myqueue->capacity * sizeof(char *));
	for (i = 0; i < get_element_buffer_count(myqueue); i++)
	{
		memset(myqueue->bufferPtr[i], 0, g_block_size);
	}


End:
	
	return ret;
}


