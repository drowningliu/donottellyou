
#include "virtualscannersvr.h"
#include <thread>
#include <vector>
#include <direct.h>
#include <io.h>
#include "virtualscannersvrmain.h"

namespace DROWNINGLIU
{

	namespace VSCANNERSVRLIB
	{
		//全局变量
		char 			g_DirPath[512] = { 0 };						//上传文件路径
		unsigned int 	g_recvSerial = 0;							//服务器 接收的 数据包流水号
		unsigned int 	g_sendSerial = 0;							//服务器 发送的 数据包流水号
		long long int 	g_fileTemplateID = 562, g_fileTableID = 365;//服务器资源文件ID 
		char 			*g_downLoadDir = NULL;						//服务器资源路径
		char 			*g_fileNameTable = "downLoadTemplateNewestFromServer_17_05_08.pdf";	//数据库表名称
		char 			*g_fileNameTemplate = "C_16_01_04_10_16_10_030_B_L.jpg";			//模板名称
	//	RecvAndSendthid_Sockfd		g_thid_sockfd_block[NUMBER];	//客户端信息
		char 			*g_sendFileContent = NULL;					//读取每轮下载模板的数据

																	//散列算法 : crc32 校验
		static const int crc32tab[] = {
			0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
			0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
			0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
			0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
			0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
			0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
			0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
			0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
			0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
			0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
			0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
			0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
			0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
			0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
			0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
			0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
			0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
			0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
			0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
			0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
			0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
			0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
			0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
			0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
			0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
			0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
			0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
			0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
			0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
			0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
			0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
			0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
			0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
			0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
			0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
			0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
			0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
			0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
			0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
			0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
			0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
			0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
			0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
			0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
			0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
			0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
			0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
			0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
			0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
			0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
			0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
			0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
			0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
			0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
			0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
			0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
			0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
			0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
			0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
			0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
			0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
			0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
			0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
			0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
		};

		/*crc32校验
		*@param : buf  数据内容
		*@param : size 数据长度
		*@retval: 返回生成的校验码
		*/
		int crc326(const  char *buf, uint32_t size)
		{
			int i, crc;
			crc = 0xFFFFFFFF;
			for (i = 0; i < size; i++)
				crc = crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
			return crc ^ 0xFFFFFFFF;
		}

		bool if_file_exist(const char *filePath)
		{
			if (filePath == NULL)
				return false;

			if (_access(filePath, 0) == 0)
				return true;

			return false;
		}

		//自实现 system 系统命令
		int pox_system(const char *cmd_line)
		{
			int ret = 0;
			//sighandler_t old_handler;
			//old_handler = signal(SIGCHLD, SIG_DFL);
			//ret = system(cmd_line);
			//signal(SIGCHLD, old_handler);
			//if (ret == 127)	ret = -1;
			return ret;
		}

		//保存图片
		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
		int save_multiPicture_func(char *recvBuf, int recvLen)
		{
			char *tmp = NULL;
			int lenth = 0, ret = 0;
			char TmpfileName[100] = { 0 };
			char fileName[100] = { 0 };
			char cmdBuf[1024] = { 0 };
			static FILE *fp = NULL;
			reqPackHead_t *tmpHead = NULL;
			int nWrite = 0;				//已经写的数据长度
			int perWrite = 0; 			//每次写入的数据长度

			//0. 获取数据包信息
			tmpHead = (reqPackHead_t *)recvBuf;
			lenth = recvLen - sizeof(reqPackHead_t) - sizeof(int) - FILENAMELENTH - sizeof(char) * 2;
			tmp = recvBuf + sizeof(reqPackHead_t) + FILENAMELENTH + sizeof(char) * 2;
			//myprint("picDataLenth : %d, currentIndex : %d, totalNumber : %d, recvLen : %d ...", lenth, tmpHead->currentIndex, tmpHead->totalPackage, recvLen);

			if (tmpHead->currentIndex != 0 && tmpHead->currentIndex + 1 < tmpHead->totalPackage)
			{
				//1.本图片的中间数据包, 将 指针指向 图片内容的部分, 将数据写入文件
				while (nWrite < lenth)
				{
					if ((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
					{
						//myprint("Err : func fopen() : fileName : %s !!!", fileName);
						goto End;
					}
					nWrite += perWrite;
				}
			}
			else if (tmpHead->currentIndex == 0)
			{
				//2.本图片的第一包数据, 获取文件名称
				memcpy(TmpfileName, recvBuf + sizeof(reqPackHead_t) + sizeof(char) * 2, 46);
				sprintf(fileName, "%s/%s", g_DirPath, TmpfileName);

				//3.查看文件是否存在, 存在即删除	
				if (if_file_exist(fileName))
				{
					sprintf(cmdBuf, "rm %s", fileName);
					if (pox_system(cmdBuf) < 0)
					{
						//myprint("Err : func pox_system()");
						ret = -1;
						goto End;
					}
				}

				//4.创建文件, 向文件中写入内容
				if ((fp = fopen(fileName, "ab")) == NULL)
				{
					//myprint("Err : func fopen() : fileName : %s !!!", fileName);
					ret = -1;
					goto End;
				}

				//5.将 指针指向 图片内容的部分, 将数据写入文件
				while (nWrite < lenth)
				{
					if ((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
					{
						//("Err : func fopen() : fileName : %s !!!", fileName);
						goto End;
					}
					nWrite += perWrite;
				}
			}
			else if (tmpHead->currentIndex + 1 == tmpHead->totalPackage)
			{
				//6.本图片的最后一包数据		
				while (nWrite < lenth)
				{
					if ((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
					{
						//myprint("Err : func fopen() : fileName : %s !!!", fileName);
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

			if (ret < 0)
			{
				if (fp)			fclose(fp);		fp = NULL;
			}

			return ret;
		}

		//保存图片
		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
		int save_picture_func(char *recvBuf, int recvLen)
		{
			char *tmp = NULL;
			int lenth = 0, ret = 0;
			char TmpfileName[100] = { 0 };
			char fileName[100] = { 0 };
			char cmdBuf[1024] = { 0 };
			static FILE *fp = NULL;
			reqPackHead_t *tmpHead = NULL;
			int nWrite = 0;				//已经写的数据长度
			int perWrite = 0; 			//每次写入的数据长度


			//0. 获取数据包信息
			tmpHead = (reqPackHead_t *)recvBuf;
			lenth = recvLen - sizeof(reqPackHead_t) - sizeof(int) - FILENAMELENTH;
			tmp = recvBuf + sizeof(reqPackHead_t) + FILENAMELENTH;
			//myprint("picDataLenth : %d, currentIndex : %d, totalNumber : %d, recvLen : %d ...", lenth, tmpHead->currentIndex, tmpHead->totalPackage, recvLen);

			if (tmpHead->currentIndex != 0 && tmpHead->currentIndex + 1 < tmpHead->totalPackage)
			{
				//1.本图片的中间数据包, 将 指针指向 图片内容的部分, 将数据写入文件

				while (nWrite < lenth)
				{
					if ((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
					{
						//myprint("Err : func fopen() : fileName : %s !!!", fileName);
						goto End;
					}
					nWrite += perWrite;
				}
			}
			else if (tmpHead->currentIndex == 0)
			{
				//2.本图片的第一包数据, 获取文件名称
				memcpy(TmpfileName, recvBuf + sizeof(reqPackHead_t), 46);
				sprintf(fileName, "%s/%s", g_DirPath, TmpfileName);

				//3.查看文件是否存在, 存在即删除	
				if (if_file_exist(fileName))
				{
					sprintf(cmdBuf, "rm %s", fileName);
					if (pox_system(cmdBuf) < 0)
					{
						//myprint("Err : func pox_system()");
						ret = -1;
						goto End;
					}
				}

				//4.创建文件, 向文件中写入内容
				if ((fp = fopen(fileName, "ab")) == NULL)
				{
					//myprint("Err : func fopen() : fileName : %s !!!", fileName);
					ret = -1;
					goto End;
				}

				//5.将 指针指向 图片内容的部分, 将数据写入文件
				while (nWrite < lenth)
				{
					if ((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
					{
						//myprint("Err : func fopen() : fileName : %s !!!", fileName);
						goto End;
					}
					nWrite += perWrite;
				}
			}
			else if (tmpHead->currentIndex + 1 == tmpHead->totalPackage)
			{
				//6.本图片的最后一包数据		
				while (nWrite < lenth)
				{
					if ((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
					{
						//myprint("Err : func fopen() : fileName : %s !!!", fileName);
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

			if (ret < 0)
			{
				if (fp)			fclose(fp);		fp = NULL;
			}

			return ret;
		}

		/*比较数据包接收是否正确
		*@param : tmpContent		数据包转义后的数据
		*@param : tmpContentLenth	转义后数据的长度, 包含: 报头 + 信息 + 校验码
		*@retval: success 0, repeat -1, NoRespond -2
		*/
		int compare_recvData_correct_server(char *tmpContent, int tmpContentLenth)
		{
			int ret = 0;
			int checkCode = 0;					//本地计算校验码
			int *tmpCheckCode = NULL;			//缓存服务器的校验码
			uint8_t cmd = 0;					//接收数据包命令
			reqPackHead_t  *tmpHead = NULL;		//请求报头
			reqTemplateErrHead_t  *Head = NULL;	//下载模板请求报头

			//1.获取接收数据包的命令字
			cmd = *((uint8_t *)tmpContent);

			//2.进行命令字的选择
			if (cmd != DOWNFILEREQ)
			{
				//3. 获取接收数据的报头信息
				tmpHead = (reqPackHead_t *)tmpContent;

				//4. 数据包长度比较
				if (tmpHead->contentLenth + sizeof(int) == tmpContentLenth)
				{
					//5.长度正确, 进行流水号比较, 去除重包			
					if (tmpHead->seriaNumber >= g_recvSerial)
					{
						//6.流水号正确,进行校验码比较
						tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
						checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
						if (checkCode == *tmpCheckCode)
						{
							//7.校验码正确, 整个数据包正确
							g_recvSerial += 1;		//流水号往下移动
							ret = 0;
						}
						else
						{
							//8.校验码错误, 数据包需要重发
							/*myprint("Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
								cmd, tmpHead->seriaNumber, checkCode, *tmpCheckCode);*/
							ret = -1;
						}
					}
					else
					{
						//9.流水号出错, 不需要重发
						/*myprint("Err : cmd : %d, package serial : %u, loacl ServerSerial : %u",
							cmd, tmpHead->seriaNumber, g_recvSerial);*/
						ret = -2;
					}
				}
				else
				{
					//10.数据长度出错, 不需要重发
					/*myprint("Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u",
						cmd, tmpHead->contentLenth + sizeof(int), tmpContentLenth, tmpHead->seriaNumber);*/
					ret = -2;
				}
			}
			else
			{
				Head = (reqTemplateErrHead_t *)tmpContent;

				//11.数据包长度比较
				if (Head->contentLenth + sizeof(int) == tmpContentLenth)
				{
					//12.数据包长度正确, 比较流水号, 去除重包
					if (g_recvSerial == Head->seriaNumber)
					{
						//13.流水号正确, 进行校验码比较
						g_recvSerial += 1;		//流水号往下移动
												//14.流水号正确,进行校验码比较
						tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
						checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
						if (*tmpCheckCode == checkCode)
						{
							//15.校验码正确, 这个数据包正确
							ret = 0;
						}
						else
						{
							//16.校验码出错, 需要重发数据包						
							/*socket_log(SocketLevel[4], ret, "Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
								cmd, Head->seriaNumber, checkCode, *tmpCheckCode);*/
							ret = -1;
						}
					}
					else
					{
						//17.流水号出错, 不需要重发
						/*socket_log(SocketLevel[3], ret, "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u",
							cmd, Head->seriaNumber, g_recvSerial);*/
						ret = -2;
					}
				}
				else
				{
					//18.数据长度出错, 不需要重发
					/*socket_log(SocketLevel[3], ret, "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u",
						cmd, Head->contentLenth + sizeof(int), tmpContentLenth, Head->seriaNumber);*/
					ret = -2;
				}
			}

			return ret;
		}

		/*获取文件分包的总包数和文件总大小
		*@param : filePath 文件路径
		*@param : number 计算出的分包数
		*@param : contentLenth 分包的标准大小
		*@param : fileSizeSrc 文件总大小
		*@retval: 成功 0; 失败 -1;
		*/
		int get_file_size(const char *filePath, int *number, int contentLenth, unsigned long *fileSizeSrc)
		{
			int ret = 0;
			struct stat statbuff;
			unsigned long filesize = 0;
			int remainder = 0;			//余数

			if (!filePath)
			{
				ret = -1;
				//socket_log(SocketLevel[4], ret, "Error: filePath == NULL");
				return -1;
			}

			//1.获取文件大小
			if (stat(filePath, &statbuff) < 0)
			{
				ret = -1;
				//socket_log(SocketLevel[4], ret, "Error: func stat() %s", strerror(errno));
				return -1;
			}
			else
				filesize = statbuff.st_size;

			//2.计算分包大小
			if (number)
			{
				if ((remainder = filesize % contentLenth) > 0)
					*number = filesize / contentLenth + 1;
				else
					*number = filesize / contentLenth;
			}

			//3.获取文件大小
			if (fileSizeSrc)
				*fileSizeSrc = filesize;

			return ret;
		}

		//对输入的数据进行解转义,  该函数中都是主调函数分配内存
		int  anti_escape(const char *inData, int inDataLenth, char *outData, int *outDataLenth)
		{
			int  ret = 0, i = 0;

			//socket_log( SocketLevel[2], ret, "func anti_escape() begin");
			if (NULL == inData || inDataLenth <= 0)
			{
				//	socket_log(SocketLevel[4], ret, "Error: inData : %p, || inDataLenth : %d", inData, inDataLenth);
				ret = -1;
				goto End;
			}

			const char *tmpInData = inData;
			const char *tmp = ++inData;
			char *tmpOutData = outData;
			int  lenth = 0;

			for (i = 0; i < inDataLenth; i++)
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

		//对输入的数据进行转义
		int  escape(char sign, const char *inData, int inDataLenth, char *outData, int *outDataLenth)
		{
			int  ret = 0, i = 0;
			//socket_log( SocketLevel[2], ret, "func escape() begin");
			if (NULL == inData || inDataLenth <= 0)
			{
				//socket_log(SocketLevel[4], ret, "Error: NULL == inData || inDataLenth <= 0");
				ret = -1;
				goto End;
			}

			//printf("log------------------------1------------------\r\n");
			const char *tmpInData = inData;
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

		void assign_serverSubPack_head(resSubPackHead_t *head, uint8_t cmd, int contentLenth, int currentIndex, int totalNumber)
		{
			memset(head, 0, sizeof(resSubPackHead_t));
			head->cmd = cmd;
			head->contentLenth = contentLenth;
			head->totalPackage = totalNumber;
			head->currentIndex = currentIndex;
			head->seriaNumber = g_sendSerial++;
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

		//模板图片集合 数据包	13号数据包应答		
		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
		int VirtualScannerSession::upload_template_set_reply(char *recvBuf, int recvLen, int index)
		{
			int  ret = 0, outDataLenth = 0; 				//发送数据的长度
			int  checkNum = 0;								//校验码
			char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
			char *sendBufStr = data_.data();						//发送数据地址
			char *tmp = NULL;
			resCommonHead_t head;							//应答数据报头
			reqPackHead_t  *tmpHead = NULL; 				//请求报头信息
			static bool latePackFlag = false;				//true : 本模板最后一包图片,最后一个数据包, false 非最后一包数据,  
			static int recvSuccessSubNumber = 0;			//每轮成功接收的数据包数目


			//1.打印接收图片的信息
			//printf_comPack_news(recvBuf); 				

			//3.获取报头信息
			tmpHead = (reqPackHead_t *)recvBuf;

			//4.进行请求信息类型判断
			if (tmpHead->isSubPack == 1)
			{
				//5.子包, 进行数据包信息判断
				if ((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
				{
					//6.数据正确, 进行图片数据的存储, 记录本轮成功接收的子包数量
					recvSuccessSubNumber += 1;
#if 1
					if ((ret = save_multiPicture_func(recvBuf, recvLen)) < 0)
					{
						printf("error: func save_picture_func() !!! [%d], [%s] \n", __LINE__, __FILE__);
						goto End;
					}
#endif
					tmp = recvBuf + sizeof(reqPackHead_t);
					if (tmpHead->currentIndex + 1 == tmpHead->totalPackage && *tmp + 1 == *(tmp + 1))
					{
						latePackFlag = true;
					}

					if (recvSuccessSubNumber < tmpHead->perRoundNumber)
					{
						//7.标识每轮成功接收到的子包数目小于发送数目, 不需要进行进行回复				
						ret = 0;
						goto End;
					}
					else if (recvSuccessSubNumber == tmpHead->perRoundNumber)
					{
						//8.标识每轮所有的子包成功接收, 进行判断该回复哪种应答(该图片成功接收应答还是本轮成功接收应答)
						recvSuccessSubNumber = 0;
						if (latePackFlag == false)
						{
							//本轮成功接收					
							outDataLenth = sizeof(resCommonHead_t);
							assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);
						}
						else if (latePackFlag == true)
						{
							//所有图片都成功接收
							latePackFlag = false;
							tmp = tmpSendBuf + sizeof(resCommonHead_t);
							*tmp++ = 0x00;
							outDataLenth = sizeof(resCommonHead_t) + 1;
							assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 1);
						}
					}

				}
				else if (ret == -1)
				{
					//9.需要重发			
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);
				}
				else
				{
					//10.不需要重发
					ret = 0;
					goto End;
				}
			}
			else
			{
				//11.总包, 立即回应.
				outDataLenth = sizeof(resCommonHead_t);
				assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
			}

			//12.申请内存
			/*if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
			{
				myprint("Err : func mem_pool_alloc()");
				goto End;
			}*/

			//13. 拷贝报头数据, 计算校验码
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			tmp = tmpSendBuf + head.contentLenth;
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//14. 转义数据内容
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				//myprint("Error : func escape() escape() !!!!");
				goto End;
			}

			//15. 组合发送的内容
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			nLen2Write_ = outDataLenth + 2;
			/*if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, MUTIUPLOADCMDRESPON)) < 0)
			{
				myprint("Err : func push_queue_send_block()");
				goto End;
			}

			sem_post(&(g_thid_sockfd_block[index].sem_send));*/

		End:
			//if (ret < 0 && sendBufStr)		mem_pool_free(g_memoryPool, sendBufStr);

			return ret;
		}

		//模板扩展数据包 数据包			12 号数据包应答
		/*@param : recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);
		*@param :  recvLen : 长度为 buf 的内容 长度
		*@param :  sendBuf : 发送 client 的数据内容
		*@param :  sendLen : 发送 数据内容的长度
		*/
		int VirtualScannerSession::template_extend_element_reply(char *recvBuf, int recvLen, int index)
		{
			int ret = 0, outDataLenth = 0;				//发送数据包长度
			resCommonHead_t head;						//发送数据报头
			char *tmp = NULL, *sendBufStr = data_.data();		//发送地址
			unsigned int contentLenth = 0;				//发送数据包长度
			int checkNum = 0;							//校验码
			char tmpSendBuf[1500] = { 0 };				//临时缓冲区

			//1. 打印接收图片的信息, 申请内存
			/*printf_template_news(recvBuf);

			if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
			{
				ret = -1;
				myprint("Err : func mem_pool_alloc()");
				goto End;
			}*/

			//2. 组包消息头
			contentLenth = sizeof(resCommonHead_t) + sizeof(char) * 1;
			assign_comPack_head(&head, TEMPLATECMDRESPON, contentLenth, NOSUBPACK, 0, 0, 1);

			//3. 组包消息体, 按接收成功处理
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			*tmp++ = 0;

			//4. 组合校验码
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//5. 转义数据内容
			if ((ret = escape(0x7e, tmpSendBuf, contentLenth + sizeof(uint32_t), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//6. 组合发送的内容
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;
			
			nLen2Write_ = outDataLenth + 2;
			/*if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, TEMPLATECMDRESPON)) < 0)
			{
				myprint("Err : func push_queue_send_block()");
				goto End;
			}

			sem_post(&(g_thid_sockfd_block[index].sem_send));*/
		End:
		//	if (ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);

			return ret;
		}

		//删除图片应答 数据包			9 号数据包
		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
		int VirtualScannerSession::delete_func_reply(char *recvBuf, int recvLen, int index)
		{
			int ret = 0, outDataLenth = 0;					//发送数据包长度
			int checkNum = 48848748;						//校验码
			char tmpSendBuf[1500] = { 0 };					//临时缓冲区
			char *tmp = NULL, *sendBufStr = data_.data();			//发送数据包地址
			char fileName[128] = { 0 };						//文件名称
			char rmFilePath[512] = { 0 };					//删除命令
			resCommonHead_t head;							//应答数据报头

			//1. 打印数据包内容
			//printf_comPack_news(recvBuf);

			//2. 申请内存
			/*if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
			{
				ret = -1;
				myprint("Err : func mem_pool_alloc()");
				goto End;
			}*/

			//3. 获取文件信息, 并删除
			memcpy(fileName, recvBuf + sizeof(reqPackHead_t), recvLen - sizeof(reqPackHead_t) - sizeof(int));
			sprintf(rmFilePath, "rm %s/%s", g_DirPath, fileName);
			ret = system(rmFilePath);
			//myprint("rmFilePath : %s", rmFilePath);
			
			//4. 组装数据包信息
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			if (ret < 0 || ret == 127)
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

			//5. 组装报头
			assign_comPack_head(&head, DELETECMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));

			//6. 组合校验码
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//7. 转义数据内容
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				//printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//6. 组合发送的内容
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			nLen2Write_ = outDataLenth + 2;
			/*if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, DELETECMDRESPON)) < 0)
			{
				myprint("Err : func push_queue_send_block()");
				goto End;
			}

			sem_post(&(g_thid_sockfd_block[index].sem_send));*/

		End:
			//if (ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);

			return ret;
		}

		int push_info_toUi_reply(char *recvBuf, int recvLen, int index)
		{
			//myprint("push infomation OK ... ");
			return 0;
		}

		//上传图片应答 数据包;  服务器收一包 回复一包.
		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
		int VirtualScannerSession::upload_func_reply(char *recvBuf, int recvLen, int index)
		{
			int  ret = 0, outDataLenth = 0;					//发送数据的长度
			int  checkNum = 0;								//校验码
			char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
			char *sendBufStr = data_.data();						//发送数据地址
			char *tmp = NULL;
			resCommonHead_t head;							//应答数据报头
			reqPackHead_t  *tmpHead = NULL;					//请求报头信息
			static bool latePackFlag = false;					//true : 图片最后一包, false 非最后一包数据,  
			static int recvSuccessSubNumber = 0;				//每轮成功接收的数据包数目

			//1.打印接收图片的信息
			//printf_comPack_news(recvBuf);					

			//3.获取报头信息
			tmpHead = (reqPackHead_t *)recvBuf;

			//4.进行请求信息类型判断
			if (tmpHead->isSubPack == 1)
			{
				//5.子包, 进行数据包信息判断
				if ((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
				{
					//6.数据正确, 进行图片数据的存储, 记录本轮成功接收的子包数量
					recvSuccessSubNumber += 1;
#if 1
					if ((ret = save_picture_func(recvBuf, recvLen)) < 0)
					{
						printf("error: func save_picture_func() !!! [%d], [%s] \n", __LINE__, __FILE__);
						goto End;
					}
#endif
					if (tmpHead->currentIndex + 1 == tmpHead->totalPackage)
					{
						latePackFlag = true;
					}

					if (recvSuccessSubNumber < tmpHead->perRoundNumber)
					{
						//7.标识每轮成功接收到的子包数目小于发送数目, 不需要进行进行回复				
						ret = 0;
						goto End;
					}
					else if (recvSuccessSubNumber == tmpHead->perRoundNumber)
					{
						//8.标识每轮所有的子包成功接收, 进行判断该回复哪种应答(该图片成功接收应答还是本轮成功接收应答)
						recvSuccessSubNumber = 0;
						if (latePackFlag == false)
						{
							//本轮成功接收					
							outDataLenth = sizeof(resCommonHead_t);
							assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);
						}
						else if (latePackFlag == true)
						{
							//该图片成功接收
							latePackFlag = false;
							tmp = tmpSendBuf + sizeof(resCommonHead_t);
							*tmp++ = 0x01;
							outDataLenth = sizeof(resCommonHead_t) + 1;
							assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 1);
						}
					}

				}
				else if (ret == -1)
				{
					//9.需要重发			
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);
				}
				else
				{
					//10.不需要重发
					ret = 0;
					goto End;
				}
			}
			else
			{
				//11.总包, 立即回应.
				outDataLenth = sizeof(resCommonHead_t);
				assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
			}

			//12.申请内存
			/*if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
			{
				myprint("Err : func mem_pool_alloc()");
				goto End;
			}*/

			//12. 拷贝报头数据, 计算校验码
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			tmp = tmpSendBuf + head.contentLenth;
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//13. 转义数据内容
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				//myprint("Error : func escape() escape() !!!!");
				goto End;
			}

			//14. 组合发送的内容
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			nLen2Write_ = outDataLenth + 2;
			/*if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, UPLOADCMDRESPON)) < 0)
			{
				myprint("Err : func push_queue_send_block()");
				goto End;
			}

			sem_post(&(g_thid_sockfd_block[index].sem_send));*/


		End:
			//if (ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);

			return ret;
		}

		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符); 
		//recvLen : 长度为 buf 的内容 长度
		int VirtualScannerSession::get_FileNewestID_reply(char *recvBuf, int recvLen, int index)
		{
			int  ret = 0, outDataLenth = 0; 				//发送数据的长度
			int  checkNum = 0;								//校验码
			char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
			char *sendBufStr = data_.data();						//发送数据地址
			char *tmp = NULL;
			resCommonHead_t head;							//应答数据报头
			int  fileType = 0;								//请求的文件类型


			//1. 打印数据包信息
			//printf_comPack_news(recvBuf);	

			//1. 获取文件类型
			tmp = recvBuf + sizeof(reqPackHead_t);
			if (*tmp == 0)				fileType = 0;
			else if (*tmp == 1)			fileType = 1;
			else						assert(0);

			//myprint("fileType : %d", fileType);

			//2. 申请内存
			/*if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
			{
				ret = -1;
				myprint("Err : func mem_pool_alloc()");
				goto End;
			}*/

			//3. 组装数据包信息
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			if (fileType == 0)
			{
				*tmp++ = fileType;
				memcpy(tmp, &g_fileTemplateID, sizeof(long long int));
			}
			else if (fileType == 1)
			{
				*tmp++ = fileType;
				memcpy(tmp, &g_fileTableID, sizeof(long long int));
			}
			outDataLenth = sizeof(resCommonHead_t) + 65;

			//4. 组装报头信息	
			assign_comPack_head(&head, NEWESCMDRESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));

			//5. 计算校验码
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			tmp += 64;
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//6. 转义数据内容
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//7. 组合发送的内容
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			nLen2Write_ = outDataLenth + 2;

			/*if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, NEWESCMDRESPON)) < 0)
			{
				myprint("Err : func push_queue_send_block()");
				goto End;
			}*/

			//sem_post(&(g_thid_sockfd_block[index].sem_send));

		End:
			//if (ret < 0)		if (sendBufStr)		mem_pool_free(g_memoryPool, sendBufStr);

			return ret;
		}

		//下载模板数据包	4号数据包应答		
		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码  
		//recvLen : 长度为 buf 的内容 长度
		int VirtualScannerSession::downLoad_template_func_reply(char *recvBuf, int recvLen, int index)
		{
			int ret = 0, outDataLenth = 0;
			resSubPackHead_t 	head;					//应答数据包报头
			char *tmp = NULL, *sendBufStr = NULL;// data_.data();
			int contentLenth = 0;
			int checkNum = 0, nRead = 0;				//校验码和 读取字节数
			char tmpSendBuf[1400] = { 0 };				//临时缓冲区
			int  doucumentLenth = 0;
			FILE *fp = NULL;
			char fileName[1024] = { 0 };
			int totalPacka = 0;							//该文件的总包数
			int indexNumber = 0;						//每包的序号
			int fileType = 0;							//下载的文件类型		
			int  tmpLenth = 0;

			//1. 打印接收数据包信息
			//printf_comPack_news(recvBuf);

			//2. get The downLoad file Type And downLoadFileID
			tmp = recvBuf + sizeof(reqPackHead_t);
			if (*tmp == 0)			sprintf(fileName, "%s%s", g_downLoadDir, g_fileNameTemplate);		//下载模板
			else if (*tmp == 1)		sprintf(fileName, "%s%s", g_downLoadDir, g_fileNameTable);			//下载数据库表
			else					assert(0);
			fileType = *tmp;
			tmp++;
			//myprint("downLoadFileName : %s", fileName);

			//3. open The template file
			if ((fp = fopen(fileName, "rb+")) == NULL)
			{
				ret = -1;
				//myprint("Err : func fopen() : %s", fileName);
				goto End;
			}

			//4. get The file Total Package Number 
			doucumentLenth = RESTEMSUBBODYLENTH - sizeof(char) * 3;
			if ((ret = get_file_size(fileName, &totalPacka, doucumentLenth, NULL)) < 0)
			{
				//myprint("Err : func get_file_size()");
				goto End;
			}

			//5. read The file content	
			while (!feof(fp))
			{
				type_data_t	data;

				sendBufStr = data.data();

				//6. get The part of The file conTent	And package The body News
				memset(tmpSendBuf, 0, sizeof(tmpSendBuf));
				tmp = tmpSendBuf + sizeof(resSubPackHead_t);
				*tmp++ = 0;
				*tmp++ = 2;
				*tmp++ = fileType;

				while (nRead < doucumentLenth && !feof(fp))
				{
					if ((tmpLenth = fread(tmp + nRead, 1, doucumentLenth - nRead, fp)) < 0)
					{
						//myprint("Error: func fread() : %s ", fileName);
						ret = -1;
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
				checkNum = crc326(tmpSendBuf, contentLenth);
				memcpy(tmpSendBuf + contentLenth, &checkNum, sizeof(int));

				//5.4 allock The block buffer in memory pool
				/*if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
				{
					ret = -1;
					myprint("Error: func mem_pool_alloc()");
					goto End;
				}*/

				//5.6 escape the buffer
				*sendBufStr = PACKSIGN;							 //flag 
				if ((ret = escape(PACKSIGN, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
				{
					//myprint("Error: func escape()");
					goto End;
				}

				*(sendBufStr + outDataLenth + 1) = PACKSIGN; 	 //flag 

				//nLen2Write_ = outDataLenth + sizeof(char) * 2;

				{
					std::lock_guard<std::mutex> lock(_mtxData);
					
					//_deqData.push_back(std::make_pair(outDataLenth + sizeof(char) * 2, std::move(data)));
					_deqData.push_back(std::make_tuple(DOWNFILEREQ, outDataLenth + sizeof(char) * 2, std::move(data)));
				}

				//5.7 push The sendData addr and sendLenth in queuebuf
				/*if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + sizeof(char) * 2, DOWNFILEZRESPON)) < 0)
				{
					myprint("Error: func push_queue_send_block()");
					fclose(fp);
					mem_pool_free(g_memoryPool, sendBufStr);
					return ret;
				}*/

				nRead = 0;
				//sem_post(&(g_thid_sockfd_block[index].sem_send));
			}


		End:
			if (fp)		fclose(fp);
			return ret;
		}

		//退出登录应答 数据包
		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
		int VirtualScannerSession::exit_func_reply(char *recvBuf, int recvLen, int index)
		{
			int  ret = 0, outDataLenth = 0;					//发送数据的长度
			int  checkNum = 0;								//校验码
			char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
			char *sendBufStr = data_.data();						//发送数据地址
			char *tmp = NULL;
			resCommonHead_t head;							//应答数据报头

			//1. 打印数据包信息		
			//printf_comPack_news(recvBuf);			//打印接收图片的信息

													//2. 申请内存
			/*if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
			{
				ret = -1;
				myprint("Err : func mem_pool_alloc()");
				goto End;
			}*/

			//3. 数据信息拷贝
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			*tmp++ = 0x01;
			outDataLenth = sizeof(resCommonHead_t) + 1;

			//4. 组装报头
			assign_comPack_head(&head, LOGOUTCMDRESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));

			//5. 计算校验码
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//6. 转义数据内容
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//7. 组合发送的内容
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			nLen2Write_ = outDataLenth + 2;
			/*if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, LOGOUTCMDRESPON)) < 0)
			{
				myprint("Err : func push_queue_send_block()");
				goto End;
			}

			sem_post(&(g_thid_sockfd_block[index].sem_send));*/

		End:
			//if (ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);

			return ret;
		}

		//登录应答数据包
		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
		int VirtualScannerSession::login_func_reply(char *recvBuf, int recvLen, int index)
		{
			int  ret = 0, outDataLenth = 0;					//发送数据的长度
			int  checkNum = 0;								//校验码
			char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
			//char sendBufStr[2800] = { 0 };						//发送数据地址
			char *sendBufStr = data_.data();
			char *tmp = NULL;
			resCommonHead_t head;							//应答数据报头
			reqPackHead_t *tmpHead = NULL;					//请求信息
			char *userName = "WuKong";						//返回账户的用户名

															//1.打印接收图片的信息
			//printf_comPack_news(recvBuf);
			tmpHead = (reqPackHead_t *)recvBuf;
			g_recvSerial = tmpHead->seriaNumber;
			g_recvSerial++;

			////2.申请内存
			//if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
			//{
			//	ret = -1;
			//	myprint("Err : func mem_pool_alloc()");
			//	goto End;
			//}

			//3.组装数据包信息, 获取发送数据包长度
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			*tmp++ = 0x01;				//登录成功
			*tmp++ = 0x00;				//成功后, 此处信息不会进行解析, 随便写
			*tmp++ = strlen(userName);	//用户名长度
			memcpy(tmp, userName, strlen(userName));	//拷贝用户名	
			outDataLenth = sizeof(resCommonHead_t) + sizeof(char) * 3 + strlen(userName);

			//4.组装报头信息		
			assign_comPack_head(&head, LOGINCMDRESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			//myprint("outDataLenth : %d, head.contentLenth : %d", outDataLenth, head.contentLenth);

			//5.组合校验码
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			tmp = tmpSendBuf + head.contentLenth;
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//6. 转义数据内容
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//7. 组合发送的内容
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			nLen2Write_ = outDataLenth + 2;
			//if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, LOGINCMDRESPON)) < 0)
			{
				//myprint("Err : func push_queue_send_block()");
				goto End;
			}

			//sem_post(&(g_thid_sockfd_block[index].sem_send));

		End:
			//if (ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);

			return ret;
		}

		//接收客户端的数据包, recvBuf: 包含: 标识符 + 报头 + 报体 + 校验码 + 标识符;   recvLen : recvBuf数据的长度
		//				sendBuf : 处理数据包, 将要发送的数据;  sendLen : 发送数据的长度
		int VirtualScannerSession::read_data_proce(char *recvBuf, int recvLen, int index, int &nCmd)
		{
			int ret = 0;
			uint8_t  cmd = 0;

			//1.获取命令字
			cmd = *((uint8_t *)recvBuf);
			nCmd = cmd;

			//2.选择处理方式
			switch (cmd) {
			case LOGINCMDREQ: 		//登录数据包
				if ((ret = login_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func login_func_reply() ");
				break;
			case LOGOUTCMDREQ: 		//退出数据包
				if ((ret = exit_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func exit_func_reply() ");
				break;
			//case HEARTCMDREQ: 		//心跳数据包
			//	if ((ret = data_un_packge(heart_beat_func_reply, recvBuf, recvLen, index)) < 0)
			//		myprint("Error: func heart_beat_func_reply() ");
			//	break;
			case DOWNFILEREQ: 		//模板下载			
				if ((ret = downLoad_template_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func downLoad_template_func_reply() ");
				break;
			case NEWESCMDREQ: 		//客户端信息请求
				if ((ret = get_FileNewestID_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func client_request_func_reply() ");
				break;
			case UPLOADCMDREQ: 		//上传图片数据包
				if ((ret = upload_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func upload_func_reply() ");
				break;
			case PUSHINFOREQ:		//消息推送
				if ((ret = push_info_toUi_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func push_info_toUi_reply() ");
				break;
			case DELETECMDREQ: 		//删除图片数据包
				if ((ret = delete_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func delete_func_reply() ");
				break;
			case TEMPLATECMDREQ:		//上传模板
				if ((ret = template_extend_element_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func template_extend_element() ");
				break;
			case MUTIUPLOADCMDREQ:		//上传图片集
				if ((ret = upload_template_set_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func upload_template_set ");
				break;
#if 0	
			case ENCRYCMDREQ:
				if ((ret = data_un_packge(encryp_or_cancelEncrypt_transfer, recvBuf, recvLen, index)) < 0)
					myprint("Error: func encryp_or_cancelEncrypt_transfer_reply ");
				break;
#endif			
			default:
				//myprint("recvCmd : %d", cmd);
				assert(0);
			}


			return ret;
		}

		//function: find  the whole packet; retval maybe error
		int	VirtualScannerSession::find_whole_package(const char *recvBuf, int recvLenth, int index, int &nCmd)
		{
			int ret = 0, i = 0;
			int  tmpContentLenth = 0;			//转义后的数据长度
			char tmpContent[1400] = { 0 }; 		//转义后的数据
			int flag = 0;				//flag 标识找到标识符的数量;
			int lenth = 0; 				//lenth : 缓存的数据包内容长度
			char content[2800] = { 0 };	//缓存完整包数据内容, 		
			char *tmp = NULL;


			tmp = content + lenth;
			for (i = 0; i < recvLenth; i++)
			{
				//1. 查找数据报头
				if (recvBuf[i] == 0x7e && flag == 0)
				{
					flag = 1;
					continue;
				}
				else if (recvBuf[i] == 0x7e && flag == 1)
				{
					//return 1;
					
					//2. 查找数据报尾, 获取到完整数据包, 将数据包进行反转义 
					if ((ret = anti_escape(content, lenth, tmpContent, &tmpContentLenth)) < 0)
					{
						//myprint("Err : func anti_escape(), srcLenth : %d, dstLenth : %d", lenth, tmpContentLenth);
						goto End;
					}

					reqPackHead_t	&req = *(reqPackHead_t *)tmpContent;

					//3.将转义后的数据包进行处理
					if ((ret = read_data_proce(tmpContent, tmpContentLenth, index, nCmd)) < 0)
					{
						//myprint("Err : func read_data_proce(), dataLenth : %d", tmpContentLenth);
						goto End;
					}
					else
					{
						//4. 数据包处理成功, 清空临时变量
						memset(content, 0, sizeof(content));
						memset(tmpContent, 0, sizeof(tmpContent));
						lenth = 0;
						flag = 0;
						tmp = content + lenth;
					}

				}
				else
				{
					//5.取出数据包内容, 去除起始标识符
					*tmp++ = recvBuf[i];
					lenth += 1;
					continue;
				}
			}


		End:

			return ret;
		}

		size_t read_complete(const boost::asio::const_buffer &buffer, const boost::system::error_code & ec, size_t bytes) {
			if (ec) 
				return 0;

			//至少要收满标志位+包头
			if (bytes < sizeof(reqPackHead_t) + 1)
				return sizeof(reqPackHead_t) + 1 - bytes;

			const char *pBuf = static_cast<const char *>(boost::asio::detail::buffer_cast_helper(buffer));

			int  tmpContentLenth = 0;			//转义后的数据长度
			char tmpContent[1400] = { 0 }; 		//转义后的数据

			//2. 跳过第一个标志位， 将数据包进行反转义 
			int ret = 0;
			if ((ret = anti_escape(pBuf + 1, bytes - 1, tmpContent, &tmpContentLenth)) < 0)
			{
				//myprint("Err : func anti_escape(), srcLenth : %d, dstLenth : %d", lenth, tmpContentLenth);
				return 0;
			}

			//继续收满包内容、校验和（int）、结束标志
			const reqPackHead_t &req = *(reqPackHead_t *)(tmpContent);
			int contLen = req.contentLenth;
			if (contLen == 0)
				return 0;

			int all = 1 + contLen + sizeof(int) + 1;
			if (bytes < all)
				return all -bytes;
			else
				return 0;
		}

		void VirtualScannerSession::do_read()
		{
			auto self(shared_from_this());
#if IF_USE_ASYNC
			socket_.async_read_some(boost::asio::buffer(data_),
#else
			boost::asio::async_read(socket_, boost::asio::buffer(data_),
				std::bind(read_complete, boost::asio::buffer(data_), std::placeholders::_1, std::placeholders::_2),
#endif
				make_custom_alloc_handler(allocator_,
					[this, self](boost::system::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					//assert(typeid(socket_.get_io_service()) == typeid(boost::asio::stream_socket_service<tcp>));
#if IF_USE_ASYNC			
					auto service = (boost::asio::stream_socket_service<tcp> *)&socket_.get_io_service();
					
#if defined(BOOST_ASIO_HAS_IOCP)
					boost::asio::detail::win_iocp_socket_service<tcp>::implementation_type t;
#else
					boost::asio::detail::reactive_socket_service<tcp>::implementation_type t;
#endif
					service->native_handle(t);

					do
					{
						std::lock_guard<std::mutex> lock(_mtxSock2Data);
						if (_mapSock2Data.size() < 100)
							break;

						//消息满了，等待 or just close socket
						std::cout << "_mapSock2Data满了\n";

					} while (std::this_thread::sleep_for(std::chrono::seconds(1)), true);

					//先缓存，考虑封装成一个mgr类
					{
						std::lock_guard<std::mutex> lock(_mtxSock2Data);
						auto itr = _mapSock2Data.find(t.socket_);
						if (itr == std::end(_mapSock2Data))
						{
							deq_data_t	deq;

							deq.push_back(std::make_pair(MSG_TYPE::Read_MSG, data_));
							_mapSock2Data[t.socket_] = std::move(deq);
						}
						else
						{
							auto &deq = _mapSock2Data[t.socket_];
						/*	std::for_each(std::begin(deq), std::end(deq), [](auto data) {

							});*/

							deq.push_back(std::make_pair(MSG_TYPE::Read_MSG, data_));
						}
					}

					bool bNeedRead = false;
					//parse data
					
					bNeedRead = true;
					if(bNeedRead)
					{
						//没收完整，继续收
						do_read();
					}
					else
						do_write(length);
#else
					int cmd = 0;
					//解析包内容，并写入缓存。
					if (0 > find_whole_package(data_.data(), length, 0, cmd))
						return;//do_read();

					if(cmd == DOWNFILEREQ)
					{
						//int nSockFD = getSocketFD();
						int nlen2write = 0;

						////只有自己的socket，因为buffer是跟session、socket绑定的。
						//std::lock_guard<std::mutex> lock(_mtxSock2Data);
						//auto itr = _mapSock2Data.find(nSockFD);
						//if (itr == std::end(_mapSock2Data))
						//	return;	//结束，没有可写的了

						/*auto &data = itr->second.second;
						nlen2write = itr->second.first;
						data_ = data;
						_mapSock2Data.erase(itr);*/

						//std::lock_guard<std::mutex> lock(_mtxData);
						
						//这里投递多次无效吧？或者说buffer拷贝走的太多了，能保证顺序吗？
						//或者这里投递一次，在do_write里继续投递一次，循环。
						//那索性这里投递0，统一在dowrite中投递一次
					/*	while (!_deqData.empty())
						{
							auto &pair = _deqData.front();

							nlen2write = pair.first;
							auto &data = pair.second;
							data_.swap(data);

							nLen2Write_ = nlen2write;

							_deqData.pop_front();

							do_write(nLen2Write_);
						}*/
						//cmd_ = DOWNFILEREQ;
						do_multiwrite(0);
						//do_read();
					}
					else
						do_write(nLen2Write_);
#endif
				}
			}));
		}

		void VirtualScannerSession::do_multiwrite(std::size_t length)
		{
			//boost::asio::async_write
			auto self(shared_from_this());
			boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
				make_custom_alloc_handler(allocator_,
					[this, self](boost::system::error_code ec, std::size_t length2)
			{
				if (!ec)
				{
					//assert(typeid(socket_.get_io_service()) == typeid(boost::asio::stream_socket_service<tcp>));
					//if (/*length2 == 0 && */cmd_ == DOWNFILEREQ)
					{
						int nlen2write = 0;
						std::lock_guard<std::mutex> lock(_mtxData);

						//dowrite中，每写完一个再投递下一个，似乎还是无法保证顺序
						//莫非要用其他方法

						//这里投递多次无效吧？
						//while (!_deqData.empty())
						if (!_deqData.empty())
						{
							auto &t = _deqData.front();

							int cmd = std::get<0>(t);
							nlen2write = std::get<1>(t);
							auto &data = std::get<2>(t);
							data_.swap(data);

							nLen2Write_ = nlen2write;

							_deqData.pop_front();

							do_multiwrite(nLen2Write_);
						}
						else
						{
							do_read();
						}
					}
				}
			}));
		}

		void VirtualScannerSession::do_write(std::size_t length)
		{
			//boost::asio::async_write
			auto self(shared_from_this());
			boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
				make_custom_alloc_handler(allocator_,
					[this, self](boost::system::error_code ec, std::size_t length2)
			{
				if (!ec)
				{
					//assert(typeid(socket_.get_io_service()) == typeid(boost::asio::stream_socket_service<tcp>));
					if (/*length2 == 0 && */cmd_ == DOWNFILEREQ)
					{
						int nlen2write = 0;
						std::lock_guard<std::mutex> lock(_mtxData);

						//dowrite中，每写完一个再投递下一个，似乎还是无法保证顺序
						//莫非要用其他方法

						//这里投递多次无效吧？
						//while (!_deqData.empty())
						if (!_deqData.empty())
						{
							auto &t = _deqData.front();

							int cmd = std::get<0>(t);
							nlen2write = std::get<1>(t);
							auto &data = std::get<2>(t);
							data_.swap(data);

							nLen2Write_ = nlen2write;

							_deqData.pop_front();

							do_write(nLen2Write_);
						}
					}
					else
						do_read();
				}
			}));
		}

		int VirtualScannerSession::push_info_toUi(int index, int fileType)
		{
			int ret = 0, outDataLenth = 0, checkNum = 0;
			char *tmp = NULL, *sendBufStr = data_.data();
			resCommonHead_t head;
			char tmpSendBuf[1400] = { 0 };

			//myprint("The queue have element number : %d ...", get_element_count_send_block(g_sendData_addrAndLenth_queue));

			//2.alloc The memory block
			/*if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
			{
				ret = -1;
				myprint("Err : func mem_pool_alloc()");
				goto End;
			}*/

			//3.package body news
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			if (fileType == 0)
			{
				*tmp++ = fileType;
				memcpy(tmp, &g_fileTemplateID, sizeof(long long int));
				g_fileTemplateID++;
			}
			else if (fileType == 1)
			{
				*tmp++ = fileType;
				memcpy(tmp, &g_fileTableID, sizeof(long long int));
				g_fileTableID++;
			}
			outDataLenth = sizeof(resCommonHead_t) + 65;

			//4.package The head news	
			assign_comPack_head(&head, PUSHINFORESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			//g_thid_sockfd_block[index].send_flag++;

			//5.calculation checkCode
			tmp = tmpSendBuf + head.contentLenth;
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//6. 转义数据内容
			ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
			if (ret)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//7. 组合发送的内容
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			nLen2Write_ = outDataLenth + 2;
			/*if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, PUSHINFORESPON)) < 0)
			{
				myprint("Err : func push_queue_send_block()");
			}

			sem_post(&(g_thid_sockfd_block[index].sem_send));*/

		End:
			//if (ret < 0) 	if (sendBufStr)		mem_pool_free(g_memoryPool, sendBufStr);

			return ret;
		}

		void find_directory_file()
		{
			char *direct = NULL;
			struct dirent *entry = NULL;
			struct stat  fileStat;
			static bool flag = false;
			int roundNum = 0, conIndex = 0, filyType = 0;
			char tableName[256] = { 0 };
			char templateName[256] = { 0 };
			char tmp01[256] = { 0 }, tmp02[256] = { 0 };

			struct FileModifyTime {
				char fileName[256];
				time_t modifyTime;
			};

			struct FileList {
				char fileName[256];
				struct FileList *next;
			}start, *node, *tmpNode;

			static struct FileModifyTime fileModifyTime[2];

#if 1
			//1.获取目录
			if ((direct = getenv("SERverSourceDir")) == NULL)
			{
				//myprint("Err : func getenv() key = SERverSourceDir");
				assert(0);
			}
			g_downLoadDir = direct;
#endif
			auto getFiles = [](const std::string &path, std::vector<struct _finddata_t> &files)
			{
				struct _finddata_t c_file;
				intptr_t		hFile;
				char			buf[512] = { 0 };

				if (_chdir(path.c_str()))
				{
					printf("Unable to locate the directory: %s\n", path.c_str());
					return -1;
				}
				else
					hFile = _findfirst("*.*", &c_file);

				/* List the files... */
				/*printf("Listing of files in the directory %s\n\n", path);
				printf("\nRDO HID SYS ARC  FILE                            DATE %20c SIZE\n", ' ');
				printf("--- --- --- ---  ----                             ---- %20c ----\n", ' ');
				printf((c_file.attrib & _A_RDONLY) ? " Y  " : " N  ");
				printf((c_file.attrib & _A_SYSTEM) ? " Y  " : " N  ");
				printf((c_file.attrib & _A_HIDDEN) ? " Y  " : " N  ");
				printf((c_file.attrib & _A_ARCH) ? " Y  " : " N  ");*/
				// unsafe version is ctime() 
				ctime_s(buf, sizeof(buf), &(c_file.time_write));
				//printf(" %-30s %.20s  %9ld\n", c_file.name, buf, c_file.size);
				if (strcmp(c_file.name, ".") != 0 && strcmp(c_file.name, "..") != 0)
					files.push_back(c_file);

				/* Find the rest of the files */
				while (_findnext(hFile, &c_file) == 0)
				{
					/*printf((c_file.attrib & _A_RDONLY) ? " Y  " : " N  ");
					printf((c_file.attrib & _A_SYSTEM) ? " Y  " : " N  ");
					printf((c_file.attrib & _A_HIDDEN) ? " Y  " : " N  ");
					printf((c_file.attrib & _A_ARCH) ? " Y  " : " N  ");*/
					// unsafe version is ctime() 

					if (strcmp(c_file.name, ".") != 0 && strcmp(c_file.name, "..") != 0)
						files.push_back(c_file);

					ctime_s(buf, sizeof(buf), &(c_file.time_access));
					//printf(" %-30s %.20s  %9ld\n", c_file.name, buf, c_file.size);

					c_file.name;

				}
				
				_findclose(hFile);
				return 0;
			};

			std::vector<struct _finddata_t> files;
			getFiles(g_downLoadDir, files);

			sprintf(tableName, "%s%s", g_downLoadDir, g_fileNameTable);
			sprintf(templateName, "%s%s", g_downLoadDir, g_fileNameTemplate);

			std::for_each(std::begin(files), std::end(files), [](auto f) {
				
			});
			//4.遍历获取的目录下的文件, 获取它们的时间属性
			node = start.next;
			while (node)
			{
				//4.3 初次打开该目录, 拷贝各文件的属性至缓存
				if (!flag)
				{
					fileModifyTime[roundNum].modifyTime = fileStat.st_atime;
					strcpy(fileModifyTime[roundNum].fileName, node->fileName);

					conIndex = 0;
					while (conIndex < NUMBER)
					{
						if (g_thid_sockfd_block[conIndex].flag == 1)
						{
							if ((strcmp(fileModifyTime[roundNum].fileName, templateName)) == 0)
								push_info_toUi(conIndex, 0);
							else if ((strcmp(fileModifyTime[roundNum].fileName, tableName)) == 0)
								push_info_toUi(conIndex, 1);
						}
						conIndex++;
					}

				}
				else		//非首次, 进行属性判断
				{
					if ((strlen(fileModifyTime[roundNum].fileName)) == 0)
					{
						strcpy(fileModifyTime[roundNum].fileName, node->fileName);
						fileModifyTime[roundNum].modifyTime = fileStat.st_atime;
						if ((strcmp(fileModifyTime[roundNum].fileName, templateName)) == 0)
						{
							filyType = 0;
						}
						else if (strcmp(fileModifyTime[roundNum].fileName, tableName) == 0)
						{
							filyType = 1;
						}
						else
						{
							assert(0);
						}
						conIndex = 0;
						while (conIndex < NUMBER)
						{
							if (g_thid_sockfd_block[conIndex].flag == 1)		push_info_toUi(conIndex, filyType);
							conIndex++;
						}

					}
					else
					{
						if ((strcmp(fileModifyTime[roundNum].fileName, templateName)) == 0 && fileModifyTime[roundNum].modifyTime != fileStat.st_atime)
						{
							filyType = 0;
						}
						else if ((strcmp(fileModifyTime[roundNum].fileName, tableName)) == 0 && fileModifyTime[roundNum].modifyTime != fileStat.st_atime)
						{
							filyType = 1;
						}
						else
						{
							goto End;
						}
						conIndex = 0;
						while (conIndex < NUMBER)
						{
							if (g_thid_sockfd_block[conIndex].flag == 1) 	push_info_toUi(conIndex, filyType);
							conIndex++;
						}

						fileModifyTime[roundNum].modifyTime = fileStat.st_atime;
					}

				}
			End:
				roundNum++;
				
			}
			flag = true;
		}

		//select() 定时器 
		void timer_select(int seconds)
		{
			struct timeval temp;
			temp.tv_sec = seconds;
			temp.tv_usec = 0;
			int err = 0;
			do {
				err = select(0, NULL, NULL, NULL, (struct timeval *)&temp);
			} while (err < 0 && errno == EINTR);

		}

		void *child_scan_func(void *arg)
		{
			//pthread_detach(pthread_self());

			while (1)
			{
				timer_select(SCAN_TIME);
				find_directory_file();
			}

			//pthread_exit(NULL);
		}

		int VirtualScannerSvr::init()
		{
			int	ret = 0;

			//1.查看上传目录是否存在
			if (_access(UPLOADDIRPATH, 0) < 0)
			{
				//2.不存在, 创建保存上传文件的目录
				if ((ret = _mkdir(UPLOADDIRPATH/*, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH*/)) < 0)
				{
					//myprint("Error : func mkdir() : %s", uploadDirParh);
					//assert(0);
					return ret;
				}
			}

			//3.进行路径处理
			memcpy(g_DirPath, UPLOADDIRPATH, strlen(UPLOADDIRPATH));
			//myprint("uploadFilePath : %s", g_DirPath);

			//8.获取扫描路径	
			if ((g_downLoadDir = getenv("SERverSourceDir")) == NULL)
			{
				//myprint("Err : func getenv() key = SERverSourceDir");
				//assert(0);
			}

			//创建子线程， 去执行扫描目录业务, 进行信息推送
			std::thread t(child_scan_func);

			return 0;
		}

		GA2NDIDSVRLIB_API int main1(int argc, char* argv[])
		{
			try
			{
				if (argc != 2)
				{
					std::cerr << "Usage: server <port>\n";
					return 1;
				}
				
				boost::asio::io_service	io_service;

				/*wchar_t * pEnd;
				long int port;
				port = std::wcstol(argv[1], &pEnd, 10);*/
				int port = std::atoi(argv[1]);
				VirtualScannerSvr		server(io_service, port);

				std::vector<std::thread>	vctThreads;
				int	nThreadCount = 1;

				auto handle_thread = [&]()
				{
					io_service.run();
				};

				for (int i = 0; i < nThreadCount; ++i)
					vctThreads.push_back(std::thread(handle_thread));

				std::for_each(std::begin(vctThreads), std::end(vctThreads), [](auto &t) {t.join(); });

			}
			catch (std::exception& e)
			{
				std::cerr << "Exception: " << e.what() << "\n";
			}

			return 0;
		}
	}

	GA2NDIDSVRLIB_API int main2(int argc, wchar_t* argv[])
	{

		return 0;
	}
}





