//queue.h 队列头文件

#ifndef _QUEUE_BUFFER_H_
#define _QUEUE_BUFFER_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
	typedef struct _queue_buffer{
		int head;
		int rear;
		int capacity;
		char **bufferPtr;		//队列申请的内存空间	
	}queue_buffer;

	/*初始化队列
	*参数： myqueue：  初始化队列    capacity：  该队列的容量（字节数）
	*返回值： 成功： 0；  失败： -1；
	*/
	queue_buffer *queue_buffer_init(int capacity, int size);

	/*队列中插入元素
	*参数：  myqueue : 插入的队列
	*		  buffer  : 插入队列内容的地址
	* 返回值： 0 成功；  -1 失败。 -2 队列已满
	*/
	int push_buffer_queue(queue_buffer *myqueue, char *buffer);

	/* (出队)
	* 参数： myqueue : 插入的队列
	*		  buffer :  插入队列内容的地址
	* 返回值： 成功： 0；  失败： -1；
	*/
	int pop_buffer_queue(queue_buffer *myqueue, char *buffer);

	/*获取当前队列的对头元素， 不删除
	*参数：  myqueue : 插入的队列
	*		  buffer :  插入队列内容的地址
	*返回值： 成功： 0；  失败： -1；
	*/
	int get_buffer_queue(queue_buffer *myqueue, char *buffer);


	/*判断队列是否为空
	*参数： 	进行判断的队列
	*返回值：  为真： 空
	*			为假： 非空
	*/
	bool is_buffer_empty(queue_buffer *myqueue);


	/*队列是否满队
	*参数：    进行判断的队列
	*返回值：  为真 ： 满对
	*			为假 ： 不满队
	*/
	bool is_buffer_full(queue_buffer *myqueue);


	/*队列中元素的个数
	*返回值： 元素的个数
	*/
	int get_element_buffer_count(queue_buffer *myqueue);


	/*得到空闲队列的元素
	* 返回值 ： 元素的个数
	*/
	int get_free_buffer_count(queue_buffer *myqueue);

	//清空队列
	int clear_buffer_queue(queue_buffer *myqueue);

	//销毁队列
	int destroy_buffer_queue(queue_buffer *myqueue);

#ifdef __cplusplus
}
#endif

#endif