//SCAN_IPC.h ����ͷ�ļ�

#ifndef _SCAN_IPC_H_
#define _SCAN_IPC_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "IPC_shmory.h"
#include "msg_que_scan.h"


/*insert cmd callFuncBack type;
*@param : The first : IPC Queue message news; news type : cmd~info~
*@param : The sencond : The handle for create list;  
*/
typedef int (*recv_data_callback)(void*, void*);

//����ڵ�
typedef struct _node{
	int 				cmd;		 			//ע���������
	recv_data_callback  func;		 			//�������ֶ�Ӧ�ĺ���
	char				arg[512];	 			//�������ֶ�Ӧ�Ĳ���
	int 				msqid;		 			//��Ϣ���еľ��
	char 				commomFileDir[512]; 	//���Ŵ洢�ļ���Ŀ¼
	char 				TemplateFileDir[512];	//ģ��ɨ��洢�ļ���Ŀ¼
	shmory_pool			*shmPool;	 			//IPC�����ڴ��
	struct _node*		next;
}listNode;

typedef  struct _node  ipcHandle; 




#define myprint( x...) do {char buf_new_content_H[1024];\
			sprintf(buf_new_content_H, x);\
	        fprintf(stdout, "%s [%d], [%s]\n", buf_new_content_H, __LINE__, __FILE__ );\
    	}while (0)   



/*init scan ipc : function : IPC queue And IPC shmory;
*@param : ipcQueKey   The appoint key for IPC queue;
*@param : ipcShmKey	  The appoint key for IPC shmory;
*@param : myList	  The handle for IPC handle
*@param : fileDir	  The dirPath for create file;  The style : home/yyx/samba
*@param : mutex		  The mutex for shmory
*@retval: success 0;  fail -1;
*/
int init_scan_ipc(key_t ipcQueKey,	key_t ipcShmKey, ipcHandle **myList, char *fileDir,  pthread_mutex_t *mutex );


/*No template Scan; The scan's data create a file and send filePath to UI by IPC queue
*@param : quehandle  The handle for IPC queue
*@param : fileName   The will crate file Name;   The style : hello.txt
*@param : picData 	 The content data for create file
*@param : picLen	 The picData Lenth
*@retval: success 0; fail -1;
*/
int create_file_send_filepath(ipcHandle *quehandle, char *fileName, void *picData, unsigned int picLen);


/*tape template sendData : The sendData is block addr for shmory mempool offset;
*@param : quehandle  The handle for IPC queue  
*@param : offset   	 The block Addr for shmory mempool init addr offset
*@retval: sucsuccess 0; fail -1;
*/
int Tape_template_sendData(ipcHandle*queHandle, unsigned int offset);


/*��Ϣ���з�����ͨ��Ϣ
*@param : cmd ������Ϣ��������
*@param : datainfo ��Ϣ������
*@param : handle �������ݾ��
*/
extern  int  scan_send_msgque(int cmd, char *datainfo, ipcHandle *handle);


/*alloc The IPC shmory mempool block
*@param : shmHandle  The handle for IPC shmory
*@param : offset 	 The block Addr for shmory mempool init addr offset
*@retval: success :  The alloc block addr;  fail : NULL;
*/
void *alloc_shmory_block(ipcHandle *quehandle, unsigned int *offset);


/*�������в��� �����ֺͶ�Ӧ�Ļص�����
 *@param : cmd 		   The cmd for inset list 
 *@param : func  	   The insert cmd correspond callback function
 *@param : quehandle   The handle for IPC queue  
 *@param : arg [in]:   insert The memory
 *@param : argLen :    The memory size
 *@param : shmHandle   The handle for IPC shmory
 *@retval: sucsuccess 0; fail -1;
*/
extern int insert_node(ipcHandle *queHandle, int cmd, recv_data_callback func);


/* destroy IPC queue And IPC shmory object
*@param : queHandle    The handle for IPC queue   
*@param : shmHandle	   The handle for IPC shmory
*@retval: sucsuccess 0; fail -1;
*/
int destroy_IPC_QueAndShm(ipcHandle*queHandle);


/*recv news from IPC queue
 *@param : handle    The handle for IPC queue   
 *@retval: 0 �ɹ�, -1 ʧ��, -2 The IPC Queue closed; (else, The cmd callBack func return value )
*/
extern int  scan_recv_msgque(ipcHandle *queHandle);



#if defined(__cplusplus)
}
#endif


#endif
