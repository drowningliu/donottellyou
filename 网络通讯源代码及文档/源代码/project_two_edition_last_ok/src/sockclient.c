 #include<stdio.h>
 #include<stdlib.h>
 #include<string.h>
 #include<strings.h>
 #include<sys/types.h>
 #include<sys/socket.h>
 #include<ctype.h>
 #include<arpa/inet.h>
 #include<unistd.h>
 #include<pthread.h>
 #include<semaphore.h>
 #include<errno.h>
 #include<strings.h>
 #include <fcntl.h>
 #include <sys/socket.h>
 #include <stdbool.h>
 #include <assert.h>
 
 #include "socklog.h"
 #include "dnsAddr.h"
 #include "sockclient.h"
 #include "sock_pack.h"


 configContent   conten;
 
 
//链接服务器
 static int sock_connect(int *tmp_cfd, const int serv_port, const char* serv_ip, unsigned int wait_seconds );
 //超时链接服务器，
 static int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);
 //设置文件描述符 为非阻塞模式
 int activate_nonblock(int fd);
 //设置文件描述符 为阻塞模式
 int deactivate_nonblock(int fd);
 
 
 

 /* 创建套接字， 绑定地址和端口
 * 参数： tmp_cfd:  返回绑定的套接字
 *		serv_port:		服务器port 
 * 		serv_ip:		服务器IP 
 * 返回值： 0    成功
 *		   非0   失败
 */
static int sock_connect(int *tmp_cfd, const int serv_port, const char* serv_ip, unsigned int wait_seconds)
{
	int 	ret = 0;	
	int 	cfd;
	struct  sockaddr_in serv_addr;			//服务器地址 和 IP
	int flag = 1, len = sizeof(int);		//用来设置端口复用
	//struct timeval timeout = { wait_seconds, 0};		//设置 send() and recv() 超时阻塞
	
	//socket_log( SocketLevel[2], ret, "func sock_connect() begin");	
	if(!tmp_cfd || serv_port <= 0 || !serv_ip)
	{
		socket_log( SocketLevel[4], ret, "Error: !tmp_cfd || serv_port <= 0 || !serv_ip");	
		ret = -1;
		goto End;	
	}
	
	
	//1.创建一个socket 指定IPv4协议族 TCP协议
    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if(cfd == -1)
	{
		socket_log( SocketLevel[4], ret, "Error: func socket()");	
		ret = -1;
		goto End;
	}    
  	
  	//2.设置端口复用、 recv和send数据超时
	ret = setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &flag, len);
	if(ret)
	{
		socket_log( SocketLevel[4], ret, "Error: func setsockopt()");				
		goto End;	
	}

    /*2.初始化一个地址结构 */
    bzero(&serv_addr, sizeof(serv_addr));         
    serv_addr.sin_family = AF_INET;                 
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip);   	   //绑定服务器IP
    serv_addr.sin_port = htons(serv_port);                 //绑定服务器port
    
    
   //3.链接指定服务器
   ret = connect_timeout(cfd, &serv_addr, wait_seconds);
   if(ret != 0)
   {
   		socket_log( SocketLevel[4], ret, "Error: func connect_timeout()");		   	
   		goto End;
   }
   else
   		printf("connected OK sockfd : %d, [%d], [%s]\n", cfd, __LINE__, __FILE__);

	*tmp_cfd = cfd;
	
	
End:
	//socket_log( SocketLevel[2], ret, "func sock_connect() end");
	return  ret;
}
 

 //客户端链接服务器
 int client_con_server(int *sockfd, unsigned int wait_seconds)
 {
 	int ret = 0, flag = 0;
 	char transServIp[20] = { 0 };
 	
 	
	//socket_log( SocketLevel[2], ret, "func client_con_server() begin");					
		
	if(*(conten.serverIp)!= 0)		
	{
		//获取 服务器IP 成功；
		ret = sock_connect(sockfd, conten.serverPort, conten.serverIp, wait_seconds );
	 	if(ret)
		{
			socket_log( SocketLevel[4], ret, "Error: the  func sock_connect()");	
			goto End;		
		}
		socket_log( SocketLevel[2], ret, "func sock_connect() OK serverIp : [%s], serverPort : [%d]",conten.serverIp, conten.serverPort );	
	}
	else if(*(conten.serverIp) == 0 )		// 如果指定的 服务器 IP为空， 就查找 DNS 服务器，通过域名链接服务器。
	{
		
		//获取 DNS 服务器解析后的  服务器IP地址
	again:
		if((ret = getDnsIp(conten.doMainName, conten.gate, transServIp)) < 0)
		{
			if(*transServIp == 0 && flag < 3)
		    {
		        usleep(30);
		        flag += 1;
		        goto again;
		    }
			else if(*transServIp == 0 && flag >= 3)
			{
				socket_log( SocketLevel[4], ret, "Error: func getDnsIp() no find domainName");				
				ret = -1;
				goto End;		
			}	    
		}
		else
		{
			//获取 服务器IP 成功；
			ret = sock_connect(sockfd, conten.defaultPort, transServIp, wait_seconds);
		 	if(ret)
			{
				socket_log( SocketLevel[4], ret, "Error: the DNS serverIp func sock_connect()");			
				goto End;
			}
			socket_log( SocketLevel[2], ret, "func sock_connect() OK serverIp : [%s], serverPort : [%d]", transServIp, conten.defaultPort);	
		}		
	}		
	
	
End:
	//socket_log( SocketLevel[2], ret, "func client_con_server() end");		
 	return ret;
 }


/**
 * connect_timeout - connect
 * @fd: 套接字
 * @addr: 要连接的对方地址
 * @wait_seconds: 等待超时秒数，如果为0表示正常模式
 * 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
 */
static int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
	int ret = 0;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	if (wait_seconds > 0)
		activate_nonblock(fd);
	

	ret = connect(fd, (struct sockaddr*)addr, addrlen);
	if (ret < 0 && errno == EINPROGRESS)
	{
		fd_set connect_fdset;
		struct timeval timeout;
		FD_ZERO(&connect_fdset);
		FD_SET(fd, &connect_fdset);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			// 一但连接建立，则套接字就可写  所以connect_fdset放在了写集合中
			ret = select(fd + 1, NULL, &connect_fdset, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);
		if (ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
			socket_log( SocketLevel[4], ret, "Error: func connect() timed_out ");	
			goto End;
		}
		else if (ret < 0)
		{
			socket_log( SocketLevel[4], ret, "Error: func connect() error");	
			goto End;
		}		
		else if (ret == 1)
		{
			/* ret返回为1（表示套接字可写），可能有两种情况，一种是连接建立成功，一种是套接字产生错误，*/
			/* 此时错误信息不会保存至errno变量中，因此，需要调用getsockopt来获取。 */
			int err;
			socklen_t socklen = sizeof(err);
			int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
			if (sockoptret == -1)
			{
				socket_log( SocketLevel[4], sockoptret, "Error: func getsockopt() ");	
				ret = -1;	
				goto End;;
			}
			if (err == 0)
			{
				ret = 0;	
				socket_log( SocketLevel[2], ret, "func connect  () OK");					
			}
			else
			{
				errno = err;
				socket_log( SocketLevel[4], err, "Error: func connect_timeout(), errno: %d ", err);		
				ret = -1; 
				goto End;
			}
		}
	}
	if (wait_seconds > 0)
	{
		ret = deactivate_nonblock(fd);
		if(ret != 0)
		{
			socket_log( SocketLevel[4], ret, "Error: func deactivate_nonblock() ");	
			goto End;	
		}
	}
	
End:
	return ret;
}


/**
 * activate_noblock - 设置I/O为非阻塞模式
 * @fd: 文件描符符
 * 返回值：  0 成功； -1 失败。
 */
int activate_nonblock(int fd)
{
	int ret = 0;
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		ret = flags;
		socket_log( SocketLevel[4], ret, "Error: func activate_nonblock()");				
		goto End;
	}
		
	flags |= O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
	{
		socket_log( SocketLevel[4], ret, "Error: func fcntl()");	
		goto End;
	}
End:
	return ret;
}

/**
 * deactivate_nonblock - 设置I/O为阻塞模式
 * @fd: 文件描符符
 */
int deactivate_nonblock(int fd)
{
	int ret = 0;
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
	{
		ret = flags;
		socket_log( SocketLevel[4], ret, "Error: func deactivate_nonblock()");	
		goto End;
	}

	flags &= ~O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if (ret < 0)
	{
		socket_log( SocketLevel[4], ret, "Error: func fcntl()");			
		goto End;
	}
End:
	return ret;
}
