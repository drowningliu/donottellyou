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


//�궨��
#define DIVISIONSIGN 			'~'
#define ACCOUTLENTH 			11							//�ͻ����˻�����
#define CAPACITY    			110							//����, ���������
#define WAIT_SECONDS 			2							//���ӷ������ĳ�ʱʱ��
#define HEART_TIME				300							//����ʱ��
#undef 	BUFSIZE
#define BUFSIZE					250							//������е� �ڵ��С
#define DOWNTEMPLATENAME		"NHi1200_TEMPLATE_NAME"		//The env for downtemplate PATHKEY
#define DOWNTABLENAME           "NHi1200_DATABASE_NAME"
#define DOWNLOADPATH			"NHi1200_TEMPLATE_DIR"
#define CERT_FILE 				"/home/yyx/nanhao/client/client-cert.pem" 
#define KEY_FILE   				"/home/yyx/nanhao/client/client-key.pem" 
#define CA_FILE  				"/home/yyx/nanhao/ca/ca-cert.pem" 
#define CIPHER_LIST	 			"AES128-SHA"
#define	MY_MIN(x, y)			((x) < (y) ? (x) : (y))	
#define TIMEOUTRETURNFLAG		-5							//��ʱ����ֵ


//�ڴ��, ����, ������Դ
mem_pool_t  				*g_memPool = NULL;				//�ڴ��
//mem_pool_t  				*g_memPool_list = NULL; 		//�ڴ��
queue       				*g_queueRecv = NULL;			//��UI�ظ����ݴ洢�Ķ���
queue_send_dataBLock 		g_queue_sendDataBlock = NULL;	//The sendData Addr and dataLenth �洢�������ݺͳ��ȵĶ���
fifo_type					*g_fifo = NULL;					//�洢�����߳� ���ݵĶ���
queue_buffer 				*g_que_buf = NULL;				//�洢����UI����Ķ���
LListSendInfo				*g_list_send_dataInfo = NULL;	//�������з��͵����ݰ���Ϣ(��ַ, ����, ������)

//�ź���, ����ͬ��
sem_t 			g_sem_cur_read;		 					//֪ͨ��UI �ظ���Ϣ���ź���
sem_t 			g_sem_cur_write;		 				//֪ͨ�����̵߳��ź���
sem_t 			g_sem_send;								//ͬ�������߳������̵߳��ź���
sem_t 			g_sem_read_open;						//֪ͨ�����߳̽������ݽ����ź���
sem_t 			g_sem_business;							//֪ͨ�����̵߳��ź���
sem_t 			g_sem_unpack;							//need between In read_thid And unpack_thid;��������ź���
sem_t 			g_sem_sequence;							//ÿ�η��ͱ������һ�������Ӧ��UI��, ����������һ�����յ�UI����



//������,��סȫ����Դ
pthread_mutex_t	 g_mutex_client = PTHREAD_MUTEX_INITIALIZER; 			
pthread_mutex_t	 g_mutex_memPool = PTHREAD_MUTEX_INITIALIZER; 			//�ڴ����(�ڴ��Ϊ ���͵���������)
pthread_mutex_t	 g_mutex_list = PTHREAD_MUTEX_INITIALIZER; 				//������
pthread_mutex_t  g_mutex_serial = PTHREAD_MUTEX_INITIALIZER;			//��ס�ͻ�����ˮ�ű仯

//ȫ�ֱ���
pthread_t  			g_thid_read = -1, g_thid_unpack = -1, g_thid_business = -1, g_thid_heartTime = -1, g_thid_time = -1,  g_thid_write = -1;	//net The thread ID 
int  				g_close_net = 0;					//0 connect server OK; 1 destroy connect sockfd; 2 active shutdown sockfd
int  				g_sockfd = 0;						//connet the server socket
char 				g_filePath[128] = { 0 };			//�����ļ�·��
bool 				g_err_flag = false;					//true : ����; false : δ����
pid_t 				g_dhcp_process = -1;				//DHCP ����
uploadWholeSet 		g_upload_set[10];					//�ϴ�����ͼƬ����
uploadWholeSet 		g_upload_error_set[10];				//�ϴ�����ͼƬ������Ϣ����
char 				*g_downLoadTemplateName = NULL, *g_downLoadTableName = NULL, *g_downLoadPath = NULL;//
int 				g_send_package_cmd = -1;			//current send package cmd
bool  				g_encrypt = false, g_encrypt_data_recv = false;				//true : encrypt Data transfer with sever; false : No encrypt transfer
char 				*g_machine = "21212111233";			//������
unsigned int 		g_seriaNumber = 0;					//�������ݰ������к�
unsigned int	 	g_recvSerial = 0;					//��ǰ���շ����������������ݰ������к�
SendDataBlockNews	g_lateSendPackNews;					//����͵����ݰ���Ϣ
bool 				g_perIsRecv = false;				//ÿ�ַ��͵����ݰ��Ƿ��Ѿ��л�Ӧ, false : �޻�Ӧ, ��ʱ��, �������ݰ�ȫ���ط�; true : �л�Ӧ, ��ʱ��,ֻ�ط����һ������  
char 				*g_sendFileContent = NULL;			//store The perRoundAll Src content will send to server	
bool 				g_residuePackNoNeedSend = false;	//ture : ��ʶʣ��Ĳ���Ҫ����, false : ����ʣ�����ݰ��ķ���, ֻ�� ����̺߳� ��ӦUI �߳�ʹ��
bool 				g_modify_config_file = false;		//true : have modify The config file. need set IP; false: not need set IP And don't modify The config file
bool 				g_template_uploadErr = false;		//true : ����Э���Դ���, ͼƬ���ݲ������ϴ�, false : ����
bool 				g_subResPonFlag = false;			//true : �������ݰ�,��Ӧ��ظ�(����Ӧ����ʲô), false : �������ݰ�, ���κ�Ӧ��
bool 				g_isSendPackCurrent = false;		//�����, �ж��Ƿ�ǰ�Ƿ����ڵȴ����յķ������ݰ��ظ�, true : �ڵȴ�; false : δ�ȴ�
#if 0
SSL_CTX 			*g_sslCtx = NULL;
SSL  				*g_sslHandle = NULL;
#endif

typedef void (*sighandler_t)(int);
//int ssl_init_handle(int socketfd, SSL_CTX **srcCtx, SSL **srcSsl);


//�źŲ�׽����
void pipe_func(int sig)
{
	myprint( "\nthread:%lu, SIGPIPIE appear %d", pthread_self(), sig);
}

//�źŲ�׽����
void sign_handler(int sign)
{	
	myprint(  "\nthread:%lu, receive signal:%u---", pthread_self(), sign);
	pthread_exit(NULL);
}

//�źŲ�׽����
void sign_usr2(int sign)
{
	myprint("\nthread:%lu, receive signal:%u---", pthread_self(), sign);
}

//�źŲ�׽����
void sign_child(int sign)
{
	while((waitpid(-1, NULL, WNOHANG)) > 0)
	{
		printf("receive signal : %u, [%d], [%s]",sign, __LINE__, __FILE__);
	}
		   
}

//����ص�����,����ָ����ַ�Ľڵ�
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


//��ʼ��; filePath : �����ļ�·��
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
	
	//1.�ź�ע��
    memset(&actions, 0, sizeof(actions));  
    sigemptyset(&actions.sa_mask); /* ������set�źż���ʼ������� */  
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
	//2 ɾ��LOGĿ¼�µ������ļ�
	if((if_file_exist("LOG")) == true)
	{
		sprintf(cmdBuf, "rm %s", "LOG/*");
		if(pox_system(cmdBuf) == 0)
		{
			myprint("Ok, rm LOG/* ...");	
		}
	}

	//2.��ʼ��ȫ�ֱ���
	if((ret = init_global_value()) < 0)
	{
		myprint( "Error: func pthread_create()");
		goto End;
	}
	//socket_log( SocketLevel[2], ret, "func init_global_value() OK");

	//3. ��ʼ���߳�
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



//����IP
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


//��ʼ��ȫ�ֱ���
static int init_global_value()
{
	int ret = 0;
	char *fileName = NULL;
	char *tableName = NULL;

	//1.����Ҫ�����ݵĵ�ַ�����ݳ���, ʹ���߳�: ����̺߳ͷ����߳�
	if((g_queue_sendDataBlock = queue_init_send_block(CAPACITY)) == NULL)
	{
		ret = -1;
		myprint( "Error: func queue_init_send_block()");
		goto End;
	}	
	//2.������������ݵ�ַ, ʹ���߳�: ����UI���̺߳ͽ���߳�
	if((g_queueRecv = queue_init(CAPACITY)) == NULL)		
	{
		ret = -1;
		myprint(  "Error: func queue_init()");
		goto End;
	}
	//3.�ڴ��, �ڴ��Ϊ ��Ҫ���͸������������ݻ�ӷ��������յ�����
	if((g_memPool = mem_pool_init(CAPACITY, PACKMAXLENTH * 2, &g_mutex_memPool)) == NULL)
	{
		ret = -1;
		myprint(  "Error: func mem_pool_init()");
		goto End;
	}
	//4.���շ��������ݵĶ���,  ʹ���߳�: �����̺߳ͽ���߳�
	if((g_fifo = fifo_init(CAPACITY * 1024 * 2)) == NULL)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: func fifo_init()");
		goto End;
	}	
	//5.����UI����Ķ���,     ʹ���߳�: ���̺߳�����߳�
	if((g_que_buf = queue_buffer_init(BUFSIZE, BUFSIZE)) == NULL)
	{
		ret = -1;
		myprint( "Error: func queue_buffer_init()");
		goto End;
	}
	//6.��������,���汾�����з������ݰ�����Ϣ
	if((g_list_send_dataInfo = Create_index_listSendInfo(PERROUNDNUMBER * 3, &g_mutex_list)) == NULL)
	{
		ret = -1;
		myprint( "Error: func Create_index_listSendInfo()");
		goto End;

	}
	//7.��ȡ�����ļ�������IP 
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
		
	//8.�ź�����ʼ��
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
	 
	 //1.���������߳�
	 if((ret = pthread_create(&g_thid_business, NULL, thid_business, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
 
	 //2.�������������߳�
	 if((ret = pthread_create(&g_thid_read, NULL, thid_server_func_read, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create() thid_server_func_read");
		 goto End;
	 }
 
	 //3.�������������߳�
	 if((ret = pthread_create(&g_thid_write, NULL, thid_server_func_write, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
	 //4.������ⳬʱ�߳�
	 if((ret = pthread_create(&g_thid_time, NULL, thid_server_func_time, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 } 
	 //5.��������߳�
	 if((ret = pthread_create(&g_thid_unpack, NULL, thid_unpack_thread, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
 
#if 0
	 //6.���������߳�
	 if((ret = pthread_create(&g_thid_heartTime, NULL, thid_hreat_beat_time, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
#endif
	 
 End:
 
	 return ret;
}


 //��ȡ�����ļ�����
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
	
	if(conten.dhcp)		//����ѡ���������, DHCP 
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
	else				//���ñ���IP
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



//����
int destroy_client_net()
{
	int ret = 0;

	//1.�޸ı�ʶ, ��ʶ���ٷ���
	g_close_net = 1;					
	sem_post(&g_sem_cur_read);			 
	sleep(1);

	//2.���ٽ��������߳�
	if((ret = pthread_kill(g_thid_read, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() recvData Thread");
	}

	//3.���ٷ��������߳�
	if((ret = pthread_kill(g_thid_write, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() sendData Thread");
	}

	//4.���ٳ�ʱ�߳�
	if((ret = pthread_kill(g_thid_time, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() timeOut Thread");
	}	

	//5.���ٴ��������߳�
	if((ret = pthread_kill(g_thid_business, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() cmd Thread");
	}

	//6.���ٽ���߳�
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

	//8.�����ڴ��
	if(g_memPool)					mem_pool_destroy(g_memPool);			

	//9.���ٶ���: ��������, ��������, �������, ���͸�UI
	if(g_queue_sendDataBlock)		destroy_queue_send_block(g_queue_sendDataBlock);	
	if(g_queueRecv)					destroy_queue(g_queueRecv);				
	if(g_fifo)						destroy_fifo_queue(g_fifo);								
	if(g_que_buf)					destroy_buffer_queue(g_que_buf);

	//10.����ȫ�ֱ���: ģ�����ƺ����ݿ�����
	if(g_downLoadTableName)			free(g_downLoadTableName);
	if(g_downLoadTemplateName)		free(g_downLoadTemplateName);
	
	//11.�����ź���
	sem_destroy(&g_sem_cur_read);
	sem_destroy(&g_sem_cur_write);
	sem_destroy(&g_sem_send);
	sem_destroy(&g_sem_read_open);
	sem_destroy(&g_sem_business);
	sem_destroy(&g_sem_sequence);
	sem_destroy(&g_sem_unpack);
	
	//12.���ٻ�����
	pthread_mutex_destroy(&g_mutex_client);
	pthread_mutex_destroy(&g_mutex_list);
	pthread_mutex_destroy(&g_mutex_memPool);
	pthread_mutex_destroy(&g_mutex_serial);
	

	//13.����ͨ���׽���
	if(g_sockfd > 0)
	{
		close(g_sockfd);
		g_sockfd = -1;
	}

	//14.����dhcp����
	if(g_dhcp_process > 0)
	{
		kill(g_dhcp_process, SIGKILL);
	}

	return ret;
}



//����UI ������
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

	//3.������洢��������
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

	//4.֪ͨ�����߳�, 
	if(ret == 0)
	{	
		//myprint("recv news : %s ", news);
		sem_post(&g_sem_business);		
	}
	
End:
	return ret;
}

//������Ϣ��UI
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

	//1.�����������
	while(1)
	{
		sem_wait(&g_sem_cur_read);	
		if(g_close_net == 0 || g_close_net == 2)
		{
			//2.�����������,����ȡ���͸� UI ������
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
		else if(g_close_net == 1)		//�����������
		{
			sprintf(g_request_reply_content, "%s", " ");				//������Ϣ, ���
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



//��UI �ظ�ʱ�����ݴ���; 0: ��Ҫ�ظ�;  -1: ����;  1: ����Ҫ��UI���лظ�
int data_process(char **buf)
{
	int ret = 0;
	char *sendUiData = NULL;				//��ȡ�����е�����	
	uint8_t cmd = 0;						//���ݰ�������
	
	
	//1.ȡ������������  ��ͷ+��Ϣ��+У����
	if((ret = pop_queue(g_queueRecv, &sendUiData)) < 0)		
	{
		myprint("Error: func pop_queue()");
 		assert(0);
	}

	//2.��ȡ������
	cmd = *((uint8_t *)sendUiData);

	//3.���������ֵ�ѡ��
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

	//4. �ͷ��ڴ�
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


//�ͷ��ڴ�
int free_memory_net(char *buf)
{
	int ret = 0;
	
	if(buf)
	{
		//sscanf(buf, "%[^~]", tmpBuf);
		if(g_close_net != 1)		//�ͷ��ڴ���ڴ��
		{
			if((ret = mem_pool_free(g_memPool, buf)) < 0)
			{
				ret = -1;
				myprint("Error: func  mem_pool_alloc() ");		
				assert(0);
			}		
		}
	#if 0	
		else			//���ȫ�ֱ�����Ϣ
		{
			memset(g_request_reply_content, 0, REQUESTCONTENTLENTH);
		}
	#endif	
	}



	return ret;
}



/*copy The news content in block buffer
*@param : news cmd~content~;¢~
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

	//�����������ݰ��ķ���
	while(1)
	{
		timer_select(HEART_TIME);

		if(!g_isSendPackCurrent)		//�������ݰ�����ʱ, �������ݰ��ķ���
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




//���� UI ������߳�
void *thid_business(void *arg)
{
	int ret = 0, cmd = -1;
	char tmpBuf[4] = { 0 };				//��ȡ����
	int numberCmd = 1;					//�������������
	
	pthread_detach(pthread_self());
	
	socket_log( SocketLevel[3], ret, "func thid_business() begin");

	while(1)
	{
		sem_wait(&g_sem_business);			//�ȴ��ź���֪ͨ	
		sem_wait(&g_sem_sequence);			//������������,����һ���������,���ؽ����UI��, �ٴ�����һ������

		char 	cmdBuffer[BUFSIZE] = { 0 };	//ȡ�������е�����
		int		num = 0;		
				
		//1.ȡ�� UI ������Ϣ
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
		
		//2.��ȡ����
		sscanf(cmdBuffer, "%[^~]", tmpBuf);
		cmd = atoi(tmpBuf);		
		
		do 
		{				
			//3.��������ѡ��
			switch (cmd){
				case 0x01:		//��¼		
					if((ret = data_packge(login_func, cmdBuffer)) < 0)			
						socket_log(SocketLevel[4], ret, "Error: data_packge() login_func"); 									
					break;				
				case 0x02:		//�ǳ�
					if((ret = data_packge(exit_program, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() exit_program");					
					break;
				case 0x03:		//����
					if((ret = data_packge(heart_beat_program, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() heart_beat_program");
					break;					
				case 0x04:		//�ӷ����������ļ�
					if((ret = data_packge(download_file_fromServer, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() download_template");
					break;
				case 0x05:		//��ȡ�ļ�����ID 
					if((ret = data_packge(get_FileNewestID, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() get_FileNewestID");
					break;	
				
				case 0x06:		//�ϴ�ͼƬ
					if((ret = data_packge(upload_picture, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() upload_picture");
					break;
#if 1					
				case 0x07:		//��Ϣ����
					if((ret = data_packge(push_info_from_server_reply, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() upload_picture");
					break;
				case 0x08:		//�����ر�����
					if((ret = data_packge(active_shutdown_net, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() active_shutdown_net");
					break;
				case 0x09:		//ɾ��ͼƬ
					if((ret = data_packge(delete_spec_pic, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() delete_spec_pic");
					break;
#endif					
				case 0x0A:		//���ӷ�����
					if((ret = data_packge(connet_server, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() connet_server");
					break;

				case 0x0B:		//��ȡ�����ļ�
					if((ret = data_packge(read_config_file, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() read_config_file");
					break;
					
				case 0x0C:		//ģ��ɨ�������ϴ�
					if((ret = data_packge(template_extend_element, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() template_extend_element");
					break;
#if 0					
				case 0x0E:		//���ܴ��� ���� ȡ�����ܴ���
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
				sleep(1);		//����������, 
				num += 1;
			}
		}while( 0 );
//		myprint("cmd : %d end...", cmd);	
		
	}

	socket_log( SocketLevel[3], ret, "func thid_business() end");

	return	NULL;
}

//���к���ע��
int  data_packge(call_back_package  func, const char *news)
{
	return func(news);
}


/*��������
 *@param: fd : ָ���׽���
 *@param: buf : ��������
 *@param: lenth: ���ݳ���
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
            else	//��������һ�����
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
	char 	*tmpSend = NULL;		//�������ݵĵ�ַ
	int 	tmpSendLenth = 0;		//�������ݳ���
	int 	nSendLen = 0;			//�����˳���

	
	//1.��ȡ���͵ĵ�ַ�ͳ���
	if(( ret = pop_queue_send_block(g_queue_sendDataBlock, &tmpSend, &tmpSendLenth, sendCmd)) < 0)
	{
		myprint( "Error: func get_queue_send_block()");
		assert(0);
	}
	g_send_package_cmd = *sendCmd;

	//2.�жϲ�ȡ����ͨ����������
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
			//3.���ͳɹ�, ���浱ǰ���͵����ݰ���Ϣ, (��Ҫ�������һ��)
			g_lateSendPackNews.cmd = *sendCmd;
			g_lateSendPackNews.sendAddr = tmpSend;
			g_lateSendPackNews.sendLenth = tmpSendLenth;
			time(&(g_lateSendPackNews.sendTime));	

			if(g_send_package_cmd == PUSHINFOREQ)
			{
				remove_senddataserver_addr();				
				sem_post(&g_sem_send);	//������������Ϣ�ɹ�Ӧ���,֪ͨ������һ�����ݵķ��� 
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

//ɾ�����������е�Ԫ��
void remove_senddataserver_addr()
{
	
	trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
	if((clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
	{			
		myprint("Error: func clear_LinkListSendInfo() ");
		assert(0);
	}

}


//��������; success 0;  fail -1;
int recv_data(int sockfd)
{
	int ret = 0, recvLenth = 0, number = 0;
	char buf[PACKMAXLENTH * 2] = { 0 };
	static int nRead = 0;
	
	while(1)
	{

		//1.ѡ��������ݵķ�ʽ
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
			//2.��������ʧ��
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
			//3.�������ѹر�
			ret = -1;
			myprint("The server has closed !!!");			
			goto End;
		}
		else if(recvLenth > 0)		
		{
			//4.�������ݳɹ�
			do{
				//5.�����ݷ��������
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

			//6.�ͷ��ź���, ֪ͨ����߳�
			sem_post(&g_sem_unpack);
		
			goto End;				
		}
	}
	
End:
	return ret;
}




/*�Ƚ����ݰ������Ƿ���ȷ
*@param : tmpContent		���ݰ�ת��������
*@param : tmpContentLenth	ת������ݵĳ���, ����: ��ͷ + ��Ϣ + У����
*@retval: success 0, repeat : -1, Noreapt : -2
*/
int compare_recvData_Correct(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	int checkCode = 0;					//���ؼ���У����
	int *tmpCheckCode = NULL;			//�����������У����
	uint8_t cmd = 0;					//�������ݰ�����
	resCommonHead_t  *comHead = NULL;	//��ͨӦ��ͷ
	resSubPackHead_t  *subHead = NULL;	//���ط������ļ��ı�ͷ


	//1.��ȡ�������ݰ���������
	cmd = *((uint8_t *)tmpContent);

	//2.���������ֵ�ѡ��
	if(cmd != DOWNFILEZRESPON)
	{
		//3. ��ȡ�������ݵı�ͷ��Ϣ
		comHead = (resCommonHead_t *)tmpContent;

		//4. ���ݰ����ȱȽ�
		if(comHead->contentLenth + sizeof(int) == tmpContentLenth)
		{
			if(cmd == LOGINCMDRESPON)	
			{
				g_recvSerial = comHead->seriaNumber;  //��һ�ν�������, ��ȡ����������ˮ�Ŵ���
			}	

			//5.������ȷ, ������ˮ�űȽ�, ȥ���ذ�			
			if(comHead->seriaNumber >= g_recvSerial)
			{				
				//6.��ˮ����ȷ,����У����Ƚ�
				tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));			
				checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
				if(checkCode == *tmpCheckCode)
				{
					//7.У������ȷ, �������ݰ���ȷ
					g_recvSerial = comHead->seriaNumber + 1;		//��ˮ�������ƶ�					
					ret = 0;				
				}
				else
				{
					//8.У�������, ���ݰ���Ҫ�ط�
					myprint( "Error: cmd : %d, contentLenth : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
									cmd , comHead->contentLenth, comHead->seriaNumber, checkCode, *tmpCheckCode);
					ret = -1;
				}		
			}
			else
			{
				//9.��ˮ�ų���, ����Ҫ�ط�
				socket_log(SocketLevel[3], ret, "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u", 
											cmd,  comHead->seriaNumber, g_recvSerial);
				ret = -2;		
			}
		}
		else
		{
			//10.���ݳ��ȳ���, ����Ҫ�ط�
			socket_log(SocketLevel[3], ret, "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u", 
						cmd, comHead->contentLenth + sizeof(int), tmpContentLenth, comHead->seriaNumber);
			ret = -2;			
		}
	}
	else	//�ӷ����������ļ�
	{
		subHead = (resSubPackHead_t *)tmpContent;

		//11.���ݰ����ȱȽ�
		if(subHead->contentLenth + sizeof(int) == tmpContentLenth)
		{
			//12.���ݰ�������ȷ, �Ƚ���ˮ��, ȥ���ذ�
			if(g_recvSerial <= subHead->seriaNumber)
			{
								
				//14.��ˮ����ȷ,����У����Ƚ�
				tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
				checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
				if(*tmpCheckCode == checkCode)
				{
					//15.У������ȷ, ������ݰ���ȷ
					g_recvSerial = subHead->seriaNumber + 1;		//��ˮ�������ƶ�					
					ret = 0;
				}
				else
				{
					//16.У�������, ��Ҫ�ط����ݰ�						
					myprint( "Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
									cmd , subHead->seriaNumber, checkCode, *tmpCheckCode);
					ret = -1;
				}
			}
			else
			{
				//17.��ˮ�ų���, ����Ҫ�ط�
				socket_log(SocketLevel[3], ret, "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u", 
											cmd,  subHead->seriaNumber, g_seriaNumber);
				ret = -2;	
			}			
		}
		else
		{
			//18.���ݳ��ȳ���, ����Ҫ�ط�
			socket_log(SocketLevel[3], ret, "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u", 
						cmd, subHead->contentLenth + sizeof(int), tmpContentLenth, subHead->seriaNumber);
			ret = -2;
		}
	}


	return ret;
}


/*�����·��͵����ݰ��޸�������ˮ��
*@param : sendAddr  	 ���������ݰ�ԭʼ��ַ
*@param : sendLenth  	 ���ݰ�����
*@param : cmd			 �������ݰ���������
*/
void modify_repeatPackSerialNews(char *sendAddr, int sendLenth, int *newSendLenth)
{
	char *tmp = NULL;
	char tmpPackData[PACKMAXLENTH] = { 0 };		//�������ݰ�������
	int outLenth = 0;
	reqPackHead_t *tmpHead;
	int  checkCode = 0;				//У����
	int	 outDataLenth = 0;

	//1. ��ת�����ݰ�����
	if((anti_escape(sendAddr + 1, sendLenth - 2, tmpPackData, &outLenth)) < 0)
	{
		myprint("Error : func anti_escape()");
		assert(0);
	} 
	
	//2.��ȡ���ݰ��ı�ͷ��Ϣ, �޸����е���ˮ��
	tmpHead = (reqPackHead_t*)tmpPackData;
	pthread_mutex_lock(&g_mutex_serial);
	tmpHead->seriaNumber = g_seriaNumber++;
	pthread_mutex_unlock(&g_mutex_serial);

	//3.���¼���У����
	checkCode = crc326((const char *)tmpPackData, tmpHead->contentLenth);
	tmp = tmpPackData + tmpHead->contentLenth;
	memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

	//4.�����ݰ�ת��
	if((escape(PACKSIGN, tmpPackData, tmpHead->contentLenth + sizeof(uint32_t), sendAddr + 1, &outDataLenth)) < 0)
	{
		myprint("Error: func escape() ");		 
		assert(0);
	}
	*(sendAddr + outDataLenth + 1) = PACKSIGN;

	//5.�����µĳ��ȷ���
	*newSendLenth = outDataLenth + sizeof(char) * 2;	
	
}


/*��ͨ����� Ӧ�������,(�˴�ָ: �ͻ��˵�һ������ ֻ��Ӧ ��������һ��Ӧ���)
*@param : tmpContent		���ݰ�ת��������
*@param : tmpContentLenth	ת������ݵĳ���, ����: ��ͷ + ��Ϣ + У����
*@retval: success 0, 
*/
int commom_respcommand_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	resCommonHead_t  *comHead = NULL;	//��ͨӦ��ͷ
	char *tmpPool = NULL;				//�ڴ��
	
	//1.��ȡӦ��ı�ͷ��Ϣ
	comHead = (resCommonHead_t*)tmpContent;

	//2.�ж����ݰ��Ƿ��͹�����, ��Ϣ����
	if(comHead->isFailPack == 1)
	{
		//3.���͹�����, ��Ϣ����, ָУ������Ϣ, �ط������ݰ�, �൱�����һ��
		repeat_latePack();		
	}
	else
	{
		//5.�����ڴ�
		if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
		{
			ret = -1;
			myprint( "Error: func mem_pool_alloc() ");
			assert(0);
		}
		memcpy(tmpPool, tmpContent, tmpContentLenth);
		
		//6.�����ݴ���� ����,֪ͨ��UI�ظ�
		if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
		{
			myprint( "Error: func push_queue() ");
			mem_pool_free(g_memPool, tmpPool);
			assert(0);
		}
		sem_post(&g_sem_cur_read);

		//7.���������ݴ����������(��Ϊ��������, ���еĿͻ������ֻ��Ӧһ��Ӧ���, ����ֱ���������Ϳ���)
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{			
			myprint("Error: func clear_LinkListSendInfo() ");
			assert(0);
		}

		//8.�޸ı�ʶ��, ��ʶ���յ������ݰ���Ӧ��
		g_isSendPackCurrent = false;
		
		//9.֪ͨ����߳̽�����һ������
		sem_post(&g_sem_send);
	}


	return ret;
}

int pushInfo_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	//resCommonHead_t  *comHead = NULL;	//��ͨӦ��ͷ
	char *tmpPool = NULL;				//�ڴ��

	//1.�����ڴ�
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}
	memcpy(tmpPool, tmpContent, tmpContentLenth);
	
	//2.�����ݴ���� ����,֪ͨ��UI�ظ�
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
	char *tmpResult = NULL;				//��ȡ���ݰ���Э�鷵�ؽ��	
	resCommonHead_t  head;				//Ӧ��ͷ
	int contentLenth = 0;	
	char *tmp = tmpPool;
	
	//1. �鿴�Ƿ�Ϊ�����Э�����(�ڴ���, ����ԭ���, �����ݰ����紫�����)
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

	//2. ��װ��ͷ	
	assign_resComPack_head(&head, TEMPLATECMDRESPON, contentLenth, 0, 1);
	memcpy(tmpPool, &head, sizeof(resCommonHead_t));
	
	
}

/*�����ϴ�ͼƬ, ��������Ӧ�������
*@param : tmpContent		���ݰ�ת��������
*@param : tmpContentLenth	ת������ݵĳ���, ����: ��ͷ + ��Ϣ + У����
*@retval: success 0, 
*/
int mutiUpload_respon_func(char *tmpContent,int tmpContentLenth)
{
	int ret = 0;
	resCommonHead_t  *comHead = NULL;	//��ͨӦ��ͷ
	node *repeatPackage = NULL;			//�����ط����ݰ�����Ϣ
	char *tmpPool = NULL;				//�ڴ�����ݿ�	
	int  newSendLenth = 0; 				//�޸���ˮ�ź����µ����ݰ�����

	//1.��ȡӦ��ı�ͷ��Ϣ
	comHead = (resCommonHead_t*)tmpContent;

	//2.�ж��Ƿ�Ϊ�Ӱ�Ӧ��
	if(comHead->isSubPack == 1)
	{		
		g_subResPonFlag = true;			//�޸ı�ʶ��, ��ʶ������Ӧ����� 
		//3.�Ӱ�Ӧ��, �ж��Ӱ��Ƿ�Ϊ�������϶�ʧ��, ��Ҫ�ط������ݰ�
		if(comHead->isFailPack == 1)
		{
			//4.ʧ��,��Ҫ�ط�ָ����������ݰ�, ȥ������ȡ����ָ�����ݰ�
			if((repeatPackage = FindNode_ByVal_listSendInfo(g_list_send_dataInfo, comHead->failPackIndex, my_compare_packIndex)) == NULL)
			{
				//5.δ�ҵ���Ҫ���·��͵����ݰ����				
				socket_log(SocketLevel[4], ret, "Error: func FindNode_ByVal_listSendInfo() ");
				ret = 0;		//��֪���˴�����η���, ��Ҫ����
			}
			else
			{
				//6.�ҵ���Ҫ���·������ݰ����, ���޸���ˮ����Ϣ				
				modify_repeatPackSerialNews(repeatPackage->sendAddr, repeatPackage->sendLenth, &newSendLenth);
				repeatPackage->sendLenth = newSendLenth;				
				if((ret = push_queue_send_block(g_queue_sendDataBlock, repeatPackage->sendAddr, repeatPackage->sendLenth, UPLOADCMDREQ)) < 0)
				{
					//7.��Ϣ�������ʧ��					
					myprint("Error: func push_queue_send_block(), package News : cmd : %d, lenth : %d, sendAdd : %p ", 
							UPLOADCMDREQ, repeatPackage->sendLenth, repeatPackage->sendAddr);
					assert(0);
				}
				else
				{
					//8.�ɹ����뷢�Ͷ���, �ͷ��ź���, ֪ͨ�����߳̽��з���
					sem_post(&g_sem_cur_write);
				}
			}
		}
		else
		{				
			//9.�Ӱ����ճɹ�,�ж��Ƿ���Ҫ��UI��Ӧ, ���������: ��ȷ����,���ǳ�����Э���ж����(�ڴ�����������������, ��Ҫ��Ӧ��UI ) 
			if(comHead->isRespondClient == 1)
			{
				//10. ��Ҫ��UI��Ӧ, �����ڴ�,������Ϣ, ����Ϣ�����Ӧ����, �ͷ��ź��� 
				if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
				{
					myprint("Err : func mem_pool_alloc() to resPond UI");					
					assert(0);
				}

				//10.1 ������װӦ�𷵻ظ�UI
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
				//11.�޸ı�ʶ��, ��ʶ���յ������ݰ���Ӧ��
				g_isSendPackCurrent = false;				
			}
							
			//12.�����з������ݴ����������,
			trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
			if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
			{
				myprint("Error : func clear_LinkListSendInfo()");
				assert(0);
			}
			g_subResPonFlag = false;				//�޸ı�ʶ������ʼ��״̬, ����Ӧ�����, ��Ϊ��ʱ���������޷���Ԫ��

			//13. ֪ͨ����߳�ȥ������һ�ֵ�����������߸��������
			sem_post(&g_sem_send);
		}
	}
	else
	{		
		//10.�ܰ�Ӧ��, ��Ҫ������Ϣ, ֱ��֪ͨ����߳�ȥ������һ�ֵ��������
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


/*�ϴ�ͼƬ, ��������Ӧ�������
*@param : tmpContent		���ݰ�ת��������
*@param : tmpContentLenth	ת������ݵĳ���, ����: ��ͷ + ��Ϣ + У����
*@retval: success 0, 
*/
int upload_respon_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	resCommonHead_t  *comHead = NULL;	//��ͨӦ��ͷ
	node *repeatPackage = NULL;			//�����ط����ݰ�����Ϣ
	char *tmpPool = NULL;				//�ڴ�����ݿ�
	char *tmpResult = NULL;				//��ȡ���ݰ���Э�鷵�ؽ��
	int  newSendLenth = 0; 				//�޸���ˮ�ź����µ����ݰ�����

	
	//1.��ȡӦ��ı�ͷ��Ϣ
	comHead = (resCommonHead_t*)tmpContent;

	//2.�ж��Ƿ�Ϊ�Ӱ�Ӧ��
	if(comHead->isSubPack == 1)
	{
		g_subResPonFlag = true;			//�޸ı�ʶ��, ��ʶ������Ӧ�����
		//3.�Ӱ�Ӧ��, �ж��Ӱ��Ƿ�Ϊ�������϶�ʧ��, ��Ҫ�ط������ݰ�
		if(comHead->isFailPack == 1)
		{
			//4.ʧ��,��Ҫ�ط�ָ����������ݰ�, ȥ������ȡ����ָ�����ݰ�
			if((repeatPackage = FindNode_ByVal_listSendInfo(g_list_send_dataInfo, comHead->failPackIndex, my_compare_packIndex)) == NULL)
			{
				//5.δ�ҵ���Ҫ���·��͵����ݰ����				
				socket_log(SocketLevel[4], ret, "Error: func FindNode_ByVal_listSendInfo() ");
				ret = 0;		//��֪���˴�����η���, ��Ҫ����
			}
			else
			{
				//6.�ҵ���Ҫ���·������ݰ����, ���޸���ˮ����Ϣ			
				modify_repeatPackSerialNews(repeatPackage->sendAddr, repeatPackage->sendLenth, &newSendLenth);
				repeatPackage->sendLenth = newSendLenth;				
				if((ret = push_queue_send_block(g_queue_sendDataBlock, repeatPackage->sendAddr, repeatPackage->sendLenth, UPLOADCMDREQ)) < 0)
				{
					//7.��Ϣ�������ʧ��					
					myprint("Error: func push_queue_send_block(), package News : cmd : %d, lenth : %d, sendAdd : %p ", 
							UPLOADCMDREQ, repeatPackage->sendLenth, repeatPackage->sendAddr);
					assert(0);
				}
				else
				{
					//8.�ɹ����뷢�Ͷ���, �ͷ��ź���, ֪ͨ�����߳̽��з���
					sem_post(&g_sem_cur_write);
				}
			}
		}
		else
		{				
			//9.�Ӱ����ճɹ�,�ж��Ƿ���Ҫ��UI��Ӧ, ���������: ��ȷ����,���ǳ�����Э���ж����(�ڴ�����������������, ��Ҫ��Ӧ��UI ) 
			if(comHead->isRespondClient == 1)
			{
				//10. ��Ҫ��UI��Ӧ, �����ڴ�,������Ϣ, ����Ϣ�����Ӧ����, �ͷ��ź��� 
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

				//11.�鿴�Ƿ�Ϊ�����Э�����(�ڴ���, ����ԭ���, �����ݰ����紫�����)
				tmpResult = tmpContent + sizeof(resCommonHead_t);
				if(*tmpResult == 0x02)
				{
					g_residuePackNoNeedSend = true;
				}
				//11.�޸ı�ʶ��, ��ʶ���յ������ݰ���Ӧ��
				g_isSendPackCurrent = false;
			
			}

			//12.�����з������ݴ����������,
			trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
			if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
			{
				myprint("Error : func clear_LinkListSendInfo()");
				assert(0);
			}
			g_subResPonFlag = false;				//�޸ı�ʶ������ʼ��״̬, ����Ӧ�����, ��Ϊ��ʱ���������޷���Ԫ��

			//13. ֪ͨ����߳�ȥ������һ�ֵ�����������߸��������
			sem_post(&g_sem_send);
		}
	}
	else
	{
		//10.�ܰ�Ӧ��, ��Ҫ������Ϣ, ֱ��֪ͨ����߳�ȥ������һ�ֵ��������
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

/*���ط������ļ�, ��Ӧ�������
*@param : tmpContent		���ݰ�ת��������
*@param : tmpContentLenth	ת������ݵĳ���, ����: ��ͷ + ��Ϣ + У����
*@retval: success 0, 
*/
int downFile_respon_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	resSubPackHead_t  *comHead = NULL;	//���ط������ļ��ı�ͷ	
	char *tmpPool = NULL;				//�ڴ��
	
	//0. ��ȡ�������ݰ���Ϣ
	comHead = (resSubPackHead_t *)tmpContent;
		

	//1.�����ڴ�
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}
	memcpy(tmpPool, tmpContent, tmpContentLenth);
	
	//2.�����ݴ���� ����,֪ͨ��UI�ظ�
	if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
	{
		myprint( "Error: func push_queue() ");
		mem_pool_free(g_memPool, tmpPool);
		assert(0);
	}
	sem_post(&g_sem_cur_read);

	//3. �ж��Ƿ�ɹ����յ����һ��������
	if(comHead->currentIndex + 1 == comHead->totalPackage)
	{	
		//4.���������ݴ����������(��Ϊ��������, ���еĿͻ������ֻ��Ӧһ��Ӧ���, ����ֱ���������Ϳ���)
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{			
			myprint("Error: func clear_LinkListSendInfo() ");
			assert(0);
		}
		//5.֪ͨ����߳̽�����һ������
		sem_post(&g_sem_send);
	}		

	return ret;
}

/*�ϴ�ģ������ ��Ӧ�������
*@param : tmpContent		���ݰ�ת��������
*@param : tmpContentLenth	ת������ݵĳ���, ����: ��ͷ + ��Ϣ + У����
*@retval: success 0, 
*/
int template_respon_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	resCommonHead_t  *comHead = NULL;	//��ͨӦ��ͷ
	char *tmpPool = NULL;				//�ڴ��
	int  newSendLenth = 0;				//�޸���ˮ�ź����µ����ݰ�����
	char *tmp = NULL;
	
	//1.��ȡӦ��ı�ͷ��Ϣ, ��ģ�������ϴ��Ľ��
	comHead = (resCommonHead_t*)tmpContent;
	tmp = tmpContent + sizeof(resCommonHead_t);

	//2.�ж����ݰ��Ƿ��͹�����, ��Ϣ����
	if(comHead->isFailPack == 1)
	{
		//3.���͹�����, ��Ϣ����, ָУ������Ϣ
		modify_repeatPackSerialNews(g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, &newSendLenth);
		g_lateSendPackNews.sendLenth = newSendLenth;
		if((push_queue_send_block(g_queue_sendDataBlock, g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, g_lateSendPackNews.cmd)) < 0)
		{
			myprint("Error : func push_queue_send_block()");
			assert(0);
		}
		//4.�ͷ��ź���,֪ͨ���������̼߳�������
		sem_post(&g_sem_cur_write);
	}
	else
	{
		if(*tmp == 0)
		{
			//ģ�������ϴ��ɹ�, ��ʱ����Ҫ�� UI������Ϣ, �ȴ�ͼƬ�ϴ���ɺ�, ��һ�𷵻�
			sem_post(&g_sem_send);
			return ret;
		}
		//5.�ϴ�����Э���Դ���, ��Ҫ��UI ����
		if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
		{
			myprint( "Error: func mem_pool_alloc() ");
			assert(0);
		}
		memcpy(tmpPool, tmpContent, tmpContentLenth);
		
		//6.�����ݴ���� ����,֪ͨ��UI�ظ�
		if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
		{
			myprint( "Error: func push_queue() ");
			assert(0);
		}
		sem_post(&g_sem_cur_read);
	
		//7.���������ݴ����������(��Ϊ��������, ���еĿͻ������ֻ��Ӧһ��Ӧ���, ����ֱ���������Ϳ���)
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{			
			myprint("Error: func clear_LinkListSendInfo() ");
			assert(0);
		}
		//8.֪ͨ����߳�Э���Դ���, �����ٴ���ͼƬ����, 
		g_template_uploadErr = true;			
		sem_post(&g_sem_send);

	}

	return ret;
}

/*�Է�������Ӧ�����ݰ�, ����ѡ����
*@param : tmpContent		���ݰ�ת��������
*@param : tmpContentLenth	ת������ݵĳ���, ����: ��ͷ + ��Ϣ + У����
*@retval: success 0, 
*/
int deal_with_pack(char *tmpContent, int tmpContentLenth )
{
	int ret = 0;
	uint8_t cmd = 0;					//�������ݰ�����


	//1.��ȡ�������ݰ���������
	cmd = *((uint8_t *)tmpContent);

	//2.�����ظ�����
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
	static int flag = 0, lenth = 0; 	//flag ��ʶ�ҵ���ʶ��������;  lenth : ���ݵĳ���
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
					//5. ���ݰ���ȷ,�������ݰ�
					//socket_log(SocketLevel[2], ret, "------ func compare_recvData_Correct() OK ------");

					if((deal_with_pack(tmpContent, tmpContentLenth )) < 0)
					{
						myprint("Error: func deal_with_pack()");
						assert(0);
					}

					//6.�����ʱ����
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
					//���յ����ݰ�У�������, ��Ҫ���������·�������
					myprint("Error: func compare_recvData_Correct()");					
					//7. clear the variable
					memset(content, 0, sizeof(content));
					memset(tmpContent, 0, sizeof(tmpContent));					
					recvLenth -= (lenth + 2);
					lenth = 0;
					flag = 0;
					tmp = content + lenth;

					//8.������������, �޸���ˮ��, �������з�������				
					trave_LinkListSendInfo(g_list_send_dataInfo, modify_listNodeNews);						
  					ret = 0;			//�˴����޸�, ����, ����Ҫ  2017.04.21
					break;
				}
				else if(ret == -2)
				{
					//socket_log(SocketLevel[2], ret, "compare_recvData_Correct() ret : %d", ret);
					//9.���յ����ݰ� ���Ⱥ���ˮ�Ų���ȷ, ����Ҫ���·�������
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


//��ʱ�߳�
void *thid_server_func_time(void *arg)
{
	time_t  nowTime = 0, timeDiffer = 0;		//ʱ��
	int waitNumber = 0;							//��ʱ��, ��ͬ���ݰ��ط�����

	//1.�������߳�
	pthread_detach(pthread_self());

	//2.������ѵ���
	while(1)
	{		
		//3.��ʱ���
		timer_select(OUTTIMEPACK + 3);
		//4.����ͻ��˳���, ������ѯ
		if(g_err_flag)
		{
			waitNumber = 0;
			continue;	
		}
		//5.�Է��������Ԫ�����������ж�
		if((Size_LinkListSendInfo(g_list_send_dataInfo)) == 0)
		{
			waitNumber = 0;
			//������������Ԫ��, ������ѯ,
			continue;	
		}
		else if((Size_LinkListSendInfo(g_list_send_dataInfo)) > 1)
		{	
			//�����������Ԫ��			
			time(&nowTime);
			timeDiffer = nowTime - g_lateSendPackNews.sendTime;

			//6.�鿴�Ƿ�ʱ
			if(timeDiffer < OUTTIMEPACK)
			{
				//δ��ʱ
				continue;
			}
			else
			{
				if(waitNumber < 2)
				{
					myprint("respond timeOut : %d > time : %d, repeatNumber : %d", timeDiffer, OUTTIMEPACK, waitNumber);
					//��ʱ, �жϱ������ݰ��Ƿ�������Ӧ��
					if(g_subResPonFlag)
					{	
						//������Ӧ��, ֻ�ط����һ������
						repeat_latePack();
					}	
					else
					{
						//û���κ�Ӧ��, �������е����ݰ�ȫ���ط�
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
			//������������Ԫ��
			time(&nowTime);
			timeDiffer = nowTime - g_lateSendPackNews.sendTime;

			//7.�鿴�Ƿ�ʱ
			if(timeDiffer < OUTTIMEPACK)
			{
				//δ��ʱ
				continue;
			}
			else
			{
				if(waitNumber < 2)
				{
					myprint("respond timeOut : %d > time : %d, repeatNumber : %d", timeDiffer, OUTTIMEPACK, waitNumber);
					//��ʱ
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

	//1.��Ҫ�ط����һ������, �޸�������ˮ��
	modify_repeatPackSerialNews(g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, &newSendLenth);

	//2.���¸�ֵ���ݰ�����, ���������
	g_lateSendPackNews.sendLenth = newSendLenth;
	if((push_queue_send_block(g_queue_sendDataBlock, g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, g_lateSendPackNews.cmd)) < 0)
	{
		myprint("Error : func push_queue_send_block()");
		assert(0);
	}
	
	//3.�ͷ��ź���,֪ͨ���������̼߳�������
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


//������Ϣ������, �����������
int send_error_netPack(int sendCmd)
{
	int ret = 0;
	int cmd = 0, err = 0, errNum = 0;	//�������ݰ���������, �����ʶ��, ����ԭ���ʶ��


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
	//2.�������ݰ���ָ��������Ϣ�� UI
	if((ret = copy_news_to_mem(cmd, err, errNum)) < 0)
	{		
		myprint("Error: func copy_news_to_mem()");
		assert(0);
	}


	//socket_log(SocketLevel[3], ret, "*****end: send_error_netPack() sendCmd : %d", sendCmd);

	return ret;
}



/*�������ݰ���ָ��������Ϣ�� UI
 *@param: cmd : ���ݰ�����
 *@param: err: �����ʶ
 *@param: errNum: ����ԭ���ʶ
*/
int copy_news_to_mem(int cmd, int err, int errNum)
{
	int ret = 0;
	resCommonHead_t head;
	char *tmp = NULL, *tmpPool = NULL;

	//1.�����ڴ�
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}

	//2.��������
	tmp = tmpPool;
	memset(&head, 0, sizeof(resCommonHead_t));
	head.cmd = cmd;
	head.contentLenth = sizeof(resCommonHead_t) + 2;
	memcpy(tmp, &head, sizeof(resCommonHead_t));
	tmp += sizeof(resCommonHead_t);
	*tmp++ = err;
	*tmp++ = errNum;
	

	//3.����������ݷ������, ֪ͨ���߳̽��д���, ���͸�UI
	if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
	{
		mem_pool_free(g_memPool, tmpPool);
		myprint("Error: func push_queue()");
		assert(0);
	}
	
	sem_post(&g_sem_cur_read);


	return ret;
}



/*��ն���, �ź����е�����; �ú���ֻ���ڷ���������� �� ��ʱ�̼߳�⵽������ʱ�ŵ���
 *@param: flag : 0 �����߳��������; 1 �����߳��������; TIMEOUTRETURNFLAG : ��ʱ�߳��������
 *@param: 20170531  ��Ϊ�ú��� �����̲߳���Ҫ���е���, ��ΪֻҪ�������, �����߳�һ�����Խ���
 *@param:			������, ����ֻ��Ҫ����һ�μ���;
*/
void clear_global(int flag)
{
	int ret = 0;
	int size = 0, i = 0;
	node rmNode;
	
	//socket_log(SocketLevel[3], ret, "func clear_global() begin flag : %d ", flag);

	//1.�޸�ȫ�ֱ�־, ��ʶ����
	g_err_flag = true;


	//2.��� ���շ����������ݶ���
	if((ret = clear_fifo(g_fifo)) < 0)								
	{
		myprint("Error: func clear_fifo()");		
		assert(0);
	}

	//3.��� �����������������Ķ���
	while (get_element_count_send_block(g_queue_sendDataBlock)) 	//5. ���Ͷ���: ���͵�ַ�ͳ���
	{
		if((ret = pop_queue_send_block(g_queue_sendDataBlock, NULL, NULL, NULL)) < 0) //6. ��ն���
		{
			myprint("Error: func pop_queue_send_block()");
			assert(0);
		}
	}

	//4.��� ���淢����Ϣ������
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


	//5.ѡ�����ĸ����ܵ��õĸú���, �����ź����Ĵ���
	if(flag == 1)				//�����̵߳��øú���
	{
		sem_init(&g_sem_read_open, 0, 0);		//�����߳��ź���, ��ʼ��������
	}
#if 0	
	if(flag == 1)								//�����̵߳��øú���
	{		
		sem_post(&g_sem_send);					//Ŀ��: ���ճ����,֪ͨ��������̲߳�Ҫ������. 	
		sem_init(&g_sem_read_open, 0, 0);		//�����߳��ź���, ��ʼ��������
		//sem_init(&g_sem_send, 0, 0);			//��ֹ����Ϣ�շ�ʱ, ����Ͽ�, ���ź���ƾ����1; �˴�Ӧע��, ��Ϊ�߳�����ʱ���Ǿ���ģʽ, �ᵼ����һ���̻߳�û�н��յ����ź���, �ֱ�����̳߳�ʼ����
	}
	else										//��ʱ�߳�
	{
		sem_post(&g_sem_send);					//Ŀ��: ���ճ����,֪ͨ��������̲߳�Ҫ������. 	
	}
#endif		
	//socket_log(SocketLevel[3], ret, "func clear_global() end flag : %d ", flag);

	
	return ;
}



/*net ����������, �������, ����0�����ݰ�
 *@param : flag  1 �����̳߳���
*/
int error_net(int flag)
{
	int ret = 0;
	char *sendData = NULL;
	resCommonHead_t  head;
	
	//socket_log(SocketLevel[2], ret, "debug: func error_net()  begin");

	//1.����ָ�����ݰ�����, g_isSendPackCurrent = true, ��ʶ��ǰ���ڵȴ��������ݰ���Ӧ��,��ʱ����,Ӧ�÷��ظ����ݰ��Ĵ����0�����ݰ�
	if(g_isSendPackCurrent)
	{
		send_error_netPack(g_send_package_cmd);
		sem_post(&g_sem_send);					//Ŀ��: ���ճ����,֪ͨ��������̲߳�Ҫ������. 	
		g_isSendPackCurrent = false;
	}
	//2.��ն���, ����
	clear_global(flag);

	//3.���ݰ����Ͷ�κ�, δ�յ��ظ�, ��ʱ
	if(flag == TIMEOUTRETURNFLAG)
	{
		return ret;
	}
	
	//4.�����ݰ�ͷ
	assign_resComPack_head(&head, ERRORFLAGMY, sizeof(resCommonHead_t), 1, 1 );
	
	//5.�����ڴ�, �����ݿ������ڴ�
	if((sendData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error : func mem_pool_alloc()");
		assert(0);
	}
	memcpy(sendData, &head, sizeof(resCommonHead_t));

	//6.������з�������
	if((ret = push_queue(g_queueRecv, sendData)) < 0)
	{
		myprint("Error : func push_queue()");
		mem_pool_free(g_memPool, sendData);		
		assert(0);
	}		

	//7.���ý��շ��������ݰ���ˮ��, �������ź�֪ͨ�����߳�
	g_recvSerial = 0;
	sem_post(&g_sem_cur_read);
	
	
	//socket_log(SocketLevel[2], ret, "debug: func error_net()  end");
	return ret;
}


//���������߳�
void *thid_server_func_read(void *arg)
{
	int ret = 0;
	int num = 0;
	
	//1.�����߳�
	pthread_detach(pthread_self());

	while(1)
	{
		//2.�ȴ��ź���֪ͨ, �������Ϸ�����, ���Խ������ݽ���
		sem_wait(&g_sem_read_open);			

		while(1)
		{
			//3.��������
			if((ret = recv_data(g_sockfd)) < 0)			
			{		
				//4.�������ݳ���
				if(g_sockfd > 0)
				{
					close(g_sockfd);
					g_sockfd = 0;
				}

				//5.�����رջ���������, ����Ҫ���г���ظ�
				if(g_close_net == 1 || g_close_net == 2)	
				{
					clear_global(1);
					myprint( "The UI active shutdown NET in read thread");				
					break;
				}
				else if(g_close_net == 0)		
				{	
					
					//6.���г���ظ�				
					do{
						if((ret = error_net(1)) < 0)		//��������, ��UI �˻ظ�
						{
							myprint(  "Error: func error_net()");
							sleep(1);	
							num++;			//�������з���
						}
						else				//���ͳɹ�,
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


//�����̷߳�������
void *thid_server_func_write(void *arg)
{
	int ret = 0;
	int sendCmd = -1;		//�������ݰ���������
	
	//socket_log( SocketLevel[3], ret, "func thid_server_func_write() begin");

	//1.�����߳�
	pthread_detach(pthread_self());

	//2.�����������
	while(1)
	{
		//3.�ȴ��ź���֪ͨ, �������ݷ���
		sem_wait(&g_sem_cur_write);		
		//pthread_mutex_lock(&g_mutex_client);

		//4.�������ݷ���
		if((ret = send_data(g_sockfd, &sendCmd)) < 0)		
		{
			//����ʧ��
			//socket_log( SocketLevel[3], ret, "---------- func send_data() err -------");
							
			//�׽�����Ҫ�ر�, �ͷŻ�����
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

	pthread_exit(NULL);		//�߳��˳�
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

//��¼���ݰ�  1~123213123~qgdyuwgqu455~;   
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

	//1.���ҷָ��
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
	memcpy(passWd, tmp + 1, strlen(tmp)-2);				//��ȡ����
	passWdSize = strlen(passWd);

	//2.�����ڴ�
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

	//3.�������ݰ��ܳ���, ����װ��ͷ
	contenLenth = sizeof(reqPackHead_t) + ACCOUTLENTH + sizeof(char) + passWdSize;
	assign_reqPack_head(&head, LOGINCMDREQ, contenLenth, NOSUBPACK, 0, 1, 1);

	//4.��װ���ݰ���Ϣ
	tmp = tmpSendPackDataOne;
	memcpy(tmp, (char *)&head, sizeof(reqPackHead_t));
	tmp += sizeof(reqPackHead_t);
	memcpy(tmp, accout, ACCOUTLENTH);
	tmp += ACCOUTLENTH;
	*tmp++ = passWdSize;
	memcpy(tmp, passWd, passWdSize);

	//5.����У����
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	tmp += passWdSize;
	memcpy(tmp, &checkCode, sizeof(int));

	//6.ת�����ݰ�����
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData + 1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;

	//7.�ͷ��ڴ����ڴ��
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

	sem_post(&g_sem_cur_write);	//�����ź���, ֪ͨ�����߳�
	sem_wait(&g_sem_send);

	//socket_log(SocketLevel[2], ret, "func login_func() end: [%s] %p", news, sendPackData);

	
	return ret;
}

//�ǳ�
int  exit_program(const char* news)
{
	int ret = 0;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0;
	int outDataLenth = 0;

	//1.�鱨ͷ
	assign_reqPack_head(&head, LOGOUTCMDREQ, sizeof(reqPackHead_t), NOSUBPACK, 0, 1, 1);

	//2.�����ڴ�
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

	//3.����У����
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const  char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne + sizeof(reqPackHead_t), (char *)&checkCode, sizeof(uint32_t));

	//4.ת����������
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//5.�ͷ���ʱ�ڴ�, �������͵�ַ�����ݳ��ȷ������
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

	sem_post(&g_sem_cur_write);	//�����ź���, ֪ͨ�����߳�
	sem_wait(&g_sem_send);


	return ret;
}


//����
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

	//3.����У����
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne + head.contentLenth, (char *)&checkCode, sizeof(uint32_t));
	
	//4.ת����������	
	*sendPackData = PACKSIGN; 
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//5.�ͷ���ʱ�ڴ�, �������͵�ַ�����ݳ��ȷ������
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

//����ģ���ļ�, cmd~type~ID~
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

//ÿ�����ݿ�ʼ����֮ǰ ���ܰ�(��Я����������)
int send_perRounc_begin(int cmd, int totalPacka,int sendPackNumber)
{
	 reqPackHead_t  head;								//������Ϣ�ı�ͷ
	 char *sendPackData = NULL;							//�������ݰ���ַ
	 char tmpSendPackDataOne[PACKMAXLENTH] = { 0 };		//��ʱ���ݰ���ַ����
	 int checkCode = 0;									//У����
	 char *tmp = NULL;
	 int ret = 0, outDataLenth = 0;
	 
	 //1.��װ��ͷ
	 assign_reqPack_head(&head, cmd, sizeof(reqPackHead_t), NOSUBPACK, 0, totalPacka, sendPackNumber);
			
	 //2. �����ڴ�, ���淢�����ݰ�
	 if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	 {			 
	 	 myprint("Error : func mem_pool_alloc()");
		 assert(0);
 	 }

	 //3.��������У����,
	 memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
	 checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
	 tmp = tmpSendPackDataOne + head.contentLenth;
	 memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

	 //4. ת�����ݰ�����
	 *sendPackData = PACKSIGN;							 //flag 
	 if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	 {
		 myprint("Error : func escape()");
		 assert(0);
	 }
	 *(sendPackData + outDataLenth + 1) = PACKSIGN; 	 //flag 

	 //5. ���������ݰ��������, ֪ͨ�����߳�
	 if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, cmd)) < 0)
	 {
		 myprint("Error : func push_queue_send_block()");
		 assert(0);
	 }

	 //6. ���������ݷ�������			 
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
	int ret = 0, index = 0, i = 0;			//index : ��ǰ�ļ��������
	uint16_t  tmpTotalPack = 0, totalPacka = 0;				//�����ļ�����Ҫ���ܰ���
	char *tmp = NULL;
	char filepath[250] = { 0 };				//�����ļ��ľ���·��
	char fileName[100] = { 0 };				//���͵��ļ���
	int  checkCode = 0;						//У����	
	int doucumentLenth = 0 ;				//ÿ���������ļ��������ݵ�����ֽ���
	int lenth = 0, packContenLenth = 0;		//lenth : ��ȡ�ļ����ݵĳ���, packContenLenth : ���ݰ���ԭʼ����
	char *sendPackData = NULL;				//�������ݰ��ĵ�ַ
	char *tmpSendPackDataOne = NULL;		//��ʱ�������ĵ�ַ
	int outDataLenth = 0;					//ÿ�����ݰ��ķ��ͳ���
	FILE *fp = NULL;						//�򿪵��ļ����
	reqPackHead_t  head;					//������Ϣ�ı�ͷ
	int   sendPackNumber = 0;				//ÿ�ַ������ݵİ���
	int   nRead = 0;						//�Ѿ������������ܳ���
	int   copyLenth = 0;					//ÿ�ο��������ݳ���
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
		//7.����ÿ�ַ��͵��ܰ���
		if(tmpTotalPack > PERROUNDNUMBER)			sendPackNumber = PERROUNDNUMBER;
		else										sendPackNumber = tmpTotalPack;
	
		//8.��ȡÿ�ַ����ļ�����������
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

		//9.�Ա��ֵ����ݽ������
		for(i = 0; i < sendPackNumber; i++)
		{
			//10.�����ļ�����
			 tmp = tmpSendPackDataOne + sizeof(reqPackHead_t);
			 memcpy(tmp, fileName, FILENAMELENTH);
			 tmp += FILENAMELENTH;
			 
			 //11.���ļ�����
			 copyLenth = MY_MIN(lenth, doucumentLenth);		
			 memcpy(tmp, g_sendFileContent + nRead , copyLenth);
			 nRead += copyLenth;
			 lenth -= copyLenth;
			
			 //12. �������ݰ�ԭʼ����, ����ʼ����ͷ
			 packContenLenth = sizeof(reqPackHead_t) + FILENAMELENTH + copyLenth;
			 assign_reqPack_head(&head, UPLOADCMDREQ, packContenLenth, SUBPACK, index, totalPacka, sendPackNumber);
			 
			 //13. �����ڴ�, ���淢�����ݰ�
			 if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			 {			 
				 myprint("Error : func mem_pool_alloc()");
				 assert(0);
		 	 }

			 //14.��������У����,
			 memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
			 checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
			 tmp = tmpSendPackDataOne + head.contentLenth;
			 memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

			 //15. ת�����ݰ�����
			 *sendPackData = PACKSIGN;							 //flag 
			 if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
			 {
				 myprint("Error : func escape()");
				 assert(0);
			 }
			 *(sendPackData + outDataLenth + 1) = PACKSIGN; 	 //flag 

			 //16. ���������ݰ��������, ֪ͨ�����߳�
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

		//17.һ�����ݷ������, ����ȫ�����ݵļ���
		nRead = 0;
		tmpTotalPack -= sendPackNumber;
		lenth = 0;

		//18.�ȴ��ź�֪ͨ, ������Ƿ���Ҫ��һ�ֵ����
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

	//19. �����Դ, �ͷ���ʱ�ڴ���ļ�������
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



//�����ر��������, �������ݰ�			
int  active_shutdown_net()
{
	int  ret = 0;
	resCommonHead_t head;				//Ӧ��ͷ��Ϣ
	char *sendData = NULL;

	sem_init(&g_sem_read_open, 0, 0);			//��ʼ�������ź���
	g_close_net = 2;							//�޸ı�ʶ��, �����ر�
	g_err_flag = true;							//�޸ı�ʶ��, ����

	//1.�ر��׽���
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


//ɾ��ͼƬ  cmd~ͼƬ·��~
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

	//4.����У����
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const  char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne+head.contentLenth, (char *)&checkCode, sizeof(uint32_t));
	//myprint("delete pic contentLenth : %d", head.contentLenth);

	
	//5.ת��
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;
	
	//6.�ͷ���ʱ�������ڴ��, �������ݰ����뷢�Ͷ��к�����
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



// ���ӷ�����, �������ݰ�			
int  connet_server(const char *news)
{	
	int  ret = 0, contentLenth = 0;		//�������ݳ���
	char *sendData = NULL;				//���͵�ַ
	resCommonHead_t head;				//Ӧ��ͷ��Ϣ

	//1. ���ӷ�����
	if(g_sockfd <= 0)
	{		
		if((ret = client_con_server(&g_sockfd, WAIT_SECONDS)) < 0)			
		{
			//2. ���ӷ�����ʧ��
			g_sockfd = -1;
			myprint("Error: func client_con_server()");
		}
		else if(ret == 0)			
		{
			//3. ���ӳɹ�, ��������ͨ�����
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
				//4. ��ʼ��ȫ�ֱ���
				sem_post(&g_sem_read_open); 	//֪ͨ���������߳�
				g_err_flag = false; 			//�����ȫ�ֱ����޸�
				g_close_net = 0;				//��ʼ��
			}			
		}
		
	}

	//5. �����ڴ�
	if((sendData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}

	//6. ��װ���ݰ���Ϣ�ͱ�ͷ����
	if(ret == 0)		*(sendData + sizeof(resCommonHead_t)) = ret;
	else				*(sendData + sizeof(resCommonHead_t)) = 1;
	contentLenth = sizeof(resCommonHead_t) + 1;	

	//7.�����ͷ��Ϣ, ������
	assign_resComPack_head(&head, CONNECTCMDRESPON, contentLenth, 0, 0);
	memcpy(sendData, &head, sizeof(resCommonHead_t));

	//8. ����Ϣ�������
	if((ret = push_queue(g_queueRecv, sendData)) < 0)
	{
		myprint("Error: func push_queue() no have data");
		mem_pool_free(g_memPool, sendData); 	
		assert(0);
	}

	//9.�ͷ��ź���, ֪ͨ���͸�UI���ݵ��߳�
	sem_post(&g_sem_cur_read);				

	return ret;
}


//��ȡ�����ļ�, �������ݰ�																
int read_config_file(const char *news)
{
	
	int  ret = 0;
	resCommonHead_t head;				//Ӧ��ͷ��Ϣ
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



/*ģ�����ݰ���չ
*@param : news   cmd~data~;
*@retval: success 0; fail -1.
*/
int template_extend_element(const char *news)
{
	int ret = 0, i = 0;
	char *tmp = NULL;
	reqPackHead_t head;							//��ͷ
	char *tmpSendPackDataOne = NULL;				//��ʱ������
	char *sendPackData = NULL;						//���͵�ַ
	int  checkCode = 0, contentLenth = 0;			//У��������ݳ���
	int outDataLenth = 0;							//���͵�ַ����						
	char bufAddr[20] = { 0 };						//ģ�����ݿ��ַ				
	unsigned int apprLen = 0; 							
	UitoNetExFrame *tmpExFrame = NULL;					
						

	//1.ȥ���ָ��, ��ȡ���ݵ�ַ
	if((tmp = strchr(news, DIVISIONSIGN)) == NULL)
	{
		myprint( "Error: func strstr() No find");
		assert(0);
	}		
	memcpy(bufAddr, tmp + 1, strlen(tmp) - 2);	
	apprLen = hex_conver_dec(bufAddr);	
	tmp = NULL + apprLen;
	tmpExFrame = (UitoNetExFrame*)tmp;

	//2.��װ��ͷ
	contentLenth = sizeof(reqPackHead_t) + sizeof(unsigned char) * 4 + strlen(tmpExFrame->sendData.identifyID) + strlen(tmpExFrame->sendData.objectAnswer) + tmpExFrame->sendData.subMapCoordNum * sizeof(Coords) + tmpExFrame->sendData.totalNumberPaper * sizeof(tmpExFrame->sendData.toalPaperName[0]);
	assign_reqPack_head(&head, TEMPLATECMDREQ, contentLenth, NOSUBPACK, 0, 1, 1);

	//3.�����ڴ��
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
	
	//5.�������
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
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);	//����У����
	tmp = tmpSendPackDataOne + head.contentLenth;
	memcpy(tmp, &checkCode, sizeof(uint32_t));	

	//6.ת������
	*sendPackData = PACKSIGN; 
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData + 1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;

	//7.�ͷ���ʱ�ڴ�
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

	//8. ����ɨ�����ݿ�ͼƬ��Ϣ
	memset(g_upload_set, 0, sizeof(g_upload_set));
	for(i = 0; i < tmpExFrame->sendData.totalNumberPaper; i++)
	{
		sprintf(g_upload_set[i].filePath, "%s/%s", tmpExFrame->fileDir, tmpExFrame->sendData.toalPaperName[i]);
		g_upload_set[i].totalNumber = tmpExFrame->sendData.totalNumberPaper;
		g_upload_set[i].indexNumber = i;
	}
	sem_post(&g_sem_cur_write);	//֪ͨ�߳̽��з���
	sem_wait(&g_sem_send);
	
	if(g_template_uploadErr)
	{
		g_template_uploadErr = false;
		goto End;
	}

	//9. ���Ͷ���ͼƬ
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



/*�ϴ�����ͼƬ
*@param : uploadSet  ͼƬ���ݼ���
*@retval: success 0; fail -1;
*/
int upload_template_set(uploadWholeSet *uploadSet)
{
	int ret = 0, i = 0;
	int contentLenth = 0;		
	
	contentLenth = COMREQDATABODYLENTH - FILENAMELENTH - sizeof( char) * 2;  
	
	for(i = 0; i < uploadSet->totalNumber; i++)
	{
		//��ͼƬ�������,
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


/*ͼƬ�������,�����ϴ�
*@param : filePath  ͼƬ·��
*@param : readLen   ���ݰ���Ϣ�ĳ���(��ȥ��ͷ, У����, ��ʶ��)
*@retval: success 0; fail -1; -2 ���յ���������ź�,��������ͼƬ���������ݰ�
*/
int image_data_package(const char *filePath, int readLen, int fileNumber, int totalFileNumber)
{
	int ret = 0, index = 0, i = 0;			//index : ��ǰ�ļ��������
	int  tmpTotalPack = 0, totalPacka = 0;	//�����ļ�����Ҫ���ܰ���
	char *tmp = NULL;
	char filepath[250] = { 0 };				//�����ļ��ľ���·��
	char fileName[100] = { 0 };				//���͵��ļ���
	int  checkCode = 0;						//У����	
	int doucumentLenth = 0 ;				//ÿ���������ļ��������ݵ�����ֽ���
	int lenth = 0, packContenLenth = 0;		//lenth : ��ȡ�ļ����ݵĳ���, packContenLenth : ���ݰ���ԭʼ����
	char *sendPackData = NULL;				//�������ݰ��ĵ�ַ
	char *tmpSendPackDataOne = NULL;		//��ʱ�������ĵ�ַ
	int outDataLenth = 0;					//ÿ�����ݰ��ķ��ͳ���
	FILE *fp = NULL;						//�򿪵��ļ����
	reqPackHead_t  head;					//������Ϣ�ı�ͷ
	int   sendPackNumber = 0;				//ÿ�ַ������ݵİ���
	int   nRead = 0;						//�Ѿ������������ܳ���
	int   copyLenth = 0;					//ÿ�ο��������ݳ���
	int  tmpLenth = 0;


	//1.��ȡ�ļ������
	if((ret = get_file_size(filePath, &totalPacka, readLen, NULL)) < 0)
	{
		myprint("Error: func get_file_size() filePath : %s", filePath);
		assert(0);
	}
	doucumentLenth = readLen;
	tmpTotalPack = totalPacka;
	
	//2.�����ļ�����
	if((ret = find_file_name(fileName, sizeof(fileName), filePath)) < 0)
	{
		myprint( "Error: func find_file_name()");
		assert(0);
	}

	//3.���ļ�
	if((fp = fopen(filePath, "rb")) == NULL)
	{
		myprint("Error: func fopen(), %s", strerror(errno));
		assert(0);
	}

	//4.�����ڴ�
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}

	//6. operation The file content and package
	while(!feof(fp))
	{
		//7.����ÿ�ַ��͵��ܰ���
		if(tmpTotalPack > PERROUNDNUMBER)			sendPackNumber = PERROUNDNUMBER;
		else										sendPackNumber = tmpTotalPack;
	
		//8.��ȡÿ�ַ����ļ�����������
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
		//9.�Ա��ֵ����ݽ������
		for(i = 0; i < sendPackNumber; i++)
		{
			 //10.�����ļ�����
			 tmp = tmpSendPackDataOne + sizeof(reqPackHead_t);
			 *tmp++ = fileNumber;
			 *tmp++ = totalFileNumber;
			 memcpy(tmp, fileName, FILENAMELENTH);
			 tmp += FILENAMELENTH;
			 
			 //11.���ļ�����
			 copyLenth = MY_MIN(lenth, doucumentLenth); 	
			 memcpy(tmp, g_sendFileContent + nRead , copyLenth);
			 nRead += copyLenth;
			 lenth -= copyLenth;

			 //12. �������ݰ�ԭʼ����, ����ʼ����ͷ
			 packContenLenth = sizeof(reqPackHead_t) + FILENAMELENTH + copyLenth + sizeof(char) * 2;
			 assign_reqPack_head(&head, MUTIUPLOADCMDREQ, packContenLenth, SUBPACK, index, totalPacka, sendPackNumber);
			 
			 //13. �����ڴ�, ���淢�����ݰ�
			 if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			 {			 
				 myprint("Error : func mem_pool_alloc()");
				 assert(0);
			 }

			 //14.��������У����,
			memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
			checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
			tmp = tmpSendPackDataOne + head.contentLenth;
			memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

			//15. ת�����ݰ�����
			*sendPackData = PACKSIGN;							//flag 
			if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
			{
				myprint("Error : func escape()");
				assert(0);
			}
			*(sendPackData + outDataLenth + 1) = PACKSIGN;		//flag 
		
			//16. ���������ݰ��������, ֪ͨ�����߳�
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
			
		//17.һ�����ݷ������, ����ȫ�����ݵļ���
		nRead = 0;
		tmpTotalPack -= sendPackNumber;
		lenth = 0;

		//18.�ȴ��ź�֪ͨ, ������Ƿ���Ҫ��һ�ֵ����
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
	//6.�ͷ���ʱ�ڴ�
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



//�������ݻظ�  0�����ݰ�
int error_net_reply(char *news, char **outDataToUi)
{
	int ret = 0;
	char *outDataNews = NULL;


	//1.�����ڴ�, ��������, ���ظ�UI
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func  mem_pool_alloc() ");		
		assert(0);
	}

	//2.�����ݿ������ڴ� 
	sprintf(outDataNews, "%d~", 0 );
	*outDataToUi = outDataNews;

	return ret;
}


/*��¼��Ϣ�ظ�
*/
int login_reply_func(char *news, char **outDataToUi)
{
	int ret = 0;
	char *tmp = NULL, *tmpOut = NULL;		//��ʱָ��
	resCommonHead_t *head = NULL;			//���ݱ�ͷ
	char *outDataNews = NULL;				//�ڴ��

	//1.�����ڴ�, ��������, ���ظ�UI
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func  mem_pool_alloc() ");		
		assert(0);
	}

	//2.�������ݿ���
	head = (resCommonHead_t *)news;
	sprintf(outDataNews, "%d~", LOGINCMDRESPON);

	//3.�ƶ�ָ��, ָ�����ݿ�, ��ȡ���ݿ鳤��
	tmp = (char *)news + sizeof(resCommonHead_t);
	tmpOut = outDataNews + strlen(outDataNews);
	if(*tmp == 0x01)
	{			
		//��¼�ɹ�
		sprintf(tmpOut, "%d~", *tmp);			
		tmp += 2;						//���Ƴ���
		tmpOut += 2;					//ָ�������ƶ�
		memcpy(tmpOut, tmp + 1, *tmp);	//��������
		tmpOut += *tmp;
		*tmpOut = '~';
	}
	else
	{
		//��¼ʧ��
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

	//6.���и�ֵ
	*outDataToUi = outDataNews;

	return ret;
}



//�ǳ���Ϣ�ظ�
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



//�������ݻظ�
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
	resSubPackHead_t *tmpHead = NULL;				//Ӧ��ͷ
	static bool openFlag = false;					//true : open The file; false : no open The file
	int  nWrite = 0, fileLenth = 0, tmpLenth = 0;

	//1. ��ȡ��ͷ��Ϣ, ���ƶ�ָ��
	tmpHead = (resSubPackHead_t *)news;
	tmp = news + sizeof(resSubPackHead_t);

	//2. �ж�����ģ���Ƿ�ɹ�
	if(*tmp == 0)		
	{		
		//3.���سɹ�, д�ļ�, open The file;  64 fileName lenth
		if(!openFlag)
		{
			tmp = news + sizeof(resSubPackHead_t) + sizeof(char) * 2;

			//4.��������ѡ��Ҫ�洢�ļ�������
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

			//5.���ļ�
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
			//8. ����Ҫ�� UI ���лظ�
			//myprint("func download_reply_template() continue index : %d, ret : %d,	indexPackge + 1 : %d, totalPackge : %d", 
			//	index++, ret, tmpHead->packgeNews.indexPackge + 1, tmpHead->packgeNews.totalPackge);
			ret = 1;
		}
		else
		{
			//9. ��ģ�����һ�����ݳɹ�����, ��Ҫ�� UI ���лظ�
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

			//9.�޸ı�ʶ��, ��ʶ���յ������ݰ���Ӧ��
			g_isSendPackCurrent = false;
			
			//socket_log(SocketLevel[2], ret, "func download_reply_template() end fclose() index : %d, ret : %d", index++, ret);
		}
	}
	else
	{
		//10.�����ļ�ʧ��	
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

		//11.�޸ı�ʶ��, ��ʶ���յ������ݰ���Ӧ��
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
	if(*tmp == 0)					sprintf(outDataNews, "%d~%d~%lld~", NEWESCMDRESPON, 0, fileID);				//1.组命令字
	else if(*tmp == 1)				sprintf(outDataNews, "%d~%d~%lld~", NEWESCMDRESPON, 1, fileID );				//1.组命令字
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
	char *tmp = NULL, *tmpOut = NULL;		//��ʱָ��
	char *outDataNews = NULL;				//�ڴ��

	//1.�����ڴ�, ��������, ���ظ�UI
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func  mem_pool_alloc() ");		
		assert(0);
	}
	
	//2.�������ݿ���
	sprintf(outDataNews, "%d~", UPLOADCMDRESPON);				
	tmpOut = outDataNews + strlen(outDataNews);			
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );

	//3.�ж��Ƿ����
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
	if(*tmp == 0)					sprintf(outDataNews, "%d~%d~%lld~", PUSHINFORESPON, 0, fileID);				//1.组命令字
	else if(*tmp == 1)				sprintf(outDataNews, "%d~%d~%lld~", PUSHINFORESPON, 1, fileID );				//1.组命令字
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

	sprintf(outDataNews, "%d~", ACTCLOSERESPON);						//1.�����ֻ���
	tmpOut = outDataNews + strlen(outDataNews);				//2.������
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

	//1.�����ڴ�
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint( "Error: func  mem_pool_alloc() ");
		assert(0);
	}

	//2. ��װ�ظ���Ϣ
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

	sprintf(outDataNews, "%d~", TEMPLATECMDRESPON);				//1.�������������
	
	tmpOut = outDataNews + strlen(outDataNews); 				//2.���洦����
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );

	if(*tmp == 1)
	{
		tmpOut = outDataNews + strlen(outDataNews);				//3.����ʧ��ԭ��
		tmp += 1;
		sprintf(tmpOut, "%d~", *tmp );
	}


	*outDataToUi = outDataNews;


	return ret;
}

