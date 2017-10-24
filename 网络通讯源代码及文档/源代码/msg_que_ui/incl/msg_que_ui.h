#ifndef _MSG_QUE_UI_H_
#define _MSG_QUE_UI_H_
 

//警告: 禁止使用 1命令字 : 通知通信双方将关闭IPC通信。// 0 己方主动IPC通信, 关闭IPC队列； 


#include "memory_pool.h"
#include "ui_ipc.h"


#define MY_MIN(x, y)	((x) < (y) ? (x) : (y))


/*打开指定的消息队列, 创建内存池
 *@param: key : 消息队列的键值
 *@retval: 0 成功, -1 失败
*/
int ui_open_fun_mesque(key_t key, int *queid);


/*关闭消息队列, 销毁内存池
 *@retval: 0 成功, -1 失败
*/
int ui_close_fun_mesque(int msqid);	



/*消息队列发送消息, 消息队列的类型为 消息句柄ID
 *@param: dataInfor : 将要发送的消息
 *@retval: 0 成功, -1 失败
*/
int ui_send_fun_mesque_include(uiIpcHandle *handle, const char *dataInfor);	


/*接收消息队列的消息
 *@param: str : 接收到的消息, 传递给主调函数
 *@retval: 0 成功, -1 失败， -2 对方关闭IPC 通信
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

//将字符串的内容转换为 16进制
unsigned int hex_conver_dec(char *src);




#endif
