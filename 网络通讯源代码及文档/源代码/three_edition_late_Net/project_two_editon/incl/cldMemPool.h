/*
  *
  */

#ifndef _CLD_MEMPOOL_H
#define _CLD_MEMPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define CLD_MEMPOOL_ALIGNMENT   8    /*Byte alignment length*/   //���г����ֽ�
#define CLD_MEMBLOCK_HEAD       (sizeof(struct _cldMemBlock) - sizeof(char))/*Memory block head*/
#define CLD_MEMPOOL_INITNUM    100  /*Init memory allocation unit number*/
#define CLD_MEMPOOL_GROWNUM    150  /*Grow memory allocation unit number*/

/*Memory blocks, each memory block management a large block of memory, 
  *including many allocation unit   �ڴ���е��ڴ�飬ÿ���ڴ������ߴ���С�ڴ棨ÿ��С�ڴ��СΪnSize����  ���������ĳ�ʼ����Ԫ
  */
typedef struct _cldMemBlock{
    int nSize;                  /*The size of the memory block(), in bytes    ���ڴ���� ���䵥Ԫ�Ĵ�С����λ�ֽ� */
    int nFree;                  /*Free allocation unit of Memory blocks  	  �ڴ�� ���еķ��䵥Ԫ*/
    int nFirst;                 /*Currently available unit serial number, starting  0  ��ǰ���õĵ�Ԫ���кţ���0��ʼ*/
    struct _cldMemBlock *next;  /*Points to the next memory block  ָ����һ���ڴ��ڵ��ָ��*/
    char aData[1];              /*Mark allocation unit start position, ���䵥Ԫ��ʼλ�üǺ�*/
}cldMemBlock;

/*Memory pool  �ڴ��*/
typedef struct _cldMemPool{
    int nInitNum;               /*Init memory block num	��ʼ���ڴ�� �з��䵥Ԫ������	*/
    int nUnitSize;              /*allocation unit of Memory blocks size	�ڴ�� ��ÿ�����䵥Ԫ ��С	*/
    int nGrowNum;               /*Grow memory block num  �����ڴ������*/
    int nMemBlockNum;           /*Current Memory block num	��ǰ�ڴ����Ŀ*/
    cldMemBlock *head;			//ָ�� �ڴ��� ����ͷָ��
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
