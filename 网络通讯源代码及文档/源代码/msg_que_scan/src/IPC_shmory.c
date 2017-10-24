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

/*ÄÚ´æ³ØµÄ³õÊ¼»¯
*²ÎÊı£º   capacity£º  ÄÚ´æ³ØµÄÈİÁ¿,¼´ÄÚ´æ¿éµÄ¸öÊı
*		  unitSize:   ÄÚ´æ¿éµÄ´óĞ¡
*·µ»ØÖµ£º ÄÚ´æ³ØµÄ¶ÔÏóÖ¸Õë£¬ ´´½¨Ê§°Ü·µ»ØNULL
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

	//1.¾ä±úÄÚ´æÉêÇë
	shmoryPool = malloc(sizeof(shmory_pool));
	if(NULL == shmoryPool)
	{
		ret = -1;
		socket_log(SocketLevel[2], ret, " func malloc(sizeof(shmory_pool));");	
		return NULL;
	}

	//2.»ñÈ¡Ö¸¶¨¼üÖµÏÂµÄ ¹²ÏíÄÚ´æ
#ifdef _COUPLE_
	shmid = shmget(key, capacity * unitSize, 0666 | IPC_CREAT | IPC_EXCL);
	if(shmid == -1)
	{
		//2.1 ³ÌĞòÆô¶¯ºó, ´æÔÚÔ­À´µÄ¹²ÏíÄÚ´æ,ÏÈÉ¾³ı,ÔÙ´´½¨ĞÂµÄ
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

	//3.½«¸Ã¹²ÏíÄÚ´æÁ¬½Óµ½¸Ã½ø³ÌµØÖ·¿Õ¼ä
	shared_memory = shmat(shmid, NULL, 0);
	if (shared_memory == (void *)-1) 
	{
	    socket_log(SocketLevel[2], ret, "Err: func shmat()");
		free(shmoryPool);
		return NULL;
  	}

	//4.½«¸Ã¹²ÏíÄÚ´æÓë ÄÚ´æ³Ø½á¹¹¹ØÁª, Ê¹Ëü±»ÄÚ´æ³Ø¹ÜÀí
	shmoryPool->buffer_arr = shared_memory;
	
	//5.ÄÚ´æ³ØÃ¿¸öµ¥ÔªµÄµØÖ·
	shmoryPool->index_arr = (char **)malloc(capacity * sizeof(char *));
	if (NULL == shmoryPool->index_arr) 
	{
	    socket_log(SocketLevel[2], ret, "Err : malloc(capacity * sizeof(char *))");
		free(shmoryPool);
		return NULL;
  	}

	//6.¶Ô shmoryPool->index_arr ³õÊ¼»¯
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


/*´ÓÄÚ´æ³ØÖĞ·ÖÅäÒ»¸öµ¥Ôª
*@param£ºmemoryPool : ÄÚ´æ³ØµÄ¾ä±ú
*@param: offset : ·µ»ØÄÚ´æ¿éµØÖ·µÄÆ«ÒÆÁ¿.
*@retval: ³É¹¦ : ·µ»ØµÄÉêÇëÄÚ´æ¿éµØÖ·;   Ê§°Ü : ·µ»ØNULL£
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

	//1. ÅĞ¶ÏÄÚ´æ³ØÊÇ·ñÒÑÂú
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




/*ÄÚ´æ³Ø»ØÊÕÒ»¸ö¶ÔÏóµ¥Ôª
*@param £º memoryPool  ÄÚ´æ³ØµÄ¾ä±ú
*@param :  offset	   »ØÊÕÄÚ´æ¿éµÄÆ«ÒÆÁ¿
*·µ»ØÖµ£º  0 ³É¹¦ £»  -1  Ê§°Ü
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

	//1.ÕÒµ½ÄÚ´æ³ØÖĞ¸ÃÆ«ÒÆÁ¿µÄµØÖ·
	objBlock = memoryPool->buffer_arr + offset;

	//2.Çå¿Õ¸ÃÆ«ÒÆÁ¿, ²¢½«µØÖ··Å»ØÄÚ´æ³ØÖĞ
	memset(objBlock, 0, g_unitMemorySize);
	*(--memoryPool->index_cur) = (char *)objBlock;


End:	
	pthread_mutex_unlock(g_mutex);

	return ret;
}




/*Ïú»ÙÄÚ´æ³Ø
*@param : memoryPool ÄÚ´æ³ØµÄ¾ä±ú
*@retval: 0 ³É¹¦; -1 Ê§°Ü
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

	//1.·ÖÀë¹²ÏíÄÚ´æ
	if (shmdt(memoryPool->buffer_arr) == -1) 
	{
		ret = -1;       
		socket_log(SocketLevel[4], ret, "Err : func shmdt()");
		goto End;
	}

	//2.É¾³ı¹²ÏíÄÚ´æ
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

	//3.ÊÍ·Å¹²ÏíÄÚ´æµÄÊı¾İ½á¹¹
	if(memoryPool->index_arr)		free(memoryPool->index_arr);	
	free(memoryPool);

		
End:	
	pthread_mutex_unlock(g_mutex);	

	return ret;
}



//Çå¿ÕÄÚ´æ³ØÊı¾İ
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

	//1.Çå¿Õ¹²ÏíÄÚ´æ
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


