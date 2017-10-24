#ifndef _MSG_QUE_SCAN_H_
#define _MSG_QUE_SCAN_H_

//����: ��ֹʹ�� 0, 1�����֣�  0 ��������IPCͨ�ţ� 1 �Է��ر�IPCͨ�š�

#if defined(__cplusplus)
extern "C" {
#endif

#include "template_scan_element_type.h"
#include "IPC_shmory.h"
#include "scan_ipc.h"


#define MY_MIN(x, y)	((x) < (y) ? (x) : (y))


/*1.����Ϣ����, 2.��������(����:����ע�ắ���� ��Ӧ��������);
 *@param: key : ��Ϣ���еļ�ֵ
 *@param: fileDir : �����ļ���Ŀ¼
 *retval: 0 �ɹ�, -1 ʧ��
*/
int  scan_open_msgque(key_t key, char *fileDir, char *templateFileDir, listNode **handle );


/*������Ϣʹ ���պ����˳�ѭ��; �ر���Ϣ����, ��������
 *retval: 0 �ɹ�, -1 ʧ��
*/
int  scan_close_msgque(listNode *handle);


/*������Ϣ���е���Ϣ, ִ����ǰע��Ļص�����
 *@param: handle : ����ľ��
 *retval: 0 �ɹ�, -1 ʧ�ܣ� -2 �Է��ر�IPC ͨ��
*/
int  scan_recv_msgque(listNode *handle);

/*�����ļ�; �ú���������: �����ļ� fileName, д�볤��Ϊ picLen ����picData; Ȼ�󽫸��ļ��ľ���·�����͸�UI �߳�
 *ע�� : ���øú�����, ����Ҫ�ٵ���scan_send_msgque()ȥ���� �ļ�·����Ϣ; �ú���create_file_send_filepath()�а����˷�����Ϣ�Ĺ���
 *@param: addr : ��Ҫ�������ļ���
 *@param: picData : Ҫд���ļ�������
 *@param: picLen : д���ļ������ݳ���
 *retval: 0 �ɹ�, -1 ʧ��
*/
int no_tmeplate_sendData(listNode *handle, void *fileName, void *picData, unsigned int picLen);



/*��Ϣ���з�����Ϣ; ����ָ�� cmd ���ݰ�����Ϣdatainfo ��UI ����
*@param: cmd : ���ݰ�������
*@param: datainfo : ���͵���Ϣ
*@param: handle : ��Ϣ���о��
*@retval: �ɹ� 0; ʧ�� -1
*/
int  scan_send_msgque(int cmd, char *datainfo, listNode *handle);


/*�������в��� �����ֺͶ�Ӧ�Ļص�����
 *@param: cmd : ���������������
 *@param: func : �������������ֶ�Ӧ�Ļص�����
 *@param: handle : ��Ϣ���о��
 *@param: arg [in]: ע����ڴ�
 *@param: argLen : �ڴ�ĳ���
 *@param: tmpPool : �����ڴ�صľ��
 *@retval: 0 �ɹ�, -1 ʧ��
*/
int insert_node(listNode *handle, int cmd, recv_data_callback func);


//��uint32 ����ת��Ϊ C�ַ���; Ĭ�ϲ���ʮ����
char* uitoa(unsigned int n, char *s);

//�� �ַ�������תΪ unsigned int ����
unsigned int atoui(const char *str);

//��unsigned int ת��Ϊ 16������ʽ���ַ���,��0x 
char *unsigned_int_to_hex(unsigned int number, char *hex_str );

/*The insert func for free shmory pool  block 
*@param : arg		  The will free block addr offset; The arg style: only the content(remove The cmd~ and behind ~). 
*@param : shmHandle   The handle for IPC shmory
*/
int call_back_free_shmory_block(void *arg, void *queHandle);



#if defined(__cplusplus)
}
#endif



#endif


