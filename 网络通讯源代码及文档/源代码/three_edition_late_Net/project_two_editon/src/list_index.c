#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "list_index.h"
static pthread_mutex_t	*g_mutex;


/*��������
@param : capacity	��������
@retval: success ����ͷ���;		 fail NULL;
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

/*��ָ��λ�ò���ڵ���Ϣ
@param : pos  �����λ��
*/
int Insert_node_LinkListSendInfo(LListSendInfo *mlist, int pos, int index, char *sendAddr, int sendLenth, int cmd)
{
	int i = 0;
	int newCapacity = 0;
	node *newSpace = NULL;

	pthread_mutex_lock(g_mutex);

	//1.�ж������Ƿ��㹻
	if (mlist->size == mlist->capacity)
	{
		//2. ��չ�ռ�
		newCapacity = mlist->capacity * 2;
		//�������ڴ�	
		newSpace = (node*)malloc(sizeof(node)* newCapacity);
		memcpy(newSpace, mlist->data, sizeof(node)* mlist->capacity);
		free(mlist->data);
		mlist->data = newSpace;
		mlist->capacity = newCapacity;
	}

	//2.�ж�λ���Ƿ�Խ�� λ�ô�0��ʼ,���Խ�磬Ĭ�ϲ��뵽β��
	if (pos < 0 || pos > mlist->size)
	{
		pos = mlist->size;
	}
	pthread_mutex_unlock(g_mutex);

	//3.���Ҳ���λ��
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

/*����ڵ�
@param : list ����ͷ���
@param : ��ڵ��в����Ԫ��
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


/*����ֵ���������еĽڵ�
*@param : list				����
*@param : packageIndex		���������в��ҵĽڵ�Ԫ��(���ݰ����к�)
*@param : compare			�Աȵķ���
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
		if (compare(list->data[i].packIndex, packageIndex))		//�� 0 ��ʶ�ҵ�
		{
			return &(list->data[i]);
		}
	}
	pthread_mutex_unlock(g_mutex);

	return NULL;
}

//ָ��λ��ɾ��
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


//ͷ��ɾ������
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


//β��ɾ������
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


//�����С
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


//��������
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

//�������
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

//������������
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



