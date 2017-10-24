#include"link_list.h"
#include <pthread.h>

static pthread_mutex_t	*g_mutex;

//初始化
LinkList Init_LinkList(pthread_mutex_t	*mutex)
{
	LList* list = (LList*)malloc(sizeof(LList));
	if (list == NULL){
		return NULL;
	}
	list->header.next = NULL;
	list->size = 0;
	g_mutex = mutex;
	
	return list;
}

//指定位置插入
int Insert_LinkList(LinkList list, int pos, LinkNode* data)
{
	int ret = 0;
	pthread_mutex_lock(g_mutex);
	
	if (NULL == list || NULL == data)
	{
		ret = -1;
		goto End;
	}
	
	LList* mlist = (LList*)list;
	if (pos < 0 || pos > mlist->size)
	{
		pos = mlist->size;
	}

#if 0
	//增加以下操作降低插入效率，由用户自觉插入非重复元素
	//判断指针地址是否重复 
	LinkNode* pTest = mlist->header.next;
	while (pTest != NULL){
		if (pTest == data){
			//printf("指针重复!\n");
			ret = -1;
			goto End;
		}
		pTest = pTest->next;
	}
#endif

	//辅助指针变量
	LinkNode* pCurrent = &(mlist->header);
	int i = 0;
	for (; i < pos;i++)
	{
		pCurrent = pCurrent->next;
	}

	//新数据入链表
	data->next = pCurrent->next;
	pCurrent->next = data;
	mlist->size++;

End:
	pthread_mutex_unlock(g_mutex);	
	return ret;
}

//头部插入操作
int PushFront_LinkList(LinkList list, LinkNode* data)
{

	int ret = 0;
	
	if (list == NULL || NULL == data)
	{
		ret = -1;
		goto End;
	}
	
	
	ret = Insert_LinkList(list, 0, data);

End:	

	return ret;
}

//尾部插入操作
int PushBack_LinkList(LinkList list, LinkNode* data)
{
	if (list == NULL || NULL == data)
	{
		return - 1;
	}
	int ret = 0;
	
	LList* mlist = (LList*)list;
	ret = Insert_LinkList(list, mlist->size, data);
	
	return ret;
}

//指定位置删除
int RemoveByPos_LinkList(LinkList list, int pos)
{
	int ret = 0;
	pthread_mutex_lock(g_mutex);	
	
	if (list == NULL)
	{
		ret = -1;
		goto End;
	}
	LList* mlist = (LList*)list;
	if (pos < 0 || pos > mlist->size - 1)
	{
		ret = -1;
		goto End;
	}
	if (mlist->size == 0)
	{
		ret = -1;
		goto End;
	}
	
	//辅助指针变量
	LinkNode* pCurernt = &(mlist->header);
	int i = 0;
	for (; i < pos;i++)
	{
		pCurernt = pCurernt->next;
	}

	//缓存下被删除节点
	LinkNode* pDel = pCurernt->next;
	//待删除节点前驱节点的后继指针指向待删除节点的后继节点
	pCurernt->next = pDel->next;
	mlist->size--;

End:
	pthread_mutex_unlock(g_mutex);
	return ret;
}

//头部位置删除
int PopFront_LinkList(LinkList list)
{
	if (list == NULL)
	{
		return -1;
	}
	LList* mlist = (LList*)list;
	int ret = 0;
	
	if (mlist->size == 0)
	{
		return -1;
	}
	
	ret = RemoveByPos_LinkList(list, 0);
	
	return ret;
}

//尾部位置删除
int  PopBack_LinkList(LinkList list)
{
	if (list == NULL)
	{
		return -1;
	}
	LList* mlist = (LList*)list;
	if (mlist->size == 0)
	{
		return -1;
	}
	
	int ret = 0;
	ret = RemoveByPos_LinkList(list, mlist->size - 1);
	
	return ret;    
}

//根据值删除
int RemoveByVal_LinkList(LinkList list, LinkNode* data, DATA_COMPARE compare)
{
	int ret = 0;
	pthread_mutex_lock(g_mutex);
	
	if (list == NULL || NULL == data || NULL == compare)
	{
		ret = -1;
		goto End;
	}
	
	LList* mlist = (LList*)list;
	if (mlist->size == 0)
	{
		ret = -1;
		goto End;
	}

	//辅助指针变量
	LinkNode* pPrev = &(mlist->header);
	LinkNode* pCurrent = pPrev->next;
	while (pCurrent != NULL)
	{
		if (compare(pCurrent, (char*)data))
		{
			pPrev->next = pCurrent->next;
			mlist->size--;
			break;
		}

		pPrev = pCurrent;
		pCurrent = pPrev->next;
	}
	
End:	
	pthread_mutex_unlock(g_mutex);
	return ret;
}
//链表大小
int Size_LinkList(LinkList list)
{

	
	if (list == NULL)
	{

		return -1;
	}
	LList* mlist = (LList*)list;
	

	return mlist->size;
}

//打印链表
void Print_LinkList(LinkList list, DATA_PRINT print)
{
	pthread_mutex_lock(g_mutex);
	if (list == NULL || NULL == print)
	{
		pthread_mutex_unlock(g_mutex);
		return ;
	}
	
	LList* mlist = (LList*)list;
	//辅助指针变量
	LinkNode* pCurrent = mlist->header.next;
	while (pCurrent != NULL)
	{
		print(pCurrent);
		pCurrent = pCurrent->next;
	}
	
	pthread_mutex_unlock(g_mutex);
}
//销毁链表
int Destroy_LinkList(LinkList list)
{
	pthread_mutex_lock(g_mutex);
	
	if (list == NULL)
	{
		pthread_mutex_unlock(g_mutex);
		return -1;
	}
	
	free(list);
	list = NULL;
	
	
	pthread_mutex_unlock(g_mutex);
	return 0;
}

//清空链表
int Clear_List(LinkList list)
{
	pthread_mutex_lock(g_mutex);
	
	if (list == NULL)
	{
		pthread_mutex_unlock(g_mutex);
		return -1;
	}
	
	LList* mlist = (LList*)list;

	mlist->header.next = NULL;
	mlist->size = 0;
	
	pthread_mutex_unlock(g_mutex);
	return 0;
}

//返回头结点
LinkNode*  Return_Head_LinkNode(LinkList list)
{
	
	
	if(NULL == list)
	{
	
		return NULL;
	}

	LList* mlist = (LList*)list;
	LinkNode* pCurrent = mlist->header.next;
	
	
	return pCurrent;		
}

//返回要删除的节点
void* Return_Delete_Node(LinkList list, LinkNode* data, DATA_COMPARE compare)
{
	pthread_mutex_lock(g_mutex);
	
	if (list == NULL || NULL == data || NULL == compare)
	{
		pthread_mutex_unlock(g_mutex);
		return NULL;
	}
	
	LList* mlist = (LList*)list;
	if (mlist->size == 0)
	{
		pthread_mutex_unlock(g_mutex);
		return NULL;
	}

	//辅助指针变量
	LinkNode* pPrev = &(mlist->header);
	LinkNode* pCurrent = pPrev->next;
	while (pCurrent != NULL)
	{
		if (compare(pCurrent, (char*)data))
		{
			pthread_mutex_unlock(g_mutex);			
			return pCurrent;
		}

		pPrev = pCurrent;
		pCurrent = pPrev->next;
	}
	
	pthread_mutex_unlock(g_mutex);
	return NULL;
}