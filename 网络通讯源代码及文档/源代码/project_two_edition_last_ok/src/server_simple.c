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
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "memory_pool.h"
#include "queue_sendAddr_dataLenth.h"
#include "sock_pack.h"
#include "escape_char.h"
#include "socklog.h"
#include "server_simple.h"
#include "func_compont.h"
#include "template_scan_element_type.h"


#if 0
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#endif

//宏定义
#define 	ERRORINPUT												//测试上传图片出错
#define 	SCAN_SOURCE_DIR  "sourcefile"							//扫描目录
#define 	CERT_FILE 	"/home/yyx/nanhao/server/server-cert.pem" 	//证书路径
#define 	KEY_FILE  	"/home/yyx/nanhao/server/server-key.pem" 	//私钥路径
#define 	CA_FILE 	"/home/yyx/nanhao/ca/ca-cert.pem" 			//CA 证书路径
#define 	CIPHER_LIST "AES128-SHA"								//采用的加密算法
#define 	NUMBER		15											//同一时间可以接收的客户端数
#define     SCAN_TIME 	15											//扫描间隔时间 15s
#define		MY_MIN(x, y)	((x) < (y) ? (x) : (y))	

typedef struct _RecvAndSendthid_Sockfd_{
	pthread_t		pthidWrite;
	pthread_t		pthidRead;
	int 			sockfd;
	int 			flag;					// 0: UnUse;  1: use	
	sem_t			sem_send;				//The send thread And recv Thread synchronization
	bool			encryptRecvFlag;		//true : encrypt, false : No encrypt
	bool			encryptSendFlag;		//ture : send data In use encyprtl, false : In use common
	bool			encryptChangeFlag;		//true : once recv cmd=14, (deal with recv And send Flag(previous)), modify to False ; false : No change		
	int 			send_flag;				//push infomation to Ui serial Number
}RecvAndSendthid_Sockfd;



//全局变量
char 			g_DirPath[512] = { 0 };						//上传文件路径
unsigned int 	g_recvSerial = 0;							//服务器 接收的 数据包流水号
unsigned int 	g_sendSerial = 0;							//服务器 发送的 数据包流水号
long long int 	g_fileTemplateID = 562, g_fileTableID = 365;//服务器资源文件ID 
char 			*g_downLoadDir = NULL;						//服务器资源路径
char 			*g_fileNameTable = "downLoadTemplateNewestFromServer_17_05_08.pdf";	//数据库表名称
char 			*g_fileNameTemplate = "C_16_01_04_10_16_10_030_B_L.jpg";			//模板名称
int  			g_listen_fd = 0;							//监听套接字
RecvAndSendthid_Sockfd		g_thid_sockfd_block[NUMBER];	//客户端信息
char 			*g_sendFileContent = NULL;					//读取每轮下载模板的数据

//队列和内存池
mem_pool_t  *g_memoryPool = NULL;					//mempool handle
queue_send_dataBLock   g_sendData_addrAndLenth_queue;


//扫描线程, 全局独一份
pthread_t  g_pthid_scan_dir;

//互斥锁,
pthread_mutex_t	 g_mutex_pool = PTHREAD_MUTEX_INITIALIZER;			

//移除套接字缓冲区数据
int removed_sockfd_buf(int sockfd);			
//清空资源
void clear_global();			
//加密句柄初始化
//int ssl_server_init_handle(int socketfd, SSL_CTX **srcCtx, SSL **srcSsl);
//接收数据的整包查询
int	find_whole_package(char *recvBuf, int recvLenth, int index);


void signUSR1_exit_thread(int sign)
{
	int ret = 0;
	socket_log( SocketLevel[2],ret , "\nthread:%lu, receive signal:%u---", pthread_self(), sign);
	pthread_exit(NULL);
}

void func_pipe(int sig)
{
	int ret = 0;
	socket_log( SocketLevel[4], ret, "SIGPIPIE appear %d",  sig);	
}

void sign_usr2(int sign)
{
	int ret = 0;
	
	myprint("\n\n()()()()()()()recv signal : %d, pthread : %lu", sign, pthread_self());
	socket_log( SocketLevel[2],ret , "\nthread:%lu, receive signal:%u---", pthread_self(), sign);
}

int std_err(const char* name)
{
    perror(name);
    exit(1);
}


int init_server(char *ip, int port, char *uploadDirParh)
{
	int 			ret = 0, i = 0;
	int 			sfd, cfd;				//服务器监听套接字, 客户端链接套接字
	pthread_t 		pthidWrite = -1;		//发送数据线程ID
	pthread_t		pthidRead = -1;			//接收数据线程ID
    socklen_t 		clie_len;				//地址长度
	struct sockaddr_in clie_addr;			//客户端地址
	int 			conIndex = 0;
	static bool  	createFlag = false;		//是否为初始化, 扫描线程只创建一次
	//SSL_CTX 		*ctx = NULL;			//加密证书信息
	//SSL 			*ssl = NULL;			//加密句柄

	//1.查看上传目录是否存在
	if(access(uploadDirParh,  0) < 0 )
	{
		//2.不存在, 创建保存上传文件的目录
		if((ret = mkdir(uploadDirParh, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) < 0)
		{
			myprint("Error : func mkdir() : %s", uploadDirParh);
			assert(0);
		}
	}

	//3.进行路径处理
	memcpy(g_DirPath, uploadDirParh, strlen(uploadDirParh));
	myprint("uploadFilePath : %s", g_DirPath);
	memset(g_thid_sockfd_block, 0, sizeof(g_thid_sockfd_block));

	//4.注册信号
	signal(SIGUSR1, signUSR1_exit_thread);
	signal(SIGPIPE, func_pipe); //开启服务器 监听套接字

	//5.创建监听套接字
	if((ret = server_bind_listen_fd(ip, port, &sfd)) < 0)
	{
		myprint("Error : func server_bind_listen_fd()");
		assert(0);
	}
    clie_len = sizeof(clie_addr); 
	g_listen_fd = sfd;

	//6.初始化 内存池和队列
    if((g_memoryPool = mem_pool_init( 1500, 2800, &g_mutex_pool)) == NULL )
	{
		myprint("Err : func mem_pool_init() ");
		assert(0);	
	}
	if((g_sendData_addrAndLenth_queue = queue_init_send_block(1500)) == NULL)
	{
		myprint("Err : func queue_init_send_block() ");
		assert(0);	
	}
	g_sendFileContent = (char *)malloc(RESTEMSUBBODYLENTH * PERROUNDNUMBER);
	memset(g_sendFileContent, 0, RESTEMSUBBODYLENTH * PERROUNDNUMBER);
	
	//7.初始化客户端缓冲区信号量
	for(i = 0; i < NUMBER; i++)
	{
		sem_init(&(g_thid_sockfd_block[i].sem_send), 0, 0);
	}

	//8.获取扫描路径	
	if((g_downLoadDir = getenv("SERverSourceDir")) == NULL)
	{
		myprint("Err : func getenv() key = SERverSourceDir");
		assert(0);
	}

	//9.进行客户端链接处理
	while(1)
	{
		//10 阻塞等待客户端发起链接请求
	    if((cfd = accept(sfd, (struct sockaddr*)&clie_addr, &clie_len)) < 0)	   	    
	    {
		   	myprint("Err : func accept()");
			assert(0);
	    }
		else
		{
#if 0
			//11.创建加密句柄	
			if((ret = ssl_server_init_handle(cfd , &ctx, &ssl)) < 0)
			{
				close(cfd);
				myprint("\n\nErr : func ssl_server_init_handle()... "); 	
				return ret;
			}
#endif
		
			//12. 遍历客户端缓存区, 找出空闲的位置
			do
			{
				for(conIndex = 0; conIndex < NUMBER; conIndex++)
				{
					if(g_thid_sockfd_block[conIndex].flag == 0)
						break;
				}
				if(conIndex == NUMBER)
				{
					myprint("connect MAX 100 have full, will sleep 5s, try again ");
					sleep(5);
				}
			}while(conIndex == NUMBER);

			//13. 对客户端缓存区进行赋值
			g_thid_sockfd_block[conIndex].flag = 1;
			g_thid_sockfd_block[conIndex].sockfd= cfd;
	//		g_thid_sockfd_block[conIndex].sslCtx = ctx;
	//		g_thid_sockfd_block[conIndex].sslHandle = ssl;
			g_thid_sockfd_block[conIndex].encryptChangeFlag = false;
			g_thid_sockfd_block[conIndex].encryptRecvFlag = false;
			g_thid_sockfd_block[conIndex].encryptSendFlag = false;		
			g_thid_sockfd_block[conIndex].send_flag = 0;

			//14. 创建子线程， 去执行发送 业务。
			if((ret = pthread_create(&pthidWrite, NULL, child_write_func, (void *)conIndex)) < 0)			
			{
				myprint("err: func pthread_create() ");		
				assert(0);
			}
			//创建子线程， 去执行接收 业务。
			if((ret = pthread_create(&pthidRead, NULL, child_read_func, (void *)conIndex)) < 0)
			{
				myprint("err: func pthread_create() ");		
				assert(0);
			}

	#if 1	
			//15. 创建扫描线程, 只创建一次
			if(!createFlag)
			{
				//创建子线程， 去执行扫描目录业务, 进行信息推送
				if((ret = pthread_create(&g_pthid_scan_dir, NULL, child_scan_func, NULL)) < 0)			
				{
					myprint("err: func child_scan_func() "); 	
					assert(0);
				}
				createFlag = true;
			}
	#endif	

			//16. 对客户端缓存区进行赋值
			g_thid_sockfd_block[conIndex].pthidRead = pthidRead;
			g_thid_sockfd_block[conIndex].pthidWrite= pthidWrite;
	
		}

	}

	
	//4.销毁资源
	destroy_server();

	return ret;
}

//销毁服务
void destroy_server()
{	
	int i = 0;
	
	//1. 关闭监听套接字
	close(g_listen_fd);

	//2. 销毁客户端所有信号量
	for(i = 0; i < NUMBER; i++)
	{		
		sem_destroy(&(g_thid_sockfd_block[i].sem_send));		
	}	

	//3.销毁互斥锁
	pthread_mutex_destroy(&g_mutex_pool);

	//4.释放内存池和队列
	mem_pool_destroy(g_memoryPool);
	destroy_queue_send_block(g_sendData_addrAndLenth_queue);

	//5.释放全局变量
	if(g_sendFileContent)		free(g_sendFileContent);
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

void *child_scan_func(void *arg)
{
	pthread_detach(pthread_self());
	
	while(1)
	{
		timer_select(SCAN_TIME);
		find_directory_file();
	}

	pthread_exit(NULL);
}



//服务器消息推送
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int push_info_toUi(int index, int fileType)
{
	int ret = 0, outDataLenth = 0, checkNum = 0;
	char *tmp = NULL, *sendBufStr = NULL;
	resCommonHead_t head;
	char tmpSendBuf[1400] = { 0 };

	
	//myprint("The queue have element number : %d ...", get_element_count_send_block(g_sendData_addrAndLenth_queue));

	//2.alloc The memory block
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	//3.package body news
	tmp = tmpSendBuf + sizeof(resCommonHead_t);
	if(fileType == 0)			
	{
		*tmp++ = fileType;
		memcpy(tmp, &g_fileTemplateID, sizeof(long long int));
		g_fileTemplateID++;
	}
	else if(fileType == 1)
	{
		*tmp++ = fileType;
		memcpy(tmp, &g_fileTableID, sizeof(long long int));
		g_fileTableID++;
	}
	outDataLenth = sizeof(resCommonHead_t) + 65;	
		
	//4.package The head news	
	assign_comPack_head(&head, PUSHINFORESPON, outDataLenth, 0, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));		
	g_thid_sockfd_block[index].send_flag++;
	
	//5.calculation checkCode
	tmp = tmpSendBuf + head.contentLenth;
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//6. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//7. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;			
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, PUSHINFORESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	
End:
	if(ret < 0) 	if(sendBufStr)		mem_pool_free(g_memoryPool, sendBufStr);

	return ret;


}


void find_directory_file()
{
	char *direct = NULL;
	DIR *directory_pointer = NULL;
  	struct dirent *entry = NULL;
	struct stat  fileStat;
	static bool flag = false;
	int roundNum = 0, conIndex = 0, filyType = 0;
	char tableName[256] = { 0 }; 
	char templateName[256] = { 0 }; 
	char tmp01[256] = { 0 }, tmp02[256] = { 0 };
	
	struct FileModifyTime{
		char fileName[256];
		time_t modifyTime;
	};

	struct FileList{
		char fileName[256];
		struct FileList *next;
	}start, *node, *tmpNode;

	static struct FileModifyTime fileModifyTime[2];

	#if 1
	//1.获取目录
	if((direct = getenv("SERverSourceDir")) == NULL)
	{
		myprint("Err : func getenv() key = SERverSourceDir");
		assert(0);
	}
	g_downLoadDir = direct;
	#endif

	//2. 打开目录, 获取目录中的文件
	if((directory_pointer = opendir(g_downLoadDir)) == NULL)
	{
		myprint("Err : func opendir() Dirpath : %s", direct);
		assert(0);
	}
	else
	{
		//3. 获取目录中的文件
		start.next = NULL;
		node = &start;
		while((entry = readdir(directory_pointer)))
		{
			node->next = (struct FileList*)malloc(sizeof(struct FileList));
			node = node->next;
			memset(node->fileName, 0, 256);
			sprintf(node->fileName, "%s%s", g_downLoadDir, entry->d_name);	
			node->next = NULL;
		}
		closedir(directory_pointer);
	}
	
	sprintf(tmp01, "%s.", g_downLoadDir);
	sprintf(tmp02, "%s..", g_downLoadDir);
	sprintf(tableName, "%s%s", g_downLoadDir, g_fileNameTable);
	sprintf(templateName, "%s%s", g_downLoadDir, g_fileNameTemplate);
	//4.遍历获取的目录下的文件, 获取它们的时间属性
	node = start.next;
	while(node)
	{	
		//4.1 去除'.'和'..'目录
		if(strcmp(node->fileName, tmp01) == 0 || strcmp(node->fileName, tmp02) == 0)
		{
			node = node->next;
			continue;
		}
			
		//4.2 获取剩余文件属性
		memset(&fileStat, 0, sizeof(struct stat));
		if((stat(node->fileName, &fileStat)) < 0)
		{
			myprint("Err : func stat() filename : %s", node->fileName);
			assert(0);
		}

		//4.3 初次打开该目录, 拷贝各文件的属性至缓存
		if(!flag)
		{			
			fileModifyTime[roundNum].modifyTime = fileStat.st_atime;
			strcpy(fileModifyTime[roundNum].fileName, node->fileName);
			
			conIndex = 0;
			while(conIndex < NUMBER)
			{
				if(g_thid_sockfd_block[conIndex].flag == 1)
				{
					if((strcmp(fileModifyTime[roundNum].fileName, templateName)) == 0)
						push_info_toUi(conIndex, 0);
					else if((strcmp(fileModifyTime[roundNum].fileName, tableName)) == 0)
						push_info_toUi(conIndex, 1);						
				}				
				conIndex++;
			}
					
		}
		else		//非首次, 进行属性判断
		{				
			if((strlen(fileModifyTime[roundNum].fileName)) == 0 )
			{
				strcpy(fileModifyTime[roundNum].fileName, node->fileName);
				fileModifyTime[roundNum].modifyTime = fileStat.st_atime;
				if((strcmp(fileModifyTime[roundNum].fileName, templateName)) == 0)
				{
					filyType = 0;
				}	
				else if(strcmp(fileModifyTime[roundNum].fileName, tableName) == 0)
				{
					filyType = 1;		
				}
				else 
				{
					assert(0);
				}
				conIndex = 0;
				while(conIndex < NUMBER)
				{
					if(g_thid_sockfd_block[conIndex].flag == 1)		push_info_toUi(conIndex, filyType);
					conIndex++;
				}
				
			}
			else
			{		
				if((strcmp(fileModifyTime[roundNum].fileName, templateName)) == 0 && fileModifyTime[roundNum].modifyTime != fileStat.st_atime)				
				{
					filyType = 0;					
				}
				else if((strcmp(fileModifyTime[roundNum].fileName, tableName)) == 0 && fileModifyTime[roundNum].modifyTime != fileStat.st_atime)
				{
					filyType = 1;									
				}
				else
				{
					goto End;
				}
				conIndex = 0;
				while(conIndex < NUMBER)
				{
					if(g_thid_sockfd_block[conIndex].flag == 1) 	push_info_toUi(conIndex, filyType);
					conIndex++;
				}	
			
				fileModifyTime[roundNum].modifyTime = fileStat.st_atime;
			}
					
		}			
	End:			
		roundNum++;
		node = node->next;
	
	}
	flag = true;
	
#if 1
	node = start.next;
	while(node)
	{
		tmpNode = node;
		node = node->next;
		free(tmpNode);			
	}
#endif	
	
}


//发送数据
void *child_write_func(void *arg)
{
	int ret = 0, j = 1;	
	char *sendDataAddr = NULL;
	int  sendDataLenth = 0;
	int  writeLenth = 0;
	int index = (int)arg;
	int clientFd = g_thid_sockfd_block[index].sockfd;
	int nWrite = 0;
	int sendCmd = 0;
	myprint("New thread will Send ...... ");
		
	while(1)
	{
		//sleep(10000);

		//1. 等待信号量通知
		sem_wait(&(g_thid_sockfd_block[index].sem_send));		
		
		//2. 获取发送数据的地址和长度
		if((ret = pop_queue_send_block(g_sendData_addrAndLenth_queue, &sendDataAddr, &sendDataLenth, &sendCmd)) < 0)
		{
			myprint("Err : func pop_queue_send_block()");
			assert(0);
		}
//goto Test;
		#if 0
		if(flag <= 4 * 1016 - 1 )
		{
			flag++;
			printf("j : %d\n", j++);
			continue;
		}
		#endif

		//3. 选择发送数据的方式, 进行数据发送
		if(!g_thid_sockfd_block[index].encryptSendFlag)
	    {
	    	writeLenth = write(clientFd, sendDataAddr, sendDataLenth);
		}
		else
		{
			//writeLenth = SSL_write(g_thid_sockfd_block[index].sslHandle, sendDataAddr, sendDataLenth);   
		}		
        if(writeLenth < 0)
        {        	
        	myprint("ERror: server send data, sockfd : %d, sendData : %s, sendDataLenth : %d ", clientFd, sendDataAddr, sendDataLenth);     
       		goto End;
        }	
	    else if(writeLenth == sendDataLenth)
        {        
	        nWrite += writeLenth;
//Test:
			//4. 发送加密应答后,立即启动接收线程 的加密接收 
			if(g_thid_sockfd_block[index].encryptChangeFlag)
			{
				g_thid_sockfd_block[index].encryptSendFlag = g_thid_sockfd_block[index].encryptRecvFlag;
				g_thid_sockfd_block[index].encryptChangeFlag = false;
				//2.1.1. send signal To thread recv
				myprint("will kill USR2 SIGNAL to read thread ID : %lu", g_thid_sockfd_block[index].pthidRead);
				if((ret = pthread_kill(g_thid_sockfd_block[index].pthidRead, SIGUSR2)) < 0)		assert(0);				
			}

			//5. 释放内存至内存池
			if((ret = mem_pool_free(g_memoryPool, sendDataAddr)) < 0 )
			{
				myprint("Err : func mem_pool_free()");
				assert(0);
			}
        }
        else if(writeLenth == 0)
        {	        	
        	myprint("client closed!!! ");            	
       		goto End;
        }
		else
		{
			myprint("server write : %dj = [%d]	", writeLenth, j++);													
			assert(0);
		}
		
	}

End:	

	//6.关闭客户端链接套接字
	//if(g_thid_sockfd_block[index].encryptSendFlag)			SSL_shutdown(g_thid_sockfd_block[index].sslHandle);   
	//else
	close(clientFd);
#if 0
	SSL_free(g_thid_sockfd_block[index].sslHandle); 
	SSL_CTX_free(g_thid_sockfd_block[index].sslCtx);
#endif
	myprint("Thread send will close ...... ");
	pthread_exit(NULL);
	return NULL;
}


//子线程执行逻辑;  返回值: 0:标识程序正常, 1: 标识客户端关闭;  -1: 标识出错.
void *child_read_func(void *arg)
{
	int ret = 0, index = (int)arg;
	int i = 1, recvLen = 0;
	char recvBuf[10240] = { 0 };
	int clientFd = g_thid_sockfd_block[index].sockfd;
	
	pthread_detach(pthread_self());

	myprint("New Thread will recv ......");
	printf("\n\n");
	
	signal(SIGUSR2, sign_usr2);

    //1. 接收线程工作逻辑,
    while(1)
    {  
    	memset(recvBuf, 0 ,  sizeof(recvBuf));

		//2.选择接收数据的方式
		if(!g_thid_sockfd_block[index].encryptRecvFlag)
	    {
	    	recvLen = read(clientFd, recvBuf, sizeof(recvBuf));
		}
		else
		{
			//recvLen = SSL_read(g_thid_sockfd_block[index].sslHandle, recvBuf, sizeof(recvBuf));  
		}
		if(recvLen > 0)		
        {    
	        //3. 正确接收数据, 进行数据整包处理
  //      	 myprint("fucn read() recv data; recvLen : %d,  recv package index : i = %d, encryptRecvFlag : %d",
//			 	recvLen, i, g_thid_sockfd_block[index].encryptRecvFlag);          
			
			 if((find_whole_package(recvBuf, recvLen, index)) < 0)
			 {
			 	printf("\n\n");
				myprint("Err : func find_whole_package()");				
				printf("\n\n");
				break;	
			 }
	                 	
        	 i++;
        }
       	else if(recvLen < 0)       	//The connect sockfd Error
       	{
       		//4. 接收出错
       		if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
       		{
       			myprint("************** ()()()()() server: errno == EINTR || errno == EAGAIN ");         		
    			continue;  				
       		}
       		else 
       		{
       			myprint("ERror: server recv data ");
				g_thid_sockfd_block[index].flag = 0;
				clear_global();
       			break;	
       		}       		
       	}
       	else if(recvLen == 0)		//client close
       	{
       		//5. 客户端关闭服务
       		myprint("client closed ");
			g_thid_sockfd_block[index].flag = 0;
			clear_global();
       		break;
       	}	      	
	
	} 

	//6. 关闭安全套接字层和 普通套接字
	if(g_thid_sockfd_block[index].encryptRecvFlag)
	{
		myprint("ssl_shurdown ssl sockfd");
//		SSL_shutdown(g_thid_sockfd_block[index].sslHandle);            
	}
	else
	{
		myprint("close common sockfd");
		close(clientFd);		
	}
#if 0
	SSL_free(g_thid_sockfd_block[index].sslHandle); 
	SSL_CTX_free(g_thid_sockfd_block[index].sslCtx);
#endif
	myprint("\n\n close  Thread recv .....");

	//7. 发信号, 关闭发送线程
	if((ret = pthread_kill(g_thid_sockfd_block[index].pthidWrite, SIGUSR1)) < 0)		assert(0);					
	pthread_exit(NULL);
	return NULL;
}


//function: find  the whole packet; retval maybe error
int	find_whole_package(char *recvBuf, int recvLenth, int index)
{
	int ret = 0, i = 0;
	int  tmpContentLenth = 0;			//转义后的数据长度
	char tmpContent[1400] = { 0 }; 		//转义后的数据
	static int flag = 0;				//flag 标识找到标识符的数量;
	static int lenth = 0; 				//lenth : 缓存的数据包内容长度
	static char content[2800] = { 0 };	//缓存完整包数据内容, 		
	char *tmp = NULL;


	tmp = content + lenth;
	for(i = 0; i < recvLenth; i++)
	{
		//1. 查找数据报头
		if(recvBuf[i] == 0x7e && flag == 0)
		{
			flag = 1;
			continue;
		}
		else if(recvBuf[i] == 0x7e && flag == 1)	
		{		

			//2. 查找数据报尾, 获取到完整数据包, 将数据包进行反转义 
			if((ret = anti_escape(content, lenth, tmpContent, &tmpContentLenth)) < 0)
			{
				myprint("Err : func anti_escape(), srcLenth : %d, dstLenth : %d",lenth,tmpContentLenth);
				goto End;
			}

			//3.将转义后的数据包进行处理
			if((ret = read_data_proce(tmpContent, tmpContentLenth, index)) < 0)
			{
				myprint("Err : func read_data_proce(), dataLenth : %d", tmpContentLenth);
				goto End;
			}
			else
			{
				//4. 数据包处理成功, 清空临时变量
				memset(content, 0, sizeof(content));
				memset(tmpContent, 0, sizeof(tmpContent));								
				lenth = 0;
				flag = 0;	
				tmp = content + lenth;
			}
		}
		else
		{
			//5.取出数据包内容, 去除起始标识符
			*tmp++ = recvBuf[i];
			lenth += 1;
			continue;
		}
	}


End:

	
	return ret;
}




//接收客户端的数据包, recvBuf: 包含: 标识符 + 报头 + 报体 + 校验码 + 标识符;   recvLen : recvBuf数据的长度
//				sendBuf : 处理数据包, 将要发送的数据;  sendLen : 发送数据的长度
int read_data_proce(char *recvBuf, int recvLen, int index)
{
	int ret = 0 ;	
	uint8_t  cmd = 0;
	
	//1.获取命令字
	cmd = *((uint8_t *)recvBuf);
			
	//2.选择处理方式
	switch (cmd){
		case LOGINCMDREQ: 		//登录数据包
			if((ret = data_un_packge(login_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func login_func_reply() ");				
			break;			
		case LOGOUTCMDREQ: 		//退出数据包
			if((ret = data_un_packge(exit_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func exit_func_reply() ");				
			break;
		case HEARTCMDREQ: 		//心跳数据包
			if((ret = data_un_packge(heart_beat_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func heart_beat_func_reply() ");
			break;			
		case DOWNFILEREQ: 		//模板下载			
			if((ret = data_un_packge(downLoad_template_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func downLoad_template_func_reply() ");
			break;					
		case NEWESCMDREQ: 		//客户端信息请求
			if((ret = data_un_packge(get_FileNewestID_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func client_request_func_reply() ");
			break;
		case UPLOADCMDREQ: 		//上传图片数据包
			if((ret = data_un_packge(upload_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func upload_func_reply() ");
			break;			
		case PUSHINFOREQ:		//消息推送
			if((ret = data_un_packge(push_info_toUi_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func push_info_toUi_reply() ");
			break;
		case DELETECMDREQ: 		//删除图片数据包
			if((ret = data_un_packge(delete_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func delete_func_reply() ");
			break;		
		case TEMPLATECMDREQ:		//上传模板
			if((ret = data_un_packge(template_extend_element_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func template_extend_element() ");
			break;			
		case MUTIUPLOADCMDREQ:		//上传图片集
			if((ret = data_un_packge(upload_template_set_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func upload_template_set ");
			break;
#if 0	
		case ENCRYCMDREQ:
			if((ret = data_un_packge(encryp_or_cancelEncrypt_transfer, recvBuf, recvLen, index)) < 0)
				myprint("Error: func encryp_or_cancelEncrypt_transfer_reply ");
			break;
#endif			
		default:			
			myprint("recvCmd : %d", cmd);
			assert(0);			
	}
	
	
	return ret;
}


int data_un_packge(call_back_un_pack func_handle, char *recvBuf, int recvLen, int index)
{
	int ret = 0;
	
	if( (ret = func_handle(recvBuf, recvLen, index)) < 0)		
		myprint("Error:  func func_handle() ");
		
	return ret;
}

void printf_comPack_news(char *buf)
{
	reqPackHead_t *tmpHead = NULL;

	tmpHead = (reqPackHead_t *)buf;
	printf("The package cmd : %d, indexNumber : %d, totalNumber : %d, contentLenth : %d, seriaNumber : %u, [%d], [%s] \n",tmpHead->cmd, tmpHead->currentIndex, 
			tmpHead->totalPackage, tmpHead->contentLenth, tmpHead->seriaNumber, __LINE__, __FILE__);
	
}


void assign_comPack_head(resCommonHead_t *head, int cmd, int contentLenth, int isSubPack, int isFailPack, int failPackIndex, int isRespond)
{
	memset(head, 0, sizeof(resCommonHead_t));
	head->cmd = cmd;
	head->contentLenth = contentLenth;
	head->isSubPack = isSubPack;
	head->isFailPack = isFailPack;
	head->failPackIndex = failPackIndex;
	head->isRespondClient = isRespond;
	head->seriaNumber = g_sendSerial++;	
}



//登录应答数据包
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int login_func_reply(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0;					//发送数据的长度
	int  checkNum = 0;								//校验码
	char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
	char *sendBufStr = NULL;						//发送数据地址
	char *tmp = NULL;
	resCommonHead_t head;							//应答数据报头
	reqPackHead_t *tmpHead = NULL;					//请求信息
	char *userName = "WuKong";						//返回账户的用户名
	
	//1.打印接收图片的信息
	printf_comPack_news(recvBuf);	
	tmpHead = (reqPackHead_t *)recvBuf;
	g_recvSerial = tmpHead->seriaNumber;
	g_recvSerial++;

	//2.申请内存
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//3.组装数据包信息, 获取发送数据包长度
	tmp = tmpSendBuf + sizeof(resCommonHead_t);
	*tmp++ = 0x01;				//登录成功
	*tmp++ = 0x00;				//成功后, 此处信息不会进行解析, 随便写
	*tmp++ = strlen(userName);	//用户名长度
	memcpy(tmp, userName, strlen(userName));	//拷贝用户名	
	outDataLenth = sizeof(resCommonHead_t) + sizeof(char) * 3 + strlen(userName);
	
	//4.组装报头信息		
	assign_comPack_head(&head, LOGINCMDRESPON, outDataLenth, 0, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));				
	//myprint("outDataLenth : %d, head.contentLenth : %d", outDataLenth, head.contentLenth);
	
	//5.组合校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp = tmpSendBuf + head.contentLenth;
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//6. 转义数据内容
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//7. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	

	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, LOGINCMDRESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		goto End;
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));

End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
	
}



//退出登录应答 数据包
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int exit_func_reply(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0;					//发送数据的长度
	int  checkNum = 0;								//校验码
	char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
	char *sendBufStr = NULL;						//发送数据地址
	char *tmp = NULL;
	resCommonHead_t head;							//应答数据报头

	//1. 打印数据包信息		
	printf_comPack_news(recvBuf);			//打印接收图片的信息

	//2. 申请内存
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	//3. 数据信息拷贝
	tmp = tmpSendBuf + sizeof(resCommonHead_t);
	*tmp++ = 0x01;	
	outDataLenth =  sizeof(resCommonHead_t) + 1;
	
	//4. 组装报头
	assign_comPack_head(&head, LOGOUTCMDRESPON, outDataLenth, 0, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));		
	
	//5. 计算校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//6. 转义数据内容
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//7. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	
		
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, LOGOUTCMDRESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		goto End;
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	
End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
}


//心跳数据包	3号数据包应答		
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int heart_beat_func_reply(char *recvBuf, int recvLen, int index)
{		
	int  ret = 0, outDataLenth = 0; 				//发送数据的长度
	int  checkNum = 0;								//校验码
	char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
	char *sendBufStr = NULL;						//发送数据地址
	char *tmp = NULL;
	resCommonHead_t head;							//应答数据报头
	

	//1. 打印数据包信息
	printf_comPack_news(recvBuf);			

	//2. 申请内存
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	//3. 组装报头信息
	outDataLenth = sizeof(resCommonHead_t);
	assign_comPack_head(&head, HEARTCMDRESPON, outDataLenth, 0, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));	
	
	//4. 计算校验码
	tmp = tmpSendBuf + sizeof(resCommonHead_t);
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//5. 转义数据内容
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//6. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	
	
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, HEARTCMDRESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		goto End;
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));

End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;	
}


void assign_serverSubPack_head(resSubPackHead_t *head, uint8_t cmd, int contentLenth, int currentIndex, int totalNumber )
{
	memset(head, 0, sizeof(resSubPackHead_t));
	head->cmd = cmd;
	head->contentLenth = contentLenth;
	head->totalPackage = totalNumber;
	head->currentIndex = currentIndex;
	head->seriaNumber = g_sendSerial++;
}


//下载模板数据包	4号数据包应答		
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码  
//recvLen : 长度为 buf 的内容 长度
int downLoad_template_func_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0;
	resSubPackHead_t 	head;					//应答数据包报头
	char *tmp = NULL, *sendBufStr = NULL;
	int contentLenth = 0;
	int checkNum = 0, nRead = 0;				//校验码和 读取字节数
	char tmpSendBuf[1400] = { 0 };				//临时缓冲区
	int  doucumentLenth = 0; 
	FILE *fp = NULL;
	char fileName[1024] = { 0 };
	int totalPacka = 0;							//该文件的总包数
	int indexNumber = 0;						//每包的序号
	int fileType = 0;							//下载的文件类型		
	int  tmpLenth = 0;
	
	//1. 打印接收数据包信息
	printf_comPack_news(recvBuf);

	//2. get The downLoad file Type And downLoadFileID
	tmp = recvBuf + sizeof(reqPackHead_t);
	if(*tmp == 0)			sprintf(fileName, "%s%s",g_downLoadDir, g_fileNameTemplate);		//下载模板
	else if(*tmp == 1)		sprintf(fileName, "%s%s",g_downLoadDir, g_fileNameTable);			//下载数据库表
	else					assert(0);
	fileType = *tmp;
	tmp++;
	myprint("downLoadFileName : %s", fileName);

	
	//3. open The template file
	if((fp = fopen(fileName, "rb+")) == NULL)
	{
		ret = -1;
		myprint("Err : func fopen() : %s", fileName);
		goto End;
	}

	//4. get The file Total Package Number 
	doucumentLenth = RESTEMSUBBODYLENTH -  sizeof(char) * 3;
	if((ret = get_file_size(fileName, &totalPacka, doucumentLenth, NULL)) < 0)
	{
		myprint("Err : func get_file_size()");
		goto End;
	}


	//5. read The file content	
	while(!feof(fp))
	{	
		 //6. get The part of The file conTent	And package The body News
		 memset(tmpSendBuf, 0, sizeof(tmpSendBuf));
		 tmp = tmpSendBuf + sizeof(resSubPackHead_t);
		 *tmp++ = 0;
		 *tmp++ = 2;
		 *tmp++ = fileType;	
		 
		 while(nRead < doucumentLenth && !feof(fp))
		 {
			 if((tmpLenth = fread(tmp + nRead, 1, doucumentLenth - nRead, fp)) < 0)
			 {
				 myprint("Error: func fread() : %s ", fileName);
				 ret= -1;
				 goto End;
			 }
			 nRead += tmpLenth;
		 }
		 
			 
		 //7. The head  of The package
		 contentLenth = sizeof(resSubPackHead_t) + nRead + 3 * sizeof(char);		 
		 assign_serverSubPack_head(&head, DOWNFILEZRESPON, contentLenth, indexNumber, totalPacka);
		 indexNumber += 1;
		 memcpy(tmpSendBuf, &head, sizeof(resSubPackHead_t));
		
		 //8. calculate checkCode
		 checkNum = crc326(tmpSendBuf,  contentLenth);
		 memcpy(tmpSendBuf + contentLenth, &checkNum, sizeof(int));		 				  

		 //5.4 allock The block buffer in memory pool
		 if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
		 {			 
			 ret = -1;
			 myprint("Error: func mem_pool_alloc()");
			 goto End;
	 	 }
	
		 //5.6 escape the buffer
		 *sendBufStr = PACKSIGN;							 //flag 
		 if((ret = escape(PACKSIGN, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr+1, &outDataLenth)) < 0)
		 {
			 myprint( "Error: func escape()");
			 goto End;
		 }
		 *(sendBufStr + outDataLenth + 1) = PACKSIGN; 	 //flag 

		 //5.7 push The sendData addr and sendLenth in queuebuf
		 if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + sizeof(char) * 2, DOWNFILEZRESPON)) < 0)
		 {
			 myprint( "Error: func push_queue_send_block()");
			 fclose(fp);
			 mem_pool_free(g_memoryPool, sendBufStr);
			 return ret;
		 }
		
		 nRead = 0;		
		 sem_post(&(g_thid_sockfd_block[index].sem_send));
		 
	}
	

End:
	if(fp)		fclose(fp);
	return ret;
}


//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符); 
//recvLen : 长度为 buf 的内容 长度
int get_FileNewestID_reply(char *recvBuf, int recvLen, int index)
{	
	int  ret = 0, outDataLenth = 0; 				//发送数据的长度
	int  checkNum = 0;								//校验码
	char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
	char *sendBufStr = NULL;						//发送数据地址
	char *tmp = NULL;
	resCommonHead_t head;							//应答数据报头
	int  fileType = 0;								//请求的文件类型


	//1. 打印数据包信息
	//printf_comPack_news(recvBuf);	
	
	//1. 获取文件类型
	tmp = recvBuf + sizeof(reqPackHead_t);
	if(*tmp == 0)				fileType = 0;
	else if(*tmp == 1)			fileType = 1;
	else						assert(0);

	myprint("fileType : %d", fileType);
	
	//2. 申请内存
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//3. 组装数据包信息
	tmp = tmpSendBuf + sizeof(resCommonHead_t);
	if(fileType == 0)			
	{
		*tmp++ = fileType;
		memcpy(tmp, &g_fileTemplateID, sizeof(long long int));		
	}
	else if(fileType == 1)
	{
		*tmp++ = fileType;
		memcpy(tmp, &g_fileTableID, sizeof(long long int));		
	}
	outDataLenth =  sizeof(resCommonHead_t) + 65;
		
	//4. 组装报头信息	
	assign_comPack_head(&head, NEWESCMDRESPON, outDataLenth, 0, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));		
	
	//5. 计算校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp += 64;
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//6. 转义数据内容
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//7. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;			
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, NEWESCMDRESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		goto End;
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	
End:
	if(ret < 0)		if(sendBufStr)		mem_pool_free(g_memoryPool, sendBufStr);

	return ret;
}


//上传图片应答 数据包;  服务器收一包 回复一包.
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int upload_func_reply(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0;					//发送数据的长度
	int  checkNum = 0;								//校验码
	char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
	char *sendBufStr = NULL;						//发送数据地址
	char *tmp = NULL;
	resCommonHead_t head;							//应答数据报头
	reqPackHead_t  *tmpHead = NULL;					//请求报头信息
	static bool latePackFlag = false;					//true : 图片最后一包, false 非最后一包数据,  
	static int recvSuccessSubNumber = 0;				//每轮成功接收的数据包数目

	
	//1.打印接收图片的信息
	//printf_comPack_news(recvBuf);					

	//3.获取报头信息
	tmpHead = (reqPackHead_t *)recvBuf;
	
	//4.进行请求信息类型判断
	if(tmpHead->isSubPack == 1)
	{
		//5.子包, 进行数据包信息判断
		if((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
		{
			//6.数据正确, 进行图片数据的存储, 记录本轮成功接收的子包数量
			recvSuccessSubNumber += 1;
#if 1
			if((ret = save_picture_func(recvBuf, recvLen)) < 0)			
			{
				printf("error: func save_picture_func() !!! [%d], [%s] \n", __LINE__, __FILE__);
				goto End;
			}
#endif
			if(tmpHead->currentIndex + 1 == tmpHead->totalPackage)
			{
				latePackFlag = true;
			}

			if(recvSuccessSubNumber < tmpHead->perRoundNumber)
			{
				//7.标识每轮成功接收到的子包数目小于发送数目, 不需要进行进行回复				
				ret = 0;
				goto End;
			}
			else if(recvSuccessSubNumber == tmpHead->perRoundNumber)
			{
				//8.标识每轮所有的子包成功接收, 进行判断该回复哪种应答(该图片成功接收应答还是本轮成功接收应答)
				recvSuccessSubNumber = 0;
				if(latePackFlag == false)
				{
					//本轮成功接收					
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);				
				}
				else if(latePackFlag == true)
				{
					//该图片成功接收
					latePackFlag = false;
					tmp = tmpSendBuf + sizeof(resCommonHead_t);
					*tmp++ = 0x01;	
					outDataLenth = sizeof(resCommonHead_t) + 1;
					assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 1);									
				}									
			}
			
		}
		else if(ret == -1)
		{
			//9.需要重发			
			outDataLenth = sizeof(resCommonHead_t);
			assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);							
		}
		else
		{
			//10.不需要重发
			ret = 0;
			goto End;
		}	
	}
	else
	{		
		//11.总包, 立即回应.
		outDataLenth = sizeof(resCommonHead_t);
		assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
	}

	//12.申请内存
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//12. 拷贝报头数据, 计算校验码
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));			
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp = tmpSendBuf + head.contentLenth;
	memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

	//13. 转义数据内容
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)	
	{
		myprint("Error : func escape() escape() !!!!");
		goto End;
	}
	
	//14. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, UPLOADCMDRESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		goto End;
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
		
		
End:
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
}



int push_info_toUi_reply(char *recvBuf, int recvLen, int index)
{
	myprint("push infomation OK ... ");
	return 0;
}

//删除图片应答 数据包			9 号数据包
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int delete_func_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0;					//发送数据包长度
	int checkNum = 48848748;						//校验码
	char tmpSendBuf[1500] = { 0 };					//临时缓冲区
	char *tmp = NULL, *sendBufStr = NULL;			//发送数据包地址
	char fileName[128] = { 0 };						//文件名称
	char rmFilePath[512] = { 0 };					//删除命令
	resCommonHead_t head;							//应答数据报头
	
	//1. 打印数据包内容
	printf_comPack_news(recvBuf);			

	//2. 申请内存
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//3. 获取文件信息, 并删除
	memcpy(fileName, recvBuf + sizeof(reqPackHead_t), recvLen - sizeof(reqPackHead_t) - sizeof(int));
	sprintf(rmFilePath, "rm %s/%s", g_DirPath, fileName);	
	ret = system(rmFilePath);
	myprint("rmFilePath : %s", rmFilePath);
	//4. 组装数据包信息
	tmp = tmpSendBuf + sizeof(resCommonHead_t);		
	if(ret < 0 || ret == 127)
	{		
		*tmp++ = 0x01;	
		*tmp++ = 0;
		outDataLenth = sizeof(resCommonHead_t) + 2;
	}
	else
	{
		*tmp++ = 0x00;	
		outDataLenth = sizeof(resCommonHead_t) + 1;
	}
		
	//5. 组装报头
	assign_comPack_head(&head, DELETECMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
	
	//6. 组合校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//7. 转义数据内容
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//6. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	

	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, DELETECMDRESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		goto End;
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));

End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
}

//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码
void printf_template_news(char *recvBuf)
{
	reqPackHead_t *tmpHead = NULL;
	unsigned char IDLen = 0;
	char ID[128] = { 0 };
	unsigned char objectLen = 0;
	char object[400] = { 0 };
	unsigned char fileNum = 0;
	char fileName[10][50];
	unsigned char subMapCoordNum;
	//主观切图个数
	Coords coorArray[50];
	char *tmp = NULL;
	int i = 0;
	UitoNetExFrame *tmpStruct = NULL;
	
	memset(fileName, 0, sizeof(fileName));	
	memset(coorArray, 0,  sizeof(coorArray));
	
	tmpHead = (reqPackHead_t*)recvBuf;	
	printf("The package cmd : %d, indexNumber : %d, totalNumber : %d, contentLenth : %d, seriaNumber : %u, [%d], [%s] \n",tmpHead->cmd, tmpHead->currentIndex,
			tmpHead->totalPackage, tmpHead->contentLenth + sizeof(int), tmpHead->seriaNumber, __LINE__, __FILE__);	
	//1. 获取ID
	tmp = recvBuf + sizeof(reqPackHead_t);
	IDLen = *((unsigned char*)tmp);	
	tmp += 1;
	memcpy(ID, tmp, IDLen);	
	//2. 获取客观题	
	tmp += IDLen;	
	objectLen = *((unsigned char*)tmp);
	tmp += 1;	
	memcpy(object, tmp, objectLen);	
	//3. 获取试卷名称	
	tmp += objectLen;
	fileNum = *((unsigned char*)tmp);
	tmp += 1;	
	for(i = 0; i < fileNum; i++)
	{		
		memcpy(fileName[i], tmp, sizeof(tmpStruct->sendData.toalPaperName[0]));		
		tmp += sizeof(tmpStruct->sendData.toalPaperName[0]);
	}	
	//4. 获取主观题
	subMapCoordNum = *((unsigned char*)tmp);	
	tmp += 1;	
	for(i = 0; i < subMapCoordNum; i++)	
	{	
		memcpy(&coorArray[i], tmp, sizeof(Coords));	
		tmp += sizeof(Coords);
	}	

	//6. 打印	
	myprint("IDLEN : %d, ID : %s", IDLen, ID);	
	myprint("objectLen : %d, object : %s", objectLen, object);
	myprint("fileNum : %d", fileNum);
	for(i = 0; i < fileNum; i++)	
	{		
		myprint("paperIndex : %d, name : %s", i+1, fileName[i]);
	}	
	myprint("subMapCoordNum : %d", subMapCoordNum);	
	for(i = 0; i < subMapCoordNum; i++)	
	{	
		myprint("coorIndex : %d, coorMapID : %d, coors{%d, %d, %d, %d}", i+1, coorArray[i].MapID,
			coorArray[i].TopX, coorArray[i].TopY, coorArray[i].DownX, coorArray[i].DownY);
	}
}


//模板扩展数据包 数据包			12 号数据包应答
/*@param : recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  
*@param :  recvLen : 长度为 buf 的内容 长度
*@param :  sendBuf : 发送 client 的数据内容
*@param :  sendLen : 发送 数据内容的长度
*/
int template_extend_element_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0;				//发送数据包长度
	resCommonHead_t head;						//发送数据报头
	char *tmp = NULL, *sendBufStr = NULL;		//发送地址
	unsigned int contentLenth = 0;				//发送数据包长度
	int checkNum = 0;							//校验码
	char tmpSendBuf[1500] = { 0 };				//临时缓冲区

	//1. 打印接收图片的信息, 申请内存
	printf_template_news(recvBuf);
	
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
			
	//2. 组包消息头
	contentLenth = sizeof(resCommonHead_t) + sizeof(char) * 1;
	assign_comPack_head(&head, TEMPLATECMDRESPON, contentLenth, NOSUBPACK, 0, 0, 1);
	
	//3. 组包消息体, 按接收成功处理
	tmp = tmpSendBuf + sizeof(resCommonHead_t); 
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
	*tmp++ = 0;
	
	//4. 组合校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth );
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//5. 转义数据内容
	if((ret = escape(0x7e, tmpSendBuf, contentLenth + sizeof(uint32_t), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//6. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	

	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, TEMPLATECMDRESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		goto End;
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;	
}


//模板图片集合 数据包	13号数据包应答		
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int upload_template_set_reply(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0; 				//发送数据的长度
	int  checkNum = 0;								//校验码
	char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
	char *sendBufStr = NULL;						//发送数据地址
	char *tmp = NULL;
	resCommonHead_t head;							//应答数据报头
	reqPackHead_t  *tmpHead = NULL; 				//请求报头信息
	static bool latePackFlag = false;				//true : 本模板最后一包图片,最后一个数据包, false 非最后一包数据,  
	static int recvSuccessSubNumber = 0;			//每轮成功接收的数据包数目

		
	//1.打印接收图片的信息
	//printf_comPack_news(recvBuf); 				

	//3.获取报头信息
	tmpHead = (reqPackHead_t *)recvBuf;
	
	//4.进行请求信息类型判断
	if(tmpHead->isSubPack == 1)
	{
		//5.子包, 进行数据包信息判断
		if((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
		{
			//6.数据正确, 进行图片数据的存储, 记录本轮成功接收的子包数量
			recvSuccessSubNumber += 1;
#if 1
			if((ret = save_multiPicture_func(recvBuf, recvLen)) < 0) 		
			{
				printf("error: func save_picture_func() !!! [%d], [%s] \n", __LINE__, __FILE__);
				goto End;
			}
#endif
			tmp = recvBuf + sizeof(reqPackHead_t);
			if(tmpHead->currentIndex + 1 == tmpHead->totalPackage && *tmp + 1 == *(tmp + 1) )
			{
				latePackFlag = true;
			}

			if(recvSuccessSubNumber < tmpHead->perRoundNumber)
			{
				//7.标识每轮成功接收到的子包数目小于发送数目, 不需要进行进行回复				
				ret = 0;
				goto End;
			}
			else if(recvSuccessSubNumber == tmpHead->perRoundNumber)
			{
				//8.标识每轮所有的子包成功接收, 进行判断该回复哪种应答(该图片成功接收应答还是本轮成功接收应答)
				recvSuccessSubNumber = 0;
				if(latePackFlag == false)
				{
					//本轮成功接收					
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);				
				}
				else if(latePackFlag == true)
				{
					//所有图片都成功接收
					latePackFlag = false;
					tmp = tmpSendBuf + sizeof(resCommonHead_t);
					*tmp++ = 0x00;	
					outDataLenth = sizeof(resCommonHead_t) + 1;
					assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 1);									
				}									
			}
				
		}
		else if(ret == -1)
		{
			//9.需要重发			
			outDataLenth = sizeof(resCommonHead_t);
			assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);							
		}
		else
		{
			//10.不需要重发
			ret = 0;
			goto End;
		}	
	}
	else
	{		
		//11.总包, 立即回应.
		outDataLenth = sizeof(resCommonHead_t);
		assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
	}

	//12.申请内存
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	//13. 拷贝报头数据, 计算校验码
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t)); 		
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp = tmpSendBuf + head.contentLenth;
	memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

	//14. 转义数据内容
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)	
	{
		myprint("Error : func escape() escape() !!!!");
		goto End;
	}
		
	//15. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, MUTIUPLOADCMDRESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		goto End;
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
						
End:
	if(ret < 0 && sendBufStr)		mem_pool_free(g_memoryPool, sendBufStr);
	
	return ret;	
}


/*比较数据包接收是否正确
*@param : tmpContent		数据包转义后的数据
*@param : tmpContentLenth	转义后数据的长度, 包含: 报头 + 信息 + 校验码
*@retval: success 0, repeat -1, NoRespond -2
*/
int compare_recvData_correct_server(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	int checkCode = 0;					//本地计算校验码
	int *tmpCheckCode = NULL;			//缓存服务器的校验码
	uint8_t cmd = 0;					//接收数据包命令
	reqPackHead_t  *tmpHead = NULL;		//请求报头
	reqTemplateErrHead_t  *Head = NULL;	//下载模板请求报头


	//1.获取接收数据包的命令字
	cmd = *((uint8_t *)tmpContent);

	//2.进行命令字的选择
	if(cmd != DOWNFILEREQ)
	{
		//3. 获取接收数据的报头信息
		tmpHead = (reqPackHead_t *)tmpContent;

		//4. 数据包长度比较
		if(tmpHead->contentLenth + sizeof(int) == tmpContentLenth)
		{
			//5.长度正确, 进行流水号比较, 去除重包			
			if(tmpHead->seriaNumber >= g_recvSerial)
			{				
				//6.流水号正确,进行校验码比较
				tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
				checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
				if(checkCode == *tmpCheckCode)
				{
					//7.校验码正确, 整个数据包正确
					g_recvSerial += 1;		//流水号往下移动
					ret = 0;				
				}
				else
				{
					//8.校验码错误, 数据包需要重发
					myprint("Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
									cmd , tmpHead->seriaNumber, checkCode, *tmpCheckCode);
					ret = -1;
				}		
			}
			else
			{
				//9.流水号出错, 不需要重发
				myprint( "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u", 
											cmd,  tmpHead->seriaNumber, g_recvSerial);
				ret = -2;		
			}
		}
		else
		{
			//10.数据长度出错, 不需要重发
			myprint( "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u", 
						cmd, tmpHead->contentLenth + sizeof(int), tmpContentLenth, tmpHead->seriaNumber);
			ret = -2;			
		}
	}
	else
	{
		Head = (reqTemplateErrHead_t *)tmpContent;

		//11.数据包长度比较
		if(Head->contentLenth + sizeof(int) == tmpContentLenth)
		{
			//12.数据包长度正确, 比较流水号, 去除重包
			if(g_recvSerial == Head->seriaNumber)
			{
				//13.流水号正确, 进行校验码比较
				g_recvSerial += 1;		//流水号往下移动
				//14.流水号正确,进行校验码比较
				tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
				checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
				if(*tmpCheckCode == checkCode)
				{
					//15.校验码正确, 这个数据包正确
					ret = 0;
				}
				else
				{
					//16.校验码出错, 需要重发数据包						
					socket_log(SocketLevel[4], ret, "Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
									cmd , Head->seriaNumber, checkCode, *tmpCheckCode);
					ret = -1;
				}
			}
			else
			{
				//17.流水号出错, 不需要重发
				socket_log(SocketLevel[3], ret, "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u", 
											cmd,  Head->seriaNumber, g_recvSerial);
				ret = -2;	
			}			
		}
		else
		{
			//18.数据长度出错, 不需要重发
			socket_log(SocketLevel[3], ret, "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u", 
						cmd, Head->contentLenth + sizeof(int), tmpContentLenth, Head->seriaNumber);
			ret = -2;
		}
	}

	return ret;
}




//上传图片应答 数据包;  服务器收一包 回复一包.
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int upload_func_reply_old0620(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0;					//发送数据的长度
	int  checkNum = 0;								//校验码
	char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
	char *sendBufStr = NULL;						//发送数据地址
	char *tmp = NULL;
	resCommonHead_t head;							//应答数据报头
	reqPackHead_t  *tmpHead = NULL;					//请求报头信息
	static bool repeatFlag = false;					//true : 重发, false 未重发,
	static bool needToReply = false;				//true : 需要应答, false 不需要应答,  该标识符主要作用 : 当一轮50 个数据包接收过程中, 中间数据包出错,
													//最后一包数据已完全正确接收, 此时修改标识符位 true, 标识当成功接收到错误重传数据包时, 应立即给出本轮传输的回应.
	static int errPackNum = 0;						//出错重发数据包的数量
	static int ind = 1;								//测试的静态变量, 正常工作时,应该被删除
	static int recvSuccessSubNumber = 1;			//每轮成功接收到的子包数目

	
	//1.打印接收图片的信息
	//printf_comPack_news(recvBuf);					

	//3.获取报头信息
	tmpHead = (reqPackHead_t *)recvBuf;
	
	//4.进行请求信息类型判断
	if(tmpHead->isSubPack == 1)
	{
		//5.子包, 进行数据包信息判断
		if((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
		{
			//6.数据正确, 进行图片数据的存储, 记录本轮成功接收的子包数量
			recvSuccessSubNumber += 1;
#if 1
			if((ret = save_picture_func(recvBuf, recvLen)) < 0)			
			{
				printf("error: func save_picture_func() !!! [%d], [%s] \n", __LINE__, __FILE__);
				assert(0);
			}
#endif
			if(tmpHead->currentIndex + 1 < tmpHead->totalPackage && recvSuccessSubNumber < tmpHead->perRoundNumber)
			{
				//7.其它正常子包, 不需要回应
				if(needToReply == true && errPackNum == 0)
				{
					//7.1 出错数据包全部接收完毕, 则此时需要回应
					recvSuccessSubNumber = 0;
					
				}
				else if(needToReply == true && errPackNum > 0)					
				{				
					errPackNum -= 1;
				}
				else
				{
					goto End;
				}
			}
			else if(recvSuccessSubNumber == tmpHead->perRoundNumber)
			{
				//8. 每轮的最后一个包正确接收需要回应, 不需要显示 给UI
				if(repeatFlag == false)
				{
					recvSuccessSubNumber = 0;		//本轮已应答, 就修改标识符, 准备下一轮数据的接收	
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);
				}
				else
				{
					//8.1 校验码错误后的 数据包还未正确接收到数据, 所以不能回应					
					goto End;
				}
			}
			else if(tmpHead->currentIndex + 1 == tmpHead->totalPackage)
			{
				//9.图片的最后一包数据, 需要回应, 显示给UI 
				recvSuccessSubNumber = 0;
				tmp = tmpSendBuf + sizeof(resCommonHead_t);
				*tmp++ = 0x01;	
				outDataLenth = sizeof(resCommonHead_t) + 1;
				assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 1);						
			}
			else
			{
				myprint("tmpHead->currentIndex : %d", tmpHead->currentIndex);
				assert(0);
			}
			
		}
		else if(ret == -1)
		{
			//9.需要重发
			repeatFlag = true;
			outDataLenth = sizeof(resCommonHead_t);
			assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);							
		}
		else
		{
			//10.不需要重发
			ret = 0;
			goto End;
		}	
	}
	else
	{		
		//11.总包, 立即回应.
		myprint("isSubPack == 1, Number : %d", ind++);
		outDataLenth = sizeof(resCommonHead_t);
		assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
	}

	//2.申请内存
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		assert(0);
	}

	//12. 拷贝报头数据, 计算校验码
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));			
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp = tmpSendBuf + head.contentLenth;
	memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

	//13. 转义数据内容
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)	
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		assert(0);
	}
	
	//14. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, UPLOADCMDRESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		assert(0);
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
		
		
End:
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
}



//上传图片应答 数据包;  服务器收一包 回复一包.
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int upload_func_reply_old(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0;					//发送数据的长度
	int  checkNum = 0;								//校验码
	char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
	char *sendBufStr = NULL;						//发送数据地址
	char *tmp = NULL;
	resCommonHead_t head;							//应答数据报头
	reqPackHead_t  *tmpHead = NULL;					//请求报头信息
	static bool repeatFlag = false;					//true : 重发, false 未重发,
	
	
	//1.打印接收图片的信息
	//printf_comPack_news(recvBuf);					

	//3.获取报头信息
	tmpHead = (reqPackHead_t *)recvBuf;
	//4.进行请求信息类型判断
	if(tmpHead->isSubPack == 1)
	{
		//5.子包, 进行数据包信息判断
		if((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
		{
			//6.数据正确
#if 1
			if((ret = save_picture_func(recvBuf, recvLen)) < 0)			
			{
				printf("error: func save_picture_func() !!! [%d], [%s] \n", __LINE__, __FILE__);
				goto End;
			}
#endif
			//7. 接收到最后一包子包数据, 需要进行数据区的组包
			if(tmpHead->currentIndex + 1 == tmpHead->totalPackage)
			{			
				tmp = tmpSendBuf + sizeof(resCommonHead_t);
				*tmp++ = 0x01;	
				outDataLenth = sizeof(resCommonHead_t) + 1;
				assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 1);
			}
			else if((tmpHead->currentIndex + 1) % tmpHead->perRoundNumber == 0)
			{
				//8. 每轮的最后一个数据包正确接收需要回应
				if(repeatFlag == false)
				{
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);
				}
				else
				{
					goto End;
				}
			}
			else
			{
				goto End;
			}
			
		}
		else if(ret == -1)
		{
			//9.需要重发
			repeatFlag = true;
			outDataLenth = sizeof(resCommonHead_t);
			assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);							
		}
		else
		{
			//10.不需要重发
			ret = 0;
			goto End;
		}	
	}
	else
	{
		//11.总包, 立即回应.
		outDataLenth = sizeof(resCommonHead_t);
		assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
	}

	//2.申请内存
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//12. 拷贝报头数据, 计算校验码
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));			
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp = tmpSendBuf + head.contentLenth;
	memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

	//13. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//14. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, UPLOADCMDRESPON)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));

		
End:
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
}




//清空 套接字缓存区的数据;
int removed_sockfd_buf(int sockfd)
{
	int ret = 0, len = 0;
	char buf[1024] = { 0 };

	//1.设置非阻塞
	activate_nonblock(sockfd);

	//2.获取当前套接字缓冲区数据长度, 全部读取
	do{
		len = recv_peek(sockfd, buf, sizeof(buf));
		if(len > 0)
		{
			 printf("\nrecv_peek() len : %d\n", len);
			 ret = recv(sockfd, buf, sizeof(buf), 0);
			 if(ret == -1 && errno == EINTR)		
			 	continue;
		}	
		
	}while(len > 0);
	
	//3.设置阻塞
	deactivate_nonblock(sockfd);
	return ret;
}


/**
 * recv_peek - 仅仅查看套接字缓冲区数据，但不移除数据
 * @sockfd: 套接字
 * @buf: 接收缓冲区
 * @len: 长度
 * 成功返回>=0，失败返回-1
 */
ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
	int ret = 0;
	
	while (1)
	{
		ret = recv(sockfd, buf, len, MSG_PEEK);
		if (ret == -1 && errno == EINTR)
			continue;
		
		return ret;
	}
}


/**
 * deactivate_nonblock - 设置I/O为阻塞模式
 * @fd: 文件描符符
 */
int deactivate_nonblock(int fd)
{
	int ret = 0;
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
	{
		ret = flags;
		Socket_Log(__FILE__, __LINE__,SocketLevel[4], ret,"func deactivate_nonblock() err");
		return ret;
	}

	flags &= ~O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
	{
		Socket_Log(__FILE__, __LINE__,SocketLevel[4], ret,"func deactivate_nonblock() err");
		return ret;
	}
	return ret;
}


/**
 * activate_noblock - 设置I/O为非阻塞模式
 * @fd: 文件描符符
 */
int activate_nonblock(int fd)
{
	int ret = 0;
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
	{
		ret = flags;
		Socket_Log(__FILE__, __LINE__,SocketLevel[4], ret,"func activate_nonblock() err");
		return ret;
	}
		
	flags |= O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
	{
		Socket_Log(__FILE__, __LINE__,SocketLevel[4], ret,"func activate_nonblock() err");
		return ret;
	}
	return ret;
}

int server_bind_listen_fd(char *ip, int port, int *listenFd)
{
	int ret = 0, sfd = 0,flag = 1;
	
	//创建服务器套节字
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd == -1)
        std_err("socket");
    
    //端口复用            
   	ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if(ret)
		std_err("setsockopt");			   
                
    //定义地址类型
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
	ret = inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr);
  	if(ret != 1)
  		std_err("inet_pton");	
	
    //绑定服务器的IP、端口；
    ret = bind(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(ret == -1)
        std_err("bind");


    //监听链接服务器的客户数量
    ret = listen(sfd, 99);
    if(ret == -1)
        std_err("listen");
	else
		printf("Server listen IP %s, Port : %d, [%d], [%s]\n\n", ip, port, __LINE__, __FILE__);
	*listenFd = sfd;
	
	return ret;
}


void clear_global()
{
	int elementNumber = 0, i = 0;
	char *elementStr = NULL;
	int elementLenth = 0, conIndex = 0;
	int cmd = 0;

	for(conIndex = 0; conIndex < NUMBER; conIndex++)
	{		
		if(g_thid_sockfd_block[conIndex].flag == 1)		return;			
	}
		
	if((elementNumber =	get_element_count_send_block(g_sendData_addrAndLenth_queue)) < 0)
	{
		myprint("Err : func get_element_count_send_block()...");
		assert(0);		
	}
	
	if(elementNumber > 0)
	{
		for(i = 0; i < elementNumber; i++)
		{
			if((pop_queue_send_block(g_sendData_addrAndLenth_queue, &elementStr, &elementLenth, &cmd)) < 0)
			{
				myprint("Err : func get_element_count_send_block()...");
				assert(0);
			}			
			
			if((mem_pool_free(g_memoryPool, elementStr)) < 0)
			{
				myprint("Err : func mem_pool_free()...");
				assert(0);
			}
			elementStr = NULL;
		}
	}		
	
}





//保存图片
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int save_multiPicture_func(char *recvBuf, int recvLen)
{
	char *tmp = NULL;
	int lenth = 0, ret = 0;
	char TmpfileName[100] = { 0 };
	char fileName[100] = { 0 };
	char cmdBuf[1024] = { 0 };
	static FILE *fp = NULL;
	reqPackHead_t *tmpHead = NULL;
	int nWrite = 0;				//已经写的数据长度
	int perWrite = 0; 			//每次写入的数据长度

	
	//0. 获取数据包信息
	tmpHead = (reqPackHead_t *)recvBuf;
	lenth = recvLen - sizeof(reqPackHead_t) - sizeof(int) - FILENAMELENTH - sizeof(char) * 2;
	tmp = recvBuf + sizeof(reqPackHead_t) + FILENAMELENTH + sizeof(char) * 2;
	//myprint("picDataLenth : %d, currentIndex : %d, totalNumber : %d, recvLen : %d ...", lenth, tmpHead->currentIndex, tmpHead->totalPackage, recvLen);
	
	if(tmpHead->currentIndex != 0 && tmpHead->currentIndex + 1 < tmpHead->totalPackage)
	{
		//1.本图片的中间数据包, 将 指针指向 图片内容的部分, 将数据写入文件
		
		while(nWrite < lenth)
		{	
			if((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
			{
				myprint("Err : func fopen() : fileName : %s !!!", fileName);
				goto End;
			}	
			nWrite += perWrite;
		}		
	}
	else if(tmpHead->currentIndex == 0)
	{
		//2.本图片的第一包数据, 获取文件名称
		memcpy(TmpfileName, recvBuf + sizeof(reqPackHead_t) + sizeof(char) * 2, 46);
		sprintf(fileName, "%s/%s", g_DirPath, TmpfileName );

		//3.查看文件是否存在, 存在即删除	
		if(if_file_exist(fileName))
		{
			sprintf(cmdBuf, "rm %s", fileName);
			if(pox_system(cmdBuf) < 0)
			{
				myprint("Err : func pox_system()");
				ret = -1;
				goto End;
			}
		}

		//4.创建文件, 向文件中写入内容
		if((fp = fopen(fileName, "ab")) == NULL)
		{
			myprint("Err : func fopen() : fileName : %s !!!", fileName);
			ret = -1;
			goto End;
		}
		
		//5.将 指针指向 图片内容的部分, 将数据写入文件
		while(nWrite < lenth)
		{	
			if((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
			{
				myprint("Err : func fopen() : fileName : %s !!!", fileName);
				goto End;
			}	
			nWrite += perWrite;
		}			
	}
	else if(tmpHead->currentIndex + 1 == tmpHead->totalPackage)
	{
		//6.本图片的最后一包数据		
		while(nWrite < lenth)
		{	
			if((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
			{
				myprint("Err : func fopen() : fileName : %s !!!", fileName);
				goto End;
			}	
			nWrite += perWrite;
		}	

		fflush(fp);
		fclose(fp);
		fp = NULL;
	}
	else
	{
		//myprint("Error : The package index is : %d, totalPackage : %d, !!! ",tmpHead->currentIndex, tmpHead->totalPackage);
		ret = -1;
		goto End;
	}	


End:

	if(ret < 0)
	{
		if(fp)			fclose(fp);		fp = NULL;
	}
	
	return ret;	
}







//保存图片
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int save_picture_func(char *recvBuf, int recvLen)
{
	char *tmp = NULL;
	int lenth = 0, ret = 0;
	char TmpfileName[100] = { 0 };
	char fileName[100] = { 0 };
	char cmdBuf[1024] = { 0 };
	static FILE *fp = NULL;
	reqPackHead_t *tmpHead = NULL;
	int nWrite = 0;				//已经写的数据长度
	int perWrite = 0; 			//每次写入的数据长度

	
	//0. 获取数据包信息
	tmpHead = (reqPackHead_t *)recvBuf;
	lenth = recvLen - sizeof(reqPackHead_t) - sizeof(int) - FILENAMELENTH;
	tmp = recvBuf + sizeof(reqPackHead_t) + FILENAMELENTH;
	//myprint("picDataLenth : %d, currentIndex : %d, totalNumber : %d, recvLen : %d ...", lenth, tmpHead->currentIndex, tmpHead->totalPackage, recvLen);
	
	if(tmpHead->currentIndex != 0 && tmpHead->currentIndex + 1 < tmpHead->totalPackage)
	{
		//1.本图片的中间数据包, 将 指针指向 图片内容的部分, 将数据写入文件
		
		while(nWrite < lenth)
		{	
			if((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
			{
				myprint("Err : func fopen() : fileName : %s !!!", fileName);
				goto End;
			}	
			nWrite += perWrite;
		}		
	}
	else if(tmpHead->currentIndex == 0)
	{
		//2.本图片的第一包数据, 获取文件名称
		memcpy(TmpfileName, recvBuf + sizeof(reqPackHead_t), 46);
		sprintf(fileName, "%s/%s", g_DirPath, TmpfileName );

		//3.查看文件是否存在, 存在即删除	
		if(if_file_exist(fileName))
		{
			sprintf(cmdBuf, "rm %s", fileName);
			if(pox_system(cmdBuf) < 0)
			{
				myprint("Err : func pox_system()");
				ret = -1;
				goto End;
			}
		}

		//4.创建文件, 向文件中写入内容
		if((fp = fopen(fileName, "ab")) == NULL)
		{
			myprint("Err : func fopen() : fileName : %s !!!", fileName);
			ret = -1;
			goto End;
		}
		
		//5.将 指针指向 图片内容的部分, 将数据写入文件
		while(nWrite < lenth)
		{	
			if((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
			{
				myprint("Err : func fopen() : fileName : %s !!!", fileName);
				goto End;
			}	
			nWrite += perWrite;
		}			
	}
	else if(tmpHead->currentIndex + 1 == tmpHead->totalPackage)
	{
		//6.本图片的最后一包数据		
		while(nWrite < lenth)
		{	
			if((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
			{
				myprint("Err : func fopen() : fileName : %s !!!", fileName);
				goto End;
			}	
			nWrite += perWrite;
		}	

		fflush(fp);
		fclose(fp);
		fp = NULL;
	}
	else
	{
		//myprint("Error : The package index is : %d, totalPackage : %d, !!! ",tmpHead->currentIndex, tmpHead->totalPackage);
		ret = -1;
		goto End;
	}	


End:

	if(ret < 0)
	{
		if(fp)			fclose(fp);		fp = NULL;
	}
	
	return ret;	
}

