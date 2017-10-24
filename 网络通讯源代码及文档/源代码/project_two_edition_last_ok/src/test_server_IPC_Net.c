#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<strings.h>
#include<ctype.h>
#include<arpa/inet.h>
#include<unistd.h>
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
#include "server_ipc_process.h"
#include "func_compont.h"


#define SERVER_IP		"192.168.1.196"	//服务器IP
#define SERVER_PORT		8009			//服务器端口
#define	KEY				0x1234			//消息队列键值
#define FILEDIR			"/home/lsh/share/NH_1200/soure"			//指定目录
#define UPLOADDIRPATH	"/home/yyx/work/openssl-FTP-TCP/project/uploadTest"		//上传文件加目录
#define RMFILEPATH 		"/home/lsh/share/NH_1200/Common_image"	
#define FILEFORMAT 		".jpg"			//指定格式




//每隔500ms， 发送指定目录下的指定格式文件
static int sendFilePath(listNode *handle, char *dirPath, char *fileFormat);
//select()  实现定时器;
static void timer_select(long tv_sec, long tv_usec);
extern int  scan_send_msgque(char *dataInfor, listNode *handle);
typedef void (*sighandler_t)(int);


extern sigjmp_buf g_sigEnv;
extern sigjmp_buf g_sigEnvEnd;
listNode *g_handle = NULL;


char *g_ipv4IP = NULL;
int  g_port = 0;



void sigint_cath(int sig)
{
	 myprint("The func sigint_cath() is %d", sig);
	 siglongjmp(g_sigEnv, 8);
}

void sign_child(int sign)
{
	while((waitpid(-1, NULL, WNOHANG)) > 0)		
		printf("receive signal : %u, [%d], [%s]",sign, __LINE__, __FILE__);		   
}
 

//子线程 逻辑， 执行 IPC内容接收
void *recv_thread_func(void *arg)
{
	int ret = 0;
	listNode *handle = (listNode *)arg;
	pthread_detach(pthread_self());

	while(1)
	{
		ret = scan_recv_msgque(handle);
		if(ret < 0)
		{
			printf("err: func scan_recv_msgque() [%d], [%s]\n",__LINE__, __FILE__);
			return NULL;
		}
	
	}
	
	printf("the son recv thread closed [%d], [%s]\n",__LINE__, __FILE__);

	return NULL;
}

//IPC SCAN进程 注册函数
void test_func(void *buf)
{
	printf("buf: %s\n", (char*)buf);
}

int rm_all_file()
{
	int 	ret = 0;
	char	buf[124] = { 0 };
	
	sprintf(buf, "rm %s/*", RMFILEPATH);
	
	ret = system(buf);
	
	return 	ret;
}


//所有测试程序的逻辑入口
int test()
{
	int ret = 0, i = 0;
	int getC = 0;
	char buf[1024] = { 0 };
	pthread_t pthid = -1;
	char fileDir[124] = { 0 };
	listNode *handle = NULL;
	pid_t pid = -1;
	
	
	//1.初始化。服务器和IPC通讯
	//int init_func(char *ip, int port, key_t key, char *fileDir, listNode **handle, pid_t *srcPid)
	ret = init_func(g_ipv4IP, g_port, UPLOADDIRPATH, KEY, RMFILEPATH, &handle, &pid);
	if(ret < 0) 
	{
		if(ret == -1)
		{
			printf("err: func init_func() server init failed [%d], [%s]\n",__LINE__, __FILE__);
			return ret;	
		}
		else if(ret == -2)
		{
			printf("err: func init_func() IPC init failed, server init success [%d], [%s]\n",__LINE__, __FILE__);
			goto End;
		}
		else if(ret == -3)
		{
			printf("err: func init_func() fork() failed[%d], [%s]\n",__LINE__, __FILE__);
			return ret;	
		}
		else
		{
			printf("err: func init_func() failed and retval not Konwn [%d], [%s]\n",__LINE__, __FILE__);
			return ret;	
		}
	}
	
	
#if 1
	//2.IPC　进行函数注册
	ret = insert_node(handle, 33, test_func, buf, sizeof(buf));
	if(ret < 0)
	{
		printf("err: func insert_node() [%d], [%s]\n",__LINE__, __FILE__);
		goto End;
	}
	ret = insert_node(handle, 1, test_func, buf, sizeof(buf));
	if(ret < 0)
	{
		printf("err: func insert_node() [%d], [%s]\n",__LINE__, __FILE__);
		goto End;
	}
	ret = insert_node(handle, 2, test_func, buf, sizeof(buf));
	if(ret < 0)
	{
		printf("err: func insert_node() [%d], [%s]\n",__LINE__, __FILE__);
		goto End;
	}
#endif	

#if 0
	//3.创建线程去执行 IPC 内容接收
	ret = pthread_create(&pthid, NULL, recv_thread_func, handle);
	if(ret < 0)
	{
		printf("err: func pthread_create() [%d], [%s]\n",__LINE__, __FILE__);
		goto End;
	}
#endif
	g_handle = handle;

	//4.用户输入不同的命令，去执行不同的操作
	while(1)
	{
		printf("\n 1: create file And send filePath to UI；  2：close progress； *another cmd, assert() exit; 3: rm -rf *FilePath the File \n");
		sprintf(fileDir, "%d%d%dtest.txt", i, i, i);
		i++;
		
		if((getC = getchar()) == 10)
			continue;
		
		switch (getC)
		{			
		#if 0				
			case 49:
				ret = sendFilePath(handle, FILEDIR, FILEFORMAT);
				if(ret < 0)
				{
					printf("err: func sendFilePath() [%d], [%s]\n",__LINE__, __FILE__);
					goto End;
				}
				break;
		#endif
			case 50:
				printf("the progress will closed [%d], [%s]\n",__LINE__, __FILE__);
				break;
			case 51:
				ret = rm_all_file();
				if(ret < 0)
				{
					printf("err: func rm_all_file() [%d], [%s]\n",__LINE__, __FILE__);
					goto End;
				}
				break;			
			default:
				;	
		}
		if(getC == 50)
			break;
		else if(getC == 10)
			continue;	
			
		getchar();
		
	}
	
	#if 0
	//5.退出消息队列程序
	ret = scan_close_msgque(handle);
	if(ret < 0)
	{
		printf("err: func scan_close_msgque() [%d], [%s]\n",__LINE__, __FILE__);
		goto End;
	}
	#endif
	
End:
	//6.退出服务器程序
	ret = close_server(pid);
	if(ret < 0)
	{
		printf("err: func close_server() [%d], [%s]\n",__LINE__, __FILE__);
		return ret;		
	}
	
	
	return ret;	
}

void cpoy_file_by_filepath(char *fileName)
{
	char buf[512] = { 0 };
	sprintf(buf, "cp %s/%s %s", FILEDIR, fileName, RMFILEPATH);
	system(buf);
	
}


//每隔500ms， 发送指定目录下的指定格式文件
int sendFilePath(listNode *handle, char *dirPath, char *fileFormat)
{
	int ret = 0;
	DIR *dp ;  
    struct dirent *dirp ;	
	int size = 0;			//目录下的文件名长度
	int fileFormatLen = strlen(fileFormat);
	
	//1.打开目录
	if( (dp = opendir( dirPath )) == NULL )  
    {  
      	printf("err: func opendir() [%d], [%s]\n",__LINE__, __FILE__);
		return ret = -1;	
    }  	
		
	//2.循环查找该目录下的 指定格式的文件	
	while((dirp = readdir(dp)) != NULL)
	{
		//1.跳过 . 和 .. 两个目录
		if(strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0 )		continue;
			
		
		size = strlen(dirp->d_name);
		
		//2.跳过与制定格式不符的文件
		if( strcmp((dirp->d_name + (size - fileFormatLen)), fileFormat ) != 0)			continue;
		
		cpoy_file_by_filepath(dirp->d_name);
		
				
		//3.发送文件名
		ret = scan_send_msgque(dirp->d_name, handle);
		if(ret < 0)
		{  
	      	printf("err: func scan_send_msgque() [%d], [%s]\n",__LINE__, __FILE__);
			return ret = -1;	
    	}  
    	
    	//4.定时器 500ms
    	timer_select(0, 500000);
    		
	}	
		
	//关闭目录
	closedir(dp);
		
	return ret;	
}


//select()  实现定时器;
void timer_select(long tv_sec, long tv_usec)
{
	struct timeval temp;
 	temp.tv_sec = tv_sec;
    temp.tv_usec = tv_usec;
    int err = 0;
	do{
		err = select(0, NULL, NULL, NULL, (struct timeval *)&temp);
	}while(err < 0 && errno == EINTR);

}



int main(int argc, char **argv)
{
	int ret = 0;
	
	signal(SIGINT, sigint_cath );
	signal(SIGCHLD, sign_child);
	
	
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
		return -1;
	} 
	
	
	
	ret = test();
	if(ret < 0)
	{
		printf("err: func test() [%d], [%s]\n",__LINE__, __FILE__);
		return ret;
	}

	/*ret = sigsetjmp(g_sigEnvEnd, 1);
	if(ret < 0)
	{
		myprint("CTRL + C 关闭程序成功, ret : %d", ret);
	}*/
		
	
	return ret;
}
