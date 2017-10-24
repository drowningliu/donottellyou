#ifndef _SOCK_PACK_H_
#define _SOCK_PACK_H_


#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#define PACKMAXLENTH  	1400		 //报文的最大长度;		报文结构：标识 1， 头 26，体， 校验 4， 标识 1
#define COMREQDATABODYLENTH 	PACKMAXLENTH - sizeof(reqPackHead_t) - sizeof(char) * 2 - sizeof(int)         //报体长度最大: 1400-25-2-4 = 1369;  当传输图片时, 图片名在消息体中占46个字节
#define REQTEMPERRBODYLENTH		PACKMAXLENTH - sizeof(reqTemplateErrHead_t) - sizeof(char) * 2 - sizeof(int)
#define COMRESBODYLENTH			PACKMAXLENTH - sizeof(resCommonHead_t) - sizeof(char) * 2 - sizeof(int)
#define RESTEMSUBBODYLENTH		PACKMAXLENTH - sizeof(resSubPackHead_t) - sizeof(char) * 2 - sizeof(int)
#define FILENAMELENTH 	46			//文件名称
#define PACKSIGN      	0x7e		//标识符
#define PERROUNDNUMBER	50			//每轮发送的数据包最大数目
#define OUTTIMEPACK 	5			//客户端接收应答的超时时间

enum HeadNews{
		NOSUBPACK = 0,		//非子包
		SUBPACK = 1			//子包
	};		

enum Cmd{
		ERRORFLAGMY			= 0x00,		//出错
		LOGINCMDREQ 		= 0x01,		//登录
		LOGINCMDRESPON 		= 0x81,
		LOGOUTCMDREQ 		= 0x02,		//登出
		LOGOUTCMDRESPON 	= 0x82,
		HEARTCMDREQ 		= 0x03,		//心跳
		HEARTCMDRESPON 		= 0x83,
		DOWNFILEREQ			= 0x04,		//下载模板
		DOWNFILEZRESPON 	= 0x84,
		NEWESCMDREQ 		= 0x05,		//获取模板最新编号
		NEWESCMDRESPON 		= 0x85,
		UPLOADCMDREQ 		= 0x06,		//上传图片
		UPLOADCMDRESPON 	= 0x86,
		PUSHINFOREQ 		= 0x07, 	//消息推送
		PUSHINFORESPON 		= 0x87,
		ACTCLOSEREQ 		= 0x08, 	//主动关闭
		ACTCLOSERESPON 		= 0x88,
		DELETECMDREQ 		= 0x09, 	//删除图片
		DELETECMDRESPON 	= 0x89,
		CONNECTCMDREQ 		= 0x0A, 	//链接服务器
		CONNECTCMDRESPON 	= 0x8A,
		MODIFYCMDREQ 		= 0x0B, 	//修改配置文件
		MODIFYCMDRESPON 	= 0x8B,
		TEMPLATECMDREQ 		= 0x0C, 	//模板数据包
		TEMPLATECMDRESPON 	= 0x8C,
		MUTIUPLOADCMDREQ	= 0x0D, 	//多张图片传输
		MUTIUPLOADCMDRESPON = 0x8D,
		ENCRYCMDREQ 		= 0x0E, 	//加密传输
		ENCRYCMDRESPON 		= 0x8E
	};

#pragma pack(push)
#pragma pack(1)

//扫描仪请求数据包报头
typedef struct _reqPackHead_t{
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
typedef struct _resCommonHead_t{
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
typedef struct _resSubPackHead_t{
    uint8_t     cmd;                //命令字
    uint16_t    contentLenth;       //长度； 包含：报头和消息体长度
    uint32_t    seriaNumber;        //流水号
    uint16_t    currentIndex;       //当前包序号
    uint16_t    totalPackage;       //总包数
}resSubPackHead_t;
//1 + 2 + 4 + 2 + 2 = 11;




#if 0

/*解释1
分包信息, 分为两种, 一种客户端一次请求,下载整个模板;多包数据应答(模板下载)
客户端每次..

一种客户端多次传输, 服务器一次应答(如: 图片传输)
服务器端 所有的数据包都成功接收:
totalPackge : 将接收到的数据直接赋值到应答
indexPackge : 将本阶段接收到的最后一(即最大的)数据包序列号赋值给它
有数据包接收失败:
failNumber : 接收失败的个数
failRecvPackIndex : 储存失败的数据包序号
*/

//扫描仪接收服务器下载文件子包出错后, 向服务器请求单包数据的报头
typedef struct _reqTemplateErrHead_t{
    uint8_t     cmd;                //命令字
    uint16_t    contentLenth;       //长度； 包含：报头和消息体长度
    uint8_t     isSubPack;          //判断是否为子包, 0 不是, 1 是;
    uint8_t     machineCode[12];    //机器码
    uint32_t    seriaNumber;        //流水号
    uint8_t     failPackNumber;     //是否失败
    uint16_t    failPackIndex;      //失败数据包的序列号
}reqTemplateErrHead_t;
//1 + 2 + 1 + 12 + 4 + 1 + 2 = 23;

#endif


#pragma pack(pop)

#if defined(__cplusplus)
}
#endif

#endif
