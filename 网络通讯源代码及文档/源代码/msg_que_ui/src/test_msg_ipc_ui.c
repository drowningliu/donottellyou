#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctype.h>

#include "ui_ipc.h"
#include "template_scan_element_type.h"
#include "msg_que_ui.h"


static bool g_flag = true;		//false : The ipc object destroy



int commom_print(void *buf)
{
	//	int ret = 0;
	//uiIpcHandle *handle = srcHandle;

	if( !buf)
	{
		printf("buf : %p [%d], [%s]\n", buf,  __LINE__, __FILE__);		
	}
	
	printf("buf : %s, [%d], [%s]\n", (char *)buf, __LINE__, __FILE__);

	#if 0
	ret = ui_free_fun_mesque(handle, buf);
	if(ret < 0)
	{
		printf("error: func ui_free_fun_mesque() \n" );
		return ;
	}
	#endif
	return 0;
}

unsigned int atoui(const char *str)
{
    unsigned int result = 0, i = 0;
    char *tmp = NULL;
    for (i = 0; isspace(str[i]) && i < strlen(str); i++)//跳过空白符;    
        ;
    tmp = (char *)(str+i);
    while (*tmp)
    {
        result = result * 10 + *tmp - '0';
        tmp++;
    }

    return result;
}


char *unsigned_int_to_hex(unsigned int number, char *hex_str)
{
	char b[17] = { "0123456789ABCDEF" };
	int c[8], i = 0, j = 0, d, base = 16;

	do{
		c[i] = number % base;
		i++;
		number = number / base;
	} while (number != 0);

	hex_str[j++] = '0';
	hex_str[j++] = 'X';

	for (--i ; i >= 0; --i, j++)
	{
		d = c[i];
		hex_str[j] = b[d];
	}
	hex_str[j] = '\0';

	return hex_str;
}


/*功能: 打印指定共享块的内容, 后发送消息,还回该共享内存块
*@param : shared_memory : 本进程链接的共享内存首地址
*@param : buf : 消息队列接收的消息, 格式: cmd~偏移量~
*/ 
int template_struct_print(void *buf )
{
	char *tmp = NULL;		
	unsigned int offset = 0;					//共享内存的偏移量
	char tmpOffsetBuf[30] = { 0 };				//获取buf中偏移量的字符串
	char sendBuf[60] = { 0 };
	templateScanElement *tmpTemplate = NULL;
		
//	printf("recv message : %s [%d],[%s]\n", (char*)buf, __LINE__, __FILE__);

	//1.获取地址
	tmp = strchr(buf, '~');
	memcpy(tmpOffsetBuf, tmp+1, strlen(tmp)-2);
	offset = hex_conver_dec(tmpOffsetBuf);		

	//2.取出指定 共享内存块中的数据
	tmpTemplate = (templateScanElement*)( NULL + offset);
#if 0
	printf("\t\ttmpTemplate->identifyID : %s \n \
		tmpTemplate->objectAnswer : %s \n \
		tmpTemplate->totalNumberPaper : %d \n \
		tmpTemplate->toalPaperName[0] : %s \n\
		tmpTemplate->subMapCoordNum : %d \n\
		tmpTemplate->coordsDataToUi[0] : ID :%d, TopPoint(%d, %d), DownPoint(%d, %d) \n\
		tmpTemplate->coordsDataToUi[1] : ID :%d, TopPoint(%d, %d), DownPoint(%d, %d) \n", tmpTemplate->identifyID,
		tmpTemplate->objectAnswer,  tmpTemplate->totalNumberPaper, 
		tmpTemplate->toalPaperName[0], tmpTemplate->subMapCoordNum,
		tmpTemplate->coordsDataToUi[0].MapID,
		tmpTemplate->coordsDataToUi[0].TopX, tmpTemplate->coordsDataToUi[0].TopY,
		tmpTemplate->coordsDataToUi[0].DownX, tmpTemplate->coordsDataToUi[0].DownY,
		tmpTemplate->coordsDataToUi[1].MapID,
		tmpTemplate->coordsDataToUi[1].TopX, tmpTemplate->coordsDataToUi[1].TopY,
		tmpTemplate->coordsDataToUi[1].DownX, tmpTemplate->coordsDataToUi[1].DownY);
#endif
	printf("paperName : %s\n", tmpTemplate->toalPaperName[0] );
	
	
	sprintf(sendBuf, "%d~%s~", 0x24, tmpOffsetBuf);
	if(ui_send_fun_mesque( sendBuf) < 0)
	{
		printf("err: func ui_send_fun_mesque() [%d],[%s]\n", __LINE__, __FILE__);
		return 0 ;
	}
	return 0 ;
}



//子线程执行逻辑
void *recv_thread_func(void *arg)
{
	int ret = 0;	
	char *recvbuf = NULL;
	pthread_detach(pthread_self());
	char cmdBuf[5] = { 0 };
	int cmd = 0;
	
	//2.执行逻辑	
	while(g_flag)
	{
		ret = ui_recv_fun_mesque(&recvbuf);
		if(ret == -1)				//fail
		{
			printf("error: func ui_recv_fun_mesque() [%d],[%s]\n",__LINE__, __FILE__ );
			return NULL;
		}
		else if(ret == -2)			//The close IPC 
		{
			printf("the opposite site will colsed, [%d],[%s]\n",__LINE__, __FILE__ );
			printf("The recv data : %s, [%d],[%s]\n", recvbuf, __LINE__, __FILE__);			
		}		

		//3.获取消息队列中的命令字
		memset(cmdBuf, 0, sizeof(cmdBuf));
		sscanf(recvbuf, "%[^~]", cmdBuf);
		cmd = atoi(cmdBuf);
		
		if(cmd == 0xA1)
		{
			template_struct_print(recvbuf);
		}
		else
		{		
			myprint("recvBuf : %s", recvbuf);
		}
		if((ui_free_fun_mesque(recvbuf)) < 0)
		{
			printf("Err : func ui_recv_fun_mesque() [%d],[%s]\n",__LINE__, __FILE__ );
			assert(0);
		}
		
	}
	
	printf("the son recv thread closed [%d] [%s]\n", __LINE__, __FILE__);

	return NULL;
}




int main()
{
	int ret = 0;
	int getc = 0;
	pthread_t pthid = -1;


	//1.init The handle;
	ret = init_ipc_ui(0x1234, 0x1235);
	if(ret < 0)
	{
		printf("err: func init_ipc_ui(), [%d],[%s]\n", __LINE__, __FILE__);
		return ret ;
	}

	
	//3.创建子线程, 用于接收信息
	ret = pthread_create(&pthid, NULL, recv_thread_func, NULL);
	if(ret < 0)
	{
		printf("err: func pthread_create() [%d],[%s]\n", __LINE__, __FILE__);
		return ret;
	}
	
	//4.执行发送逻辑
	while(1)
	{
		printf("1 : send message; 3 : close IPC object\n");
		getc = getchar();

		switch (getc)
		{
			case 49:
				ret = ui_send_fun_mesque( "88~nihao~");
				if(ret < 0)
				{
					printf("err: func ui_send_fun_mesque() [%d],[%s]\n", __LINE__, __FILE__);
					return ret ;
				}
				break;
			case 51:
				g_flag = false;		
				printf("------0---------\n");
				ret = destroy_ipc_Que_Shm();
				if(ret < 0)
				{
					printf("error: func ui_close_fun_mesque() [%d],[%s]\n", __LINE__, __FILE__);
					return -1;
				}
				break;
			default:
				assert(0);		
		}
		if(getc == 51)		break;

		getchar();
	}

	
	return ret;
}

