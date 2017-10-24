#ifndef _MSG_QUE_SCAN_H_
#define _MSG_QUE_SCAN_H_

//警告: 禁止使用 0, 1命令字；  0 己方主动IPC通信； 1 对方关闭IPC通信。

#if defined(__cplusplus)
extern "C" {
#endif

#include "template_scan_element_type.h"
#include "IPC_shmory.h"
#include "scan_ipc.h"


#define MY_MIN(x, y)	((x) < (y) ? (x) : (y))


/*1.打开消息队列, 2.创建链表(作用:保存注册函数和 对应的命令字);
 *@param: key : 消息队列的键值
 *@param: fileDir : 创建文件的目录
 *retval: 0 成功, -1 失败
*/
int  scan_open_msgque(key_t key, char *fileDir, char *templateFileDir, listNode **handle );


/*发送消息使 接收函数退出循环; 关闭消息队列, 销毁链表
 *retval: 0 成功, -1 失败
*/
int  scan_close_msgque(listNode *handle);


/*接收消息队列的消息, 执行提前注册的回调函数
 *@param: handle : 链表的句柄
 *retval: 0 成功, -1 失败， -2 对方关闭IPC 通信
*/
int  scan_recv_msgque(listNode *handle);

/*创建文件; 该函数的作用: 创建文件 fileName, 写入长度为 picLen 数据picData; 然后将该文件的绝对路径发送给UI 线程
 *注意 : 调用该函数后, 不需要再调用scan_send_msgque()去发送 文件路径信息; 该函数create_file_send_filepath()中包含了发送信息的功能
 *@param: addr : 将要创建的文件名
 *@param: picData : 要写入文件的数据
 *@param: picLen : 写入文件的数据长度
 *retval: 0 成功, -1 失败
*/
int no_tmeplate_sendData(listNode *handle, void *fileName, void *picData, unsigned int picLen);



/*消息队列发送消息; 发送指定 cmd 数据包的信息datainfo 至UI 进程
*@param: cmd : 数据包命令字
*@param: datainfo : 发送的信息
*@param: handle : 消息队列句柄
*@retval: 成功 0; 失败 -1
*/
int  scan_send_msgque(int cmd, char *datainfo, listNode *handle);


/*向链表中插入 命令字和对应的回调函数
 *@param: cmd : 插入链表的命令字
 *@param: func : 插入链表命令字对应的回调函数
 *@param: handle : 消息队列句柄
 *@param: arg [in]: 注册的内存
 *@param: argLen : 内存的长度
 *@param: tmpPool : 共享内存池的句柄
 *@retval: 0 成功, -1 失败
*/
int insert_node(listNode *handle, int cmd, recv_data_callback func);


//将uint32 类型转换为 C字符串; 默认采用十进制
char* uitoa(unsigned int n, char *s);

//将 字符串内容转为 unsigned int 类型
unsigned int atoui(const char *str);

//将unsigned int 转换为 16进制形式的字符串,无0x 
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


