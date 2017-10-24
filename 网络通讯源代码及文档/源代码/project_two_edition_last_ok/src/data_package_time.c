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
#include <netinet/tcp.h>


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


//
#define DIVISIONSIGN 			'~'
#define ACCOUTLENTH 			11							//
#define CAPACITY    			110							//
#define WAIT_SECONDS 			2							//
#define HEART_TIME				300							//
#undef 	BUFSIZE
#define BUFSIZE					250							//
#define DOWNTEMPLATENAME		"NHi1200_TEMPLATE_NAME"		//
#define DOWNTABLENAME           "NHi1200_DATABASE_NAME"
#define DOWNLOADPATH			"NHi1200_TEMPLATE_DIR"
#define CERT_FILE 				"/home/yyx/nanhao/client/client-cert.pem" 
#define KEY_FILE   				"/home/yyx/nanhao/client/client-key.pem" 
#define CA_FILE  				"/home/yyx/nanhao/ca/ca-cert.pem" 
#define CIPHER_LIST	 			"AES128-SHA"
#define	MY_MIN(x, y)			((x) < (y) ? (x) : (y))	
#define TIMEOUTRETURNFLAG		-5							//


//
mem_pool_t  				*g_memPool = NULL;				//
queue       				*g_queueRecv = NULL;			//
queue_send_dataBLock 		g_queue_sendDataBlock = NULL;	//
fifo_type					*g_fifo = NULL;					//
queue_buffer 				*g_que_buf = NULL;				//
LListSendInfo				*g_list_send_dataInfo = NULL;	//

//
sem_t 			g_sem_cur_read;		 					//
sem_t 			g_sem_cur_write;		 				//
sem_t 			g_sem_send;								//
sem_t 			g_sem_read_open;						//
sem_t 			g_sem_business;							//
sem_t 			g_sem_unpack;							//
sem_t 			g_sem_sequence;							//



//
pthread_mutex_t	 g_mutex_client = PTHREAD_MUTEX_INITIALIZER; 			
pthread_mutex_t	 g_mutex_memPool = PTHREAD_MUTEX_INITIALIZER; 			//
pthread_mutex_t	 g_mutex_list = PTHREAD_MUTEX_INITIALIZER; 				//
pthread_mutex_t  g_mutex_serial = PTHREAD_MUTEX_INITIALIZER;			//

//
pthread_t  			g_thid_read = -1, g_thid_unpack = -1, g_thid_business = -1, g_thid_heartTime = -1, g_thid_time = -1,  g_thid_write = -1;	//
int  				g_close_net = 0;					//
int  				g_sockfd = 0;						//
char 				g_filePath[128] = { 0 };			//
bool 				g_err_flag = false;					//
pid_t 				g_dhcp_process = -1;				//
uploadWholeSet 		g_upload_set[10];					//
uploadWholeSet 		g_upload_error_set[10];				//
char 				*g_downLoadTemplateName = NULL, *g_downLoadTableName = NULL, *g_downLoadPath = NULL;//
int 				g_send_package_cmd = -1;			//
bool  				g_encrypt = false, g_encrypt_data_recv = false;				//
char 				*g_machine = "21212111233";			//
unsigned int 		g_seriaNumber = 0;					//
unsigned int	 	g_recvSerial = 0;					//
SendDataBlockNews	g_lateSendPackNews;					//
bool 				g_perIsRecv = false;				//
char 				*g_sendFileContent = NULL;			//
bool 				g_residuePackNoNeedSend = false;	//
bool 				g_modify_config_file = false;		//
bool 				g_template_uploadErr = false;		//
bool 				g_subResPonFlag = false;			//
bool 				g_isSendPackCurrent = false;		//
#if 0
SSL_CTX 			*g_sslCtx = NULL;
SSL  				*g_sslHandle = NULL;
#endif

typedef void (*sighandler_t)(int);
//


//
void pipe_func(int sig)
{
	myprint( "\nthread:%lu, SIGPIPIE appear %d", pthread_self(), sig);
}

//
void sign_handler(int sign)
{	
	myprint(  "\nthread:%lu, receive signal:%u---", pthread_self(), sign);
	pthread_exit(NULL);
}

//
void sign_usr2(int sign)
{
	myprint("\nthread:%lu, receive signal:%u---", pthread_self(), sign);
}

//
void sign_child(int sign)
{
	while((waitpid(-1, NULL, WNOHANG)) > 0)
	{
		printf("receive signal : %u, [%d], [%s]",sign, __LINE__, __FILE__);
	}
		   
}

//
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


//
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
	
	//
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
	//
	if((if_file_exist("LOG")) == true)
	{
		sprintf(cmdBuf, "rm %s", "LOG/*");
		if(pox_system(cmdBuf) == 0)
		{
			myprint("Ok, rm LOG/* ...");	
		}
	}

	//
	if((ret = init_global_value()) < 0)
	{
		myprint( "Error: func pthread_create()");
		goto End;
	}
	//

	//
 	if((ret = init_pthread_create()) < 0)
	{
		myprint( "Error: func init_pthread_create()");
		goto End;
	}
	//

End:
	printf("func init_client() end, [%d],[%s]\n", __LINE__, __FILE__);
	return ret;
}



//
static int set_local_ip_gate(char *loaclIp, char *gate)
{
	int ret = 0;
	char buf[512] = { 0 };
	//
		
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
	//
	return ret;
}


//
static int init_global_value()
{
	int ret = 0;
	char *fileName = NULL;
	char *tableName = NULL;

	//
	if((g_queue_sendDataBlock = queue_init_send_block(CAPACITY)) == NULL)
	{
		ret = -1;
		myprint( "Error: func queue_init_send_block()");
		goto End;
	}	
	//
	if((g_queueRecv = queue_init(CAPACITY)) == NULL)		
	{
		ret = -1;
		myprint(  "Error: func queue_init()");
		goto End;
	}
	//
	if((g_memPool = mem_pool_init(CAPACITY, PACKMAXLENTH * 2, &g_mutex_memPool)) == NULL)
	{
		ret = -1;
		myprint(  "Error: func mem_pool_init()");
		goto End;
	}
	//
	if((g_fifo = fifo_init(CAPACITY * 1024 * 2)) == NULL)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: func fifo_init()");
		goto End;
	}	
	//
	if((g_que_buf = queue_buffer_init(BUFSIZE, BUFSIZE)) == NULL)
	{
		ret = -1;
		myprint( "Error: func queue_buffer_init()");
		goto End;
	}
	//
	if((g_list_send_dataInfo = Create_index_listSendInfo(PERROUNDNUMBER * 3, &g_mutex_list)) == NULL)
	{
		ret = -1;
		myprint( "Error: func Create_index_listSendInfo()");
		goto End;

	}
	//
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
		
	//
	sem_init(&g_sem_cur_read, 0, 0);			
	sem_init(&g_sem_cur_write, 0, 0);			
	sem_init(&g_sem_send, 0, 0);				
	sem_init(&g_sem_read_open, 0, 0);			
	sem_init(&g_sem_business, 0, 0);			
	sem_init(&g_sem_unpack, 0, 0);				
	sem_init(&g_sem_sequence, 0, 1);			


	//
	memset(g_upload_error_set, 0, sizeof(g_upload_error_set));
	memset(g_upload_set, 0, sizeof(g_upload_set));

	//
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
	 
	 //
	 if((ret = pthread_create(&g_thid_business, NULL, thid_business, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
 
	 //
	 if((ret = pthread_create(&g_thid_read, NULL, thid_server_func_read, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create() thid_server_func_read");
		 goto End;
	 }
 
	 //
	 if((ret = pthread_create(&g_thid_write, NULL, thid_server_func_write, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
	 //
	 if((ret = pthread_create(&g_thid_time, NULL, thid_server_func_time, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 } 
	 //
	 if((ret = pthread_create(&g_thid_unpack, NULL, thid_unpack_thread, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
 
#if 0
	 //
	 if((ret = pthread_create(&g_thid_heartTime, NULL, thid_hreat_beat_time, NULL)) < 0)
	 {
		 myprint( "Error: func pthread_create()");
		 goto End;
	 }
#endif
	 
 End:
 
	 return ret;
}


 //
static int get_config_file_para()
{
 	int ret = 0;
 	char *localIp = NULL, *gateTmp = NULL, *ServerIpTmp = NULL, *doMainNameTmp = NULL;
 	dictionary  *ini ;

 	//
 	if((ini = iniparser_load(g_filePath)) == NULL)
 	{
 		ret = -1;
        myprint(  "Error : func iniparser_load() don't open the file g_filePath : %s", g_filePath);
 		goto End;
    }

    //
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


//
//
int chose_setip_method()
{
	int ret = 0;
	
	if(conten.dhcp)		//
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
	else				//
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



//
int destroy_client_net()
{
	int ret = 0;

	//
	g_close_net = 1;					
	sem_post(&g_sem_cur_read);			 
	sleep(1);

	//
	if((ret = pthread_kill(g_thid_read, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() recvData Thread");
	}

	//
	if((ret = pthread_kill(g_thid_write, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() sendData Thread");
	}

	//
	if((ret = pthread_kill(g_thid_time, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() timeOut Thread");
	}	

	//
	if((ret = pthread_kill(g_thid_business, SIGUSR1)) < 0)
	{
		myprint("Error: func pthread_kill() cmd Thread");
	}

	//
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

	//
	if(g_memPool)					mem_pool_destroy(g_memPool);			

	//
	if(g_queue_sendDataBlock)		destroy_queue_send_block(g_queue_sendDataBlock);	
	if(g_queueRecv)					destroy_queue(g_queueRecv);				
	if(g_fifo)						destroy_fifo_queue(g_fifo);								
	if(g_que_buf)					destroy_buffer_queue(g_que_buf);

	//
	if(g_downLoadTableName)			free(g_downLoadTableName);
	if(g_downLoadTemplateName)		free(g_downLoadTemplateName);
	
	//
	sem_destroy(&g_sem_cur_read);
	sem_destroy(&g_sem_cur_write);
	sem_destroy(&g_sem_send);
	sem_destroy(&g_sem_read_open);
	sem_destroy(&g_sem_business);
	sem_destroy(&g_sem_sequence);
	sem_destroy(&g_sem_unpack);
	
	//
	pthread_mutex_destroy(&g_mutex_client);
	pthread_mutex_destroy(&g_mutex_list);
	pthread_mutex_destroy(&g_mutex_memPool);
	pthread_mutex_destroy(&g_mutex_serial);
	

	//
	if(g_sockfd > 0)
	{
		close(g_sockfd);
		g_sockfd = -1;
	}

	//
	if(g_dhcp_process > 0)
	{
		kill(g_dhcp_process, SIGKILL);
	}

	return ret;
}



//
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

	//
	sscanf(news, "%[^~]", cmdBuf);
	cmd = atoi(cmdBuf);

	//
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

	//
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

	//
	if(ret == 0)
	{	
		//
		sem_post(&g_sem_business);		
	}
	
End:
	return ret;
}

//
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

	//
	while(1)
	{
		sem_wait(&g_sem_cur_read);	
		if(g_close_net == 0 || g_close_net == 2)
		{
			//
			if((ret = data_process(buf)) == -1)
			{
				myprint("Error: func data_process() index : %d", index++);			
				assert(0);			
			}
			else if(ret == 1)
			{
				//
				continue;
			}
			else if(ret == 0)
			{
				//
				break;
			}

		}
	#if 0	
		else if(g_close_net == 1)		//
		{
			sprintf(g_request_reply_content, "%s", " ");				//
			*buf = g_request_reply_content;
			break;
		}
	#endif	
	}
	
	sem_post(&g_sem_sequence);	
	
	//

End:
	return ret;
}



//
int data_process(char **buf)
{
	int ret = 0;
	char *sendUiData = NULL;				//
	uint8_t cmd = 0;						//
	
	
	//
	if((ret = pop_queue(g_queueRecv, &sendUiData)) < 0)		
	{
		myprint("Error: func pop_queue()");
 		assert(0);
	}

	//
	cmd = *((uint8_t *)sendUiData);

	//
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

	//
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


//
int free_memory_net(char *buf)
{
	int ret = 0;
	
	if(buf)
	{
		//
		if(g_close_net != 1)		//
		{
			if((ret = mem_pool_free(g_memPool, buf)) < 0)
			{
				ret = -1;
				myprint("Error: func  mem_pool_alloc() ");		
				assert(0);
			}		
		}
	#if 0	
		else			//
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


	//
	if((tmp = strchr(news, '~')) == NULL)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error : func strchr()");
		goto End;
	}
	memcpy(bufAddr, tmp + 1, strlen(tmp) - 2);

	
	//
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		socket_log(SocketLevel[4], ret, "Error : NULL == news");
		goto End;
	}


	//
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

	//
	while(1)
	{
		timer_select(HEART_TIME);

		if(!g_isSendPackCurrent)		//
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




//
void *thid_business(void *arg)
{
	int ret = 0, cmd = -1;
	char tmpBuf[4] = { 0 };				//
	int numberCmd = 1;					//
	
	pthread_detach(pthread_self());
	
	socket_log( SocketLevel[3], ret, "func thid_business() begin");

	while(1)
	{
		sem_wait(&g_sem_business);			//
		sem_wait(&g_sem_sequence);			//

		char 	cmdBuffer[BUFSIZE] = { 0 };	//
		int		num = 0;		
				
		//
		memset(tmpBuf, 0, sizeof(tmpBuf));
		ret = pop_buffer_queue(g_que_buf, cmdBuffer);
		if(ret == -1)
		{			
			socket_log(SocketLevel[4], ret, "Error: func pop_buffer_queue()");
			continue;
		}
		
		printf("\n-------------- number cmd : %d, content : %s\n\n", numberCmd, cmdBuffer);
		//
		numberCmd += 1;
		
		//
		sscanf(cmdBuffer, "%[^~]", tmpBuf);
		cmd = atoi(tmpBuf);		
		
		do 
		{				
			//
			switch (cmd){
				case 0x01:		//
					if((ret = data_packge(login_func, cmdBuffer)) < 0)			
						socket_log(SocketLevel[4], ret, "Error: data_packge() login_func"); 									
					break;				
				case 0x02:		//
					if((ret = data_packge(exit_program, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() exit_program");					
					break;
				case 0x03:		//
					if((ret = data_packge(heart_beat_program, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() heart_beat_program");
					break;					
				case 0x04:		//
					if((ret = data_packge(download_file_fromServer, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() download_template");
					break;
				case 0x05:		//
					if((ret = data_packge(get_FileNewestID, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() get_FileNewestID");
					break;	
				
				case 0x06:		//
					if((ret = data_packge(upload_picture, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() upload_picture");
					break;
#if 1					
				case 0x07:		//
					if((ret = data_packge(push_info_from_server_reply, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() upload_picture");
					break;
				case 0x08:		//
					if((ret = data_packge(active_shutdown_net, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() active_shutdown_net");
					break;
				case 0x09:		//
					if((ret = data_packge(delete_spec_pic, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() delete_spec_pic");
					break;
#endif					
				case 0x0A:		//
					if((ret = data_packge(connet_server, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() connet_server");
					break;

				case 0x0B:		//
					if((ret = data_packge(read_config_file, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() read_config_file");
					break;
					
				case 0x0C:		//
					if((ret = data_packge(template_extend_element, cmdBuffer)) < 0)
						socket_log(SocketLevel[4], ret, "Error: data_packge() template_extend_element");
					break;
#if 0					
				case 0x0E:		//
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
				sleep(1);		//
				num += 1;
			}
		}while( 0 );
//
		
	}

	socket_log( SocketLevel[3], ret, "func thid_business() end");

	return	NULL;
}

//
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
            else	//
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
	char 	*tmpSend = NULL;		//
	int 	tmpSendLenth = 0;		//
	int 	nSendLen = 0;			//

	
	//
	if(( ret = pop_queue_send_block(g_queue_sendDataBlock, &tmpSend, &tmpSendLenth, sendCmd)) < 0)
	{
		myprint( "Error: func get_queue_send_block()");
		assert(0);
	}
	g_send_package_cmd = *sendCmd;

	//
	if(!g_encrypt)					//
	{		
		nSendLen = writen(g_sockfd, tmpSend, tmpSendLenth);		
	}
	else if(g_encrypt)				//
	{		
//
	}
	do{
		if(nSendLen == tmpSendLenth)	
		{
			//
			g_lateSendPackNews.cmd = *sendCmd;
			g_lateSendPackNews.sendAddr = tmpSend;
			g_lateSendPackNews.sendLenth = tmpSendLenth;
			time(&(g_lateSendPackNews.sendTime));	

			if(g_send_package_cmd == PUSHINFOREQ)
			{
				remove_senddataserver_addr();				
				sem_post(&g_sem_send);	//
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

//
void remove_senddataserver_addr()
{
	
	trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
	if((clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
	{			
		myprint("Error: func clear_LinkListSendInfo() ");
		assert(0);
	}

}


//
int recv_data(int sockfd)
{
	int ret = 0, recvLenth = 0, number = 0;
	char buf[PACKMAXLENTH * 2] = { 0 };
	static int nRead = 0;
	
	while(1)
	{

		//
		if(g_encrypt == false )
		{
			recvLenth = read(g_sockfd, buf, sizeof(buf));
		}
		else if(g_encrypt)
		{
//
		}
		nRead += recvLenth;;
		if(recvLenth < 0)		
		{			
			//
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
			//
			ret = -1;
			myprint("The server has closed !!!");			
			goto End;
		}
		else if(recvLenth > 0)		
		{
			//
			do{
				//
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

			//
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
	int checkCode = 0;					//
	int *tmpCheckCode = NULL;			//
	uint8_t cmd = 0;					//
	resCommonHead_t  *comHead = NULL;	//
	resSubPackHead_t  *subHead = NULL;	//


	//
	cmd = *((uint8_t *)tmpContent);

	//
	if(cmd != DOWNFILEZRESPON)
	{
		//
		comHead = (resCommonHead_t *)tmpContent;

		//
		if(comHead->contentLenth + sizeof(int) == tmpContentLenth)
		{
			if(cmd == LOGINCMDRESPON)	
			{
				g_recvSerial = comHead->seriaNumber;  //
			}	

			//
			if(comHead->seriaNumber >= g_recvSerial)
			{				
				//
				tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));			
				checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
				if(checkCode == *tmpCheckCode)
				{
					//
					g_recvSerial = comHead->seriaNumber + 1;		//
					ret = 0;				
				}
				else
				{
					//
					myprint( "Error: cmd : %d, contentLenth : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
									cmd , comHead->contentLenth, comHead->seriaNumber, checkCode, *tmpCheckCode);
					ret = -1;
				}		
			}
			else
			{
				//
				socket_log(SocketLevel[3], ret, "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u", 
											cmd,  comHead->seriaNumber, g_recvSerial);
				ret = -2;		
			}
		}
		else
		{
			//
			socket_log(SocketLevel[3], ret, "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u", 
						cmd, comHead->contentLenth + sizeof(int), tmpContentLenth, comHead->seriaNumber);
			ret = -2;			
		}
	}
	else	//
	{
		subHead = (resSubPackHead_t *)tmpContent;

		//
		if(subHead->contentLenth + sizeof(int) == tmpContentLenth)
		{
			//
			if(g_recvSerial <= subHead->seriaNumber)
			{
								
				//
				tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
				checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
				if(*tmpCheckCode == checkCode)
				{
					//
					g_recvSerial = subHead->seriaNumber + 1;		//
					ret = 0;
				}
				else
				{
					//
					myprint( "Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
									cmd , subHead->seriaNumber, checkCode, *tmpCheckCode);
					ret = -1;
				}
			}
			else
			{
				//
				socket_log(SocketLevel[3], ret, "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u", 
											cmd,  subHead->seriaNumber, g_seriaNumber);
				ret = -2;	
			}			
		}
		else
		{
			//
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
	char tmpPackData[PACKMAXLENTH] = { 0 };		//
	int outLenth = 0;
	reqPackHead_t *tmpHead;
	int  checkCode = 0;				//
	int	 outDataLenth = 0;

	//
	if((anti_escape(sendAddr + 1, sendLenth - 2, tmpPackData, &outLenth)) < 0)
	{
		myprint("Error : func anti_escape()");
		assert(0);
	} 
	
	//
	tmpHead = (reqPackHead_t*)tmpPackData;
	pthread_mutex_lock(&g_mutex_serial);
	tmpHead->seriaNumber = g_seriaNumber++;
	pthread_mutex_unlock(&g_mutex_serial);

	//
	checkCode = crc326((const char *)tmpPackData, tmpHead->contentLenth);
	tmp = tmpPackData + tmpHead->contentLenth;
	memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

	//
	if((escape(PACKSIGN, tmpPackData, tmpHead->contentLenth + sizeof(uint32_t), sendAddr + 1, &outDataLenth)) < 0)
	{
		myprint("Error: func escape() ");		 
		assert(0);
	}
	*(sendAddr + outDataLenth + 1) = PACKSIGN;

	//
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
	resCommonHead_t  *comHead = NULL;	//
	char *tmpPool = NULL;				//
	
	//
	comHead = (resCommonHead_t*)tmpContent;

	//
	if(comHead->isFailPack == 1)
	{
		//
		repeat_latePack();		
	}
	else
	{
		//
		if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
		{
			ret = -1;
			myprint( "Error: func mem_pool_alloc() ");
			assert(0);
		}
		memcpy(tmpPool, tmpContent, tmpContentLenth);
		
		//
		if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
		{
			myprint( "Error: func push_queue() ");
			mem_pool_free(g_memPool, tmpPool);
			assert(0);
		}
		sem_post(&g_sem_cur_read);

		//
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{			
			myprint("Error: func clear_LinkListSendInfo() ");
			assert(0);
		}

		//
		g_isSendPackCurrent = false;
		
		//
		sem_post(&g_sem_send);
	}


	return ret;
}

int pushInfo_func(char *tmpContent, int tmpContentLenth)
{
	int ret = 0;
	//
	char *tmpPool = NULL;				//

	//
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}
	memcpy(tmpPool, tmpContent, tmpContentLenth);
	
	//
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
	char *tmpResult = NULL;				//
	resCommonHead_t  head;				//
	int contentLenth = 0;	
	char *tmp = tmpPool;
	
	//
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

	//
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
	resCommonHead_t  *comHead = NULL;	//
	node *repeatPackage = NULL;			//
	char *tmpPool = NULL;				//
	int  newSendLenth = 0; 				//

	//
	comHead = (resCommonHead_t*)tmpContent;

	//
	if(comHead->isSubPack == 1)
	{		
		g_subResPonFlag = true;			//
		//
		if(comHead->isFailPack == 1)
		{
			//
			if((repeatPackage = FindNode_ByVal_listSendInfo(g_list_send_dataInfo, comHead->failPackIndex, my_compare_packIndex)) == NULL)
			{
				//
				socket_log(SocketLevel[4], ret, "Error: func FindNode_ByVal_listSendInfo() ");
				ret = 0;		//
			}
			else
			{
				//
				modify_repeatPackSerialNews(repeatPackage->sendAddr, repeatPackage->sendLenth, &newSendLenth);
				repeatPackage->sendLenth = newSendLenth;				
				if((ret = push_queue_send_block(g_queue_sendDataBlock, repeatPackage->sendAddr, repeatPackage->sendLenth, UPLOADCMDREQ)) < 0)
				{
					//
					myprint("Error: func push_queue_send_block(), package News : cmd : %d, lenth : %d, sendAdd : %p ", 
							UPLOADCMDREQ, repeatPackage->sendLenth, repeatPackage->sendAddr);
					assert(0);
				}
				else
				{
					//
					sem_post(&g_sem_cur_write);
				}
			}
		}
		else
		{				
			//
			if(comHead->isRespondClient == 1)
			{
				//
				if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
				{
					myprint("Err : func mem_pool_alloc() to resPond UI");					
					assert(0);
				}

				//
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
				//
				g_isSendPackCurrent = false;				
			}
							
			//
			trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
			if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
			{
				myprint("Error : func clear_LinkListSendInfo()");
				assert(0);
			}
			g_subResPonFlag = false;				//

			//
			sem_post(&g_sem_send);
		}
	}
	else
	{		
		//
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
	resCommonHead_t  *comHead = NULL;	//
	node *repeatPackage = NULL;			//
	char *tmpPool = NULL;				//
	char *tmpResult = NULL;				//
	int  newSendLenth = 0; 				//

	
	//
	comHead = (resCommonHead_t*)tmpContent;

	//
	if(comHead->isSubPack == 1)
	{
		g_subResPonFlag = true;			//
		//
		if(comHead->isFailPack == 1)
		{
			//
			if((repeatPackage = FindNode_ByVal_listSendInfo(g_list_send_dataInfo, comHead->failPackIndex, my_compare_packIndex)) == NULL)
			{
				//
				socket_log(SocketLevel[4], ret, "Error: func FindNode_ByVal_listSendInfo() ");
				ret = 0;		//
			}
			else
			{
				//
				modify_repeatPackSerialNews(repeatPackage->sendAddr, repeatPackage->sendLenth, &newSendLenth);
				repeatPackage->sendLenth = newSendLenth;				
				if((ret = push_queue_send_block(g_queue_sendDataBlock, repeatPackage->sendAddr, repeatPackage->sendLenth, UPLOADCMDREQ)) < 0)
				{
					//
					myprint("Error: func push_queue_send_block(), package News : cmd : %d, lenth : %d, sendAdd : %p ", 
							UPLOADCMDREQ, repeatPackage->sendLenth, repeatPackage->sendAddr);
					assert(0);
				}
				else
				{
					//
					sem_post(&g_sem_cur_write);
				}
			}
		}
		else
		{				
			//
			if(comHead->isRespondClient == 1)
			{
				//
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

				//
				tmpResult = tmpContent + sizeof(resCommonHead_t);
				if(*tmpResult == 0x02)
				{
					g_residuePackNoNeedSend = true;
				}
				//
				g_isSendPackCurrent = false;
			
			}

			//
			trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
			if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
			{
				myprint("Error : func clear_LinkListSendInfo()");
				assert(0);
			}
			g_subResPonFlag = false;				//

			//
			sem_post(&g_sem_send);
		}
	}
	else
	{
		//
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
	resSubPackHead_t  *comHead = NULL;	//
	char *tmpPool = NULL;				//
	
	//
	comHead = (resSubPackHead_t *)tmpContent;
		

	//
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}
	memcpy(tmpPool, tmpContent, tmpContentLenth);
	
	//
	if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
	{
		myprint( "Error: func push_queue() ");
		mem_pool_free(g_memPool, tmpPool);
		assert(0);
	}
	sem_post(&g_sem_cur_read);

	//
	if(comHead->currentIndex + 1 == comHead->totalPackage)
	{	
		//
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{			
			myprint("Error: func clear_LinkListSendInfo() ");
			assert(0);
		}
		//
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
	resCommonHead_t  *comHead = NULL;	//
	char *tmpPool = NULL;				//
	int  newSendLenth = 0;				//
	char *tmp = NULL;
	
	//
	comHead = (resCommonHead_t*)tmpContent;
	tmp = tmpContent + sizeof(resCommonHead_t);

	//
	if(comHead->isFailPack == 1)
	{
		//
		modify_repeatPackSerialNews(g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, &newSendLenth);
		g_lateSendPackNews.sendLenth = newSendLenth;
		if((push_queue_send_block(g_queue_sendDataBlock, g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, g_lateSendPackNews.cmd)) < 0)
		{
			myprint("Error : func push_queue_send_block()");
			assert(0);
		}
		//
		sem_post(&g_sem_cur_write);
	}
	else
	{
		if(*tmp == 0)
		{
			//
			sem_post(&g_sem_send);
			return ret;
		}
		//
		if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
		{
			myprint( "Error: func mem_pool_alloc() ");
			assert(0);
		}
		memcpy(tmpPool, tmpContent, tmpContentLenth);
		
		//
		if((ret = push_queue(g_queueRecv, tmpPool)) < 0)
		{
			myprint( "Error: func push_queue() ");
			assert(0);
		}
		sem_post(&g_sem_cur_read);
	
		//
		trave_LinkListSendInfo(g_list_send_dataInfo, freeMapToMemPool);
		if((ret = clear_LinkListSendInfo(g_list_send_dataInfo)) < 0)
		{			
			myprint("Error: func clear_LinkListSendInfo() ");
			assert(0);
		}
		//
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
	uint8_t cmd = 0;					//


	//
	cmd = *((uint8_t *)tmpContent);

	//
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


//
int	data_unpack_process(int recvLenth)
{
	int ret = 0, tmpContentLenth = 0;
	static int flag = 0, lenth = 0; 	//
	char buffer[2] = { 0 };
	static char content[2800] = { 0 };	//
	char tmpContent[1400] = { 0 }; 		//
	char *tmp = NULL;
	
	tmp = content + lenth;
	while(recvLenth > 0)
	{
		while(1)
		{

			//
			if(get_fifo_element_count(g_fifo) == 0)
			{
				goto End;
			}
			//
			if((ret = pop_fifo(g_fifo, (char *)buffer, 1)) < 0)
			{
				myprint( "Error: func pop_fifo() ");
				assert(0);
			}
			//
			if(*buffer == 0x7e && flag == 0)
			{
				flag = 1;
				continue;
			}
			else if(*buffer == 0x7e && flag == 1)	//
			{
				//
				if((ret = anti_escape(content, lenth, tmpContent, &tmpContentLenth)) < 0)
				{
					myprint( "Error: func anti_escape() no have data");
					goto End;
				}
										
				if((ret = compare_recvData_Correct( tmpContent, tmpContentLenth)) == 0)				
				{
					//
					//

					if((deal_with_pack(tmpContent, tmpContentLenth )) < 0)
					{
						myprint("Error: func deal_with_pack()");
						assert(0);
					}

					//
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
					//
					myprint("Error: func compare_recvData_Correct()");					
					//
					memset(content, 0, sizeof(content));
					memset(tmpContent, 0, sizeof(tmpContent));					
					recvLenth -= (lenth + 2);
					lenth = 0;
					flag = 0;
					tmp = content + lenth;

					//
					trave_LinkListSendInfo(g_list_send_dataInfo, modify_listNodeNews);						
  					ret = 0;			//
					break;
				}
				else if(ret == -2)
				{
					//
					//
					//
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
				//
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

//
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


//
void *thid_server_func_time(void *arg)
{
	time_t  nowTime = 0, timeDiffer = 0;		//
	int waitNumber = 0;							//

	//
	pthread_detach(pthread_self());

	//
	while(1)
	{		
		//
		timer_select(OUTTIMEPACK + 3);
		//
		if(g_err_flag)
		{
			waitNumber = 0;
			continue;	
		}
		//
		if((Size_LinkListSendInfo(g_list_send_dataInfo)) == 0)
		{
			waitNumber = 0;
			//
			continue;	
		}
		else if((Size_LinkListSendInfo(g_list_send_dataInfo)) > 1)
		{	
			//
			time(&nowTime);
			timeDiffer = nowTime - g_lateSendPackNews.sendTime;

			//
			if(timeDiffer < OUTTIMEPACK)
			{
				//
				continue;
			}
			else
			{
				if(waitNumber < 2)
				{
					myprint("respond timeOut : %d > time : %d, repeatNumber : %d", timeDiffer, OUTTIMEPACK, waitNumber);
					//
					if(g_subResPonFlag)
					{	
						//
						repeat_latePack();
					}	
					else
					{
						//
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
			//
			time(&nowTime);
			timeDiffer = nowTime - g_lateSendPackNews.sendTime;

			//
			if(timeDiffer < OUTTIMEPACK)
			{
				//
				continue;
			}
			else
			{
				if(waitNumber < 2)
				{
					myprint("respond timeOut : %d > time : %d, repeatNumber : %d", timeDiffer, OUTTIMEPACK, waitNumber);
					//
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

	//
	modify_repeatPackSerialNews(g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, &newSendLenth);

	//
	g_lateSendPackNews.sendLenth = newSendLenth;
	if((push_queue_send_block(g_queue_sendDataBlock, g_lateSendPackNews.sendAddr, g_lateSendPackNews.sendLenth, g_lateSendPackNews.cmd)) < 0)
	{
		myprint("Error : func push_queue_send_block()");
		assert(0);
	}
	
	//
	sem_post(&g_sem_cur_write);

}


/*unpack The data */
void *thid_unpack_thread(void *arg)
{
	int   ret = 0;
	pthread_detach(pthread_self());

	while(1)
	{
		//
		sem_wait(&g_sem_unpack);

		//
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


//
int send_error_netPack(int sendCmd)
{
	int ret = 0;
	int cmd = 0, err = 0, errNum = 0;	//


	//


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
	//
	if((ret = copy_news_to_mem(cmd, err, errNum)) < 0)
	{		
		myprint("Error: func copy_news_to_mem()");
		assert(0);
	}


	//

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

	//
	if((tmpPool = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}

	//
	tmp = tmpPool;
	memset(&head, 0, sizeof(resCommonHead_t));
	head.cmd = cmd;
	head.contentLenth = sizeof(resCommonHead_t) + 2;
	memcpy(tmp, &head, sizeof(resCommonHead_t));
	tmp += sizeof(resCommonHead_t);
	*tmp++ = err;
	*tmp++ = errNum;
	

	//
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
	
	//

	//
	g_err_flag = true;


	//
	if((ret = clear_fifo(g_fifo)) < 0)								
	{
		myprint("Error: func clear_fifo()");		
		assert(0);
	}

	//
	while (get_element_count_send_block(g_queue_sendDataBlock)) 	//
	{
		if((ret = pop_queue_send_block(g_queue_sendDataBlock, NULL, NULL, NULL)) < 0) //
		{
			myprint("Error: func pop_queue_send_block()");
			assert(0);
		}
	}

	//
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


	//
	if(flag == 1)				//
	{
		sem_init(&g_sem_read_open, 0, 0);		//
	}
#if 0	
	if(flag == 1)								//
	{		
		sem_post(&g_sem_send);					//
		sem_init(&g_sem_read_open, 0, 0);		//
		//
	}
	else										//
	{
		sem_post(&g_sem_send);					//
	}
#endif		
	//

	
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
	
	//

	//
	if(g_isSendPackCurrent)
	{
		send_error_netPack(g_send_package_cmd);
		sem_post(&g_sem_send);					//
		g_isSendPackCurrent = false;
	}
	//
	clear_global(flag);

	//
	if(flag == TIMEOUTRETURNFLAG)
	{
		return ret;
	}
	
	//
	assign_resComPack_head(&head, ERRORFLAGMY, sizeof(resCommonHead_t), 1, 1 );
	
	//
	if((sendData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error : func mem_pool_alloc()");
		assert(0);
	}
	memcpy(sendData, &head, sizeof(resCommonHead_t));

	//
	if((ret = push_queue(g_queueRecv, sendData)) < 0)
	{
		myprint("Error : func push_queue()");
		mem_pool_free(g_memPool, sendData);		
		assert(0);
	}		

	//
	g_recvSerial = 0;
	sem_post(&g_sem_cur_read);
	
	
	//
	return ret;
}


//
void *thid_server_func_read(void *arg)
{
	int ret = 0;
	int num = 0;
	
	//
	pthread_detach(pthread_self());

	while(1)
	{
		//
		sem_wait(&g_sem_read_open);			

		while(1)
		{
			//
			if((ret = recv_data(g_sockfd)) < 0)			
			{		
				//
				if(g_sockfd > 0)
				{
					close(g_sockfd);
					g_sockfd = 0;
				}

				//
				if(g_close_net == 1 || g_close_net == 2)	
				{
					clear_global(1);
					myprint( "The UI active shutdown NET in read thread");				
					break;
				}
				else if(g_close_net == 0)		
				{	
					
					//
					do{
						if((ret = error_net(1)) < 0)		//
						{
							myprint(  "Error: func error_net()");
							sleep(1);	
							num++;			//
						}
						else				//
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


//
void *thid_server_func_write(void *arg)
{
	int ret = 0;
	int sendCmd = -1;		//
	
	//

	//
	pthread_detach(pthread_self());

	//
	while(1)
	{
		//
		sem_wait(&g_sem_cur_write);		
		//

		//
		if((ret = send_data(g_sockfd, &sendCmd)) < 0)		
		{
			//
			//
							
			//
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

	pthread_exit(NULL);		//
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

//
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

	//

	//
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
	memcpy(passWd, tmp + 1, strlen(tmp)-2);				//
	passWdSize = strlen(passWd);

	//
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

	//
	contenLenth = sizeof(reqPackHead_t) + ACCOUTLENTH + sizeof(char) + passWdSize;
	assign_reqPack_head(&head, LOGINCMDREQ, contenLenth, NOSUBPACK, 0, 1, 1);

	//
	tmp = tmpSendPackDataOne;
	memcpy(tmp, (char *)&head, sizeof(reqPackHead_t));
	tmp += sizeof(reqPackHead_t);
	memcpy(tmp, accout, ACCOUTLENTH);
	tmp += ACCOUTLENTH;
	*tmp++ = passWdSize;
	memcpy(tmp, passWd, passWdSize);

	//
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	tmp += passWdSize;
	memcpy(tmp, &checkCode, sizeof(int));

	//
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData + 1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;

	//
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

	sem_post(&g_sem_cur_write);	//
	sem_wait(&g_sem_send);

	//

	
	return ret;
}

//
int  exit_program(const char* news)
{
	int ret = 0;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0;
	int outDataLenth = 0;

	//
	assign_reqPack_head(&head, LOGOUTCMDREQ, sizeof(reqPackHead_t), NOSUBPACK, 0, 1, 1);

	//
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

	//
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const  char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne + sizeof(reqPackHead_t), (char *)&checkCode, sizeof(uint32_t));

	//
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//
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

	sem_post(&g_sem_cur_write);	//
	sem_wait(&g_sem_send);


	return ret;
}


//
int  heart_beat_program(const char* news)
{
	int ret = 0;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0;
	int outDataLenth = 0;


	//
	assign_reqPack_head(&head, HEARTCMDREQ, sizeof(reqPackHead_t), NOSUBPACK, 0, 1, 1);

	//
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

	//
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne + head.contentLenth, (char *)&checkCode, sizeof(uint32_t));
	
	//
	*sendPackData = PACKSIGN; 
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//
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

//
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

	//
	if((tmp = strchr(news, DIVISIONSIGN)) == NULL)
	{		
		myprint( "Error: func strstr() No find");
		assert(0);
	}
	tmp++;

	//
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

	//
	tmpOut = tmpSendPackDataOne + sizeof(reqPackHead_t);
	if(*tmp == '0') 			*tmpOut++ = 0;
	else if(*tmp == '1')		*tmpOut++ = 1;
	else						assert(0);
	tmp += 2;
	memcpy(IdBuf, tmp, strlen(tmp) - 1);
	fileid = atoll(IdBuf);
	memcpy(tmpOut, &fileid, sizeof(long long int)); 	
	contentLenth = sizeof(reqPackHead_t) + sizeof(char) * 65;

	//
	assign_reqPack_head(&head, DOWNFILEREQ, contentLenth, NOSUBPACK, 0, 1, 1);
	memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));

	//
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	tmp = tmpSendPackDataOne + head.contentLenth;
	memcpy(tmp, (char *)&checkCode, sizeof(int));


	//
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//
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



//
int  get_FileNewestID(const char* news)
{
	int ret = 0;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0;
	int outDataLenth = 0, contentLenth = 0;
	char *tmp = NULL, *tmpOut = NULL ;
	
	//
	if((tmp = strchr(news, DIVISIONSIGN)) == NULL)
	{
		myprint( "Error: func strstr() No find");
		assert(0);
	}
	tmp++;
	
	//
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

	//
	tmpOut = tmpSendPackDataOne + sizeof(reqPackHead_t);
	if(*tmp == '0') 			*tmpOut++ = 0;
	else if(*tmp == '1')		*tmpOut++ = 1;
	else						assert(0);
	contentLenth = sizeof(reqPackHead_t) + sizeof(char) * 1;
			
	//
	assign_reqPack_head(&head, NEWESCMDREQ, contentLenth, NOSUBPACK, 0, 1, 1);
	memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));

	//
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	tmp = tmpSendPackDataOne + head.contentLenth;
	memcpy(tmp, (char *)&checkCode, sizeof(int));
	
	
	//
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//
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

//
int send_perRounc_begin(int cmd, int totalPacka,int sendPackNumber)
{
	 reqPackHead_t  head;								//
	 char *sendPackData = NULL;							//
	 char tmpSendPackDataOne[PACKMAXLENTH] = { 0 };		//
	 int checkCode = 0;									//
	 char *tmp = NULL;
	 int ret = 0, outDataLenth = 0;
	 
	 //
	 assign_reqPack_head(&head, cmd, sizeof(reqPackHead_t), NOSUBPACK, 0, totalPacka, sendPackNumber);
			
	 //
	 if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
	 {			 
	 	 myprint("Error : func mem_pool_alloc()");
		 assert(0);
 	 }

	 //
	 memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
	 checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
	 tmp = tmpSendPackDataOne + head.contentLenth;
	 memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

	 //
	 *sendPackData = PACKSIGN;							 //
	 if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	 {
		 myprint("Error : func escape()");
		 assert(0);
	 }
	 *(sendPackData + outDataLenth + 1) = PACKSIGN; 	 //

	 //
	 if((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, cmd)) < 0)
	 {
		 myprint("Error : func push_queue_send_block()");
		 assert(0);
	 }

	 //
	 if((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, cmd )) < 0)
	 {
		  myprint( "Error: func Pushback_node_index_listSendInfo()");
		  assert(0);
	 }
	
	 sem_post(&g_sem_cur_write);
	 sem_wait(&g_sem_send);

	 return ret;

}


//
int  upload_picture(const char* news)
{
	int ret = 0, index = 0, i = 0;			//
	uint16_t  tmpTotalPack = 0, totalPacka = 0;				//
	char *tmp = NULL;
	char filepath[250] = { 0 };				//
	char fileName[100] = { 0 };				//
	int  checkCode = 0;						//
	int doucumentLenth = 0 ;				//
	int lenth = 0, packContenLenth = 0;		//
	char *sendPackData = NULL;				//
	char *tmpSendPackDataOne = NULL;		//
	int outDataLenth = 0;					//
	FILE *fp = NULL;						//
	reqPackHead_t  head;					//
	int   sendPackNumber = 0;				//
	int   nRead = 0;						//
	int   copyLenth = 0;					//
	int  tmpLenth = 0;

	
	//
	doucumentLenth = COMREQDATABODYLENTH - FILENAMELENTH;
	
	//
	if((tmp = strchr(news, '~')) == NULL)
	{
		myprint("Error : The content style is err : %s", news);
		assert(0);
	}
	memcpy(filepath, tmp + 1, strlen(tmp) - 2);

	//
	if((ret = get_file_size(filepath, (int *)&totalPacka, doucumentLenth, NULL)) < 0)
	{
		myprint( "Error: func get_file_size()");
		assert(0);
	}
	tmpTotalPack = totalPacka;

	//
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

	//
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;		
		myprint("Error: func mem_pool_alloc() ");
		assert(0);
	}

	//
	while(!feof(fp))
	{
		//
		if(tmpTotalPack > PERROUNDNUMBER)			sendPackNumber = PERROUNDNUMBER;
		else										sendPackNumber = tmpTotalPack;
	
		//
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

		//
		for(i = 0; i < sendPackNumber; i++)
		{
			//
			 tmp = tmpSendPackDataOne + sizeof(reqPackHead_t);
			 memcpy(tmp, fileName, FILENAMELENTH);
			 tmp += FILENAMELENTH;
			 
			 //
			 copyLenth = MY_MIN(lenth, doucumentLenth);		
			 memcpy(tmp, g_sendFileContent + nRead , copyLenth);
			 nRead += copyLenth;
			 lenth -= copyLenth;
			
			 //
			 packContenLenth = sizeof(reqPackHead_t) + FILENAMELENTH + copyLenth;
			 assign_reqPack_head(&head, UPLOADCMDREQ, packContenLenth, SUBPACK, index, totalPacka, sendPackNumber);
			 
			 //
			 if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			 {			 
				 myprint("Error : func mem_pool_alloc()");
				 assert(0);
		 	 }

			 //
			 memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
			 checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
			 tmp = tmpSendPackDataOne + head.contentLenth;
			 memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

			 //
			 *sendPackData = PACKSIGN;							 //
			 if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
			 {
				 myprint("Error : func escape()");
				 assert(0);
			 }
			 *(sendPackData + outDataLenth + 1) = PACKSIGN; 	 //

			 //
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

		//
		nRead = 0;
		tmpTotalPack -= sendPackNumber;
		lenth = 0;

		//
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

	//
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint("Error : func mem_pool_free()");
		assert(0);
	}
	if(fp)			fclose(fp);			fp = NULL;


	return ret;
}


//
int  push_info_from_server_reply(const char* news)
{
	int ret = 0;
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
	int  checkCode = 0;
	int outDataLenth = 0;

	//
	assign_reqPack_head(&head, PUSHINFOREQ, sizeof(reqPackHead_t), NOSUBPACK, 0, 1, 1);

	//
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

	//
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne + head.contentLenth, (char *)&checkCode, sizeof(int));


	//
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData+outDataLenth+1) = PACKSIGN;

	//
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



//
int  active_shutdown_net()
{
	int  ret = 0;
	resCommonHead_t head;				//
	char *sendData = NULL;

	sem_init(&g_sem_read_open, 0, 0);			//
	g_close_net = 2;							//
	g_err_flag = true;							//

	//
	if(g_sockfd > 0)
	{
		shutdown(g_sockfd, SHUT_RDWR); 
		g_sockfd = -1;
	}

	//
	if((sendData = mem_pool_alloc(g_memPool)) == NULL)
	{		
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}
		
	//
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

	//
	if((ret = push_queue(g_queueRecv, sendData)) < 0)
	{
		myprint( "Error: func push_queue() " );								
		assert(0);
	}		
	sem_post(&g_sem_cur_read);			

	//
	if(g_modify_config_file)
	{
		if((ret = chose_setip_method()) < 0)
		{
			myprint( "Error: func chose_setip_method() " );								
			assert(0);
		}
		
		sleep(2);		//
		g_modify_config_file = false;
	}
	
	return ret;
}


//
int delete_spec_pic(const char *news)
{
	int ret = 0;
	char fileName[50] = { 0 };			
	reqPackHead_t head;
	char *tmpSendPackDataOne = NULL, *sendPackData = NULL, *tmp = NULL;
	int  checkCode = 0;
	int outDataLenth = 0;		

	//
	if((tmp = strchr(news, DIVISIONSIGN)) == NULL)
	{
		myprint( "Error: func strstr() No find");
		assert(0);
	}
	//
	
	memcpy(fileName, tmp + 1, strlen(tmp) - 2);			

	//
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
	//
	//
	assign_reqPack_head(&head, DELETECMDREQ, sizeof(reqPackHead_t) + FILENAMELENTH, NOSUBPACK, 0, 1, 1);

	//
	memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
	checkCode = crc326((const  char*)tmpSendPackDataOne, head.contentLenth);
	memcpy(tmpSendPackDataOne+head.contentLenth, (char *)&checkCode, sizeof(uint32_t));
	//

	
	//
	*sendPackData = PACKSIGN;  
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;
	
	//
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



//
int  connet_server(const char *news)
{	
	int  ret = 0, contentLenth = 0;		//
	char *sendData = NULL;				//
	resCommonHead_t head;				//
	int keepalive = 1;
	int keepidle = 2;
	int keepinterval = 1;
	int keepcount = 1;

	//
	if(g_sockfd <= 0)
	{		
		if((ret = client_con_server(&g_sockfd, WAIT_SECONDS)) < 0)			
		{
			//
			g_sockfd = -1;
			myprint("Error: func client_con_server()");
		}
		else if(ret == 0)			
		{		
			//
			sem_post(&g_sem_read_open); 	//
			g_err_flag = false; 			//
			g_close_net = 0;				//
			setsockopt(g_sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
			setsockopt(g_sockfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
			setsockopt(g_sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
			setsockopt(g_sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));
						
		}
		
	}

	//
	if((sendData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}

	//
	if(ret == 0)		*(sendData + sizeof(resCommonHead_t)) = ret;
	else				*(sendData + sizeof(resCommonHead_t)) = 1;
	contentLenth = sizeof(resCommonHead_t) + 1;	

	//
	assign_resComPack_head(&head, CONNECTCMDRESPON, contentLenth, 0, 0);
	memcpy(sendData, &head, sizeof(resCommonHead_t));

	//
	if((ret = push_queue(g_queueRecv, sendData)) < 0)
	{
		myprint("Error: func push_queue() no have data");
		mem_pool_free(g_memPool, sendData); 	
		assert(0);
	}

	//
	sem_post(&g_sem_cur_read);				

	return ret;
}


//
int read_config_file(const char *news)
{
	
	int  ret = 0;
	resCommonHead_t head;				//
	int contenLenth = 0;
	char *sendData = NULL, *tmp = NULL;

	//
	ret = get_config_file_para();

	//
	if((sendData = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func mem_pool_alloc() ");
		assert(0);
	}
	tmp = sendData + sizeof(reqPackHead_t);
	if(ret < 0) 	*tmp = 1;
	else			*tmp = ret;
	contenLenth = sizeof(reqPackHead_t) + sizeof(char);

	//
	assign_resComPack_head(&head, MODIFYCMDRESPON, contenLenth, 0, 0);

	//
	memcpy(sendData, &head, sizeof(resCommonHead_t));

	//
	if((ret = push_queue(g_queueRecv, sendData)) < 0)
	{
		myprint( "Error: func push_queue() no have data");
		assert(0);
	}

	//
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
	reqPackHead_t head;							//
	char *tmpSendPackDataOne = NULL;				//
	char *sendPackData = NULL;						//
	int  checkCode = 0, contentLenth = 0;			//
	int outDataLenth = 0;							//
	char bufAddr[20] = { 0 };						//
	unsigned int apprLen = 0; 							
	UitoNetExFrame *tmpExFrame = NULL;					
						

	//
	if((tmp = strchr(news, DIVISIONSIGN)) == NULL)
	{
		myprint( "Error: func strstr() No find");
		assert(0);
	}		
	memcpy(bufAddr, tmp + 1, strlen(tmp) - 2);	
	apprLen = hex_conver_dec(bufAddr);	
	tmp = NULL + apprLen;
	tmpExFrame = (UitoNetExFrame*)tmp;

	//
	contentLenth = sizeof(reqPackHead_t) + sizeof(unsigned char) * 4 + strlen(tmpExFrame->sendData.identifyID) + strlen(tmpExFrame->sendData.objectAnswer) + tmpExFrame->sendData.subMapCoordNum * sizeof(Coords) + tmpExFrame->sendData.totalNumberPaper * sizeof(tmpExFrame->sendData.toalPaperName[0]);
	assign_reqPack_head(&head, TEMPLATECMDREQ, contentLenth, NOSUBPACK, 0, 1, 1);

	//
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
	
	//
	//
	apprLen = strlen(tmpExFrame->sendData.identifyID);		
	tmp = tmpSendPackDataOne;
	memcpy(tmp, (char *)&head, sizeof(reqPackHead_t));			
	tmp += sizeof(reqPackHead_t);								
	*tmp = apprLen;
	tmp += 1;
	memcpy(tmp, tmpExFrame->sendData.identifyID, apprLen);
	//
	tmp += apprLen;
	apprLen = strlen(tmpExFrame->sendData.objectAnswer);
	*tmp = apprLen;
	tmp += 1;
	memcpy(tmp, tmpExFrame->sendData.objectAnswer, apprLen);
	//
	tmp += apprLen;	
	*tmp = tmpExFrame->sendData.totalNumberPaper;
	tmp += 1;
	apprLen = tmpExFrame->sendData.totalNumberPaper;
	memcpy(tmp, tmpExFrame->sendData.toalPaperName, sizeof(tmpExFrame->sendData.toalPaperName[1]) * apprLen );
	tmp += sizeof(tmpExFrame->sendData.toalPaperName[1]) * apprLen;
	//
	*tmp = tmpExFrame->sendData.subMapCoordNum;
	tmp += 1;
	memcpy(tmp, tmpExFrame->sendData.coordsDataToUi, tmpExFrame->sendData.subMapCoordNum * sizeof(Coords));

	//
	checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);	//
	tmp = tmpSendPackDataOne + head.contentLenth;
	memcpy(tmp, &checkCode, sizeof(uint32_t));	

	//
	*sendPackData = PACKSIGN; 
	if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData + 1, &outDataLenth)) < 0)
	{
		myprint( "Error: func escape()");
		assert(0);
	}
	*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;

	//
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

	//
	memset(g_upload_set, 0, sizeof(g_upload_set));
	for(i = 0; i < tmpExFrame->sendData.totalNumberPaper; i++)
	{
		sprintf(g_upload_set[i].filePath, "%s/%s", tmpExFrame->fileDir, tmpExFrame->sendData.toalPaperName[i]);
		g_upload_set[i].totalNumber = tmpExFrame->sendData.totalNumberPaper;
		g_upload_set[i].indexNumber = i;
	}
	sem_post(&g_sem_cur_write);	//
	sem_wait(&g_sem_send);
	
	if(g_template_uploadErr)
	{
		g_template_uploadErr = false;
		goto End;
	}

	//
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
		//
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
	int ret = 0, index = 0, i = 0;			//
	int  tmpTotalPack = 0, totalPacka = 0;	//
	char *tmp = NULL;
	char filepath[250] = { 0 };				//
	char fileName[100] = { 0 };				//
	int  checkCode = 0;						//
	int doucumentLenth = 0 ;				//
	int lenth = 0, packContenLenth = 0;		//
	char *sendPackData = NULL;				//
	char *tmpSendPackDataOne = NULL;		//
	int outDataLenth = 0;					//
	FILE *fp = NULL;						//
	reqPackHead_t  head;					//
	int   sendPackNumber = 0;				//
	int   nRead = 0;						//
	int   copyLenth = 0;					//
	int  tmpLenth = 0;


	//
	if((ret = get_file_size(filePath, &totalPacka, readLen, NULL)) < 0)
	{
		myprint("Error: func get_file_size() filePath : %s", filePath);
		assert(0);
	}
	doucumentLenth = readLen;
	tmpTotalPack = totalPacka;
	
	//
	if((ret = find_file_name(fileName, sizeof(fileName), filePath)) < 0)
	{
		myprint( "Error: func find_file_name()");
		assert(0);
	}

	//
	if((fp = fopen(filePath, "rb")) == NULL)
	{
		myprint("Error: func fopen(), %s", strerror(errno));
		assert(0);
	}

	//
	if((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint("Error: func mem_pool_alloc()");
		assert(0);
	}

	//
	while(!feof(fp))
	{
		//
		if(tmpTotalPack > PERROUNDNUMBER)			sendPackNumber = PERROUNDNUMBER;
		else										sendPackNumber = tmpTotalPack;
	
		//
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
		//
		for(i = 0; i < sendPackNumber; i++)
		{
			 //
			 tmp = tmpSendPackDataOne + sizeof(reqPackHead_t);
			 *tmp++ = fileNumber;
			 *tmp++ = totalFileNumber;
			 memcpy(tmp, fileName, FILENAMELENTH);
			 tmp += FILENAMELENTH;
			 
			 //
			 copyLenth = MY_MIN(lenth, doucumentLenth); 	
			 memcpy(tmp, g_sendFileContent + nRead , copyLenth);
			 nRead += copyLenth;
			 lenth -= copyLenth;

			 //
			 packContenLenth = sizeof(reqPackHead_t) + FILENAMELENTH + copyLenth + sizeof(char) * 2;
			 assign_reqPack_head(&head, MUTIUPLOADCMDREQ, packContenLenth, SUBPACK, index, totalPacka, sendPackNumber);
			 
			 //
			 if((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			 {			 
				 myprint("Error : func mem_pool_alloc()");
				 assert(0);
			 }

			 //
			memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
			checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
			tmp = tmpSendPackDataOne + head.contentLenth;
			memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

			//
			*sendPackData = PACKSIGN;							//
			if((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData+1, &outDataLenth)) < 0)
			{
				myprint("Error : func escape()");
				assert(0);
			}
			*(sendPackData + outDataLenth + 1) = PACKSIGN;		//
		
			//
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
			
		//
		nRead = 0;
		tmpTotalPack -= sendPackNumber;
		lenth = 0;

		//
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
	//
	if((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
	{
		myprint( "Error: func mem_pool_free()");
		assert(0);
	}

	if(fp) 					fclose(fp);	
						
	return ret;
}



//
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



//
int error_net_reply(char *news, char **outDataToUi)
{
	int ret = 0;
	char *outDataNews = NULL;


	//
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func  mem_pool_alloc() ");		
		assert(0);
	}

	//
	sprintf(outDataNews, "%d~", 0 );
	*outDataToUi = outDataNews;

	return ret;
}


/*登录信息回复
*/
int login_reply_func(char *news, char **outDataToUi)
{
	int ret = 0;
	char *tmp = NULL, *tmpOut = NULL;		//
	resCommonHead_t *head = NULL;			//
	char *outDataNews = NULL;				//

	//
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func  mem_pool_alloc() ");		
		assert(0);
	}

	//
	head = (resCommonHead_t *)news;
	sprintf(outDataNews, "%d~", LOGINCMDRESPON);

	//
	tmp = (char *)news + sizeof(resCommonHead_t);
	tmpOut = outDataNews + strlen(outDataNews);
	if(*tmp == 0x01)
	{			
		//
		sprintf(tmpOut, "%d~", *tmp);			
		tmp += 2;						//
		tmpOut += 2;					//
		memcpy(tmpOut, tmp + 1, *tmp);	//
		tmpOut += *tmp;
		*tmpOut = '~';
	}
	else
	{
		//
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

	//
	*outDataToUi = outDataNews;

	return ret;
}



//
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



//
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
	resSubPackHead_t *tmpHead = NULL;				//
	static bool openFlag = false;					//
	int  nWrite = 0, fileLenth = 0, tmpLenth = 0;

	//
	tmpHead = (resSubPackHead_t *)news;
	tmp = news + sizeof(resSubPackHead_t);

	//
	if(*tmp == 0)		
	{		
		//
		if(!openFlag)
		{
			tmp = news + sizeof(resSubPackHead_t) + sizeof(char) * 2;

			//
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

			//
			if((fp = fopen(fileName, "wb" )) == NULL)
			{
				myprint( "Error: func fopen(), The fileName %s, %s", fileName, strerror(errno));
				assert(0);
			}
			openFlag = true;
		}
		//
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
	
		//
		if(tmpHead->currentIndex + 1 < tmpHead->totalPackage)
		{
			//
			//
			//
			ret = 1;
		}
		else
		{
			//
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

			//
			g_isSendPackCurrent = false;
			
			//
		}
	}
	else
	{
		//
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

		//
		g_isSendPackCurrent = false;
		//
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

	//
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func  mem_pool_alloc() ");
		assert(0);
	}
	
	//
	tmp = (char *)news + sizeof(resCommonHead_t);
	memcpy(&fileID, tmp + 1, sizeof(long long int));
	if(*tmp == 0)					sprintf(outDataNews, "%d~%d~%lld~", NEWESCMDRESPON, 0, fileID);				//
	else if(*tmp == 1)				sprintf(outDataNews, "%d~%d~%lld~", NEWESCMDRESPON, 1, fileID );				//
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
	char *tmp = NULL, *tmpOut = NULL;		//
	char *outDataNews = NULL;				//

	//
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint("Error: func  mem_pool_alloc() ");		
		assert(0);
	}
	
	//
	sprintf(outDataNews, "%d~", UPLOADCMDRESPON);				
	tmpOut = outDataNews + strlen(outDataNews);			
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );

	//
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

	//
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		myprint( "Error: func  mem_pool_alloc() ");
		assert(0);
	}
	
	//
	tmp = (char *)news + sizeof(resCommonHead_t);
	memcpy(&fileID, tmp + 1, sizeof(long long int));
	if(*tmp == 0)					sprintf(outDataNews, "%d~%d~%lld~", PUSHINFORESPON, 0, fileID);				//
	else if(*tmp == 1)				sprintf(outDataNews, "%d~%d~%lld~", PUSHINFORESPON, 1, fileID );				//
	else 							assert(0);

	//
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

	sprintf(outDataNews, "%d~", ACTCLOSERESPON);						//
	tmpOut = outDataNews + strlen(outDataNews);				//
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

	//
	if((outDataNews = mem_pool_alloc(g_memPool)) == NULL)
	{
		ret = -1;
		myprint( "Error: func  mem_pool_alloc() ");
		assert(0);
	}

	//
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

	sprintf(outDataNews, "%d~", TEMPLATECMDRESPON);				//
	
	tmpOut = outDataNews + strlen(outDataNews); 				//
	tmp = (char *)news + sizeof(resCommonHead_t);
	sprintf(tmpOut, "%d~", *tmp );

	if(*tmp == 1)
	{
		tmpOut = outDataNews + strlen(outDataNews);				//
		tmp += 1;
		sprintf(tmpOut, "%d~", *tmp );
	}


	*outDataToUi = outDataNews;


	return ret;
}

