//socketserver.h 服务器头文件

#ifndef _SOCKET_SERVER_H_
#define _SOCKET_SERVER_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define MAX_EVENTS  1024                                   	 //监听上限数
#define BUFLEN 4096
 

//结构体
struct myevent_s {
     int fd;                                                 			 //要监听的文件描述符
     int events;                                             		 //对应的监听事件
     void *arg;                                             		 //泛型参数
     void (*call_back)(int fd, int events, void *arg);       //回调函数
     int status;                                             		//是否在监听:1->在红黑树上(监听), 0->不在(不监听)
     char buf[BUFLEN];
     int len;
     long last_active;                                      		 //记录每次加入红黑树 g_efd 的时间值
 };
 
 int		 g_efd;                                                  	//保存epoll_create返回的文件描述符
 struct 	myevent_s g_events[MAX_EVENTS+1];           //自定义结构体类型数组. +1-->listen fd



 /*
 *	将结构体 myevent_s 成员变量 初始化
 *	ev: 初始化文件描述符的结构体元素
 *	fd:  设置的文件描述符
 *	call_back： 回调函数
 *	arg： 变参，赋值给上述的回调函数
 */
 void eventset(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg);


/* 
*	向 epoll监听的红黑树 添加一个 文件描述符
*  	efd : 设置的文件描述符
*	events: 该套接字事件
*	ev:	文件描述符的结构体元素
*/
 void eventadd(int efd, int events, struct myevent_s *ev);


 /*
 *	 从epoll 监听的 红黑树中删除一个 文件描述符
 *	efd：设置的文件描述符
 *	ev： 文件描述符的结构体元素
 */
 void eventdel(int efd, struct myevent_s *ev);


  /*  
  *	当有文件描述符就绪, epoll返回, 调用该函数 与客户端建立链接 
  *	lfd:  监听客户端连接的文件描述符
  *	events:
  *	arg:
  */
 void acceptconn(int lfd, int events, void *arg);

 /*
 *	接收数据的回调函数
 *	fd:  操作的文件描述符
 *	events:
 *	arg:
 */
 void recvdata(int fd, int events, void *arg);


  /*
  *	发送数据的回调函数
  *	fd:  操作的文件描述符
  *	events:
  *	arg:
  */
 void senddata(int fd, int events, void *arg);

 /*
 *	创建 socket, 初始化lfd，将lfd放入epoll红黑树 
 *	efd: epoll红黑树句柄
 */
 int initlistensocket(int efd);

/*
 *	获取服务器的地址
 *	sock_get_ip： 服务器创建的套接字
 *	addr	：		地址
*/
int  	getAddr(int sock_get_ip,  char *addr);

/*
 *	获取服务器的端口
 *	port: 端口
*/
int  	getPort(int *port);

/*
 *	获取监听的数量
 *	lisnum: 监听的数量
*/
int 	getConNum(int *lisnum);



#if defined(__cplusplus)
}
#endif

#endif