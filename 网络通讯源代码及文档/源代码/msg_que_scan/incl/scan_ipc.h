//SCAN_IPC.h 队列头文件

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

//链表节点
typedef struct _node{
	int 				cmd;		 			//注册的命令字
	recv_data_callback  func;		 			//该命令字对应的函数
	char				arg[512];	 			//该命令字对应的参数
	int 				msqid;		 			//消息队列的句柄
	char 				commomFileDir[512]; 	//单张存储文件的目录
	char 				TemplateFileDir[512];	//模板扫描存储文件的目录
	shmory_pool			*shmPool;	 			//IPC共享内存池
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


/*消息队列发送普通消息
*@param : cmd 发送信息的命令字
*@param : datainfo 信息的内容
*@param : handle 发送数据句柄
*/
extern  int  scan_send_msgque(int cmd, char *datainfo, ipcHandle *handle);


/*alloc The IPC shmory mempool block
*@param : shmHandle  The handle for IPC shmory
*@param : offset 	 The block Addr for shmory mempool init addr offset
*@retval: success :  The alloc block addr;  fail : NULL;
*/
void *alloc_shmory_block(ipcHandle *quehandle, unsigned int *offset);


/*向链表中插入 命令字和对应的回调函数
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
 *@retval: 0 成功, -1 失败, -2 The IPC Queue closed; (else, The cmd callBack func return value )
*/
extern int  scan_recv_msgque(ipcHandle *queHandle);



#if defined(__cplusplus)
}
#endif


#endif
