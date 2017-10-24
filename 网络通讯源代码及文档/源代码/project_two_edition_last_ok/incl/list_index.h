#ifndef LIST_INDEX_H_
#define LIST_INDEX_H_

#include <pthread.h>

//链表节点
typedef struct _node{
	int					packIndex;			//数据包序列号
	char				*sendAddr;			//数据包发送地址
	int					sendLenth;			//数据包长度
	int 				cmd;				//发送数据包的命令字
}node;


//链表
typedef struct _LINKLIST_{
	node	*data;		//数据节点存储
	int		size;		//链表大小
	int		capacity;	//链表容量
}LListSendInfo;



typedef int(*DATA_COMPARE_PACKINDEX)(int, int);
typedef void(*DO_NODE_LIST)(node *);

/*创建链表
@param : capacity	链表容量
@retval: success 链表头结点;		 fail NULL;
*/
LListSendInfo* Create_index_listSendInfo(int capacity, pthread_mutex_t *mutex);

/*向指定位置插入节点信息
@param : pos  插入的位置
*/
int Insert_node_LinkListSendInfo(LListSendInfo *list, int pos, int index, char *sendAddr, int sendLenth, int cmd);

/*插入节点
@param : list 链表头结点
@param : 向节点中插入的元素
@retval: success 0; fail -1;
*/
int Pushback_node_index_listSendInfo(LListSendInfo *list, int index, char *sendAddr, int sendLenth, int cmd);

/*根据值查找链表中的节点
*@param : list				链表
*@param : packageIndex		将在链表中查找的节点元素(数据包序列号)
*@param : compare			对比的方法
*@retval: success findNode; fail NULL;
*/
node *FindNode_ByVal_listSendInfo(LListSendInfo *list, int packageIndex, DATA_COMPARE_PACKINDEX compare);

//指定位置删除
int RemoveByPos_LinkListSendInfo(LListSendInfo *list, int pos, node *rmNode);

//头部删除操作
int PopFront_LinkListSendInfo(LListSendInfo *list, node *rmNode);

//尾部删除操作
int PopBack_LinkListSendInfo(LListSendInfo *list, node *rmNode);

//链表大小
int Size_LinkListSendInfo(LListSendInfo *list);

//遍历整个链表, 对所有节点进行修改
void trave_LinkListSendInfo(LListSendInfo *list, DO_NODE_LIST  do_func);

//清空链表
int clear_LinkListSendInfo(LListSendInfo *list);

//销毁链表
int Destroy_LinkListSendInfo(LListSendInfo *list);

#endif