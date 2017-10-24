#include "escape_char.h"
#include "socklog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//对输入的数据进行转义
int  escape(char sign, char *inData, int inDataLenth, char *outData, int *outDataLenth)
{
	int  ret = 0, i = 0;
	//socket_log( SocketLevel[2], ret, "func escape() begin");
	if (NULL == inData || inDataLenth <= 0)
	{
		socket_log( SocketLevel[4], ret, "Error: NULL == inData || inDataLenth <= 0");
		ret = -1;
		goto End;
	}

	//printf("log------------------------1------------------\r\n");
	char *tmpInData = inData;
	char *tmpOutData = outData;
	int  lenth = 0;
	
	for (i = 0; i < inDataLenth; i++)
	{
		if (*tmpInData == sign)
		{
			*tmpOutData = 0x7d;
			tmpOutData += 1;
			*tmpOutData = 0x02;
			lenth += 2;
		}
		else if (*tmpInData == 0x7d)
		{
			*tmpOutData = 0x7d;
			tmpOutData += 1;
			*tmpOutData = 0x01;
			lenth += 2;
		}
		else
		{
			*tmpOutData = *tmpInData;
			lenth += 1;
		}
		tmpOutData += 1;
		tmpInData++;
		
	}

	*outDataLenth = lenth;

End:
	//socket_log( SocketLevel[2], ret, "func escape() end");
	return ret;
}

//对输入的数据进行解转义,  该函数中都是主调函数分配内存
int  anti_escape(char *inData, int inDataLenth, char *outData, int *outDataLenth)
{
	int  ret = 0, i = 0;
	
	//socket_log( SocketLevel[2], ret, "func anti_escape() begin");
	if (NULL == inData || inDataLenth <= 0)
	{
		socket_log( SocketLevel[4], ret, "Error: inData : %p, || inDataLenth : %d", inData, inDataLenth);
		ret = -1;
		goto End;
	}

	
	char *tmpInData = inData;
	char *tmp = ++inData;
	char *tmpOutData = outData;
	int  lenth = 0;
	
	for (i = 0; i < inDataLenth ; i++)
	{
		if (*tmpInData == 0x7d && *tmp == 0x01)
		{
			*tmpOutData = 0x7d;
			++tmpInData;
			++tmp;
			lenth += 1;
			i++;
		}
		else if (*tmpInData == 0x7d && *tmp == 0x02)
		{
			*tmpOutData = 0x7e;
			++tmpInData;
			++tmp;
			lenth += 1;
			i++;
		}
		else
		{
			*tmpOutData = *tmpInData;	
			lenth += 1;		
		}
		++tmpOutData;
		++tmpInData;
		++tmp;
		
	}
	
	

	*outDataLenth = lenth;
End:
	//socket_log( SocketLevel[2], ret, "func anti_escape() end");
	return ret;
}
