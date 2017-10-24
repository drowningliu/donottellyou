#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


#include "IPC_shmory.h"
#include "socklog.h"
#include "template_scan_element_type.h"
#include "msg_que_scan.h"
#include "scan_ipc.h"



pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int send_template_msg(ipcHandle *queHandle );


int call_back_commmon_pack(void *arg, void *shmHandle)
{
	int ret = 0;
	printf("recv data : %s [%d], [%s]\n", (char *)arg, __LINE__, __FILE__);
	return ret;
}


//���߳�ִ���߼� : ��������,��ִ�лص�����
void *child_thread(void *arg)
{
	pthread_detach(pthread_self());					//�������߳�
	
	ipcHandle *queHandle = (ipcHandle *)arg;			//��Ϣ���о��
	int  ret = 0;

	while(1)
	{
		ret = scan_recv_msgque(queHandle);
		if(ret == -1)
		{
			socket_log(SocketLevel[2], ret, "Err : func scan_recv_msgque()");
			printf("Err : func scan_recv_msgque() [%d], [%s]\n", __LINE__, __FILE__);
			goto End;
		} 		
		else if(ret == -2)
		{
			socket_log(SocketLevel[2], 0, "The opssite IPC queue closed !!!"); 
			printf("The opssite IPC queue closed !!![%d], [%s]\n", __LINE__, __FILE__);
			goto End;
		}
	}

End:	
	return NULL;
}



int main()
{
	int ret = 0, i = 0;
	ipcHandle *queHandle = NULL;		//��Ϣ���о��		
	pthread_t thread;					//���̱߳�ʶ��


	int getC;		//���ս�������, ���ж���ѡ��
		
	//1.��ʼ��
	ret = init_scan_ipc( 0x1234, 0x1236, &queHandle, "./", &g_mutex);
	if(ret < 0)
	{
		printf("Err : func init_scan_ipc() [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	//3.ע��ص���������Ӧ�Ļص�����
	if(insert_node(queHandle, 0x21, call_back_commmon_pack) < 0)
	{
		ret = -1;
		printf("Err : func insert_node() [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}
	if(insert_node(queHandle, 88, call_back_commmon_pack) < 0)
	{
		ret = -1;
		printf("Err : func insert_node() [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}

	//3.�������߳�, ���̷߳�������, ���߳̽�������,��ִ�лص�����	
	ret = pthread_create(&thread, NULL, child_thread, queHandle);
	if(ret < 0)
	{
		printf("Err : func pthread_create() [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}

	int j = 0;
	//4.���߳�ִ���߼�
	while(1)
	{
		myprint("1 : send template Data; 3 : destroy IPC object, 4 : send common file");
		getC = getchar();
		switch (getC)
		{
			case 49: 
				for(j = 0; j < 100; j++)
				{
					if((ret = send_template_msg(queHandle)) < 0)				
					{
						printf("Err : func send_msg() [%d], [%s]\n", __LINE__, __FILE__);				
						break;
					}				
					usleep(1000000);
				}
				break;
			case 50:
				if((ret = scan_send_msgque(49,"haha", queHandle)) < 0)				
					printf("Err : func send_msg() [%d], [%s]\n", __LINE__, __FILE__);				
				break;
			case 51:
				if(destroy_IPC_QueAndShm(queHandle) < 0)
					printf("Err : func destroy() [%d], [%s]\n", __LINE__, __FILE__);
				break;
			case 52:
				while(i < 20)
				{
					if(create_file_send_filepath(queHandle, "IMAGE0-0-0-0.jpg", "hello9", 6) < 0)
					{
						printf("Err : func destroy() [%d], [%s]\n", __LINE__, __FILE__);
						break;								
					}
					i++;
				}
				i = 0;
				break;
			default	:
				printf("Err : The getchar is not find [%d], [%s]\n", __LINE__, __FILE__);
		}
		if(getC == 51)		break;
		getchar();
	}

	
	
End:
	if(ret == -1)		//-1: ��ʶ��ʼ���ɹ���������, ��Ҫ�ͷŵ���Դ. -2: ��ʼ��ʧ��,����Ҫ�ͷ���Դ
	{
		if(destroy_IPC_QueAndShm(queHandle ) < 0)
		{
			printf("Err : func destroy() [%d], [%s]\n", __LINE__, __FILE__);
		}		
	}
	
	
	return ret;
}



int send_template_msg022(ipcHandle *queHandle)
{
	int ret = 0;	
	unsigned int offset = 0;			//����Ĺ����ڴ���ƫ����
	char *allocTmp = NULL;//�����ڴ�����ʱ��ַ
	
	if(queHandle == NULL )
	{
		printf("Err : listHandle == NULL || shmHandle == NULL [%d], [%s]\n", __LINE__, __FILE__);
		ret = -1;
		goto End;
	}
						 
	if( (allocTmp = (char *)alloc_shmory_block(queHandle, &offset)) == NULL)
	{
		socket_log(SocketLevel[2], -1, "Err : func insert_node() call_back_free_shmory_block"); 
		printf("Err : func pthread_create() [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}	

	memset(allocTmp, 49, sizeof(templateScanElement));
//	memcpy(allocTmp, "124548713115452144dqewbchabcuqwgyuqwde", 18);
	
	ret = Tape_template_sendData(queHandle, offset);
	if(ret < 0)
	{
		printf("Err : func Tape_template_sendData() [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}

End:
	
	return ret;
}


int send_template_msg(ipcHandle *queHandle)
{
	int ret = 0;	
	char *fileName = "IMAGE0.jpg";
	Coords coorTmp = {21, 12, 12, 25, 25}; 
	char *objAnswer = "A~ABC~ABD~D~";
	unsigned int offset = 0;			//����Ĺ����ڴ���ƫ����
	templateScanElement *allocTmp = NULL;//�����ڴ�����ʱ��ַ

	
	if(queHandle == NULL )
	{
		printf("Err : listHandle == NULL || shmHandle == NULL [%d], [%s]\n", __LINE__, __FILE__);
		ret = -1;
		goto End;
	}
					 
	if( (allocTmp = (templateScanElement*)alloc_shmory_block(queHandle, &offset)) == NULL)
	{
		socket_log(SocketLevel[2], -1, "Err : func insert_node() call_back_free_shmory_block"); 
		printf("Err : func pthread_create() [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}	
	
	memcpy(allocTmp->identifyID, "124548713qwdeqw115452144", 20);
	memcpy(allocTmp->objectAnswer, objAnswer, strlen(objAnswer));
	allocTmp->totalNumberPaper = 1;
	memcpy(allocTmp->toalPaperName[0], fileName, strlen(fileName) );
	allocTmp->subMapCoordNum = 2; 
	memcpy(&(allocTmp->coordsDataToUi[0]) , &coorTmp, sizeof(Coords));
	memcpy(&(allocTmp->coordsDataToUi[1]), &coorTmp, sizeof(Coords));
	
	//myprint("offset : %d, allocTmp : %p", offset , allocTmp);
	ret = Tape_template_sendData(queHandle, offset);
	if(ret < 0)
	{
		printf("Err : func Tape_template_sendData() [%d], [%s]\n", __LINE__, __FILE__);
		goto End;
	}

End:
	
	return ret;
}


