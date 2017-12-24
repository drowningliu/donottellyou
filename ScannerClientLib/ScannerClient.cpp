#include <WinSock2.h>
#include <winbase.h>
#include "ScannerClient.h"
#include "TEMPLATE_SCAN_ELEMENT_TYPE.h"

namespace DROWNINGLIU
{
	namespace SCANNERCLIENTLIB
	{
		//宏定义
#define DIVISIONSIGN 			'~'
#define ACCOUTLENTH 			11							//客户端账户长度
#define CAPACITY    			110							//队列, 链表的容量
#define WAIT_SECONDS 			2							//链接服务器的超时时间
#define HEART_TIME				3							//心跳时间
#undef 	BUFSIZE
#define BUFSIZE					250							//命令队列的 节点大小
#define DOWNTEMPLATENAME		"NHi1200_TEMPLATE_NAME"		//The env for downtemplate PATHKEY
#define DOWNTABLENAME           "NHi1200_DATABASE_NAME"
#define DOWNLOADPATH			"NHi1200_TEMPLATE_DIR"
#define CERT_FILE 				"/home/yyx/nanhao/client/client-cert.pem" 
#define KEY_FILE   				"/home/yyx/nanhao/client/client-key.pem" 
#define CA_FILE  				"/home/yyx/nanhao/ca/ca-cert.pem" 
#define CIPHER_LIST	 			"AES128-SHA"
#define	MY_MIN(x, y)			((x) < (y) ? (x) : (y))	
#define TIMEOUTRETURNFLAG		-5							//超时返回值

		char 				*g_machine = "21212111233";			//机器码
		unsigned int 		g_seriaNumber = 0;					//发送数据包的序列号
		unsigned int	 	g_recvSerial = 0;					//当前接收服务器主动发送数据包的序列号

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

		void assign_reqPack_head(reqPackHead_t *head, uint8_t cmd, int contentLenth, int isSubPack, int currentIndex, int totalNumber, int perRoundNumber)
		{
			memset(head, 0, sizeof(reqPackHead_t));
			head->cmd = cmd;
			head->contentLenth = contentLenth;
			strcpy((char *)head->machineCode, g_machine);
			head->isSubPack = isSubPack;
			head->totalPackage = totalNumber;
			head->currentIndex = currentIndex;
			head->perRoundNumber = perRoundNumber;
			//pthread_mutex_lock(&g_mutex_serial);
			head->seriaNumber = g_seriaNumber++;
			//pthread_mutex_unlock(&g_mutex_serial);
		}

		//对输入的数据进行转义
		int  escape(char sign, char *inData, int inDataLenth, char *outData, int *outDataLenth)
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

		int ScannerClient::init()
		{
			//发送线程
			auto do_write = [this]()
			{
				while (!_bStop)
				{
					bool hasData = false;
					{
						std::lock_guard<std::mutex> lock(_mtxSendData);

						hasData = !_deqSendData.empty();
						if (hasData)
						{
							//send data
							Sleep(100);

							std::string &data = _deqSendData.front();
							data.size();
							data.c_str();

							_deqSendData.pop_front();
#if 0
							if (nSendLen == tmpSendLenth)
							{
								//3.发送成功, 缓存当前发送的数据包信息, (主要缓存最后一包)
								g_lateSendPackNews.cmd = *sendCmd;
								g_lateSendPackNews.sendAddr = tmpSend;
								g_lateSendPackNews.sendLenth = tmpSendLenth;
								time(&(g_lateSendPackNews.sendTime));

								if (g_send_package_cmd == PUSHINFOREQ)
								{
									remove_senddataserver_addr();
									sem_post(&g_sem_send);	//服务器推送消息成功应答后,通知进行下一包数据的发送 
									goto End;
								}
							}
#endif
						}
					}
				}
			};

			//接收线程
			//接收数据线程
			auto do_read = [this]()
			{
				int ret = 0;
				int num = 0;

				//1.分离线程
				//pthread_detach(pthread_self());

				while (1)
				{
					//2.等待信号量通知, 已链接上服务器, 可以进行数据接收
					//sem_wait(&g_sem_read_open);

					while (1)
					{
						//3.接收数据
						if ((ret = recv_data()) < 0)
						{
							//4.接收数据出错
							if (_sockfd != INVALID_SOCKET)
							{
								closesocket(_sockfd);
								_sockfd = INVALID_SOCKET;
							}

							//5.主动关闭或销毁网络, 不需要进行出错回复
							if (g_close_net == 1 || g_close_net == 2)
							{
								clear_global(1);
								myprint("The UI active shutdown NET in read thread");
								break;
							}
							else if (g_close_net == 0)
							{

								//6.进行出错回复				
								do {
									if ((ret = error_net(1)) < 0)		//网络出错后, 给UI 端回复
									{
										//myprint("Error: func error_net()");
										Sleep(1000);
										num++;			//继续进行发送
									}
									else				//发送成功,
									{
										num = 0;
										break;
									}
								} while (num < 2);

								break;
							}
						}
					}
				}

				//pthread_exit(NULL);
			};

			_vctThreads.push_back(std::thread(do_write));
			_vctThreads.push_back(std::thread(do_read));

			int numsaw = PERROUNDNUMBER * COMREQDATABODYLENTH;
			if ((_sendFileContent = new char[numsaw]) == NULL)
				return -1;

			return 0;
		}

		unsigned int power(int x, int y)
		{
			unsigned int result = 1;
			while (y--)
				result *= x;
			return result;
		}

		unsigned int hex_conver_dec(char *src)
		{
			char *tmp = src;
			int len = 0;
			unsigned int hexad = 0;
			char sub = 0;

			while (1)
			{
				if (*tmp == '0')            //去除字符串 首位0
				{
					tmp++;
					continue;
				}
				else if (*tmp == 'X')
				{
					tmp++;
					continue;
				}
				else if (*tmp == 'x')
				{
					tmp++;
					continue;
				}
				else
				{
					break;
				}
			}

			len = strlen(tmp);
			for (; len > 0; len--)
			{
				sub = toupper(*tmp++) - '0';
				if (sub > 9)
					sub -= 7;
				hexad += sub * power(16, len - 1);
			}

			return hexad;
		}
		
		/*copy The news content in block buffer
		*@param : news cmd~content~;垄~
		*@retval: OK block buffer addr; fail NULL
		*/
		int ScannerClient::copy_the_ui_cmd_news(const std::string &msg, std::string &news)
		{
			news.clear();
			
			if (msg.empty())
				return -1;

			//1.find The flag in news
			size_t pos = msg.find('~');
			if (pos == std::string::npos)
				return -1;

			news = msg.substr(pos);
			if (news.size() < 1)
				return -1;

			//3.The buffer content conver 16 hex
			unsigned int	apprLen = hex_conver_dec((char *)news.c_str()+1);
			char			*tmp = (char *)NULL + apprLen;

			news.assign(tmp, sizeof(UitoNetExFrame));

			return 0;
		}

		int ScannerClient::recv_ui_data(const std::string &msg)
		{
			if (msg.empty())
				return -1;

			char	cmdBuf[32] = { 0 };
			char	storeNews[128] = { 0 };
			
			std::string news;

			//1. get The cmd  of packet in news.
			sscanf(msg.c_str(), "%[^~]", cmdBuf);
			int cmd = atoi(cmdBuf);

			//2.choose the cmd
			if (cmd == 0x0c)
			{
				if (0 > copy_the_ui_cmd_news(msg, news) || news.empty())
				{
					//myprint("Error : func copy_the_ui_cmd_news()");
					return -1;
				}

				sprintf(storeNews, "%d~%p~", 12, news.c_str());
				//tmpNews = storeNews;

				_deqMsg.push_back(storeNews);
			}
			else
			{
				//tmpNews = msg.c_str();
				_deqMsg.push_back(msg);
			}

			////4.通知命令线程,
			//if (ret == 0)
			//{
			//	//myprint("recv news : %s ", news);
			//	sem_post(&g_sem_business);
			//}

			return 0;
		}

		//登录数据包
		//"1~15934098456~121544~"
		int ScannerClient::login_func(const std::string &msg)
		{
			int ret = 0;
			char *tmp = NULL;
			
			uint8_t passWdSize = 0;
			reqPackHead_t head = { 0 };
			//char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
			int  checkCode = 0, contenLenth = 0;
			int outDataLenth = 0;

			//socket_log(SocketLevel[2], ret, "func login_func() begin: [%s]", news);

			//1.查找分割符
			int cmd = 0;
			char szAccount[64] = { 0 };
			char szPassword[64] = { 0 };

			if(3 != sscanf(msg.c_str(), "%d~%s~%s~", &cmd, szAccount, szPassword))
				return -1;

			passWdSize = strlen(szPassword);
		//	size_t pos = msg.find(DIVISIONSIGN);
		//	if (pos == std::string::npos)
		//		return -1;

		////	if ((tmp = strchr(news, '~')) == NULL)

		//	std::string accout = msg.substr(pos + 1, ACCOUTLENTH);
		//
		//	//tmp += ACCOUTLENTH;
		//	//if ((tmp = strchr(tmp, DIVISIONSIGN)) == NULL)

		//	pos = msg.find(DIVISIONSIGN, pos + 1 + ACCOUTLENTH);
		//	if (pos == std::string::npos)
		//		return -1;

		//	std::string passWd = msg.substr(pos + 1, ACCOUTLENTH);

		//	memcpy(passWd, tmp + 1, strlen(tmp) - 2);				//获取密码
		//	passWdSize = strlen(passWd);

			type_data_t data;
			char	*sendPackData = data.data();

			char	tmpSendPackDataOne[SEND_BUFFER] = { 0 };

			//2.申请内存
			//if ((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			//{
			//	//myprint("Error: func mem_pool_alloc()");
			//	//assert(0);
			//}
			//if ((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
			//{
			//	//myprint("Error: func mem_pool_alloc()");
			//	//assert(0);
			//}

			//3.计算数据包总长度, 并组装报头
			contenLenth = sizeof(reqPackHead_t) + ACCOUTLENTH + sizeof(char) + passWdSize;
			assign_reqPack_head(&head, LOGINCMDREQ, contenLenth, NOSUBPACK, 0, 1, 1);

			//4.组装数据包信息
			tmp = tmpSendPackDataOne;
			memcpy(tmp, (char *)&head, sizeof(reqPackHead_t));
			tmp += sizeof(reqPackHead_t);
			memcpy(tmp, szAccount, ACCOUTLENTH);
			tmp += ACCOUTLENTH;
			*tmp++ = passWdSize;
			memcpy(tmp, szPassword, passWdSize);

			//5.计算校验码
			checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
			tmp += passWdSize;
			memcpy(tmp, &checkCode, sizeof(int));

			//6.转义数据包内容
			*sendPackData = PACKSIGN;
			if ((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData + 1, &outDataLenth)) < 0)
			{
				//myprint("Error: func escape()");
				//assert(0);
			}

			*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;

			//7.释放内存至内存池
			/*if ((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
			{
				myprint("Error: func mem_pool_free()");
				assert(0);
			}*/
			{
				std::lock_guard<std::mutex> lock(_mtxSendData);
				_deqSendData.push_back(std::string(sendPackData, outDataLenth + sizeof(char) * 2));
			}
			/*if ((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, LOGINCMDREQ)) < 0)
			{
				myprint("Error: func push_queue_send_block()");
				assert(0);
			}
			if ((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, LOGINCMDREQ)) < 0)
			{
				myprint("Error: func Pushback_node_index_listSendInfo()");
				assert(0);
			}*/

			//sem_post(&g_sem_cur_write);	//发送信号量, 通知发送线程
			//sem_wait(&g_sem_send);

			//socket_log(SocketLevel[2], ret, "func login_func() end: [%s] %p", news, sendPackData);


			return ret;
		}

		//登出
		int ScannerClient::exit_program(const std::string &msg)
		{
			int ret = 0;
			reqPackHead_t head = { 0 };
			//char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
			int	checkCode = 0;
			int	outDataLenth = 0;

			//1.组报头
			assign_reqPack_head(&head, LOGOUTCMDREQ, sizeof(reqPackHead_t), NOSUBPACK, 0, 1, 1);

			type_data_t data;
			char	*sendPackData = data.data();

			char	tmpSendPackDataOne[SEND_BUFFER] = { 0 };
			//2.申请内存
			/*if ((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			{
				myprint("Error: func mem_pool_alloc()");
				assert(0);
			}
			if ((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
			{
				myprint("Error: func mem_pool_alloc()");
				assert(0);
			}*/

			//3.计算校验码
			memcpy(tmpSendPackDataOne, (char *)&head, sizeof(reqPackHead_t));
			checkCode = crc326((const  char*)tmpSendPackDataOne, head.contentLenth);
			memcpy(tmpSendPackDataOne + sizeof(reqPackHead_t), (char *)&checkCode, sizeof(uint32_t));

			//4.转义数据内容
			*sendPackData = PACKSIGN;
			if ((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData + 1, &outDataLenth)) < 0)
			{
				//myprint("Error: func escape()");
				//assert(0);
			}
			
			*(sendPackData + outDataLenth + 1) = PACKSIGN;

			//5.释放临时内存, 并将发送地址和数据长度放入队列
			/*if ((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
			{
				myprint("Error: func mem_pool_free()");
				assert(0);
			}*/
			{
				std::lock_guard<std::mutex> lock(_mtxSendData);
				_deqSendData.push_back(std::string(sendPackData, outDataLenth + sizeof(char) * 2));
			}
			/*if ((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, LOGOUTCMDREQ)) < 0)
			{
				myprint("Error: func push_queue_send_block()");
				assert(0);
			}
			if ((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, LOGOUTCMDREQ)) < 0)
			{
				myprint("Error: func Pushback_node_index_listSendInfo()");
				assert(0);
			}*/

			//sem_post(&g_sem_cur_write);	//发送信号量, 通知发送线程
			//sem_wait(&g_sem_send);


			return ret;
		}

		//下载模板文件, cmd~type~ID~
		int ScannerClient::download_file_fromServer(const std::string &msg)
		{
			int ret = 0;
			reqPackHead_t head = { 0 };
			//char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
			int  checkCode = 0;
			int outDataLenth = 0, contentLenth = 0;
			char *tmp = NULL, *tmpOut = NULL;
			char IdBuf[65] = { 0 };
			long long int fileid = 0;

			//1.find The selected content
			size_t pos = msg.find(DIVISIONSIGN);
			if (pos == std::string::npos)
				return -1;

			std::string news = msg.substr(pos);
			if (news.size() < 1)
				return -1;
			//if ((tmp = strchr(news, DIVISIONSIGN)) == NULL)
			//{
			//	//myprint("Error: func strstr() No find");
			//	//assert(0);
			//}
			tmp = (char *)news.c_str();
			tmp++;

			type_data_t data;
			char	*sendPackData = data.data();

			char	tmpSendPackDataOne[SEND_BUFFER] = { 0 };
			//2.alloc The memory block
			/*if ((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			{
				myprint("Error: func mem_pool_alloc()");
				assert(0);
			}
			if ((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
			{
				myprint("Error: func mem_pool_alloc()");
				assert(0);
			}*/

			//3.get The downLoad FileType And FileID
			tmpOut = tmpSendPackDataOne + sizeof(reqPackHead_t);
			if (*tmp == '0') 			
				*tmpOut++ = 0;
			else if (*tmp == '1')		
				*tmpOut++ = 1;
			else
				return -1;//assert(0);

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
			if ((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData + 1, &outDataLenth)) < 0)
			{
				//myprint("Error: func escape()");
				//assert(0);
			}
			*(sendPackData + outDataLenth + 1) = PACKSIGN;

			//8.free The tmp mempool block And push The sendData Addr and lenth in queue
			/*if ((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
			{
				myprint("Error: func mem_pool_free()");
				assert(0);
			}*/
			{
				std::lock_guard<std::mutex> lock(_mtxSendData);
				_deqSendData.push_back(std::string(sendPackData, outDataLenth + sizeof(char) * 2));
			}
			/*if ((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, DOWNFILEREQ)) < 0)
			{
				myprint("Error: func push_queue_send_block()");
				assert(0);
			}*/
			/*if ((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, DOWNFILEREQ)) < 0)
			{
				myprint("Error: func Pushback_node_index_listSendInfo()");
				assert(0);
			}

			sem_post(&g_sem_cur_write);
			sem_wait(&g_sem_send);*/

			return ret;
		}

		//Get The specify File type NewEst ID, cmd~type~
		int ScannerClient::get_FileNewestID(const std::string &msg)
		{
			int ret = 0;
			reqPackHead_t head = { 0 };
			//char *tmpSendPackDataOne = NULL, *sendPackData = NULL;
			int  checkCode = 0;
			int outDataLenth = 0, contentLenth = 0;
			char *tmp = NULL, *tmpOut = NULL;

			//1.find The selected content
			size_t pos = msg.find(DIVISIONSIGN);
			if (pos == std::string::npos)
				return -1;

			std::string news = msg.substr(pos);
			if (news.size() < 1)
				return -1;
			/*if ((tmp = strchr(news, DIVISIONSIGN)) == NULL)
			{
				myprint("Error: func strstr() No find");
				assert(0);
			}*/
			tmp = (char *)news.c_str();
			tmp++;

			type_data_t data;
			char	*sendPackData = data.data();

			char	tmpSendPackDataOne[SEND_BUFFER] = { 0 };

			//2.alloc The memory block
			/*if ((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			{
				myprint("Error: func mem_pool_alloc()");
				assert(0);
			}
			if ((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
			{
				myprint("Error: func mem_pool_alloc()");
				assert(0);
			}*/

			//3.get The downLoad FileType
			tmpOut = tmpSendPackDataOne + sizeof(reqPackHead_t);
			if (*tmp == '0')
				*tmpOut++ = 0;
			else if (*tmp == '1')
				*tmpOut++ = 1;
			else
				return -1;

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
			if ((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData + 1, &outDataLenth)) < 0)
			{
				//myprint("Error: func escape()");
				//assert(0);
			}

			*(sendPackData + outDataLenth + 1) = PACKSIGN;

			//8.free The tmp mempool block And push The sendData Addr and lenth in queue
			/*if ((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
			{
				myprint("Error: func mem_pool_free()");
				assert(0);
			}*/
			{
				std::lock_guard<std::mutex> lock(_mtxSendData);
				_deqSendData.push_back(std::string(sendPackData, outDataLenth + sizeof(char) * 2));
			}
			/*if ((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, NEWESCMDREQ)) < 0)
			{
				myprint("Error: func push_queue_send_block()");
				assert(0);
			}*/
			/*if ((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, NEWESCMDREQ)) < 0)
			{
				myprint("Error: func Pushback_node_index_listSendInfo()");
				assert(0);
			}*/

			//sem_post(&g_sem_cur_write);
			//sem_wait(&g_sem_send);

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
			int			ret = 0;
			struct stat	statbuff;
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

		//find The file name in The Absolute filepath
		int find_file_name(char *fileName, int desLenth, const char *filePath)
		{
			int ret = 0;
			const char *tmp = NULL;

			tmp = filePath + strlen(filePath);

			//linux : /
			while (*tmp != '/' && *tmp != '\\')
				--tmp;

			tmp += 1;
			desLenth > strlen(tmp) ? memcpy(fileName, tmp, strlen(tmp)) : memcpy(fileName, tmp, desLenth);

			return ret;
		}

		//每轮数据开始发送之前 的总包(不携带具体数据)
		int ScannerClient::send_perRounc_begin(int cmd, int totalPacka, int sendPackNumber)
		{
			reqPackHead_t  head;								//请求信息的报头
			//char *sendPackData = NULL;							//发送数据包地址
			//char tmpSendPackDataOne[PACKMAXLENTH] = { 0 };		//临时数据包地址长度
			int checkCode = 0;									//校验码
			char *tmp = NULL;
			int ret = 0, outDataLenth = 0;

			//1.组装报头
			assign_reqPack_head(&head, cmd, sizeof(reqPackHead_t), NOSUBPACK, 0, totalPacka, sendPackNumber);

			//2. 申请内存, 储存发送数据包
			/*if ((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
			{
				myprint("Error : func mem_pool_alloc()");
				assert(0);
			}*/

			type_data_t data;
			char	*sendPackData = data.data();

			char	tmpSendPackDataOne[SEND_BUFFER] = { 0 };

			//3.计算数据校验码,
			memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
			checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
			tmp = tmpSendPackDataOne + head.contentLenth;
			memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

			//4. 转义数据包内容
			*sendPackData = PACKSIGN;							 //flag 
			if ((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData + 1, &outDataLenth)) < 0)
			{
				//myprint("Error : func escape()");
				//assert(0);
			}
			*(sendPackData + outDataLenth + 1) = PACKSIGN; 	 //flag 

			//5. 将发送数据包放入队列, 通知发送线程
			/*if ((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, cmd)) < 0)
			{
				myprint("Error : func push_queue_send_block()");
				assert(0);
			}*/

			{
				std::lock_guard<std::mutex> lock(_mtxSendData);
				_deqSendData.push_back(std::string(sendPackData, outDataLenth + sizeof(char) * 2));
			}

			//6. 将发送数据放入链表			 
			/*if ((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, 0, sendPackData, outDataLenth + sizeof(char) * 2, cmd)) < 0)
			{
				myprint("Error: func Pushback_node_index_listSendInfo()");
				assert(0);
			}*/

			//sem_post(&g_sem_cur_write);
			//sem_wait(&g_sem_send);

			return ret;
		}

		//upload file content  6~liunx.c~	   The package cmd : 6
		int ScannerClient::upload_picture(const std::string &msg)
		{
			int ret = 0, index = 0, i = 0;			//index : 当前文件包的序号
			uint16_t  tmpTotalPack = 0, totalPacka = 0;				//发送文件所需要的总包数
			char *tmp = NULL;
			char filepath[250] = { 0 };				//发送文件的绝对路径
			char fileName[100] = { 0 };				//发送的文件名
			int  checkCode = 0;						//校验码	
			int doucumentLenth = 0;				//每个包储存文件数据内容的最大字节数
			int lenth = 0, packContenLenth = 0;		//lenth : 读取文件内容的长度, packContenLenth : 数据包的原始长度
			//char *sendPackData = NULL;				//发送数据包的地址
			//char *tmpSendPackDataOne = NULL;		//临时缓冲区的地址
			int outDataLenth = 0;					//每个数据包的发送长度
			FILE *fp = NULL;						//打开的文件句柄
			reqPackHead_t  head = { 0 };					//请求信息的报头
			int   sendPackNumber = 0;				//每轮发送数据的包数
			int   nRead = 0;						//已经拷贝的数据总长度
			int   copyLenth = 0;					//每次拷贝的数据长度
			int  tmpLenth = 0;

			//1. calculate The package store fileContent Max Lenth;  1369 - 46 = 1323
			doucumentLenth = COMREQDATABODYLENTH - FILENAMELENTH;

			//2.check The news style and find '~' position, copy The filePath
			size_t pos = msg.find(DIVISIONSIGN);
			if (pos == std::string::npos)
				return -1;

			std::string news = msg.substr(pos);
			if (news.size() < 1)
				return -1;
			
			tmp = (char *)news.c_str();
			/*if ((tmp = strchr(news, '~')) == NULL)
			{
				myprint("Error : The content style is err : %s", news);
				assert(0);
			}*/
			memcpy(filepath, tmp + 1, strlen(tmp) - 2);

			//3.get The filepath totalPacka by doucumentLenth of part of the filecontent	
			if ((ret = get_file_size(filepath, (int *)&totalPacka, doucumentLenth, NULL)) < 0)
			{
				//myprint("Error: func get_file_size()");
				//assert(0);
			}
			tmpTotalPack = totalPacka;

			//4.find The fileName and open The file
			if ((ret = find_file_name(fileName, sizeof(fileName), filepath)) < 0)
			{
				//myprint("Error: func find_file_name()");
				//assert(0);
			}
			if ((fp = fopen(filepath, "rb")) == NULL)
			{
				//myprint("Error: func fopen() : %s", filepath);
				//assert(0);
			}

			char	tmpSendPackDataOne[SEND_BUFFER] = { 0 };
			//5.allock The block buffer in memory Pool
			/*if ((tmpSendPackDataOne = mem_pool_alloc(g_memPool)) == NULL)
			{
				ret = -1;
				myprint("Error: func mem_pool_alloc() ");
				assert(0);
			}*/

			//6. operation The file content and package
			while (!feof(fp))
			{
				//7.计算每轮发送的总包数
				if (tmpTotalPack > PERROUNDNUMBER)
					sendPackNumber = PERROUNDNUMBER;
				else
					sendPackNumber = tmpTotalPack;

				//8.读取每轮发送文件的所有内容
				
				memset(_sendFileContent, 0, PERROUNDNUMBER * COMREQDATABODYLENTH);
				while (lenth < sendPackNumber * doucumentLenth && !feof(fp))
				{
					if ((tmpLenth = fread(_sendFileContent + lenth, 1, sendPackNumber * doucumentLenth - lenth, fp)) < 0)
					{
						//myprint("Error : func fread() : %s", filepath);
						//assert(0);
					}

					lenth += tmpLenth;
				}

				send_perRounc_begin(UPLOADCMDREQ, totalPacka, sendPackNumber);
				if (g_residuePackNoNeedSend)
				{
					g_residuePackNoNeedSend = false;
					goto End;
				}
				if (g_err_flag)
				{
					goto End;
				}

				//9.对本轮的数据进行组包
				for (i = 0; i < sendPackNumber; i++)
				{
					//10.拷贝文件名称
					tmp = tmpSendPackDataOne + sizeof(reqPackHead_t);
					memcpy(tmp, fileName, FILENAMELENTH);
					tmp += FILENAMELENTH;

					//11.拷文件数据
					copyLenth = MY_MIN(lenth, doucumentLenth);
					memcpy(tmp, _sendFileContent + nRead, copyLenth);
					nRead += copyLenth;
					lenth -= copyLenth;

					//12. 计算数据包原始长度, 并初始化报头
					packContenLenth = sizeof(reqPackHead_t) + FILENAMELENTH + copyLenth;
					assign_reqPack_head(&head, UPLOADCMDREQ, packContenLenth, SUBPACK, index, totalPacka, sendPackNumber);

					//13. 申请内存, 储存发送数据包
					type_data_t data;
					char	*sendPackData = data.data();
					/*if ((sendPackData = mem_pool_alloc(g_memPool)) == NULL)
					{
						myprint("Error : func mem_pool_alloc()");
						assert(0);
					}*/

					//14.计算数据校验码,
					memcpy(tmpSendPackDataOne, &head, sizeof(reqPackHead_t));
					checkCode = crc326((const char *)tmpSendPackDataOne, head.contentLenth);
					tmp = tmpSendPackDataOne + head.contentLenth;
					memcpy(tmp, (char *)&checkCode, sizeof(uint32_t));

					//15. 转义数据包内容
					*sendPackData = PACKSIGN;							 //flag 
					if ((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(uint32_t), sendPackData + 1, &outDataLenth)) < 0)
					{
						//myprint("Error : func escape()");
						//assert(0);
					}
					*(sendPackData + outDataLenth + 1) = PACKSIGN; 	 //flag 

					//16. 将发送数据包放入队列, 通知发送线程
					{
						std::lock_guard<std::mutex> lock(_mtxSendData);
						_deqSendData.push_back(std::string(sendPackData, outDataLenth + sizeof(char) * 2));
					}
					/*if ((ret = push_queue_send_block(g_queue_sendDataBlock, sendPackData, outDataLenth + sizeof(char) * 2, UPLOADCMDREQ)) < 0)
					{
						myprint("Error : func push_queue_send_block()");
						assert(0);
					}*/

					/*if ((ret = Pushback_node_index_listSendInfo(g_list_send_dataInfo, index, sendPackData, outDataLenth + sizeof(char) * 2, UPLOADCMDREQ)) < 0)
					{
						myprint("Error: func Pushback_node_index_listSendInfo()");
						assert(0);
					}*/

					index += 1;
					//sem_post(&g_sem_cur_write);
				}

				//17.一轮数据发送完毕, 进行全局数据的计算
				nRead = 0;
				tmpTotalPack -= sendPackNumber;
				lenth = 0;

				//18.等待信号通知, 并检查是否还需要下一轮的组包
				//sem_wait(&g_sem_send);
				if (g_residuePackNoNeedSend)
				{
					g_residuePackNoNeedSend = false;
					goto End;
				}
				if (g_err_flag)
				{
					goto End;
				}
			}

		End:
			//19. 情况资源, 释放临时内存和文件描述符
			/*if ((ret = mem_pool_free(g_memPool, tmpSendPackDataOne)) < 0)
			{
				myprint("Error : func mem_pool_free()");
				assert(0);
			}*/
			if (fp)
			{
				fclose(fp);
				fp = NULL;
			}

			return ret;
		}

		//处理 UI 命令的线程
		int thid_business(void *param)
		{
			if(param == NULL)
				return -1;

			ScannerClient &client = *(ScannerClient *)param;

			int ret = 0, cmd = -1;
			char tmpBuf[32] = { 0 };				//获取命令
			int numberCmd = 1;					//接收命令计数器

			//pthread_detach(pthread_self());

			//socket_log(SocketLevel[3], ret, "func thid_business() begin");

			while (1)
			{
				//sem_wait(&g_sem_business);			//等待信号量通知	
				//sem_wait(&g_sem_sequence);			//进行命令处理控制,必须一条命令处理完,返回结果给UI后, 再处理下一条命令

				//char 	cmdBuffer[BUFSIZE] = { 0 };	//取出队列中的命令
				int		num = 0;

				//1.取出 UI 命令信息
//				memset(tmpBuf, 0, sizeof(tmpBuf));
//				ret = pop_buffer_queue(g_que_buf, cmdBuffer);

				std::string &msg = client._deqMsg.front();
				if (ret == -1)
				{
					//socket_log(SocketLevel[4], ret, "Error: func pop_buffer_queue()");
					continue;
				}

				printf("\n-------------- number cmd : %d, content : %s\n\n", numberCmd, msg.c_str());
				//socket_log(SocketLevel[2], ret, "\n---------- number cmd : %d, content : %s", numberCmd, cmdBuffer);
				numberCmd += 1;

				//2.获取命令
				sscanf(msg.c_str(), "%[^~]", tmpBuf);
				cmd = atoi(tmpBuf);

				//do
				{
					//3.进行命令选择
					switch (cmd) {
					case 0x01:		//登录		
						if ((ret = client.login_func(msg)) < 0)
							;//socket_log(SocketLevel[4], ret, "Error: data_packge() login_func");
						break;
					case 0x02:		//登出
						if ((ret = client.exit_program(msg)) < 0)
							;//socket_log(SocketLevel[4], ret, "Error: data_packge() exit_program");
						break;
//					case 0x03:		//心跳
//						if ((ret = data_packge(heart_beat_program, cmdBuffer)) < 0)
//							socket_log(SocketLevel[4], ret, "Error: data_packge() heart_beat_program");
//						break;
					case 0x04:		//从服务器下载文件
						if ((ret = client.download_file_fromServer(msg)) < 0)
							;// socket_log(SocketLevel[4], ret, "Error: data_packge() download_template");
						break;
					case 0x05:		//获取文件最新ID 
						if ((ret = client.get_FileNewestID(msg)) < 0)
							;//socket_log(SocketLevel[4], ret, "Error: data_packge() get_FileNewestID");
						break;
					case 0x06:		//上传图片
						if ((ret = client.upload_picture(msg)) < 0)
							;// socket_log(SocketLevel[4], ret, "Error: data_packge() upload_picture");
						break;
//#if 1					
//					case 0x07:		//消息推送
//						if ((ret = data_packge(push_info_from_server_reply, cmdBuffer)) < 0)
//							socket_log(SocketLevel[4], ret, "Error: data_packge() upload_picture");
//						break;
//					case 0x08:		//主动关闭网络
//						if ((ret = data_packge(active_shutdown_net, cmdBuffer)) < 0)
//							socket_log(SocketLevel[4], ret, "Error: data_packge() active_shutdown_net");
//						break;
//					case 0x09:		//删除图片
//						if ((ret = data_packge(delete_spec_pic, cmdBuffer)) < 0)
//							socket_log(SocketLevel[4], ret, "Error: data_packge() delete_spec_pic");
//						break;
//#endif					
//					case 0x0A:		//链接服务器
//						if ((ret = data_packge(connet_server, cmdBuffer)) < 0)
//							socket_log(SocketLevel[4], ret, "Error: data_packge() connet_server");
//						break;
//
//					case 0x0B:		//读取配置文件
//						if ((ret = data_packge(read_config_file, cmdBuffer)) < 0)
//							socket_log(SocketLevel[4], ret, "Error: data_packge() read_config_file");
//						break;
//
//					case 0x0C:		//模板扫描数据上传
//						if ((ret = data_packge(template_extend_element, cmdBuffer)) < 0)
//							socket_log(SocketLevel[4], ret, "Error: data_packge() template_extend_element");
//						break;
#if 0					
					case 0x0E:		//加密传输 或者 取消加密传输
						if ((ret = data_packge(encryp_or_cancelEncrypt_transfer, cmdBuffer)) < 0)
							socket_log(SocketLevel[4], ret, "Error: data_packge() encryp_or_cancelEncrypt_transfer");
						break;
#endif					
					default:
						//myprint("UI send cmd : %d", cmd);
						//assert(0);
						continue;
					}

					if (ret < 0)
					{
						Sleep(1000);		//当处理出错后, 
						num += 1;
					}

				} while (0);

			}

		//	socket_log(SocketLevel[3], ret, "func thid_business() end");

			return	NULL;
		}

		//function: find  the whole packet; retval maybe error
		int	data_unpack_process(int recvLenth)
		{
			int ret = 0, tmpContentLenth = 0;
			static int flag = 0, lenth = 0; 	//flag 标识找到标识符的数量;  lenth : 数据的长度
			char buffer[2] = { 0 };
			static char content[3000] = { 0 };	//
			char tmpContent[1500] = { 0 }; 		//tmpBufFlag[15] = { 0 };
			char *tmp = NULL;

			tmp = content + lenth;
			while (recvLenth > 0)
			{
				while (1)
				{

					//1. jurge current fifo is empty
					if (get_fifo_element_count(g_fifo) == 0)
					{
						goto End;
					}
					//2. get The fifo element
					if ((ret = pop_fifo(g_fifo, (char *)buffer, 1)) < 0)
					{
						myprint("Error: func pop_fifo() ");
						assert(0);
					}
					//3.flag : have find The whole package head 
					if (*buffer == 0x7e && flag == 0)
					{
						flag = 1;
						continue;
					}
					else if (*buffer == 0x7e && flag == 1)	//4. flag : have find The whole package 
					{
						//4. anti escape recv data
						if ((ret = anti_escape(content, lenth, tmpContent, &tmpContentLenth)) < 0)
						{
							myprint("Error: func anti_escape() no have data");
							goto End;
						}

						if ((ret = compare_recvData_Correct(tmpContent, tmpContentLenth)) == 0)
						{
							//5. 数据包正确,处理数据包
							//socket_log(SocketLevel[2], ret, "------ func compare_recvData_Correct() OK ------");

							if ((deal_with_pack(tmpContent, tmpContentLenth)) < 0)
							{
								myprint("Error: func deal_with_pack()");
								assert(0);
							}

							//6.清空临时变量
							memset(content, 0, sizeof(content));
							memset(tmpContent, 0, sizeof(tmpContent));
							recvLenth -= (lenth + 2);
							lenth = 0;
							flag = 0;
							tmp = content + lenth;
							break;
						}
						else if (ret == -1)
						{
							//接收的数据包校验码出错, 需要服务器重新发送数据
							myprint("Error: func compare_recvData_Correct()");
							//7. clear the variable
							memset(content, 0, sizeof(content));
							memset(tmpContent, 0, sizeof(tmpContent));
							recvLenth -= (lenth + 2);
							lenth = 0;
							flag = 0;
							tmp = content + lenth;

							//8.遍历整个链表, 修改流水号, 重新所有发送数据				
							trave_LinkListSendInfo(g_list_send_dataInfo, modify_listNodeNews);
							ret = 0;			//此处被修改, 增加, 很重要  2017.04.21
							break;
						}
						else if (ret == -2)
						{
							//socket_log(SocketLevel[2], ret, "compare_recvData_Correct() ret : %d", ret);
							//9.接收的数据包 长度和流水号不正确, 不需要重新发送数据
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
					else if (*buffer != 0x7e && flag == 1)
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

		/*unpack The data */
		int thid_unpack_thread(void *arg)
		{
			int   ret = 0;
			//pthread_detach(pthread_self());

			while (1)
			{
				//1. wait The semaphore
				//sem_wait(&g_sem_unpack);

				//2. unpack The Queue Data
				if ((ret = data_unpack_process()) < 0)
				{
					//myprint("Error: func data_unpack_process()");
				}

			}
			//pthread_exit(NULL);
			return 0;
		}


		//接收数据; success 0;  fail -1;
		int ScannerClient::recv_data()
		{
			int ret = 0, recvLenth = 0;
			char buf[SEND_BUFFER * 2] = { 0 };
			static int nRead = 0;

			while (!_bStop)
			{
				//1.选择接收数据的方式
				recvLenth = recv(_sockfd, buf, sizeof(buf), 0);
				
				nRead += recvLenth;;
				if (recvLenth < 0)
				{
					//2.接收数据失败
					if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
					{
						continue;
					}
					else
					{
						ret = -1;
						//myprint("Err : func recv() from server !!!");
						goto End;
					}
				}
				else if (recvLenth == 0)
				{
					//3.服务器已关闭
					ret = -1;
					//myprint("The server has closed !!!");
					goto End;
				}
				else if (recvLenth > 0)
				{
					//4.接收数据成功

					//5.将数据放入队列中
					{
						std::lock_guard<std::mutex> lock(_mtxRecvData);
						_deqRecvData.push_back(std::string(buf, recvLenth));
					}
					//ret = push_fifo(g_fifo, buf, recvLenth);

					//6.释放信号量, 通知解包线程
					//sem_post(&g_sem_unpack);
					ret = 0;
					goto End;
				}
			}

		End:
			return ret;
		}

		
	}
}