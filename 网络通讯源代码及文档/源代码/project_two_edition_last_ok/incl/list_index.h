#ifndef LIST_INDEX_H_
#define LIST_INDEX_H_

#include <pthread.h>

//����ڵ�
typedef struct _node{
	int					packIndex;			//���ݰ����к�
	char				*sendAddr;			//���ݰ����͵�ַ
	int					sendLenth;			//���ݰ�����
	int 				cmd;				//�������ݰ���������
}node;


//����
typedef struct _LINKLIST_{
	node	*data;		//���ݽڵ�洢
	int		size;		//�����С
	int		capacity;	//��������
}LListSendInfo;



typedef int(*DATA_COMPARE_PACKINDEX)(int, int);
typedef void(*DO_NODE_LIST)(node *);

/*��������
@param : capacity	��������
@retval: success ����ͷ���;		 fail NULL;
*/
LListSendInfo* Create_index_listSendInfo(int capacity, pthread_mutex_t *mutex);

/*��ָ��λ�ò���ڵ���Ϣ
@param : pos  �����λ��
*/
int Insert_node_LinkListSendInfo(LListSendInfo *list, int pos, int index, char *sendAddr, int sendLenth, int cmd);

/*����ڵ�
@param : list ����ͷ���
@param : ��ڵ��в����Ԫ��
@retval: success 0; fail -1;
*/
int Pushback_node_index_listSendInfo(LListSendInfo *list, int index, char *sendAddr, int sendLenth, int cmd);

/*����ֵ���������еĽڵ�
*@param : list				����
*@param : packageIndex		���������в��ҵĽڵ�Ԫ��(���ݰ����к�)
*@param : compare			�Աȵķ���
*@retval: success findNode; fail NULL;
*/
node *FindNode_ByVal_listSendInfo(LListSendInfo *list, int packageIndex, DATA_COMPARE_PACKINDEX compare);

//ָ��λ��ɾ��
int RemoveByPos_LinkListSendInfo(LListSendInfo *list, int pos, node *rmNode);

//ͷ��ɾ������
int PopFront_LinkListSendInfo(LListSendInfo *list, node *rmNode);

//β��ɾ������
int PopBack_LinkListSendInfo(LListSendInfo *list, node *rmNode);

//�����С
int Size_LinkListSendInfo(LListSendInfo *list);

//������������, �����нڵ�����޸�
void trave_LinkListSendInfo(LListSendInfo *list, DO_NODE_LIST  do_func);

//�������
int clear_LinkListSendInfo(LListSendInfo *list);

//��������
int Destroy_LinkListSendInfo(LListSendInfo *list);

#endif