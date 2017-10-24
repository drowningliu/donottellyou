#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdint.h>
#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <setjmp.h>

#include "socklog.h"
#include "server_simple.h"
#include "msg_que_scan.h"
#include "func_compont.h"


#define SERVER_IP		"192.168.1.196"	//服务器IP
#define SERVER_PORT		8009			//服务器端口
#define	KEY				0x1234			//消息队列键值
#define FILEDIR			"/home/lsh/share/NH_1200/soure"			//指定目录
//#define UPLOADDIRPATH	"/home/yyx/work/openssl-FTP-TCP/project/uploadTest"		//上传文件加目录
#define UPLOADDIRPATH	"./uploadTest"		//上传文件加目录
#define RMFILEPATH 		"/home/lsh/share/NH_1200/Common_image"	
#define FILEFORMAT 		".jpg"			//指定格式


typedef void (*sighandler_t)(int);
extern void destroy_server();


sigjmp_buf g_sigEnv;


char *g_ipv4IP = NULL;
int  g_port = 0;



void sigint_cath(int sig)
{
	 myprint("The func sigint_cath() signal : SIGINT");
	 siglongjmp(g_sigEnv, 6);
}


//�������߼� 
int test()
{
	int ret = 0;

	//1. ���л���������ע��
	ret = sigsetjmp(g_sigEnv, 1);
    if(ret > 0)
    {
    	destroy_server();		    	
		return 0;
    }
		
	//2. Init server Ok
	if((ret = init_server(g_ipv4IP, g_port, UPLOADDIRPATH)) < 0)
	{
		printf("err: func init_server() [%d], [%s]\n",__LINE__, __FILE__);
		return ret;		
	}
	

	return ret;	
}



int main(int argc, char **argv)
{
	int ret = 0;
	
	signal(SIGINT, sigint_cath);
	memset(&g_sigEnv, 0, sizeof(sigjmp_buf));

	//1.��ȡ����������, IP, Port
	if(argc == 1)
	{
		g_ipv4IP = SERVER_IP;
		g_port = SERVER_PORT;
	}
	else if(argc == 2)
	{		
		g_ipv4IP = argv[1];
		g_port = SERVER_PORT;		
	}
	else if(argc == 3)
	{
		g_ipv4IP = argv[1];
		g_port = atoi(argv[2]);
	}
	else
	{
		printf("The Enter Style : ./server IP Port \n");
		return -1;
	} 
	
	
	
	ret = test();
	if(ret < 0)
	{
		printf("err: func test() [%d], [%s]\n",__LINE__, __FILE__);
		return ret;
	}
		
	return ret;
}
