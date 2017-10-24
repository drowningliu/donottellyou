#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#include "queue_buffer.h"
#include "socklog.h"

static int g_block_size = -1;

//��ʼ������
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

	//1.���еĳ�ʼ��
	myqueue = (queue_buffer*)malloc(sizeof(queue_buffer));
	myqueue->head = 0;
	myqueue->rear = 0;
	myqueue->capacity = capacity;
	//�ȷ����Լ���ַ�� �ٸ�һ��ַ����ռ�
	myqueue->bufferPtr = (char **)malloc(capacity * sizeof(char *));
	for (i = 0; i < capacity; i++)
	{
		(myqueue->bufferPtr)[i] = malloc(g_block_size);
		memset((myqueue->bufferPtr)[i], 0, g_block_size);
	}
	
	
End:
	
	return myqueue;
}

//�����в���Ԫ��
int push_buffer_queue(queue_buffer *myqueue, char *buffer)
{
	int ret = 0;
	
	if (!myqueue || !buffer)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue || !buffer");
		goto End;
	}

	//�ж϶����Ƿ� ����
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

//����
int pop_buffer_queue(queue_buffer *myqueue, char *buffer)
{
	int ret = 0;
	
	
	if (!myqueue || !buffer)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue || !buffer");
		goto End;
	}
	//�ж϶����Ƿ�Ϊ��
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


/*��ȡ��ǰ���еĶ�ͷԪ�أ� ��ɾ��*/
int get_buffer_queue(queue_buffer *myqueue, char *buffer)
{
	int ret = 0;

	
	if (!myqueue || !buffer)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error: !myqueue || !buffer");
		goto End;
	}
	//�ж϶����Ƿ�Ϊ��
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


//�ж϶����Ƿ�Ϊ��
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

//�����Ƿ�����
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

//������Ԫ�صĸ���
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

//�õ����ж��е�Ԫ�ظ���
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

//���ٶ���
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


//��ն���
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


