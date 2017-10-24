//IPC_shmory.h 队列头文件

#ifndef _IPC_SHMORY_H_
#define _IPC_SHMORY_H_


#if defined(__cplusplus)
extern "C" {
#endif

#if 1

typedef struct _shmory_pool_
{
	char*  buffer_arr;					//申请的整块大内存	
	char** index_arr;					//指针数组，分别指向每块内存的首地址
	char** index_cur;					//遍历指针，指向当前可分配的内存块
	char** index_end;					//可分配内存块的末尾
	int    shmid;						//共享内存的句柄
}shmory_pool;
#endif

typedef shmory_pool ipcShm;


/*内存池的初始化
*参数：   capacity：  内存池的容量,即内存块的个数
*		  unitSize:   内存块的大小
*返回值： 内存池的对象指针， 创建失败返回NULL
*/
shmory_pool *shmory_pool_init(key_t key, int capacity, int unitSize, pthread_mutex_t *mutex);




/*从内存池中分配一个单元
*@param：memoryPool : 内存池的句柄
*@param: offset : 返回内存块地址的偏移量.
*@retval: 成功 : 返回的申请内存块地址;   失败 : 返回NULL；
*/
void *shmory_pool_alloc(shmory_pool *memoryPool, unsigned int *offset);	



/*内存池回收一个对象单元
*@param ： memoryPool  内存池的句柄
*@param :  offset	   回收内存块的偏移量
*返回值：  0 成功 ；  -1  失败
*/
int shmory_pool_free(shmory_pool *memoryPool, unsigned int offset);	



/*销毁内存池
*@param : memoryPool 内存池的句柄
*@retval: 0 成功; -1 失败
*/
int  shmory_pool_destroy(shmory_pool *memoryPool);


/*清空内存池数据
*@param : memoryPool 内存池的句柄
*@retval: 0 成功; -1 失败
*/
int  shmory_pool_clear(shmory_pool *memoryPool);



#if defined(__cplusplus)
}
#endif


#endif







