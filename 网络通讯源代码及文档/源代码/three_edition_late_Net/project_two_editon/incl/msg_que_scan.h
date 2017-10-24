#ifndef _MSG_QUE_SCAN_H_
#define _MSG_QUE_SCAN_H_


typedef void (*recv_data_callback)(void *);

//链表节点
typedef struct _node{
	int 				cmd;		//注册的命令字
	recv_data_callback  func;		//该命令字对应的函数
	void*				arg;		//该命令字对应的参数
	int 				argLen;		//参数的长度
	int 				msqid;		//消息队列的句柄
	char 				fileDir[50];//保存文件的目录	
	struct _node*		next;
}listNode;



/*1.打开消息队列, 2.创建链表(作用:保存注册函数和 对应的命令字);
 *@param: key : 消息队列的键值
 *@param: fileDir : 创建文件的目录
 *retval: 0 成功, -1 失败
*/
int  scan_open_msgque(key_t key, char *fileDir, listNode **handle );

/*发送消息使 接收函数退出循环; 关闭消息队列, 销毁链表
 *retval: 0 成功, -1 失败
*/
int  scan_close_msgque(listNode *handle);


/*接收消息队列的消息, 执行提前注册的回调函数
 *@param: handle : 链表的句柄
 *retval: 0 成功, -1 失败
*/
int  scan_recv_msgque(listNode *handle);

/*创建文件;
 *@param: addr : 将要创建的文件名
 *@param: picData : 要写入文件的数据
 *@param: picLen : 写入文件的数据长度
 *retval: 0 成功, -1 失败
*/
int create_file_send_filepath(listNode *handle, void *addr, void *picData, unsigned int picLen);

/*向链表中插入 命令字和对应的回调函数
 *@param: cmd : 插入链表的命令字
 *@param: func : 插入链表命令字对应的回调函数
 *retval: 0 成功, -1 失败
*/
int insert_node(listNode *handle, int cmd, recv_data_callback func, void *arg, int argLen);


#endif


