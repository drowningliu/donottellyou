#ifndef _MSG_QUE_SCAN_H_
#define _MSG_QUE_SCAN_H_


typedef void (*recv_data_callback)(void *);

//����ڵ�
typedef struct _node{
	int 				cmd;		//ע���������
	recv_data_callback  func;		//�������ֶ�Ӧ�ĺ���
	void*				arg;		//�������ֶ�Ӧ�Ĳ���
	int 				argLen;		//�����ĳ���
	int 				msqid;		//��Ϣ���еľ��
	char 				fileDir[50];//�����ļ���Ŀ¼	
	struct _node*		next;
}listNode;



/*1.����Ϣ����, 2.��������(����:����ע�ắ���� ��Ӧ��������);
 *@param: key : ��Ϣ���еļ�ֵ
 *@param: fileDir : �����ļ���Ŀ¼
 *retval: 0 �ɹ�, -1 ʧ��
*/
int  scan_open_msgque(key_t key, char *fileDir, listNode **handle );

/*������Ϣʹ ���պ����˳�ѭ��; �ر���Ϣ����, ��������
 *retval: 0 �ɹ�, -1 ʧ��
*/
int  scan_close_msgque(listNode *handle);


/*������Ϣ���е���Ϣ, ִ����ǰע��Ļص�����
 *@param: handle : ����ľ��
 *retval: 0 �ɹ�, -1 ʧ��
*/
int  scan_recv_msgque(listNode *handle);

/*�����ļ�;
 *@param: addr : ��Ҫ�������ļ���
 *@param: picData : Ҫд���ļ�������
 *@param: picLen : д���ļ������ݳ���
 *retval: 0 �ɹ�, -1 ʧ��
*/
int create_file_send_filepath(listNode *handle, void *addr, void *picData, unsigned int picLen);

/*�������в��� �����ֺͶ�Ӧ�Ļص�����
 *@param: cmd : ���������������
 *@param: func : �������������ֶ�Ӧ�Ļص�����
 *retval: 0 �ɹ�, -1 ʧ��
*/
int insert_node(listNode *handle, int cmd, recv_data_callback func, void *arg, int argLen);


#endif


