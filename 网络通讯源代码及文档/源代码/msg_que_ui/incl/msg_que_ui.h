#ifndef _MSG_QUE_UI_H_
#define _MSG_QUE_UI_H_
 

//����: ��ֹʹ�� 1������ : ֪ͨͨ��˫�����ر�IPCͨ�š�// 0 ��������IPCͨ��, �ر�IPC���У� 


#include "memory_pool.h"
#include "ui_ipc.h"


#define MY_MIN(x, y)	((x) < (y) ? (x) : (y))


/*��ָ������Ϣ����, �����ڴ��
 *@param: key : ��Ϣ���еļ�ֵ
 *@retval: 0 �ɹ�, -1 ʧ��
*/
int ui_open_fun_mesque(key_t key, int *queid);


/*�ر���Ϣ����, �����ڴ��
 *@retval: 0 �ɹ�, -1 ʧ��
*/
int ui_close_fun_mesque(int msqid);	



/*��Ϣ���з�����Ϣ, ��Ϣ���е�����Ϊ ��Ϣ���ID
 *@param: dataInfor : ��Ҫ���͵���Ϣ
 *@retval: 0 �ɹ�, -1 ʧ��
*/
int ui_send_fun_mesque_include(uiIpcHandle *handle, const char *dataInfor);	


/*������Ϣ���е���Ϣ
 *@param: str : ���յ�����Ϣ, ���ݸ���������
 *@retval: 0 �ɹ�, -1 ʧ�ܣ� -2 �Է��ر�IPC ͨ��
*/
int ui_recv_fun_mesque_include(uiIpcHandle *handle, char *str);	 


/*create The sigal list
*@retval: fail : NULL
*/
uiIpcHandle* create_list();

/*destroy The list
*@param : list   The handle for create list
*@retval: success 0, fail -1;
*/
int destroy_list(uiIpcHandle *list);

/*find The list Node insert cmd
*@param : handle  for The sigal list
*@param : cmd    The find cmd in list handle
*@retval: success The list Node addr,   fial NULL.
*/
uiIpcHandle *find_cmd_func(uiIpcHandle *handle, int cmd);

/*insert The cmd And cmd_call_Back in the list
*@param :  handle 		for The sigal list 
*@param :  cmd  		insert The list Node cmd
*@param :  func 		The call func for cmd 
*@retval:  success 0, fail -1;
*/
int insert_node_include(uiIpcHandle *handle, int cmd, cmd_callback func);	

//���ַ���������ת��Ϊ 16����
unsigned int hex_conver_dec(char *src);




#endif
