#ifndef _CLD_MEMPOOL_H
#define _CLD_MEMPOOL_H



#if defined(__cplusplus)
extern "C" {
#endif



typedef void*	mem_pool_t;

/*内存池的初始化
 *参数：  capacity：  内存池的容量
 *		  unitSize:   内存块的大小
 *返回值： 内存池的对象指针， 创建失败返回NULL
*/
mem_pool_t *mem_pool_init(int capacity, int unitSize);


/*从内存池中分配一个单元
 *参数： 内存池的指针
 *返回值： 返回的申请内存块地址；   失败返回NULL；
*/
void *mem_pool_alloc(mem_pool_t *memoryPool);


/*内存池回收一个对象单元
 *参数： memoryPool    内存池的指针
 *		 objBlock 	   待回收的对象单元
 *返回值：  0 成功 ；  -1  失败
*/
int mem_pool_free(mem_pool_t *memoryPool, void *objBlock);

/*销毁内存池
 *参数： 	memoryPool    内存池的指针
 *返回值：  0 成功 ；  -1  失败
*/
int  mem_pool_destroy(mem_pool_t *memoryPool);



#if defined(__cplusplus)
}
#endif

#endif
