#pragma once
#include <string>
#include <deque>
#include <array>

namespace DROWNINGLIU
{
	namespace SCANNERCLIENTLIB
	{
#define PACKMAXLENTH  	1400		 //报文的最大长度;		报文结构：标识 1， 头 26，体， 校验 4， 标识 1
#define COMREQDATABODYLENTH 	PACKMAXLENTH - sizeof(reqPackHead_t) - sizeof(char) * 2 - sizeof(int)         //报体长度最大: 1400-25-2-4 = 1369;  当传输图片时, 图片名在消息体中占46个字节
#define REQTEMPERRBODYLENTH		PACKMAXLENTH - sizeof(reqTemplateErrHead_t) - sizeof(char) * 2 - sizeof(int)
#define COMRESBODYLENTH			PACKMAXLENTH - sizeof(resCommonHead_t) - sizeof(char) * 2 - sizeof(int)
#define RESTEMSUBBODYLENTH		PACKMAXLENTH - sizeof(resSubPackHead_t) - sizeof(char) * 2 - sizeof(int)
#define FILENAMELENTH 	46			//文件名称
#define PACKSIGN      	0x7e		//标识符
#define PERROUNDNUMBER	50			//每轮发送的数据包最大数目
#define OUTTIMEPACK 	5			//客户端接收应答的超时时间

		enum HeadNews {
			NOSUBPACK = 0,		//非子包
			SUBPACK = 1			//子包
		};

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

#define SEND_BUFFER 1500

		class ScannerClient
		{
		public:
			ScannerClient();
			virtual ~ScannerClient();

			/*从UI界面接收消息
			*参数：news:接收的消息;  包含内容：1.该条消息的命令字，unsigned char型
			*									2.消息发送服务器是否加密（char型 0x00.加密， 0x01.不加密），
			* 									3.消息的具体内容。
			*						消息格式：11accout:123213123passwd:qgdyuwgqu455;   (描述：登录命令，不加密，账号：123213123，密码:qgdyuwgqu455)
			*返回值：成功 0；   失败 -1；
			*/
			int  recv_ui_data(const std::string &msg);

			/*将解包的数据发送给 UI 界面
			*参数： news ：  将信息拷贝给该地址；   被调函数分配内存
			*返回值： 0 成功；  -1 失败；
			*/
			int  send_ui_data(char **buf);

			typedef std::array<char, SEND_BUFFER>	type_data_t;

			std::deque<std::string> _deqMsg;
			std::deque<std::string> _deqSendData;

			int copy_the_ui_cmd_news(const std::string &msg, std::string &news);

			int  login_func(const std::string &msg);
			int  exit_program(const std::string &msg);
			int download_file_fromServer(const std::string &msg);
		};

	}
};