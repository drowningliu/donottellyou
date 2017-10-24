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

//�궨��
#define 	ERRORINPUT												//�����ϴ�ͼƬ����
#define 	SCAN_SOURCE_DIR  "sourcefile"							//ɨ��Ŀ¼
#define 	CERT_FILE 	"/home/yyx/nanhao/server/server-cert.pem" 	//֤��·��
#define 	KEY_FILE  	"/home/yyx/nanhao/server/server-key.pem" 	//˽Կ·��
#define 	CA_FILE 	"/home/yyx/nanhao/ca/ca-cert.pem" 			//CA ֤��·��
#define 	CIPHER_LIST "AES128-SHA"								//���õļ����㷨
#define 	NUMBER		15											//ͬһʱ����Խ��յĿͻ�����
#define     SCAN_TIME 	15											//ɨ����ʱ�� 15s
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



//ȫ�ֱ���
char 			g_DirPath[512] = { 0 };						//�ϴ��ļ�·��
unsigned int 	g_recvSerial = 0;							//������ ���յ� ���ݰ���ˮ��
unsigned int 	g_sendSerial = 0;							//������ ���͵� ���ݰ���ˮ��
long long int 	g_fileTemplateID = 562, g_fileTableID = 365;//��������Դ�ļ�ID 
char 			*g_downLoadDir = NULL;						//��������Դ·��
char 			*g_fileNameTable = "downLoadTemplateNewestFromServer_17_05_08.pdf";	//���ݿ������
char 			*g_fileNameTemplate = "C_16_01_04_10_16_10_030_B_L.jpg";			//ģ������
int  			g_listen_fd = 0;							//�����׽���
RecvAndSendthid_Sockfd		g_thid_sockfd_block[NUMBER];	//�ͻ�����Ϣ
char 			*g_sendFileContent = NULL;					//��ȡÿ������ģ�������

//���к��ڴ��
mem_pool_t  *g_memoryPool = NULL;					//mempool handle
queue_send_dataBLock   g_sendData_addrAndLenth_queue;


//ɨ���߳�, ȫ�ֶ�һ��
pthread_t  g_pthid_scan_dir;

//������,
pthread_mutex_t	 g_mutex_pool = PTHREAD_MUTEX_INITIALIZER;			

//�Ƴ��׽��ֻ���������
int removed_sockfd_buf(int sockfd);			
//�����Դ
void clear_global();			
//���ܾ����ʼ��
//int ssl_server_init_handle(int socketfd, SSL_CTX **srcCtx, SSL **srcSsl);
//�������ݵ�������ѯ
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
	int 			sfd, cfd;				//�����������׽���, �ͻ��������׽���
	pthread_t 		pthidWrite = -1;		//���������߳�ID
	pthread_t		pthidRead = -1;			//���������߳�ID
    socklen_t 		clie_len;				//��ַ����
	struct sockaddr_in clie_addr;			//�ͻ��˵�ַ
	int 			conIndex = 0;
	static bool  	createFlag = false;		//�Ƿ�Ϊ��ʼ��, ɨ���߳�ֻ����һ��
	//SSL_CTX 		*ctx = NULL;			//����֤����Ϣ
	//SSL 			*ssl = NULL;			//���ܾ��

	//1.�鿴�ϴ�Ŀ¼�Ƿ����
	if(access(uploadDirParh,  0) < 0 )
	{
		//2.������, ���������ϴ��ļ���Ŀ¼
		if((ret = mkdir(uploadDirParh, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) < 0)
		{
			myprint("Error : func mkdir() : %s", uploadDirParh);
			assert(0);
		}
	}

	//3.����·������
	memcpy(g_DirPath, uploadDirParh, strlen(uploadDirParh));
	myprint("uploadFilePath : %s", g_DirPath);
	memset(g_thid_sockfd_block, 0, sizeof(g_thid_sockfd_block));

	//4.ע���ź�
	signal(SIGUSR1, signUSR1_exit_thread);
	signal(SIGPIPE, func_pipe); //���������� �����׽���

	//5.���������׽���
	if((ret = server_bind_listen_fd(ip, port, &sfd)) < 0)
	{
		myprint("Error : func server_bind_listen_fd()");
		assert(0);
	}
    clie_len = sizeof(clie_addr); 
	g_listen_fd = sfd;

	//6.��ʼ�� �ڴ�غͶ���
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
	
	//7.��ʼ���ͻ��˻������ź���
	for(i = 0; i < NUMBER; i++)
	{
		sem_init(&(g_thid_sockfd_block[i].sem_send), 0, 0);
	}

	//8.��ȡɨ��·��	
	if((g_downLoadDir = getenv("SERverSourceDir")) == NULL)
	{
		myprint("Err : func getenv() key = SERverSourceDir");
		assert(0);
	}

	//9.���пͻ������Ӵ���
	while(1)
	{
		//10 �����ȴ��ͻ��˷�����������
	    if((cfd = accept(sfd, (struct sockaddr*)&clie_addr, &clie_len)) < 0)	   	    
	    {
		   	myprint("Err : func accept()");
			assert(0);
	    }
		else
		{
#if 0
			//11.�������ܾ��	
			if((ret = ssl_server_init_handle(cfd , &ctx, &ssl)) < 0)
			{
				close(cfd);
				myprint("\n\nErr : func ssl_server_init_handle()... "); 	
				return ret;
			}
#endif
		
			//12. �����ͻ��˻�����, �ҳ����е�λ��
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

			//13. �Կͻ��˻��������и�ֵ
			g_thid_sockfd_block[conIndex].flag = 1;
			g_thid_sockfd_block[conIndex].sockfd= cfd;
	//		g_thid_sockfd_block[conIndex].sslCtx = ctx;
	//		g_thid_sockfd_block[conIndex].sslHandle = ssl;
			g_thid_sockfd_block[conIndex].encryptChangeFlag = false;
			g_thid_sockfd_block[conIndex].encryptRecvFlag = false;
			g_thid_sockfd_block[conIndex].encryptSendFlag = false;		
			g_thid_sockfd_block[conIndex].send_flag = 0;

			//14. �������̣߳� ȥִ�з��� ҵ��
			if((ret = pthread_create(&pthidWrite, NULL, child_write_func, (void *)conIndex)) < 0)			
			{
				myprint("err: func pthread_create() ");		
				assert(0);
			}
			//�������̣߳� ȥִ�н��� ҵ��
			if((ret = pthread_create(&pthidRead, NULL, child_read_func, (void *)conIndex)) < 0)
			{
				myprint("err: func pthread_create() ");		
				assert(0);
			}

	#if 1	
			//15. ����ɨ���߳�, ֻ����һ��
			if(!createFlag)
			{
				//�������̣߳� ȥִ��ɨ��Ŀ¼ҵ��, ������Ϣ����
				if((ret = pthread_create(&g_pthid_scan_dir, NULL, child_scan_func, NULL)) < 0)			
				{
					myprint("err: func child_scan_func() "); 	
					assert(0);
				}
				createFlag = true;
			}
	#endif	

			//16. �Կͻ��˻��������и�ֵ
			g_thid_sockfd_block[conIndex].pthidRead = pthidRead;
			g_thid_sockfd_block[conIndex].pthidWrite= pthidWrite;
	
		}

	}

	
	//4.������Դ
	destroy_server();

	return ret;
}

//���ٷ���
void destroy_server()
{	
	int i = 0;
	
	//1. �رռ����׽���
	close(g_listen_fd);

	//2. ���ٿͻ��������ź���
	for(i = 0; i < NUMBER; i++)
	{		
		sem_destroy(&(g_thid_sockfd_block[i].sem_send));		
	}	

	//3.���ٻ�����
	pthread_mutex_destroy(&g_mutex_pool);

	//4.�ͷ��ڴ�غͶ���
	mem_pool_destroy(g_memoryPool);
	destroy_queue_send_block(g_sendData_addrAndLenth_queue);

	//5.�ͷ�ȫ�ֱ���
	if(g_sendFileContent)		free(g_sendFileContent);
}


//select() ��ʱ�� 
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



//��������Ϣ����
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
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
		
	//6. ת����������
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//7. ��Ϸ��͵�����
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
	//1.��ȡĿ¼
	if((direct = getenv("SERverSourceDir")) == NULL)
	{
		myprint("Err : func getenv() key = SERverSourceDir");
		assert(0);
	}
	g_downLoadDir = direct;
	#endif

	//2. ��Ŀ¼, ��ȡĿ¼�е��ļ�
	if((directory_pointer = opendir(g_downLoadDir)) == NULL)
	{
		myprint("Err : func opendir() Dirpath : %s", direct);
		assert(0);
	}
	else
	{
		//3. ��ȡĿ¼�е��ļ�
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
	//4.������ȡ��Ŀ¼�µ��ļ�, ��ȡ���ǵ�ʱ������
	node = start.next;
	while(node)
	{	
		//4.1 ȥ��'.'��'..'Ŀ¼
		if(strcmp(node->fileName, tmp01) == 0 || strcmp(node->fileName, tmp02) == 0)
		{
			node = node->next;
			continue;
		}
			
		//4.2 ��ȡʣ���ļ�����
		memset(&fileStat, 0, sizeof(struct stat));
		if((stat(node->fileName, &fileStat)) < 0)
		{
			myprint("Err : func stat() filename : %s", node->fileName);
			assert(0);
		}

		//4.3 ���δ򿪸�Ŀ¼, �������ļ�������������
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
		else		//���״�, ���������ж�
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


//��������
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

		//1. �ȴ��ź���֪ͨ
		sem_wait(&(g_thid_sockfd_block[index].sem_send));		
		
		//2. ��ȡ�������ݵĵ�ַ�ͳ���
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

		//3. ѡ�������ݵķ�ʽ, �������ݷ���
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
			//4. ���ͼ���Ӧ���,�������������߳� �ļ��ܽ��� 
			if(g_thid_sockfd_block[index].encryptChangeFlag)
			{
				g_thid_sockfd_block[index].encryptSendFlag = g_thid_sockfd_block[index].encryptRecvFlag;
				g_thid_sockfd_block[index].encryptChangeFlag = false;
				//2.1.1. send signal To thread recv
				myprint("will kill USR2 SIGNAL to read thread ID : %lu", g_thid_sockfd_block[index].pthidRead);
				if((ret = pthread_kill(g_thid_sockfd_block[index].pthidRead, SIGUSR2)) < 0)		assert(0);				
			}

			//5. �ͷ��ڴ����ڴ��
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

	//6.�رտͻ��������׽���
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


//���߳�ִ���߼�;  ����ֵ: 0:��ʶ��������, 1: ��ʶ�ͻ��˹ر�;  -1: ��ʶ����.
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

    //1. �����̹߳����߼�,
    while(1)
    {  
    	memset(recvBuf, 0 ,  sizeof(recvBuf));

		//2.ѡ��������ݵķ�ʽ
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
	        //3. ��ȷ��������, ����������������
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
       		//4. ���ճ���
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
       		//5. �ͻ��˹رշ���
       		myprint("client closed ");
			g_thid_sockfd_block[index].flag = 0;
			clear_global();
       		break;
       	}	      	
	
	} 

	//6. �رհ�ȫ�׽��ֲ�� ��ͨ�׽���
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

	//7. ���ź�, �رշ����߳�
	if((ret = pthread_kill(g_thid_sockfd_block[index].pthidWrite, SIGUSR1)) < 0)		assert(0);					
	pthread_exit(NULL);
	return NULL;
}


//function: find  the whole packet; retval maybe error
int	find_whole_package(char *recvBuf, int recvLenth, int index)
{
	int ret = 0, i = 0;
	int  tmpContentLenth = 0;			//ת�������ݳ���
	char tmpContent[1400] = { 0 }; 		//ת��������
	static int flag = 0;				//flag ��ʶ�ҵ���ʶ��������;
	static int lenth = 0; 				//lenth : ��������ݰ����ݳ���
	static char content[2800] = { 0 };	//������������������, 		
	char *tmp = NULL;


	tmp = content + lenth;
	for(i = 0; i < recvLenth; i++)
	{
		//1. �������ݱ�ͷ
		if(recvBuf[i] == 0x7e && flag == 0)
		{
			flag = 1;
			continue;
		}
		else if(recvBuf[i] == 0x7e && flag == 1)	
		{		

			//2. �������ݱ�β, ��ȡ���������ݰ�, �����ݰ����з�ת�� 
			if((ret = anti_escape(content, lenth, tmpContent, &tmpContentLenth)) < 0)
			{
				myprint("Err : func anti_escape(), srcLenth : %d, dstLenth : %d",lenth,tmpContentLenth);
				goto End;
			}

			//3.��ת�������ݰ����д���
			if((ret = read_data_proce(tmpContent, tmpContentLenth, index)) < 0)
			{
				myprint("Err : func read_data_proce(), dataLenth : %d", tmpContentLenth);
				goto End;
			}
			else
			{
				//4. ���ݰ�����ɹ�, �����ʱ����
				memset(content, 0, sizeof(content));
				memset(tmpContent, 0, sizeof(tmpContent));								
				lenth = 0;
				flag = 0;	
				tmp = content + lenth;
			}
		}
		else
		{
			//5.ȡ�����ݰ�����, ȥ����ʼ��ʶ��
			*tmp++ = recvBuf[i];
			lenth += 1;
			continue;
		}
	}


End:

	
	return ret;
}




//���տͻ��˵����ݰ�, recvBuf: ����: ��ʶ�� + ��ͷ + ���� + У���� + ��ʶ��;   recvLen : recvBuf���ݵĳ���
//				sendBuf : �������ݰ�, ��Ҫ���͵�����;  sendLen : �������ݵĳ���
int read_data_proce(char *recvBuf, int recvLen, int index)
{
	int ret = 0 ;	
	uint8_t  cmd = 0;
	
	//1.��ȡ������
	cmd = *((uint8_t *)recvBuf);
			
	//2.ѡ����ʽ
	switch (cmd){
		case LOGINCMDREQ: 		//��¼���ݰ�
			if((ret = data_un_packge(login_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func login_func_reply() ");				
			break;			
		case LOGOUTCMDREQ: 		//�˳����ݰ�
			if((ret = data_un_packge(exit_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func exit_func_reply() ");				
			break;
		case HEARTCMDREQ: 		//�������ݰ�
			if((ret = data_un_packge(heart_beat_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func heart_beat_func_reply() ");
			break;			
		case DOWNFILEREQ: 		//ģ������			
			if((ret = data_un_packge(downLoad_template_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func downLoad_template_func_reply() ");
			break;					
		case NEWESCMDREQ: 		//�ͻ�����Ϣ����
			if((ret = data_un_packge(get_FileNewestID_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func client_request_func_reply() ");
			break;
		case UPLOADCMDREQ: 		//�ϴ�ͼƬ���ݰ�
			if((ret = data_un_packge(upload_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func upload_func_reply() ");
			break;			
		case PUSHINFOREQ:		//��Ϣ����
			if((ret = data_un_packge(push_info_toUi_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func push_info_toUi_reply() ");
			break;
		case DELETECMDREQ: 		//ɾ��ͼƬ���ݰ�
			if((ret = data_un_packge(delete_func_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func delete_func_reply() ");
			break;		
		case TEMPLATECMDREQ:		//�ϴ�ģ��
			if((ret = data_un_packge(template_extend_element_reply, recvBuf, recvLen, index)) < 0)
				myprint("Error: func template_extend_element() ");
			break;			
		case MUTIUPLOADCMDREQ:		//�ϴ�ͼƬ��
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



//��¼Ӧ�����ݰ�
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
int login_func_reply(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0;					//�������ݵĳ���
	int  checkNum = 0;								//У����
	char tmpSendBuf[1500] = { 0 };					//��ʱ���ݻ�����
	char *sendBufStr = NULL;						//�������ݵ�ַ
	char *tmp = NULL;
	resCommonHead_t head;							//Ӧ�����ݱ�ͷ
	reqPackHead_t *tmpHead = NULL;					//������Ϣ
	char *userName = "WuKong";						//�����˻����û���
	
	//1.��ӡ����ͼƬ����Ϣ
	printf_comPack_news(recvBuf);	
	tmpHead = (reqPackHead_t *)recvBuf;
	g_recvSerial = tmpHead->seriaNumber;
	g_recvSerial++;

	//2.�����ڴ�
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//3.��װ���ݰ���Ϣ, ��ȡ�������ݰ�����
	tmp = tmpSendBuf + sizeof(resCommonHead_t);
	*tmp++ = 0x01;				//��¼�ɹ�
	*tmp++ = 0x00;				//�ɹ���, �˴���Ϣ������н���, ���д
	*tmp++ = strlen(userName);	//�û�������
	memcpy(tmp, userName, strlen(userName));	//�����û���	
	outDataLenth = sizeof(resCommonHead_t) + sizeof(char) * 3 + strlen(userName);
	
	//4.��װ��ͷ��Ϣ		
	assign_comPack_head(&head, LOGINCMDRESPON, outDataLenth, 0, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));				
	//myprint("outDataLenth : %d, head.contentLenth : %d", outDataLenth, head.contentLenth);
	
	//5.���У����
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp = tmpSendBuf + head.contentLenth;
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//6. ת����������
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//7. ��Ϸ��͵�����
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



//�˳���¼Ӧ�� ���ݰ�
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
int exit_func_reply(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0;					//�������ݵĳ���
	int  checkNum = 0;								//У����
	char tmpSendBuf[1500] = { 0 };					//��ʱ���ݻ�����
	char *sendBufStr = NULL;						//�������ݵ�ַ
	char *tmp = NULL;
	resCommonHead_t head;							//Ӧ�����ݱ�ͷ

	//1. ��ӡ���ݰ���Ϣ		
	printf_comPack_news(recvBuf);			//��ӡ����ͼƬ����Ϣ

	//2. �����ڴ�
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	//3. ������Ϣ����
	tmp = tmpSendBuf + sizeof(resCommonHead_t);
	*tmp++ = 0x01;	
	outDataLenth =  sizeof(resCommonHead_t) + 1;
	
	//4. ��װ��ͷ
	assign_comPack_head(&head, LOGOUTCMDRESPON, outDataLenth, 0, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));		
	
	//5. ����У����
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//6. ת����������
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//7. ��Ϸ��͵�����
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


//�������ݰ�	3�����ݰ�Ӧ��		
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
int heart_beat_func_reply(char *recvBuf, int recvLen, int index)
{		
	int  ret = 0, outDataLenth = 0; 				//�������ݵĳ���
	int  checkNum = 0;								//У����
	char tmpSendBuf[1500] = { 0 };					//��ʱ���ݻ�����
	char *sendBufStr = NULL;						//�������ݵ�ַ
	char *tmp = NULL;
	resCommonHead_t head;							//Ӧ�����ݱ�ͷ
	

	//1. ��ӡ���ݰ���Ϣ
	printf_comPack_news(recvBuf);			

	//2. �����ڴ�
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	//3. ��װ��ͷ��Ϣ
	outDataLenth = sizeof(resCommonHead_t);
	assign_comPack_head(&head, HEARTCMDRESPON, outDataLenth, 0, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));	
	
	//4. ����У����
	tmp = tmpSendBuf + sizeof(resCommonHead_t);
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//5. ת����������
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//6. ��Ϸ��͵�����
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


//����ģ�����ݰ�	4�����ݰ�Ӧ��		
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����  
//recvLen : ����Ϊ buf ������ ����
int downLoad_template_func_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0;
	resSubPackHead_t 	head;					//Ӧ�����ݰ���ͷ
	char *tmp = NULL, *sendBufStr = NULL;
	int contentLenth = 0;
	int checkNum = 0, nRead = 0;				//У����� ��ȡ�ֽ���
	char tmpSendBuf[1400] = { 0 };				//��ʱ������
	int  doucumentLenth = 0; 
	FILE *fp = NULL;
	char fileName[1024] = { 0 };
	int totalPacka = 0;							//���ļ����ܰ���
	int indexNumber = 0;						//ÿ�������
	int fileType = 0;							//���ص��ļ�����		
	int  tmpLenth = 0;
	
	//1. ��ӡ�������ݰ���Ϣ
	printf_comPack_news(recvBuf);

	//2. get The downLoad file Type And downLoadFileID
	tmp = recvBuf + sizeof(reqPackHead_t);
	if(*tmp == 0)			sprintf(fileName, "%s%s",g_downLoadDir, g_fileNameTemplate);		//����ģ��
	else if(*tmp == 1)		sprintf(fileName, "%s%s",g_downLoadDir, g_fileNameTable);			//�������ݿ��
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


//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��); 
//recvLen : ����Ϊ buf ������ ����
int get_FileNewestID_reply(char *recvBuf, int recvLen, int index)
{	
	int  ret = 0, outDataLenth = 0; 				//�������ݵĳ���
	int  checkNum = 0;								//У����
	char tmpSendBuf[1500] = { 0 };					//��ʱ���ݻ�����
	char *sendBufStr = NULL;						//�������ݵ�ַ
	char *tmp = NULL;
	resCommonHead_t head;							//Ӧ�����ݱ�ͷ
	int  fileType = 0;								//������ļ�����


	//1. ��ӡ���ݰ���Ϣ
	//printf_comPack_news(recvBuf);	
	
	//1. ��ȡ�ļ�����
	tmp = recvBuf + sizeof(reqPackHead_t);
	if(*tmp == 0)				fileType = 0;
	else if(*tmp == 1)			fileType = 1;
	else						assert(0);

	myprint("fileType : %d", fileType);
	
	//2. �����ڴ�
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//3. ��װ���ݰ���Ϣ
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
		
	//4. ��װ��ͷ��Ϣ	
	assign_comPack_head(&head, NEWESCMDRESPON, outDataLenth, 0, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));		
	
	//5. ����У����
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp += 64;
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//6. ת����������
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//7. ��Ϸ��͵�����
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


//�ϴ�ͼƬӦ�� ���ݰ�;  ��������һ�� �ظ�һ��.
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
int upload_func_reply(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0;					//�������ݵĳ���
	int  checkNum = 0;								//У����
	char tmpSendBuf[1500] = { 0 };					//��ʱ���ݻ�����
	char *sendBufStr = NULL;						//�������ݵ�ַ
	char *tmp = NULL;
	resCommonHead_t head;							//Ӧ�����ݱ�ͷ
	reqPackHead_t  *tmpHead = NULL;					//����ͷ��Ϣ
	static bool latePackFlag = false;					//true : ͼƬ���һ��, false �����һ������,  
	static int recvSuccessSubNumber = 0;				//ÿ�ֳɹ����յ����ݰ���Ŀ

	
	//1.��ӡ����ͼƬ����Ϣ
	//printf_comPack_news(recvBuf);					

	//3.��ȡ��ͷ��Ϣ
	tmpHead = (reqPackHead_t *)recvBuf;
	
	//4.����������Ϣ�����ж�
	if(tmpHead->isSubPack == 1)
	{
		//5.�Ӱ�, �������ݰ���Ϣ�ж�
		if((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
		{
			//6.������ȷ, ����ͼƬ���ݵĴ洢, ��¼���ֳɹ����յ��Ӱ�����
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
				//7.��ʶÿ�ֳɹ����յ����Ӱ���ĿС�ڷ�����Ŀ, ����Ҫ���н��лظ�				
				ret = 0;
				goto End;
			}
			else if(recvSuccessSubNumber == tmpHead->perRoundNumber)
			{
				//8.��ʶÿ�����е��Ӱ��ɹ�����, �����жϸûظ�����Ӧ��(��ͼƬ�ɹ�����Ӧ���Ǳ��ֳɹ�����Ӧ��)
				recvSuccessSubNumber = 0;
				if(latePackFlag == false)
				{
					//���ֳɹ�����					
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);				
				}
				else if(latePackFlag == true)
				{
					//��ͼƬ�ɹ�����
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
			//9.��Ҫ�ط�			
			outDataLenth = sizeof(resCommonHead_t);
			assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);							
		}
		else
		{
			//10.����Ҫ�ط�
			ret = 0;
			goto End;
		}	
	}
	else
	{		
		//11.�ܰ�, ������Ӧ.
		outDataLenth = sizeof(resCommonHead_t);
		assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
	}

	//12.�����ڴ�
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//12. ������ͷ����, ����У����
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));			
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp = tmpSendBuf + head.contentLenth;
	memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

	//13. ת����������
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)	
	{
		myprint("Error : func escape() escape() !!!!");
		goto End;
	}
	
	//14. ��Ϸ��͵�����
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

//ɾ��ͼƬӦ�� ���ݰ�			9 �����ݰ�
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
int delete_func_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0;					//�������ݰ�����
	int checkNum = 48848748;						//У����
	char tmpSendBuf[1500] = { 0 };					//��ʱ������
	char *tmp = NULL, *sendBufStr = NULL;			//�������ݰ���ַ
	char fileName[128] = { 0 };						//�ļ�����
	char rmFilePath[512] = { 0 };					//ɾ������
	resCommonHead_t head;							//Ӧ�����ݱ�ͷ
	
	//1. ��ӡ���ݰ�����
	printf_comPack_news(recvBuf);			

	//2. �����ڴ�
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//3. ��ȡ�ļ���Ϣ, ��ɾ��
	memcpy(fileName, recvBuf + sizeof(reqPackHead_t), recvLen - sizeof(reqPackHead_t) - sizeof(int));
	sprintf(rmFilePath, "rm %s/%s", g_DirPath, fileName);	
	ret = system(rmFilePath);
	myprint("rmFilePath : %s", rmFilePath);
	//4. ��װ���ݰ���Ϣ
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
		
	//5. ��װ��ͷ
	assign_comPack_head(&head, DELETECMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 1);
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
	
	//6. ���У����
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//7. ת����������
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//6. ��Ϸ��͵�����
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

//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����
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
	//������ͼ����
	Coords coorArray[50];
	char *tmp = NULL;
	int i = 0;
	UitoNetExFrame *tmpStruct = NULL;
	
	memset(fileName, 0, sizeof(fileName));	
	memset(coorArray, 0,  sizeof(coorArray));
	
	tmpHead = (reqPackHead_t*)recvBuf;	
	printf("The package cmd : %d, indexNumber : %d, totalNumber : %d, contentLenth : %d, seriaNumber : %u, [%d], [%s] \n",tmpHead->cmd, tmpHead->currentIndex,
			tmpHead->totalPackage, tmpHead->contentLenth + sizeof(int), tmpHead->seriaNumber, __LINE__, __FILE__);	
	//1. ��ȡID
	tmp = recvBuf + sizeof(reqPackHead_t);
	IDLen = *((unsigned char*)tmp);	
	tmp += 1;
	memcpy(ID, tmp, IDLen);	
	//2. ��ȡ�͹���	
	tmp += IDLen;	
	objectLen = *((unsigned char*)tmp);
	tmp += 1;	
	memcpy(object, tmp, objectLen);	
	//3. ��ȡ�Ծ�����	
	tmp += objectLen;
	fileNum = *((unsigned char*)tmp);
	tmp += 1;	
	for(i = 0; i < fileNum; i++)
	{		
		memcpy(fileName[i], tmp, sizeof(tmpStruct->sendData.toalPaperName[0]));		
		tmp += sizeof(tmpStruct->sendData.toalPaperName[0]);
	}	
	//4. ��ȡ������
	subMapCoordNum = *((unsigned char*)tmp);	
	tmp += 1;	
	for(i = 0; i < subMapCoordNum; i++)	
	{	
		memcpy(&coorArray[i], tmp, sizeof(Coords));	
		tmp += sizeof(Coords);
	}	

	//6. ��ӡ	
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


//ģ����չ���ݰ� ���ݰ�			12 �����ݰ�Ӧ��
/*@param : recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  
*@param :  recvLen : ����Ϊ buf ������ ����
*@param :  sendBuf : ���� client ����������
*@param :  sendLen : ���� �������ݵĳ���
*/
int template_extend_element_reply(char *recvBuf, int recvLen, int index)
{
	int ret = 0, outDataLenth = 0;				//�������ݰ�����
	resCommonHead_t head;						//�������ݱ�ͷ
	char *tmp = NULL, *sendBufStr = NULL;		//���͵�ַ
	unsigned int contentLenth = 0;				//�������ݰ�����
	int checkNum = 0;							//У����
	char tmpSendBuf[1500] = { 0 };				//��ʱ������

	//1. ��ӡ����ͼƬ����Ϣ, �����ڴ�
	printf_template_news(recvBuf);
	
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
			
	//2. �����Ϣͷ
	contentLenth = sizeof(resCommonHead_t) + sizeof(char) * 1;
	assign_comPack_head(&head, TEMPLATECMDRESPON, contentLenth, NOSUBPACK, 0, 0, 1);
	
	//3. �����Ϣ��, �����ճɹ�����
	tmp = tmpSendBuf + sizeof(resCommonHead_t); 
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
	*tmp++ = 0;
	
	//4. ���У����
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth );
	memcpy( tmp, (char *)&checkNum, sizeof(checkNum));
		
	//5. ת����������
	if((ret = escape(0x7e, tmpSendBuf, contentLenth + sizeof(uint32_t), sendBufStr + 1, &outDataLenth)) < 0)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//6. ��Ϸ��͵�����
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


//ģ��ͼƬ���� ���ݰ�	13�����ݰ�Ӧ��		
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
int upload_template_set_reply(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0; 				//�������ݵĳ���
	int  checkNum = 0;								//У����
	char tmpSendBuf[1500] = { 0 };					//��ʱ���ݻ�����
	char *sendBufStr = NULL;						//�������ݵ�ַ
	char *tmp = NULL;
	resCommonHead_t head;							//Ӧ�����ݱ�ͷ
	reqPackHead_t  *tmpHead = NULL; 				//����ͷ��Ϣ
	static bool latePackFlag = false;				//true : ��ģ�����һ��ͼƬ,���һ�����ݰ�, false �����һ������,  
	static int recvSuccessSubNumber = 0;			//ÿ�ֳɹ����յ����ݰ���Ŀ

		
	//1.��ӡ����ͼƬ����Ϣ
	//printf_comPack_news(recvBuf); 				

	//3.��ȡ��ͷ��Ϣ
	tmpHead = (reqPackHead_t *)recvBuf;
	
	//4.����������Ϣ�����ж�
	if(tmpHead->isSubPack == 1)
	{
		//5.�Ӱ�, �������ݰ���Ϣ�ж�
		if((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
		{
			//6.������ȷ, ����ͼƬ���ݵĴ洢, ��¼���ֳɹ����յ��Ӱ�����
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
				//7.��ʶÿ�ֳɹ����յ����Ӱ���ĿС�ڷ�����Ŀ, ����Ҫ���н��лظ�				
				ret = 0;
				goto End;
			}
			else if(recvSuccessSubNumber == tmpHead->perRoundNumber)
			{
				//8.��ʶÿ�����е��Ӱ��ɹ�����, �����жϸûظ�����Ӧ��(��ͼƬ�ɹ�����Ӧ���Ǳ��ֳɹ�����Ӧ��)
				recvSuccessSubNumber = 0;
				if(latePackFlag == false)
				{
					//���ֳɹ�����					
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);				
				}
				else if(latePackFlag == true)
				{
					//����ͼƬ���ɹ�����
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
			//9.��Ҫ�ط�			
			outDataLenth = sizeof(resCommonHead_t);
			assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);							
		}
		else
		{
			//10.����Ҫ�ط�
			ret = 0;
			goto End;
		}	
	}
	else
	{		
		//11.�ܰ�, ������Ӧ.
		outDataLenth = sizeof(resCommonHead_t);
		assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
	}

	//12.�����ڴ�
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}
	
	//13. ������ͷ����, ����У����
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t)); 		
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp = tmpSendBuf + head.contentLenth;
	memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

	//14. ת����������
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)	
	{
		myprint("Error : func escape() escape() !!!!");
		goto End;
	}
		
	//15. ��Ϸ��͵�����
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


/*�Ƚ����ݰ������Ƿ���ȷ
*@param : tmpContent		���ݰ�ת��������
*@param : tmpContentLenth	ת������ݵĳ���, ����: ��ͷ + ��Ϣ + У����
*@retval: success 0, repeat -1, NoRespond -2
*/
int compare_recvData_correct_server(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	int checkCode = 0;					//���ؼ���У����
	int *tmpCheckCode = NULL;			//�����������У����
	uint8_t cmd = 0;					//�������ݰ�����
	reqPackHead_t  *tmpHead = NULL;		//����ͷ
	reqTemplateErrHead_t  *Head = NULL;	//����ģ������ͷ


	//1.��ȡ�������ݰ���������
	cmd = *((uint8_t *)tmpContent);

	//2.���������ֵ�ѡ��
	if(cmd != DOWNFILEREQ)
	{
		//3. ��ȡ�������ݵı�ͷ��Ϣ
		tmpHead = (reqPackHead_t *)tmpContent;

		//4. ���ݰ����ȱȽ�
		if(tmpHead->contentLenth + sizeof(int) == tmpContentLenth)
		{
			//5.������ȷ, ������ˮ�űȽ�, ȥ���ذ�			
			if(tmpHead->seriaNumber >= g_recvSerial)
			{				
				//6.��ˮ����ȷ,����У����Ƚ�
				tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
				checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
				if(checkCode == *tmpCheckCode)
				{
					//7.У������ȷ, �������ݰ���ȷ
					g_recvSerial += 1;		//��ˮ�������ƶ�
					ret = 0;				
				}
				else
				{
					//8.У�������, ���ݰ���Ҫ�ط�
					myprint("Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
									cmd , tmpHead->seriaNumber, checkCode, *tmpCheckCode);
					ret = -1;
				}		
			}
			else
			{
				//9.��ˮ�ų���, ����Ҫ�ط�
				myprint( "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u", 
											cmd,  tmpHead->seriaNumber, g_recvSerial);
				ret = -2;		
			}
		}
		else
		{
			//10.���ݳ��ȳ���, ����Ҫ�ط�
			myprint( "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u", 
						cmd, tmpHead->contentLenth + sizeof(int), tmpContentLenth, tmpHead->seriaNumber);
			ret = -2;			
		}
	}
	else
	{
		Head = (reqTemplateErrHead_t *)tmpContent;

		//11.���ݰ����ȱȽ�
		if(Head->contentLenth + sizeof(int) == tmpContentLenth)
		{
			//12.���ݰ�������ȷ, �Ƚ���ˮ��, ȥ���ذ�
			if(g_recvSerial == Head->seriaNumber)
			{
				//13.��ˮ����ȷ, ����У����Ƚ�
				g_recvSerial += 1;		//��ˮ�������ƶ�
				//14.��ˮ����ȷ,����У����Ƚ�
				tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
				checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
				if(*tmpCheckCode == checkCode)
				{
					//15.У������ȷ, ������ݰ���ȷ
					ret = 0;
				}
				else
				{
					//16.У�������, ��Ҫ�ط����ݰ�						
					socket_log(SocketLevel[4], ret, "Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
									cmd , Head->seriaNumber, checkCode, *tmpCheckCode);
					ret = -1;
				}
			}
			else
			{
				//17.��ˮ�ų���, ����Ҫ�ط�
				socket_log(SocketLevel[3], ret, "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u", 
											cmd,  Head->seriaNumber, g_recvSerial);
				ret = -2;	
			}			
		}
		else
		{
			//18.���ݳ��ȳ���, ����Ҫ�ط�
			socket_log(SocketLevel[3], ret, "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u", 
						cmd, Head->contentLenth + sizeof(int), tmpContentLenth, Head->seriaNumber);
			ret = -2;
		}
	}

	return ret;
}




//�ϴ�ͼƬӦ�� ���ݰ�;  ��������һ�� �ظ�һ��.
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
int upload_func_reply_old0620(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0;					//�������ݵĳ���
	int  checkNum = 0;								//У����
	char tmpSendBuf[1500] = { 0 };					//��ʱ���ݻ�����
	char *sendBufStr = NULL;						//�������ݵ�ַ
	char *tmp = NULL;
	resCommonHead_t head;							//Ӧ�����ݱ�ͷ
	reqPackHead_t  *tmpHead = NULL;					//����ͷ��Ϣ
	static bool repeatFlag = false;					//true : �ط�, false δ�ط�,
	static bool needToReply = false;				//true : ��ҪӦ��, false ����ҪӦ��,  �ñ�ʶ����Ҫ���� : ��һ��50 �����ݰ����չ�����, �м����ݰ�����,
													//���һ����������ȫ��ȷ����, ��ʱ�޸ı�ʶ��λ true, ��ʶ���ɹ����յ������ش����ݰ�ʱ, Ӧ�����������ִ���Ļ�Ӧ.
	static int errPackNum = 0;						//�����ط����ݰ�������
	static int ind = 1;								//���Եľ�̬����, ��������ʱ,Ӧ�ñ�ɾ��
	static int recvSuccessSubNumber = 1;			//ÿ�ֳɹ����յ����Ӱ���Ŀ

	
	//1.��ӡ����ͼƬ����Ϣ
	//printf_comPack_news(recvBuf);					

	//3.��ȡ��ͷ��Ϣ
	tmpHead = (reqPackHead_t *)recvBuf;
	
	//4.����������Ϣ�����ж�
	if(tmpHead->isSubPack == 1)
	{
		//5.�Ӱ�, �������ݰ���Ϣ�ж�
		if((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
		{
			//6.������ȷ, ����ͼƬ���ݵĴ洢, ��¼���ֳɹ����յ��Ӱ�����
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
				//7.���������Ӱ�, ����Ҫ��Ӧ
				if(needToReply == true && errPackNum == 0)
				{
					//7.1 �������ݰ�ȫ���������, ���ʱ��Ҫ��Ӧ
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
				//8. ÿ�ֵ����һ������ȷ������Ҫ��Ӧ, ����Ҫ��ʾ ��UI
				if(repeatFlag == false)
				{
					recvSuccessSubNumber = 0;		//������Ӧ��, ���޸ı�ʶ��, ׼����һ�����ݵĽ���	
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);
				}
				else
				{
					//8.1 У��������� ���ݰ���δ��ȷ���յ�����, ���Բ��ܻ�Ӧ					
					goto End;
				}
			}
			else if(tmpHead->currentIndex + 1 == tmpHead->totalPackage)
			{
				//9.ͼƬ�����һ������, ��Ҫ��Ӧ, ��ʾ��UI 
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
			//9.��Ҫ�ط�
			repeatFlag = true;
			outDataLenth = sizeof(resCommonHead_t);
			assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);							
		}
		else
		{
			//10.����Ҫ�ط�
			ret = 0;
			goto End;
		}	
	}
	else
	{		
		//11.�ܰ�, ������Ӧ.
		myprint("isSubPack == 1, Number : %d", ind++);
		outDataLenth = sizeof(resCommonHead_t);
		assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
	}

	//2.�����ڴ�
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		assert(0);
	}

	//12. ������ͷ����, ����У����
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));			
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp = tmpSendBuf + head.contentLenth;
	memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

	//13. ת����������
	if((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)	
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		assert(0);
	}
	
	//14. ��Ϸ��͵�����
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



//�ϴ�ͼƬӦ�� ���ݰ�;  ��������һ�� �ظ�һ��.
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
int upload_func_reply_old(char *recvBuf, int recvLen, int index)
{
	int  ret = 0, outDataLenth = 0;					//�������ݵĳ���
	int  checkNum = 0;								//У����
	char tmpSendBuf[1500] = { 0 };					//��ʱ���ݻ�����
	char *sendBufStr = NULL;						//�������ݵ�ַ
	char *tmp = NULL;
	resCommonHead_t head;							//Ӧ�����ݱ�ͷ
	reqPackHead_t  *tmpHead = NULL;					//����ͷ��Ϣ
	static bool repeatFlag = false;					//true : �ط�, false δ�ط�,
	
	
	//1.��ӡ����ͼƬ����Ϣ
	//printf_comPack_news(recvBuf);					

	//3.��ȡ��ͷ��Ϣ
	tmpHead = (reqPackHead_t *)recvBuf;
	//4.����������Ϣ�����ж�
	if(tmpHead->isSubPack == 1)
	{
		//5.�Ӱ�, �������ݰ���Ϣ�ж�
		if((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
		{
			//6.������ȷ
#if 1
			if((ret = save_picture_func(recvBuf, recvLen)) < 0)			
			{
				printf("error: func save_picture_func() !!! [%d], [%s] \n", __LINE__, __FILE__);
				goto End;
			}
#endif
			//7. ���յ����һ���Ӱ�����, ��Ҫ���������������
			if(tmpHead->currentIndex + 1 == tmpHead->totalPackage)
			{			
				tmp = tmpSendBuf + sizeof(resCommonHead_t);
				*tmp++ = 0x01;	
				outDataLenth = sizeof(resCommonHead_t) + 1;
				assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 1);
			}
			else if((tmpHead->currentIndex + 1) % tmpHead->perRoundNumber == 0)
			{
				//8. ÿ�ֵ����һ�����ݰ���ȷ������Ҫ��Ӧ
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
			//9.��Ҫ�ط�
			repeatFlag = true;
			outDataLenth = sizeof(resCommonHead_t);
			assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);							
		}
		else
		{
			//10.����Ҫ�ط�
			ret = 0;
			goto End;
		}	
	}
	else
	{
		//11.�ܰ�, ������Ӧ.
		outDataLenth = sizeof(resCommonHead_t);
		assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
	}

	//2.�����ڴ�
	if((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
	{
		ret = -1;
		myprint("Err : func mem_pool_alloc()");
		goto End;
	}

	//12. ������ͷ����, ����У����
	memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));			
	checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
	tmp = tmpSendBuf + head.contentLenth;
	memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

	//13. ת����������
	ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
	if(ret)
	{
		printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	
	//14. ��Ϸ��͵�����
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




//��� �׽��ֻ�����������;
int removed_sockfd_buf(int sockfd)
{
	int ret = 0, len = 0;
	char buf[1024] = { 0 };

	//1.���÷�����
	activate_nonblock(sockfd);

	//2.��ȡ��ǰ�׽��ֻ��������ݳ���, ȫ����ȡ
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
	
	//3.��������
	deactivate_nonblock(sockfd);
	return ret;
}


/**
 * recv_peek - �����鿴�׽��ֻ��������ݣ������Ƴ�����
 * @sockfd: �׽���
 * @buf: ���ջ�����
 * @len: ����
 * �ɹ�����>=0��ʧ�ܷ���-1
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
 * deactivate_nonblock - ����I/OΪ����ģʽ
 * @fd: �ļ������
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
 * activate_noblock - ����I/OΪ������ģʽ
 * @fd: �ļ������
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
	
	//�����������׽���
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd == -1)
        std_err("socket");
    
    //�˿ڸ���            
   	ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if(ret)
		std_err("setsockopt");			   
                
    //�����ַ����
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
	ret = inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr);
  	if(ret != 1)
  		std_err("inet_pton");	
	
    //�󶨷�������IP���˿ڣ�
    ret = bind(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(ret == -1)
        std_err("bind");


    //�������ӷ������Ŀͻ�����
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





//����ͼƬ
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
int save_multiPicture_func(char *recvBuf, int recvLen)
{
	char *tmp = NULL;
	int lenth = 0, ret = 0;
	char TmpfileName[100] = { 0 };
	char fileName[100] = { 0 };
	char cmdBuf[1024] = { 0 };
	static FILE *fp = NULL;
	reqPackHead_t *tmpHead = NULL;
	int nWrite = 0;				//�Ѿ�д�����ݳ���
	int perWrite = 0; 			//ÿ��д������ݳ���

	
	//0. ��ȡ���ݰ���Ϣ
	tmpHead = (reqPackHead_t *)recvBuf;
	lenth = recvLen - sizeof(reqPackHead_t) - sizeof(int) - FILENAMELENTH - sizeof(char) * 2;
	tmp = recvBuf + sizeof(reqPackHead_t) + FILENAMELENTH + sizeof(char) * 2;
	//myprint("picDataLenth : %d, currentIndex : %d, totalNumber : %d, recvLen : %d ...", lenth, tmpHead->currentIndex, tmpHead->totalPackage, recvLen);
	
	if(tmpHead->currentIndex != 0 && tmpHead->currentIndex + 1 < tmpHead->totalPackage)
	{
		//1.��ͼƬ���м����ݰ�, �� ָ��ָ�� ͼƬ���ݵĲ���, ������д���ļ�
		
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
		//2.��ͼƬ�ĵ�һ������, ��ȡ�ļ�����
		memcpy(TmpfileName, recvBuf + sizeof(reqPackHead_t) + sizeof(char) * 2, 46);
		sprintf(fileName, "%s/%s", g_DirPath, TmpfileName );

		//3.�鿴�ļ��Ƿ����, ���ڼ�ɾ��	
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

		//4.�����ļ�, ���ļ���д������
		if((fp = fopen(fileName, "ab")) == NULL)
		{
			myprint("Err : func fopen() : fileName : %s !!!", fileName);
			ret = -1;
			goto End;
		}
		
		//5.�� ָ��ָ�� ͼƬ���ݵĲ���, ������д���ļ�
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
		//6.��ͼƬ�����һ������		
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







//����ͼƬ
//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
int save_picture_func(char *recvBuf, int recvLen)
{
	char *tmp = NULL;
	int lenth = 0, ret = 0;
	char TmpfileName[100] = { 0 };
	char fileName[100] = { 0 };
	char cmdBuf[1024] = { 0 };
	static FILE *fp = NULL;
	reqPackHead_t *tmpHead = NULL;
	int nWrite = 0;				//�Ѿ�д�����ݳ���
	int perWrite = 0; 			//ÿ��д������ݳ���

	
	//0. ��ȡ���ݰ���Ϣ
	tmpHead = (reqPackHead_t *)recvBuf;
	lenth = recvLen - sizeof(reqPackHead_t) - sizeof(int) - FILENAMELENTH;
	tmp = recvBuf + sizeof(reqPackHead_t) + FILENAMELENTH;
	//myprint("picDataLenth : %d, currentIndex : %d, totalNumber : %d, recvLen : %d ...", lenth, tmpHead->currentIndex, tmpHead->totalPackage, recvLen);
	
	if(tmpHead->currentIndex != 0 && tmpHead->currentIndex + 1 < tmpHead->totalPackage)
	{
		//1.��ͼƬ���м����ݰ�, �� ָ��ָ�� ͼƬ���ݵĲ���, ������д���ļ�
		
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
		//2.��ͼƬ�ĵ�һ������, ��ȡ�ļ�����
		memcpy(TmpfileName, recvBuf + sizeof(reqPackHead_t), 46);
		sprintf(fileName, "%s/%s", g_DirPath, TmpfileName );

		//3.�鿴�ļ��Ƿ����, ���ڼ�ɾ��	
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

		//4.�����ļ�, ���ļ���д������
		if((fp = fopen(fileName, "ab")) == NULL)
		{
			myprint("Err : func fopen() : fileName : %s !!!", fileName);
			ret = -1;
			goto End;
		}
		
		//5.�� ָ��ָ�� ͼƬ���ݵĲ���, ������д���ļ�
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
		//6.��ͼƬ�����һ������		
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

