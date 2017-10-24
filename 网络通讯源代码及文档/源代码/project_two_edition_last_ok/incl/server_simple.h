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
    uint8_t     cmd;                //命令字
    uint16_t    contentLenth;       //长度； 包含：报头和消息体长度
    uint8_t     isSubPack;          //判断是否为子包, 0 不是, 1 是;
    uint8_t     machineCode[12];    //机器码
    uint32_t    seriaNumber;        //流水号
    uint8_t     failPackNumber;     //是否失败
    uint16_t    failPackIndex;      //失败数据包的序列号
}reqTemplateErrHead_t;
//娉ㄥ唽缁勫寘鐨勫洖璋冨嚱鏁�
typedef int (*call_back_un_pack)(char *recvBuf, int recvLen, int index);

//鍑洪敊鍑芥暟
int std_err(const char* name);

//瀛愯繘绋� 閫�鍑轰俊鍙峰鐞嗗嚱鏁�
void func_data(int sig);

//鏈嶅姟鍣� 寮�鍚洃鍚鎺ュ瓧
int server_bind_listen_fd(char *ip, int port, int *listenFd);

//瀛愮嚎绋� 涓氬姟鎵ц閫昏緫;  杩斿洖鍊�: 0:鏍囪瘑绋嬪簭姝ｅ父, 1: 鏍囪瘑瀹㈡埛绔叧闂�;  -1: 鏍囪瘑鍑洪敊.
void *child_read_func(void *arg);

//The send Data to client
void *child_write_func(void *arg);

//scan directory 
void *child_scan_func(void *arg);

//鎺ユ敹瀹㈡埛绔殑鏁版嵁鍖�, recvBuf: 鍖呭惈: 鏍囪瘑绗� + 鎶ュご + 鎶ヤ綋 + 鏍￠獙鐮� + 鏍囪瘑绗�;   recvLen : recvBuf鏁版嵁鐨勯暱搴�
//				sendBuf : 澶勭悊鏁版嵁鍖�, 灏嗚鍙戦�佺殑鏁版嵁;  sendLen : 鍙戦�佹暟鎹殑闀垮害
int read_data_proce(char *recvBuf, int recvLen, int index);

//find directory have file And push infomation to Ui
void find_directory_file();

/*比较数据包接收是否正确
*@param : tmpContent		数据包转义后的数据
*@param : tmpContentLenth	转义后数据的长度, 包含: 报头 + 信息 + 校验码
*@retval: success 0, repeat -1, NoRespond -2
*/
int compare_recvData_correct_server(char *tmpContent, int tmpContentLenth);
int save_multiPicture_func(char *recvBuf, int recvLen);
void assign_comPack_head(resCommonHead_t *head, int cmd, int contentLenth, int isSubPack, int isFailPack, int failPackIndex, int isRespond);
//娉ㄥ唽鍥炶皟鍑芥暟
int data_un_packge(call_back_un_pack func_handle, char *recvBuf, int recvLen, int index);

//push Infomation To UI
int push_info_toUi(int index, int fileFlag);

//push Infomation To UI reply
int push_info_toUi_reply(char *recvBuf, int recvLen, int index);

//鐧诲綍搴旂瓟鏁版嵁鍖�;  flag : 0, 鏍囪瘑闇�瑕佸彂閫佹暟鎹寘, 闈�0, 鏍囪瘑涓嶉渶瑕佸彂閫佹暟鎹寘
int login_func_reply(char *recvBuf, int recvLen, int index);

int heart_beat_func_reply(char *recvBuf, int recvLen, int index);

int downLoad_template_func_reply(char *recvBuf, int recvLen, int index);

int client_request_func_reply(char *recvBuf, int recvLen, int index);

//checkOut The fileId whether NewEst
int checkOut_fileID_newest_reply(char *recvBuf, int recvLen, int srcIndex);

//get The specify FileType Newest FileID
int get_FileNewestID_reply(char *recvBuf, int recvLen, int index);

//涓婁紶鍥剧墖搴旂瓟 鏁版嵁鍖�
int upload_func_reply(char *recvBuf, int recvLen, int index);

//鍒犻櫎鍥剧墖搴旂瓟 鏁版嵁鍖�
int delete_func_reply(char *recvBuf, int recvLen, int index);

//鍒犻櫎鍥剧墖搴旂瓟 鏁版嵁鍖�
int exit_func_reply(char *recvBuf, int recvLen, int index);

//encrypt data transfer
int encryp_or_cancelEncrypt_transfer(char *recvBuf, int recvLen, int index);

//妯℃澘鎵╁睍鏁版嵁鍖� 鏁版嵁鍖�
int template_extend_element_reply(char *recvBuf, int recvLen, int index);

//妯℃澘鍥剧墖闆嗗悎 鏁版嵁鍖�
int upload_template_set_reply(char *recvBuf, int recvLen, int index);

/*瀵规暟鎹寘鐨勫ご璧嬪��
*@param : head 璧嬪�肩殑鎶ュご
*@param : cmd 璇ユ暟鎹寘鐨勫懡浠ゅ瓧
*@param : contentLenth 璇ユ暟鎹寘鐨勯暱搴�(鎶ュご + 娑堟伅浣�)
*@param : subPackOrNo 璇ユ姤鏂囨槸鍚︿负鍒嗗寘浼犺緭
*@param : machine 鏈哄櫒鐮�

*@param : totalPackNum 璇ユ姤鏂囩殑鎬讳紶杈撳寘鏁�
*@param : indexPackNum 褰撳墠浼犺緭鍖呮暟
*/
void assign_pack_head(void *head, uint8_t cmd, int serial, uint32_t contentLenth, uint8_t subPackOrNo, char *machine,  uint16_t totalPackNum, uint16_t indexPackNum);


//鎵撳嵃鎺ユ敹鍐呭鐨勪俊鎭�
void printf_pack_news(char *recvBuf);

//淇濆瓨鍥剧墖
int save_picture_func(char *recvBuf, int recvLen);

//淇濆瓨涓婁紶鐨勫浘鐗囧悎闆�
int save_set_picture_func(char *recvBuf, int recvLen);

//SIGPIPE 淇″彿鐨勬崟鎹�
void func_pipe(int sig);

//鏈嶅姟鍣ㄥ垵濮嬪寲
int init_server(char *ip, int port, char *uploadDirParh);
void destroy_server();
/////////////////////////////
/*read_package  璇讳竴鍖呮暟鎹�;
 *鍙傛暟: buf: 涓昏皟鍑芥暟鍒嗛厤鍐呭瓨, 鎷疯礉璇诲彇鍒扮殑涓�鍖呮暟鎹�;
 *		maxline:  buf鍐呭瓨鐨勬渶澶х┖闂�
 *		sockfd:   閾炬帴鐨勬帴瀛�
 *		tmpFlag:  褰� 鎵�鍙栫殑 鏁版嵁鍖呬笉姝ｇ‘鏃�, 鍐嶆閲嶈, 闇�瑕佷繚鐣欏墠闈㈢殑鏍囪瘑绗�, 淇濊瘉涓嶉敊杩囨暟鎹寘
 *杩斿洖鍊�: 杩斿洖璇诲彇鐨勬暟鎹寘涓惈鏈夌殑 瀛楄妭鏁�
*/
int read_package(char *recvBuf, int dataLenth, char **sendBuf, int *sendLen);

// 杩斿洖鍊�: -1: 鏍囪瘑璇诲彇鍑洪敊, 0: 鏍囪瘑濂楁帴瀛楀叧闂�; > 0: 鏍囪瘑璇诲彇鏁版嵁鍖呮甯�
int do_read(int sockfd);
 
 /**
 * recv_peek - 浠呬粎鏌ョ湅濂楁帴瀛楃紦鍐插尯鏁版嵁锛屼絾涓嶇Щ闄ゆ暟鎹�
 * @sockfd: 濂楁帴瀛�
 * @buf: 鎺ユ敹缂撳啿鍖�
 * @len: 闀垮害
 * 鎴愬姛杩斿洖>=0锛屽け璐ヨ繑鍥�-1
 */
ssize_t recv_peek(int sockfd, void *buf, size_t len);
 
 /**
 * readn - 璇诲彇鍥哄畾瀛楄妭鏁�
 * @fd: 鏂囦欢鎻忚堪绗�
 * @buf: 鎺ユ敹缂撳啿鍖�
 * @count: 瑕佽鍙栫殑瀛楄妭鏁�
 * 鎴愬姛杩斿洖count锛屽け璐ヨ繑鍥�-1锛岃鍒癊OF杩斿洖<count
 */
ssize_t readn(int fd, void *buf, size_t count);

int activate_nonblock(int fd);

int deactivate_nonblock(int fd);

#if defined(__cplusplus)
}
#endif

#endif