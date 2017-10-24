#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <time.h>
#include <net/if.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>

#include "sockserver.h"
#include "socklog.h"
#include "sock_pack.h"


/*
*	获取服务器地址
*	sock_get_ip: 已创建的监听套接字
*	addr：	服务器的地址
*/
int  	getAddr(int sock_get_ip,  char *addr)
{
	int 	ret = 0 ;
	struct   sockaddr_in *sin;  
	struct   ifreq ifr_ip;     
		 
	memset(&ifr_ip, 0, sizeof(ifr_ip));     
	strncpy(ifr_ip.ifr_name, "eth0", sizeof(ifr_ip.ifr_name) - 1);     

	if( ioctl( sock_get_ip, SIOCGIFADDR, &ifr_ip) < 0 )     
	{    
		ret = -1; 
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "func ioctl() error");
	    goto End;    
	}       
	sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;     
	strcpy(addr, inet_ntoa(sin->sin_addr));         
	  
	Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "serverIP: %s", addr);

End:
	return ret;
}

//获取服务器的端口
int  	getPort(int *port)
{
	int 	ret = 0;
	
	*port = 8009;
	Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "port: %d", *port);
	return ret;
}

//获取监听的数量
int 	getConNum(int *lisnum)
{
	int 	ret = 0;
	
	*lisnum = 100;
	Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "lisnum: %d", *lisnum);
	return ret;	
}

 //eventset(ev, fd, senddata, ev);                     //设置该 fd 对应的回调函数为 senddata
 
 /*将结构体 myevent_s 成员变量 初始化*/
 void eventset(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg)
 {
 	 int  ret = 0;
     ev->fd = fd;
     ev->call_back = call_back;
     ev->events = 0;
     ev->arg = arg;
     ev->status = 0;
     memset(ev->buf, 0, sizeof(ev->buf));
     ev->len = 0;
     ev->last_active = time(NULL);                       //调用eventset函数的时间
 	 Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "func eventset() OK [fd:%d]", ev->fd);
     return;
 }
 
   //      eventadd(g_efd, EPOLLOUT, ev);                      //将fd加入红黑树g_efd中,监听其写事件
 
  /* 向 epoll监听的红黑树 添加一个 文件描述符 */
 void eventadd(int efd, int events, struct myevent_s *ev)
 {
     struct epoll_event epv = {0, {0}};
     int op = 0, ret = 0 ;
     epv.data.ptr = ev;
     epv.events = ev->events = events;       //EPOLLIN 或 EPOLLOUT
 	
     if (ev->status == 0) 					 //已经在红黑树 g_efd 里
     {                                          
         op = EPOLL_CTL_ADD;                 //将其加入红黑树 g_efd, 并将status置1
         ev->status = 1;
     }
 	
 	
     if (epoll_ctl(efd, op, ev->fd, &epv) < 0)                       //实际添加/修改
 	 {
 		  ret = -1;
 		  Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "event add failed [fd=%d], events[%d] ", ev->fd, events); 		  
 	 }        
     else
          Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "event add OK [fd=%d], op=%d, events[%d] ", ev->fd, op, events);
 
     return ;
 }
 
 
 /* 从epoll 监听的 红黑树中删除一个 文件描述符*/
 void eventdel(int efd, struct myevent_s *ev)
 {
     struct epoll_event epv = {EPOLL_CTL_DEL, {0}};
 	 int ret = 0;
     if (ev->status != 1)                                        //不在红黑树上
     {
    	  ret = -1;
    	  Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "ev->status != 1");	
    	  return ;
     } 
 
     epv.data.ptr = ev;
     ev->status = 0;                                             //修改状态
     ev->call_back = NULL;
     ev->arg = NULL;
     epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv);                //从红黑树 efd 上将 ev->fd 摘除
 	 Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "func eventdel() OK, delete[fd = %d]", ev->fd);
 	 printf("111111111\n");
     return ;
 }
 
 
/*  当有文件描述符就绪, epoll返回, 调用该函数 与客户端建立链接 */
void acceptconn(int lfd, int events, void *arg)
{

	struct sockaddr_in cin;
	socklen_t len = sizeof(cin);
	int cfd = 0, i = 0, ret = 0;

	if ((cfd = accept(lfd, (struct sockaddr *)&cin, &len)) == -1) 
	{
		if (errno != EAGAIN && errno != EINTR) 
		{	     
	     	Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], cfd, "func accept() error, [%d]", errno);	
	     	return;
		}
	}

	do {
	 for (i = 0; i < MAX_EVENTS; i++)                                //从全局数组g_events中找一个空闲元素
	 {
	 	 if (g_events[i].status == 0)                                //类似于select中找值为-1的元素
	         break;                                                  //跳出 for
	 }
	    
	 if (i == MAX_EVENTS) 
	 {	  
	 	 ret = -1;   
	     Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "max connect limit[%d]", MAX_EVENTS);	
	     break;                                                      //跳出do while(0) 不执行后续代码
	 }

	 int flag = 0;
	 if ((flag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0) 			 	//将cfd也设置为非阻塞
	 {            
	     ret = -1;
	     Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, " fcntl() nonblocking failed " );	
	     break;
	 }

	 /* 给cfd设置一个 myevent_s 结构体, 回调函数 设置为 recvdata */
	 eventset(&g_events[i], cfd, recvdata, &g_events[i]);   
	 eventadd(g_efd, EPOLLIN, &g_events[i]);                         //将cfd添加到红黑树g_efd中,监听读事件

	} while(0);

	printf("new connect [%s:%d][time:%ld], pos[%d]\n", 
	     inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), g_events[i].last_active, i);
	Socket_Log(__FILE__,  __LINE__,  SocketLevel[2], ret, "new connect [%s:%d][time:%ld], pos[%d]", 
	     inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), g_events[i].last_active, i );	
	return ;
}

 
 /*接收数据的回调函数*/
void recvdata(int fd, int events, void *arg)
{
	struct myevent_s *ev = (struct myevent_s *)arg;
	int len = 0, ret = 0;

again:
	len = recv(fd, ev->buf, sizeof(ev->buf), 0);            //读文件描述符, 数据存入myevent_s成员buf中
	eventdel(g_efd, ev);        //将该节点从红黑树上摘除	
	if (len > 0) 
	{
		 ev->len = len;
		 ev->buf[len] = '\0';                                //手动添加字符串结束标记
		 Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "RecvFd : [%d], RecvDatalenth : [%d], RecvData : [%s]", fd, len, ev->buf);	
		 printf("RecvFd : [%d], RecvDatalenth : [%d], RecvData : [%s], [%d], [%s]\n", fd, len, ev->buf, __LINE__, __FILE__);
		 eventset(ev, fd, senddata, ev);                     //设置该 fd 对应的回调函数为 senddata
		 eventadd(g_efd, EPOLLOUT, ev);                      //将fd加入红黑树g_efd中,监听其写事件
	} 
	else if (len == 0 && errno == EINTR) 
	{
		 close(ev->fd);
		 /* ev-g_events 地址相减得到偏移元素位置 */
		 Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "[fd=%d] pos[%ld], closed", fd, ev-g_events);	
	}
	else if(errno == EAGAIN)
	{
		goto again;		
	}		
	else if(len < 0)
	{
		 close(ev->fd);		
		 Socket_Log(__FILE__,  __LINE__,  SocketLevel[4], ret, "recv[fd=%d] error[%d]", fd, errno);		
	}
    
}
 

 //对发送的数据进行组包
int data_proce(char *buf, int *lenth)
{
	int ret = 0;
	
	packHead head;
	char *tmpbuf = buf;
	char tmp[1024] = { 0 };
	char *str = "123456789321";
	//1.消息头
	head.cmd = 0x11;
	head.subPackOrNo = 0;
	memcpy(head.machineCode, str, strlen(str));
	head.seriaNumber = 1;
	head.packgeNews.totalPackge = 0;
	head.packgeNews.indexPackge = 0;
	//strcpy(tmp, "111"); 
	tmp[0] = 0x02;
	tmp[1] = 0x01;
	head.contentLenth = sizeof(packHead) + strlen(tmp);
	
	//2.消息体
	*tmpbuf++ = 0x7e;
	memcpy(tmpbuf, &head, sizeof(packHead) );
	tmpbuf += sizeof(packHead);
	memcpy(tmpbuf, tmp, strlen(tmp));
	tmpbuf += strlen(tmp);
	memcpy(tmpbuf, &ret, sizeof(int));
	tmpbuf += sizeof(int);
	*tmpbuf = 0x7e;
	*lenth = sizeof(char) * 2 + sizeof(packHead) + strlen(tmp) + sizeof(int);
	
	return ret;
} 
 
/*发送数据的回调函数*/
void senddata(int fd, int events, void *arg)
{
	struct myevent_s *ev = (struct myevent_s *)arg;
	int len, ret = 0, i = 3;

	//strcpy(ev->buf,  "nihao");
	
	ret = data_proce(ev->buf, &(ev->len));
	if(ret)
	{
		socket_log( SocketLevel[4], ret, "Error: func data_proce()");				
	}
	
	//ev->len = strlen(ev->buf);
	//len =  write(fd, ev->buf, ev->len);
	while(i-- > 0)
	{
again:
	len = send(fd, ev->buf, ev->len, 0);            	//直接将数据 回写给客户端。未作处理
	eventdel(g_efd, ev);                                //从红黑树g_efd中移除
	if (len > 0) 
	{		
		 Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "send[fd=%d], [%d]%s", fd, len, ev->buf);	
		 printf("send[fd=%d], [%d]%s, [%d], [%s]\n", fd, len, ev->buf, __LINE__, __FILE__);
		if(i == 1)
		{
			eventset(ev, fd, recvdata, ev);                     //将该fd的 回调函数改为 recvdata
		 	eventadd(g_efd, EPOLLIN, ev);                       //从新添加到红黑树上， 设为监听读事件
		}
	}
	else if(len == 0 && errno == EINTR)
	{
	 	 close(ev->fd);    
	 	 ret = -1;  
		 Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "The client have closed");		
	}
	else if(errno == EAGAIN)
		goto again;
	else 
	{
	 	close(ev->fd);  			//关闭链接   
	 	ret = -1;                                      
	  	Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "func write() error, %d", len);		
	}
	} 
}
 
 
 /*创建 socket, 初始化lfd，将lfd放入epoll红黑树 */
int initlistensocket(int efd)
{
    struct sockaddr_in sin;
 	char   addr[20] = {0};	
    int lfd = 0, port = 0, lisnum = 0, ret = 0;
     	
     	//1.创建一个socket 指定IPv4协议族 TCP协议
	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if(lfd)
	{
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "func socket() OK");
	}		
	else
	{		
		ret = -1;
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "func socket() error");
		goto End;
	}
	 fcntl(lfd, F_SETFL, O_NONBLOCK);                                            //将socket设为非阻塞

	/*2.初始化一个地址结构 */
	bzero(&sin, sizeof(sin));           //将整个结构体清零
	sin.sin_family = AF_INET;                 //选择协议族为IPv4
	
	ret = getAddr(lfd,  addr);  //获取服务器地址
	if(ret)
	{
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "func getAddr() error");
		goto End;		
	}	
	else
	{				
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "func getAddr() OK");
	}
	
	ret = inet_pton(AF_INET, addr, &sin.sin_addr.s_addr);
  	if(ret != 1)
      	{
      		Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "func inet_pton() error");
		goto End;	
      	}	
	else
	{				
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], 0, "func inet_pton() OK");
	}
	
	ret = getPort(&port);		//获取端口
	if(ret)
	{
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "func getPort() error");
		goto End;		
	}	
	else
	{				
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "func getPort() OK");
	}		
	sin.sin_port = htons(port);            

	//3.绑定地址和端口
	ret = bind(lfd, (struct sockaddr *)&sin, sizeof(sin));
	if(ret)
	{
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "func bind() error");
		goto End;		
	}	
	else
	{				
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "func bind() OK");
	}	
			
	//4.监听链接数量
	ret = getConNum(&lisnum);
	if(ret)
	{
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "func getConNum() error");
		goto End;		
	}	
	else
	{				
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "func getConNum() OK");
	}	
	
	//5.监听
	if(listen(lfd, lisnum) == -1)
	{
		ret = -1;
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[4], ret, "func listen() error");		
		goto End;
	}
	else
	{
		Socket_Log(__FILE__,  __LINE__,   SocketLevel[2], ret, "func listen() OK");		
	}

	//6.设置该监听端口文件描述符
	 eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);
	 eventadd(efd, EPOLLIN, &g_events[MAX_EVENTS]);
End:
	 return	ret ;
 }
 