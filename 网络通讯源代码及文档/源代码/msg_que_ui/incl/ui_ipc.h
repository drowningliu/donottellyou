//ui_ipc.h
#ifndef _UI_IPC_H_
#define _UI_IPC_H_

#if defined(__cplusplus)
extern "C" {
#endif



/*insert cmd callFuncBack type;
*@param : The first : IPC Queue message news; news type : cmd~info~
*@param : The sencond : The handle for create list;  
*/
typedef int (*cmd_callback)(void*);


//链表节点
typedef struct _node{
	int 				cmd;		 //注册的命令字
	int			  		shmid;		 //IPC shmory descriptor
	int  				msqid;		 //IPC Queue descriptor
	cmd_callback  		func;		 //该命令字对应的函数	
	char				arg[512];	 //该命令字对应的参数
	char				*share_pool;	 	 //内存池
	struct _node*		next;
}uiIpcHandle;

#define myprint( x...) do {char buf_new_content_H[1024];\
			sprintf(buf_new_content_H, x);\
	        fprintf(stdout, "%s [%d], [%s]\n", buf_new_content_H, __LINE__, __FILE__ );\
    	}while (0)   


/*init The ipc object : IPC Queue And IPC Shmory
*@param : keyMsg	The Key for IPC Queue
*@param : keyShm	The Key for IPC Shmory
*@param : shmid		The handle for shmory
*@retval: success 0;  fail -1;
*/
int init_ipc_ui(key_t keyMsg, key_t keyShm);


/*The UI send message By IPC Queue
*@param : handle  	The create list Handle
*@param : str  		The new will be send By IPC Queue; style : cmd~data~
*@retval: success 0; fail -1;
*/
int ui_send_fun_mesque( const char *str);


/*The UI recv message By IPC Queue
*@param : str  	recv data from IPC
*@retval: 0 success, -1 fail, -2 The IPC Queue closed; (else, The cmd callBack func return value )
*/
int ui_recv_fun_mesque(char **str);


/* destroy The IPC object : IPC Queue And IPC Shmory
*@retval: success 0; fail -1;
*/
int destroy_ipc_Que_Shm();


/*free The recv Buf from lib.so
*@param : str  	The buf addr
*@retval: success 0; fail -1;
*/
int ui_free_fun_mesque(char *str);	



#if defined(__cplusplus)
}
#endif


#endif


