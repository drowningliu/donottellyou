#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <time.h> 
#include <stdbool.h>
#include <signal.h>

#include "IPC_shmory.h"
#include "socklog.h"
#include "template_scan_element_type.h"
#include "msg_que_scan.h"
#include "scan_ipc.h"

	
#undef  CAPACITY_SHMORY
#define CAPACITY_SHMORY 100

typedef void (*sighandler_t)(int);


//init scan ipc : function : IPC queue And IPC shmory;
int init_scan_ipc(key_t ipcQueKey,  key_t ipcShmKey, ipcHandle **myList, char *fileDir,  pthread_mutex_t *mutex )
{
	int ret = 0;
	ipcHandle *handle = NULL;
	ipcShm *memory = NULL;
	char *commonFileDir = NULL;
	char *templateFileDir = NULL;

	if(!myList || !mutex)
	{
		myprint("Err : myList : %p, mutex : %p", myList, mutex);
		ret = -1;
		goto End;
	}


	//1.ģ��ɨ��·��
	if((templateFileDir = getenv("NHi1200_TEMPLATE_SCAN_DIR")) == NULL)
	{
		printf("Err : func getenv(), Key : NHi1200_TEMPLATE_SCAN_DIR [%d], [%s]\n", __LINE__, __FILE__);
		return  -1;
	}
	else
	{
		printf("The templateFileDir : %s [%d], [%s]\n", templateFileDir,  __LINE__, __FILE__);
	}
	//2.����ɨ��·��(��ģ��)
	if((commonFileDir = getenv("NHi1200_COMMON_DIR")) == NULL)
	{
		printf("Err : func getenv(), Key : NHi1200_COMMON_DIR [%d], [%s]\n", __LINE__, __FILE__);
		return  -1;
	}
	else
	{
		printf("The commonFileDir : %s [%d], [%s]\n", commonFileDir,  __LINE__, __FILE__);
	}

	//2. init The handle
	if((ret = scan_open_msgque(0x1234, commonFileDir, templateFileDir, &handle)) < 0)
	{
		myprint("Err : func scan_open_msgque()");
		socket_log(SocketLevel[4], ret, "Err : func scan_open_msgque()"); 
		goto End;
	}	
	if((memory = shmory_pool_init(0x1235, CAPACITY_SHMORY, sizeof(templateScanElement), mutex)) == NULL)
	{
		ret = -1;
		myprint("Err : func scan_open_msgque()");
		socket_log(SocketLevel[4], ret, "Err : func shmory_pool_init()"); 
		goto End;
	}


	handle->shmPool = memory;		//IPC Shmory Pool handle	
	*myList = handle;

End:

	return ret;
}



int create_file_send_filepath(ipcHandle *quehandle, char *fileName, void *picData, unsigned int picLen)
{
	int ret = 0;
	time_t nowtime;    
	struct tm *timeinfo;    
	static int index_front = 0, index_rever = 0;			
	int  wheelNumer = 0;
	char bufContent[100] = { 0 };
	
	char *ext = strrchr(fileName,'.');	
	if(!quehandle || !fileName || !picData || picLen < 0)
	{
		myprint("Err : quehandle : %p, fileName : %p, picData : %p, picLen : %u", quehandle, fileName, picData, picLen );
		socket_log(SocketLevel[4], -1, "Err : quehandle : %p, fileName : %p, picData : %p, picLen : %u", 
										quehandle, fileName, picData, picLen); 
		goto End;
	}

	//1. get The Now time 
	time( &nowtime );	 
	timeinfo = localtime( &nowtime );  
	
	
	//2. parase The scan Time File
	if((strncmp(fileName, "IMAGE0", 6)) == 0)
	{
		wheelNumer = index_front++ % 100;			//��ȡ����ɨ�����к�
		sprintf(bufContent, "C_%02d_%02d_%02d_%02d_%02d_%02d_%03d_F_L.%s", 
		timeinfo->tm_year - 100, timeinfo->tm_mon + 1, timeinfo->tm_mday, 
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, wheelNumer, ext+1);
	}
	else if((strncmp(fileName, "IMAGE1", 6)) == 0 )		
	{
		wheelNumer = index_rever++ % 100;			//��ȡ����ɨ�����к�
		sprintf(bufContent, "C_%02d_%02d_%02d_%02d_%02d_%02d_%03d_B_L.%s",
			timeinfo->tm_year - 100, timeinfo->tm_mon + 1, timeinfo->tm_mday, 
			timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, wheelNumer, ext+1);
	}
	else 
	{
		myprint("Err : The fileName type is not correct: %s", fileName);
		assert(0);
	}
		
	//3.create The file to And send News To Ui by IPC Queue
	if((ret = no_tmeplate_sendData(quehandle, bufContent, picData, picLen)) < 0)
	{
		myprint("Err : func create_file_send_filepath()");
		socket_log(SocketLevel[4], -1, "Err :  func create_file_send_filepath()"); 
		goto End;
	}

End:

	return ret;
}


void *alloc_shmory_block(ipcHandle *shmHandle, unsigned int *offset)
{
	void *blockAddr = NULL;

	if(!shmHandle || !offset)
	{
		myprint("Err : !shmHandle || !offset");
		socket_log(SocketLevel[4], -1, "Err : !shmHandle || !offset");
		return NULL;
	}
	
	if((blockAddr = shmory_pool_alloc(shmHandle->shmPool, offset)) == NULL)
	{
		myprint("Err : func shmory_pool_alloc()");
		socket_log(SocketLevel[4], -1, "Err : func create_file_send_filepath()"); 
	}

	return blockAddr;
}

//��ʵ�� system ϵͳ����
int pox_system(const char *cmd_line) 
{ 
	int ret = 0; 	
	sighandler_t old_handler; 
	old_handler = signal(SIGCHLD, SIG_DFL); 
	ret = system(cmd_line); 
	signal(SIGCHLD, old_handler); 
	if(ret == 127)	ret = -1;
	return ret; 
 }



//modify The file style for scan IMAGE0_
void modify_file_style(ipcHandle *quehandle, unsigned int offset)
{
	templateScanElement *tmp = NULL; 
	struct tm *timeinfo;
	time_t nowtime;    
	int  wheelNumer = 0, i = 0;
	static int index_front = 0, index_rever = 0;
	char fileNameSrc[256] = { 0 };
	char fileNameDst[256] = { 0 };
	char systemBuf[1024] = { 0 };
	char *ext = NULL; 
	char extBuf[10] = { 0 };
	
	//1. get The current time 
	time( &nowtime );	 
	timeinfo = localtime( &nowtime ); 
	tmp = (templateScanElement *)(quehandle->shmPool->buffer_arr + offset);
//	myprint("scan tmp : %p, offset : %d, quehandle->shmPool->buffer_arr : %p", tmp, offset, quehandle->shmPool->buffer_arr );

	//2. parase The scan Time File
	for(i = 0; i < tmp->totalNumberPaper; i++)
	{		
		sprintf(fileNameSrc, "%s%s", quehandle->TemplateFileDir, tmp->toalPaperName[i]);	//��ȡԭʼ�ļ��ľ����ļ�·����
		ext = strrchr(tmp->toalPaperName[i], '.');						//����ԭʼ�ļ����±�
		strcpy(extBuf, ext+1);
		
		if((strncmp(tmp->toalPaperName[i], "IMAGE0", 6)) == 0)			//�޸�ԭʼ�ļ�����
		{			
			wheelNumer = index_front++ % 100;							//��ȡ����ɨ�����к�
			memset(tmp->toalPaperName[i], 0, sizeof(tmp->toalPaperName[i]));
			sprintf(tmp->toalPaperName[i], "C_%02d_%02d_%02d_%02d_%02d_%02d_%03d_F_L.%s", 
					timeinfo->tm_year - 100, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour,
					timeinfo->tm_min, timeinfo->tm_sec, wheelNumer, extBuf);
		}	
		else if((strncmp(tmp->toalPaperName[i], "IMAGE1", 6)) == 0)
		{
			wheelNumer = index_rever++ % 100;							//��ȡ����ɨ�����к�
			memset(tmp->toalPaperName[i], 0, sizeof(tmp->toalPaperName[i]));
			sprintf(tmp->toalPaperName[i], "C_%02d_%02d_%02d_%02d_%02d_%02d_%03d_B_L.%s", 
				timeinfo->tm_year - 100, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour,
				timeinfo->tm_min, timeinfo->tm_sec, wheelNumer, extBuf);
		}	
		else
		{
			myprint("The Name : %s is Error", tmp->toalPaperName[i]);
			assert(0);
		}	
		
		#if 1
		//3. ��ȡ�޸ĺ��ļ��ľ���·����
		sprintf(fileNameDst, "%s%s", quehandle->TemplateFileDir, tmp->toalPaperName[i]);
		sprintf(systemBuf, "mv %s %s",fileNameSrc, fileNameDst);		//�����ڵ�ԭʼ�ļ����޸�Ϊ��Ҫ���ļ���		
		if((pox_system((const char *)systemBuf)) < 0)
		{
			myprint("Error : system() : %s", systemBuf);
			assert(0);
		}
		memset(fileNameDst, 0, sizeof(fileNameDst));
		memset(systemBuf, 0, sizeof(systemBuf));
		memset(fileNameSrc, 0, sizeof(fileNameSrc));
		#endif
	}	
		

}



int Tape_template_sendData(ipcHandle *quehandle, unsigned int offset)
{
	int ret = 0;
	char buf[128] = { 0 };
	int cmd = 0xA1;
	
	modify_file_style(quehandle, offset);
	
	if((ret = scan_send_msgque(cmd, unsigned_int_to_hex(offset, buf), quehandle)) < 0)
	{
		socket_log(SocketLevel[4], -1, "Err : func scan_send_msgque() "); 
		printf("Err : func scan_send_msgque() [%d], [%s]\n", __LINE__, __FILE__);
		return ret;
	}

	return ret;
}

int destroy_IPC_QueAndShm(ipcHandle *listHandle)
{
	int ret = 0;

	if( NULL == listHandle )
	{
		socket_log(SocketLevel[2], -1, "NULL == listHandle || NULL == shmHandle"); 
		printf("NULL == listHandle  [%d], [%s]\n", __LINE__, __FILE__);
		return ret = -1;
	}

	if( (ret = scan_close_msgque(listHandle)) < 0)
	{
		socket_log(SocketLevel[2], 0, "Err : func scan_close_msgque()"); 
		printf("Err : func scan_close_msgque() [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}


	if((ret = shmory_pool_destroy(listHandle->shmPool)) < 0)
	{
		socket_log(SocketLevel[2], 0, "Err : func shmory_pool_destroy()"); 
		printf("Err : func shmory_pool_destroy() [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}

End:
	
	return ret;
}


