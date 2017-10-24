//queue_int.h 服务器头文件

#ifndef _QUEUE_INT_H_
#define _QUEUE_INT_H_


#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

typedef struct queue_int{
	int head;			//头
	int rear;			//尾	
	int capacity;		//队列容量
	int *bufferPtr;		//队列申请的内存空间	
}queue_int;




/*初始化队列
 *参数： myqueue：  初始化队列    capacity：  该队列的容量（字节数）
 *返回值： 成功： 0；  失败： -1；
*/
queue_int *queue_int_init(int capacity);

/*队列中插入元素
 *参数：  myqueue : 插入的队列
 *		  buffer  : 插入队列内容的地址
 * 返回值： 0 成功；  -1 失败。
*/
int push_int_queue(queue_int *myqueue, int lenth);

/* (出队)
 * 参数： myqueue : 插入的队列
 *		  buffer :  插入队列内容的地址
 * 返回值： 成功： 0；  失败： -1；
*/
int pop_int_queue(queue_int *myqueue , int *lenth);

/*获取当前队列的对头元素， 不删除
 *参数：  myqueue : 插入的队列
 *		  buffer :  插入队列内容的地址
 *返回值： 成功： 0；  失败： -1；
*/
int get_int_queue(queue_int *myqueue, int *buffer);

/*判断队列是否为空
 *参数： 	进行判断的队列
 *返回值：  为真： 空 
 *			为假： 非空     
*/
bool is_int_empty(queue_int *myqueue);


/*队列是否满队
 *参数：    进行判断的队列
 *返回值：  为真 ： 满对
 *			为假 ： 不满队
*/
bool is_int_full(queue_int *myqueue);


/*队列中元素的个数
 *返回值： 同上
*/
int get_int_element_count(queue_int *myqueue);


/*得到空闲队列的元素
 * 返回值 ： 同上。  
*/
int get_int_free_count(queue_int *myqueue);

//销毁队列
int destroy_int_queue(queue_int *myqueue);

//清空队列
int clear_int_queue(queue_int *myqueue);

#ifdef __cplusplus
}
#endif

#endif