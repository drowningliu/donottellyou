//IPC_shmory.h ����ͷ�ļ�

#ifndef _IPC_SHMORY_H_
#define _IPC_SHMORY_H_


#if defined(__cplusplus)
extern "C" {
#endif

#if 1

typedef struct _shmory_pool_
{
	char*  buffer_arr;					//�����������ڴ�	
	char** index_arr;					//ָ�����飬�ֱ�ָ��ÿ���ڴ���׵�ַ
	char** index_cur;					//����ָ�룬ָ��ǰ�ɷ�����ڴ��
	char** index_end;					//�ɷ����ڴ���ĩβ
	int    shmid;						//�����ڴ�ľ��
}shmory_pool;
#endif

typedef shmory_pool ipcShm;


/*�ڴ�صĳ�ʼ��
*������   capacity��  �ڴ�ص�����,���ڴ��ĸ���
*		  unitSize:   �ڴ��Ĵ�С
*����ֵ�� �ڴ�صĶ���ָ�룬 ����ʧ�ܷ���NULL
*/
shmory_pool *shmory_pool_init(key_t key, int capacity, int unitSize, pthread_mutex_t *mutex);




/*���ڴ���з���һ����Ԫ
*@param��memoryPool : �ڴ�صľ��
*@param: offset : �����ڴ���ַ��ƫ����.
*@retval: �ɹ� : ���ص������ڴ���ַ;   ʧ�� : ����NULL��
*/
void *shmory_pool_alloc(shmory_pool *memoryPool, unsigned int *offset);	



/*�ڴ�ػ���һ������Ԫ
*@param �� memoryPool  �ڴ�صľ��
*@param :  offset	   �����ڴ���ƫ����
*����ֵ��  0 �ɹ� ��  -1  ʧ��
*/
int shmory_pool_free(shmory_pool *memoryPool, unsigned int offset);	



/*�����ڴ��
*@param : memoryPool �ڴ�صľ��
*@retval: 0 �ɹ�; -1 ʧ��
*/
int  shmory_pool_destroy(shmory_pool *memoryPool);


/*����ڴ������
*@param : memoryPool �ڴ�صľ��
*@retval: 0 �ɹ�; -1 ʧ��
*/
int  shmory_pool_clear(shmory_pool *memoryPool);



#if defined(__cplusplus)
}
#endif


#endif







