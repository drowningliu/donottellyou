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
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <setjmp.h>



#include "data_package.h"
#include "data_package_time.h"
#include "socklog.h"
#include "sock_pack.h"
#include "memory_pool.h"
#include "queue.h"
#include "escape_char.h"
#include "fifo.h"
#include "sockclient.h"
#include "link_list.h"
#include "iniparser.h"
#include "dictionary.h"
#include "queue_int.h"
#include "queue_buffer.h"
#include "dhcp.h"
#include "template_scan_element_type.h"
#include "func_compont.h"
#include "queue_sendAddr_dataLenth.h"
#include "list_index.h"

#if 0
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#endif


//宏定义
#define DIVISIONSIGN 			'~'
#define ACCOUTLENTH 			11							//客户端账户长度
#define CAPACITY    			110							//队列, 链表的容量
#define WAIT_SECONDS 			2							//链接服务器的超时时间
#define HEART_TIME				300							//心跳时间
#undef 	BUFSIZE
#define BUFSIZE					250							//命令队列的 节点大小
#define DOWNTEMPLATENAME		"NHi1200_TEMPLATE_NAME"		//The env for downtemplate PATHKEY
#define DOWNTABLENAME           "NHi1200_DATABASE_NAME"
#define DOWNLOADPATH			"NHi1200_TEMPLATE_DIR"
#define CERT_FILE 				"/home/yyx/nanhao/client/client-cert.pem" 
#define KEY_FILE   				"/home/yyx/nanhao/client/client-key.pem" 
#define CA_FILE  				"/home/yyx/nanhao/ca/ca-cert.pem" 
#define CIPHER_LIST	 			"AES128-SHA"
#define	MY_MIN(x, y)			((x) < (y) ? (x) : (y))	
#define TIMEOUTRETURNFLAG		-5							//超时返回值


//内存池, 队列, 链表资源
mem_pool_t  				*g_memPool = NULL;				//内存池
//mem_pool_t  				*g_memPool_list = NULL; 		//内存池
queue       				*g_queueRecv = NULL;			//给UI回复数据存储的队列
queue_send_dataBLock 		g_queue_sendDataBlock = NULL;	//The sendData Addr and dataLenth 存储发送数据和长度的队列
fifo_type					*g_fifo = NULL;					//存储接收线程 数据的队列
queue_buffer 				*g_que_buf = NULL;				//存储接收UI命令的队列
LListSendInfo				*g_list_send_dataInfo = NULL;	//储存所有发送的数据包信息(地址, 长度, 命令字)

//信号量, 用作同步
sem_t 			g_sem_cur_read;		 					//通知给UI 回复消息的信号量
sem_t 			g_sem_cur_write;		 				//通知发送线程的信号量
sem_t 			g_sem_send;								//同步发送线程与解包线程的信号量
sem_t 			g_sem_read_open;						//通知接收线程进行数据接收信号量
sem_t 			g_sem_business;							//通知命令线程的信号量
sem_t 			g_sem_unpack;							//need between In read_thid And unpack_thid;解包数据信号量
sem_t 			g_sem_sequence;							//每次发送必须等上一个命令回应给UI后, 才允许发送下一条接收的UI命令



//互斥锁,锁住全局资源
pthread_mutex_t	 g_mutex_client = PTHREAD_MUTEX_INITIALIZER; 			
pthread_mutex_t	 g_mutex_memPool = PTHREAD_MUTEX_INITIALIZER; 			//内存池锁(内存块为 发送的数据内容)
pthread_mutex_t	 g_mutex_list = PTHREAD_MUTEX_INITIALIZER; 				//链表锁
pthread_mutex_t  g_mutex_serial = PTHREAD_MUTEX_INITIALIZER;			//锁住客户端流水号变化

//全局变量
pthread_t  			g_thid_read = -1, g_thid_unpack = -1, g_thid_business = -1, g_thid_heartTime = -1, g_thid_time = -1,  g_thid_write = -1;	//net The thread ID 
int  				g_close_net = 0;					//0 connect server OK; 1 destroy connect sockfd; 2 active shutdown sockfd
int  				g_sockfd = 0;						//connet the server socket
char 				g_filePath[128] = { 0 };			//配置文件路径
bool 				g_err_flag = false;					//true : 出错; false : 未出错
pid_t 				g_dhcp_process = -1;				//DHCP 进程
uploadWholeSet 		g_upload_set[10];					//上传整套图片集合
uploadWholeSet 		g_upload_error_set[10];				//上传整套图片出错信息集合
char 				*g_downLoadTemplateName = NULL, *g_downLoadTableName = NULL, *g_downLoadPath = NULL;//
int 				g_send_package_cmd = -1;			//current send package cmd
bool  				g_encrypt = false, g_encrypt_data_recv = false;				//true : encrypt Data transfer with sever; false : No encrypt transfer
char 				*g_machine = "21212111233";			//机器码
unsigned int 		g_seriaNumber = 0;					//发送数据包的序列号
unsigned int	 	g_recvSerial = 0;					//当前接收服务器主动发送数据包的序列号
SendDataBlockNews	g_lateSendPackNews;					//最后发送的数据包信息
bool 				g_perIsRecv = false;				//每轮发送的数据包是否已经有回应, false : 无回应, 超时后, 该轮数据包全部重发; true : 有回应, 超时后,只重发最后一条命令  
char 				*g_sendFileContent = NULL;			//store The perRoundAll Src content will send to server	
bool 				g_residuePackNoNeedSend = false;	//ture : 标识剩余的不需要发送, false : 继续剩余数据包的发送, 只在 解包线程和 回应UI 线程使用
bool 				g_modify_config_file = false;		//true : have modify The config file. need set IP; false: not need set IP And don't modify The config file
bool 				g_template_uploadErr = false;		//true : 出现协议性错误, 图片数据不用再上传, false : 正常
bool 				g_subResPonFlag = false;			//true : 本轮数据包,有应答回复(无论应答是什么), false : 本轮数据包, 无任何应答
bool 				g_isSendPackCurrent = false;		//出错后, 判断是否当前是否正在等待接收的发送数据包回复, true : 在等待; false : 未等待
#if 0
SSL_CTX 			*g_sslCtx = NULL;
SSL  				*g_sslHandle = NULL;
#endif

typedef void (*sighandler_t)(int);
//int ssl_init_handle(int socketfd, SSL_CTX **srcCtx, SSL **srcSsl);


//信号捕捉函数
void pipe_func(int sig)
{
	myprint( "\nthread:%lu, SIGPIPIE appear %d", pthread_self(), sig);
}

//信号捕捉函数
void sign_handler(int sign)
{	
	myprint(  "\nthread:%lu, receive signal:%u---", pthread_self(), sign);
	pthread_exit(NULL);
}

//信号捕捉函数
void sign_usr2(int sign)
{
	myprint("\nthread:%lu, receive signal:%u---", pthread_self(), sign);
}

//信号捕捉函数
void sign_child(int sign)
{
	while((waitpid(-1, NULL, WNOHANG)) > 0)
	{
		printf("receive signal : %u, [%d], [%s]",sign, __LINE__, __FILE__);
	}
		   
}

//链表回调函数,查找指定地址的节点
int my_compare_packIndex(int nodeIndex, int packageIndex)
{
	return  nodeIndex == packageIndex;
}


void freeMapToMemPool(node *rmNode)
{
	if((mem_pool_free(g_memPool, rmNode->sendAddr)) < 0)
	{
		myprint("Error : func mem_pool_free() cmd : %d, addr : %p", rmNode->cmd, rmNode->sendAddr);
		assert(0);
	}
	
}


//初始化; filePath : 配置文件路径
int init_client_net(char * filePath)
{
	int ret = 0;
	struct sigaction        actions;
	char cmdBuf[256] = { 0 };
	
	if(filePath == NULL)
	{
		printf( "Error: func init_client_net() filePath == NULL(), [%d],[%s]\n", __LINE__, __FILE__);
		goto End;
	}

	printf( "func init_client() begin, [%d],[%s]\n", __LINE__, __FILE__);
	socket_log( SocketLevel[2], ret, "func init_client() begin");
	
	//1.信号注册
    memset(&actions, 0, sizeof(actions));  
    sigemptyset(&actions.sa_mask); /* 将参数set信号集初始化并清空 */  
    actions.sa_flags = 0;  
    actions.sa_handler = pipe_func;  
    ret = sigaction(SIGPIPE,&actions,NULL);  
	actions.sa_flags = 0;  
	actions.sa_handler = sign_usr2; 
	ret = sigaction(SIGUSR2,&actions,NULL);  
	actions.sa_flags = 0;  
	actions.sa_handler = sign_handler; 
	ret = sigaction(SIGUSR1,&actions,NULL);  
	actions.sa_flags = 0;  
	actions.sa_handler = sign_child; 
	ret = sigaction(SIGCHLD,&actions,NULL);  

	memcpy(g_filePath, filePath, strlen(filePath));
	//2 删除LOG目录下的所有文件
	if((if_file_exist("LOG")) == true)
	{
		sprintf(cmdBuf, "rm %s", "LOG/*");
		if(pox_system(cmdBuf) == 0)
		{
			myprint("Ok, rm LOG/* ...");	
		}
	}

	//2.初始化全局变量
	if((ret = init_global_value()) < 0)
	{
		myprint( "Error: func pthread_create()");
		goto End;
	}
	//socket_log( SocketLevel[2], ret, "func init_global_value() OK");

	//3. 初始化线程
 	if((ret = init_pthread_create()) < 0)
	{
		myprint( "Error: func init_pthread_create()");
		goto End;
	}
	//socket_log( SocketLevel[2], ret, "func init_client() end");

End:
	printf("func init_client() end, [%d],[%s]\n", __LINE__, __FILE__);
	return ret;
}



//设置IP
static int set_local_ip_gate(char *loaclIp, char *gate)
{
	int ret = 0;
	char buf[512] = { 0 };
	//socket_log( SocketLevel[3], ret, " begin func set_local_ip_gate()");
		
	sprintf(buf, "ifconfig eth0 %s",  loaclIp);
	ret = pox_system(buf);
	if(ret < 0 || ret == 127)
	{		
		myprint(  "Error : func system() ifconfig localip, %s ", strerror(errno));
		ret = -1;
		goto End;
	}
	
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "route add default gw %s", gate);
	ret = pox_system(buf );        
    if(ret < 0 )
	{	
		if(errno == EEXIST)
		{
			ret = 0;
		}		
		else
		{
			myprint(  "Error : func system() route add default gw %s ", strerror(errno));
			ret = -1;
			goto End;
		}
	}
	else
	{
		ret = 0;
	}
	
End:	
	//socket_log( SocketLevel[3], ret, " end func set_local_ip_gate()");
	return ret;
}


//初始化全局变量
static int init_global_value()
{
	int ret = 0;
	char *fileName = NULL;
	char *tableName = NULL;

	//1.储存要送数据的地址和数据长度, 使用线程: 组包线程和发送线程
	if((g_queue_sendDataBlock = queue_init_send_block(CAPACITY)) == NULL)
	{
		ret = -1;
		myprint( "Error: func queue_init_send_block()");
		goto End;
	}	
	//2.储存解包后的数据地址, 使用线程: 发送UI主线程和解包线程
	if((g_queueRecv = queue_init(CAPACITY)) == NULL)		
	{
		ret = -1;
		myprint(  "Error: func queue_init()");
		goto End;
	}
	//3.内存池, 内存块为 将要发送给服务器的数据或从服务器接收的数据
	if((g_memPool = mem_pool_init(CAPACITY, PACKMAXLENTH * 2, &g_mutex_memPool)) == NULL)
	{
		ret = -1;
		myprint(  "Error: func mem_pool_init()");
		goto End;
	}
	//4.接收服务器数据的队列,  使用线程: 接收线程和解包线程
	if((g_fifo = fifo_init(CAPACITY * 1024 * 2)) == NULL)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: func fifo_init()");
		goto End;
	}	
	//5.储存UI命令的队列,     使用线程: 主线程和组包线程
	if((g_que_buf = queue_buffer_init(BUFSIZE, BUFSIZE)) == NULL)
	{
		ret = -1;
		myprint( "Error: func queue_buffer_init()");
		goto End;
	}
	//6.创建链表,储存本轮所有发送数据包的信息
	if((g_list_send_dataInfo = Create_index_listSendInfo(PERROUNDNUMBER * 3, &g_mutex_list)) == NULL)
	{
		ret = -1;
		myprint( "Error: func Create_index_listSendInfo()");
		goto End;

	}
	//7.读取配置文件并设置IP 
	if((ret = get_config_file_para()) < 0)
	{
		myprint( "Error: func get_config_file_para()");
		goto End;
	}
#if 0	
	if((ret = chose_setip_method()) < 0)
	{
		myprint( "Error: func chose_setip_method() " );								
		goto End;
	}
#endif	
		
	//8.信号量初始化
	sem_init(&g_sem_cur_read, 0, 0);			
	sem_init(&g_sem_cur_write, 0, 0);			
	sem_init(&g_sem_send, 0, 0);				
	sem_init(&g_sem_read_open, 0, 0);			
	sem_init(&g_sem_business, 0, 0);			
	sem_init(&g_sem_unpack, 0, 0);				
	sem_init(&g_sem_sequence, 0, 1);			


	//9.init The global
	memset(g_upload_error_set, 0, sizeof(g_upload_error_set));
	memset(g_upload_set, 0, sizeof(g_upload_set));

	//10. get The path And Name for downLoad Template And table
	if((g_downLoadTableName = malloc(1024 )) == NULL)			assert(0);
	if((g_downLoadTemplateName = malloc(1024 )) == NULL)		assert(0);
	memset(g_downLoadTableName, 0, 1024);
	memset(g_downLoadTemplateName, 0, 1024);
	int numsaw = PERROUNDNUMBER * COMREQDATABODYLENTH;
	if((g_sendFileContent = malloc(numsaw)) == NULL)			assert(0);
		
	if((fileName = getenv(DOWNTEMPLATENAME)) == NULL)
	{
		myprint( "Error: func getenv() NHi1200_TEMPLATE_NAME == NULL");
		ret = -1;
		assert(0);			
	}
	if((tableName = getenv(DOWNTABLENAME)) == NULL)
	{
		myprint( "Error: func getenv() NHi1200_DATABASE_NAME == NULL");
		ret = -1;
		assert(0);		
	}
	if((g_downLoadPath = getenv(DOWNLOADPATH)) == NULL)
	{
		myprint( "Error: func getenv() NHi1200_TEMPLATE_DIR == NULL");
		ret = -1;
		assert(0);
	}
	sprintf(g_downLoadTableName, "%s%s",g_downLoadPath, tableName);
	sprintf(g_downLoadTemplateName, "%s%s",g_downLoadPath, fileName);
	
End:

	return ret;
}

 
 
 static int init_pthread_create()
 {
	 int ret = 0;
	 
	 //1.创建命令线程
	 if((ret = pthread_create(&g_thid_business, NULL, thid_business, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
 
	 //2.创建接收数据线程
	 if((ret = pthread_create(&g_thid_read, NULL, thid_server_func_read, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create() thid_server_func_read");
		 goto End;
	 }
 
	 //3.创建发送数据线程
	 if((ret = pthread_create(&g_thid_write, NULL, thid_server_func_write, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
	 //4.创建检测超时线程
	 if((ret = pthread_create(&g_thid_time, NULL, thid_server_func_time, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 } 
	 //5.创建解包线程
	 if((ret = pthread_create(&g_thid_unpack, NULL, thid_unpack_thread, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
 
#if 0
	 //6.创建心跳线程
	 if((ret = pthread_create(&g_thid_heartTime, NULL, thid_hreat_beat_time, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
#endif
	 
 End:
 
	 return ret;
}


 //获取配置文件内容
static int get_config_file_para()
{
 	int ret = 0;
 	char *localIp = NULL, *gateTmp = NULL, *ServerIpTmp = NULL, *doMainNameTmp = NULL;
 	dictionary  *ini ;

 	//1.init The config file
 	if((ini = iniparser_load(g_filePath)) == NULL)
 	{
 		ret = -1;
        myprint(  "Error : func iniparser_load() don't open the file g_filePath : %s", g_filePath);
 		goto End;
    }

    //2.get The appoint content
    conten.dhcp = iniparser_getboolean(ini, "local:DHCP", -1);				

    localIp = iniparser_getstring(ini, "local:IP", NULL);				
    memcpy(conten.loaclIp, localIp, strlen(localIp));

    gateTmp = iniparser_getstring(ini, "local:gate", NULL);		
    memcpy(conten.gate, gateTmp, strlen(gateTmp));

    ServerIpTmp = iniparser_getstring(ini, "server:IP", NULL);			
	memcpy(conten.serverIp, ServerIpTmp, strlen(ServerIpTmp));

	conten.serverPort = iniparser_getint(ini, "server:port", -1);			

    doMainNameTmp = iniparser_getstring(ini, "server:DNS", NULL);		
 	memcpy(conten.doMainName, doMainNameTmp, strlen(doMainNameTmp));

 	conten.defaultPort = 8000;

  	iniparser_freedict(ini);

	if(g_sockfd <= 0)
	{
		if((ret = chose_setip_method()) < 0)
		{
			myprint( "Error : func chose_setip_method() ");
			goto End;
		}
	}

 End:
 	return ret;
}


//choose The method to set local IP and gate;
//1. DHCP, 2. manual set IP And gate
int chose_setip_method()
{
	int ret = 0;
	
	if(conten.dhcp)		//根据选项进行设置, DHCP 
	{		
		if(g_dhcp_process > 0)
		{
			kill(g_dhcp_process, SIGKILL);
			g_dhcp_process = -1;
		}
		if((ret = dhcp_interface(&g_dhcp_process)) < 0)
		{
			myprint( "Error : func dhcp_interface() ");
			goto End;
		}
	}
	else				//设置本地IP
	{		
		if((ret = set_local_ip_gate(conten.loaclIp, conten.gate)) < 0)
		{
			myprint(  "Error : func set_local_ip_gate() ");
			goto End;
		}
		if(g_dhcp_process > 0)
		{
			kill(g_dhcp_process, SIGKILL);
			g_dhcp_process = -1;
		}
				
	}

End:
	return ret;
}



//销毁
int destroy_client_net()
{
	int ret = 0;

	//1.修改标识, 标识销毁服务
	g_close_net = 1;					
	sem_post(&g_sem_cur_read);			 
	sleep(1);

	//2.销毁接收数据线程
	if((ret = pthread_kill(g_thid_read, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() recvData Thread");
	}

	//3.销毁发送数据线程
	if((ret = pthread_kill(g_thid_write, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() sendData Thread");
	}

	//4.销毁超时线程
	if((ret = pthread_kill(g_thid_time, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() timeOut Thread");
	}	

	//5.销毁处理命令线程
	if((ret = pthread_kill(g_thid_business, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() cmd Thread");
	}

	//6.销毁解包线程
	if((ret = pthread_kill(g_thid_unpack, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() unpack Thread");
	}
	
#if 0
	if((ret = pthread_kill(g_thid_heartTime, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() heart Thread");
	}
#endif	

	//8.销毁内存池
	if(g_memPool)					mem_pool_destroy(g_memPool);			

	//9.销毁队列: 发送数据, 接收数据, 命令队列, 推送给UI
	if(g_queue_sendDataBlock)		destroy_queue_send_block(g_queue_sendDataBlock);	
	if(g_queueRecv)					destroy_queue(g_queueRecv);				
	if(g_fifo)						destroy_fifo_queue(g_fifo);								
	if(g_que_buf)					destroy_buffer_queue(g_que_buf);

	//10.销毁全局变量: 模板名称和数据库名称
	if(g_downLoadTableName)			free(g_downLoadTableName);
	if(g_downLoadTemplateName)		free(g_downLoadTemplateName);
	
	//11.销毁信号量
	sem_destroy(&g_sem_cur_read);
	sem_destroy(&g_sem_cur_write);
	sem_destroy(&g_sem_send);
	sem_destroy(&g_sem_read_open);
	sem_destroy(&g_sem_business);
	sem_destroy(&g_sem_sequence);
	sem_destroy(&g_sem_unpack);
	
	//12.销毁互斥锁
	pthread_mutex_destroy(&g_mutex_client);
	pthread_mutex_destroy(&g_mutex_list);
	pthread_mutex_destroy(&g_mutex_memPool);
	pthread_mutex_destroy(&g_mutex_serial);
	

	//13.销毁通信套接字
	if(g_sockfd > 0)
	{
		close(g_sockfd);
		g_sockfd = -1;
	}

	//14.销毁dhcp进程
	if(g_dhcp_process > 0)
	{
		kill(g_dhcp_process, SIGKILL);
	}

	return ret;
}



//接收UI 的命令
int  recv_ui_data(const char *news)
{
	int ret = 0, num = 0;
	char cmdBuf[5] = { 0 };	
	int cmd = 0;
	const char *tmpNews = NULL; 
	char storeNews[100] = { 0 };
	
	if(NULL == news)
	{
		ret = -1;
		myprint("Error : news : %p", news);		
		goto End;
	}

	//1. get The cmd  of packet in news.
	sscanf(news, "%[^~]", cmdBuf);
	cmd = atoi(cmdBuf);

	//2.choose the cmd
	if(cmd == 0x0c )
	{
		if((tmpNews = copy_the_ui_cmd_news(news)) == NULL)
		{
			myprint( "Error : func copy_the_ui_cmd_news()");
			goto End;
		}
		sprintf(storeNews, "%d~%p~", 12, tmpNews);
		tmpNews = storeNews;		
	}
	else
	{	
		tmpNews = news;
	}

	//3.将命令存储进队列中
	do{		
		if((ret = push_buffer_queue(g_que_buf, (char *)tmpNews)) == -2)
		{			
			if(num < 3)		
			{
				myprint( "wait : because push_buffer_queue() full");
			}
			else if(num >= 3)
			{
				myprint("Error : push_buffer_queue() full");
			}
			num += 1;
			sleep(1);
		}		
		else if(ret == -1)
		{
			myprint( "Error : push_buffer_queue() ");
			goto End;
		}
		
	}while(ret == -2 && num < 3);		

	//4.通知命令线程, 
	if(ret == 0)
	{	
		//myprint("recv news : %s ", news);
		sem_post(&g_sem_business);		
	}
	
End:
	return ret;
}

//发送信息给UI
int send_ui_data(char **buf)
{
	int ret = 0;
	int index = 1;
	
	if(!buf)
	{
		ret = -1;
		myprint( "Error: buf == NULL");
		goto End;
	}

	//1.进入操作流程
	while(1)
	{
		sem_wait(&g_sem_cur_read);	
		if(g_close_net == 0 || g_close_net == 2)
		{
			//2.处理队列数据,并获取发送给 UI 的数据
			if((ret = data_process(buf)) == -1)
			{
				myprint("Error: func data_process() index : %d", index++);			
				assert(0);			
			}
			else if(ret == 1)
			{
				//socket_log(SocketLevel[2], ret, " *buf : %p, index : %d", *buf, index++);
				continue;
			}
			else if(ret == 0)
			{
				//socket_log(SocketLevel[2], ret, " *buf : %s, addr : %p, index : %d", *buf, *buf, index++);
				break;
			}

		}
	#if 0	
		else if(g_close_net == 1)		//销毁网络服务
		{
			sprintf(g_request_reply_content, "%s", " ");				//给出信息, 随便
			*buf = g_request_reply_content;
			break;
		}
	#endif	
	}
	
	sem_post(&g_sem_sequence);	
	
	//socket_log(SocketLevel[2], ret, "send Data to UI is %s, %p, index : %d", *buf, *buf, index++);

End:
	return ret;
}



//给UI 回复时的数据处理; 0: 需要回复;  -1: 出错;  1: 不需要给UI进行回复
int data_process(char **buf)
{
	int ret = 0;
	char *sendUiData = NULL;				//获取队列中的数据	
	uint8_t cmd = 0;						//数据包命令字
	
	
	//1.取出队列中数据  报头+消息体+校验码
	if((ret = pop_queue(g_queueRecv, &sendUiData)) < 0)		
	{
		myprint("Error: func pop_queue()");
 		assert(0);
	}

	//2.获取命令字
	cmd = *((uint8_t *)sendUiData);

	//3.进行命令字的选择
	switch (cmd){
		case ERRORFLAGMY:
			if((ret = data_un_packge(error_net_reply, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() error_net_reply");
			break;
		case LOGINCMDRESPON:
			if((ret = data_un_packge(login_reply_func, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() login_reply_func");
			break;

		case LOGOUTCMDRESPON:
			if((ret = data_un_packge(exit_reply_program, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() exit_reply_program");
			break;
		case HEARTCMDRESPON:
			if((ret = data_un_packge(heart_beat_reply_program, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() heart_beat_reply_program");
			break;					
		case DOWNFILEZRESPON:
			if((ret = data_un_packge(download_reply_template, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() download_reply_template");
			break;
		case NEWESCMDRESPON:
			if((ret = data_un_packge(get_FileNewestID_reply, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() get_FileNewestID_reply");
			break;
		case UPLOADCMDRESPON:
			if((ret = data_un_packge(upload_picture_reply_request, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() upload_picture_reply_request");
			break;			
		case PUSHINFORESPON:
			if((ret = data_un_packge(push_info_from_server, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() push_info_from_server");
			break;
		case ACTCLOSERESPON:
			if((ret = data_un_packge(active_shutdown_net_reply, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() active_shutdown_net_reply");
			break;
		case DELETECMDRESPON:
			if((ret = data_un_packge(delete_spec_pic_reply, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() delete_spec_pic_reply");
			break;			
		case CONNECTCMDRESPON:
			if((ret = data_un_packge(connet_server_reply, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() connet_server_reply");
			break;
			
		case MODIFYCMDRESPON:
			if((ret = data_un_packge(read_config_file_reply, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() read_config_file_reply");
			break;
		
		case TEMPLATECMDRESPON:
			if((ret = data_un_packge(template_extend_element_reply, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() template_extend_element_reply");
			break;
#if 0				
		case ENCRYCMDRESPON:
			if((ret = data_un_packge(encryp_or_cancelEncrypt_transfer_reply, sendUiData, buf)) < 0)
				myprint("Error: data_un_packge() encryp_or_cancelEncrypt_transfer_reply");
			break;
#endif			
		default:
			myprint("Error : No found The cmd : %d respond", cmd);
			assert(0);
		}

	//4. 释放内存
	if((mem_pool_free(g_memPool, sendUiData)) < 0)
	{
		myprint("Error : func mem_pool_free()");
		assert(0);
	}
	
	return ret;
}

/*call back func; deal with reply package
*@param : news   The reply content from server; style : head + body + checkCode;
*@param : outDataNews reply data to UI;
*@retval: 0: success;  -1: fail; 1: continue And not reply UI
*/
int data_un_packge(call_back_un_pack func_data, char *news, char **outDataNews)
{
	int ret = 0;
	
	if((ret = func_data(news, outDataNews)) < 0)
	{
		ret = -1;
		myprint("Error: func func_data()");
		assert(0);
	}
 
	return ret;
}


//释放内存
int free_memory_net(char *buf)
{
	int ret = 0;
	
	if(buf)
	{
		//sscanf(buf, "%[^~]", tmpBuf);
		if(g_close_net != 1)		//释放内存回内存池
		{
			if((ret = mem_pool_free(g_memPool, buf)) < 0)
			{
				ret = -1;
				myprint("Error: func  mem_pool_alloc() ");		
				assert(0);
			}		
		}
	#if 0	
		else			//清空全局变量信息
		{
			memset(g_request_reply_content, 0, REQUESTCONTENTLENTH);
		}
	#endif	
	}



	return ret;
}



/*copy The news content in block buffer
*@param : news cmd~content~;垄~
*@retval: OK block buffer addr; fail NULL
*/
void *copy_the_ui_cmd_news(const char *news)
{
	int ret = 0;
	unsigned int apprLen = 0;
	char *tmpPool = NULL;
	char *tmp = NULL;
	char bufAddr[20] = { 0 };


	//1.find The flag in news
	if((tmp = strchr(news, '~')) == NULL)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error : func strchr()");
		goto End;
	}
	memcpy(bufAddr, tmp + 1, strlen(tmp) - 2);

	
	//2.alloc The block in mempry pool
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error : NULL == news");
		goto End;
	}


	//3.The buffer content conver 16 hex
	apprLen = hex_conver_dec(bufAddr);
	tmp = NULL + apprLen;	
	memcpy(tmpPool, tmp, sizeof(UitoNetExFrame));
	
End:
	
	return tmpPool;
}


void *thid_hreat_beat_time(void *arg)
{
	int ret = 0 ;
	pthread_detach(pthread_self());

	//进行心跳数据包的发送
	while(1)
	{
		timer_select(HEART_TIME);

		if(!g_isSendPackCurrent)		//当无数据包发送时, 进行数据包的发送
		{
			if((ret = recv_ui_data("3~")) < 0)
			{
				socket_log(SocketLevel[4], ret, "Error: func recv_ui_data()");
				assert(0);	
			}
		}
		myprint("---------- heart Time -----------");
	}

	pthread_exit(NULL); 	
	return NULL;
}




//处理 UI 命令的线程
void *thid_business(void *arg)
{
	int ret = 0, cmd = -1;
	char tmpBuf[4] = { 0 };				//获取命令
	int numberCmd = 1;					//接收命令计数器
	
	pthread_detach(pthread_self());
	
	socket_log( SocketLevel[3], ret, "func thid_business() begin");

	while(1)
	{
		sem_wait(&g_sem_business);			//等待信号量通知	
		sem_wait(&g_sem_sequence);			//进行命令处理控制,必须一条命令处理完,返回结果给UI后, 再处理下一条命令

		char 	cmdBuffer[BUFSIZE] = { 0 };	//取出队列中的命令
		int		num = 0;		
				
		//1.取出 UI 命令信息
		memset(tmpBuf, 0, sizeof(tmpBuf));
		ret = pop_buffer_queue(g_que_buf, cmdBuffer);
		if(ret == -1)
		{			
			socket_log(SocketLevel[4], ret, "Error: func pop_buffer_queue()");
			continue;
		}
		
		printf("\n-------------- number cmd : %d, content : %s\n\n", numberCmd, cmdBuffer);
		//socket_log(SocketLevel[2], ret, "\n---------- number cmd : %d, content : %s", numberCmd, cmdBuffer);
		numberCmd += 1;
		
		//2.获取命令
		sscanf(cmdBuffer, "%[^~]", tmpBuf);
		cmd = atoi(tmpBuf);		
		
		do 
		{				
			//3.进行命令选择
			switch (cmd){
				case 0x01:		//登录		
					if((ret = data_packge(login_func, cmdBuffer)) < 0)			
						socket_log(SocketLevel[4], ret, "Error: data_packge() login_func"); 									
					break;				
				case 0x02:		//登出
					if((ret = data_packge(exit_program, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() exit_program");					
					break;
				case 0x03:		//心跳
					if((ret = data_packge(heart_beat_program, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() heart_beat_program");
					break;					
				case 0x04:		//从服务器下载文件
					if((ret = data_packge(download_file_fromServer, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() download_template");
					break;
				case 0x05:		//获取文件最新ID 
					if((ret = data_packge(get_FileNewestID, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() get_FileNewestID");
					break;	
				
				case 0x06:		//上传图片
					if((ret = data_packge(upload_picture, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() upload_picture");
					break;
#if 1					
				case 0x07:		//消息推送
					if((ret = data_packge(push_info_from_server_reply, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() upload_picture");
					break;
				case 0x08:		//主动关闭网络
					if((ret = data_packge(active_shutdown_net, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() active_shutdown_net");
					break;
				case 0x09:		//删除图片
					if((ret = data_packge(delete_spec_pic, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() delete_spec_pic");
					break;
#endif					
				case 0x0A:		//链接服务器
					if((ret = data_packge(connet_server, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() connet_server");
					break;

				case 0x0B:		//读取配置文件
					if((ret = data_packge(read_config_file, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() read_config_file");
					break;
					
				case 0x0C:		//模板扫描数据上传
					if((ret = data_packge(template_extend_element, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() template_extend_element");
					break;
#if 0					
				case 0x0E:		//加密传输 或者 取消加密传输
					if((ret = data_packge(encryp_or_cancelEncrypt_transfer, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() encryp_or_cancelEncrypt_transfer");
					break;
#endif					
				default :
					myprint("UI send cmd : %d", cmd);
					assert(0);
			}

			if(ret < 0)		
			{
				sleep(1);		//当处理出错后, 
				num += 1;
			}
		}while( 0 );
//		myprint("cmd : %d end...", cmd);	
		
	}

	socket_log( SocketLevel[3], ret, "func thid_business() end");

	return	NULL;
}

//进行函数注册
int  data_packge(call_back_package  func, const char *news)
{
	return func(news);
}


/*发送数据
 *@param: fd : 指定套接字
 *@param: buf : 数据内容
 *@param: lenth: 数据长度
 *@retval : -1: fail  >=0 : The sendLenth
*/
ssize_t writen(int fd, const void *buf, ssize_t lenth)
{
	ssize_t nleft;
    ssize_t nwrite;

    const void *ptr = buf;
    nleft = lenth;

    while(nleft > 0)
    {
        if((nwrite = send(fd, ptr, nleft , 0)) < 0)
        {
        	if(errno == EINTR)
        	{        	
        		continue;
        	}
            else if( nleft == lenth )
            {
                return -1;
            }
            else	//发送数据一半出错
            {
                break;
            }
        }
        else if(nwrite == 0 )
        {
            break;
        }

        ptr += nwrite ;            	
        nleft -= nwrite ;           
    }

    return (lenth - nleft);			
}


int send_data(int sockfd, int *sendCmd)
{
	int 	ret = 0;
	char 	*tmpSend = NULL;		//发送数据的地址
	int 	tmpSendLenth = 0;		//发送数据长度
	int 	nSendLen = 0;			//发送了长度

	
	//1.获取发送的地址和长度
	if(( ret = pop_queue_send_block(g_queue_sendDataBlock, &tmpSend, &tmpSendLenth, sendCmd)) < 0)
	{
		myprint( "Error: func get_queue_send_block()");
		assert(0);
	}
	g_send_package_cmd = *sendCmd;

	//2.判断采取哪种通道发送数据
	if(!g_encrypt)					//No encrypt
	{		
		nSendLen = writen(g_sockfd, tmpSend, tmpSendLenth);		
	}
	else if(g_encrypt)				//encrypt 
	{		
//		nSendLen = SSL_write(g_sslHandle, tmpSend, tmpSendLenth);
	}
	do{
		if(nSendLen == tmpSendLenth)	
		{
			//3.发送成功, 缓存当前发送的数据包信息, (主要缓存最后一包)
			g_lateSendPackNews.cmd = *sendCmd;
			g_lateSendPackNews.sendAddr = tmpSend;
			g_lateSendPackNews.sendLenth = tmpSendLenth;
			time(&(g_lateSendPackNews.sendTime));	

			if(g_send_package_cmd == PUSHINFOREQ)
			{
				remove_senddataserver_addr();				
				sem_post(&g_sem_send);	//服务器推送消息成功应答后,通知进行下一包数据的发送 
				goto End;
			}
		}
		else if(nSendLen == 0 && tmpSendLenth != 0)
		{
			ret = -1;
			myprint( "Error: func writen(),the server has closed ");
			goto End;
		}
		else if(nSendLen < tmpSendLenth)
		{
			ret = -1;
			myprint( "Error: func writen(),have send byte : %d", nSendLen);
			goto End;
		}
		else if(nSendLen < 0)
		{	
			ret = -1;
		}
	}while(0);
	
	
End:
	
	return ret;
}

//删除链表中所有的元素
void remove_senddataserver_addr()
{
	
	trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
	if((clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
	{			
		myprint("Error: func clear_LinkListSendInfo() ");
		assert(0);
	}

}


//接收数据; success 0;  fail -1;
int recv_data(int sockfd)
{
	int ret = 0, recvLenth = 0, number = 0;
	char buf[PACKMAXLENTH * 2] = { 0 };
	static int nRead = 0;
	
	while(1)
	{

		//1.选择接收数据的方式
		if(g_encrypt == false )
		{
			recvLenth = read(g_sockfd, buf, sizeof(buf));
		}
		else if(g_encrypt)
		{
//			recvLenth = SSL_read(g_sslHandle, buf, sizeof(buf));
		}
		nRead += recvLenth;;
		if(recvLenth < 0)		
		{			
			//2.接收数据失败
			if(errno == EINTR || errno == EAGAIN  || errno == EWOULDBLOCK )
			{			
				continue;
			}	
	 		else
	 		{					
				ret = -1;	
				myprint("Err : func recv() from server !!!");
				goto End;
	 		}
		}
		else if(recvLenth == 0)		
		{
			//3.服务器已关闭
			ret = -1;
			myprint("The server has closed !!!");			
			goto End;
		}
		else if(recvLenth > 0)		
		{
			//4.接收数据成功
			do{
				//5.将数据放入队列中
				ret = push_fifo(g_fifo, buf, recvLenth);
				if(ret == -1)
				{
					myprint( "err : func push_fifo()");
					goto End;
				}
				else if(ret == -2)
				{
					number += 1;
					sleep(1);
				}

			}while(ret == -2 && number < 3);
			
			if(ret == -2)
			{
				ret = -1;
				myprint( "err : func push_fifo() full");
				goto End;
			}

			//6.释放信号量, 通知解包线程
			sem_post(&g_sem_unpack);
		
			goto End;				
		}
	}
	
End:
	return ret;
}




/*比较数据包接收是否正确
*@param : tmpContent		数据包转义后的数据
*@param : tmpContentLenth	转义后数据的长度, 包含: 报头 + 信息 + 校验码
*@retval: success 0, repeat : -1, Noreapt : -2
*/
int compare_recvData_Correct(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	int checkCode = 0;					//本地计算校验码
	int *tmpCheckCode = NULL;			//缓存服务器的校验码
	uint8_t cmd = 0;					//接收数据包命令
	resCommonHead_t  *comHead = NULL;	//普通应答报头
	resSubPackHead_t  *subHead = NULL;	//下载服务器文件的报头


	//1.获取接收数据包的命令字
	cmd = *((uint8_t *)tmpContent);

	//2.进行命令字的选择
	if(cmd != DOWNFILEZRESPON)
	{
		//3. 获取接收数据的报头信息
		comHead = (resCommonHead_t *)tmpContent;

		//4. 数据包长度比较
		if(comHead->contentLenth + sizeof(int) == tmpContentLenth)
		{
			if(cmd == LOGINCMDRESPON)	
			{
				g_recvSerial = comHead->seriaNumber;  //第一次接收数据, 获取服务器的流水号储存
			}	

			//5.长度正确, 进行流水号比较, 去除重包			
			if(comHead->seriaNumber >= g_recvSerial)
			{				
				//6.流水号正确,进行校验码比较
				tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));			
				checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
				if(checkCode == *tmpCheckCode)
				{
					//7.校验码正确, 整个数据包正确
					g_recvSerial = comHead->seriaNumber + 1;		//流水号往下移动					
					ret = 0;				
				}
				else
				{
					//8.校验码错误, 数据包需要重发
					myprint( "Error: cmd : %d, contentLenth : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
									cmd , comHead->contentLenth, comHead->seriaNumber, checkCode, *tmpCheckCode);
					ret = -1;
				}		
			}
			else
			{
				//9.流水号出错, 不需要重发
				socket_log(SocketLevel[3], ret, "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u", 
											cmd,  comHead->seriaNumber, g_recvSerial);
				ret = -2;		
			}
		}
		else
		{
			//10.数据长度出错, 不需要重发
			socket_log(SocketLevel[3], ret, "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u", 
						cmd, comHead->contentLenth + sizeof(int), tmpContentLenth, comHead->seriaNumber);
			ret = -2;			
		}
	}
	else	//从服务器下载文件
	{
		subHead = (resSubPackHead_t *)tmpContent;

		//11.数据包长度比较
		if(subHead->contentLenth + sizeof(int) == tmpContentLenth)
		{
			//12.数据包长度正确, 比较流水号, 去除重包
			if(g_recvSerial <= subHead->seriaNumber)
			{
								
				//14.流水号正确,进行校验码比较
				tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
				checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
				if(*tmpCheckCode == checkCode)
				{
					//15.校验码正确, 这个数据包正确
					g_recvSerial = subHead->seriaNumber + 1;		//流水号往下移动					
					ret = 0;
				}
				else
				{
					//16.校验码出错, 需要重发数据包						
					myprint( "Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
									cmd , subHead->seriaNumber, checkCode, *tmpCheckCode);
					ret = -1;
				}
			}
			else
			{
				//17.流水号出错, 不需要重发
				socket_log(SocketLevel[3], ret, "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u", 
											cmd,  subHead->seriaNumber, g_seriaNumber);
				ret = -2;	
			}			
		}
		else
		{
			//18.数据长度出错, 不需要重发
			socket_log(SocketLevel[3], ret, "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u", 
						cmd, subHead->contentLenth + sizeof(int), tmpContentLenth, subHead->seriaNumber);
			ret = -2;
		}
	}


	return ret;
}


/*对重新发送的数据包修改它的流水号
*@param : sendAddr  	 完整的数据包原始地址
*@param : sendLenth  	 数据包长度
*@param : cmd			 发送数据包的命令字
*/
void modify_repeatPackSerialNews(char *sendAddr, int sendLenth, int *newSendLenth)
{
	char *tmp = NULL;
	char tmpPackData[PACKMAXLENTH] = { 0 };		//缓存数据包的内容
	int outLenth = 0;
	reqPackHead_t *tmpHead;
	int  checkCode = 0;				//校验码
	int	 outDataLenth = 0;

	//1. 解转义数据包内容
	if((anti_escape(sendAddr + 1, sendLenth - 2, tmpPackData, &outLenth)) < 0)
	{
		myprint("Error : func anti_escape()");
		assert(0);
	} 
	
	//2.获取数据包的报头信息, 修改其中的流水号
	tmpHead = (reqPackHead_t*)tmpPackData;
	pthread_mutex_lock(&g_mutex_serial);
	tmpHead->seriaNumber = g_seriaNumber++;
	pthread_mutex_unlock(&g_mutex_serial);

	//3.重新计算校验码
	checkCode = crc326((const char *)tmpPackData, tmpHead->contentLenth);
	tmp = tmpPackData + tmpHead->contentLenth;
	memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

	//4.对数据包转义
	if((escape(PACKSIGN, tmpPackData, tmpHead->contentLenth + sizeof(uint32_t), sendAddr + 1, &outDataLenth)) < 0)
	{
		myprint("Error: func escape() ");		 
		assert(0);
	}
	*(sendAddr + outDataLenth + 1) = PACKSIGN;

	//5.将最新的长度返回
	*newSendLenth = outDataLenth + sizeof(char) * 2;	
	
}


/*普通命令的 应答包处理,(此处指: 客户端的一个命令 只对应 服务器的一个应答包)
*@param : tmpContent		数据包转义后的数据
*@param : tmpContentLenth	转义后数据的长度, 包含: 报头 + 信息 + 校验码
*@retval: success 0, 
*/
int commom_respcommand_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	resCommonHead_t  *comHead = NULL;	//普通应答报头
	char *tmpPool = NULL;				//内存块
	
	//1.获取应答的报头信息
	comHead = (resCommonHead_t*)tmpContent;

	//2.判断数据包是否发送过程中, 信息出错
	if(comHead->isFailPack == 1)
	{
		//3.发送过程中, 信息出错, 指校验码信息, 重发该数据包, 相当于最后一包
		repeat_latePack();		
	}
	else
	{
		//5.申请内存
		if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
		{
			ret = -1;
			myprint( "Error: func mem_pool_alloc() ");
			assert(0);
		}
		memcpy(tmpPool, tmpContent, tmpContentLenth);
		
		//6.将内容储存进 队列,通知给UI回复
		if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
		{
			myprint( "Error: func push_queue() ");
			mem_pool_free(g_memPool, tmpPool);
			assert(0);
		}
		sem_post(&g_sem_cur_read);

		//7.将发送内容从链表中清除(因为本函数中, 所有的客户端命令都只对应一个应答包, 所以直接清空链表就可以)
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{			
			myprint("Error: func clear_LinkListSendInfo() ");
			assert(0);
		}

		//8.修改标识符, 标识接收到该数据包的应答
		g_isSendPackCurrent = false;
		
		//9.通知组包线程进行下一个任务
		sem_post(&g_sem_send);
	}


	return ret;
}

int pushInfo_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	//resCommonHead_t  *comHead = NULL;	//普通应答报头
	char *tmpPool = NULL;				//内存块

	//1.申请内存
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}
	memcpy(tmpPool, tmpContent, tmpContentLenth);
	
	//2.将内容储存进 队列,通知给UI回复
	if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
	{
		myprint( "Error: func push_queue() ");
		mem_pool_free(g_memPool, tmpPool);
		assert(0);
	}
	sem_post(&g_sem_cur_read);

	return ret;
}


void reset_template_respon(char *tmpPool, char *tmpContent, int tmpContentLenth)
{
	char *tmpResult = NULL;				//获取数据包的协议返回结果	
	resCommonHead_t  head;				//应答报头
	int contentLenth = 0;	
	char *tmp = tmpPool;
	
	//1. 查看是否为定义的协议出错(内存满, 其他原因等, 非数据包网络传输出错)
	tmpResult = tmpContent + sizeof(resCommonHead_t);
	tmp += sizeof(resCommonHead_t);
	if(*tmpResult == 0x01)
	{
		g_residuePackNoNeedSend = true;		
		*tmp = 0x01;
		contentLenth =  sizeof(resCommonHead_t) + sizeof(char) * 2;
	}
	else
	{
		*tmp = 0x00;
		contentLenth =  sizeof(resCommonHead_t) + sizeof(char);
	}

	//2. 组装报头	
	assign_resComPack_head(&head, TEMPLATECMDRESPON, contentLenth, 0, 1);
	memcpy(tmpPool, &head, sizeof(resCommonHead_t));
	
	
}

/*多张上传图片, 服务器的应答包处理
*@param : tmpContent		数据包转义后的数据
*@param : tmpContentLenth	转义后数据的长度, 包含: 报头 + 信息 + 校验码
*@retval: success 0, 
*/
int mutiUpload_respon_func(char *tmpContent,int tmpContentLenth)
{
	int ret = 0;
	resCommonHead_t  *comHead = NULL;	//普通应答报头
	node *repeatPackage = NULL;			//查找重发数据包的信息
	char *tmpPool = NULL;				//内存池数据块	
	int  newSendLenth = 0; 				//修改流水号后最新的数据包长度

	//1.获取应答的报头信息
	comHead = (resCommonHead_t*)tmpContent;

	//2.判断是否为子包应答
	if(comHead->isSubPack == 1)
	{		
		g_subResPonFlag = true;			//修改标识符, 标识本轮有应答接收 
		//3.子包应答, 判断子包是否为服务器认定失败, 需要重发的数据包
		if(comHead->isFailPack == 1)
		{
			//4.失败,需要重发指定的序号数据包, 去链表中取出该指定数据包
			if((repeatPackage = FindNode_ByVal_listSendInfo(g_list_send_dataInfo, comHead->failPackIndex, my_compare_packIndex)) == NULL)
			{
				//5.未找到需要重新发送的数据包序号				
				socket_log(SocketLevel[4], ret, "Error: func FindNode_ByVal_listSendInfo() ");
				ret = 0;		//不知道此处该如何返回, 不要理他
			}
			else
			{
				//6.找到需要重新发送数据包序号, 先修改流水号信息				
				modify_repeatPackSerialNews(repeatPackage->sendAddr, repeatPackage->sendLenth, &newSendLenth);
				repeatPackage->sendLenth = newSendLenth;				
				if((ret = push_queue_send_block(g_queue_sendDataBlock, repeatPackage->sendAddr, repeatPackage->sendLenth, UPLOADCMDREQ)) < 0)
				{
					//7.信息放入队列失败					
					myprint("Error: func push_queue_send_block(), package News : cmd : %d, lenth : %d, sendAdd : %p ", 
							UPLOADCMDREQ, repeatPackage->sendLenth, repeatPackage->sendAddr);
					assert(0);
				}
				else
				{
					//8.成功放入发送队列, 释放信号量, 通知发送线程进行发送
					sem_post(&g_sem_cur_write);
				}
			}
		}
		else
		{				
			//9.子包接收成功,判断是否需要给UI回应, 这种情况是: 正确接收,但是出现了协议中定义的(内存已满或者其他错误, 需要回应给UI ) 
			if(comHead->isRespondClient == 1)
			{
				//10. 需要给UI回应, 申请内存,拷贝信息, 将信息放入回应队列, 释放信号量 
				if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
				{
					myprint("Err : func mem_pool_alloc() to resPond UI");					
					assert(0);
				}

				//10.1 重新组装应答返回给UI
				reset_template_respon(tmpPool, tmpContent, tmpContentLenth);
								
				if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
				{
					myprint("Err : func push_queue() to resPond UI, cmd : %d", UPLOADCMDRESPON);					
					assert(0);
				}
				else
				{
					sem_post(&g_sem_cur_read);
				}
				//11.修改标识符, 标识接收到该数据包的应答
				g_isSendPackCurrent = false;				
			}
							
			//12.将所有发送内容从链表中清除,
			trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
			if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
			{
				myprint("Error : func clear_LinkListSendInfo()");
				assert(0);
			}
			g_subResPonFlag = false;				//修改标识符至初始化状态, 即无应答接收, 因为此时链表中已无发送元素

			//13. 通知组包线程去进行下一轮的数据组包或者该任务结束
			sem_post(&g_sem_send);
		}
	}
	else
	{		
		//10.总包应答, 不要解析信息, 直接通知组包线程去进行下一轮的数据组包
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{
			myprint("Error : func clear_LinkListSendInfo()");
			assert(0);
		}
		sem_post(&g_sem_send);
	}
	

	return ret;
}


/*上传图片, 服务器的应答包处理
*@param : tmpContent		数据包转义后的数据
*@param : tmpContentLenth	转义后数据的长度, 包含: 报头 + 信息 + 校验码
*@retval: success 0, 
*/
int upload_respon_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	resCommonHead_t  *comHead = NULL;	//普通应答报头
	node *repeatPackage = NULL;			//查找重发数据包的信息
	char *tmpPool = NULL;				//内存池数据块
	char *tmpResult = NULL;				//获取数据包的协议返回结果
	int  newSendLenth = 0; 				//修改流水号后最新的数据包长度

	
	//1.获取应答的报头信息
	comHead = (resCommonHead_t*)tmpContent;

	//2.判断是否为子包应答
	if(comHead->isSubPack == 1)
	{
		g_subResPonFlag = true;			//修改标识符, 标识本轮有应答接收
		//3.子包应答, 判断子包是否为服务器认定失败, 需要重发的数据包
		if(comHead->isFailPack == 1)
		{
			//4.失败,需要重发指定的序号数据包, 去链表中取出该指定数据包
			if((repeatPackage = FindNode_ByVal_listSendInfo(g_list_send_dataInfo, comHead->failPackIndex, my_compare_packIndex)) == NULL)
			{
				//5.未找到需要重新发送的数据包序号				
				socket_log(SocketLevel[4], ret, "Error: func FindNode_ByVal_listSendInfo() ");
				ret = 0;		//不知道此处该如何返回, 不要理他
			}
			else
			{
				//6.找到需要重新发送数据包序号, 先修改流水号信息			
				modify_repeatPackSerialNews(repeatPackage->sendAddr, repeatPackage->sendLenth, &newSendLenth);
				repeatPackage->sendLenth = newSendLenth;				
				if((ret = push_queue_send_block(g_queue_sendDataBlock, repeatPackage->sendAddr, repeatPackage->sendLenth, UPLOADCMDREQ)) < 0)
				{
					//7.信息放入队列失败					
					myprint("Error: func push_queue_send_block(), package News : cmd : %d, lenth : %d, sendAdd : %p ", 
							UPLOADCMDREQ, repeatPackage->sendLenth, repeatPackage->sendAddr);
					assert(0);
				}
				else
				{
					//8.成功放入发送队列, 释放信号量, 通知发送线程进行发送
					sem_post(&g_sem_cur_write);
				}
			}
		}
		else
		{				
			//9.子包接收成功,判断是否需要给UI回应, 这种情况是: 正确接收,但是出现了协议中定义的(内存已满或者其他错误, 需要回应给UI ) 
			if(comHead->isRespondClient == 1)
			{
				//10. 需要给UI回应, 申请内存,拷贝信息, 将信息放入回应队列, 释放信号量 
				if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
				{
					myprint("Err : func mem_pool_alloc() to resPond UI");		
					assert(0);
				}
				memcpy(tmpPool, tmpContent, tmpContentLenth);
				if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
				{
					myprint("Err : func push_queue() to resPond UI, cmd : %d", UPLOADCMDRESPON);					
					assert(0);
				}
				else
				{
					sem_post(&g_sem_cur_read);
				}

				//11.查看是否为定义的协议出错(内存满, 其他原因等, 非数据包网络传输出错)
				tmpResult = tmpContent + sizeof(resCommonHead_t);
				if(*tmpResult == 0x02)
				{
					g_residuePackNoNeedSend = true;
				}
				//11.修改标识符, 标识接收到该数据包的应答
				g_isSendPackCurrent = false;
			
			}

			//12.将所有发送内容从链表中清除,
			trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
			if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
			{
				myprint("Error : func clear_LinkListSendInfo()");
				assert(0);
			}
			g_subResPonFlag = false;				//修改标识符至初始化状态, 即无应答接收, 因为此时链表中已无发送元素

			//13. 通知组包线程去进行下一轮的数据组包或者该任务结束
			sem_post(&g_sem_send);
		}
	}
	else
	{
		//10.总包应答, 不要解析信息, 直接通知组包线程去进行下一轮的数据组包
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{
			myprint("Error : func clear_LinkListSendInfo()");
			assert(0);
		}
		sem_post(&g_sem_send);
	}

	return ret;
}

/*下载服务器文件, 的应答包处理
*@param : tmpContent		数据包转义后的数据
*@param : tmpContentLenth	转义后数据的长度, 包含: 报头 + 信息 + 校验码
*@retval: success 0, 
*/
int downFile_respon_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	resSubPackHead_t  *comHead = NULL;	//下载服务器文件的报头	
	char *tmpPool = NULL;				//内存块
	
	//0. 获取接收数据包信息
	comHead = (resSubPackHead_t *)tmpContent;
		

	//1.申请内存
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}
	memcpy(tmpPool, tmpContent, tmpContentLenth);
	
	//2.将内容储存进 队列,通知给UI回复
	if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
	{
		myprint( "Error: func push_queue() ");
		mem_pool_free(g_memPool, tmpPool);
		assert(0);
	}
	sem_post(&g_sem_cur_read);

	//3. 判断是否成功接收到最后一报送数据
	if(comHead->currentIndex + 1 == comHead->totalPackage)
	{	
		//4.将发送内容从链表中清除(因为本函数中, 所有的客户端命令都只对应一个应答包, 所以直接清空链表就可以)
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{			
			myprint("Error: func clear_LinkListSendInfo() ");
			assert(0);
		}
		//5.通知组包线程进行下一个任务
		sem_post(&g_sem_send);
	}		

	return ret;
}

/*上传模板数据 的应答包处理
*@param : tmpContent		数据包转义后的数据
*@param : tmpContentLenth	转义后数据的长度, 包含: 报头 + 信息 + 校验码
*@retval: success 0, 
*/
int template_respon_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	resCommonHead_t  *comHead = NULL;	//普通应答报头
	char *tmpPool = NULL;				//内存块
	int  newSendLenth = 0;				//修改流水号后最新的数据包长度
	char *tmp = NULL;
	
	//1.获取应答的报头信息, 和模板数据上传的结果
	comHead = (resCommonHead_t*)tmpContent;
	tmp = tmpContent + sizeof(resCommonHead_t);

	//2.判断数据包是否发送过程中, 信息出错
	if(comHead->isFailPack == 1)
	{
		//3.发送过程中, 信息出错, 指校验码信息
		modify_repeatPackSerialNews(g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, &newSendLenth);
		g_lateSendPackNews.sendLenth = newSendLenth;
		if((push_queue_send_block(g_queue_sendDataBlock, g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, g_lateSendPackNews.cmd)) < 0)
		{
			myprint("Error : func push_queue_send_block()");
			assert(0);
		}
		//4.释放信号量,通知发送数据线程继续发送
		sem_post(&g_sem_cur_write);
	}
	else
	{
		if(*tmp == 0)
		{
			//模板数据上传成功, 此时不需要给 UI返回信息, 等待图片上传完成后, 再一起返回
			sem_post(&g_sem_send);
			return ret;
		}
		//5.上传出现协议性错误, 需要给UI 返回
		if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
		{
			myprint( "Error: func mem_pool_alloc() ");
			assert(0);
		}
		memcpy(tmpPool, tmpContent, tmpContentLenth);
		
		//6.将内容储存进 队列,通知给UI回复
		if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
		{
			myprint( "Error: func push_queue() ");
			assert(0);
		}
		sem_post(&g_sem_cur_read);
	
		//7.将发送内容从链表中清除(因为本函数中, 所有的客户端命令都只对应一个应答包, 所以直接清空链表就可以)
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{			
			myprint("Error: func clear_LinkListSendInfo() ");
			assert(0);
		}
		//8.通知组包线程协议性错误, 不用再传输图片数据, 
		g_template_uploadErr = true;			
		sem_post(&g_sem_send);

	}

	return ret;
}

/*对服务器的应答数据包, 进行选择处理
*@param : tmpContent		数据包转义后的数据
*@param : tmpContentLenth	转义后数据的长度, 包含: 报头 + 信息 + 校验码
*@retval: success 0, 
*/
int deal_with_pack(char *tmpContent, int tmpContentLenth )
{
	int ret = 0;
	uint8_t cmd = 0;					//接收数据包命令


	//1.获取接收数据包的命令字
	cmd = *((uint8_t *)tmpContent);

	//2.单包回复命令
	if(cmd == LOGINCMDRESPON || cmd == LOGOUTCMDRESPON || cmd == HEARTCMDRESPON || cmd == NEWESCMDRESPON
			 || cmd == DELETECMDRESPON || cmd == ENCRYCMDRESPON )
	{	
		ret = commom_respcommand_func(tmpContent, tmpContentLenth);
	}
	else
	{		
		switch (cmd){
			case DOWNFILEZRESPON:
				if((ret = downFile_respon_func(tmpContent, tmpContentLenth)) < 0)
					myprint("Error : func downFile_respon_func()");
				break;
			case UPLOADCMDRESPON:
				if((ret = upload_respon_func(tmpContent, tmpContentLenth)) < 0)
					myprint("Error : func upload_respon_func()");
				break;		
			case TEMPLATECMDRESPON:				
				if((ret = template_respon_func(tmpContent, tmpContentLenth)) < 0)
					myprint("Error : func template_respon_func()");
				break;
			case MUTIUPLOADCMDRESPON:
				if((ret = mutiUpload_respon_func(tmpContent, tmpContentLenth)) < 0)
					myprint("Error : func mutiUpload_respon_func()");
				break;
			case PUSHINFORESPON:
				if((ret = pushInfo_func(tmpContent, tmpContentLenth)) < 0)
					myprint("Error : func mutiUpload_respon_func()");
				break;
			default:
				myprint("recv from server cmd : %d Not found", cmd);
				assert(0);
		}
	}

	

	return ret;
}


void modify_listNodeNews(node *listNode)
{

	int newSendLenth = 0;

	modify_repeatPackSerialNews(listNode->sendAddr, listNode->sendLenth, &newSendLenth);
	listNode->sendLenth = newSendLenth;
	
	if((push_queue_send_block(g_queue_sendDataBlock, listNode->sendAddr, listNode->sendLenth, listNode->cmd)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		assert(0);
	}

	sem_post(&g_sem_cur_write);

}


//function: find  the whole packet; retval maybe error
int	data_unpack_process(int recvLenth)
{
	int ret = 0, tmpContentLenth = 0;
	static int flag = 0, lenth = 0; 	//flag 标识找到标识符的数量;  lenth : 数据的长度
	char buffer[2] = { 0 };
	static char content[2800] = { 0 };	//
	char tmpContent[1400] = { 0 }; 		//tmpBufFlag[15] = { 0 };		
	char *tmp = NULL;
	
	tmp = content + lenth;
	while(recvLenth > 0)
	{
		while(1)
		{

			//1. jurge current fifo is empty
			if(get_fifo_element_count(g_fifo) == 0)
			{
				goto End;
			}
			//2. get The fifo element
			if((ret = pop_fifo(g_fifo, (char *)buffer, 1)) < 0)
			{
				myprint( "Error: func pop_fifo() ");
				assert(0);
			}
			//3.flag : have find The whole package head 
			if(*buffer == 0x7e && flag == 0)
			{
				flag = 1;
				continue;
			}
			else if(*buffer == 0x7e && flag == 1)	//4. flag : have find The whole package 
			{
				//4. anti escape recv data
				if((ret = anti_escape(content, lenth, tmpContent, &tmpContentLenth)) < 0)
				{
					myprint( "Error: func anti_escape() no have data");
					goto End;
				}
										
				if((ret = compare_recvData_Correct( tmpContent, tmpContentLenth)) == 0)				
				{
					//5. 数据包正确,处理数据包
					//socket_log(SocketLevel[2], ret, "------ func compare_recvData_Correct() OK ------");

					if((deal_with_pack(tmpContent, tmpContentLenth )) < 0)
					{
						myprint("Error: func deal_with_pack()");
						assert(0);
					}

					//6.清空临时变量
					memset(content, 0, sizeof(content));
					memset(tmpContent, 0, sizeof(tmpContent));					
					recvLenth -= (lenth + 2);
					lenth = 0;
					flag = 0;
					tmp = content + lenth;
					break;					
				}
				else if(ret == -1)
				{
					//接收的数据包校验码出错, 需要服务器重新发送数据
					myprint("Error: func compare_recvData_Correct()");					
					//7. clear the variable
					memset(content, 0, sizeof(content));
					memset(tmpContent, 0, sizeof(tmpContent));					
					recvLenth -= (lenth + 2);
					lenth = 0;
					flag = 0;
					tmp = content + lenth;

					//8.遍历整个链表, 修改流水号, 重新所有发送数据				
					trave_LinkListSendInfo(g_list_send_dataInfo, modify_listNodeNews);						
  					ret = 0;			//此处被修改, 增加, 很重要  2017.04.21
					break;
				}
				else if(ret == -2)
				{
					//socket_log(SocketLevel[2], ret, "compare_recvData_Correct() ret : %d", ret);
					//9.接收的数据包 长度和流水号不正确, 不需要重新发送数据
					//clear the variable
					memset(content, 0, sizeof(content));
					memset(tmpContent, 0, sizeof(tmpContent));					
					recvLenth -= (lenth + 2);
					lenth = 0;
					flag = 0;
					tmp = content + lenth;
					break;	
				}
			}
			else if(*buffer != 0x7e && flag == 1)
			{
				//10. training in rotation
				*tmp++ = *buffer;
				lenth += 1;
				continue;
			}
			else
			{
				*tmp++ = *buffer;
				lenth += 1;
				continue;
			}
		}

	}


End:
	return ret;

}

//select() 定时器 
void timer_select(int seconds)
{
	struct timeval temp;
 	temp.tv_sec = seconds;
    temp.tv_usec = 0;
    int err = 0;
	do{
		err = select(0, NULL, NULL, NULL, (struct timeval *)&temp);
	}while(err < 0 && errno == EINTR);

}


//超时线程
void *thid_server_func_time(void *arg)
{
	time_t  nowTime = 0, timeDiffer = 0;		//时间
	int waitNumber = 0;							//超时后, 相同数据包重发次数

	//1.分离子线程
	pthread_detach(pthread_self());

	//2.进行轮训检测
	while(1)
	{		
		//3.定时检测
		timer_select(OUTTIMEPACK + 3);
		//4.如果客户端出错, 继续轮询
		if(g_err_flag)
		{
			waitNumber = 0;
			continue;	
		}
		//5.对发送链表的元素数量进行判断
		if((Size_LinkListSendInfo(g_list_send_dataInfo)) == 0)
		{
			waitNumber = 0;
			//发送链表中无元素, 继续轮询,
			continue;	
		}
		else if((Size_LinkListSendInfo(g_list_send_dataInfo)) > 1)
		{	
			//发送链表含多包元素			
			time(&nowTime);
			timeDiffer = nowTime - g_lateSendPackNews.sendTime;

			//6.查看是否超时
			if(timeDiffer < OUTTIMEPACK)
			{
				//未超时
				continue;
			}
			else
			{
				if(waitNumber < 2)
				{
					myprint("respond timeOut : %d > time : %d, repeatNumber : %d", timeDiffer, OUTTIMEPACK, waitNumber);
					//超时, 判断本轮数据包是否有其他应答
					if(g_subResPonFlag)
					{	
						//有其他应答, 只重发最后一包数据
						repeat_latePack();
					}	
					else
					{
						//没有任何应答, 本轮所有的数据包全部重发
						trave_LinkListSendInfo(g_list_send_dataInfo, modify_listNodeNews);						
					}
					waitNumber++;
				}
				else
				{
					error_net(TIMEOUTRETURNFLAG);
					waitNumber = 0;
				}
			}
		}
		else if((Size_LinkListSendInfo(g_list_send_dataInfo)) == 1)
		{
			//发送链表含单包元素
			time(&nowTime);
			timeDiffer = nowTime - g_lateSendPackNews.sendTime;

			//7.查看是否超时
			if(timeDiffer < OUTTIMEPACK)
			{
				//未超时
				continue;
			}
			else
			{
				if(waitNumber < 2)
				{
					myprint("respond timeOut : %d > time : %d, repeatNumber : %d", timeDiffer, OUTTIMEPACK, waitNumber);
					//超时
					repeat_latePack();
					waitNumber++;
				}
				else
				{	
					error_net(TIMEOUTRETURNFLAG);
					waitNumber = 0;
				}
			}			
		}			
	}

	return NULL;
}


void repeat_latePack()
{
	int newSendLenth = 0;

	//1.需要重发最后一包数据, 修改它的流水号
	modify_repeatPackSerialNews(g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, &newSendLenth);

	//2.重新赋值数据包长度, 并加入队列
	g_lateSendPackNews.sendLenth = newSendLenth;
	if((push_queue_send_block(g_queue_sendDataBlock, g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, g_lateSendPackNews.cmd)) < 0)
	{
		myprint("Error : func push_queue_send_block()");
		assert(0);
	}
	
	//3.释放信号量,通知发送数据线程继续发送
	sem_post(&g_sem_cur_write);

}


/*unpack The data */
void *thid_unpack_thread(void *arg)
{
	int   ret = 0;
	pthread_detach(pthread_self());

	while(1)
	{
		//1. wait The semaphore
		sem_wait(&g_sem_unpack);

		//2. unpack The Queue Data
		if((ret = data_unpack_process(get_fifo_element_count(g_fifo))) < 0)
		{							
			myprint( "Error: func data_unpack_process()");							
		}

	}
	pthread_exit(NULL);
}


void assign_resComPack_head(resCommonHead_t *head, int cmd, int contentLenth, int serial, int isSubPack)
{
	memset(head, 0, sizeof(resCommonHead_t));
	head->cmd = cmd;
	head->contentLenth = contentLenth;
	head->seriaNumber = serial;
	head->isSubPack = isSubPack;

}


//发送信息过程中, 出现网络错误
int send_error_netPack(int sendCmd)
{
	int ret = 0;
	int cmd = 0, err = 0, errNum = 0;	//设置数据包的命令字, 错误标识符, 错误原因标识符


	//socket_log(SocketLevel[3], ret, "*****begin: send_error_netPack() sendCmd : %d", sendCmd);		


	switch (sendCmd)
	{
		case 1:		
			cmd = LOGINCMDRESPON;
			err = 2;
			errNum = 5;
			break;
		case 2:		
			return 0;
		case 3:
			cmd = HEARTCMDRESPON;
			err = 1;
			errNum = 5;
			break;
		case 4:
			cmd = DOWNFILEZRESPON;
			err = 1;
			errNum = 5;
			break;
		case 5:
			cmd = NEWESCMDRESPON;
			err = 1;
			errNum = 5;
			break;
		case 6:		
			cmd = UPLOADCMDRESPON;
			err = 2;
			errNum = 5;
			break;
		case 7:		
			cmd = PUSHINFOREQ;
			err = 1;
			errNum = 5;
			break;
		case 9:		
			cmd = DELETECMDRESPON;
			err = 1;
			errNum = 5;
			break;
		case 12:
			cmd = TEMPLATECMDRESPON;
			err = 1;
			errNum = 5;
			break;
		case 13:
			cmd = TEMPLATECMDRESPON;
			err = 1;
			errNum = 5;
			break;
		case 14:
			cmd = ENCRYCMDRESPON;
			err = 1;
			errNum = 5;
			break;
		default:
			printf("\nsendCmd : %d, [%d], [%s]\n", sendCmd, __LINE__, __FILE__);
			assert(0);
			
	}
	//2.发送数据包的指定错误信息至 UI
	if((ret = copy_news_to_mem(cmd, err, errNum)) < 0)
	{		
		myprint("Error: func copy_news_to_mem()");
		assert(0);
	}


	//socket_log(SocketLevel[3], ret, "*****end: send_error_netPack() sendCmd : %d", sendCmd);

	return ret;
}



/*发送数据包的指定错误信息至 UI
 *@param: cmd : 数据包命令
 *@param: err: 错误标识
 *@param: errNum: 错误原因标识
*/
int copy_news_to_mem(int cmd, int err, int errNum)
{
	int ret = 0;
	resCommonHead_t head;
	char *tmp = NULL, *tmpPool = NULL;

	//1.申请内存
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}

	//2.拷贝内容
	tmp = tmpPool;
	memset(&head, 0, sizeof(resCommonHead_t));
	head.cmd = cmd;
	head.contentLenth = sizeof(resCommonHead_t) + 2;
	memcpy(tmp, &head, sizeof(resCommonHead_t));
	tmp += sizeof(resCommonHead_t);
	*tmp++ = err;
	*tmp++ = errNum;
	

	//3.将组包的数据放入队列, 通知主线程进行处理, 发送给UI
	if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
	{
		mem_pool_free(g_memPool, tmpPool);
		myprint("Error: func push_queue()");
		assert(0);
	}
	
	sem_post(&g_sem_cur_read);


	return ret;
}



/*清空队列, 信号量中的数据; 该函数只有在发生网络错误 和 超时线程检测到达上限时才调用
 *@param: flag : 0 发送线程清除数据; 1 接收线程清除数据; TIMEOUTRETURNFLAG : 超时线程清除数据
 *@param: 20170531  认为该函数 发送线程不需要进行调用, 因为只要网络错误, 接收线程一定可以接收
 *@param:			到错误, 所以只需要调用一次即可;
*/
void clear_global(int flag)
{
	int ret = 0;
	int size = 0, i = 0;
	node rmNode;
	
	//socket_log(SocketLevel[3], ret, "func clear_global() begin flag : %d ", flag);

	//1.修改全局标志, 标识出错
	g_err_flag = true;


	//2.清空 接收服务器的数据队列
	if((ret = clear_fifo(g_fifo)) < 0)								
	{
		myprint("Error: func clear_fifo()");		
		assert(0);
	}

	//3.清空 发送数据至服务器的队列
	while (get_element_count_send_block(g_queue_sendDataBlock)) 	//5. 发送队列: 发送地址和长度
	{
		if((ret = pop_queue_send_block(g_queue_sendDataBlock, NULL, NULL, NULL)) < 0) //6. 清空队列
		{
			myprint("Error: func pop_queue_send_block()");
			assert(0);
		}
	}

	//4.清空 保存发送信息的链表
	if((size = Size_LinkListSendInfo(g_list_send_dataInfo)) < 0)
	{
		myprint("Error: func Size_LinkListSendInfo()");		
		assert(0);
	}
	for(i = 0; i < size; i++)
	{
		if((PopBack_LinkListSendInfo(g_list_send_dataInfo, &rmNode)) < 0)
		{
			myprint("Error: func PopBack_LinkListSendInfo()");		
			assert(0);
		}

		if((mem_pool_free(g_memPool, rmNode.sendAddr)) < 0)
		{
			myprint("Error: func mem_pool_free()");		
			assert(0);
		}
	}


	//5.选择是哪个功能调用的该函数, 进行信号量的处理
	if(flag == 1)				//接收线程调用该函数
	{
		sem_init(&g_sem_read_open, 0, 0);		//接收线程信号量, 开始接收数据
	}
#if 0	
	if(flag == 1)								//接收线程调用该函数
	{		
		sem_post(&g_sem_send);					//目的: 接收出错后,通知任务组包线程不要阻塞了. 	
		sem_init(&g_sem_read_open, 0, 0);		//接收线程信号量, 开始接收数据
		//sem_init(&g_sem_send, 0, 0);			//防止无消息收发时, 网络断开, 该信号量凭空增1; 此处应注释, 因为线程运行时间是竞争模式, 会导致另一个线程还没有接收到该信号量, 又被这个线程初始化了
	}
	else										//超时线程
	{
		sem_post(&g_sem_send);					//目的: 接收出错后,通知任务组包线程不要阻塞了. 	
	}
#endif		
	//socket_log(SocketLevel[3], ret, "func clear_global() end flag : %d ", flag);

	
	return ;
}



/*net 接收数据中, 网络出错, 发错0号数据包
 *@param : flag  1 接收线程出错
*/
int error_net(int flag)
{
	int ret = 0;
	char *sendData = NULL;
	resCommonHead_t  head;
	
	//socket_log(SocketLevel[2], ret, "debug: func error_net()  begin");

	//1.发送指定数据包错误, g_isSendPackCurrent = true, 标识当前正在等待发送数据包的应答,此时出错,应该返回该数据包的错误和0号数据包
	if(g_isSendPackCurrent)
	{
		send_error_netPack(g_send_package_cmd);
		sem_post(&g_sem_send);					//目的: 接收出错后,通知任务组包线程不要阻塞了. 	
		g_isSendPackCurrent = false;
	}
	//2.清空队列, 链表
	clear_global(flag);

	//3.数据包发送多次后, 未收到回复, 超时
	if(flag == TIMEOUTRETURNFLAG)
	{
		return ret;
	}
	
	//4.组数据包头
	assign_resComPack_head(&head, ERRORFLAGMY, sizeof(resCommonHead_t), 1, 1 );
	
	//5.申请内存, 将数据拷贝进内存
	if((sendData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error : func mem_pool_alloc()");
		assert(0);
	}
	memcpy(sendData, &head, sizeof(resCommonHead_t));

	//6.向队列中放入数据
	if((ret = push_queue(g_queueRecv, sendData)) < 0)
	{
		myprint("Error : func push_queue()");
		mem_pool_free(g_memPool, sendData);		
		assert(0);
	}		

	//7.重置接收服务器数据包流水号, 并发送信号通知发送线程
	g_recvSerial = 0;
	sem_post(&g_sem_cur_read);
	
	
	//socket_log(SocketLevel[2], ret, "debug: func error_net()  end");
	return ret;
}


//接收数据线程
void *thid_server_func_read(void *arg)
{
	int ret = 0;
	int num = 0;
	
	//1.分离线程
	pthread_detach(pthread_self());

	while(1)
	{
		//2.等待信号量通知, 已链接上服务器, 可以进行数据接收
		sem_wait(&g_sem_read_open);			

		while(1)
		{
			//3.接收数据
			if((ret = recv_data(g_sockfd)) < 0)			
			{		
				//4.接收数据出错
				if(g_sockfd > 0)
				{
					close(g_sockfd);
					g_sockfd = 0;
				}

				//5.主动关闭或销毁网络, 不需要进行出错回复
				if(g_close_net == 1 || g_close_net == 2)	
				{
					clear_global(1);
					myprint( "The UI active shutdown NET in read thread");				
					break;
				}
				else if(g_close_net == 0)		
				{	
					
					//6.进行出错回复				
					do{
						if((ret = error_net(1)) < 0)		//网络出错后, 给UI 端回复
						{
							myprint(  "Error: func error_net()");
							sleep(1);	
							num++;			//继续进行发送
						}
						else				//发送成功,
						{		
							num = 0;
							break;
						}	
					}while(num < 2);

					break;
				}
			}			
		}
	}

	pthread_exit(NULL);
}


//发送线程发送数据
void *thid_server_func_write(void *arg)
{
	int ret = 0;
	int sendCmd = -1;		//发送数据包的命令字
	
	//socket_log( SocketLevel[3], ret, "func thid_server_func_write() begin");

	//1.分离线程
	pthread_detach(pthread_self());

	//2.进入操作流程
	while(1)
	{
		//3.等待信号量通知, 进行数据发送
		sem_wait(&g_sem_cur_write);		
		//pthread_mutex_lock(&g_mutex_client);

		//4.进行数据发送
		if((ret = send_data(g_sockfd, &sendCmd)) < 0)		
		{
			//发送失败
			//socket_log( SocketLevel[3], ret, "---------- func send_data() err -------");
							
			//套接字需要关闭, 释放互斥锁
			if(g_sockfd > 0)
			{
				close(g_sockfd );
				g_sockfd = 0;
			}					
		}			
		else			
		{		
			if(sendCmd != PUSHINFOREQ )		g_isSendPackCurrent = true;	
		}

	}

	pthread_exit(NULL);		//线程退出
	socket_log( SocketLevel[3], ret, "func thid_server_func_write() end");

	return NULL;
}



void assign_reqPack_head(reqPackHead_t *head, uint8_t cmd, int contentLenth, int isSubPack, int currentIndex, int totalNumber, int perRoundNumber )
{
	memset(head, 0, sizeof(reqPackHead_t));
	head->cmd = cmd;
	head->contentLenth = contentLenth;
	strcpy((char *)head->machineCode, g_machine);
	head->isSubPack = isSubPack;
	head->totalPackage = totalNumber;
	head->currentIndex = currentIndex;
	head->perRoundNumber =  perRoundNumber;
	pthread_mutex_lock(&g_mutex_serial);
	head->seriaNumber = g_seriaNumber++;
	pthread_mutex_unlock(&g_mutex_serial);
}

//登录数据包  1~123213123~qgdyuwgqu455~;   
int  login_func(const char* news)
{	
	int ret = 0;
	char *tmp = NULL;
	char accout[11] = { 0 };
	char passWd[20] = { 0 };
	uint8_t passWdSize;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0, contenLenth = 0;
	int outDataLenth = 0;

	//socket_log(SocketLevel[2], ret, "func login_func() begin: [%s]", news);

	//1.查找分割符
	if((tmp = strchr(news, DIVISIONSIGN)) == NULL)
	{		
		myprint("Error: func strstr() No find");
		assert(0);
	}

	memcpy(accout, tmp + 1, ACCOUTLENTH);
	tmp += ACCOUTLENTH;
	if((tmp = strchr(tmp, DIVISIONSIGN)) == NULL)
	{
		myprint( "Error: func strstr() No find");
		assert(0);
	}
	memcpy(passWd, tmp + 1, strlen(tmp)-2);				//获取密码
	passWdSize = strlen(passWd);

	//2.申请内存
	if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc()");
		assert(0);
	}
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}

	//3.计算数据包总长度, 并组装报头
	contenLenth = sizeof(reqPackHead_t) + ACCOUTLENTH + sizeof(char) + passWdSize;
	assign_reqPack_head(&head, LOGINCMDREQ, contenLenth, NOSUBPACK, 0, 1, 1);

	//4.组装数据包信息
	tmp = tmpSendPackDataOne;
	memcpy(tmp, (char *)&head, sizeof(reqPackHead_t));
	tmp += sizeof(reqPackHead_t);
	memcpy(tmp, accout, ACCOUTLENTH);
	tmp += ACCOUTLENTH;
	*tmp++ = passWdSize;
	memcpy(tmp, passWd, passWdSize);

	//5.计算校验码
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	tmp += passWdSize;
	memcpy(tmp, &checkCode, sizeof(int));

	//6.转义数据包内容
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData + 1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;

	//7.释放内存至内存池
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint( "Error: func mem_pool_free()");
		assert(0);
	}
	if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, LOGINCMDREQ)) < 0)
	{
		myprint( "Error: func push_queue_send_block()");
		assert(0);
	}
	if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, LOGINCMDREQ )) < 0)
	{
		myprint( "Error: func Pushback_node_index_listSendInfo()");
		assert(0);
	}

	sem_post(&g_sem_cur_write);	//发送信号量, 通知发送线程
	sem_wait(&g_sem_send);

	//socket_log(SocketLevel[2], ret, "func login_func() end: [%s] %p", news, sendPackData);

	
	return ret;
}

//登出
int  exit_program(const char* news)
{
	int ret = 0;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0;
	int outDataLenth = 0;

	//1.组报头
	assign_reqPack_head(&head, LOGOUTCMDREQ, sizeof(reqPackHead_t), NOSUBPACK, 0, 1, 1);

	//2.申请内存
	if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc()");
		assert(0);
	}

	//3.计算校验码
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const  char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne + sizeof(reqPackHead_t), (char *)&checkCode, sizeof(uint32_t));

	//4.转义数据内容
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//5.释放临时内存, 并将发送地址和数据长度放入队列
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint("Error: func mem_pool_free()");
		assert(0);
	}
	if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, LOGOUTCMDREQ)) < 0)
	{
		myprint( "Error: func push_queue_send_block()");
		assert(0);
	}
	if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, LOGOUTCMDREQ )) < 0)
	{
		myprint( "Error: func Pushback_node_index_listSendInfo()");
		assert(0);
	}

	sem_post(&g_sem_cur_write);	//发送信号量, 通知发送线程
	sem_wait(&g_sem_send);


	return ret;
}


//心跳
int  heart_beat_program(const char* news)
{
	int ret = 0;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0;
	int outDataLenth = 0;


	//1.package The Head news
	assign_reqPack_head(&head, HEARTCMDREQ, sizeof(reqPackHead_t), NOSUBPACK, 0, 1, 1);

	//2.alloc The memory block
	if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc()");
		assert(0);
	}
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc()");
		assert(0);
	}

	//3.计算校验码
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne + head.contentLenth, (char *)&checkCode, sizeof(uint32_t));
	
	//4.转义数据内容	
	*sendPackData = PACKSIGN; 
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//5.释放临时内存, 并将发送地址和数据长度放入队列
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint( "Error: func mem_pool_free()");
		assert(0);
	}
	if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, HEARTCMDREQ)) < 0)
	{
		myprint( "Error: func push_queue_send_block()");
		assert(0);
	}
	if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, HEARTCMDREQ )) < 0)
	{
		myprint( "Error: func Pushback_node_index_listSendInfo()");
		assert(0);
	}

	sem_post(&g_sem_cur_write);
	sem_wait(&g_sem_send);


	return ret;
}

//下载模板文件, cmd~type~ID~
int download_file_fromServer(const char *news)
{
	int ret = 0;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0;
	int outDataLenth = 0, contentLenth = 0;
	char *tmp = NULL, *tmpOut = NULL ;
	char IdBuf[65] = { 0 };
	long long int fileid = 0;

	//1.find The selected content
	if((tmp = strchr(news, DIVISIONSIGN)) == NULL)
	{		
		myprint( "Error: func strstr() No find");
		assert(0);
	}
	tmp++;

	//2.alloc The memory block
	if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}

	//3.get The downLoad FileType And FileID
	tmpOut = tmpSendPackDataOne + sizeof(reqPackHead_t);
	if(*tmp == '0') 			*tmpOut++ = 0;
	else if(*tmp == '1')		*tmpOut++ = 1;
	else						assert(0);
	tmp += 2;
	memcpy(IdBuf, tmp, strlen(tmp) - 1);
	fileid = atoll(IdBuf);
	memcpy(tmpOut, &fileid, sizeof(long long int)); 	
	contentLenth = sizeof(reqPackHead_t) + sizeof(char) * 65;

	//5. package The head news
	assign_reqPack_head(&head, DOWNFILEREQ, contentLenth, NOSUBPACK, 0, 1, 1);
	memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));

	//6. calculation The checkCode
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	tmp = tmpSendPackDataOne + head.contentLenth;
	memcpy(tmp, (char *)&checkCode, sizeof(int));


	//7. escape The origin Data
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//8.free The tmp mempool block And push The sendData Addr and lenth in queue
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint( "Error: func mem_pool_free()");
		assert(0);
	}
	if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, DOWNFILEREQ)) < 0)
	{
		myprint( "Error: func push_queue_send_block()");
		assert(0);
	}
	if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, DOWNFILEREQ)) < 0)
	{
		myprint( "Error: func Pushback_node_index_listSendInfo()");
		assert(0);
	}

	sem_post(&g_sem_cur_write);
	sem_wait(&g_sem_send);

	return ret;
}



//Get The specify File type NewEst ID, cmd~type~
int  get_FileNewestID(const char* news)
{
	int ret = 0;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0;
	int outDataLenth = 0, contentLenth = 0;
	char *tmp = NULL, *tmpOut = NULL ;
	
	//1.find The selected content
	if((tmp = strchr(news, DIVISIONSIGN)) == NULL)
	{
		myprint( "Error: func strstr() No find");
		assert(0);
	}
	tmp++;
	
	//2.alloc The memory block
	if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc()");
		assert(0);
	}

	//3.get The downLoad FileType
	tmpOut = tmpSendPackDataOne + sizeof(reqPackHead_t);
	if(*tmp == '0') 			*tmpOut++ = 0;
	else if(*tmp == '1')		*tmpOut++ = 1;
	else						assert(0);
	contentLenth = sizeof(reqPackHead_t) + sizeof(char) * 1;
			
	//4. package The head news
	assign_reqPack_head(&head, NEWESCMDREQ, contentLenth, NOSUBPACK, 0, 1, 1);
	memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));

	//6. calculation The checkCode
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	tmp = tmpSendPackDataOne + head.contentLenth;
	memcpy(tmp, (char *)&checkCode, sizeof(int));
	
	
	//7. escape The origin Data
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//8.free The tmp mempool block And push The sendData Addr and lenth in queue
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint( "Error: func mem_pool_free()");
		assert(0);
	}
	if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, NEWESCMDREQ)) < 0)
	{
		myprint( "Error: func push_queue_send_block()");
		assert(0);
	}
	if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, NEWESCMDREQ )) < 0)
	{
		myprint( "Error: func Pushback_node_index_listSendInfo()");
		assert(0);
	}

	sem_post(&g_sem_cur_write);
	sem_wait(&g_sem_send);
	

	return ret;
}

//每轮数据开始发送之前 的总包(不携带具体数据)
int send_perRounc_begin(int cmd, int totalPacka,int sendPackNumber)
{
	 reqPackHead_t  head;								//请求信息的报头
	 char *sendPackData = NULL;							//发送数据包地址
	 char tmpSendPackDataOne[PACKMAXLENTH] = { 0 };		//临时数据包地址长度
	 int checkCode = 0;									//校验码
	 char *tmp = NULL;
	 int ret = 0, outDataLenth = 0;
	 
	 //1.组装报头
	 assign_reqPack_head(&head, cmd, sizeof(reqPackHead_t), NOSUBPACK, 0, totalPacka, sendPackNumber);
			
	 //2. 申请内存, 储存发送数据包
	 if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	 {			 
	 	 myprint("Error : func mem_pool_alloc()");
		 assert(0);
 	 }

	 //3.计算数据校验码,
	 memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
	 checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
	 tmp = tmpSendPackDataOne + head.contentLenth;
	 memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

	 //4. 转义数据包内容
	 *sendPackData = PACKSIGN;							 //flag 
	 if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	 {
		 myprint("Error : func escape()");
		 assert(0);
	 }
	 *(sendPackData + outDataLenth + 1) = PACKSIGN; 	 //flag 

	 //5. 将发送数据包放入队列, 通知发送线程
	 if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, cmd)) < 0)
	 {
		 myprint("Error : func push_queue_send_block()");
		 assert(0);
	 }

	 //6. 将发送数据放入链表			 
	 if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, cmd )) < 0)
	 {
		  myprint( "Error: func Pushback_node_index_listSendInfo()");
		  assert(0);
	 }
	
	 sem_post(&g_sem_cur_write);
	 sem_wait(&g_sem_send);

	 return ret;

}


//upload file content  6~liunx.c~	   The package cmd : 6
int  upload_picture(const char* news)
{
	int ret = 0, index = 0, i = 0;			//index : 当前文件包的序号
	uint16_t  tmpTotalPack = 0, totalPacka = 0;				//发送文件所需要的总包数
	char *tmp = NULL;
	char filepath[250] = { 0 };				//发送文件的绝对路径
	char fileName[100] = { 0 };				//发送的文件名
	int  checkCode = 0;						//校验码	
	int doucumentLenth = 0 ;				//每个包储存文件数据内容的最大字节数
	int lenth = 0, packContenLenth = 0;		//lenth : 读取文件内容的长度, packContenLenth : 数据包的原始长度
	char *sendPackData = NULL;				//发送数据包的地址
	char *tmpSendPackDataOne = NULL;		//临时缓冲区的地址
	int outDataLenth = 0;					//每个数据包的发送长度
	FILE *fp = NULL;						//打开的文件句柄
	reqPackHead_t  head;					//请求信息的报头
	int   sendPackNumber = 0;				//每轮发送数据的包数
	int   nRead = 0;						//已经拷贝的数据总长度
	int   copyLenth = 0;					//每次拷贝的数据长度
	int  tmpLenth = 0;

	
	//1. calculate The package store fileContent Max Lenth;  1369 - 46 = 1323
	doucumentLenth = COMREQDATABODYLENTH - FILENAMELENTH;
	
	//2.check The news style and find '~' position, copy The filePath
	if((tmp = strchr(news, '~')) == NULL)
	{
		myprint("Error : The content style is err : %s", news);
		assert(0);
	}
	memcpy(filepath, tmp + 1, strlen(tmp) - 2);

	//3.get The filepath totalPacka by doucumentLenth of part of the filecontent	
	if((ret = get_file_size(filepath, (int *)&totalPacka, doucumentLenth, NULL)) < 0)
	{
		myprint( "Error: func get_file_size()");
		assert(0);
	}
	tmpTotalPack = totalPacka;

	//4.find The fileName and open The file
	if((ret = find_file_name(fileName, sizeof(fileName), filepath)) < 0)
	{
		myprint( "Error: func find_file_name()");
		assert(0);
	}
	if((fp = fopen(filepath, "rb")) == NULL)
	{
		myprint("Error: func fopen() : %s", filepath);		
		assert(0);
	}

	//5.allock The block buffer in memory Pool
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;		
		myprint("Error: func mem_pool_alloc() ");
		assert(0);
	}

	//6. operation The file content and package
	while(!feof(fp))
	{
		//7.计算每轮发送的总包数
		if(tmpTotalPack > PERROUNDNUMBER)			sendPackNumber = PERROUNDNUMBER;
		else										sendPackNumber = tmpTotalPack;
	
		//8.读取每轮发送文件的所有内容
		memset(g_sendFileContent, 0, PERROUNDNUMBER * COMREQDATABODYLENTH);
		while(lenth < sendPackNumber * doucumentLenth && !feof(fp) )
		{
			if((tmpLenth = fread(g_sendFileContent + lenth, 1, sendPackNumber * doucumentLenth - lenth, fp)) < 0)
			{
				myprint("Error : func fread() : %s", filepath);
				assert(0);
			}

			lenth += tmpLenth;
		}
	
		send_perRounc_begin(UPLOADCMDREQ, totalPacka, sendPackNumber);
		if(g_residuePackNoNeedSend)			
		{
			g_residuePackNoNeedSend = false;
			goto End;
		}
		if(g_err_flag)
		{
			goto End;
		}

		//9.对本轮的数据进行组包
		for(i = 0; i < sendPackNumber; i++)
		{
			//10.拷贝文件名称
			 tmp = tmpSendPackDataOne + sizeof(reqPackHead_t);
			 memcpy(tmp, fileName, FILENAMELENTH);
			 tmp += FILENAMELENTH;
			 
			 //11.拷文件数据
			 copyLenth = MY_MIN(lenth, doucumentLenth);		
			 memcpy(tmp, g_sendFileContent + nRead , copyLenth);
			 nRead += copyLenth;
			 lenth -= copyLenth;
			
			 //12. 计算数据包原始长度, 并初始化报头
			 packContenLenth = sizeof(reqPackHead_t) + FILENAMELENTH + copyLenth;
			 assign_reqPack_head(&head, UPLOADCMDREQ, packContenLenth, SUBPACK, index, totalPacka, sendPackNumber);
			 
			 //13. 申请内存, 储存发送数据包
			 if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			 {			 
				 myprint("Error : func mem_pool_alloc()");
				 assert(0);
		 	 }

			 //14.计算数据校验码,
			 memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
			 checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
			 tmp = tmpSendPackDataOne + head.contentLenth;
			 memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

			 //15. 转义数据包内容
			 *sendPackData = PACKSIGN;							 //flag 
			 if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
			 {
				 myprint("Error : func escape()");
				 assert(0);
			 }
			 *(sendPackData + outDataLenth + 1) = PACKSIGN; 	 //flag 

			 //16. 将发送数据包放入队列, 通知发送线程
			 if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, UPLOADCMDREQ)) < 0)
			 {
				 myprint("Error : func push_queue_send_block()");
				 assert(0);
			 }
			 
			 if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, index, sendPackData, outDataLenth + sizeof(char) * 2, UPLOADCMDREQ )) < 0)
			 {
				 myprint("Error: func Pushback_node_index_listSendInfo()");
				 assert(0);
			 }
			 
			 index += 1;
			 sem_post(&g_sem_cur_write);
		}

		//17.一轮数据发送完毕, 进行全局数据的计算
		nRead = 0;
		tmpTotalPack -= sendPackNumber;
		lenth = 0;

		//18.等待信号通知, 并检查是否还需要下一轮的组包
		sem_wait(&g_sem_send);	
		if(g_residuePackNoNeedSend)			
		{
			g_residuePackNoNeedSend = false;
			goto End;
		}
		if(g_err_flag)
		{
			goto End;
		}
		
	}

End:

	//19. 情况资源, 释放临时内存和文件描述符
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint("Error : func mem_pool_free()");
		assert(0);
	}
	if(fp)			fclose(fp);			fp = NULL;


	return ret;
}


//push information from server reply
int  push_info_from_server_reply(const char* news)
{
	int ret = 0;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0;
	int outDataLenth = 0;

	//1.package The Head news
	assign_reqPack_head(&head, PUSHINFOREQ, sizeof(reqPackHead_t), NOSUBPACK, 0, 1, 1);

	//2.alloc The memory block
	if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc()");
		assert(0);
	}
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc()");
		assert(0);
	}

	//3.
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne + head.contentLenth, (char *)&checkCode, sizeof(int));


	//4.
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//5.
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint( "Error: func mem_pool_free()");
		assert(0);
	}	
	if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, PUSHINFOREQ)) < 0)
	{
		myprint( "Error: func push_queue_send_block()");
		assert(0);
	}
	if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, PUSHINFOREQ)) < 0)
	{
		myprint( "Error: func Pushback_node_index_listSendInfo()");
		assert(0);
	}
	
	sem_post(&g_sem_cur_write);
	sem_wait(&g_sem_send);		

	return ret;
}



//主动关闭网络服务, 本地数据包			
int  active_shutdown_net()
{
	int  ret = 0;
	resCommonHead_t head;				//应答报头信息
	char *sendData = NULL;

	sem_init(&g_sem_read_open, 0, 0);			//初始化接收信号量
	g_close_net = 2;							//修改标识符, 主动关闭
	g_err_flag = true;							//修改标识符, 出错

	//1.关闭套接字
	if(g_sockfd > 0)
	{
		shutdown(g_sockfd, SHUT_RDWR); 
		g_sockfd = -1;
	}

	//2.alloc The memory block And package  News
	if((sendData = mem_pool_alloc(g_memPool)) == NULL)
	{		
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}
		
	//3.package The head News And body news
	if(ret == -1)
	{
		*(sendData + sizeof(reqPackHead_t)) = 1;
	}
	else 
	{
		*(sendData + sizeof(reqPackHead_t)) = ret;		
	}
	
	assign_resComPack_head(&head, ACTCLOSERESPON, sizeof(reqPackHead_t) + 1, 0, 0);	
	memcpy(sendData, &head, sizeof(reqPackHead_t));

	//4. push The sendData addr in queue
	if((ret = push_queue(g_queueRecv, sendData)) < 0)
	{
		myprint( "Error: func push_queue() " );								
		assert(0);
	}		
	sem_post(&g_sem_cur_read);			

	//5. have recv The cmd 11 to modify the config file And need set IP 
	if(g_modify_config_file)
	{
		if((ret = chose_setip_method()) < 0)
		{
			myprint( "Error: func chose_setip_method() " );								
			assert(0);
		}
		
		sleep(2);		// sleep' reason : set IP need response Time 
		g_modify_config_file = false;
	}
	
	return ret;
}


//删除图片  cmd~图片路径~
int delete_spec_pic(const char *news)
{
	int ret = 0;
	char fileName[50] = { 0 };			
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL, *tmp = NULL;
	int  checkCode = 0;
	int outDataLenth = 0;		

	//1.find The filePath And fileName
	if((tmp = strchr(news, DIVISIONSIGN)) == NULL)
	{
		myprint( "Error: func strstr() No find");
		assert(0);
	}
	//myprint("delete News : %s", news);
	
	memcpy(fileName, tmp + 1, strlen(tmp) - 2);			

	//2.alloc The memory block And package The Body news
	if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	{	
		myprint( "Error: func mem_pool_alloc()");
		assert(0);
	}
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc()");
		assert(0);
	}
	memcpy(tmpSendPackDataOne + sizeof(reqPackHead_t), fileName, strlen(fileName));
	//myprint("rm fileName : %s", fileName);
	//3.package The head news
	assign_reqPack_head(&head, DELETECMDREQ, sizeof(reqPackHead_t) + FILENAMELENTH, NOSUBPACK, 0, 1, 1);

	//4.计算校验码
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const  char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne+head.contentLenth, (char *)&checkCode, sizeof(uint32_t));
	//myprint("delete pic contentLenth : %d", head.contentLenth);

	
	//5.转义
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;
	
	//6.释放临时内容至内存池, 并将数据包放入发送队列和链表
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint( "Error: func mem_pool_free()");
		assert(0);
	}
	if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, DELETECMDREQ)) < 0)
	{
		myprint( "Error: func push_queue_send_block()");
		assert(0);
	}
	if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, DELETECMDREQ)) < 0)
	{
		myprint( "Error: func Pushback_node_index_listSendInfo()");
		assert(0);
	}
	
	sem_post(&g_sem_cur_write);
	sem_wait(&g_sem_send);


	return ret;
}



// 链接服务器, 本地数据包			
int  connet_server(const char *news)
{	
	int  ret = 0, contentLenth = 0;		//发送数据长度
	char *sendData = NULL;				//发送地址
	resCommonHead_t head;				//应答报头信息

	//1. 链接服务器
	if(g_sockfd <= 0)
	{		
		if((ret = client_con_server(&g_sockfd, WAIT_SECONDS)) < 0)			
		{
			//2. 链接服务器失败
			g_sockfd = -1;
			myprint("Error: func client_con_server()");
		}
		else if(ret == 0)			
		{
			//3. 链接成功, 创建加密通道句柄
		#if 0
			if((ret = ssl_init_handle(g_sockfd, &g_sslCtx, &g_sslHandle)) < 0)
			{
				g_sockfd = -1;
				g_sslCtx = NULL;
				g_sslHandle = NULL;
				myprint("Error: func ssl_init_handle()");
			}
			else
		#endif
			{
				//4. 初始化全局变量
				sem_post(&g_sem_read_open); 	//通知接收数据线程
				g_err_flag = false; 			//出错的全局变量修改
				g_close_net = 0;				//初始化
			}			
		}
		
	}

	//5. 申请内存
	if((sendData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}

	//6. 组装数据包信息和报头长度
	if(ret == 0)		*(sendData + sizeof(resCommonHead_t)) = ret;
	else				*(sendData + sizeof(resCommonHead_t)) = 1;
	contentLenth = sizeof(resCommonHead_t) + 1;	

	//7.组包报头信息, 并拷贝
	assign_resComPack_head(&head, CONNECTCMDRESPON, contentLenth, 0, 0);
	memcpy(sendData, &head, sizeof(resCommonHead_t));

	//8. 将消息放入队列
	if((ret = push_queue(g_queueRecv, sendData)) < 0)
	{
		myprint("Error: func push_queue() no have data");
		mem_pool_free(g_memPool, sendData); 	
		assert(0);
	}

	//9.释放信号量, 通知发送给UI数据的线程
	sem_post(&g_sem_cur_read);				

	return ret;
}


//读取配置文件, 本地数据包																
int read_config_file(const char *news)
{
	
	int  ret = 0;
	resCommonHead_t head;				//应答报头信息
	int contenLenth = 0;
	char *sendData = NULL, *tmp = NULL;

	//1.read config file And manual set The ip or operation The DHCP
	ret = get_config_file_para();

	//2.alloc The memory block And package Body news
	if((sendData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}
	tmp = sendData + sizeof(reqPackHead_t);
	if(ret < 0) 	*tmp = 1;
	else			*tmp = ret;
	contenLenth = sizeof(reqPackHead_t) + sizeof(char);

	//3.package Head
	assign_resComPack_head(&head, MODIFYCMDRESPON, contenLenth, 0, 0);

	//4.copy The head news in memory block
	memcpy(sendData, &head, sizeof(resCommonHead_t));

	//5.push The memory block Addr in queue
	if((ret = push_queue(g_queueRecv, sendData)) < 0)
	{
		myprint( "Error: func push_queue() no have data");
		assert(0);
	}

	//6.notice The shutwon function set IP. And post The semphore
	g_modify_config_file = true;		
	sem_post(&g_sem_cur_read);

	return ret;
}



/*模板数据包拓展
*@param : news   cmd~data~;
*@retval: success 0; fail -1.
*/
int template_extend_element(const char *news)
{
	int ret = 0, i = 0;
	char *tmp = NULL;
	reqPackHead_t head;							//报头
	char *tmpSendPackDataOne = NULL;				//临时缓冲区
	char *sendPackData = NULL;						//发送地址
	int  checkCode = 0, contentLenth = 0;			//校验码和数据长度
	int outDataLenth = 0;							//发送地址长度						
	char bufAddr[20] = { 0 };						//模板数据块地址				
	unsigned int apprLen = 0; 							
	UitoNetExFrame *tmpExFrame = NULL;					
						

	//1.去除分割符, 获取数据地址
	if((tmp = strchr(news, DIVISIONSIGN)) == NULL)
	{
		myprint( "Error: func strstr() No find");
		assert(0);
	}		
	memcpy(bufAddr, tmp + 1, strlen(tmp) - 2);	
	apprLen = hex_conver_dec(bufAddr);	
	tmp = NULL + apprLen;
	tmpExFrame = (UitoNetExFrame*)tmp;

	//2.组装报头
	contentLenth = sizeof(reqPackHead_t) + sizeof(unsigned char) * 4 + strlen(tmpExFrame->sendData.identifyID) + strlen(tmpExFrame->sendData.objectAnswer) + tmpExFrame->sendData.subMapCoordNum * sizeof(Coords) + tmpExFrame->sendData.totalNumberPaper * sizeof(tmpExFrame->sendData.toalPaperName[0]);
	assign_reqPack_head(&head, TEMPLATECMDREQ, contentLenth, NOSUBPACK, 0, 1, 1);

	//3.申请内存块
	if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}
	
	//5.组包报文
	//ID
	apprLen = strlen(tmpExFrame->sendData.identifyID);		
	tmp = tmpSendPackDataOne;
	memcpy(tmp, (char *)&head, sizeof(reqPackHead_t));			
	tmp += sizeof(reqPackHead_t);								
	*tmp = apprLen;
	tmp += 1;
	memcpy(tmp, tmpExFrame->sendData.identifyID, apprLen);
	//object
	tmp += apprLen;
	apprLen = strlen(tmpExFrame->sendData.objectAnswer);
	*tmp = apprLen;
	tmp += 1;
	memcpy(tmp, tmpExFrame->sendData.objectAnswer, apprLen);
	//fileName
	tmp += apprLen;	
	*tmp = tmpExFrame->sendData.totalNumberPaper;
	tmp += 1;
	apprLen = tmpExFrame->sendData.totalNumberPaper;
	memcpy(tmp, tmpExFrame->sendData.toalPaperName, sizeof(tmpExFrame->sendData.toalPaperName[1]) * apprLen );
	tmp += sizeof(tmpExFrame->sendData.toalPaperName[1]) * apprLen;
	//subcoord
	*tmp = tmpExFrame->sendData.subMapCoordNum;
	tmp += 1;
	memcpy(tmp, tmpExFrame->sendData.coordsDataToUi, tmpExFrame->sendData.subMapCoordNum * sizeof(Coords));

	//checkCode
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);	//计算校验码
	tmp = tmpSendPackDataOne + head.contentLenth;
	memcpy(tmp, &checkCode, sizeof(uint32_t));	

	//6.转义数据
	*sendPackData = PACKSIGN; 
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData + 1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;

	//7.释放临时内存
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint( "Error: func mem_pool_free()");
		assert(0);
	}

	if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, TEMPLATECMDREQ)) < 0)
	{
		myprint( "Error: func push_queue_send_block()");
		assert(0);
	}
	if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, TEMPLATECMDREQ)) < 0)
	{
		myprint( "Error: func Pushback_node_index_listSendInfo()");
		assert(0);
	}

	//8. 缓存扫描数据块图片信息
	memset(g_upload_set, 0, sizeof(g_upload_set));
	for(i = 0; i < tmpExFrame->sendData.totalNumberPaper; i++)
	{
		sprintf(g_upload_set[i].filePath, "%s/%s", tmpExFrame->fileDir, tmpExFrame->sendData.toalPaperName[i]);
		g_upload_set[i].totalNumber = tmpExFrame->sendData.totalNumberPaper;
		g_upload_set[i].indexNumber = i;
	}
	sem_post(&g_sem_cur_write);	//通知线程进行发送
	sem_wait(&g_sem_send);
	
	if(g_template_uploadErr)
	{
		g_template_uploadErr = false;
		goto End;
	}

	//9. 发送多张图片
	if((ret = upload_template_set(g_upload_set)) < 0)
	{
		myprint("Error: upload_template_set()");
		assert(0);
	}	

End:
	if(tmpExFrame)					mem_pool_free(g_memPool, tmpExFrame);	
	if(ret < 0)
	{
		if(sendPackData)			mem_pool_free(g_memPool, sendPackData);
		if(tmpSendPackDataOne)		mem_pool_free(g_memPool, tmpSendPackDataOne);
	}
	return ret;
}



/*上传多张图片
*@param : uploadSet  图片数据集合
*@retval: success 0; fail -1;
*/
int upload_template_set(uploadWholeSet *uploadSet)
{
	int ret = 0, i = 0;
	int contentLenth = 0;		
	
	contentLenth = COMREQDATABODYLENTH - FILENAMELENTH - sizeof( char) * 2;  
	
	for(i = 0; i < uploadSet->totalNumber; i++)
	{
		//对图片数据组包,
		if((ret = image_data_package(uploadSet[i].filePath, contentLenth, i, uploadSet->totalNumber)) < 0)
		{
			if(ret == -1)
			{
				myprint( "Error: func image_data_package()");
				assert(0);
			}
			else if(ret == -2)
			{
				myprint( "Error: func image_data_package() ret : %d", ret);
				ret = 0;
				goto End;
			}
		}
	}

End:
	return ret;
}


/*图片数据组包,进行上传
*@param : filePath  图片路径
*@param : readLen   数据包信息的长度(出去报头, 校验码, 标识符)
*@retval: success 0; fail -1; -2 接收到网络错误信号,接下来的图片不会组数据包
*/
int image_data_package(const char *filePath, int readLen, int fileNumber, int totalFileNumber)
{
	int ret = 0, index = 0, i = 0;			//index : 当前文件包的序号
	int  tmpTotalPack = 0, totalPacka = 0;	//发送文件所需要的总包数
	char *tmp = NULL;
	char filepath[250] = { 0 };				//发送文件的绝对路径
	char fileName[100] = { 0 };				//发送的文件名
	int  checkCode = 0;						//校验码	
	int doucumentLenth = 0 ;				//每个包储存文件数据内容的最大字节数
	int lenth = 0, packContenLenth = 0;		//lenth : 读取文件内容的长度, packContenLenth : 数据包的原始长度
	char *sendPackData = NULL;				//发送数据包的地址
	char *tmpSendPackDataOne = NULL;		//临时缓冲区的地址
	int outDataLenth = 0;					//每个数据包的发送长度
	FILE *fp = NULL;						//打开的文件句柄
	reqPackHead_t  head;					//请求信息的报头
	int   sendPackNumber = 0;				//每轮发送数据的包数
	int   nRead = 0;						//已经拷贝的数据总长度
	int   copyLenth = 0;					//每次拷贝的数据长度
	int  tmpLenth = 0;


	//1.获取文件组包数
	if((ret = get_file_size(filePath, &totalPacka, readLen, NULL)) < 0)
	{
		myprint("Error: func get_file_size() filePath : %s", filePath);
		assert(0);
	}
	doucumentLenth = readLen;
	tmpTotalPack = totalPacka;
	
	//2.查找文件名字
	if((ret = find_file_name(fileName, sizeof(fileName), filePath)) < 0)
	{
		myprint( "Error: func find_file_name()");
		assert(0);
	}

	//3.打开文件
	if((fp = fopen(filePath, "rb")) == NULL)
	{
		myprint("Error: func fopen(), %s", strerror(errno));
		assert(0);
	}

	//4.申请内存
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}

	//6. operation The file content and package
	while(!feof(fp))
	{
		//7.计算每轮发送的总包数
		if(tmpTotalPack > PERROUNDNUMBER)			sendPackNumber = PERROUNDNUMBER;
		else										sendPackNumber = tmpTotalPack;
	
		//8.读取每轮发送文件的所有内容
		memset(g_sendFileContent, 0, PERROUNDNUMBER * COMREQDATABODYLENTH);
		while(lenth < sendPackNumber * doucumentLenth && !feof(fp) )
		{
			if((tmpLenth = fread(g_sendFileContent + lenth, 1, sendPackNumber * doucumentLenth - lenth, fp)) < 0)
			{
				myprint("Error : func fread() : %s", filepath);
				assert(0);
			}

			lenth += tmpLenth;
		}
	
		send_perRounc_begin(MUTIUPLOADCMDREQ, totalPacka, sendPackNumber);
		if(g_residuePackNoNeedSend)			
		{
			ret = -2;
			g_residuePackNoNeedSend = false;
			goto End;
		}
		if(g_err_flag)
		{
			ret = -2;
			goto End;
		}
		//9.对本轮的数据进行组包
		for(i = 0; i < sendPackNumber; i++)
		{
			 //10.拷贝文件名称
			 tmp = tmpSendPackDataOne + sizeof(reqPackHead_t);
			 *tmp++ = fileNumber;
			 *tmp++ = totalFileNumber;
			 memcpy(tmp, fileName, FILENAMELENTH);
			 tmp += FILENAMELENTH;
			 
			 //11.拷文件数据
			 copyLenth = MY_MIN(lenth, doucumentLenth); 	
			 memcpy(tmp, g_sendFileContent + nRead , copyLenth);
			 nRead += copyLenth;
			 lenth -= copyLenth;

			 //12. 计算数据包原始长度, 并初始化报头
			 packContenLenth = sizeof(reqPackHead_t) + FILENAMELENTH + copyLenth + sizeof(char) * 2;
			 assign_reqPack_head(&head, MUTIUPLOADCMDREQ, packContenLenth, SUBPACK, index, totalPacka, sendPackNumber);
			 
			 //13. 申请内存, 储存发送数据包
			 if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			 {			 
				 myprint("Error : func mem_pool_alloc()");
				 assert(0);
			 }

			 //14.计算数据校验码,
			memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
			checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
			tmp = tmpSendPackDataOne + head.contentLenth;
			memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

			//15. 转义数据包内容
			*sendPackData = PACKSIGN;							//flag 
			if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
			{
				myprint("Error : func escape()");
				assert(0);
			}
			*(sendPackData + outDataLenth + 1) = PACKSIGN;		//flag 
		
			//16. 将发送数据包放入队列, 通知发送线程
			if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, MUTIUPLOADCMDREQ)) < 0)
			{
				myprint("Error : func push_queue_send_block()");
				assert(0);
			}
			
			if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, index, sendPackData, outDataLenth + sizeof(char) * 2, MUTIUPLOADCMDREQ )) < 0)
			{
				myprint("Error: func Pushback_node_index_listSendInfo()");
				assert(0);
			}
			
			index += 1;
			sem_post(&g_sem_cur_write);
			
		}
			
		//17.一轮数据发送完毕, 进行全局数据的计算
		nRead = 0;
		tmpTotalPack -= sendPackNumber;
		lenth = 0;

		//18.等待信号通知, 并检查是否还需要下一轮的组包
		sem_wait(&g_sem_send);	
		if(g_residuePackNoNeedSend)			
		{
			ret = -2;
			g_residuePackNoNeedSend = false;
			break;
		}
		if(g_err_flag)
		{
			ret = -2;
			break;
		}
		
	}

End:	
	//6.释放临时内存
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint( "Error: func mem_pool_free()");
		assert(0);
	}

	if(fp) 					fclose(fp);	
						
	return ret;
}



//find The file name in The Absolute filepath
int find_file_name(char *fileName, int desLenth, const char *filePath)
{
 	int ret = 0;
	const char *tmp  = NULL;

 	tmp = filePath + strlen(filePath);

 	while(*tmp != '/')			
 		--tmp;

	tmp += 1;
	desLenth > strlen(tmp) ? memcpy(fileName, tmp , strlen(tmp)) : memcpy(fileName, tmp , desLenth);

 	return ret;
}



//出错数据回复  0号数据包
int error_net_reply(char *news, char **outDataToUi)
{
	int ret = 0;
	char *outDataNews = NULL;


	//1.申请内存, 储存数据, 返回给UI
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func  mem_pool_alloc() ");		
		assert(0);
	}

	//2.将数据拷贝进内存 
	sprintf(outDataNews, "%d~", 0 );
	*outDataToUi = outDataNews;

	return ret;
}


/*登录信息回复
*/
int login_reply_func(char *news, char **outDataToUi)
{
	int ret = 0;
	char *tmp = NULL, *tmpOut = NULL;		//临时指针
	resCommonHead_t *head = NULL;			//数据报头
	char *outDataNews = NULL;				//内存块

	//1.申请内存, 储存数据, 返回给UI
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func  mem_pool_alloc() ");		
		assert(0);
	}

	//2.进行数据拷贝
	head = (resCommonHead_t *)news;
	sprintf(outDataNews, "%d~", LOGINCMDRESPON);

	//3.移动指针, 指向数据块, 获取数据块长度
	tmp = (char *)news + sizeof(resCommonHead_t);
	tmpOut = outDataNews + strlen(outDataNews);
	if(*tmp == 0x01)
	{			
		//登录成功
		sprintf(tmpOut, "%d~", *tmp);			
		tmp += 2;						//名称长度
		tmpOut += 2;					//指针向下移动
		memcpy(tmpOut, tmp + 1, *tmp);	//拷贝名称
		tmpOut += *tmp;
		*tmpOut = '~';
	}
	else
	{
		//登录失败
		sprintf(tmpOut, "%d~", *tmp);
		tmp += 1;
		tmpOut += 2;
		sprintf(tmpOut, "%d~", *tmp);
		tmp += 1;
		tmpOut += 2;
		if(*tmp > 0)
		{
			memcpy(tmpOut, tmp + 1, *tmp);
			tmpOut += *tmp;
			*tmpOut = '~';
		}
	}

	//6.进行赋值
	*outDataToUi = outDataNews;

	return ret;
}



//登出信息回复
int exit_reply_program( char *news, char **outDataToUi)
{
	int ret = 0;
	char *tmp = NULL, *tmpOut = NULL;
	char *outDataNews = NULL;


	if(!news )
	{		
		myprint( "Error: news == NULL");
		assert(0);
	}
	
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func  mem_pool_alloc() ");
		assert(0);
	}
		
	sprintf(outDataNews, "%d~", LOGOUTCMDRESPON);		
	tmpOut = outDataNews + strlen(outDataNews);			
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );

	*outDataToUi = outDataNews;
 
	return ret;
}



//心跳数据回复
int heart_beat_reply_program( char *news, char **outDataNews)
{

	if(!news  )
	{
		myprint( "Error: news == NULL");
		return -1;
	}

	return 1;
}


int download_reply_template( char *news, char **outDataToUi)
{
	int ret = 0;
	const char *tmp = NULL;
	char *fileName;
	char *outDataNews = NULL;
	static FILE *fp = NULL;
	resSubPackHead_t *tmpHead = NULL;				//应答报头
	static bool openFlag = false;					//true : open The file; false : no open The file
	int  nWrite = 0, fileLenth = 0, tmpLenth = 0;

	//1. 获取报头信息, 并移动指针
	tmpHead = (resSubPackHead_t *)news;
	tmp = news + sizeof(resSubPackHead_t);

	//2. 判断下载模板是否成功
	if(*tmp == 0)		
	{		
		//3.下载成功, 写文件, open The file;  64 fileName lenth
		if(!openFlag)
		{
			tmp = news + sizeof(resSubPackHead_t) + sizeof(char) * 2;

			//4.根据类型选择将要存储文件的名称
			if(*tmp == 0)
			{
				fileName = g_downLoadTemplateName;
			}
			else if(*tmp == 1)
			{
				fileName = g_downLoadTableName;
			}
			else
			{
				myprint("The downFile type : %d", *tmp);
				assert(0);
			}

			//5.打开文件
			if((fp = fopen(fileName, "wb" )) == NULL)
			{
				myprint( "Error: func fopen(), The fileName %s, %s", fileName, strerror(errno));
				assert(0);
			}
			openFlag = true;
		}
		//6. write And create The file
		tmp = news + sizeof(resSubPackHead_t) + sizeof(char) * 3;
		fileLenth = tmpHead->contentLenth - sizeof(resSubPackHead_t) - sizeof(char) * 3;
		do{ 		
			if((tmpLenth = fwrite(tmp + nWrite, 1, fileLenth - nWrite, fp)) < 0)
			{
				myprint("Error: func fwrite(), The fileName %s, %s", fileName, strerror(errno));
				assert(0);			
			}
			else
			{
				nWrite += tmpLenth;
			}
		}while(nWrite < fileLenth);
	
		//7. jump whether reply UI message
		if(tmpHead->currentIndex + 1 < tmpHead->totalPackage)
		{
			//8. 不需要对 UI 进行回复
			//myprint("func download_reply_template() continue index : %d, ret : %d,	indexPackge + 1 : %d, totalPackge : %d", 
			//	index++, ret, tmpHead->packgeNews.indexPackge + 1, tmpHead->packgeNews.totalPackge);
			ret = 1;
		}
		else
		{
			//9. 本模板最后一包数据成功接收, 需要向 UI 进行回复
			if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
			{
				myprint("Error: func  mem_pool_alloc() ");
				assert(0);	
			}
			sprintf(outDataNews, "%d~%d~", DOWNFILEZRESPON, 0);
			
			if((ret = fflush(fp)) < 0)
			{
				myprint("Error: func fflush(), The fileName %s, %s", fileName, strerror(errno));
				assert(0);
			}
			if((ret = fclose(fp)) < 0)
			{
				myprint( "Error: func fclose(), The fileName %s, %s", fileName, strerror(errno));
				assert(0);
			}		
			fp = NULL;
			*outDataToUi = outDataNews;
			openFlag = false;
			ret = 0;

			//9.修改标识符, 标识接收到该数据包的应答
			g_isSendPackCurrent = false;
			
			//socket_log(SocketLevel[2], ret, "func download_reply_template() end fclose() index : %d, ret : %d", index++, ret);
		}
	}
	else
	{
		//10.下载文件失败	
		if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
		{
			myprint( "Error: func  mem_pool_alloc() ");
			assert(0);
		}
					
		sprintf(outDataNews, "%d~%d~%d~", DOWNFILEZRESPON, *tmp, *(tmp + 1));
		if(fp)		fclose(fp); 		fp = NULL;			
		*outDataToUi = outDataNews;
		openFlag = false;
		ret = 0;

		//11.修改标识符, 标识接收到该数据包的应答
		g_isSendPackCurrent = false;
		//socket_log(SocketLevel[2], ret, "func download_reply_template() end  index : %d, ret : %d", index++, ret);
	}
	

	
	return ret;
}


/*Get The newest File ID
*@param : news   The reply content from server; style : head + body + checkCode;
*@param : lenth  The head + body lenth;
*@param : outDataNews reply data to UI;   style : cmd~struct content~
*@retval: 0: success;  -1: fail; 1: continue And not reply UI
*/
int get_FileNewestID_reply( char *news, char **outDataToUi)
{
	int  ret = 0;
	char *tmp = NULL;
	char *outDataNews = NULL;
	long long int fileID =  0;

	//1. alloc The memory block
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func  mem_pool_alloc() ");
		assert(0);
	}
	
	//2. get The fileType from server And fileID	
	tmp = (char *)news + sizeof(resCommonHead_t);
	memcpy(&fileID, tmp + 1, sizeof(long long int));
	if(*tmp == 0)					sprintf(outDataNews, "%d~%d~%lld~", NEWESCMDRESPON, 0, fileID);				//1.缁勫懡浠ゅ瓧
	else if(*tmp == 1)				sprintf(outDataNews, "%d~%d~%lld~", NEWESCMDRESPON, 1, fileID );				//1.缁勫懡浠ゅ瓧
	else 							assert(0);
	
	*outDataToUi = outDataNews;
	
	return ret;
}


/*deal with sclient request news reply 
*@param : news   The reply content from server; style : head + body + checkCode;
*@param : outDataNews reply data to UI;   style : cmd~struct content~
*@retval: 0: success;  -1: fail; 1: continue And not reply UI
*/
int upload_picture_reply_request( char *news,  char **outDataToUi)
{

	int ret = 0;
	char *tmp = NULL, *tmpOut = NULL;		//临时指针
	char *outDataNews = NULL;				//内存块

	//1.申请内存, 储存数据, 返回给UI
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func  mem_pool_alloc() ");		
		assert(0);
	}
	
	//2.进行数据拷贝
	sprintf(outDataNews, "%d~", UPLOADCMDRESPON);				
	tmpOut = outDataNews + strlen(outDataNews);			
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );

	//3.判断是否出错
	if(*tmp == 0x02)		
	{
		tmpOut = outDataNews + strlen(outDataNews);			
		tmp = (char *)news + sizeof(resCommonHead_t) + 1;
		sprintf(tmpOut, "%d~", *tmp );
	}


	*outDataToUi = outDataNews;

	return ret;
}



int push_info_from_server( char *news, char **outDataToUi)
{
	int  ret = 0;
	char *tmp = NULL;
	char *outDataNews = NULL;
	long long int fileID =	0;

	//1. alloc The memory block
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func  mem_pool_alloc() ");
		assert(0);
	}
	
	//2. get The fileType from server And fileID	
	tmp = (char *)news + sizeof(resCommonHead_t);
	memcpy(&fileID, tmp + 1, sizeof(long long int));
	if(*tmp == 0)					sprintf(outDataNews, "%d~%d~%lld~", PUSHINFORESPON, 0, fileID);				//1.缁勫懡浠ゅ瓧
	else if(*tmp == 1)				sprintf(outDataNews, "%d~%d~%lld~", PUSHINFORESPON, 1, fileID );				//1.缁勫懡浠ゅ瓧
	else 							assert(0);

	//3. send response To server
	recv_ui_data("7~");
	
	*outDataToUi = outDataNews;

	return ret;
}


/*deal with active shutdown sock
*@param : news   The reply content from server; style : head + body + checkCode;
*@param : lenth  The head + body lenth;
*@param : outDataNews reply data to UI;   style : cmd~struct content~
*@retval: 0: success;  -1: fail; 1: continue And not reply UI
*/
int active_shutdown_net_reply( char *news, char **outDataToUi)
{
	int ret = 0;
	char *tmp = NULL, *tmpOut = NULL;
	char *outDataNews = NULL;

	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func  mem_pool_alloc() ");
		assert(0);
	}

	sprintf(outDataNews, "%d~", ACTCLOSERESPON);						//1.命令字缓存
	tmpOut = outDataNews + strlen(outDataNews);				//2.处理结果
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );

	*outDataToUi = outDataNews;

	return ret;
}



/*deal with sclient request news reply 
*@param : news   The reply content from server; style : head + body + checkCode;
*@param : lenth  The head + body lenth;
*@param : outDataNews reply data to UI;   style : cmd~struct content~
*@retval: 0: success;  -1: fail; 1: continue And not reply UI
*/
int delete_spec_pic_reply( char *news, char **outDataToUi)
{
	int ret = 0;
	char *tmp = NULL, *tmpOut = NULL;
	char *outDataNews = NULL;

	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func  mem_pool_alloc() ");
		assert(0);
	}

	sprintf(outDataNews, "%d~", DELETECMDRESPON);				

	tmpOut = outDataNews + strlen(outDataNews);				
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );

	if(*tmp == 1)
	{
		tmpOut = outDataNews + strlen(outDataNews);				
		tmp += 1;
		sprintf(tmpOut, "%d~", *tmp );
	}

	*outDataToUi = outDataNews;

	return ret;
}

	
/*deal with sclient request news reply 
*@param : news   The reply content from server; style : head + body + checkCode;
*@param : outDataNews reply data to UI;   style : cmd~struct content~
*@retval: 0: success;  -1: fail; 1: continue And not reply UI
*/
int connet_server_reply( char *news, char **outDataToUi)
{
	int ret = 0;
	char *tmp = NULL, *tmpOut = NULL;
	char *outDataNews = NULL;

	//1.申请内存
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint( "Error: func  mem_pool_alloc() ");
		assert(0);
	}

	//2. 组装回复信息
	sprintf(outDataNews, "%d~", 0x8A);				
	tmpOut = outDataNews + strlen(outDataNews);				
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );
	
	*outDataToUi = outDataNews;

	return ret;
}


/*deal with sclient request news reply 
*@param : news   The reply content from server; style : head + body + checkCode;
*@param : lenth  The head + body lenth;
*@param : outDataNews reply data to UI;   style : cmd~struct content~
*@retval: 0: success;  -1: fail; 1: continue And not reply UI
*/
int read_config_file_reply( char *news, char **outDataToUi)
{
	int ret = 0;
	char *tmp = NULL, *tmpOut = NULL;
	char *outDataNews = NULL;
	
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func  mem_pool_alloc() ");
		assert(0);
	}


	sprintf(outDataNews, "%d~", MODIFYCMDRESPON);		

	tmpOut = outDataNews + strlen(outDataNews);				
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );


	*outDataToUi = outDataNews;

	return ret;
}

/*deal with client template Data response 
*@param : news   The reply content from server; style : head + body + checkCode;
*@param : lenth  The head + body lenth;
*@param : outDataNews reply data to UI;   style : cmd~struct content~
*@retval: 0: success;  -1: fail; 1: continue And not reply UI
*/
int template_extend_element_reply( char *news, char **outDataToUi)
{
	int ret = 0;
	char *tmp = NULL, *tmpOut = NULL;
	char *outDataNews = NULL;

	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint( "Error: func  mem_pool_alloc() ");
		assert(0);
	}

	sprintf(outDataNews, "%d~", TEMPLATECMDRESPON);				//1.储存数据命令包
	
	tmpOut = outDataNews + strlen(outDataNews); 				//2.储存处理结果
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );

	if(*tmp == 1)
	{
		tmpOut = outDataNews + strlen(outDataNews);				//3.储存失败原因
		tmp += 1;
		sprintf(tmpOut, "%d~", *tmp );
	}


	*outDataToUi = outDataNews;


	return ret;
}

