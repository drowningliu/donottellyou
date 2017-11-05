#pragma once

//必须在windows.h之前 ！！！！
#include <WinSock2.h>
#include <windows.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <boost/asio.hpp>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <io.h>

namespace DROWNINGLIU
{
	namespace VSCANNERSVRLIB
	{
		using boost::asio::ip::tcp;
#define IF_USE_ASYNC	0
#define FAST_IO	1
		enum class MSG_TYPE
		{
			Read_MSG = 1,
			Write_MSG = 2
		};

		enum class FILE_TYPE
		{
			FILE_TEMPLATE = 0,
			FILE_TABLE = 1
		};

#define SERVER_IP		"127.0.0.1"	//服务器IP
#define SERVER_PORT		8009			//服务器端�?

#pragma pack(push)
#pragma pack(1)

		//扫描仪请求数据包报头
		typedef struct _reqPackHead_t {
			uint8_t         cmd;                //命令字
			uint16_t        contentLenth;       //长度； 包含：报头和消息体长度
			uint8_t         isSubPack;          //判断是否为子包, 0 不是, 1 是;
			uint8_t         machineCode[12];    //机器码
			uint32_t        seriaNumber;        //流水号
			uint16_t        currentIndex;       //当前包序号
			uint16_t        totalPackage;       //总包数
			uint8_t         perRoundNumber;     //本轮传输的数据包个数
		}reqPackHead_t;
		//1 + 2 + 1 + 12 + 4 + 2 + 2 + 1 = 25;

		//服务器的普通应答
		typedef struct _resCommonHead_t {
			uint8_t     cmd;                //命令字
			uint16_t    contentLenth;       //长度； 包含：报头和消息体长度
			uint8_t     isSubPack;          //判断是否为子包, 			0 不是, 1 是;
			uint32_t    seriaNumber;        //流水号
			uint8_t     isFailPack;     	//是否失败					0成功, 1 失败
			uint16_t    failPackIndex;      //失败数据包的序列号
			uint8_t		isRespondClient;	//是否需要客户端作出界面响应,  0 不需要, 1 需要作出界面响应	
		}resCommonHead_t;
		//1 + 2 + 1 + 4 + 1 + 2 + 1 = 12

		//从服务器下载文件应答
		typedef struct _resSubPackHead_t {
			uint8_t     cmd;                //命令字
			uint16_t    contentLenth;       //长度； 包含：报头和消息体长度
			uint32_t    seriaNumber;        //流水号
			uint16_t    currentIndex;       //当前包序号
			uint16_t    totalPackage;       //总包数
		}resSubPackHead_t;
		//1 + 2 + 4 + 2 + 2 = 11;

#pragma pack(pop)

#define PACKMAXLENTH  	1400		 //报文的最大长度;		报文结构：标识 1， 头 26，体， 校验 4， 标识 1
#define COMREQDATABODYLENTH 	PACKMAXLENTH - sizeof(reqPackHead_t) - sizeof(char) * 2 - sizeof(int)         //报体长度最大: 1400-25-2-4 = 1369;  当传输图片时, 图片名在消息体中占46个字节
#define REQTEMPERRBODYLENTH		PACKMAXLENTH - sizeof(reqTemplateErrHead_t) - sizeof(char) * 2 - sizeof(int)
#define COMRESBODYLENTH			PACKMAXLENTH - sizeof(resCommonHead_t) - sizeof(char) * 2 - sizeof(int)
#define RESTEMSUBBODYLENTH		PACKMAXLENTH - sizeof(resSubPackHead_t) - sizeof(char) * 2 - sizeof(int)
#define FILENAMELENTH 	46			//文件名称
#define PACKSIGN      	0x7e		//标识符
#define PERROUNDNUMBER	50			//每轮发送的数据包最大数目
#define OUTTIMEPACK 	5			//客户端接收应答的超时时间

		enum Cmd {
			ERRORFLAGMY = 0x00,		//出错
			LOGINCMDREQ = 0x01,		//登录
			LOGINCMDRESPON = 0x81,
			LOGOUTCMDREQ = 0x02,		//登出
			LOGOUTCMDRESPON = 0x82,
			HEARTCMDREQ = 0x03,		//心跳
			HEARTCMDRESPON = 0x83,
			DOWNFILEREQ = 0x04,		//下载模板
			DOWNFILEZRESPON = 0x84,
			NEWESCMDREQ = 0x05,		//获取模板最新编号
			NEWESCMDRESPON = 0x85,
			UPLOADCMDREQ = 0x06,		//上传图片
			UPLOADCMDRESPON = 0x86,
			PUSHINFOREQ = 0x07, 	//消息推送
			PUSHINFORESPON = 0x87,
			ACTCLOSEREQ = 0x08, 	//主动关闭
			ACTCLOSERESPON = 0x88,
			DELETECMDREQ = 0x09, 	//删除图片
			DELETECMDRESPON = 0x89,
			CONNECTCMDREQ = 0x0A, 	//链接服务器
			CONNECTCMDRESPON = 0x8A,
			MODIFYCMDREQ = 0x0B, 	//修改配置文件
			MODIFYCMDRESPON = 0x8B,
			TEMPLATECMDREQ = 0x0C, 	//模板数据包
			TEMPLATECMDRESPON = 0x8C,
			MUTIUPLOADCMDREQ = 0x0D, 	//多张图片传输
			MUTIUPLOADCMDRESPON = 0x8D,
			ENCRYCMDREQ = 0x0E, 	//加密传输
			ENCRYCMDRESPON = 0x8E
		};

		enum HeadNews {
			NOSUBPACK = 0,		//非子包
			SUBPACK = 1			//子包
		};
#define PACKSIGN      	0x7e		//标识符

		typedef struct _reqTemplateErrHead_t {
			uint8_t     cmd;                //命令字
			uint16_t    contentLenth;       //长度； 包含：报头和消息体长度
			uint8_t     isSubPack;          //判断是否为子包, 0 不是, 1 是;
			uint8_t     machineCode[12];    //机器码
			uint32_t    seriaNumber;        //流水号
			uint8_t     failPackNumber;     //是否失败
			uint16_t    failPackIndex;      //失败数据包的序列号
		}reqTemplateErrHead_t;

#define 	NUMBER		15											//同一时间可以接收的客户端数
#define     SCAN_TIME 	15											//扫描间隔时间 15s


		// Class to manage the memory to be used for handler-based custom allocation.
		// It contains a single block of memory which may be returned for allocation
		// requests. If the memory is in use when an allocation request is made, the
		// allocator delegates allocation to the global heap.
		class handler_allocator
		{
		public:
			handler_allocator()
				: in_use_(false)
			{
			}

			handler_allocator(const handler_allocator&) = delete;
			handler_allocator& operator=(const handler_allocator&) = delete;

			void* allocate(std::size_t size)
			{
				if (!in_use_ && size < sizeof(storage_))
				{
					in_use_ = true;
					return &storage_;
				}
				else
				{
					return ::operator new(size);
				}
			}

			void deallocate(void* pointer)
			{
				if (pointer == &storage_)
				{
					in_use_ = false;
				}
				else
				{
					::operator delete(pointer);
				}
			}

		private:
			// Storage space used for handler-based custom memory allocation.
			typename std::aligned_storage<2804>::type storage_;

			// Whether the handler-based custom allocation storage has been used.
			bool in_use_;
		};

		// Wrapper class template for handler objects to allow handler memory
		// allocation to be customised. Calls to operator() are forwarded to the
		// encapsulated handler.
		template <typename Handler>
		class custom_alloc_handler
		{
		public:
			custom_alloc_handler(handler_allocator& a, Handler h)
				: allocator_(a),
				handler_(h)
			{
			}

			template <typename ...Args>
			void operator()(Args&&... args)
			{
				handler_(std::forward<Args>(args)...);
			}

			friend void* asio_handler_allocate(std::size_t size,
				custom_alloc_handler<Handler>* this_handler)
			{
				return this_handler->allocator_.allocate(size);
			}

			friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/,
				custom_alloc_handler<Handler>* this_handler)
			{
				this_handler->allocator_.deallocate(pointer);
			}

		private:
			handler_allocator& allocator_;
			Handler handler_;
		};

		// Helper function to wrap a handler object to add custom allocation.
		template <typename Handler>
		inline custom_alloc_handler<Handler> make_custom_alloc_handler(
			handler_allocator& a, Handler h)
		{
			return custom_alloc_handler<Handler>(a, h);
		}

		class session
			: public std::enable_shared_from_this<session>
		{
		public:
			session(tcp::socket socket)
				: socket_(std::move(socket))
			{
			}

			virtual void start()
			{
				do_read();
			}

			typedef std::array<char, 2800>	type_data_t;
			//typedef std::string	type_data_t;

		protected:
			virtual void do_read()
			{
				auto self(shared_from_this());
				socket_.async_read_some(boost::asio::buffer(data_),
					make_custom_alloc_handler(allocator_,
						[this, self](boost::system::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						do_write(length);
					}
				}));
			}

			virtual void do_write(std::size_t length)
			{
				auto self(shared_from_this());
				boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
					make_custom_alloc_handler(allocator_,
						[this, self](boost::system::error_code ec, std::size_t /*length*/)
				{
					if (!ec)
					{
						do_read();
					}
				}));
			}

			// The socket used to communicate with the client.
			tcp::socket socket_;

			// Buffer used to store data received from the client.
			type_data_t data_;
			int nLen2Write_ = 0;
			int cmd_ = 0;

			// The allocator to use for handler-based custom memory allocation.
			handler_allocator allocator_;
		};

		template<class T>
		class server
		{
		public:
			server(boost::asio::io_service& io_service, short port)
				: acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
				socket_(io_service)
			{
				do_accept();
			}

			std::deque<std::shared_ptr<T>>	_deqSessions;
			mutable std::mutex				_mtxSessions;	//consider using rwlock

			mutable std::mutex			_mtxFiles;
			std::vector<_finddata_t>	_vctfiles;
			
			//服务器资源文件ID 
			__int64	_fileTemplateID = 562;
			__int64	_fileTableID = 365;
			//上传文件路径
			std::string	_DirPath = "d:\\uploadTest";
			//服务器资源路径
			std::string	_downLoadDir = "d:\\serversource";
			//数据库表名称
			std::string	_fileNameTable = "NHi1200_database.db";
			//模板名称
			std::string	_fileNameTemplate = "info.xml";

			std::vector<std::thread>	_vctThreads;
		private:
			void do_accept()
			{
				std::cout << "do_accept\r\n";

				acceptor_.async_accept(socket_,
					[this](boost::system::error_code ec)
				{
					if (!ec)
					{
						std::cout << "on_accept\r\n";

						//std::make_shared<session>(std::move(socket_))->start();
						auto session = std::make_shared<T>(std::move(socket_), *this);
						int count = session.use_count();
						session->start();

						_deqSessions.push_back(session);
						/*std::for_each(std::begin(_deqSessions), std::end(_deqSessions), [](auto &s) {
							s->
							t.use_count();
						});*/

						{
							std::lock_guard<std::mutex> lock(_mtxSessions);
							//清理一个不再引用的session
							auto itr = _deqSessions.begin();
							for (; itr != _deqSessions.end(); ++itr)
							{
								if (itr->use_count() == 1)
								{
									_deqSessions.erase(itr);
									std::cout << "erase session\r\n";
									break;
								}
							}
						}
					}

					do_accept();
				});
			}

			tcp::acceptor acceptor_;
			//std::shared_ptr<tcp::socket> socket_;
			
			tcp::socket socket_;
		};

		class VirtualScannerSession : public session
		{
		public:
			VirtualScannerSession(tcp::socket socket, server<VirtualScannerSession> &svr)
				: session(std::move(socket)), _svr(svr)
			{
				/*_svr._vctThreads.push_back(
					std::thread(
						std::bind(&VirtualScannerSession::Write_Thread, this)
					));*/
			}
			virtual void start() override
			{
				//do_read();
				do_multiwriteNonBlock(0);
				//do_readNonBlock();
				do_readSomeNonBlock();
			}

			//服务器 接收的 数据包流水号
			unsigned int	_recvSerial = 0;
			//服务器 发送的 数据包流水号
			unsigned int 	_sendSerial = 0;

			server<VirtualScannerSession> &_svr;

			int  _ContentLenth = 0;			//转义后的数据长度
												//比1400长一倍，以防溢出
			char _Content[2800] = { 0 }; 		//转义后的数据

			//cmd len data
			typedef std::tuple<int, int, type_data_t> tuple_data_t;
			typedef std::pair<int, type_data_t> pair_data_t;

			//std::lock_guard<std::mutex> lock(_mtxData);
			mutable std::mutex	_mtxData;	//consider using rwlock
			typedef std::deque<tuple_data_t> deq_data_t;
			deq_data_t	_deqData;

			
			std::string _buffer;


			virtual void do_read() override;
			virtual void do_write(std::size_t length) override;
			void do_multiwrite(std::size_t length);
			void do_multiwriteNonBlock(std::size_t length);
			void do_readNonBlock();

			void do_readSomeNonBlock();

			size_t read_complete(const boost::asio::const_buffer &buffer, const boost::system::error_code & ec, size_t bytes);
		public:
			int getSocketFD()
			{
				auto service = (boost::asio::stream_socket_service<tcp> *)&socket_.get_io_service();

#if defined(BOOST_ASIO_HAS_IOCP)
				boost::asio::detail::win_iocp_socket_service<tcp>::implementation_type t;
#else
				boost::asio::detail::reactive_socket_service<tcp>::implementation_type t;
#endif
				service->native_handle(t);

				return t.socket_;
			}

			int	find_whole_package(const char *recvBuf, int recvLenth, int index, int &nCmd);
			int	find_whole_package2(const char *recvBuf, int recvLenth, int index, int &nCmd);
			int login_func_reply(char *recvBuf, int recvLen, int index);
			int read_data_proce(char *recvBuf, int recvLen, int index, int &nCmd);

			int exit_func_reply(char *recvBuf, int recvLen, int index);

			int downLoad_template_func_reply(char *recvBuf, int recvLen, int index);
			int get_FileNewestID_reply(char *recvBuf, int recvLen, int index);
			int upload_func_reply(char *recvBuf, int recvLen, int index);

			int delete_func_reply(char *recvBuf, int recvLen, int index);
			int template_extend_element_reply(char *recvBuf, int recvLen, int index);
			int upload_template_set_reply(char *recvBuf, int recvLen, int index);

			int heart_beat_func_reply(char *recvBuf, int recvLen, int index);
			int push_info_toUi(int index, enum class FILE_TYPE fileType);

			int save_multiPicture_func(char *recvBuf, int recvLen);
			int save_picture_func(char *recvBuf, int recvLen);
			int compare_recvData_correct_server(char *tmpContent, int tmpContentLenth);
			void assign_serverSubPack_head(resSubPackHead_t *head, uint8_t cmd, int contentLenth, int currentIndex, int totalNumber);
			void assign_comPack_head(resCommonHead_t *head, int cmd, int contentLenth, int isSubPack, int isFailPack, int failPackIndex, int isRespond);

			void Write_Thread();
		};

		class VirtualScannerSvr : public server<VirtualScannerSession>
		{
		public:
			VirtualScannerSvr(boost::asio::io_service& io_service, short port)
				: server(io_service, port)
			{

			}

			bool _bStop = false;

			int init();
			void find_directory_file();
		};

	}
}