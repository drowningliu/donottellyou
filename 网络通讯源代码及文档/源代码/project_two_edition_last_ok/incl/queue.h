 //queue.h 队列头文件

#ifndef _QUEUE_H_
#define _QUEUE_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
typedef struct queue{
	int head;
	int rear;	
	int capacity;	
	char **bufferPtr;		//队列申请的内存空间	
}queue;


/*初始化队列
 *参数： myqueue：  初始化队列    capacity：  该队列的容量（字节数）
 *返回值： 成功： 0；  失败： -1；
*/
queue *queue_init(int capacity);

/*队列中插入元素
 *参数：  myqueue : 插入的队列
 *		  buffer  : 插入队列内容的地址
 * 返回值： 0 成功；  -1 失败。
*/
int push_queue(queue *myqueue, char *buffer);

/* (出队)
 * 参数： myqueue : 插入的队列
 *		  buffer :  插入队列内容的地址
 * 返回值： 成功： 0；  失败： -1；
*/
int pop_queue(queue *myqueue, char **buffer);

/*获取当前队列的对头元素， 不删除
 *参数：  myqueue : 插入的队列
 *		  buffer :  插入队列内容的地址
 *返回值： 成功： 0；  失败： -1；
*/
int get_queue(queue *myqueue, char **buffer);


/*判断队列是否为空
 *参数： 	进行判断的队列
 *返回值：  为真： 空 
 *			为假： 非空     
*/
bool is_empty(queue *myqueue);


/*队列是否满队
 *参数：    进行判断的队列
 *返回值：  为真 ： 满对
 *			为假 ： 不满队
*/
bool is_full(queue *myqueue);


/*队列中元素的个数
 *返回值： 同上
*/
int get_element_count(queue *myqueue);


/*得到空闲队列的元素
 * 返回值 ： 同上。  
*/
int get_free_count(queue *myqueue);

//清空队列
int clear_queue(queue *myqueue);

//销毁队列
int destroy_queue(queue *myqueue);

#ifdef __cplusplus
}
#endif

#endif