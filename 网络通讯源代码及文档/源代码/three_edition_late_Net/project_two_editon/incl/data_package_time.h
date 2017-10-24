#ifndef DATA_PACKAGE_TIME_H
#define DATA_PACKAGE_TIME_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <time.h>
#include "sock_pack.h"

//注册组包的回调函数
typedef int (*call_back_package)(const char *news);
typedef int (*call_back_un_pack)(char *news, char **outDataNews);


//本地保存一套试卷的结构信息
typedef struct _uploadWholeSet{
	char filePath [128];				//上传文件的绝对路径
	unsigned char totalNumber;			//本套试卷的图片总数
	unsigned char indexNumber;			//本张图片的序号, 从 0 开始
	unsigned char result;				//本张图片的上传结果
	unsigned char reason;				//本张图片的出错原因, 上传正确时,该值无效
}uploadWholeSet;

typedef struct _sendDataBlock{
	char 	*sendAddr;		//数据包发送地址
	int 	sendLenth;		//数据包发送长度
	int 	cmd;			//数据包发送命令字
	time_t  sendTime;		//数据包发送时间
}SendDataBlockNews;



// 全局变量初始化
static int init_global_value();
// 创建线程
static int init_pthread_create();


//发送数据至服务器线程
void *thid_server_func_write(void *arg);
//读线程,从服务器读取数据
void *thid_server_func_read(void *arg);
//时间线程, 计时发送数据包的时间
void *thid_server_func_time(void *arg);
//处理UI命令线程
void *thid_business(void *arg);
//心跳线程
void *thid_hreat_beat_time(void *arg);
/*unpack The data */
void *thid_unpack_thread(void *arg);

//重新发送最后一包数据
void repeat_latePack();


//对数据进行处理， 并组成 发送给UI 线程的数据包
//static int data_option(char *storeData, char **sendUiData );
//对接收的服务器数据处理, 参数: 主调函数分配内存
int data_process(char **buf);
/*客户端接收到服务器数据，先进行处理，整理成一个整包，放入接收队列中，发送给UI。
 *参数： recvLenth ： 该套接字接收队列中现存数据长度。
 *返回值： 0 成功； -1： 失败。
*/
int	data_unpack_process(int recvLenth);

//灏� 鎺ユ敹鍒扮殑鏁村寘鏁版嵁鏀惧叆闃熷垪涓�
int save_data(char *content, int contentLenth);

/*计算校验和接口
 *参数： buf : 需要计算的字符串
 *		 size； 该字符串的长度
 *返回值： 计算的校验和
*/
uint32_t crc32(const unsigned char *buf, uint32_t size);

/*发送指定 字节的数据
 *参数： fd, 网络套接字；
 *		buf: 要发送的数据
 *		lenth: 发送的数据的长度
 *返回值： 已经发送数据的长度；  > 0 : 已经发送的长度。  < 0: 发送出错。
*/
ssize_t writen(int fd, const void *buf, ssize_t lenth);

/*获取发送队列中数据的长度
 *参数： tmpSend ： 要发送的数据包
 *		contentLenth： 该数据包的长度；	主调函数分配内存
 *返回值： 0 成功；  -1：失败。
*/
int get_content_lenth(char *tmpSend, int *contentLenth);

//修改 上传图片信息出错集合,记录出错的图片位置,数量及出错信息
void modify_upload_error_array(int index, uploadWholeSet *err);

/*遍历 上传图片信息集合,查看是否需要有重新发送的图片*/
int foreach_array(uploadWholeSet *array);

//select 定时器, second : 单位 s
void timer_select(int seconds);

/*将UI进程 指定偏移位置内存块数据信息拷贝至 net层内存池内存块
*@param : news cmd~地址偏移信息~
*@retval: 成功 : net层内存池内存块的地址; 失败 : NULL
*/
void *copy_the_ui_cmd_news(const char *news);

//读配置文件， 获取参数： IP， port 等；
static int get_config_file_para();

 //choose The method to set local IP and gate;
 //1. DHCP, 2. manual set IP And gate
 int chose_setip_method();

/*print The  client request reply from server 
*@param : replyNews   reply content;  style : cmd~struct content~
*/
 void print_client_request(char *srcReplyNews);

 
 /*回去发送数据包的命令字 和 流水号
  *@param: sendData : 要发送的数据包(转义过的完整数据包)
  *@param: sendDataLenth : 发送数据包的长度
  *@param: sendCmd : 获取发送数据包的命令字
  *@param: serialNumber : 获取发送数据包的流水号
  *@retval: 成功 0; 失败 -1;
 */
int get_send_pack_cmd(char *sendData, int sendDataLenth, int *sendCmd, unsigned int *serialNumber);
int get_send_encryptpack_content(char *sendData, int sendDataLenth, int *sendCmd, char *flag);


/*发送网络数据包时，网络断开，判断发送哪种数据包*/
int send_error_netPack(int sendCmd);

/*出错后，清理全局变量
 *@param: flag : 0 标识发送时清理， 1 标识接收清理
*/
void clear_global(int flag);

/*发送登录数据包时，网络断开， 发送指定命令字下网络断开数据包*/
int send_login_pack_netError();

/*发送图片数据包时，网络断开， 发送指定命令字下网络断开数据包*/
int send_upload_pack_netError();

/*校验下载的文件是否为最新 数据包, 网络断开, 发送指定命令字下网络断开数据包*/
int send_checkout_fileID_newest_pack_netError();

//Get The specify File type NewEst ID
int  get_FileNewestID(const char* news);

/*发送下载模板数据包, 网络断开, 发送指定命令字下网络断开数据包*/
int send_downLoadTemplate_pack_netError();

/*发送心跳数据包, 网络断开, 发送指定命令字下网络断开数据包*/
int send_heart_pack_netError();

/*发送消息推送, 网络断开, 发送指定命令字下网络断开数据包*/
int send__messagePush_pack_netError();

/*发送删除图片数据包, 网络断开, 发送指定命令字下网络断开数据包*/
int send_deletePicture_pack_netError();

//上传模板扫描数据过程中,网络错误
int send_template_pack_netError();

//加密传输或取消加密传输数据包发送, 网络断开错误
int send_encryOrCancel_pack_netError();


/*发送数据过程中,网络出错,发送线程 发出指定数据包的错误信息.
 *@param : cmd : 上传数据包类型的标识符
 *@param : err: 不同数据包的上传结果
 *@param : errNum: 不同数据包的出错信息标识符
 *@retval: 成功 0; 失败 -1;
*/
int copy_news_to_mem(int cmd, int err, int errNum);

/*找出指定路径下图片的名称
*@param : filePath  指定路径
*@param : desLenth  destBuf的内存长度
*@param : destBuf	拷贝的区域
*@retval: 成功 0; 失败 -1;
*/
int find_file_name(char *destBuf, int desLenth, const char *filePath);

/*接收线程: 数据包接收成功后; 1.删除刚才发送的数据包地址, 并将该地址放回内存池
* 2.删除时间链表中对应的地址, 并释放该节点内存
* 3.释放发送数据包长度的地址.
*/
void remove_senddataserver_addr();

/*比较接收的是否正确, 采用crc32散列校验.
*@param : tmpHead 接收数据的报头
*@param : tmpContent 接收数据的内容(报头 + 报体 + 校验码)
*@param : tmpContentLenth 数据内容的长度
*@retval: 成功 0; 失败 -1;
*/
int compare_recvData_Correct(char *tmpContent, int tmpContentLenth);


/*对数据包的头赋值
*@param : head 赋值的报头
*@param : cmd 该数据包的命令字
*@param : contentLenth 该数据包的长度(报头 + 消息体)
*@param : subPackOrNo 该报文是否为分包传输
*@param : machine 机器码
*@param : totalPackNum 该报文的总传输包数
*@param : indexPackNum 当前传输包数
*/
void assign_reqPack_head(reqPackHead_t *head, uint8_t cmd, int contentLenth, int isSubPack, int currentIndex, int totalNumber, int perRoundNumber );
void assign_resComPack_head(resCommonHead_t *head, int cmd, int contentLenth, int serial, int isSubPack);


/*图片数据组包
*@param : filePath  文件绝对路径
*@param : readLen 每次读取文件的大小
*@retval: 成功 0; 失败 -1;
*/
int image_data_package(const char *filePath, int readLen, int fileNumber, int totalFileNumber);


/*从服务器接收数据
 *参数： sockfd ： TCP网络连接套接字
 *返回值： 成功 0； 失败： -1；
*/
int recv_data(int sockfd);

/*向服务器发送数据
 *参数： sockfd ： TCP网络连接套接字
 		 sendCmd : 发送数据包的命令字
 *返回值： 成功 0； 失败： -1；
*/
int send_data(int sockfd, int *sendCmd);

/*数据进行组包
 *参数： func 对应命令字的回调函数
 *		 news  从UI界面接收的消息
 *返回值：成功 0；   失败  -1；
*/
int  data_packge(call_back_package  func, const char *news);

/*模板扫描,成功后,进行的应答包数据处理
*@param : sendUiData  进行处理的数据包
*@param : buf  返回给UI的数据内容
*@retval: 成功 0; 失败 -1;
*/
int reset_packet_send_ui(char *sendUiData, char **buf);

/*解包的数据处理
 *参数: 	news : 服务器回复的应答;  包含: 消息头 + 消息体 + 校验码
 *			lenth: news的部分数据长度;   包含: 消息头 + 数据体.
 *			outDataNews:  将处理后的数据拷贝至 该地址内存, 返回给 UI进程.
 *返回值:  成功: 0;  失败: -1;
*/
int data_un_packge(call_back_un_pack func_data,  char *news, char **outDataNews);


//////////// 发送数据包 与 应答数据包的出理方法  //////////////


//net 服务层出错, 组包告诉 UI 进程  			0号数据包
int  error_net();

/*处理登录数据包
 *参数：  news  从UI界面接收的消息				1号数据包
 *返回值：成功 0；   失败  -1；
*/
int  login_func(const char* news);

//退出数据包消息格式 ：消息体为空，只有报文头 	2号数据包                       
int  exit_program(const char* news);

//心跳数据包消息格式 ：消息体为空，只有报文头   3号数据包                     
int  heart_beat_program(const char* news);

//下载模板数据包格式： 							4号数据包
int download_file_fromServer(const char *news);

//checkOut The downLoad file whether newest
int  checkOut_downLoad_file_newest(const char* news);

//Get The newest File ID
int get_FileNewestID_reply(  char *news, char **outDataToUi);

//客户端信息请求								5号数据包
int  clinet_news_request(const char* news);

//上传扫描图片数据包   news:包含命令字和该图片所在位置。		6号数据包
int  upload_picture(const char *news);

//服务器消息推送回复数据包
int  push_info_from_server_reply(const char* news);


//net 服务层关闭, 组包告诉 UI 进程  			8号数据包
int  active_shutdown_net();

//删除图片数据包								9号数据包
int delete_spec_pic(const char *news);

// 链接服务器									10号数据包
int  connet_server(const char *news);

//修改文件后,重新读取数据						11号数据包
int read_config_file(const char *news);

/*第二版: 传输拓展包: 包含 学生ID, 本科试卷的扫描图片份数, 各图片名字, 本科试卷的客观题答案, 主观题切图个数, 主观题所有切图坐标点
*@param : news  UI-->net信息数据包; 格式为: cmd~data~;  data : 内存数据地址
*@retval: 成功 0; 失败 -1.						12号数据包
*/
int template_extend_element(const char *news);

/*上传一套试卷的图片(即多张图片传输, 传输过程中出错,底层程序会根据错误类型自动重传或进行报错(有的错误不能进行重传))
*@param : uploadSet  图片集合					伪13号数据包.(现阶段只在net层和server层之间使用)
*@retval: 成功 0; 失败 -1;
*/
int upload_template_set(uploadWholeSet *uploadSet);

//encrypt transfer or No cancel encrypt tansfer
int  encryp_or_cancelEncrypt_transfer(const char *news);


//底层 net 服务出错数据包			0号应答包
int error_net_reply( char *news, char **outDataNews);

//登录应答包						1号应答包
int login_reply_func( char *news, char **outDataNews);

//退出应答包						2号应答包
int exit_reply_program( char *news,  char **outDataNews);

//心跳应答包						3号应答包
int heart_beat_reply_program( char *news, char **outDataNews);

//下载模板应答包					4号应答包
int download_reply_template( char *news, char **outDataNews);

//校验下载的文件是否为最新			5号数据包
int checkOut_downLoad_file_newest_reply( char *news, char **outDataToUi);


//客户端信息请求应答包				5号应答包
int clinet_news_reply_request( char *news, char **outDataNews);

//上传扫描图片应答包				6号应答包
int upload_picture_reply_request( char *news, char **outDataNews);

//服务器推送消息功能实现
int push_info_from_server( char *news, char **outDataToUi);

//底层 net 服务主动关闭数据包		8号应答包
int active_shutdown_net_reply( char *news, char **outDataNews);

//删除图片应答 应答数据包			9号应答包
int delete_spec_pic_reply( char *news, char **outDataNews);

//链接服务器应答;  news: 消息头 + 消息体(链接服务器结果)			回复给 UI 进程的消息: 0x8A~0~
									//10号数据包应答
int connet_server_reply( char *news,  char **outDataNews);

//读取配置文件应答; news: 消息头 + 消息体(读取配置文件是否成功);  回复给 UI 进程的消息: 0x8B~0~
									//11号数据包应答
int read_config_file_reply( char *news, char **outDataNews);

//上传整套数据包的应答; news: 消息头 + 消息体(读取配置文件是否成功);  回复给 UI 进程的消息: 0x8B~0~
									//12号数据包
int template_extend_element_reply( char *news, char **outDataNews);


/*deal with encrypt transfer  reply 
*@param : news   The reply content from server; style : head + body + checkCode;
*@param : lenth  The head + body lenth;
*@param : outDataNews reply data to UI;   style : cmd~struct content~
*@retval: 0: success;  -1: fail; 1: continue And not reply UI
*/
int encryp_or_cancelEncrypt_transfer_reply( char *news, char **outDataToUi);


#if defined(__cplusplus)
}
#endif

#endif
