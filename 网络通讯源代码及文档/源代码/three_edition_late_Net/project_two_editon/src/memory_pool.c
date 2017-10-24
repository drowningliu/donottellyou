#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#include "memory_pool.h"
#include "socklog.h"

static int g_unitMemorySize = -1;
static int g_memory_capacity = -1;
static pthread_mutex_t	*g_mutex;


//内存池的初始化
mem_pool_t *mem_pool_init(int capacity, int unitSize, pthread_mutex_t *mutex)
{
	int  i = 0, ret = 0; 
	char *work = NULL;	
	mem_pool_t *memoryPool = NULL;
	
	
	g_unitMemorySize = unitSize;
	g_memory_capacity = capacity;
	
	
	if(capacity <= 0 || unitSize <= 0 || !mutex)
	{
		ret = -1;
        socket_log(SocketLevel[4], ret, "capacity <= 0 || unitSize <= 0 || !mutex");
        goto End;		
	}
	
	//1.对内存池进行初始化
	memoryPool = (mem_pool_t *)malloc(sizeof(mem_pool_t));
	if(NULL == memoryPool)
	{
		ret = -1;
        socket_log(SocketLevel[4], ret, "func malloc() error");
        goto End;			
	}
	//1.内存池的大块内存空间
	memoryPool->buffer_arr = (char*)malloc(capacity * unitSize );
	if(NULL == memoryPool->buffer_arr)
	{
		ret = -1;
        socket_log(SocketLevel[4], ret, "func malloc() error");
        goto End;			
	}
	//2.存储每个内存单元的地址
	memoryPool->index_arr = (char **)malloc(capacity * sizeof(char *));
	if(NULL == memoryPool->index_arr)
	{
		ret = -1;
        socket_log(SocketLevel[4], ret, "func malloc() error");
        goto End;			
	}
	//3.对 memoryPool->index_arr 初始化
	work = memoryPool->buffer_arr;
	for(i = 0; i < capacity; i++, work += unitSize)
	{
		memoryPool->index_arr[i] = work;		
	}
	
	memoryPool->index_end = memoryPool->index_arr + capacity;
	memoryPool->index_cur = memoryPool->index_arr;
	g_mutex = mutex;
	
	
End:
	
	return memoryPool;
}

//从内存池中分配一个单元
void *mem_pool_alloc(mem_pool_t *memoryPool)
{
	int ret = 0;
	void *charBuf = NULL;
	
	pthread_mutex_lock(g_mutex);
	
	if(NULL == memoryPool)
	{
		ret = -1;
        socket_log(SocketLevel[4], ret, "NULL == emoryPool");
        goto End;			
	}
	
	//判断内存池是否已满
	if(memoryPool->index_cur >= memoryPool->index_end)
	{
		ret = -1;
        socket_log(SocketLevel[4], ret, "memoryPool->index_cur >= memoryPool->index_end");
        goto End;			
	}
		
	charBuf = *(memoryPool->index_cur++);
	memset(charBuf, 0, g_unitMemorySize);	

	
End:	
	pthread_mutex_unlock(g_mutex);
	return charBuf;
}


//内存池回收一个对象单元
int mem_pool_free(mem_pool_t *memoryPool, void *objBlock)
{
	
	int ret = 0;
	pthread_mutex_lock(g_mutex);
	
	if(NULL == memoryPool || NULL == objBlock)
	{
		ret = -1;
        socket_log(SocketLevel[4], ret, "NULL == emoryPool || NULL == objBlock");
        goto End;			
	}
	memset(objBlock, 0, g_unitMemorySize);
	*(--memoryPool->index_cur) = (char *)objBlock;

	
End:
	pthread_mutex_unlock(g_mutex);
	return ret;
}


//销毁内存池
int  mem_pool_destroy(mem_pool_t *memoryPool)
{
	int ret = 0;

	pthread_mutex_lock(g_mutex);
	if(NULL == memoryPool )
	{
		ret = -1;
        socket_log(SocketLevel[4], ret, "NULL == memoryPool");
        goto End;			
	}
	
	if(memoryPool->buffer_arr)		free(memoryPool->buffer_arr);
	if(memoryPool->index_arr)		free(memoryPool->index_arr);	
	free(memoryPool);

End:
	pthread_mutex_unlock(g_mutex);
	
	return ret;
}

//清空内存池数据
int  mem_pool_clear(mem_pool_t *memoryPool)
{
	int ret = 0, i = 0;
	char *work = NULL;
	
	pthread_mutex_lock(g_mutex);
	
	if(NULL == memoryPool)
	{
		ret = -1;
        socket_log(SocketLevel[4], ret, "NULL == emoryPool");
        goto End;			
	}

	memset(memoryPool->buffer_arr, 0, g_unitMemorySize * g_memory_capacity);

	work = memoryPool->buffer_arr;
	for (i = 0; i < g_memory_capacity; i++, work += g_unitMemorySize)
	{
		memoryPool->index_arr[i] = work;
	}
	memoryPool->index_end = memoryPool->index_arr + g_memory_capacity;
	memoryPool->index_cur = memoryPool->index_arr;

	
	
End:
	pthread_mutex_unlock(g_mutex);
	
	return ret;	
}





































