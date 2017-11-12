
#include "virtualscannersvr.h"
#include <thread>
#include <vector>
#include <direct.h>
#include <io.h>
#include "virtualscannersvrmain.h"
#include "Ga7Exception.h"
namespace DROWNINGLIU
{
	namespace VSCANNERSVRLIB
	{
		using GAFIS7::GA7BASE::Ga7Exception;
		//ȫ�ֱ���
		//	RecvAndSendthid_Sockfd		_thid_sockfd_block[NUMBER];	//�ͻ�����Ϣ
		//char 			*_sendFileContent = NULL;					//��ȡÿ������ģ�������

		//ɢ���㷨 : crc32 У��
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

		/*crc32У��
		*@param : buf  ��������
		*@param : size ���ݳ���
		*@retval: �������ɵ�У����
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

		//����ͼƬ
		//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
		int VirtualScannerSession::save_multiPicture_func(char *recvBuf, int recvLen)
		{
			static FILE		*fp = NULL;
			reqPackHead_t	*tmpHead = NULL;

			char	*tmp = NULL;
			int		lenth = 0, ret = 0;
			char	TmpfileName[100] = { 0 };
			
			std::string	fileName;
			char	cmdBuf[1024] = { 0 };
			int		nWrite = 0;				//�Ѿ�д�����ݳ���
			int		perWrite = 0; 			//ÿ��д������ݳ���

			//0. ��ȡ���ݰ���Ϣ
			tmpHead = (reqPackHead_t *)recvBuf;
			lenth = recvLen - sizeof(reqPackHead_t) - sizeof(int) - FILENAMELENTH - sizeof(char) * 2;
			tmp = recvBuf + sizeof(reqPackHead_t) + FILENAMELENTH + sizeof(char) * 2;
			//myprint("picDataLenth : %d, currentIndex : %d, totalNumber : %d, recvLen : %d ...", lenth, tmpHead->currentIndex, tmpHead->totalPackage, recvLen);

			try
			{
				if (tmpHead->currentIndex != 0 && tmpHead->currentIndex + 1 < tmpHead->totalPackage)
				{
					if (fp == NULL)
					{
						ret = -1;
						throw Ga7Exception(__FILE__, __LINE__);
					}

					//1.��ͼƬ���м����ݰ�, �� ָ��ָ�� ͼƬ���ݵĲ���, ������д���ļ�
					while (nWrite < lenth)
					{
						if ((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
						{
							//myprint("Err : func fopen() : fileName : %s !!!", fileName);
							throw Ga7Exception(__FILE__, __LINE__);
						}
						nWrite += perWrite;
					}
				}
				else if (tmpHead->currentIndex == 0)
				{
					//2.��ͼƬ�ĵ�һ������, ��ȡ�ļ�����
					memcpy(TmpfileName, recvBuf + sizeof(reqPackHead_t) + sizeof(char) * 2, 46);
					fileName = _svr._DirPath + "\\" + TmpfileName;

					//3.�鿴�ļ��Ƿ����, ���ڼ�ɾ��	
					if (if_file_exist(fileName.c_str()))
					{
						if (fp)
						{
							fclose(fp);
							fp = NULL;
						}

						if (!DeleteFileA(fileName.c_str()))
						{
							//myprint("Err : func pox_system()");
							ret = -1;
							throw Ga7Exception(__FILE__, __LINE__);
						}
					}

					//4.�����ļ�, ���ļ���д������
					if ((fp = fopen(fileName.c_str(), "ab")) == NULL)
					{
						//myprint("Err : func fopen() : fileName : %s !!!", fileName);
						ret = -1;
						throw Ga7Exception(__FILE__, __LINE__);
					}

					//5.�� ָ��ָ�� ͼƬ���ݵĲ���, ������д���ļ�
					while (nWrite < lenth)
					{
						if ((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
						{
							//("Err : func fopen() : fileName : %s !!!", fileName);
							throw Ga7Exception(__FILE__, __LINE__);
						}
						nWrite += perWrite;
					}
				}
				else if (tmpHead->currentIndex + 1 == tmpHead->totalPackage)
				{
					if (fp == NULL)
					{
						ret = -1;
						throw Ga7Exception(__FILE__, __LINE__);
					}

					//6.��ͼƬ�����һ������		
					while (nWrite < lenth)
					{
						if ((perWrite = fwrite(tmp + nWrite, 1, lenth - nWrite, fp)) < 0)
						{
							//myprint("Err : func fopen() : fileName : %s !!!", fileName);
							throw Ga7Exception(__FILE__, __LINE__);
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
					throw Ga7Exception(__FILE__, __LINE__);
				}
			}
			catch (const Ga7Exception&e)
			{
				e.print();
			}
			
			if (ret < 0 && fp)
			{
				fclose(fp);
				fp = NULL;
			}

			return ret;
		}

		//����ͼƬ
		//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
		int VirtualScannerSession::save_picture_func(char *recvBuf, int recvLen)
		{
			static FILE		*fp = NULL;
			reqPackHead_t	*tmpHead = NULL;

			char	*tmp = NULL;
			int		lenth = 0, ret = 0;
			char	TmpfileName[100] = { 0 };
			std::string	fileName;
			char	cmdBuf[1024] = { 0 };
			int		nWrite = 0;				//�Ѿ�д�����ݳ���
			int		perWrite = 0; 			//ÿ��д������ݳ���

			//0. ��ȡ���ݰ���Ϣ
			tmpHead = (reqPackHead_t *)recvBuf;
			lenth = recvLen - sizeof(reqPackHead_t) - sizeof(int) - FILENAMELENTH;
			tmp = recvBuf + sizeof(reqPackHead_t) + FILENAMELENTH;
			//myprint("picDataLenth : %d, currentIndex : %d, totalNumber : %d, recvLen : %d ...", lenth, tmpHead->currentIndex, tmpHead->totalPackage, recvLen);

			if (tmpHead->currentIndex != 0 && tmpHead->currentIndex + 1 < tmpHead->totalPackage)
			{
				//1.��ͼƬ���м����ݰ�, �� ָ��ָ�� ͼƬ���ݵĲ���, ������д���ļ�
				while (nWrite < lenth)
				{
					if (fp == NULL)
					{
						ret = -1;
						goto End;
					}

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
				//2.��ͼƬ�ĵ�һ������, ��ȡ�ļ�����
				memcpy(TmpfileName, recvBuf + sizeof(reqPackHead_t), 46);
				fileName = _svr._DirPath + "\\" + TmpfileName;

				//3.�鿴�ļ��Ƿ����, ���ڼ�ɾ��	
				if (if_file_exist(fileName.c_str()))
				{
					if (fp)
					{
						fclose(fp);
						fp = NULL;
					}
					if(!DeleteFileA(fileName.c_str()))
					{
						//myprint("Err : func pox_system()");
						ret = -1;
						goto End;
					}
				}

				//4.�����ļ�, ���ļ���д������
				if ((fp = fopen(fileName.c_str(), "ab")) == NULL)
				{
					//myprint("Err : func fopen() : fileName : %s !!!", fileName);
					ret = -1;
					goto End;
				}

				//5.�� ָ��ָ�� ͼƬ���ݵĲ���, ������д���ļ�
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
				if (fp == NULL)
				{
					ret = -1;
					goto End;
				}

				//6.��ͼƬ�����һ������		
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
			if (ret < 0 && fp)
			{
				fclose(fp);
				fp = NULL;
			}

			return ret;
		}

		/*�Ƚ����ݰ������Ƿ���ȷ
		*@param : tmpContent		���ݰ�ת��������
		*@param : tmpContentLenth	ת������ݵĳ���, ����: ��ͷ + ��Ϣ + У����
		*@retval: success 0, repeat -1, NoRespond -2
		*/
		int VirtualScannerSession::compare_recvData_correct_server(char *tmpContent, int tmpContentLenth)
		{
			int		ret = 0;
			int		checkCode = 0;				//���ؼ���У����
			int		*tmpCheckCode = NULL;		//�����������У����
			uint8_t	cmd = 0;					//�������ݰ�����

			reqPackHead_t			*tmpHead = NULL;	//����ͷ
			reqTemplateErrHead_t	*Head = NULL;		//����ģ������ͷ

			//1.��ȡ�������ݰ���������
			cmd = *((uint8_t *)tmpContent);

			//2.���������ֵ�ѡ��
			if (cmd != DOWNFILEREQ)
			{
				//3. ��ȡ�������ݵı�ͷ��Ϣ
				tmpHead = (reqPackHead_t *)tmpContent;

				//4. ���ݰ����ȱȽ�
				if (tmpHead->contentLenth + sizeof(int) == tmpContentLenth)
				{
					//5.������ȷ, ������ˮ�űȽ�, ȥ���ذ�			
					if (tmpHead->seriaNumber >= _recvSerial)
					{
						//6.��ˮ����ȷ,����У����Ƚ�
						tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
						checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
						if (checkCode == *tmpCheckCode)
						{
							//7.У������ȷ, �������ݰ���ȷ
							_recvSerial += 1;		//��ˮ�������ƶ�
							ret = 0;
						}
						else
						{
							//8.У�������, ���ݰ���Ҫ�ط�
							/*myprint("Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
								cmd, tmpHead->seriaNumber, checkCode, *tmpCheckCode);*/
							ret = -1;
						}
					}
					else
					{
						//9.��ˮ�ų���, ����Ҫ�ط�
						/*myprint("Err : cmd : %d, package serial : %u, loacl ServerSerial : %u",
							cmd, tmpHead->seriaNumber, _recvSerial);*/
						ret = -2;
					}
				}
				else
				{
					//10.���ݳ��ȳ���, ����Ҫ�ط�
					/*myprint("Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u",
						cmd, tmpHead->contentLenth + sizeof(int), tmpContentLenth, tmpHead->seriaNumber);*/
					ret = -2;
				}
			}
			else
			{
				Head = (reqTemplateErrHead_t *)tmpContent;

				//11.���ݰ����ȱȽ�
				if (Head->contentLenth + sizeof(int) == tmpContentLenth)
				{
					//12.���ݰ�������ȷ, �Ƚ���ˮ��, ȥ���ذ�
					if (_recvSerial == Head->seriaNumber)
					{
						//13.��ˮ����ȷ, ����У����Ƚ�
						_recvSerial += 1;		//��ˮ�������ƶ�
						//14.��ˮ����ȷ,����У����Ƚ�
						tmpCheckCode = (int *)(tmpContent + tmpContentLenth - sizeof(uint32_t));
						checkCode = crc326(tmpContent, tmpContentLenth - sizeof(uint32_t));
						if (*tmpCheckCode == checkCode)
						{
							//15.У������ȷ, ������ݰ���ȷ
							ret = 0;
						}
						else
						{
							//16.У�������, ��Ҫ�ط����ݰ�						
							/*socket_log(SocketLevel[4], ret, "Error: cmd : %d, package serial : %u checkCode : %u != *tmpCheckCode : %u",
								cmd, Head->seriaNumber, checkCode, *tmpCheckCode);*/
							ret = -1;
						}
					}
					else
					{
						//17.��ˮ�ų���, ����Ҫ�ط�
						/*socket_log(SocketLevel[3], ret, "Err : cmd : %d, package serial : %u, loacl ServerSerial : %u",
							cmd, Head->seriaNumber, _recvSerial);*/
						ret = -2;
					}
				}
				else
				{
					//18.���ݳ��ȳ���, ����Ҫ�ط�
					/*socket_log(SocketLevel[3], ret, "Err : cmd : %d, package Lenth : %d != recvLenth : %d, package serial : %u",
						cmd, Head->contentLenth + sizeof(int), tmpContentLenth, Head->seriaNumber);*/
					ret = -2;
				}
			}

			return ret;
		}

		/*��ȡ�ļ��ְ����ܰ������ļ��ܴ�С
		*@param : filePath �ļ�·��
		*@param : number ������ķְ���
		*@param : contentLenth �ְ��ı�׼��С
		*@param : fileSizeSrc �ļ��ܴ�С
		*@retval: �ɹ� 0; ʧ�� -1;
		*/
		int get_file_size(const char *filePath, int *number, int contentLenth, unsigned long *fileSizeSrc)
		{
			int			ret = 0;
			struct stat	statbuff;
			unsigned long filesize = 0;
			int remainder = 0;			//����

			if (!filePath)
			{
				ret = -1;
				//socket_log(SocketLevel[4], ret, "Error: filePath == NULL");
				return -1;
			}

			//1.��ȡ�ļ���С
			if (stat(filePath, &statbuff) < 0)
			{
				ret = -1;
				//socket_log(SocketLevel[4], ret, "Error: func stat() %s", strerror(errno));
				return -1;
			}
			else
				filesize = statbuff.st_size;

			//2.����ְ���С
			if (number)
			{
				if ((remainder = filesize % contentLenth) > 0)
					*number = filesize / contentLenth + 1;
				else
					*number = filesize / contentLenth;
			}

			//3.��ȡ�ļ���С
			if (fileSizeSrc)
				*fileSizeSrc = filesize;

			return ret;
		}

		//����������ݽ��н�ת��,  �ú����ж����������������ڴ�
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

		//����������ݽ���ת��
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

		void VirtualScannerSession::assign_serverSubPack_head(resSubPackHead_t *head, uint8_t cmd, int contentLenth, int currentIndex, int totalNumber)
		{
			memset(head, 0, sizeof(resSubPackHead_t));
			head->cmd = cmd;
			head->contentLenth = contentLenth;
			head->totalPackage = totalNumber;
			head->currentIndex = currentIndex;
			head->seriaNumber = _sendSerial++;
		}

		void VirtualScannerSession::assign_comPack_head(resCommonHead_t *head, int cmd, int contentLenth, int isSubPack, int isFailPack, int failPackIndex, int isRespond)
		{
			memset(head, 0, sizeof(resCommonHead_t));
			head->cmd = cmd;
			head->contentLenth = contentLenth;
			head->isSubPack = isSubPack;
			head->isFailPack = isFailPack;
			head->failPackIndex = failPackIndex;
			head->isRespondClient = isRespond;
			head->seriaNumber = _sendSerial++;
		}

		//�������ݰ�	3�����ݰ�Ӧ��		
		//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
		int VirtualScannerSession::heart_beat_func_reply(char *recvBuf, int recvLen, int index)
		{
			int		ret = 0, outDataLenth = 0; 				//�������ݵĳ���
			int		checkNum = 0;							//У����
			char	tmpSendBuf[SEND_BUFFER] = { 0 };				//��ʱ���ݻ�����
			char	*sendBufStr = NULL;						//�������ݵ�ַ
			char	*tmp = NULL;
			
			resCommonHead_t	head = { 0 };					//Ӧ�����ݱ�ͷ

			type_data_t	data;

			sendBufStr = data.data();
			//1. ��ӡ���ݰ���Ϣ
			//printf_comPack_news(recvBuf);

			//3. ��װ��ͷ��Ϣ
			outDataLenth = sizeof(resCommonHead_t);
			assign_comPack_head(&head, HEARTCMDRESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));

			//4. ����У����
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//5. ת����������
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//6. ��Ϸ��͵�����
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			if (outDataLenth > SEND_BUFFER)
				throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

			{
				std::lock_guard<std::mutex> lock(_mtxData);

				_deqData.push_back(std::make_tuple(HEARTCMDRESPON, outDataLenth + 2, std::move(data)));
			}

		End:
			return ret;
		}

		//ģ��ͼƬ���� ���ݰ�	13�����ݰ�Ӧ��		
		//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
		int VirtualScannerSession::upload_template_set_reply(char *recvBuf, int recvLen, int index)
		{
			int		ret = 0, outDataLenth = 0; 				//�������ݵĳ���
			int		checkNum = 0;							//У����
			char	tmpSendBuf[SEND_BUFFER] = { 0 };				//��ʱ���ݻ�����
			char	*sendBufStr = NULL;						//�������ݵ�ַ
			char	*tmp = NULL;

			resCommonHead_t	head = { 0 };					//Ӧ�����ݱ�ͷ
			reqPackHead_t	*tmpHead = NULL; 				//����ͷ��Ϣ
			static bool		latePackFlag = false;			//true : ��ģ�����һ��ͼƬ,���һ�����ݰ�, false �����һ������,  
			static int		recvSuccessSubNumber = 0;		//ÿ�ֳɹ����յ����ݰ���Ŀ

			type_data_t	data;

			sendBufStr = data.data();
			//1.��ӡ����ͼƬ����Ϣ
			//printf_comPack_news(recvBuf); 				

			//3.��ȡ��ͷ��Ϣ
			tmpHead = (reqPackHead_t *)recvBuf;

			//4.����������Ϣ�����ж�
			if (tmpHead->isSubPack == 1)
			{
				//5.�Ӱ�, �������ݰ���Ϣ�ж�
				if ((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
				{
					//6.������ȷ, ����ͼƬ���ݵĴ洢, ��¼���ֳɹ����յ��Ӱ�����
					recvSuccessSubNumber += 1;
#if FAST_IO == 1
					//д����У��������߳���Ӳ��д
					ret = 0;
#else
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
						//7.��ʶÿ�ֳɹ����յ����Ӱ���ĿС�ڷ�����Ŀ, ����Ҫ���н��лظ�				
						ret = 0;
						goto End;
					}
					else if (recvSuccessSubNumber == tmpHead->perRoundNumber)
					{
						//8.��ʶÿ�����е��Ӱ��ɹ�����, �����жϸûظ�����Ӧ��(��ͼƬ�ɹ�����Ӧ���Ǳ��ֳɹ�����Ӧ��)
						recvSuccessSubNumber = 0;
						if (latePackFlag == false)
						{
							//���ֳɹ�����					
							outDataLenth = sizeof(resCommonHead_t);
							assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);
						}
						else if (latePackFlag == true)
						{
							//����ͼƬ���ɹ�����
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
					//9.��Ҫ�ط�			
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);
				}
				else
				{
					//10.����Ҫ�ط�
					ret = 0;
					goto End;
				}
			}
			else
			{
				//11.�ܰ�, ������Ӧ.
				outDataLenth = sizeof(resCommonHead_t);
				assign_comPack_head(&head, MUTIUPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
			}

			//13. ������ͷ����, ����У����
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			tmp = tmpSendBuf + head.contentLenth;
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//14. ת����������
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				//myprint("Error : func escape() escape() !!!!");
				goto End;
			}

			//15. ��Ϸ��͵�����
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			if (outDataLenth > SEND_BUFFER)
				throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

			{
				std::lock_guard<std::mutex> lock(_mtxData);

				_deqData.push_back(std::make_tuple(MUTIUPLOADCMDRESPON, outDataLenth + 2, std::move(data)));
			}

		End:
			return ret;
		}

		//ģ����չ���ݰ� ���ݰ�			12 �����ݰ�Ӧ��
		/*@param : recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);
		*@param :  recvLen : ����Ϊ buf ������ ����
		*@param :  sendBuf : ���� client ����������
		*@param :  sendLen : ���� �������ݵĳ���
		*/
		int VirtualScannerSession::template_extend_element_reply(char *recvBuf, int recvLen, int index)
		{
			resCommonHead_t	head = { 0 };				//�������ݱ�ͷ
			unsigned int	contentLenth = 0;			//�������ݰ�����

			int		ret = 0, outDataLenth = 0;			//�������ݰ�����
			char	*tmp = NULL, *sendBufStr = NULL;	//���͵�ַ
			int		checkNum = 0;						//У����
			char	tmpSendBuf[SEND_BUFFER] = { 0 };			//��ʱ������

			type_data_t	data;
			sendBufStr = data.data();

			//1. ��ӡ����ͼƬ����Ϣ, �����ڴ�
			//printf_template_news(recvBuf);

			//2. �����Ϣͷ
			contentLenth = sizeof(resCommonHead_t) + sizeof(char) * 1;
			assign_comPack_head(&head, TEMPLATECMDRESPON, contentLenth, NOSUBPACK, 0, 0, 1);

			//3. �����Ϣ��, �����ճɹ�����
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			*tmp++ = 0;

			//4. ���У����
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//5. ת����������
			if ((ret = escape(0x7e, tmpSendBuf, contentLenth + sizeof(uint32_t), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//6. ��Ϸ��͵�����
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;
			
			if (outDataLenth > SEND_BUFFER)
				throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

			{
				std::lock_guard<std::mutex> lock(_mtxData);

				_deqData.push_back(std::make_tuple(TEMPLATECMDRESPON, outDataLenth + 2, std::move(data)));
			}
			
		End:
			return ret;
		}

		//ɾ��ͼƬӦ�� ���ݰ�			9 �����ݰ�
		//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
		int VirtualScannerSession::delete_func_reply(char *recvBuf, int recvLen, int index)
		{
			int		ret = 0, outDataLenth = 0;					//�������ݰ�����
			int		checkNum = 48848748;						//У����
			char	tmpSendBuf[SEND_BUFFER] = { 0 };					//��ʱ������
			char	*tmp = NULL, *sendBufStr = NULL;			//�������ݰ���ַ
			std::string		fileName;							//�ļ�����
			resCommonHead_t	head = { 0 };						//Ӧ�����ݱ�ͷ

			type_data_t	data;
			sendBufStr = data.data();

			//1. ��ӡ���ݰ�����
			//printf_comPack_news(recvBuf);

			//3. ��ȡ�ļ���Ϣ, ��ɾ��
			fileName.assign(recvBuf + sizeof(reqPackHead_t), recvLen - sizeof(reqPackHead_t) - sizeof(int));
			std::string	rmFile = _svr._DirPath + "\\" + fileName;
			BOOL bSuccess = DeleteFileA(rmFile.c_str());

			//4. ��װ���ݰ���Ϣ
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			if (!bSuccess)
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

			//5. ��װ��ͷ
			assign_comPack_head(&head, DELETECMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));

			//6. ���У����
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//7. ת����������
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				//printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//6. ��Ϸ��͵�����
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			if (outDataLenth > SEND_BUFFER)
				throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

			{
				std::lock_guard<std::mutex> lock(_mtxData);

				_deqData.push_back(std::make_tuple(DELETECMDRESPON, outDataLenth + 2, std::move(data)));
			}
		
		End:
			return ret;
		}

		int push_info_toUi_reply(char *recvBuf, int recvLen, int index)
		{
			//myprint("push infomation OK ... ");
			return 0;
		}

		//�ϴ�ͼƬӦ�� ���ݰ�;  ��������һ�� �ظ�һ��.
		//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
		int VirtualScannerSession::upload_func_reply(char *recvBuf, int recvLen, int index)
		{
			int		ret = 0, outDataLenth = 0;					//�������ݵĳ���
			int		checkNum = 0;								//У����
			char	tmpSendBuf[SEND_BUFFER] = { 0 };					//��ʱ���ݻ�����
			char	*sendBufStr = NULL;							//�������ݵ�ַ
			char	*tmp = NULL;
			
			resCommonHead_t head = { 0 };						//Ӧ�����ݱ�ͷ
			reqPackHead_t	*tmpHead = NULL;					//����ͷ��Ϣ
			static bool		latePackFlag = false;				//true : ͼƬ���һ��, false �����һ������,  
			static int		recvSuccessSubNumber = 0;			//ÿ�ֳɹ����յ����ݰ���Ŀ

			type_data_t	data;
			sendBufStr = data.data();

			//1.��ӡ����ͼƬ����Ϣ
			//printf_comPack_news(recvBuf);					

			//3.��ȡ��ͷ��Ϣ
			tmpHead = (reqPackHead_t *)recvBuf;

			//4.����������Ϣ�����ж�
			if (tmpHead->isSubPack == 1)
			{
				//5.�Ӱ�, �������ݰ���Ϣ�ж�
				if ((ret = compare_recvData_correct_server(recvBuf, recvLen)) == 0)
				{
					//6.������ȷ, ����ͼƬ���ݵĴ洢, ��¼���ֳɹ����յ��Ӱ�����
					recvSuccessSubNumber += 1;
#if FAST_IO == 1
					//д����У��������߳���Ӳ��д
					ret = 0;
#else
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
						//7.��ʶÿ�ֳɹ����յ����Ӱ���ĿС�ڷ�����Ŀ, ����Ҫ���н��лظ�				
						ret = 0;
						goto End;
					}
					else if (recvSuccessSubNumber == tmpHead->perRoundNumber)
					{
						//8.��ʶÿ�����е��Ӱ��ɹ�����, �����жϸûظ�����Ӧ��(��ͼƬ�ɹ�����Ӧ���Ǳ��ֳɹ�����Ӧ��)
						recvSuccessSubNumber = 0;
						if (latePackFlag == false)
						{
							//���ֳɹ�����					
							outDataLenth = sizeof(resCommonHead_t);
							assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 0, 0, 0);
						}
						else if (latePackFlag == true)
						{
							//��ͼƬ�ɹ�����
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
					//9.��Ҫ�ط�			
					outDataLenth = sizeof(resCommonHead_t);
					assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, SUBPACK, 1, tmpHead->currentIndex, 0);
				}
				else
				{
					//10.����Ҫ�ط�
					ret = 0;
					goto End;
				}
			}
			else
			{
				//11.�ܰ�, ������Ӧ.
				outDataLenth = sizeof(resCommonHead_t);
				assign_comPack_head(&head, UPLOADCMDRESPON, outDataLenth, NOSUBPACK, 0, 0, 0);
			}

			//12. ������ͷ����, ����У����
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			tmp = tmpSendBuf + head.contentLenth;
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//13. ת����������
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				//myprint("Error : func escape() escape() !!!!");
				goto End;
			}

			//14. ��Ϸ��͵�����
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			if (outDataLenth > SEND_BUFFER)
				throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

			{
				std::lock_guard<std::mutex> lock(_mtxData);

				_deqData.push_back(std::make_tuple(UPLOADCMDRESPON, outDataLenth + 2, std::move(data)));
			}
			
		End:
			return ret;
		}

		//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��); 
		//recvLen : ����Ϊ buf ������ ����
		int VirtualScannerSession::get_FileNewestID_reply(char *recvBuf, int recvLen, int index)
		{
			int		ret = 0, outDataLenth = 0; 				//�������ݵĳ���
			int		checkNum = 0;							//У����
			char	tmpSendBuf[SEND_BUFFER] = { 0 };				//��ʱ���ݻ�����
			char	*sendBufStr = NULL;						//�������ݵ�ַ
			char	*tmp = NULL;
			int		fileType = 0;							//������ļ�����

			resCommonHead_t	head = { 0 };					//Ӧ�����ݱ�ͷ

			type_data_t	data;
			sendBufStr = data.data();

			//1. ��ӡ���ݰ���Ϣ
			//printf_comPack_news(recvBuf);	

			//1. ��ȡ�ļ�����
			tmp = recvBuf + sizeof(reqPackHead_t);
			if (*tmp == 0)
				fileType = 0;
			else if (*tmp == 1)
				fileType = 1;
			else
				return -1;

			//3. ��װ���ݰ���Ϣ
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			if (fileType == 0)
			{
				*tmp++ = fileType;
				memcpy(tmp, &_svr._fileTemplateID, sizeof(_svr._fileTemplateID));
			}
			else if (fileType == 1)
			{
				*tmp++ = fileType;
				memcpy(tmp, &_svr._fileTableID, sizeof(_svr._fileTemplateID));
			}

			outDataLenth = sizeof(resCommonHead_t) + 65;

			//4. ��װ��ͷ��Ϣ	
			assign_comPack_head(&head, NEWESCMDRESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));

			//5. ����У����
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			tmp += 64;
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//6. ת����������
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//7. ��Ϸ��͵�����
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			if (outDataLenth > SEND_BUFFER)
				throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

			{
				std::lock_guard<std::mutex> lock(_mtxData);

				_deqData.push_back(std::make_tuple(NEWESCMDRESPON, outDataLenth + 2, std::move(data)));
			}

		End:
			return ret;
		}

		//����ģ�����ݰ�	4�����ݰ�Ӧ��		
		//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����  
		//recvLen : ����Ϊ buf ������ ����
		int VirtualScannerSession::downLoad_template_func_reply(char *recvBuf, int recvLen, int index)
		{
			int		ret = -1;
			char	*tmp = NULL;

			//���ص��ļ�����
			enum class FILE_TYPE	fileType;		
			std::string				fileName;
			
			auto sendFile = [this, &fileName, &ret](int nType)
			{
				int		doucumentLenth = 0, outDataLenth = 0;
				int		contentLenth = 0;
				FILE	*fp = NULL;
				int		totalPacka = 0;							//���ļ����ܰ���
				int		indexNumber = 0;						//ÿ�������
				int		tmpLenth = 0;
				char	*tmp = NULL, *sendBufStr = NULL;		// data_.data();
				int		checkNum = 0, nRead = 0;				//У����� ��ȡ�ֽ���
				char	tmpSendBuf[SEND_BUFFER] = { 0 };				//��ʱ������
				resSubPackHead_t 	head = { 0 };				//Ӧ�����ݰ���ͷ

																//3. open The template file
				if ((fp = fopen(fileName.c_str(), "rb+")) == NULL)
				{
					//myprint("Err : func fopen() : %s", fileName);
					goto End;
				}

				//4. get The file Total Package Number 
				doucumentLenth = RESTEMSUBBODYLENTH - sizeof(char) * 3;
				if ((ret = get_file_size(fileName.c_str(), &totalPacka, doucumentLenth, NULL)) < 0)
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
					*tmp++ = nType;

					while (nRead < doucumentLenth && !feof(fp))
					{
						if ((tmpLenth = fread(tmp + nRead, 1, doucumentLenth - nRead, fp)) < 0)
						{
							//myprint("Error: func fread() : %s ", fileName);
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

					//5.6 escape the buffer
					*sendBufStr = PACKSIGN;							 //flag 
					if ((ret = escape(PACKSIGN, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
					{
						//myprint("Error: func escape()");
						goto End;
					}

					*(sendBufStr + outDataLenth + 1) = PACKSIGN; 	 //flag 

					if (outDataLenth > SEND_BUFFER)
						throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

					{
						std::lock_guard<std::mutex> lock(_mtxData);

						_deqData.push_back(std::make_tuple(DOWNFILEREQ, outDataLenth + sizeof(char) * 2, std::move(data)));
					}

					nRead = 0;
				}

				ret = 0;
			End:
				if (fp)
					fclose(fp);
			};

			//1. ��ӡ�������ݰ���Ϣ
			//printf_comPack_news(recvBuf);

			//2. get The downLoad file Type And downLoadFileID
			tmp = recvBuf + sizeof(reqPackHead_t);
			if (*tmp == static_cast<int>(FILE_TYPE::FILE_TEMPLATE))
			{
				//����ģ��
				fileName = _svr._downLoadDir + "\\" + _svr._fileNameTemplate;
				fileType = FILE_TYPE::FILE_TEMPLATE;
				//sendFile(0);

				//�ȷ�ͼƬ�����ģ��
			}
			else if (*tmp == static_cast<int>(FILE_TYPE::FILE_TABLE))
			{
				fileName = _svr._downLoadDir + "\\" + _svr._fileNameTable;
				fileType = FILE_TYPE::FILE_TABLE;
				sendFile(1);
			}
			else
			{
				return -1;
			}
			
			//return 0;
			if (fileType == FILE_TYPE::FILE_TEMPLATE)
			{
				std::vector<std::string> jpgs;

				{
					std::lock_guard<std::mutex> lock(_svr._mtxFiles);
					std::for_each(std::begin(_svr._vctfiles), std::end(_svr._vctfiles), [this, &jpgs](auto &f)
					{
						if (f.name == _svr._fileNameTable || f.name == _svr._fileNameTemplate)
							return;

						jpgs.push_back(f.name);
					});
				}

				for (auto &j : jpgs)
				{
					if (j.empty())
						continue;

					fileName = _svr._downLoadDir + "\\" + j;

					char mark = j.at(0);
					mark -= '0';
					std::cout << fileName;
					//1.jpg -> 2, 2.jpg -> 3
					sendFile(mark + 1);
				}

				fileName = _svr._downLoadDir + "\\" + _svr._fileNameTemplate;
				//fileType = FILE_TYPE::FILE_TEMPLATE;
				sendFile(0);
			}

			return ret;
		}

		//�˳���¼Ӧ�� ���ݰ�
		//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
		int VirtualScannerSession::exit_func_reply(char *recvBuf, int recvLen, int index)
		{
			int		ret = 0, outDataLenth = 0;					//�������ݵĳ���
			int		checkNum = 0;								//У����
			char	tmpSendBuf[SEND_BUFFER] = { 0 };					//��ʱ���ݻ�����
			char	*sendBufStr = NULL;							//�������ݵ�ַ
			char	*tmp = NULL;

			resCommonHead_t	head = { 0 };						//Ӧ�����ݱ�ͷ

			type_data_t	data;
			sendBufStr = data.data();

			//1. ��ӡ���ݰ���Ϣ		
			//printf_comPack_news(recvBuf);			//��ӡ����ͼƬ����Ϣ

			//3. ������Ϣ����
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			*tmp++ = 0x01;
			outDataLenth = sizeof(resCommonHead_t) + 1;

			//4. ��װ��ͷ
			assign_comPack_head(&head, LOGOUTCMDRESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));

			//5. ����У����
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//6. ת����������
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//7. ��Ϸ��͵�����
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;
			
			if (outDataLenth > SEND_BUFFER)
				throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

			{
				std::lock_guard<std::mutex> lock(_mtxData);

				_deqData.push_back(std::make_tuple(LOGOUTCMDRESPON, outDataLenth + 2, std::move(data)));
			}
		
		End:
			return ret;
		}

		//��¼Ӧ�����ݰ�
		//recvBuf : ת��֮�������:  ���� : ��Ϣͷ + ��Ϣ�� + У����     (ֻ��ȥ����, ������ʾ��);  recvLen : ����Ϊ buf ������ ����
		int VirtualScannerSession::login_func_reply(char *recvBuf, int recvLen, int index)
		{
			int		ret = 0, outDataLenth = 0;					//�������ݵĳ���
			int		checkNum = 0;								//У����
			char	tmpSendBuf[SEND_BUFFER] = { 0 };					//��ʱ���ݻ�����
			char	*sendBufStr = NULL;
			char	*tmp = NULL;
			char	*userName = "WuKong";						//�����˻����û���

			resCommonHead_t	head = { 0 };						//Ӧ�����ݱ�ͷ
			reqPackHead_t	*tmpHead = NULL;					//������Ϣ
			
			type_data_t	data;
			sendBufStr = data.data();
			
			//1.��ӡ����ͼƬ����Ϣ
			//printf_comPack_news(recvBuf);

			tmpHead = (reqPackHead_t *)recvBuf;
			_recvSerial = tmpHead->seriaNumber;
			_recvSerial++;

			//3.��װ���ݰ���Ϣ, ��ȡ�������ݰ�����
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			*tmp++ = 0x01;				//��¼�ɹ�
			*tmp++ = 0x00;				//�ɹ���, �˴���Ϣ������н���, ���д
			*tmp++ = (char)strlen(userName);	//�û�������
			memcpy(tmp, userName, strlen(userName));	//�����û���	
			outDataLenth = sizeof(resCommonHead_t) + sizeof(char) * 3 + strlen(userName);

			//4.��װ��ͷ��Ϣ		
			assign_comPack_head(&head, LOGINCMDRESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			//myprint("outDataLenth : %d, head.contentLenth : %d", outDataLenth, head.contentLenth);

			//5.���У����
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			tmp = tmpSendBuf + head.contentLenth;
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//6. ת����������
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//7. ��Ϸ��͵�����
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;
			
			if (outDataLenth > SEND_BUFFER)
				throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

			{
				std::lock_guard<std::mutex> lock(_mtxData);

				_deqData.push_back(std::make_tuple(LOGINCMDRESPON, outDataLenth + 2, std::move(data)));
			}
		
		End:
			return ret;
		}

		//���տͻ��˵����ݰ�, recvBuf: ����: ��ʶ�� + ��ͷ + ���� + У���� + ��ʶ��;   recvLen : recvBuf���ݵĳ���
		//				sendBuf : �������ݰ�, ��Ҫ���͵�����;  sendLen : �������ݵĳ���
		int VirtualScannerSession::read_data_proce(char *recvBuf, int recvLen, int index, int &nCmd)
		{
			int		ret = 0;
			uint8_t	cmd = 0;

			//1.��ȡ������
			cmd = *((uint8_t *)recvBuf);
			nCmd = cmd;

			//2.ѡ����ʽ
			switch (cmd) {
			case LOGINCMDREQ: 		//��¼���ݰ�
				std::cout << "login_func_reply\r\n";
				if ((ret = login_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func login_func_reply() ");
				break;
			case LOGOUTCMDREQ: 		//�˳����ݰ�
				std::cout << "exit_func_reply\r\n";
				if ((ret = exit_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func exit_func_reply() ");
				break;
			case HEARTCMDREQ: 		//�������ݰ�
				std::cout << "heart_beat_func_reply\r\n";
				if ((ret = heart_beat_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;// myprint("Error: func heart_beat_func_reply() ");
				break;
			case DOWNFILEREQ: 		//ģ������
				std::cout << "downLoad_template_func_reply\r\n";
				if ((ret = downLoad_template_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func downLoad_template_func_reply() ");
				break;
			case NEWESCMDREQ: 		//�ͻ�����Ϣ����
				std::cout << "get_FileNewestID_reply\r\n";
				if ((ret = get_FileNewestID_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func client_request_func_reply() ");
				break;
			case UPLOADCMDREQ: 		//�ϴ�ͼƬ���ݰ�
				//std::cout << "upload_func_reply\r\n";
				if ((ret = upload_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func upload_func_reply() ");
				break;
			case PUSHINFOREQ:		//��Ϣ����
				std::cout << "push_info_toUi_reply\r\n";
				if ((ret = push_info_toUi_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func push_info_toUi_reply() ");
				break;
			case DELETECMDREQ: 		//ɾ��ͼƬ���ݰ�
				std::cout << "delete_func_reply\r\n";
				if ((ret = delete_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func delete_func_reply() ");
				break;
			case TEMPLATECMDREQ:		//�ϴ�ģ��
				std::cout << "template_extend_element_reply\r\n";
				if ((ret = template_extend_element_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func template_extend_element() ");
				break;
			case MUTIUPLOADCMDREQ:		//�ϴ�ͼƬ��
				//std::cout << "upload_template_set_reply\r\n";
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
				return -1;
			}

			return ret;
		}

		//function: find  the whole packet; retval maybe error
		int	VirtualScannerSession::find_whole_package(const char *recvBuf, int recvLenth, int index, int &nCmd)
		{
			int		ret = -1, i = 0;
			int		tmpContentLenth = 0;			//ת�������ݳ���
			char	tmpContent[2800] = { 0 }; 		//ת��������
	
			//recvLenth���Դ���1400��tmpContentLenth����
			if (recvBuf[0] != 0x7e || recvBuf[recvLenth - 1] != 0x7e)
				return -1;

			//2. �������ݱ�β, ��ȡ���������ݰ�, �����ݰ����з�ת�� 
			if ((ret = anti_escape(recvBuf + 1, recvLenth - 2, tmpContent, &tmpContentLenth)) < 0 || tmpContentLenth > 1400)
			{
				//myprint("Err : func anti_escape(), srcLenth : %d, dstLenth : %d", lenth, tmpContentLenth);
				ret = -1;
				goto End;
			}

			reqPackHead_t	&req = *(reqPackHead_t *)tmpContent;

			//3.��ת�������ݰ����д���
			if ((ret = read_data_proce(tmpContent, tmpContentLenth, index, nCmd)) < 0)
			{
				//myprint("Err : func read_data_proce(), dataLenth : %d", tmpContentLenth);
				goto End;
			}

		End:
			return ret;
		}

		int	VirtualScannerSession::find_whole_package2(const char *recvBuf, int recvLenth, int index, int &nCmd)
		{
			int		ret = -1;
			//recvLenth���Դ���1400��tmpContentLenth����
			if (recvBuf[recvLenth - 1] != 0x7e)
				return -1;

			reqPackHead_t	&req = *(reqPackHead_t *)recvBuf;

			//3.��ת�������ݰ����д���
			if ((ret = read_data_proce(const_cast<char *>(recvBuf), recvLenth - 1, index, nCmd)) < 0)
			{
				//myprint("Err : func read_data_proce(), dataLenth : %d", tmpContentLenth);
				goto End;
			}

		End:
			return ret;
		}

		size_t VirtualScannerSession::read_complete(const boost::asio::const_buffer &buffer, const boost::system::error_code & ec, size_t bytes)
		{
			if (ec) 
				return 0;

			//����Ҫ������־λ+��ͷ
			if (bytes < sizeof(reqPackHead_t) + 1)
				return sizeof(reqPackHead_t) + 1 - bytes;

			const char *pBuf = static_cast<const char *>(boost::asio::detail::buffer_cast_helper(buffer));

			if (*pBuf != 0x7e)
				return 0;

			int  tempContentLenth = 0;	
			char *tempContent = _Content;

			//2. ������һ����־λ�� �����ݰ����з�ת�� 
			int ret = 0;
			if ((ret = anti_escape(pBuf + 1, bytes - 1, tempContent, &tempContentLenth)) < 0)
			{
				//myprint("Err : func anti_escape(), srcLenth : %d, dstLenth : %d", lenth, tmpContentLenth);
				return 0;
			}

			_ContentLenth = tempContentLenth;
			if (_ContentLenth > 1400)
				return 0;

			//��ͷ���ܰ���ת���ַ������ʵ�ʳ��Ȼ����sizeof(reqPackHead_t)
			if (_ContentLenth < sizeof(reqPackHead_t))
				return sizeof(reqPackHead_t) - _ContentLenth;

			//�������������ݡ�У��ͣ�int����������־
			const reqPackHead_t &req = *(reqPackHead_t *)(_Content);
			int contLen = req.contentLenth;
			if (contLen == 0 || contLen > 1400)
				return 0;

			int all = 1 + contLen + sizeof(int) + 1;
			if (all > 1400)
				return 0;

			//�����а���ת���ַ������ʵ�ʳ��Ȼ����all
			if (_ContentLenth + 1 < all)
				return all - _ContentLenth - 1;
			else
				return 0;
		}

		void VirtualScannerSession::do_read()
		{
			std::cout << "do_read\r\n";

			auto self(shared_from_this());
#if IF_USE_ASYNC
			socket_.async_read_some(boost::asio::buffer(data_),
#else
			boost::asio::async_read(socket_, boost::asio::buffer(data_),
				std::bind(&VirtualScannerSession::read_complete, this, boost::asio::buffer(data_), std::placeholders::_1, std::placeholders::_2),
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

						//��Ϣ���ˣ��ȴ� or just close socket
						std::cout << "_mapSock2Data����\n";

					} while (std::this_thread::sleep_for(std::chrono::seconds(1)), true);

					//�Ȼ��棬���Ƿ�װ��һ��mgr��
					{
						std::lock_guard<std::mutex> lock(_mtxSock2Data);
						auto itr = _mapSock2Data.find(t.socket_);
						if (itr == std::end(_mapSock2Data))
						{
							deq_data_t	deq;

							deq.push_back(std::make_pair(MS_TYPE::Read_MSG, data_));
							_mapSock2Data[t.socket_] = std::move(deq);
						}
						else
						{
							auto &deq = _mapSock2Data[t.socket_];
						/*	std::for_each(std::begin(deq), std::end(deq), [](auto data) {

							});*/

							deq.push_back(std::make_pair(MS_TYPE::Read_MSG, data_));
						}
					}

					bool bNeedRead = false;
					//parse data
					
					bNeedRead = true;
					if(bNeedRead)
					{
						//û��������������
						do_read();
					}
					else
						do_write(length);
#else
					std::cout << "on_read\r\n";

					int cmd = 0;
					//���������ݣ���д�뻺�档
					if (0 > find_whole_package(data_.data(), length, 0, cmd))
					{
						std::cout << "find_whole_package failed\r\n";
						//������������
						return;
						//do_read();
					}
					else
					{
						std::cout << cmd;
						std::cout << "find_whole_package succeed\r\n";

						bool hasData = false;
						{
							std::lock_guard<std::mutex> lock(_mtxData);
							hasData = !_deqData.empty();
						}

						if (hasData)
							do_multiwrite(1);
						else
							do_read();
					}
#endif
				}
			}));
		}

		void VirtualScannerSession::do_multiwrite(std::size_t length)
		{
			std::cout << "do_multiwrite\r\n";

			auto self(shared_from_this());
			auto func = [this, self](boost::system::error_code ec, std::size_t length2)
			{
				if (!ec)
				{
					std::cout << "on_multiwrite\r\n";

					//assert(typeid(socket_.get_io_service()) == typeid(boost::asio::stream_socket_service<tcp>));
					bool hasData = false;
					{
						std::lock_guard<std::mutex> lock(_mtxData);

						hasData = !_deqData.empty();
					}

					if (!hasData)
					{
						do_read();
					}
					else
						do_multiwrite(1);
				}
			};
		
			{
				std::lock_guard<std::mutex> lock(_mtxData);

				if (!_deqData.empty())
				{
					boost::asio::async_write(socket_, boost::asio::buffer(std::get<2>(_deqData.front()), std::get<1>(_deqData.front())),
						make_custom_alloc_handler(allocator_, func));

					_deqData.pop_front();
					return;
				}
				else
				{
					std::cout << "do_multiwrite no data!\r\n";
					return;
				}
			}
		}

		void VirtualScannerSession::do_readNonBlock()
		{
			//std::cout << "do_readNonBlock\r\n";

			auto self(shared_from_this());

			boost::asio::async_read(socket_, boost::asio::buffer(data_),
				std::bind(&VirtualScannerSession::read_complete, this, boost::asio::buffer(data_), std::placeholders::_1, std::placeholders::_2),
				make_custom_alloc_handler(allocator_,
					[this, self](boost::system::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					//std::cout << "on_readNonBlock\r\n";

					int cmd = 0;
					//���������ݣ���д�뻺�档
					//if (0 > find_whole_package(data_.data(), length, 0, cmd))
					if (0 > find_whole_package2(_Content, _ContentLenth, 0, cmd))
					{
						_ContentLenth = 0;
						memset(_Content, 0, sizeof(_Content));

						std::cout << "find_whole_package failed\r\n";
						//������������
						return;
						//do_read();
					}
					else
					{
						_ContentLenth = 0;
						memset(_Content, 0, sizeof(_Content));

						//std::cout << cmd;
						//std::cout << "find_whole_package succeed\r\n";

						do_readNonBlock();
					}
				}
			}));
		}

		void VirtualScannerSession::do_readMulti()
		{
			auto self(shared_from_this());
			socket_.async_receive(boost::asio::buffer(data_recv_),
			//boost::asio::async_read(socket_, boost::asio::buffer(data_recv_),
				make_custom_alloc_handler(allocator_,
					[this, self](boost::system::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					//std::cout << length << std::endl;

					int  tmpContentLenth = 0;			//ת�������ݳ���
					char tmpContent[SEND_BUFFER] = { 0 }; 		//ת��������
					int ret = 0, cmd = 0;

					bool bFind = false;
					//if (!_buffer.empty())
					auto pos = _buffer.find(0x7e);
					if (pos != std::string::npos)
						bFind = true;

					for (int i = 0; i < length; ++i)
					{
						char c = data_recv_.data()[i];
						if (0x7e == c && bFind == false)
						{
							bFind = true;
							_buffer.push_back(c);
							continue;
						}
						else if (0x7e == c && bFind)
						{
							auto on_finish = [&]()
							{
								memset(tmpContent, 0, sizeof(tmpContent));
								tmpContentLenth = 0;
								bFind = false;
								_buffer.clear();
							};

							try
							{
								pos = _buffer.find(0x7e);
								if (pos == std::string::npos || _buffer.size() == 1)
									throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

								//2. �������ݱ�β, ��ȡ���������ݰ�, �����ݰ����з�ת�� 
								if ((ret = anti_escape(_buffer.c_str() + pos + 1, _buffer.length() - pos - 1, tmpContent, &tmpContentLenth)) < 0)
								{
									//myprint("Err : func anti_escape(), srcLenth : %d, dstLenth : %d", lenth, tmpContentLenth);
									throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);
								}

								if(tmpContentLenth > SEND_BUFFER)
									throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

								//3.��ת�������ݰ����д���
								if ((ret = read_data_proce(tmpContent, tmpContentLenth, 0, cmd)) < 0)
								{
									std::cout << "read_data_proce failed\r\n";
									//myprint("Err : func read_data_proce(), dataLenth : %d", tmpContentLenth);
									throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);
								}

								//std::cout << cmd;
								//std::cout << "read_data_proce succeed\r\n";
							}
							catch (const GAFIS7::GA7BASE::Ga7Exception& e)
							{
								//4. ���ݰ�����ɹ�, �����ʱ����
								on_finish();

								continue;
							}

							on_finish();
						}
						else
						{
							//ȡ�����ݰ�����, ȥ����ʼ��ʶ��
							_buffer.push_back(c);
							continue;
						}
					}

					do_readMulti();
				}
			}));

		}

		void VirtualScannerSession::do_readSomeNonBlock()
		{
			auto self(shared_from_this());

			socket_.async_read_some(boost::asio::buffer(data_),
				make_custom_alloc_handler(allocator_,
					[this, self](boost::system::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					//std::cout << length << std::endl;

					int  tmpContentLenth = 0;			//ת�������ݳ���
					char tmpContent[SEND_BUFFER] = { 0 }; 		//ת��������
					int ret = 0, cmd = 0;

					bool bFind = false;
					//if (!_buffer.empty())
					auto pos = _buffer.find(0x7e);
					if (pos != std::string::npos)
						bFind = true;

					for (int i = 0; i < length; ++i)
					{
						char c = data_.data()[i];
						if (0x7e == c && bFind == false)
						{
							bFind = true;
							_buffer.push_back(c);
							continue;
						}
						else if (0x7e == c && bFind)
						{
							auto on_finish = [&]()
							{
								memset(tmpContent, 0, sizeof(tmpContent));
								tmpContentLenth = 0;
								bFind = false;
								_buffer.clear();
							};

							try
							{
								pos = _buffer.find(0x7e);
								if (pos == std::string::npos || _buffer.size() == 1)
									throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

								//2. �������ݱ�β, ��ȡ���������ݰ�, �����ݰ����з�ת�� 
								if ((ret = anti_escape(_buffer.c_str() + pos + 1, _buffer.length() - pos - 1, tmpContent, &tmpContentLenth)) < 0)
								{
									//myprint("Err : func anti_escape(), srcLenth : %d, dstLenth : %d", lenth, tmpContentLenth);
									throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);
								}

								if (tmpContentLenth > SEND_BUFFER)
									throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

								//3.��ת�������ݰ����д���
								if ((ret = read_data_proce(tmpContent, tmpContentLenth, 0, cmd)) < 0)
								{
									std::cout << "read_data_proce failed\r\n";
									//myprint("Err : func read_data_proce(), dataLenth : %d", tmpContentLenth);
									throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);
								}

								//std::cout << cmd;
								//std::cout << "read_data_proce succeed\r\n";
							}
							catch (const GAFIS7::GA7BASE::Ga7Exception& e)
							{
								//4. ���ݰ�����ɹ�, �����ʱ����
								on_finish();

								continue;
							}
							
							on_finish();
						}
						else
						{
							//ȡ�����ݰ�����, ȥ����ʼ��ʶ��
							_buffer.push_back(c);
							continue;
						}
					}

					do_readSomeNonBlock();
				}
			}));
		}

		void VirtualScannerSession::do_multiwriteNonBlock(std::size_t length)
		{
			//std::cout << "do_multiwriteNonBlock\r\n";

			auto self(shared_from_this());
			auto func = [this, self](boost::system::error_code ec, std::size_t length2)
			{
				if (!ec)
				{
					//std::cout << "on_multiwriteNonBlock\r\n";

					do_multiwriteNonBlock(0);
				}
			};

			bool hasData = false;
			{
				std::lock_guard<std::mutex> lock(_mtxData);

				hasData = !_deqData.empty();
				if (hasData)
				{
					boost::asio::async_write(socket_, boost::asio::buffer(std::get<2>(_deqData.front()), std::get<1>(_deqData.front())),
						make_custom_alloc_handler(allocator_, func));

					_deqData.pop_front();
					return;
				}
			}

			if (!hasData)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				boost::asio::async_write(socket_, boost::asio::buffer(data_, 0),
					make_custom_alloc_handler(allocator_, func));
			}
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

				}
			}));
		}

		int VirtualScannerSession::push_info_toUi(int index, enum class FILE_TYPE fileType)
		{
			int		ret = 0, outDataLenth = 0, checkNum = 0;
			char	*tmp = NULL, *sendBufStr = NULL;
			char	tmpSendBuf[SEND_BUFFER] = { 0 };

			resCommonHead_t	head = { 0 };

			type_data_t	data;
			sendBufStr = data.data();
			//myprint("The queue have element number : %d ...", get_element_count_send_block(_sendData_addrAndLenth_queue));

			//3.package body news
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			if (fileType == FILE_TYPE::FILE_TEMPLATE)
			{
				*tmp++ = static_cast<int>(fileType);
				memcpy(tmp, &_svr._fileTemplateID, sizeof(long long int));
				_svr._fileTemplateID++;
			}
			else if (fileType == FILE_TYPE::FILE_TABLE)
			{
				*tmp++ = static_cast<int>(fileType);
				memcpy(tmp, &_svr._fileTableID, sizeof(long long int));
				_svr._fileTableID++;
			}
			
			outDataLenth = sizeof(resCommonHead_t) + 65;

			//4.package The head news	
			assign_comPack_head(&head, PUSHINFORESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			//û�ط��ã�ע�͵�
			//_thid_sockfd_block[index].send_flag++;

			//5.calculation checkCode
			tmp = tmpSendBuf + head.contentLenth;
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//6. ת����������
			ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth);
			if (ret)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//7. ��Ϸ��͵�����
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			if (outDataLenth > SEND_BUFFER)
				throw GAFIS7::GA7BASE::Ga7Exception(__FILE__, __LINE__);

			{
				std::lock_guard<std::mutex> lock(_mtxData);

				_deqData.push_back(std::make_tuple(PUSHINFORESPON, outDataLenth + 2, std::move(data)));
			}

			std::cout << "push_info_toUi\r\n";
			//��������������multiwriteһ��д���ɡ�����Ҫÿ��putui������multiwrite��ô�����ˡ�

		End:
			return ret;
		}

		void VirtualScannerSession::Write_Thread()
		{

		}

		void VirtualScannerSvr::find_directory_file()
		{
			static bool	flag = false;

			auto getFiles = [](const std::string &path, std::vector<_finddata_t> &files)
			{
				_finddata_t	c_file;
				intptr_t	hFile;
				char		buf[512] = { 0 };

				if (_chdir(path.c_str()))
				{
					printf("Unable to locate the directory: %s\n", path.c_str());
					return -1;
				}
				else
					hFile = _findfirst("*.*", &c_file);

				//ctime_s(buf, sizeof(buf), &(c_file.time_write));
				//printf(" %-30s %.20s  %9ld\n", c_file.name, buf, c_file.size);
				if (strcmp(c_file.name, ".") != 0 && strcmp(c_file.name, "..") != 0)
					files.push_back(c_file);

				/* Find the rest of the files */
				while (_findnext(hFile, &c_file) == 0)
				{
					if (strcmp(c_file.name, ".") != 0 && strcmp(c_file.name, "..") != 0)
						files.push_back(c_file);

					//ctime_s(buf, sizeof(buf), &(c_file.time_access));
					//printf(" %-30s %.20s  %9ld\n", c_file.name, buf, c_file.size);
				}
				
				_findclose(hFile);
				return 0;
			};

			{
				std::lock_guard<std::mutex> lock(_mtxFiles);

				std::vector<struct _finddata_t> files;
				getFiles(_downLoadDir, files);

				//4.������ȡ��Ŀ¼�µ��ļ�, ��ȡ���ǵ�ʱ������
				std::for_each(std::begin(files), std::end(files), [this](auto &f) {

					//4.3 ���δ򿪸�Ŀ¼, �������ļ�������������
					if (!flag)
					{
						_vctfiles.push_back(f);

						{
							std::lock_guard<std::mutex> lock(_mtxSessions);
							std::for_each(std::begin(_deqSessions), std::end(_deqSessions), [this, &f](auto &s)
							{
								if (_fileNameTemplate == f.name)
									s->push_info_toUi(0, FILE_TYPE::FILE_TEMPLATE);
								else if (_fileNameTable == f.name)
									s->push_info_toUi(0, FILE_TYPE::FILE_TABLE);
							});
						}
					}
					else		//���״�, ���������ж�
					{
						bool bFind = false;
						for (auto &_f : _vctfiles)
						{
							if (0 != strcmp(_f.name, f.name))
								continue;

							bFind = true;
							break;
						}

						//������û��
						if (!bFind)
						{
							_vctfiles.push_back(f);
							
							enum class FILE_TYPE filyType;
							if (_fileNameTemplate == f.name)
								filyType = FILE_TYPE::FILE_TEMPLATE;
							else if (_fileNameTable == f.name)
								filyType = FILE_TYPE::FILE_TABLE;
							else
								goto End;

							{
								std::lock_guard<std::mutex> lock(_mtxSessions);
								std::for_each(std::begin(_deqSessions), std::end(_deqSessions), [&filyType](auto &s)
								{
									s->push_info_toUi(0, filyType);
								});
							}
						}
						else
						{
							enum class FILE_TYPE filyType;
							for (auto &_f : _vctfiles)
							{
								if (0 != strcmp(_f.name, f.name))
									continue;

								if (_f.time_access == f.time_access)
									continue;

								if (_fileNameTemplate == _f.name)
									filyType = FILE_TYPE::FILE_TEMPLATE;
								else if (_fileNameTable == _f.name)
									filyType = FILE_TYPE::FILE_TABLE;
								else
									goto End;

								{
									std::lock_guard<std::mutex> lock(_mtxSessions);
									std::for_each(std::begin(_deqSessions), std::end(_deqSessions), [&filyType](auto &s)
									{
										s->push_info_toUi(0, filyType);
									});
								}

								_f.time_access = f.time_access;
							}
						}
					}
				End:
					return;
				});

				flag = true;
			}
		}

		//select() ��ʱ�� 
		void timer_select(int seconds)
		{
			/*struct timeval temp;
			temp.tv_sec = seconds;
			temp.tv_usec = 0;
			int err = 0;
			do {
				err = select(0, NULL, NULL, NULL, (struct timeval *)&temp);
			} while (err < 0 && errno == EINTR);*/

			std::this_thread::sleep_for(std::chrono::seconds(seconds));
		}

		void child_scan_func(VirtualScannerSvr *svr)
		{
			while (!svr->_bStop)
			{
				svr->find_directory_file();
				timer_select(SCAN_TIME);
			}
		}

		int VirtualScannerSvr::init()
		{
			int	ret = 0;

			//1.�鿴�ϴ�Ŀ¼�Ƿ����
			if (_access(_DirPath.c_str(), 0) < 0)
			{
				//2.������, ���������ϴ��ļ���Ŀ¼
				if ((ret = _mkdir(_DirPath.c_str()/*, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH*/)) < 0)
				{
					//myprint("Error : func mkdir() : %s", uploadDirParh);
					return ret;
				}
			}

			return 0;
		}

		GA2NDIDSVRLIB_API int main1(int argc, char* argv[])
		{
			try
			{
				int port = argc > 1 ? std::atoi(argv[1]) : SERVER_PORT;
				
				boost::asio::io_service	io_service;
				VirtualScannerSvr		server(io_service, port);
				
				server.init();

				int	nThreadCount = 5;

				auto handle_thread = [&]()
				{
					io_service.run();
				};

				for (int i = 0; i < nThreadCount; ++i)
					server._vctThreads.push_back(std::thread(handle_thread));

				for (auto &t : server._vctThreads)
				{
					SetThreadPriority(t.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
				}

				//�������̣߳� ȥִ��ɨ��Ŀ¼ҵ��, ������Ϣ����
				server._vctThreads.push_back(std::thread(child_scan_func, &server));

				std::for_each(std::begin(server._vctThreads), std::end(server._vctThreads), [](auto &t)
				{
					t.join();
				});

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





