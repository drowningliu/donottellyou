/*
  *
  */

#ifndef _CLD_MEMPOOL_H
#define _CLD_MEMPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define CLD_MEMPOOL_ALIGNMENT   8    /*Byte alignment length*/   //队列长度字节
#define CLD_MEMBLOCK_HEAD       (sizeof(struct _cldMemBlock) - sizeof(char))/*Memory block head*/
#define CLD_MEMPOOL_INITNUM    100  /*Init memory allocation unit number*/
#define CLD_MEMPOOL_GROWNUM    150  /*Grow memory allocation unit number*/

/*Memory blocks, each memory block management a large block of memory, 
  *including many allocation unit   内存池中的内存块，每个内存块管理者大量小内存（每个小内存大小为nSize）。  包含大量的初始化单元
  */
typedef struct _cldMemBlock{
    int nSize;                  /*The size of the memory block(), in bytes    该内存块中 分配单元的大小，单位字节 */
    int nFree;                  /*Free allocation unit of Memory blocks  	  内存块 空闲的分配单元*/
    int nFirst;                 /*Currently available unit serial number, starting  0  当前可用的单元序列号，从0开始*/
    struct _cldMemBlock *next;  /*Points to the next memory block  指向下一个内存块节点的指针*/
    char aData[1];              /*Mark allocation unit start position, 分配单元开始位置记号*/
}cldMemBlock;

/*Memory pool  内存池*/
typedef struct _cldMemPool{
    int nInitNum;               /*Init memory block num	初始化内存块 中分配单元的数量	*/
    int nUnitSize;              /*allocation unit of Memory blocks size	内存块 中每个分配单元 大小	*/
    int nGrowNum;               /*Grow memory block num  增长内存块数量*/
    int nMemBlockNum;           /*Current Memory block num	当前内存块数目*/
    cldMemBlock *head;			//指向 内存块的 链表头指针
}cldMemPool;


/*function interface*/

#define CLD_MEMPOOL_INIT(type)  (_cldMemPool_init(sizeof(struct type),CLD_MEMPOOL_INITNUM,CLD_MEMPOOL_GROWNUM))
cldMemPool * _cldMemPool_init(int nUnitSize,int nInitNum,int nGrowNum);
void _cldMemPool_exit(cldMemPool *pMemPool);
void * _cldMem_malloc(cldMemPool * pMemPool);
int cldMem_free(cldMemPool * pMemPool,void * pFree);
void print_MemoryBlockInfoByPointer(cldMemPool *pMemPool, void *ptr);
void Print_MemoryPool_INFO(cldMemPool *pMemPool);



#endif
