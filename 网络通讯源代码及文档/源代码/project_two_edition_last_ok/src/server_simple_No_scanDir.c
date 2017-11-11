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

#define 	ERRORINPUT
#define 	CERT_FILE 	"/home/yyx/nanhao/server/server-cert.pem" 
#define 	KEY_FILE  	"/home/yyx/nanhao/server/server-key.pem" 
#define 	CA_FILE 	"/home/yyx/nanhao/ca/ca-cert.pem" 			/*Trusted CAs location*/
#define 	CIPHER_LIST "AES128-SHA"								/*Cipher list to be used*/
#define 	NUMBER		15
#define		MY_MIN(x, y)	((x) < (y) ? (x) : (y))	

char g_DirPath[512];
unsigned int g_flag = 0;							//服务器 接收的 数据包序列号,进行缓存
unsigned int g_sendFlag = 0;						//服务器自己数据包序列号
mem_pool_t  *g_memoryPool = NULL;					//mempool handle
queue_send_dataBLock   g_sendData_addrAndLenth_queue;
clientRequestContent  *g_requestContent = NULL;
long long int g_fileTemplateID = 494986, g_fileTableID = 464698;
char *g_fileNameTable = "downLoadTemplateNewestFromServer_17_05_08.pdf";
char *g_fileNameTemplate = "C_16_01_04_10_16_10_030_B_L.jpg";

pthread_mutex_t	 g_mutex_pool = PTHREAD_MUTEX_INITIALIZER; 	
pthread_mutex_t	 g_mutex_data_addrAndLenth = PTHREAD_MUTEX_INITIALIZER; 	

sem_t g_sem_send;						//send data package OK , post The sem_t

#if 0
typedef struct _RecvAndSendthid_Sockfd_{
	pthread_t		pthidWrite;
	pthread_t		pthidRead;
	int 			sockfd;
	int 			flag;					// 0: UnUse;  1: use
	SSL_CTX 		*sslCtx;				// The Once connection 
	SSL 			*sslHandle;				// The Once connection for transfer
	sem_t			sem_send;				//The send thread And recv Thread synchronization
	bool			encryptRecvFlag;		//true : encrypt, false : No encrypt
	bool			encryptSendFlag;		//ture : send data In use encyprtl, false : In use common
	bool			encryptChangeFlag;		//true : once recv cmd=14, (deal with recv And send Flag(previous)), modify to False ; false : No change		
}RecvAndSendthid_Sockfd;
#endif

typedef struct _RecvAndSendthid_Sockfd_{
	pthread_t		pthidWrite;
	pthread_t		pthidRead;
	int 			sockfd;
	int 			flag;					// 0: UnUse;  1: use	
	sem_t			sem_send;				//The send thread And recv Thread synchronization
	bool			encryptRecvFlag;		//true : encrypt, false : No encrypt
	bool			encryptSendFlag;		//ture : send data In use encyprtl, false : In use common
	bool			encryptChangeFlag;		//true : once recv cmd=14, (deal with recv And send Flag(previous)), modify to False ; false : No change		
}RecvAndSendthid_Sockfd;


RecvAndSendthid_Sockfd		g_thid_sockfd_block[NUMBER];

int removed_sockfd_buf(int sockfd);
int removefd(int sockfd);
void clear_global();			//clear global resource

//int ssl_server_init_handle(int socketfd, SSL_CTX **srcCtx, SSL **srcSsl);


void signUSR1_exit_thread(int sign)
{
	int ret = 0;
	socket_log( SocketLevel[2],ret , "\nthread:%lu, receive signal:%u---", pthread_self(), sign);
	pthread_exit(NULL);
}

void sign_usr2(int sign)
{
	int ret = 0;
	
	myprint("\n\n()()()()()()()recv signal : %d, pthread : %lu", sign, pthread_self());
	socket_log( SocketLevel[2],ret , "\nthread:%lu, receive signal:%u---", pthread_self(), sign);
}



int init_server(char *ip, int port, char *uploadDirParh)
{
	int 			ret = 0, i = 0;
	int 			sfd, cfd;
    socklen_t 		clie_len;
	pthread_t 		pthidWrite = -1, pthidRead = -1;		//pthread ID
	struct sockaddr_in clie_addr;
	int 			conIndex = 0;
	//SSL_CTX 		*ctx = NULL;	
	//SSL 			*ssl = NULL;	
	//1.创建保存上传文件的目录
	if(access(uploadDirParh,  0) < 0 )
	{
		ret = mkdir(uploadDirParh, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	 	if(ret)		std_err("mkdir");	
	}
	
	memset(g_DirPath, 0, sizeof(g_DirPath));
	memcpy(g_DirPath, uploadDirParh, strlen(uploadDirParh));
	memset(g_thid_sockfd_block, 0, sizeof(g_thid_sockfd_block));

	//2. register signal function
	signal(SIGUSR1, signUSR1_exit_thread);
	signal(SIGPIPE, func_pipe);	//开启服务器 监听套接字
	//signal(SIGUSR2, sign_usr2);

	//3.create The listhenSockfd And bind IP , PORT
    ret = server_bind_listen_fd(ip, port, &sfd);
    if(ret)		std_err("server_bind_listen_fd");		
    clie_len = sizeof(clie_addr); 

	//4.init The glabal value : : mempool And Queue
    if((g_memoryPool = mem_pool_init( 1500, 2800, &g_mutex_pool)) == NULL )
	{
		myprint("Err : func mem_pool_init() ");
		assert(0);	
	}
	if((g_sendData_addrAndLenth_queue = queue_init_send_block(1500, &g_mutex_data_addrAndLenth)) == NULL)
	{
		myprint("Err : func queue_init_send_block() ");
		assert(0);	
	}

	//5.init The sem_t 	semaphore
	sem_init(&g_sem_send, 0, 0);		

	for(i = 0; i < NUMBER; i++)
		sem_init(&(g_thid_sockfd_block[i].sem_send), 0, 0);

	//6. accept The connect from client And create The work thread  to recv And send data
    while(1)
    {
    	 //6.1 阻塞等待客户端发起链接请求
    	cfd = accept(sfd, (struct sockaddr*)&clie_addr, &clie_len);
    	if(cfd == -1)
       	{
       		std_err("accept");
		}
		else
		{
		#if 0
			myprint("===========  1  ============");
			if((ret = ssl_server_init_handle(cfd , &ctx, &ssl)) < 0)
			{
				close(cfd);
				myprint("\n\nErr : func ssl_server_init_handle()... ");		
				return ret;
			}
		#endif
		}

		//6.2 training in rotation find a array element 
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
		
	
		g_thid_sockfd_block[conIndex].flag = 1;
		g_thid_sockfd_block[conIndex].sockfd= cfd;
//		g_thid_sockfd_block[conIndex].sslCtx = ctx;
//		g_thid_sockfd_block[conIndex].sslHandle = ssl;
		g_thid_sockfd_block[conIndex].encryptChangeFlag = false;
		g_thid_sockfd_block[conIndex].encryptRecvFlag = false;
		g_thid_sockfd_block[conIndex].encryptSendFlag = false;		

		//创建子线程， 去执行发送 业务。
		ret = pthread_create(&pthidWrite, NULL, child_write_func, (void *)conIndex);
		if(ret < 0)
		{
			myprint("err: func pthread_create() ");		
			return ret;
		}
		//创建子线程， 去执行接收 业务。
		ret = pthread_create(&pthidRead, NULL, child_read_func, (void *)conIndex);
		if(ret < 0)
		{
			myprint("err: func pthread_create() ");		
			return ret;
		}
	
		g_thid_sockfd_block[conIndex].pthidRead = pthidRead;
		g_thid_sockfd_block[conIndex].pthidWrite= pthidWrite;	
		
    }

	//4. close The listen sockfd;
    close(sfd);
	for(i = 0; i < NUMBER; i++)			sem_destroy(&(g_thid_sockfd_block[i].sem_send));		
	pthread_mutex_destroy(&g_mutex_pool);
	pthread_mutex_destroy(&g_mutex_data_addrAndLenth);			

	return ret;
}

#if 0
//子线程执行逻辑;  返回值: 0:标识程序正常, 1: 标识客户端关闭;  -1: 标识出错.
void *child_read_func02(void *arg)
{
	int ret = 0;
	int clientFd = (int)arg;
	int i = 1, j = 1, len = 0;
	char recvBuf[10240] = { 0 };
	char *sendBuf = NULL;
	int recvLen = 0, sendLen = 0;
	
	pthread_detach(pthread_self());
    //传输数据
    while(1)
    {  
    	begin:
    	memset(recvBuf, 0 ,  sizeof(recvBuf));  
        recvLen = read(clientFd, recvBuf, sizeof(recvBuf));
        if(recvLen > 0)		//正确接收数据, 进行处理
        {    
        	 myprint("fucn read() recv data; recvLen : %d,  包数: i = %d ", recvLen, i);          
        	 removed_sockfd_buf(clientFd);
        	 ret = read_data_proce(recvBuf, recvLen, &sendBuf, &sendLen);	
        	 //ret = read_package(recvBuf, recvLen, &sendBuf, &sendLen);
        	 if(ret < 0)
        	 {
        	 	  myprint("接收数据后 i = %d, 处理出错, 服务器程序退出!!! ", i);        	 	  
        	 	  close(clientFd);
        	 	  ret = -1;
        	 	  break;
        	 }
        	 else if(ret == 2)
        	 {
        	 	printf("\n\n");
        	 	goto begin;
        	 }        	
        	 i++;
        }
       	else if(recvLen < 0)       	//接收数据出错;
       	{
       		if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
       		{
       			myprint("server: errno == EINTR || errno == EAGAIN ");         		
    			continue;  				
       		}
       		else 
       		{
       			myprint("ERror: server recv data ");         			
       			close(clientFd);
       			ret = -1;
       			break;	
       		}       		
       	}
       	else if(recvLen == 0)		//客户端关闭
       	{
       		myprint("client closed ");               		
       		ret = 1;				
       		close(clientFd);
       		break;
       	}	      	
				
		//发送数据
        len = write(clientFd, sendBuf, sendLen);
        if(len < 0)
        {        	
        	myprint("ERror: server send data  ");    
        	free(sendBuf);
        	ret = -1;
        	close(clientFd);
        	break;
        }
        else if(len < sendLen && len > 0)
        {
        	myprint("Error: server have send data lenth [%d] ", len);            	
        	free(sendBuf);
        	ret = -1;
        	close(clientFd);
        	break;
        }
        else if(len == sendLen)
        {
        	myprint("server write OK : %s, write data = [%d], j = [%d]  ", sendBuf, len, j++);              	
        	free(sendBuf);
        }
        else if(len == 0)
        {
        	close(clientFd);
        	free(sendBuf);
        	myprint("client closed!!! ");            	
       		break;
        }
        
        printf("\n\n");
        memset(recvBuf, 0 ,  sizeof(recvBuf));  
        sendBuf = NULL;    
		//memset(sendBuf, 0 ,  sizeof(sendBuf)); 
	} 
	
	pthread_exit(NULL);
	return NULL;
}

#endif


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
    //传输数据
    while(1)
    {  

    	memset(recvBuf, 0 ,  sizeof(recvBuf));
		if(!g_thid_sockfd_block[index].encryptRecvFlag)
	        recvLen = read(clientFd, recvBuf, sizeof(recvBuf));
		else
		{
			myprint("SSL_read function... ");
			//recvLen = SSL_read(g_thid_sockfd_block[index].sslHandle, recvBuf, sizeof(recvBuf));  
		}
		if(recvLen > 0)		//正确接收数据, 进行处理
        {    
        	 myprint("fucn read() recv data; recvLen : %d,  recv package index : i = %d, encryptRecvFlag : %d",
			 	recvLen, i, g_thid_sockfd_block[index].encryptRecvFlag);          

			 if((ret = removed_sockfd_buf(clientFd)) < 0)
			 {
				myprint("Err : func removed_sockfd_buf()");
				break;
			 }
			 
        	 ret = read_data_proce(recvBuf, recvLen, index);	        
        	 if(ret < 0)
        	 {
        	 	  myprint("Err : func read_data_proce() The index recv package i = %d !!! ", i);        	 	  
        	 	  break;
        	 }           	
        	 i++;
        }
       	else if(recvLen < 0)       	//The connect sockfd Error
       	{
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
       		myprint("client closed ");
			g_thid_sockfd_block[index].flag = 0;
			clear_global();
       		break;
       	}	      	
		printf("\n\n");		
	} 

	
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
	if((ret = pthread_kill(g_thid_sockfd_block[index].pthidWrite, SIGUSR1)) < 0)		assert(0);					
	pthread_exit(NULL);
	return NULL;
}


void clear_global()
{
	int elementNumber = 0, i = 0;
	char *elementStr = NULL;
	int elementLenth = 0, conIndex = 0;


	for(conIndex = 0; conIndex < NUMBER; conIndex++)
	{
		printf("---------=========== 0 ============----------\n");
		//assert(0);
		if(g_thid_sockfd_block[conIndex].flag == 1)		return;			
	}
		
	if((elementNumber =	get_element_count_send_block(g_sendData_addrAndLenth_queue)) < 0)
	{
		myprint("Err : func get_element_count_send_block()...");
		assert(0);
		return;
	}
	printf("---------=========== 1 ============----------\n");

	if(elementNumber > 0)
	{
		for(i = 0; i < elementNumber; i++)
		{
			if((pop_queue_send_block(g_sendData_addrAndLenth_queue, &elementStr, &elementLenth)) < 0)
			{
				myprint("Err : func get_element_count_send_block()...");
				assert(0);
			}			
			printf("--------- %d ----------\n", i);
			if((mem_pool_free(g_memoryPool, elementStr)) < 0)
			{
				myprint("Err : func mem_pool_free()...");
				assert(0);
			}
			elementStr = NULL;
		}
	}
			printf("---------=========== 2 ============----------\n");
	
}

void print_write_package(char *sendPackage)
{
	char *tmp = sendPackage + 1;
	myprint("func print_write_package()");
	printf_pack_news(tmp);

}


void *child_write_func(void *arg)
{
	int ret = 0, j = 1;	
	char *sendDataAddr = NULL;
	int  sendDataLenth = 0;
	int  writeLenth = 0;
	int index = (int)arg;
	int clientFd = g_thid_sockfd_block[index].sockfd;
	int nWrite = 0;
	
	myprint("New thread will Send ...... ");
		
	while(1)
	{
		
		sem_wait(&(g_thid_sockfd_block[index].sem_send));		
		//1. get send Data And data Lenth
		if((ret = pop_queue_send_block(g_sendData_addrAndLenth_queue, &sendDataAddr, &sendDataLenth)) < 0)
		{
			myprint("Err : func pop_queue_send_block()");
			assert(0);
		}

		#if 0
		if(flag <= 4 * 1016 - 1 )
		{
			flag++;
			printf("j : %d\n", j++);
			continue;
		}
		#endif
		print_write_package(sendDataAddr);



		//2. send Data To client 	
		if(!g_thid_sockfd_block[index].encryptSendFlag)
	    	writeLenth = write(clientFd, sendDataAddr, sendDataLenth);
		else
		{
			myprint("\n\nSSL_write function... ");
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
			myprint("********** have write Byte : %d **********", nWrite);
        	myprint("server write OK : %p, write data = [%d], encryptSendFlag : %d, j = %d ",
				sendDataAddr, writeLenth, g_thid_sockfd_block[index].encryptSendFlag, j++);              		        					

			//2.1 jurge send data style
			if(g_thid_sockfd_block[index].encryptChangeFlag)
			{
				g_thid_sockfd_block[index].encryptSendFlag = g_thid_sockfd_block[index].encryptRecvFlag;
				g_thid_sockfd_block[index].encryptChangeFlag = false;
				//2.1.1. send signal To thread recv
				myprint("will kill USR2 SIGNAL to read thread ID : %lu", g_thid_sockfd_block[index].pthidRead);
				if((ret = pthread_kill(g_thid_sockfd_block[index].pthidRead, SIGUSR2)) < 0)		assert(0);				
			}

			//2.2 free block to mempool
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
		
		printf("\n\n");
	}

End:	
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



//接收客户端的数据包, recvBuf: 包含: 标识符 + 报头 + 报体 + 校验码 + 标识符;   recvLen : recvBuf数据的长度
//				sendBuf : 处理数据包, 将要发送的数据;  sendLen : 发送数据的长度
int read_data_proce(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0 ;	
	
	char buf[2800] = { 0 };	
	packHead *tmpHead = NULL;

	
	//1. buf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  outDataLenth : 长度为 buf 的内容 长度
	ret = anti_escape(recvBuf + 1,  recvLen - 2, buf, &outDataLenth);
	if(ret < 0)
	{
		myprint("Err : func anti_escape() !!!! ");    
		return ret;
	}
		
	//2. jump The cmd And store The serial Number from recv package
	tmpHead = (packHead *)buf;
	g_flag = tmpHead->seriaNumber;
	
	switch (tmpHead->cmd){
		case 0x01: 		//登录数据包
			if((ret = data_un_packge(login_func_reply, buf, outDataLenth, index)) < 0)
				myprint("Error: func login_func_reply() ");				
			break;
		case 0x02: 		//退出数据包
			if((ret = data_un_packge(exit_func_reply, buf, outDataLenth, index)) < 0)
				myprint("Error: func exit_func_reply() ");				
			break;
		case 0x03: 		//心跳数据包
			if((ret = data_un_packge(heart_beat_func_reply, buf, outDataLenth, index)) < 0)
				myprint("Error: func heart_beat_func_reply() ");
			break;
		case 0x04: 		//模板下载			
			if((ret = data_un_packge(downLoad_template_func_reply, buf, outDataLenth, index)) < 0)
				myprint("Error: func downLoad_template_func_reply() ");
			break;
		case 0x05: 		//客户端信息请求
			if((ret = data_un_packge(get_FileNewestID_reply, buf, outDataLenth, index)) < 0)
				myprint("Error: func client_request_func_reply() ");
			break;
		case 0x06: 		//上传图片数据包
			if((ret = data_un_packge(upload_func_reply, buf, outDataLenth, index)) < 0)
				myprint("Error: func upload_func_reply() ");
			break;
		case 0x07:
			if((ret = data_un_packge(push_info_toUi_reply, buf, outDataLenth, index)) < 0)
				myprint("Error: func upload_func_reply() ");
			break;
		case 0x09: 		//删除图片数据包
			if((ret = data_un_packge(delete_func_reply, buf, outDataLenth, index)) < 0)
				myprint("Error: func delete_func_reply() ");
			break;
		case 0x0C:		//上传模板
			if((ret = data_un_packge(template_extend_element_reply, buf, outDataLenth, index)) < 0)
				myprint("Error: func template_extend_element() ");
			break;
		case 0x0D:		//上传图片集
			if((ret = data_un_packge(upload_template_set_reply, buf, outDataLenth, index)) < 0)
				myprint("Error: func upload_template_set ");
			break;
		case 0x0E:
			if((ret = data_un_packge(encryp_or_cancelEncrypt_transfer, buf, outDataLenth, index)) < 0)
				myprint("Error: func encryp_or_cancelEncrypt_transfer_reply ");
			break;
		default:			
			printf_pack_news(buf);
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


/*组装报头
*@param : head 报头结构体
*@param : cmd 数据包命令字
*@param : contentLenth 数据包内容长度
*@param : subPackOrNo 数据包分包与否
*@param : machine 机器码
*@param : totalPackNum 总包数
*@param : indexPackNum 当前包数序号
*/
void assign_pack_head(packHead *head, uint8_t cmd, uint32_t contentLenth, uint8_t subPackOrNo, char *machine,  uint16_t totalPackNum, uint16_t indexPackNum)   
{	
	static int recvSerial = 0;
	
	head->contentLenth = contentLenth;
	head->cmd = cmd;
	head->subPackOrNo = subPackOrNo;
	memcpy(head->machineCode, machine, 12);
	if(cmd != 0x07)
	{
		head->seriaNumber = g_seriaNumber;
		g_seriaNumber += 1;
	}
	else 
	{	
		head->seriaNumber = recvSerial;	
		g_sendCmFlag = recvSerial;
		recvSerial += 1;
	}		
	head->packgeNews.totalPackge =  totalPackNum;
	head->packgeNews.indexPackge = indexPackNum;
	g_send_package_cmd = cmd;
	if(cmd == 13) 		g_specialPacketCmd = 12;

}


//保存上传的图片合集
int save_set_picture_func(char *recvBuf, int recvLen)
{
	char *tmp = NULL;
	int lenth = 0, contentLenth = 0, nwrite = 0;
	char TmpfileName[100] = { 0 };
	char fileName[100] = { 0 };
	
	//1.读取上传的文件名称
	memcpy(TmpfileName, recvBuf + sizeof(packHead) + 1, 46);
	sprintf(fileName, "/%s/%s", g_DirPath, TmpfileName );
	
	//将 指针指向 图片内容的部分
	tmp = recvBuf + sizeof(packHead) + 46 + 1;
	
	
	//将消息体 写入文件
	FILE *fp = fopen(fileName, "ab+"); 
	if(fp == NULL)
		std_err("fopen");
	
	//获取图片内容的长度
	contentLenth = recvLen - sizeof(packHead) - sizeof(int) - 46 - 1;
	
	while(nwrite < contentLenth)
	{
		lenth = fwrite(tmp + nwrite, 1, contentLenth - nwrite, fp);
		if(lenth < 0)
			std_err("fwrite");
		nwrite += lenth;
	}
		
	fflush(fp);
	fclose(fp);
	
	return 0;
}

//心跳数据包	3号数据包应答		
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int heart_beat_func_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0, checkNum = 0;
	char tmpSendBuf[1500] = { 0 };
	char *tmp = NULL, *sendBufStr = NULL;	
	char *machine = "123456789321";
	packHead head, *tmpHead = NULL;
	
	printf_pack_news(recvBuf);			//打印心跳数据包信息

	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	
	//1.package The head news
	outDataLenth = sizeof(packHead);
	tmpHead = (packHead*)recvBuf;
	assign_pack_head(&head, 0x83, outDataLenth, 1, machine, tmpHead->packgeNews.totalPackge, tmpHead->packgeNews.indexPackge);
	memcpy(tmpSendBuf, &head, sizeof(packHead));	
	
	//2.calculation checkCode
	tmp = tmpSendBuf + sizeof(packHead);
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//3. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//5. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	
	
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	//sem_post(&g_sem_send);

End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
	
}


//下载模板数据包	4号数据包应答		
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码  
//recvLen : 长度为 buf 的内容 长度
int downLoad_template_func_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0;
	packHead head;
	char *tmp = NULL, *sendBufStr = NULL;
	int contentLenth = 0;
	char *machine = "54454547871";
	int checkNum = 0, nRead = 0;
	char tmpSendBuf[1500] = { 0 };
	int  doucumentLenth = 1400 - sizeof(packHead) - 3 * sizeof(char) - 2 * sizeof(char) - sizeof(int);
	FILE *fp = NULL;
	char *fileName = NULL;
	int totalPacka = 0;
	int indexNumber = 0;
	int fileType = 0;
	
	printf_pack_news(recvBuf);

	//1. get The downLoad file Type And downLoadFileID
	tmp = recvBuf + sizeof(packHead);
	if(*tmp == 0)			fileName = g_fileNameTemplate;
	else if(*tmp == 1)		fileName = g_fileNameTable;
	else					assert(0);
	fileType = *tmp;
	tmp++;
	myprint("downLoadFileName : %s", fileName);

	
	//2. open The template file
	if((fp = fopen(fileName, "rb+")) == NULL)
	{
		myprint("Err : func fopen()");
		goto End;
	}

	//2.get The file Total Package Number 
	if((ret = get_file_size(fileName, &totalPacka, doucumentLenth, NULL)) < 0)
	{
		myprint("Err : func get_file_size()");
		goto End;
	}

	//3.read The file content	
	while(!feof(fp))
	{
		 //3.1 get The part of The file conTent	And package The body News
		 memset(tmpSendBuf, 0, sizeof(tmpSendBuf));
		 tmp = tmpSendBuf + sizeof(packHead);
		 *tmp++ = 0;
		 *tmp++ = 2;
		 *tmp++ = fileType;		      
		 if((nRead = fread(tmp , 1, doucumentLenth , fp)) < 0)
		 {
			 myprint("Error: func fread()");
			 socket_log(SocketLevel[4], ret, "Error: func fread()");
			 assert(0);
			 goto End;
		 }
		 
		 //3.2 The head  of The package
		 contentLenth = sizeof(packHead) + nRead + 3 * sizeof(char);
		 assign_pack_head(&head, 0x84, contentLenth, 1, machine, totalPacka, indexNumber);
		 indexNumber += 1;
		 memcpy(tmpSendBuf, &head, sizeof(packHead));
		
		 //3.3 calculate checkCode
		 checkNum = crc326(tmpSendBuf,  contentLenth);
		 memcpy(tmpSendBuf + contentLenth, &checkNum, sizeof(int));		 				  

		 //5.4 allock The block buffer in memory pool
		 sendBufStr = mem_pool_alloc(g_memoryPool);
		 if(!sendBufStr)
		 {			 
			 ret = -1;
			 socket_log(SocketLevel[4], ret, "Error: func mem_pool_alloc()");
			 goto End;
	 	 }
	
		 //5.6 escape the buffer
		 *sendBufStr = PACKSIGN;							 //flag 
		 ret = escape(PACKSIGN, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr+1, &outDataLenth);
		 if(ret != 0)
		 {
			 socket_log(SocketLevel[4], ret, "Error: func escape()");
			 goto End;
		 }
		 *(sendBufStr + outDataLenth + 1) = PACKSIGN; 	 //flag 

		 //5.7 push The sendData addr and sendLenth in queuebuf
		 ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + sizeof(char) * 2);
		 if(ret)
		 {
			 socket_log(SocketLevel[4], ret, "Error: func push_queue_send_block()");
			 goto End;
		 }
		
		 nRead = 0;		
		 sem_post(&(g_thid_sockfd_block[index].sem_send));
		 //sem_post(&g_sem_send);
		 
	}
	
	myprint("========5 === indexNumber : %d,    ret : %d =====", indexNumber, ret);


End:
	print_write_package(sendBufStr);
	if(fp)		fclose(fp);
	return ret;
}




void print_client_request_reply(char *srcReplyNews)
{
	char *tmp = srcReplyNews;
	clientRequestContent *tmpReply = NULL;
	int i = 0, j = 0;

	if((tmpReply = malloc(sizeof(clientRequestContent))) == NULL)
		assert(0);

	tmpReply->projectNum = *tmp++;
	for (i = 0; i < tmpReply->projectNum; i++)
	{

		tmpReply->projectSet[i].projectID = *tmp++;
		tmpReply->projectSet[i].projectNameLenth = *tmp++;
		memcpy(tmpReply->projectSet[i].projectName, tmp, sizeof(tmpReply->projectSet[i].projectName));
		tmp += sizeof(tmpReply->projectSet[i].projectName);
		tmpReply->projectSet[i].optionNum = *tmp++;
		for (j = 0; j < tmpReply->projectSet[i].optionNum; j++)
		{
			tmpReply->projectSet[i].optionSet[j].optionID = *tmp++;
			tmpReply->projectSet[i].optionSet[j].optionNameLenth = *tmp++;
			memcpy(tmpReply->projectSet[i].optionSet[j].optionName, tmp, sizeof(tmpReply->projectSet[i].optionSet[j].optionName));
			tmp += sizeof(tmpReply->projectSet[i].optionSet[j].optionName);
		}
	}

	
	myprint("projectNum : %d",tmpReply->projectNum);
	for(i = 0; i < tmpReply->projectNum; i++)
	{
		myprint("project[%d]   proID : %d, proNameLenth : %d, proName : %s", i, 
			tmpReply->projectSet[i].projectID, 
			tmpReply->projectSet[i].projectNameLenth, 
			tmpReply->projectSet[i].projectName);
		myprint("project[%d]   optionNum : %d", i, tmpReply->projectSet[i].optionNum);
		for(j = 0; j < tmpReply->projectSet[i].optionNum; j++)		
			myprint("option[%d]  optID : %d, optNamelenth : %d, optName : %s", j, 
			tmpReply->projectSet[i].optionSet[j].optionID, 
			tmpReply->projectSet[i].optionSet[j].optionNameLenth, 
			tmpReply->projectSet[i].optionSet[j].optionName);		
	}
		
}

void packStruct(char *requestContent, int *outLenth)
{
	char *tmpOutSendBuf = requestContent;
	int i = 0, j = 0, lenth =0;
	int projectNum = 5, optionNum = 6;

	memset(requestContent, 0, sizeof(clientRequestContent));
	tmpOutSendBuf = requestContent;
#if 0
	*tmpOutSendBuf++ = projectNum;
	lenth += 1;
	for (i = 0; i < projectNum; i++)
	{
		*tmpOutSendBuf++ = 2;		//proID
		*tmpOutSendBuf++ = 6;		//proNameLenth
		memcpy(tmpOutSendBuf, "helloshu", 6);	//proName
		tmpOutSendBuf += 256;
		*tmpOutSendBuf++ = optionNum;		//optionNumber
		lenth += (3 + 256);
		for (j = 0; j < optionNum; j++)
		{
			*tmpOutSendBuf++ = 2;		//optionID
			*tmpOutSendBuf++ = 8;		//optionNamelenth
			memcpy(tmpOutSendBuf, "niao qwdq", 8);	//optionName
			tmpOutSendBuf += 256;
			lenth += (2 + 256);
		}
	
	}

	*outLenth = lenth;
#endif
#if 1
	//1. package struct	
	*tmpOutSendBuf++ = projectNum;					//projectNumber
	lenth += 1;
	for (i = 0; i < projectNum; i++)
	{
		*tmpOutSendBuf++ = 2;						//proID
		*tmpOutSendBuf++ = 6;						//proNameLenth
		memcpy(tmpOutSendBuf, "helloshu", 6);		//proName
		tmpOutSendBuf += sizeof(g_requestContent->projectSet->projectName);
		*tmpOutSendBuf++ = optionNum;				//optionNumber
		lenth += (3 + sizeof(g_requestContent->projectSet->projectName));
		for (j = 0; j < optionNum; j++)
		{
			*tmpOutSendBuf++ = 2;					//optionID
			*tmpOutSendBuf++ = 8;					//optionNamelenth
			memcpy(tmpOutSendBuf, "niao qwdq", 8);	//optionName
			tmpOutSendBuf += sizeof(g_requestContent->projectSet->optionSet->optionName);
			lenth += (2 + sizeof(g_requestContent->projectSet->optionSet->optionName));
			
		}
	
	}

	*outLenth = lenth;
#endif
	
}

//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符); 
//recvLen : 长度为 buf 的内容 长度
int get_FileNewestID_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0, checkNum = 0; 
	char *tmp = NULL, *sendBufStr = NULL;
	int fileType = 0;
	packHead  head;
	char tmpSendBuf[1500] = { 0 };
	char *machine = "54454547871";	

	
	//1. get The file type
	tmp = recvBuf + sizeof(packHead);
	if(*tmp == 0)				fileType = 0;
	else if(*tmp == 1)			fileType = 1;
	else						assert(0);

	//2. alloc The memory block
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//3.package body news
	tmp = tmpSendBuf + sizeof(packHead);
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
	outDataLenth =  sizeof(packHead) + 65;
	tmp += 64;
	
	//4.package The head news	
	assign_pack_head(&head, 0x85, outDataLenth, 1, machine, 1, 0);
	memcpy(tmpSendBuf, &head, sizeof(packHead));		
	
	//5.calculation checkCode
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
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	
End:
	if(ret < 0)		if(sendBufStr)		mem_pool_free(g_memoryPool, sendBufStr);

	return ret;
}



//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符); 
//recvLen : 长度为 buf 的内容 长度
int checkOut_fileID_newest_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0, checkNum = 0;
	char *fileNameTemplate = "downLoadTemplateNewestFromServer_17_05_08.pdf";
	char *fileNameTable = "C_16_01_04_10_16_10_030_B_L.jpg";
	char *tmp = NULL, *sendBufStr = NULL;
	int fileType = 0;
	char fileName[65] = { 0 };
	int  comRet = 0;
	packHead  head;
	char tmpSendBuf[1500] = { 0 };
	char *machine = "54454547871";

	myprint("The queue have element number : %d ...", get_element_count_send_block(g_sendData_addrAndLenth_queue));


	
	//1. get The file type
	tmp = recvBuf + sizeof(packHead);
	if(*tmp == 0)				fileType = 0;
	else if(*tmp == 1)			fileType = 1;
	else						assert(0);
	tmp++;

	//2.get The fileID
	memcpy(fileName, tmp, 64);
	printf("\n\n");
	myprint("checkFileName : %s", fileName);
	
	//3.compare The ID
	if(fileType == 0)		comRet = strcmp(fileName, fileNameTemplate);
	else if(fileType == 1)	comRet = strcmp(fileName, fileNameTable);


	//4. alloc The memory block
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	//5.package body news
	tmp = tmpSendBuf + sizeof(packHead);
	if(comRet == 0)				*tmp++ = comRet;
	else						*tmp++ = 1;		
	outDataLenth =  sizeof(packHead) + 1;
	
	//6.package The head news	
	assign_pack_head(&head, 0x85, outDataLenth, 1, machine, 1, 0);
	memcpy(tmpSendBuf, &head, sizeof(packHead));		
	
	//3.calculation checkCode
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//4. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//5. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	
		
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	//sem_post(&g_sem_send);
	
End:
	if(ret < 0)		if(sendBufStr)		mem_pool_free(g_memoryPool, sendBufStr);

	return ret;
}


int client_request_func_reply(char *recvBuf, int recvLen, int srcIndex)
{
	int ret = 0, outDataLenth = 0, outDataLenthDest = 0;
	packHead head ;
	char  *sendBufStr = NULL;
	unsigned int contentLenth = 0;
	char *machine = "54454547871";
	int checkNum = 0;
	char tmpSendBuf[1500] = { 0 };
	int index = 0, totalNumber = 0;
	char *requestContent = NULL;
	int residualByte = 0;
	char *tmpOutSendBuf = NULL, *tmp = NULL;
	int  copywrite = 0, havewrite = 0;
	

	//1. package The whole body news
	if((requestContent = malloc(sizeof(clientRequestContent))) == NULL)
	{
		myprint("func malloc() clientRequestContent");
		assert(0);
	}

	//1. package struct News
	packStruct(requestContent, &outDataLenth);

	//2. calculate The totalNumber And Body content  Lenth	
	contentLenth = PACKMAXLENTH - sizeof(char) * 2 - sizeof(char) * 2 - sizeof(int) -sizeof(packHead);		//The content Lenth
	if((outDataLenth % contentLenth) > 0 )
		totalNumber = outDataLenth / contentLenth + 1;
	else
		totalNumber = outDataLenth / contentLenth;

	tmpOutSendBuf = (char*)requestContent;

	do
	{

		//1.package body news
		memset(tmpSendBuf, 0, sizeof(tmpSendBuf));		
		residualByte = outDataLenth - (index + 1) * contentLenth;
		if(residualByte > 0)	copywrite = contentLenth;
		else					copywrite = outDataLenth - index * contentLenth;
		tmp = tmpSendBuf + sizeof(packHead);
		*tmp++ = 0;
		*tmp++ = 2;
		memcpy(tmp, tmpOutSendBuf + havewrite, copywrite);
		havewrite += copywrite;

		//2.package head news
		assign_pack_head(&head, 0x85, copywrite + sizeof(packHead) + sizeof(char)*2, 0, machine, totalNumber, index);
		memcpy(tmpSendBuf, &head, sizeof(packHead));
		index += 1;

		//3.checkNum
		checkNum = crc326(tmpSendBuf, head.contentLenth);
		memcpy(tmpSendBuf + head.contentLenth, &checkNum, sizeof(int));

		//4.alloc The send buffer memory
		if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
		{
			ret = -1;
			myprint("Err : func mem_pool_alloc()");
			goto End;
		}

		//5.escape
		ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenthDest);
		if(ret)
		{
			printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
			goto End;
		}

		//6.push The send Data Addr And lenth in queue
		*sendBufStr = 0x7e;
		*(sendBufStr + 1 + outDataLenthDest) = 0x7e;			
		if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenthDest + 2)) < 0)
		{
			myprint("Err : func push_queue_send_block()");
			assert(0);
		}	

		sem_post(&(g_thid_sockfd_block[srcIndex].sem_send));
		//sem_post(&g_sem_send);

	}while(index < totalNumber);

	//print_client_request_reply(tmpOutSendBuf);
End:
	if(requestContent)  free(requestContent);
	
	return ret;
}

void print_hex_buf(char *buf, int lenth)
{
	int i = 0;

	for(i = 0; i < lenth; i++)
	{
		printf("%x ", buf[i]);
	}
	printf("\n");
}




//模板图片集合 数据包	13号数据包应答		
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int upload_template_set_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0;
	packHead head, *tmpHead = NULL;
	char *tmp = NULL, *sendBufStr = NULL;
	unsigned int contentLenth = 0;
	char *machine = "54454547871";
	int checkNum = 0;
	char tmpSendBuf[1500] = { 0 };

	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	
	#if 1
	//保存图片
	ret = save_set_picture_func(recvBuf, recvLen);	
	if(ret < 0 )
	{
		printf("error: func save_set_picture_func() !!! [%d], [%s] \n", __LINE__, __FILE__);
		goto End;
	}
	#endif	
	
	//0.打印图片信息, 强转类型为结构体,获取其中的信息
	printf_pack_news(recvBuf);				//打印接收图片的信息	
	tmpHead = (packHead*)recvBuf;
	
	//1.组包消息体, 按接收成功处理
	tmp = tmpSendBuf + sizeof(packHead); 
	*tmp++ = *(recvBuf + sizeof(packHead));
	
	#ifdef ERRORINPUT	
		if(*(recvBuf + sizeof(packHead)) == 3 && tmpHead->packgeNews.indexPackge == 100)			//这个文件的这包数据会发送出错
		{
			//assert(0);
			*tmp++ = 1;
			*tmp++ = 1;		
			contentLenth = sizeof(packHead) + sizeof(unsigned char) * 3;	
		}
		else
		{
			*tmp++ = 0;
			contentLenth = sizeof(packHead) + sizeof(unsigned char) * 2;
		}
	#else
		*tmp++ = 0;
		contentLenth = sizeof(packHead) + sizeof(unsigned char) * 2;
	#endif
	
	//2.组包消息头
	assign_pack_head(&head, 0x8D, contentLenth, 1, machine, tmpHead->packgeNews.totalPackge, tmpHead->packgeNews.indexPackge);
	memcpy(tmpSendBuf, &head, sizeof(packHead));
	
	//3.组合校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth );
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//4. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, contentLenth + 4, sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//5. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	
	
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
		assert(0);
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));

//	sem_post(&g_sem_send);
	
End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
	
	return ret;	
}


//模板扩展数据包 数据包			12 号数据包应答
/*@param : recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  
*@param :  recvLen : 长度为 buf 的内容 长度
*@param :  sendBuf : 发送 client 的数据内容
*@param :  sendLen : 发送 数据内容的长度
*/
int template_extend_element_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0;
	packHead head;
	char *tmp = NULL, *sendBufStr = NULL;
	unsigned int contentLenth = 0;
	char *machine = "54454547871";
	int checkNum = 47515;
	char tmpSendBuf[1500] = { 0 };


	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	printf_pack_news(recvBuf);				//打印接收图片的信息
	
	//1.组包消息头
	contentLenth = sizeof(packHead) + sizeof(unsigned char) * 1;
	assign_pack_head(&head, 0x8C, contentLenth, 1, machine, 0, 0);
	
	//2.组包消息体, 按接收成功处理
	tmp = tmpSendBuf + sizeof(packHead); 
	memcpy(tmpSendBuf, &head, sizeof(packHead));
	*tmp++ = 0;
	
	//3.组合校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth );
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//4. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, contentLenth + sizeof(uint32_t), sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//5. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	

	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	//sem_post(&g_sem_send);
	
End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;	
}



//删除图片应答 数据包			9 号数据包
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int delete_func_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0, checkNum = 48848748;
	char tmpSendBuf[1500] = { 0 };
	char *tmp = NULL, *sendBufStr = NULL;	
	char *machine = "123456789321";
	char fileName[128] = { 0 };
	char rmFilePath[512] = { 0 };
	packHead head, *tmpHead = NULL;

	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	//1.printf The recvBuf news And delete The picture	
	printf_pack_news(recvBuf);			//打印接收图片的信息
	memcpy(fileName, recvBuf + sizeof(packHead), recvLen - sizeof(packHead) - sizeof(int));
	sprintf(rmFilePath, "rm %s/%s", g_DirPath, fileName);	
	ret = system(rmFilePath);
	
	//2.package The body news	
	tmp = tmpSendBuf;	
	tmp += sizeof(packHead);		
	if(ret < 0 || ret == 127)
	{		
		*tmp++ = 0x01;	
		*tmp++ = 0;
		outDataLenth = sizeof(packHead) + 2;
	}
	else
	{
		*tmp++ = 0x00;	
		outDataLenth = sizeof(packHead) + 1;
	}
		
	//3.组包消息头
	tmpHead = (packHead*)recvBuf;
	assign_pack_head(&head, 0x89, outDataLenth, 1, machine, tmpHead->packgeNews.totalPackge, tmpHead->packgeNews.indexPackge);
	memcpy(tmpSendBuf, &head, sizeof(packHead));
	
	//4.组合校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//5. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//6. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	

	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	//sem_post(&g_sem_send);

End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
}

//退出登录应答 数据包
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int exit_func_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0, checkNum = 48848748;
	char tmpSendBuf[1500] = { 0 };
	char *tmp = NULL, *sendBufStr = NULL;	
	char *machine = "123456789321";
	packHead head, *tmpHead = NULL;
		
	printf_pack_news(recvBuf);			//打印接收图片的信息

	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	//1.package body news
	tmp = tmpSendBuf;			
	tmp += sizeof(packHead);
	*tmp++ = 0x01;	
	outDataLenth =  sizeof(packHead) + 1;
	
	//2.package The head news	
	tmpHead = (packHead*)recvBuf;
	assign_pack_head(&head, 0x82, outDataLenth, 1, machine, tmpHead->packgeNews.totalPackge, tmpHead->packgeNews.indexPackge);
	memcpy(tmpSendBuf, &head, sizeof(packHead));		
	
	//3.calculation checkCode
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//4. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//5. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	
		
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	//sem_post(&g_sem_send);
	
End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
}



//登录应答数据包
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int login_func_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0, checkNum = 48848748;
	char tmpSendBuf[1500] = { 0 };
	char *tmp = NULL, *sendBufStr = NULL;	
	char *machine = "123456789321";
	packHead head, *tmpHead = NULL;
		
	printf_pack_news(recvBuf);			//打印接收图片的信息

	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	
	//1. package The body news
	tmp = tmpSendBuf;	
	tmp += sizeof(packHead);
	*tmp++ = 0x01;	
	outDataLenth = sizeof(packHead) + 1;
	
	//2.package The head news	
	tmpHead = (packHead*)recvBuf;
	assign_pack_head(&head, 0x81, outDataLenth, 1, machine, tmpHead->packgeNews.totalPackge, tmpHead->packgeNews.indexPackge);
	memcpy(tmpSendBuf, &head, sizeof(packHead));				
	
	//3.组合校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//4. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//5. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	

	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	//sem_post(&g_sem_send);
End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
	
}

//上传图片应答 数据包;  服务器收一包 回复一包.
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int upload_func_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0, checkNum = 48848748;
	char tmpSendBuf[1500] = { 0 };
	char *tmp = NULL, *sendBufStr = NULL;	
	char *machine = "123456789321";
	packHead head, *tmpHead = NULL;

	printf_pack_news(recvBuf);					//打印接收图片的信息

	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	
	#if 1
	//保存图片
	ret = save_picture_func(recvBuf, recvLen);	
	if(ret)
	{
		printf("error: func save_picture_func() !!! [%d], [%s] \n", __LINE__, __FILE__);
		goto End;
	}
	#endif	
	
	//1.package The body News
	tmp = tmpSendBuf + sizeof(packHead);
	*tmp++ = 0x01;	
	outDataLenth = sizeof(packHead) + 1;
	
	//2.package The head news	
	tmpHead = (packHead*)recvBuf;
	assign_pack_head(&head, 0x86, outDataLenth, 1, machine, tmpHead->packgeNews.totalPackge, tmpHead->packgeNews.indexPackge);
	memcpy(tmpSendBuf, &head, sizeof(packHead));	
		
	//3.组合校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy(tmp, (char *)&checkNum, sizeof(checkNum));
		
	//4. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//5. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;
	
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	//sem_post(&g_sem_send);
		
End:
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
}

int push_info_toUi_reply(char *recvBuf, int recvLen, int index)
{
	myprint("push infomation OK ... ");
	return 0;
}


//服务器消息推送
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int push_info_toUi(int index)
{
	int ret = 0, outDataLenth = 0, checkNum = 0;
	char *tmp = NULL, *sendBufStr = NULL;
	int fileType = 0;
	packHead  head;
	char tmpSendBuf[1500] = { 0 };
	char *machine = "54454547871";

	
	myprint("The queue have element number : %d ...", get_element_count_send_block(g_sendData_addrAndLenth_queue));

	//2.alloc The memory block
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	

	//3.package body news
	tmp = tmpSendBuf + sizeof(packHead);
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
	outDataLenth = sizeof(packHead) + 65;
	tmp += 64;
		
	//4.package The head news	
	assign_pack_head(&head, 0x87, outDataLenth, 1, machine, 1, 0);
	memcpy(tmpSendBuf, &head, sizeof(packHead));		
	
	//5.calculation checkCode
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
	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));
	
End:
	if(ret < 0) 	if(sendBufStr)		mem_pool_free(g_memoryPool, sendBufStr);

	return ret;


}



//保存图片
//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
int save_picture_func(char *recvBuf, int recvLen)
{
	char *tmp = NULL;
	int lenth = 0;
	char TmpfileName[100] = { 0 };
	char fileName[100] = { 0 };
	
	//1.读取上传的文件
	memcpy(TmpfileName, recvBuf + sizeof(packHead), 46);
	sprintf(fileName, "/%s/%s", g_DirPath, TmpfileName );
	
	//将 指针指向 图片内容的部分
	tmp = recvBuf + sizeof(packHead) + 46 ;
	
	
	//将消息体 写入文件
	FILE *fp = fopen(fileName, "ab+"); 
	if(fp == NULL)
		std_err("fopen");
	
	lenth = fwrite(tmp, 1, recvLen - sizeof(packHead) - sizeof(int) - 46, fp);
	if(lenth < 0)
		std_err("fwrite");
	
	fflush(fp);
	fclose(fp);
	
	return 0;
	
}

//打印接收内容的信息
void printf_pack_news(char *recvBuf)
{
	
	packHead *tmpHead = NULL;
	
	tmpHead = (packHead*)recvBuf;
	printf("The package cmd : %d, indexNumber : %d, totalNumber : %d, contentLenth : %d, seriaNumber : %u, [%d], [%s] \n",tmpHead->cmd, tmpHead->packgeNews.indexPackge, 
			tmpHead->packgeNews.totalPackge, tmpHead->contentLenth, tmpHead->seriaNumber, __LINE__, __FILE__);
	
}
#if 1
// packet cmd data process
int switch_cmd_data(char *actiBuf, int acatiLen, char **sendBuf, int *sendLen)
{
	int ret = 0;
	packHead *tmpHead = NULL;
	uint8_t cmd;
	
	tmpHead = (packHead *)actiBuf;
	cmd = (uint8_t)tmpHead->cmd;

	switch (cmd){
		case 0x01: 		//net服务层 出错数据包;  标识链接断开, 不管是正常断开还是非正常断开, 此时上层到会去重连 服务器
			if((ret = data_un_packge(login_func_reply, actiBuf, acatiLen, ret)) < 0)
				printf("Error: login_func_reply()  \r\n");
			break;
		case 0x02: 		//net服务层 出错数据包;  标识链接断开, 不管是正常断开还是非正常断开, 此时上层到会去重连 服务器
			if((ret = data_un_packge(exit_func_reply, actiBuf, acatiLen, ret)) < 0)
				printf( "Error: exit_func_reply()  \r\n");
			break;
		case 0x06: 		//net服务层 出错数据包;  标识链接断开, 不管是正常断开还是非正常断开, 此时上层到会去重连 服务器
			if((ret = data_un_packge(upload_func_reply, actiBuf, acatiLen, ret)) < 0)
				printf( "Error: upload_func_reply() \r\n");
			break;
		case 0x09: 		//net服务层 出错数据包;  标识链接断开, 不管是正常断开还是非正常断开, 此时上层到会去重连 服务器
			if((ret = data_un_packge(delete_func_reply, actiBuf, acatiLen, ret)) < 0)
				printf( "Error: delete_func_reply()  \r\n");
			break;
		default:
			{
				printf_pack_news(actiBuf);			
				assert(0);
			}
		}
	return ret;
}
#endif

/*read_package  读一包数据;
 *参数: buf: 主调函数分配内存, 拷贝读取到的一包数据;
 *		maxline:  buf内存的最大空间
 *		sockfd:   链接的接字
 *		tmpFlag:  当 所取的 数据包不正确时, 再次重读, 需要保留前面的标识符, 保证不错过数据包
 *返回值: 返回读取的数据包中含有的 字节数;  == -1; 表示出错;  =0: 表示对端关闭;  > 0: 标识正确,即一个数据包的字节数
*/
int read_package(char *recvBuf, int dataLenth, char **sendBuf, int *sendLen)
{
	int ret = 0, i = 0;
	char buf[1500] = { 0 };
	int  bufLen = 0;	
	char actiBuf[1500] = { 0 };
	int  acatiLen = 0;
	static int flag = 0;			// sign for the header and tailer identifier;  0: header; 1:tailer;
	char *tmpBuf = buf;
	packHead  *tmpHead = NULL;
	FILE *fp = NULL;	
	int fileLen = 0;			
	for(i = 0; i < dataLenth; i++)
	{
		if(i == dataLenth -1)		//print the last byte in buf;
		{
			printf("i == nread -1; recvBuf[i]: %d,  [%d], [%s]\n", recvBuf[i],  __LINE__, __FILE__);						
		}
		if(i == 0)				//print the first byte in buf;
		{
			printf("i == 0; recvBuf[i]: %d,  [%d], [%s]\n", recvBuf[i],  __LINE__, __FILE__);						
		}
				
		if(recvBuf[i] == 0x7e && flag == 0)	//find the header identifier 
		{					
			//找到一个标识开头标识符，
			++flag;
			continue;	
		}
		else if(recvBuf[i] == 0x7e && flag == 1)		//find the tail identifier
		{
			// find the whole package;
			
			// 1. anti_escape the recv data;
			ret = anti_escape(buf, bufLen, actiBuf, &acatiLen);
			if(ret)
			{
				printf("反转义出错!!!! [%d], [%s] \n", __LINE__, __FILE__);
				break;
			}
			tmpHead = (packHead*)actiBuf;		
			// 2. check the data lenth
			if(tmpHead->contentLenth + sizeof(uint32_t) == acatiLen)
			{
				// 3. data packet processing;	
				ret = switch_cmd_data(actiBuf, acatiLen, sendBuf, sendLen);
				if(ret)
				{
					printf("error: func switch_cmd_data(), [%d], [%s] \n", __LINE__, __FILE__);
					flag = 0;
					break;
				}
				else
				{
					flag = 0;		//将静态变量置 0;
				}	
			}
			else
			{
				flag = 0;
				printf("error: mpHead->contentLenth + sizeof(int) != acatiLen!!!! [%d], [%s] \n", __LINE__, __FILE__);
				break;
			}			
		}
		else 
		{
			//copy the data packet
			*tmpBuf++ = recvBuf[i];
			bufLen += 1;
			continue;	
		}
	}

	if(recvBuf[i-1] != 0x7e)
	{
		fp = fopen("error_file.txt", "w");
		fileLen = fwrite(recvBuf, 1, dataLenth, fp);
		if(fileLen != dataLenth)
			assert(0);
		fclose(fp);
		printf("no find the whole packet : %d,  [%d], [%s]\n", recvBuf[i-1],  __LINE__, __FILE__);	
		flag = 0;					
		ret = 2;
	}	

	return ret;	
}


int std_err(const char* name)
{
    perror(name);
    exit(1);
}

void func_data(int sig)
{
	int pid = 0;
	while((pid = waitpid(-1, NULL, WNOHANG)) > 0)
	{
		;
	}
	
}

void func_pipe(int sig)
{
	int ret = 0;
	socket_log( SocketLevel[4], ret, "SIGPIPIE appear %d",  sig);	
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
	else
		printf(" func inet_pton() OK , [%d], [%s]\n", __LINE__, __FILE__);

	
    //绑定服务器的IP、端口；
    ret = bind(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(ret == -1)
        std_err("bind");


    //监听链接服务器的客户数量
    ret = listen(sfd, 99);
    if(ret == -1)
        std_err("listen");
	
	*listenFd = sfd;
	
	return ret;
}

//清空 套接字缓存区的数据;
//思想: 使用select机制, 实现一个非阻塞套接字, 然后监控该套接字的读行为,
//读取出数据, 丢弃或者其他处理;  当套接字中无数据时, 就跳出循环.
int removefd(int sockfd)
{
    int ret = 0;
    int nRet = 0;
    char buf[2] = { 0 };

    struct timeval tmOut;
    tmOut.tv_sec = 0;
    tmOut.tv_usec = 0;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);
    
    while(1)
    {
        nRet = select(sockfd + 1, &fds, NULL, NULL, &tmOut);

        if(nRet == 0)
            break;
        recv(sockfd, buf, 1, 0);
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

//清空 套接字缓存区的数据;
int removed_sockfd_buf(int sockfd)
{
	int ret = 0, len = 0;
	char buf[1024] = { 0 };
	activate_nonblock(sockfd);
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
	

	deactivate_nonblock(sockfd);
	return ret;
}

#if 0
int ssl_server_init_handle(int socketfd, SSL_CTX **srcCtx, SSL **srcSsl)
{
	int ret = 0;
	const SSL_METHOD *meth = NULL;    
	SSL_CTX *ctx = NULL;    
	SSL *myssl = NULL;

	if(socketfd <= 0 || !srcCtx || !srcSsl)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Err : socketfd : %d, srcCtx : %p, srcSsl : %p", socketfd, srcCtx, srcSsl);
		goto End;
	}

	//1. init 
	SSL_library_init();    
	SSL_load_error_strings();	
	meth=SSLv23_server_method();
	myprint("===========  2  ============");

    /*2. Create a new context block*/
	ctx=SSL_CTX_new(meth);    
	if (!ctx) 
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Err : func SSL_CTX_new() creating the context");
		goto End;
	}
	/*3. Set cipher list*/
	if (SSL_CTX_set_cipher_list(ctx, CIPHER_LIST) <= 0)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Err : func SSL_CTX_set_cipher_list() setting the cipher list");
		goto End;
	}
	/*4. Indicate the certificate file to be used*/
	if (SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0) 
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Err : func SSL_CTX_use_certificate_file() setting the certificate file");
		goto End;
	}
				myprint("===========  3  ============");
	/*5. Indicate the key file to be used*/
	if (SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM) <= 0) 
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Err : func SSL_CTX_use_PrivateKey_file() setting the key file");		
		goto End;
	}
	/*6. Make sure the key and certificate file match*/
	if (SSL_CTX_check_private_key(ctx) == 0) 
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Err : func SSL_CTX_check_private_key() Private key does not match the certificate public key");		
		goto End;		
	}
				myprint("===========  4  ============");
	/*7. Used only if client authentication will be used*/
	SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,NULL);
	/*8. Set the list of trusted CAs based on the file and/or directory provided*/
	if(SSL_CTX_load_verify_locations(ctx,CA_FILE, NULL)<1)
	{   
		ret = -1;
		socket_log( SocketLevel[4], ret, "Err : func SSL_CTX_load_verify_locations() setting the CA certificate file");		
		goto End;
	}
	/*9. Create new ssl object*/
	myssl=SSL_new(ctx);
	if(!myssl) 
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Err : func SSL_new()");		
		goto End;
	}
				myprint("===========  5  ============");

	/*10. Bind the socket to the SSL structure*/
	SSL_set_fd(myssl, socketfd);
	/*11. Connect to the server, SSL layer.*/
	ret = SSL_accept(myssl);
	if (ret != 1) 
	{
		myprint("===========  Err : func SSL_accept()  ============");
		ret=SSL_get_error(myssl,ret);
		socket_log( SocketLevel[4], ret, "Err : func SSL_get_error() SSL error #%d in accept,program terminated\n",ret);		
		ret = -1;
		goto End;		
	}
				myprint("===========  6  ============");
	printf("\n\nSSL connection on socket Version: %s, Cipher: %s ......\n", 		
				SSL_get_version(myssl),  SSL_get_cipher(myssl));
		
End:
	myprint("===========  ret : %d ============", ret);
	if(ret < 0)
	{		
		if(socketfd > 0)		close(socketfd);
		if(myssl)				SSL_free(myssl);
		if(ctx) 				SSL_CTX_free(ctx);	
	}
	else
	{	
		*srcCtx = ctx;
		*srcSsl = myssl;
	}
	
	return ret;	
}
#endif

int encryp_or_cancelEncrypt_transfer(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0, checkNum = 48848748;
	char tmpSendBuf[1500] = { 0 };
	char *tmp = NULL, *sendBufStr = NULL;	
	char *machine = "123456789321";
	packHead head, *tmpHead = NULL;
		
	printf_pack_news(recvBuf);			//打印接收图片的信息

	tmp = recvBuf + sizeof(packHead);

	if(*tmp == 0)		g_thid_sockfd_block[index].encryptRecvFlag = true;			//encrypt
	else if(*tmp == 1)	g_thid_sockfd_block[index].encryptRecvFlag = false;			//cancel encrypt
	else				assert(0);
	g_thid_sockfd_block[index].encryptChangeFlag = true;
	
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//1. package The body news
	*(tmpSendBuf + sizeof(packHead)) = 0;
	outDataLenth = sizeof(packHead) + 1;

	//2.package The head news	
	tmpHead = (packHead*)recvBuf;
	assign_pack_head(&head, 0x8E, outDataLenth, 1, machine, tmpHead->packgeNews.totalPackge, tmpHead->packgeNews.indexPackge);
	memcpy(tmpSendBuf, &head, sizeof(packHead));

	//3.组合校验码
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmpSendBuf + head.contentLenth, (char *)&checkNum, sizeof(checkNum));

	//myprint("\n\n ****(((((((( checkCode : %d", checkNum);
	//4. 转义数据内容
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//5. 组合发送的内容
	*sendBufStr = 0x7e;
	*(sendBufStr + 1 + outDataLenth) = 0x7e;	

	if((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2)) < 0)
	{
		myprint("Err : func push_queue_send_block()");
	}

	sem_post(&(g_thid_sockfd_block[index].sem_send));

End:	
	if(ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);
		
	return ret;
}



