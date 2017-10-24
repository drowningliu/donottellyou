 //fifo.h 队列头文件

#ifndef _FIFO_H_
#define _FIFO_H_


#if defined(__cplusplus)
extern "C" {
#endif


#include <stdbool.h>
typedef struct _fifo_type{
	int head;
	int rear;	
	int capacity;	
	char *bufferPtr;		//队列申请的内存空间	
}fifo_type;

/*初始化队列
 *参数： myqueue：  初始化队列    capacity：  该队列的容量（字节数）
 *返回值： 成功： 0；  失败： -1；
*/
fifo_type *fifo_init(int capacity);

/*队列中插入元素
 *参数：  buffer : 要插入的数据内容
 *		  count  : 插入元素的数量（字节数）
 * 返回值： 0 成功；  -1 失败。 当插入的内容大于剩余空间时，返回值 -2；
*/
int push_fifo(fifo_type *myqueue, const char *buffer, const int count);

/* (出队)
 * 参数： buffer : 主调函数分配内存，将取出的内容拷贝进该内存
 *		  maxNum : 将要取队列元素的个数 
 * 返回值： 队列中取出的元素个数
*/
int pop_fifo(fifo_type *myqueue, char *buffer, const int maxNum);


/*判断队列是否为空
 *参数： 	进行判断的队列
 *返回值：  为真： 空 
 *			为假： 非空     
*/
bool is_empty_fifo(fifo_type *myqueue);


/*队列是否满队
 *参数：    进行判断的队列
 *返回值：  为真 ： 满对
 *			为假 ： 不满队
*/
bool is_full_fifo(fifo_type *myqueue);


/*队列中元素的个数
 *返回值： 同上
*/
int get_fifo_element_count(fifo_type *myqueue);


/*得到空闲队列的元素
 * 返回值 ： 同上。  
*/
int get_fifo_free_count(fifo_type *myqueue);

//销毁队列
int destroy_fifo_queue(fifo_type *myqueue);

//清空队列
int clear_fifo(fifo_type *myqueue);


#if defined(__cplusplus)
}
#endif


#endif