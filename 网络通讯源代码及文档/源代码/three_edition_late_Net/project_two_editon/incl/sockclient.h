//sockclient.h 服务器头文件

#ifndef _SOCKET_CLIENT_H_
#define _SOCKET_CLIENT_H_


typedef struct  _config_content{
	bool dhcp;
	char loaclIp[20];
	//char dnsServerIp[20];
    char gate[20];
	char serverIp[20];
	int  serverPort;
	char doMainName[40];
	//int  defaultPort;
    bool defaultPort;
}configContent;



extern configContent  conten;

/*
*	li链接服务器
*/
int client_con_server(int *sockfd,  unsigned int wait_seconds);
 
/**
 * deactivate_nonblock - 设置I/O为阻塞模式
 * @fd: 文件描符符
 */
int deactivate_nonblock(int fd);

/**
 * activate_noblock - 设置I/O为非阻塞模式
 * @fd: 文件描符符
 * 返回值：  0 成功； -1 失败。
 */
int activate_nonblock(int fd);


#endif
