#ifndef _SERVER_IPC_PROCESS_H_
#define _SERVER_IPC_PROCESS_H_

#ifdef __cplusplus
extern "C" {
#endif



/*子进程执行逻辑; 处理服务器的业务
 *@param：ip ： 服务器IP地址；
 *@param：port：服务器监听端口
 *@retval:   0 成功；  -1 失败
*/
int child_process_for_server(char *ip, int port, char *uploadDirParh);

/*关闭服务器进程
 *@param：pid : 服务器进程的ID
 *@retval:   0 成功；  -1 失败
*/
int close_server(pid_t pid);


/*初始化： 完成功能： 1.开启服务器的端口监听，当有客户端连接时，开辟线程去执行业务，	2.开始IPC的初始化，创建链表成功，打开消息队列成功；
 *@param：ip ： 服务器IP地址；
 *@param：port：服务器监听端口
 *@param：key： 消息队列的键值；
 *@param：fileDir： 创建文件的目录
 *@param：handle： 链表的句柄
 *@retval:   0 成功；  -1 服务器初始化失败； -2 IPC创建失败，服务器创建成功； -3  创建子线程失败 。
*/
int init_func(char *ip, int port, char *uploadDirParh,  key_t key, char *fileDir, listNode **handle, pid_t *srcPid);





#ifdef __cplusplus
}
#endif


#endif