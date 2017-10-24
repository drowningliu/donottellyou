//queue.h ����ͷ�ļ�

#ifndef _QUEUE_BUFFER_H_
#define _QUEUE_BUFFER_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
	typedef struct _queue_buffer{
		int head;
		int rear;
		int capacity;
		char **bufferPtr;		//����������ڴ�ռ�	
	}queue_buffer;

	/*��ʼ������
	*������ myqueue��  ��ʼ������    capacity��  �ö��е��������ֽ�����
	*����ֵ�� �ɹ��� 0��  ʧ�ܣ� -1��
	*/
	queue_buffer *queue_buffer_init(int capacity, int size);

	/*�����в���Ԫ��
	*������  myqueue : ����Ķ���
	*		  buffer  : ����������ݵĵ�ַ
	* ����ֵ�� 0 �ɹ���  -1 ʧ�ܡ� -2 ��������
	*/
	int push_buffer_queue(queue_buffer *myqueue, char *buffer);

	/* (����)
	* ������ myqueue : ����Ķ���
	*		  buffer :  ����������ݵĵ�ַ
	* ����ֵ�� �ɹ��� 0��  ʧ�ܣ� -1��
	*/
	int pop_buffer_queue(queue_buffer *myqueue, char *buffer);

	/*��ȡ��ǰ���еĶ�ͷԪ�أ� ��ɾ��
	*������  myqueue : ����Ķ���
	*		  buffer :  ����������ݵĵ�ַ
	*����ֵ�� �ɹ��� 0��  ʧ�ܣ� -1��
	*/
	int get_buffer_queue(queue_buffer *myqueue, char *buffer);


	/*�ж϶����Ƿ�Ϊ��
	*������ 	�����жϵĶ���
	*����ֵ��  Ϊ�棺 ��
	*			Ϊ�٣� �ǿ�
	*/
	bool is_buffer_empty(queue_buffer *myqueue);


	/*�����Ƿ�����
	*������    �����жϵĶ���
	*����ֵ��  Ϊ�� �� ����
	*			Ϊ�� �� ������
	*/
	bool is_buffer_full(queue_buffer *myqueue);


	/*������Ԫ�صĸ���
	*����ֵ�� Ԫ�صĸ���
	*/
	int get_element_buffer_count(queue_buffer *myqueue);


	/*�õ����ж��е�Ԫ��
	* ����ֵ �� Ԫ�صĸ���
	*/
	int get_free_buffer_count(queue_buffer *myqueue);

	//��ն���
	int clear_buffer_queue(queue_buffer *myqueue);

	//���ٶ���
	int destroy_buffer_queue(queue_buffer *myqueue);

#ifdef __cplusplus
}
#endif

#endif