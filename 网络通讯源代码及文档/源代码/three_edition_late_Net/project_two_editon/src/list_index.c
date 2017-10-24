#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "list_index.h"
static pthread_mutex_t	*g_mutex;


/*创建链表
@param : capacity	链表容量
@retval: success 链表头结点;		 fail NULL;
*/
LListSendInfo* Create_index_listSendInfo(int capacity, pthread_mutex_t *mutex)
{
	LListSendInfo* list = NULL;

	
	if ((list = (LListSendInfo *)malloc(sizeof(LListSendInfo))) == NULL)
	{
		printf("Err : malloc() [%d], [%s]\n", __LINE__, __FILE__);
		return NULL;
	}
	list->size = 0;
	list->capacity = capacity;
	if ((list->data = (node *)malloc(sizeof(node)* capacity)) == NULL)
	{
		free(list);
		printf("Err : malloc() [%d], [%s]\n", __LINE__, __FILE__);
		return NULL;
	}
	g_mutex = mutex;

	return list;
}

/*向指定位置插入节点信息
@param : pos  插入的位置
*/
int Insert_node_LinkListSendInfo(LListSendInfo *mlist, int pos, int index, char *sendAddr, int sendLenth, int cmd)
{
	int i = 0;
	int newCapacity = 0;
	node *newSpace = NULL;

	pthread_mutex_lock(g_mutex);

	//1.判断容量是否足够
	if (mlist->size == mlist->capacity)
	{
		//2. 拓展空间
		newCapacity = mlist->capacity * 2;
		//申请新内存	
		newSpace = (node*)malloc(sizeof(node)* newCapacity);
		memcpy(newSpace, mlist->data, sizeof(node)* mlist->capacity);
		free(mlist->data);
		mlist->data = newSpace;
		mlist->capacity = newCapacity;
	}

	//2.判断位置是否越界 位置从0开始,如果越界，默认插入到尾部
	if (pos < 0 || pos > mlist->size)
	{
		pos = mlist->size;
	}
	pthread_mutex_unlock(g_mutex);

	//3.查找插入位置
	i = mlist->size - 1;
	for (; i >= pos; i--)
	{
		mlist->data[i + 1] = mlist->data[i];
	}
	mlist->data[pos].packIndex = index;
	mlist->data[pos].sendAddr = sendAddr;
	mlist->data[pos].sendLenth = sendLenth;
	mlist->data[pos].cmd = cmd;
	mlist->size += 1;

	return 0;
}

/*插入节点
@param : list 链表头结点
@param : 向节点中插入的元素
@retval: success 0; fail -1;
*/
int Pushback_node_index_listSendInfo(LListSendInfo *list, int index, char *sendAddr, int sendLenth, int cmd)
{

	if (list == NULL)
	{
		printf("Err : list : %p, [%d], [%s]\n", list, __LINE__, __FILE__);
		return -1;
	}


	return Insert_node_LinkListSendInfo(list, list->size, index, sendAddr, sendLenth, cmd);
}


/*根据值查找链表中的节点
*@param : list				链表
*@param : packageIndex		将在链表中查找的节点元素(数据包序列号)
*@param : compare			对比的方法
*@retval: success findNode; fail NULL;
*/
node *FindNode_ByVal_listSendInfo(LListSendInfo *list, int packageIndex, DATA_COMPARE_PACKINDEX compare)
{	
	int i = 0;

	if (list == NULL || list->size == 0)
	{
		printf("Err : list : %p, listSize :%d, [%d], [%s]\n", list, list->size, __LINE__, __FILE__);
		return NULL;
	}

	pthread_mutex_lock(g_mutex);
	for (i = 0; i < list->size; i++)
	{
		if (compare(list->data[i].packIndex, packageIndex))		//非 0 标识找到
		{
			return &(list->data[i]);
		}
	}
	pthread_mutex_unlock(g_mutex);

	return NULL;
}

//指定位置删除
int RemoveByPos_LinkListSendInfo(LListSendInfo *list, int pos, node *rmNode)
{
	
	int i = 0;
	
	pthread_mutex_lock(g_mutex);
	if (pos < 0 || pos > list->size - 1)
	{
		printf("Err : rmNodePos : %d, listSize : %d [%d], [%s]\n",
				pos, list->size, __LINE__, __FILE__);
		return -1;
	}

	*rmNode = list->data[pos];
	for (i = pos; i < list->size - 1; i++)
	{
		list->data[i] = list->data[i + 1];
	}
	list->size -= 1;
	pthread_mutex_unlock(g_mutex);

	return 0;
}


//头部删除操作
int PopFront_LinkListSendInfo(LListSendInfo *list, node *rmNode)
{

	if (list == NULL || list->size == 0 || rmNode == NULL)
	{
		printf("Err : list : %p, listSize : %d, rmNode : %p, [%d], [%s]\n",
			list, list->size, rmNode, __LINE__, __FILE__);
		return -1;
	}
	
	return RemoveByPos_LinkListSendInfo(list, 0, rmNode);
}


//尾部删除操作
int PopBack_LinkListSendInfo(LListSendInfo *list, node *rmNode)
{
	if (list == NULL || list->size == 0 || rmNode == NULL)
	{
		printf("Err : list : %p, listSize : %d, rmNode : %p, [%d], [%s]\n",
			list, list->size, rmNode, __LINE__, __FILE__);
		return -1;
	}

	return RemoveByPos_LinkListSendInfo(list, list->size - 1, rmNode);
}


//链表大小
int Size_LinkListSendInfo(LListSendInfo *list)
{
	
	if (list == NULL)
	{
		printf("Err : list : %p, [%d], [%s]\n", list, __LINE__, __FILE__);
		return -1;
	}
	int size = 0;
		
	pthread_mutex_lock(g_mutex);
	size = list->size;
	pthread_mutex_unlock(g_mutex);

	return size;

}


//销毁链表
int Destroy_LinkListSendInfo(LListSendInfo *list)
{
	if (list == NULL)
	{
		printf("Err : list : %p, [%d], [%s]\n", list, __LINE__,  __FILE__);
		return -1;
	}
	
	pthread_mutex_lock(g_mutex);
	free(list->data);
	free(list);
	pthread_mutex_unlock(g_mutex);

	return 0;
}

//清空链表
int clear_LinkListSendInfo(LListSendInfo *list)
{
	if (list == NULL)
	{
		printf("Err : list : %p, [%d], [%s]\n", list, __LINE__,  __FILE__);
		return -1;
	}

	pthread_mutex_lock(g_mutex);
	list->size = 0;
	pthread_mutex_unlock(g_mutex);

	return 0;
}

//遍历整个链表
void trave_LinkListSendInfo(LListSendInfo *list, DO_NODE_LIST  do_func)
{

	if (list == NULL)
	{
		return;
	}
	
	int i = 0;
	pthread_mutex_lock(g_mutex);
	for (; i < list->size;i++)
	{
		do_func(&(list->data[i]));
	}
	pthread_mutex_unlock(g_mutex);
}



