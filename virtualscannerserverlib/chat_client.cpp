//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <thread>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class chat_client
{
public:
	chat_client(boost::asio::io_service& io_service,
		tcp::resolver::iterator endpoint_iterator)
		: io_service_(io_service),
		socket_(io_service)
	{
		do_connect(endpoint_iterator);
	}

	void write(const std::string& msg)
	{
		io_service_.post(
			[this, msg]()
		{
			memcpy(write_msg_.data(), msg.data(), msg.size());
			do_write(msg.size());
		});
	}

	void close()
	{
		io_service_.post([this]() { socket_.close(); });
	}

private:
	void do_connect(tcp::resolver::iterator endpoint_iterator)
	{
		boost::asio::async_connect(socket_, endpoint_iterator,
			[this](boost::system::error_code ec, tcp::resolver::iterator)
		{
			if (!ec)
			{
				//do_read();
			}
		});
	}

	void do_read()
	{
		boost::asio::async_read(socket_,
			boost::asio::buffer(read_msg_),
			[this](boost::system::error_code ec, std::size_t /*length*/)
		{
			if (!ec)
			{
				do_write(0);
			}
			else
			{
				socket_.close();
			}
		});
	}

	void do_write(int nlen)
	{
		boost::asio::async_write(socket_,
			boost::asio::buffer(write_msg_, nlen),
			[this](boost::system::error_code ec, std::size_t /*length*/)
		{
			if (!ec)
			{
				do_read();
			}
			else
			{
				socket_.close();
			}
		});
	}

private:
	boost::asio::io_service& io_service_;
	tcp::socket socket_;
	std::array<char, 2800> read_msg_;
	std::array<char, 2800> write_msg_;
};

#include "virtualscannersvrmain.h"
#include "virtualscannersvr.h"
namespace DROWNINGLIU
{

	namespace VSCANNERSVRLIB
	{
		extern int  escape(char sign, const char *inData, int inDataLenth, char *outData, int *outDataLenth);
		extern int crc326(const  char *buf, uint32_t size);

#define ACCOUTLENTH 			11							//客户端账户长度

		void assign_reqPack_head(reqPackHead_t *head, uint8_t cmd, int contentLenth, int isSubPack, int currentIndex, int totalNumber, int perRoundNumber)
		{
			memset(head, 0, sizeof(reqPackHead_t));
			head->cmd = cmd;
			head->contentLenth = contentLenth;
		//	strcpy((char *)head->machineCode, g_machine);
			head->isSubPack = isSubPack;
			head->totalPackage = totalNumber;
			head->currentIndex = currentIndex;
			head->perRoundNumber = perRoundNumber;
		/*	pthread_mutex_lock(&g_mutex_serial);
			head->seriaNumber = g_seriaNumber++;
			pthread_mutex_unlock(&g_mutex_serial);*/
		}

		GA2NDIDSVRLIB_API int main_client(int argc, char* argv[])
		{
			try
			{
				if (argc != 3)
				{
					std::cerr << "Usage: chat_client <host> <port>\n";
					return 1;
				}

				boost::asio::io_service io_service;

				tcp::resolver resolver(io_service);
				auto endpoint_iterator = resolver.resolve({ argv[1], argv[2] });
				chat_client c(io_service, endpoint_iterator);

				std::string msg;

				auto login = [&msg]()
				{
					char	szBuf[2800] = { 0 };
					char	szBuf2[2800] = { 0 };

					int ret = 0;
					char *tmp = NULL;
					char accout[11] = { 0 };
					char passWd[20] = "qgdyuwgqu455";
					uint8_t passWdSize = strlen(passWd);
					reqPackHead_t head;
					char *tmpSendPackDataOne = szBuf, *sendPackData = szBuf2;
					int  checkCode = 0, contenLenth = 0;
					int outDataLenth = 0;

					//3.计算数据包总长度, 并组装报头
					contenLenth = sizeof(reqPackHead_t) + ACCOUTLENTH + sizeof(char) + passWdSize;
					assign_reqPack_head(&head, LOGINCMDREQ, contenLenth, NOSUBPACK, 0, 1, 1);

					//4.组装数据包信息
					tmp = tmpSendPackDataOne;
					memcpy(tmp, (char *)&head, sizeof(reqPackHead_t));
					tmp += sizeof(reqPackHead_t);
					memcpy(tmp, accout, ACCOUTLENTH);
					tmp += ACCOUTLENTH;
					*tmp++ = passWdSize;
					memcpy(tmp, passWd, passWdSize);

					//5.计算校验码
					checkCode = crc326((const char*)tmpSendPackDataOne, head.contentLenth);
					tmp += passWdSize;
					memcpy(tmp, &checkCode, sizeof(int));

					//6.转义数据包内容
					*sendPackData = PACKSIGN;
					if ((ret = escape(PACKSIGN, tmpSendPackDataOne, head.contentLenth + sizeof(int), sendPackData + 1, &outDataLenth)) < 0)
					{
						//myprint("Error: func escape()");
						assert(0);
					}
					*(sendPackData + outDataLenth + sizeof(char)) = PACKSIGN;

					//escape
					msg.assign(sendPackData, outDataLenth + sizeof(char) * 2);
				};

				login();

				c.write(msg);

				std::thread t([&io_service]() { io_service.run(); });

				c.close();
				t.join();
			}
			catch (std::exception& e)
			{
				std::cerr << "Exception: " << e.what() << "\n";
			}

			return 0;
		}
	}
}