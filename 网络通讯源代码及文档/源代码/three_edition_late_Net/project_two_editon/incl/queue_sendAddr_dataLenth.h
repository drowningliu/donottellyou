 //queue_sendAddr_dataLenth.h ����ͷ�ļ�

#ifndef _QUEUE_SENDADDR_DATALENTH_
#define _QUEUE_SENDADDR_DATALENTH_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

//typedef void* LinkList;
typedef void* queue_send_dataBLock;

typedef struct dataBlock{
	void *sendDataAddr;			//�������ݰ���ַ
	int  sendDataLenth;			//�������ݰ�����
	int  cmd;					//���ݰ�������
}dataBlock;

typedef struct sendDataBlock_{
	int head;
	int rear;	
	int capacity;	
	dataBlock *buffer;	//�������ݿ�
}sendDataBlock;



/*��ʼ������
 *������ myqueue��  ��ʼ������    capacity��  �ö��е��������ֽ�����
 *����ֵ�� �ɹ��� 0��  ʧ�ܣ� -1��
*/
queue_send_dataBLock queue_init_send_block(int capacity);

/*�����в���Ԫ��
 *������  myqueue : ����Ķ���
 *		  buffer  : ����������ݵĵ�ַ
 * ����ֵ�� 0 �ɹ���  -1 ʧ�ܡ�
*/
int push_queue_send_block(queue_send_dataBLock myqueue, void *sendDataAddr, int sendDataLenth, int cmd);

/* (����)
 * ������ myqueue : ����Ķ���
 *		  buffer :  ����������ݵĵ�ַ
 * ����ֵ�� �ɹ��� 0��  ʧ�ܣ� -1��
*/
int pop_queue_send_block(queue_send_dataBLock myqueue, char **sendDataAddr, int *sendDataLenth, int *cmd);

/*��ȡ��ǰ���еĶ�ͷԪ�أ� ��ɾ��
 *������  myqueue : ����Ķ���
 *		  buffer :  ����������ݵĵ�ַ
 *����ֵ�� �ɹ��� 0��  ʧ�ܣ� -1��
*/
int get_queue_send_block(queue_send_dataBLock myqueue, char  **sendDataAddr, int *sendDataLenth, int *cmd);


/*�ж϶����Ƿ�Ϊ��
 *������ 	�����жϵĶ���
 *����ֵ��  Ϊ�棺 �� 
 *			Ϊ�٣� �ǿ�     
*/
bool is_empty_send_block(queue_send_dataBLock myqueue);


/*�����Ƿ�����
 *������    �����жϵĶ���
 *����ֵ��  Ϊ�� �� ����
 *			Ϊ�� �� ������
*/
bool is_full_send_block(queue_send_dataBLock myqueue);


/*������Ԫ�صĸ���
 *����ֵ�� ͬ��
*/
int get_element_count_send_block(queue_send_dataBLock myqueue);


/*�õ����ж��е�Ԫ��
 * ����ֵ �� ͬ�ϡ�  
*/
int get_free_count_send_block(queue_send_dataBLock myqueue);

//��ն���
int clear_queue_send_block(queue_send_dataBLock myqueue);

//���ٶ���
int destroy_queue_send_block(queue_send_dataBLock myqueue);

#ifdef __cplusplus
}
#endif

#endif