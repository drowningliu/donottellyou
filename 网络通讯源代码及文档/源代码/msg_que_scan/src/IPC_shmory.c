#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <errno.h>


#include "IPC_shmory.h"
#include "socklog.h"


static int g_unitMemorySize = -1;
static int g_memory_capacity = -1;
static pthread_mutex_t *g_mutex;

#define _COUPLE_
#define _NO_COUPLE_ 1

/*内存池的初始化
*参数：   capacity：  内存池的容量,即内存块的个数
*		  unitSize:   内存块的大小
*返回值： 内存池的对象指针， 创建失败返回NULL
*/
shmory_pool *shmory_pool_init(key_t key, int capacity, int unitSize, pthread_mutex_t *mutex)
{
	int  i = 0, ret = 0;
	int  shmid = 0;
	char *work = NULL;
	shmory_pool *shmoryPool = NULL;;
	void *shared_memory = NULL;

	if(capacity <= 0 || unitSize <= 0)
	{
		ret = -1;
		socket_log(SocketLevel[2], ret, "capacity <= 0 || unitSize <= 0");	
		return NULL;
	}

	//1.句柄内存申请
	shmoryPool = malloc(sizeof(shmory_pool));
	if(NULL == shmoryPool)
	{
		ret = -1;
		socket_log(SocketLevel[2], ret, " func malloc(sizeof(shmory_pool));");	
		return NULL;
	}

	//2.获取指定键值下的 共享内存
#ifdef _COUPLE_
	shmid = shmget(key, capacity * unitSize, 0666 | IPC_CREAT | IPC_EXCL);
	if(shmid == -1)
	{
		//2.1 程序启动后, 存在原来的共享内存,先删除,再创建新的
		if(errno == EEXIST)
		{
			shmid = shmget(key, 0, 0);
			if(shmid < 0)
			{
				socket_log(SocketLevel[4], ret, "Err: func shmget() 0666 | IPC_CREAT"); 
				free(shmoryPool);
				return NULL;
			}
			
			ret = shmctl(shmid,  IPC_RMID, NULL);
			if(ret < 0)
			{
				socket_log(SocketLevel[4], ret, "Err: func shmctl() IPC_RMID");	
				free(shmoryPool);
				return NULL;
			}	
			
			shmid = shmget(key, capacity * unitSize, 0666 | IPC_CREAT);
			if(shmid < 0)
			{
				socket_log(SocketLevel[4], ret, "Err: func shmget() 0666 | IPC_CREAT"); 
				free(shmoryPool);
				return NULL;
			}
		}
		else
		{
			socket_log(SocketLevel[4], ret, "Err: func shmget()");	
			free(shmoryPool);
			return NULL;
		}

	}
#endif

#ifdef _NO_COUPLE_
	shmid = shmget(key, capacity * unitSize, 0666 | IPC_CREAT | IPC_EXCL);
	if(shmid == -1)
	{
		if(errno == EEXIST)
		{
			shmid = shmget(key, 0, 0);
			if(shmid < 0)
			{
				socket_log(SocketLevel[4], ret, "Err: func shmget() 0666 | IPC_CREAT"); 
				free(shmoryPool);
				return NULL;
			}
		}
		else
		{
			socket_log(SocketLevel[4], ret, "Err: func shmget()");	
			free(shmoryPool);
			return NULL;
		}
	}

#endif

	//3.将该共享内存连接到该进程地址空间
	shared_memory = shmat(shmid, NULL, 0);
	if (shared_memory == (void *)-1) 
	{
	    socket_log(SocketLevel[2], ret, "Err: func shmat()");
		free(shmoryPool);
		return NULL;
  	}

	//4.将该共享内存与 内存池结构关联, 使它被内存池管理
	shmoryPool->buffer_arr = shared_memory;
	
	//5.内存池每个单元的地址
	shmoryPool->index_arr = (char **)malloc(capacity * sizeof(char *));
	if (NULL == shmoryPool->index_arr) 
	{
	    socket_log(SocketLevel[2], ret, "Err : malloc(capacity * sizeof(char *))");
		free(shmoryPool);
		return NULL;
  	}

	//6.对 shmoryPool->index_arr 初始化
	work = shmoryPool->buffer_arr;
	for(i = 0; i < capacity; i++, work += unitSize)
	{
		shmoryPool->index_arr[i] = work;
	}
	
	shmoryPool->index_end = shmoryPool->index_arr + capacity;	
	shmoryPool->index_cur = shmoryPool->index_arr;
	shmoryPool->shmid = shmid;
	g_mutex = mutex;
	g_unitMemorySize = unitSize;
	g_memory_capacity = capacity;

	
	return shmoryPool;
}


/*从内存池中分配一个单元
*@param：memoryPool : 内存池的句柄
*@param: offset : 返回内存块地址的偏移量.
*@retval: 成功 : 返回的申请内存块地址;   失败 : 返回NULL�
*/
void *shmory_pool_alloc(shmory_pool *memoryPool, unsigned int *offset)	
{
	int ret = 0;
	void *charBuf = NULL;
	pthread_mutex_lock(g_mutex);

	if(NULL == memoryPool || NULL == offset )	
	{		
		ret = -1;		 
		socket_log(SocketLevel[4], ret, "NULL == memoryPool || NULL == offset ");		  
		goto End;
	}	

	//1. 判断内存池是否已满
	if(memoryPool->index_cur >= memoryPool->index_end)
	{
		ret = -1;		 
		socket_log(SocketLevel[4], ret, "memoryPool->index_cur >= memoryPool->index_end");		  
		goto End;
	}

	charBuf = *(memoryPool->index_cur++);
	memset(charBuf, 0, g_unitMemorySize);
	*offset = (char*)charBuf - memoryPool->buffer_arr;
	
End:
	
	pthread_mutex_unlock(g_mutex);
	return charBuf;
}




/*内存池回收一个对象单元
*@param ： memoryPool  内存池的句柄
*@param :  offset	   回收内存块的偏移量
*返回值：  0 成功 ；  -1  失败
*/
int shmory_pool_free(shmory_pool *memoryPool, unsigned int offset)	
{
	int ret = 0;
	void *objBlock = NULL;
	pthread_mutex_lock(g_mutex);

	
	if(NULL == memoryPool || offset < 0 )
	{
		ret = -1;        
		socket_log(SocketLevel[4], ret, "memoryPool : %p, offset : %u, g_unitMemorySize : %d",
					memoryPool, offset, g_unitMemorySize);     
		goto End;	
	}

	//1.找到内存池中该偏移量的地址
	objBlock = memoryPool->buffer_arr + offset;

	//2.清空该偏移量, 并将地址放回内存池中
	memset(objBlock, 0, g_unitMemorySize);
	*(--memoryPool->index_cur) = (char *)objBlock;


End:	
	pthread_mutex_unlock(g_mutex);

	return ret;
}




/*销毁内存池
*@param : memoryPool 内存池的句柄
*@retval: 0 成功; -1 失败
*/
int  shmory_pool_destroy(shmory_pool *memoryPool)	
{
	int ret = 0;	
	pthread_mutex_lock(g_mutex);	
	if(NULL == memoryPool )	
	{		
		ret = -1;       
		socket_log(SocketLevel[4], ret, "NULL == memoryPool");
		goto End;				
	}		

	//1.分离共享内存
	if (shmdt(memoryPool->buffer_arr) == -1) 
	{
		ret = -1;       
		socket_log(SocketLevel[4], ret, "Err : func shmdt()");
		goto End;
	}

	//2.删除共享内存
	ret = shmctl(memoryPool->shmid, IPC_RMID, NULL);
	if(ret < 0)
	{
		if(errno == EINVAL)
			ret = 0;
		else if(errno == EIDRM)
			ret =  0;
		else
		{
			socket_log(SocketLevel[2], ret, "Err: func shmctl() IPC_RMID %s", strerror(errno));	
		}	
	}	

	//3.释放共享内存的数据结构
	if(memoryPool->index_arr)		free(memoryPool->index_arr);	
	free(memoryPool);

		
End:	
	pthread_mutex_unlock(g_mutex);	

	return ret;
}



//清空内存池数据
int  shmory_pool_clear(shmory_pool *memoryPool)	
{
	int ret = 0, i = 0;
	char *work = NULL;		
	pthread_mutex_lock(g_mutex);
	
	if(memoryPool == NULL)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "NULL == memoryPool"); 
		goto End;		
	}

	//1.清空共享内存
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


