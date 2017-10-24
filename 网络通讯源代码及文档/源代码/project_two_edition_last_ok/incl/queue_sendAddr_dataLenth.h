 //queue_sendAddr_dataLenth.h 队列头文件

#ifndef _QUEUE_SENDADDR_DATALENTH_
#define _QUEUE_SENDADDR_DATALENTH_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

//typedef void* LinkList;
typedef void* queue_send_dataBLock;

typedef struct dataBlock{
	void *sendDataAddr;			//发送数据包地址
	int  sendDataLenth;			//发送数据包长度
	int  cmd;					//数据包命令字
}dataBlock;

typedef struct sendDataBlock_{
	int head;
	int rear;	
	int capacity;	
	dataBlock *buffer;	//内容数据块
}sendDataBlock;



/*初始化队列
 *参数： myqueue：  初始化队列    capacity：  该队列的容量（字节数）
 *返回值： 成功： 0；  失败： -1；
*/
queue_send_dataBLock queue_init_send_block(int capacity);

/*队列中插入元素
 *参数：  myqueue : 插入的队列
 *		  buffer  : 插入队列内容的地址
 * 返回值： 0 成功；  -1 失败。
*/
int push_queue_send_block(queue_send_dataBLock myqueue, void *sendDataAddr, int sendDataLenth, int cmd);

/* (出队)
 * 参数： myqueue : 插入的队列
 *		  buffer :  插入队列内容的地址
 * 返回值： 成功： 0；  失败： -1；
*/
int pop_queue_send_block(queue_send_dataBLock myqueue, char **sendDataAddr, int *sendDataLenth, int *cmd);

/*获取当前队列的对头元素， 不删除
 *参数：  myqueue : 插入的队列
 *		  buffer :  插入队列内容的地址
 *返回值： 成功： 0；  失败： -1；
*/
int get_queue_send_block(queue_send_dataBLock myqueue, char  **sendDataAddr, int *sendDataLenth, int *cmd);


/*判断队列是否为空
 *参数： 	进行判断的队列
 *返回值：  为真： 空 
 *			为假： 非空     
*/
bool is_empty_send_block(queue_send_dataBLock myqueue);


/*队列是否满队
 *参数：    进行判断的队列
 *返回值：  为真 ： 满对
 *			为假 ： 不满队
*/
bool is_full_send_block(queue_send_dataBLock myqueue);


/*队列中元素的个数
 *返回值： 同上
*/
int get_element_count_send_block(queue_send_dataBLock myqueue);


/*得到空闲队列的元素
 * 返回值 ： 同上。  
*/
int get_free_count_send_block(queue_send_dataBLock myqueue);

//清空队列
int clear_queue_send_block(queue_send_dataBLock myqueue);

//销毁队列
int destroy_queue_send_block(queue_send_dataBLock myqueue);

#ifdef __cplusplus
}
#endif

#endif