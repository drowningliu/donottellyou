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
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "socklog.h"
#include "data_package.h"
#include "sock_pack.h"
#include "memory_pool.h"
#include "queue.h"
#include "escape_char.h"
#include "dnsAddr.h"
#include "fifo.h"
#include "template_scan_element_type.h"

#define FILEDIR "/home/yyx/work/openssl-FTP-TCP/project"
#define FILENAME "C_16_01_04_10_16_10_010_B_L.jpg"


#define myprint( x...) do {char buf[1024];\
            sprintf(buf, x);\
            fprintf(stdout, "%s [%d], [%s]\n", buf,__LINE__, __FILE__ );\
    }while (0)


static bool g_flag = true;
sem_t g_sem_test;

int g_flag_close = 1;			//0 标识关闭，   1： 标识正常

//子线程执行逻辑
void *recv_thread_func(void *arg)
{
	int ret = 0;
	pthread_detach(pthread_self());
	char *str = NULL;


	while(g_flag)
	{
		ret = send_ui_data(&str);
		if(ret)
		{
			printf("Error: func send_ui_data() %s, [%d]\n", __FILE__, __LINE__);
				if(strcmp(str, "0~") == 0)			continue;
			return NULL;
		}
		printf("buf : %s, [%d], [%s]\n", str, __LINE__, __FILE__);	
		
		ret = free_memory_net(str);
		if(ret)
		{
			printf("Error: func free_memory_net() %s, [%d]\n", __FILE__, __LINE__);
			return NULL;
		}	
		if(strcmp(str, "0~") == 0)			g_flag_close = 0;
				
		sem_post(&g_sem_test);
	}
	
	printf("the son pthread will closed [%s], [%d]\n", __FILE__, __LINE__);
	
	return NULL;
	
}	

//制作结构体信息, UI --> Net
void template_element_news(UitoNetExFrame *news)
{
	int i = 0;
	char *object = "A~C~DCA~A~BDC~D~";
	Coords tmpCoord = { 21, 45, 45, 45, 25 };
	
	memcpy(news->fileDir, FILEDIR, strlen(FILEDIR));
	memcpy(news->sendData.identifyID, "1548645154864514515", 20);
	memcpy(news->sendData.objectAnswer, object, strlen(object));
	news->sendData.totalNumberPaper =1;
	
	for(i = 0; i < news->sendData.totalNumberPaper; i++)	
		memcpy(news->sendData.toalPaperName[i], FILENAME, strlen(FILENAME));
	
	news->sendData.subMapCoordNum = 20;
	for(i = 0; i < news->sendData.subMapCoordNum; i++)
		news->sendData.coordsDataToUi[i] = tmpCoord;
	
}


//发送数据给 Net, 
int send_content_to_server()
{
	int ret = 0, i = 0;
	UitoNetExFrame sendNews;
	char buf[50] = { 0 };
	
	//1. 链接 TCP
	ret = recv_ui_data("10~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}		
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");

	
	//2. 登录用户
	ret = recv_ui_data("1~44845548158~121544~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}		
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");
	
	myprint("---- test 0 -----------");
	
	//3. 发送模板扫描信息集合, 包含多张图片和数据块
	memset(buf, 0, sizeof(buf));
	template_element_news(&sendNews);
	myprint("---- test 1 -----------" );
	//sprintf(buf, "12~%p~", &sendNews);
	sprintf(buf, "12~%p~", &sendNews);
	printf("========test 2 : %s============", buf );
	//2. 登录用户
	ret = recv_ui_data(buf);
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}		
	sem_wait(&g_sem_test);

	myprint("---- test 3 -----------");
	//goto End;
	//5.上传图片
	for( i= 0;  i < 3000; i++)
	{
		ret = recv_ui_data("6~./C_16_01_04_10_16_10_010_B_L.jpg~");
		if(ret)
		{
			printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
			break;
		}	
		if(g_flag_close == 0)	goto End;
		sem_wait(&g_sem_test);
		//printf("\n-----The send Number is : %d --------------------------\n\n", i+1);
	}
sleep(3);
		//6.删除服务器指定图片
	ret = recv_ui_data("9~./C_16_01_04_10_16_10_010_B_L.jpg~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");
	
		
	//8.修改配置文件，重新读取
	ret = recv_ui_data("11~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");
	
	//8.1 主动断开网络链接
	ret = recv_ui_data("8~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");
	
	
	//9.退出程序
	g_flag = false;
	ret = destroy_client_net();
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_destroy(&g_sem_test);

End:
	
	return ret;
}


int test_main()
{
	int ret = 0;
	pthread_t pthid = -1;
	sem_init(&g_sem_test, 0, 0);			//
	
	//1.初始化
	ret = init_client_net("./net.ini");
	if(ret)
	{
		printf("Error: func init_client() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}
	printf("----------------22222222222---------------\n\n");
		
	//2.创建子线程, 用于接收信息
	ret = pthread_create(&pthid, NULL, recv_thread_func, NULL);
	if(ret < 0)
	{
		printf("err: func pthread_create()\n");
		return ret;
	}
	
	printf("-------------1--------------\n");
	
	//3.进行数据信息发送
	ret = send_content_to_server();
	if(ret < 0)
		myprint("Error : func send_content_to_server()");
	

	return ret;
}


int main()
{	
	int ret = 0;
	
	ret = test_main();
	if(ret < 0)
		myprint("Error : func test_main()");
	
	return ret;
}
