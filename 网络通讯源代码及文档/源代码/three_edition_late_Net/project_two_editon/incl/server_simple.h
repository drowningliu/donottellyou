#ifndef _SERVER_SIMPLE_H_
#define _SERVER_SIMPLE_H_


#if defined(__cplusplus)
extern "C" {
#endif

#include "sock_pack.h"

#if 0
typedef struct _RecvAndSendthid_Sockfd_{
	pthread_t		pthidWrite;
	pthread_t		pthidRead;
	int 			sockfd;
	int 			flag;					// 0: UnUse;  1: use
	SSL_CTX 		*sslCtx;				// The Once connection 
	SSL 			*sslHandle;				// The Once connection for transfer
	sem_t			sem_send;				//The send thread And recv Thread synchronization
	bool			encryptRecvFlag;		//true : encrypt, false : No encrypt
	bool			encryptSendFlag;		//ture : send data In use encyprtl, false : In use common
	bool			encryptChangeFlag;		//true : once recv cmd=14, (deal with recv And send Flag(previous)), modify to False ; false : No change		
}RecvAndSendthid_Sockfd;
#endif

typedef struct _reqTemplateErrHead_t{
    uint8_t     cmd;                //������
    uint16_t    contentLenth;       //���ȣ� ��������ͷ����Ϣ�峤��
    uint8_t     isSubPack;          //�ж��Ƿ�Ϊ�Ӱ�, 0 ����, 1 ��;
    uint8_t     machineCode[12];    //������
    uint32_t    seriaNumber;        //��ˮ��
    uint8_t     failPackNumber;     //�Ƿ�ʧ��
    uint16_t    failPackIndex;      //ʧ�����ݰ������к�
}reqTemplateErrHead_t;
//注册组包的回调函数
typedef int (*call_back_un_pack)(char *recvBuf, int recvLen, int index);

//出错函数
int std_err(const char* name);

//子进程 退出信号处理函数
void func_data(int sig);

//服务器 开启监听套接字
int server_bind_listen_fd(char *ip, int port, int *listenFd);

//子线程 业务执行逻辑;  返回值: 0:标识程序正常, 1: 标识客户端关闭;  -1: 标识出错.
void *child_read_func(void *arg);

//The send Data to client
void *child_write_func(void *arg);

//scan directory 
void *child_scan_func(void *arg);

//接收客户端的数据包, recvBuf: 包含: 标识符 + 报头 + 报体 + 校验码 + 标识符;   recvLen : recvBuf数据的长度
//				sendBuf : 处理数据包, 将要发送的数据;  sendLen : 发送数据的长度
int read_data_proce(char *recvBuf, int recvLen, int index);

//find directory have file And push infomation to Ui
void find_directory_file();

/*�Ƚ����ݰ������Ƿ���ȷ
*@param : tmpContent		���ݰ�ת��������
*@param : tmpContentLenth	ת������ݵĳ���, ����: ��ͷ + ��Ϣ + У����
*@retval: success 0, repeat -1, NoRespond -2
*/
int compare_recvData_correct_server(char *tmpContent, int tmpContentLenth);
int save_multiPicture_func(char *recvBuf, int recvLen);
void assign_comPack_head(resCommonHead_t *head, int cmd, int contentLenth, int isSubPack, int isFailPack, int failPackIndex, int isRespond);
//注册回调函数
int data_un_packge(call_back_un_pack func_handle, char *recvBuf, int recvLen, int index);

//push Infomation To UI
int push_info_toUi(int index, int fileFlag);

//push Infomation To UI reply
int push_info_toUi_reply(char *recvBuf, int recvLen, int index);

//登录应答数据包;  flag : 0, 标识需要发送数据包, 非0, 标识不需要发送数据包
int login_func_reply(char *recvBuf, int recvLen, int index);

int heart_beat_func_reply(char *recvBuf, int recvLen, int index);

int downLoad_template_func_reply(char *recvBuf, int recvLen, int index);

int client_request_func_reply(char *recvBuf, int recvLen, int index);

//checkOut The fileId whether NewEst
int checkOut_fileID_newest_reply(char *recvBuf, int recvLen, int srcIndex);

//get The specify FileType Newest FileID
int get_FileNewestID_reply(char *recvBuf, int recvLen, int index);

//上传图片应答 数据包
int upload_func_reply(char *recvBuf, int recvLen, int index);

//删除图片应答 数据包
int delete_func_reply(char *recvBuf, int recvLen, int index);

//删除图片应答 数据包
int exit_func_reply(char *recvBuf, int recvLen, int index);

//encrypt data transfer
int encryp_or_cancelEncrypt_transfer(char *recvBuf, int recvLen, int index);

//模板扩展数据包 数据包
int template_extend_element_reply(char *recvBuf, int recvLen, int index);

//模板图片集合 数据包
int upload_template_set_reply(char *recvBuf, int recvLen, int index);

/*对数据包的头赋值
*@param : head 赋值的报头
*@param : cmd 该数据包的命令字
*@param : contentLenth 该数据包的长度(报头 + 消息体)
*@param : subPackOrNo 该报文是否为分包传输
*@param : machine 机器码

*@param : totalPackNum 该报文的总传输包数
*@param : indexPackNum 当前传输包数
*/
void assign_pack_head(void *head, uint8_t cmd, int serial, uint32_t contentLenth, uint8_t subPackOrNo, char *machine,  uint16_t totalPackNum, uint16_t indexPackNum);


//打印接收内容的信息
void printf_pack_news(char *recvBuf);

//保存图片
int save_picture_func(char *recvBuf, int recvLen);

//保存上传的图片合集
int save_set_picture_func(char *recvBuf, int recvLen);

//SIGPIPE 信号的捕捉
void func_pipe(int sig);

//服务器初始化
int init_server(char *ip, int port, char *uploadDirParh);
void destroy_server();
/////////////////////////////
/*read_package  读一包数据;
 *参数: buf: 主调函数分配内存, 拷贝读取到的一包数据;
 *		maxline:  buf内存的最大空间
 *		sockfd:   链接的接字
 *		tmpFlag:  当 所取的 数据包不正确时, 再次重读, 需要保留前面的标识符, 保证不错过数据包
 *返回值: 返回读取的数据包中含有的 字节数
*/
int read_package(char *recvBuf, int dataLenth, char **sendBuf, int *sendLen);

// 返回值: -1: 标识读取出错, 0: 标识套接字关闭; > 0: 标识读取数据包正常
int do_read(int sockfd);
 
 /**
 * recv_peek - 仅仅查看套接字缓冲区数据，但不移除数据
 * @sockfd: 套接字
 * @buf: 接收缓冲区
 * @len: 长度
 * 成功返回>=0，失败返回-1
 */
ssize_t recv_peek(int sockfd, void *buf, size_t len);
 
 /**
 * readn - 读取固定字节数
 * @fd: 文件描述符
 * @buf: 接收缓冲区
 * @count: 要读取的字节数
 * 成功返回count，失败返回-1，读到EOF返回<count
 */
ssize_t readn(int fd, void *buf, size_t count);

int activate_nonblock(int fd);

int deactivate_nonblock(int fd);

#if defined(__cplusplus)
}
#endif

#endif