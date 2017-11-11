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
#include <stdbool.h>


#include "socklog.h"
#include "data_package.h"
#include "sock_pack.h"
#include "memory_pool.h"
#include "queue.h"
#include "escape_char.h"
#include "dnsAddr.h"
#include "fifo.h"
#include "template_scan_element_type.h"
#include "func_compont.h"

#define FILEDIR 	"/home/tirvideo/Desktop/project_two_edition_last_ok/"
#define FILENAME 	"C_16_01_04_10_16_10_030_B_L.jpg"




static bool g_flag = true;
sem_t g_sem_test;

int g_flag_close = 1;			//0 标识关闭，   1： 标识正常

//子线程执行逻辑 : 接收消息
void *recv_thread_func(void *arg)
{
	int ret = 0;
	pthread_detach(pthread_self());
	char *str = NULL;
	bool flag = false;

	while(g_flag)
	{
		ret = send_ui_data(&str);
		if(ret)
		{
			printf("Error: func send_ui_data() %s, [%d]\n", __FILE__, __LINE__);
			//if(strcmp(str, "0~") == 0)			continue;
			return NULL;
		}
		printf("buf : %s, addr : %p, [%d], [%s]\n", str, str,__LINE__, __FILE__);	
		
#if 0
{
			if(strncmp(str, "0~", 2) != 0)		sem_post(&g_sem_test);			
		if(strncmp(str, "135~0~", 6) == 0)	
		{
			
			ret = recv_ui_data("4~0~122~");	
			if(ret < 0)
			{
				printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
				assert(0);
			}	
			sem_wait(&g_sem_test);
			printf("-------------------------------\n\n");
		}				
		else if(strncmp(str, "135~1~", 6) == 0)	
		{
			//sleep(1);
			ret = recv_ui_data("4~1~253~");	
			if(ret < 0)
			{
				printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
				assert(0);
			}	
			sem_wait(&g_sem_test);
			printf("-------------------------------\n\n");
		}
		else if(strncmp(str, "0~", 2) == 0)
		{
			//1. 链接 TCP
			ret = recv_ui_data("10~");
			if(ret)
			{
				printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
				return -1;
			}		
			printf("-------------------------------\n\n");
			if(flag)			sleep(3);
									
			flag = true;
		}
		else if(strncmp(str, "138~1~", 6) == 0)
		{
			//1. 链接 TCP
			ret = recv_ui_data("10~");
			if(ret)
			{
				printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
				return -1;
			}		
			printf("-------------------------------\n\n");
			if(flag)			sleep(3);
		}
		else
		{
			flag = false;
		}

}
#endif		
		ret = free_memory_net(str);
		if(ret < 0)
		{
			printf("Error: func free_memory_net() %s, [%d]\n", __FILE__, __LINE__);
			return NULL;
		}	
	
		str = NULL;		
		sem_post(&g_sem_test);
	}
	
	printf("the son pthread will closed [%s], [%d]\n", __FILE__, __LINE__);
	
	return NULL;
	
}	

// UI --> Net
void template_element_news(UitoNetExFrame *news)
{
	int i = 0;
	char *object = "AB~CB~CAB~ABCD~D~B~CBD~";
	Coords tmpCoord = { 21, 45, 45, 45, 25 };
	char *dir = "/home/tirvideo/Desktop/project_two_edition_last_ok";
	
	memset(news, 0, sizeof(UitoNetExFrame));
	memcpy(news->fileDir, dir, strlen(dir));
	memcpy(news->sendData.identifyID, "1548645154864514515", 20);
	memcpy(news->sendData.objectAnswer, object, strlen(object));
	news->sendData.totalNumberPaper = 3;
	
	for(i = 0; i < news->sendData.totalNumberPaper; i++)	
		memcpy(news->sendData.toalPaperName[i], FILENAME, strlen(FILENAME));
	
	news->sendData.subMapCoordNum = 20;
	for(i = 0; i < news->sendData.subMapCoordNum; i++)
		news->sendData.coordsDataToUi[i] = tmpCoord;
	
}


void down_load_template_element(char *buf)
{
	int i = 0, len = 0; 
	downLoadTemplateRequest  downFile;
	
	downFile.projectNum = 6;
	for(i = 0; i < 6; i++)
	{
		(downFile.conData)[i].projectID = i * 10 + 1;
		(downFile.conData)[i].optionID = i + 5;
	}
	
	len = sprintf(buf, "%s", "4~");
	memcpy(buf + len, &downFile, sizeof(downLoadTemplateRequest));
	*(buf + sizeof(downLoadTemplateRequest) + len) = '~';
}

//发送数据给 Net, 
int send_content_to_server()
{
	int ret = 0, i = 0;
	UitoNetExFrame sendNews;
	
	char buf[120] = { 0 };
	
	//1. 链接 TCP
	ret = recv_ui_data("10~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}		
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");
	
	//2. 客户端登录
	for(i = 0; i < 1; i++)
	{
		ret = recv_ui_data("1~15934098456~121544~");
		if(ret)
		{
			printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
			return -1;
		}		
		sem_wait(&g_sem_test);
	}
	
sleep(100000);

	//6.删除服务器指定图片
	ret = recv_ui_data("9~C_16_01_04_10_16_10_030_B_L.jpg~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");


	for(i = 0; i < 2; i++)
	{
		template_element_news(&sendNews);
		sprintf(buf, "12~%p~", &sendNews);
		//2. 
		ret = recv_ui_data(buf);
		if(ret)
		{
			printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
			return -1;
		}		
		sem_wait(&g_sem_test);
	}


	memset(buf, 0, sizeof(buf));
	sprintf(buf, "4~%d~%d~", 1, 1574514);
	ret = recv_ui_data(buf);	
	if(ret < 0)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");


	memset(buf, 0, sizeof(buf));
	sprintf(buf, "4~%d~%d~", 0, 1574514);
	ret = recv_ui_data(buf);	
	if(ret < 0)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");

	//3. 上传图片
	for( i= 0;  i < 3; i++)
	{
		ret = recv_ui_data("6~./C_16_01_04_10_16_10_030_B_L.jpg~");
		if(ret)
		{
			printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
			break;
		}	
		if(g_flag_close == 0)	goto End;
		sem_wait(&g_sem_test);	
		//sleep(15);
		//break;
	}
	

	ret = recv_ui_data("5~0~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}		
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");

	//5. 上传模板
	for(i = 0; i < 3; i++)
	{
		template_element_news(&sendNews);
		sprintf(buf, "12~%p~", &sendNews);

		ret = recv_ui_data(buf);
		if(ret)
		{
			printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
			return -1;
		}		
		sem_wait(&g_sem_test);
	}
	ret = recv_ui_data("5~1~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}		
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");	



	//7.修改配置文件，重新读取
	ret = recv_ui_data("11~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");
sleep(10000);	
	//7.1 主动断开网络链接
	ret = recv_ui_data("8~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");


#if 0


	
	

#endif		
	
	sleep(100000);
	//4. 下载模板
	

		
#if 0



	//1. 加密传输
	ret = recv_ui_data("14~0~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}		
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");

	//2. 客户端登录
	ret = recv_ui_data("1~15934098456~121544~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}		
	sem_wait(&g_sem_test);
	
	

	/*
	down_load_template_element(buf);	
	ret = recv_ui_data(buf);	
	if(ret < 0)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");
	*/

	//3. 发送模板扫描信息集合, 包含多张图片和数据块

	
	



	getchar();	
	//5.上传图片
	for( i= 0;  i < 3000; i++)
	{
		ret = recv_ui_data("6~./C_16_01_04_10_16_10_030_B_L.jpg~");
		if(ret)
		{
			printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
			break;
		}	
		if(g_flag_close == 0)	goto End;
		sem_wait(&g_sem_test);		
	}
	
	
	sleep(3);
	
	//6.删除服务器指定图片
	ret = recv_ui_data("9~./C_16_01_04_10_16_10_030_B_L.jpg~");
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_wait(&g_sem_test);
	printf("-------------------------------\n\n");
	
		

	
	
	//8.退出程序
	g_flag = false;
	ret = destroy_client_net();
	if(ret)
	{
		printf("Error: func recv_ui_data() %s, [%d]\n", __FILE__, __LINE__);
		return -1;
	}	
	sem_destroy(&g_sem_test);
#endif
	

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
