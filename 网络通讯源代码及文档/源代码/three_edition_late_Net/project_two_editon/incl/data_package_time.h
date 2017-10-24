#ifndef DATA_PACKAGE_TIME_H
#define DATA_PACKAGE_TIME_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <time.h>
#include "sock_pack.h"

//ע������Ļص�����
typedef int (*call_back_package)(const char *news);
typedef int (*call_back_un_pack)(char *news, char **outDataNews);


//���ر���һ���Ծ�Ľṹ��Ϣ
typedef struct _uploadWholeSet{
	char filePath [128];				//�ϴ��ļ��ľ���·��
	unsigned char totalNumber;			//�����Ծ��ͼƬ����
	unsigned char indexNumber;			//����ͼƬ�����, �� 0 ��ʼ
	unsigned char result;				//����ͼƬ���ϴ����
	unsigned char reason;				//����ͼƬ�ĳ���ԭ��, �ϴ���ȷʱ,��ֵ��Ч
}uploadWholeSet;

typedef struct _sendDataBlock{
	char 	*sendAddr;		//���ݰ����͵�ַ
	int 	sendLenth;		//���ݰ����ͳ���
	int 	cmd;			//���ݰ�����������
	time_t  sendTime;		//���ݰ�����ʱ��
}SendDataBlockNews;



// ȫ�ֱ�����ʼ��
static int init_global_value();
// �����߳�
static int init_pthread_create();


//�����������������߳�
void *thid_server_func_write(void *arg);
//���߳�,�ӷ�������ȡ����
void *thid_server_func_read(void *arg);
//ʱ���߳�, ��ʱ�������ݰ���ʱ��
void *thid_server_func_time(void *arg);
//����UI�����߳�
void *thid_business(void *arg);
//�����߳�
void *thid_hreat_beat_time(void *arg);
/*unpack The data */
void *thid_unpack_thread(void *arg);

//���·������һ������
void repeat_latePack();


//�����ݽ��д��� ����� ���͸�UI �̵߳����ݰ�
//static int data_option(char *storeData, char **sendUiData );
//�Խ��յķ��������ݴ���, ����: �������������ڴ�
int data_process(char **buf);
/*�ͻ��˽��յ����������ݣ��Ƚ��д��������һ��������������ն����У����͸�UI��
 *������ recvLenth �� ���׽��ֽ��ն������ִ����ݳ��ȡ�
 *����ֵ�� 0 �ɹ��� -1�� ʧ�ܡ�
*/
int	data_unpack_process(int recvLenth);

//将 接收到的整包数据放入队列中
int save_data(char *content, int contentLenth);

/*����У��ͽӿ�
 *������ buf : ��Ҫ������ַ���
 *		 size�� ���ַ����ĳ���
 *����ֵ�� �����У���
*/
uint32_t crc32(const unsigned char *buf, uint32_t size);

/*����ָ�� �ֽڵ�����
 *������ fd, �����׽��֣�
 *		buf: Ҫ���͵�����
 *		lenth: ���͵����ݵĳ���
 *����ֵ�� �Ѿ��������ݵĳ��ȣ�  > 0 : �Ѿ����͵ĳ��ȡ�  < 0: ���ͳ���
*/
ssize_t writen(int fd, const void *buf, ssize_t lenth);

/*��ȡ���Ͷ��������ݵĳ���
 *������ tmpSend �� Ҫ���͵����ݰ�
 *		contentLenth�� �����ݰ��ĳ��ȣ�	�������������ڴ�
 *����ֵ�� 0 �ɹ���  -1��ʧ�ܡ�
*/
int get_content_lenth(char *tmpSend, int *contentLenth);

//�޸� �ϴ�ͼƬ��Ϣ������,��¼�����ͼƬλ��,������������Ϣ
void modify_upload_error_array(int index, uploadWholeSet *err);

/*���� �ϴ�ͼƬ��Ϣ����,�鿴�Ƿ���Ҫ�����·��͵�ͼƬ*/
int foreach_array(uploadWholeSet *array);

//select ��ʱ��, second : ��λ s
void timer_select(int seconds);

/*��UI���� ָ��ƫ��λ���ڴ��������Ϣ������ net���ڴ���ڴ��
*@param : news cmd~��ַƫ����Ϣ~
*@retval: �ɹ� : net���ڴ���ڴ��ĵ�ַ; ʧ�� : NULL
*/
void *copy_the_ui_cmd_news(const char *news);

//�������ļ��� ��ȡ������ IP�� port �ȣ�
static int get_config_file_para();

 //choose The method to set local IP and gate;
 //1. DHCP, 2. manual set IP And gate
 int chose_setip_method();

/*print The  client request reply from server 
*@param : replyNews   reply content;  style : cmd~struct content~
*/
 void print_client_request(char *srcReplyNews);

 
 /*��ȥ�������ݰ��������� �� ��ˮ��
  *@param: sendData : Ҫ���͵����ݰ�(ת������������ݰ�)
  *@param: sendDataLenth : �������ݰ��ĳ���
  *@param: sendCmd : ��ȡ�������ݰ���������
  *@param: serialNumber : ��ȡ�������ݰ�����ˮ��
  *@retval: �ɹ� 0; ʧ�� -1;
 */
int get_send_pack_cmd(char *sendData, int sendDataLenth, int *sendCmd, unsigned int *serialNumber);
int get_send_encryptpack_content(char *sendData, int sendDataLenth, int *sendCmd, char *flag);


/*�����������ݰ�ʱ������Ͽ����жϷ����������ݰ�*/
int send_error_netPack(int sendCmd);

/*���������ȫ�ֱ���
 *@param: flag : 0 ��ʶ����ʱ���� 1 ��ʶ��������
*/
void clear_global(int flag);

/*���͵�¼���ݰ�ʱ������Ͽ��� ����ָ��������������Ͽ����ݰ�*/
int send_login_pack_netError();

/*����ͼƬ���ݰ�ʱ������Ͽ��� ����ָ��������������Ͽ����ݰ�*/
int send_upload_pack_netError();

/*У�����ص��ļ��Ƿ�Ϊ���� ���ݰ�, ����Ͽ�, ����ָ��������������Ͽ����ݰ�*/
int send_checkout_fileID_newest_pack_netError();

//Get The specify File type NewEst ID
int  get_FileNewestID(const char* news);

/*��������ģ�����ݰ�, ����Ͽ�, ����ָ��������������Ͽ����ݰ�*/
int send_downLoadTemplate_pack_netError();

/*�����������ݰ�, ����Ͽ�, ����ָ��������������Ͽ����ݰ�*/
int send_heart_pack_netError();

/*������Ϣ����, ����Ͽ�, ����ָ��������������Ͽ����ݰ�*/
int send__messagePush_pack_netError();

/*����ɾ��ͼƬ���ݰ�, ����Ͽ�, ����ָ��������������Ͽ����ݰ�*/
int send_deletePicture_pack_netError();

//�ϴ�ģ��ɨ�����ݹ�����,�������
int send_template_pack_netError();

//���ܴ����ȡ�����ܴ������ݰ�����, ����Ͽ�����
int send_encryOrCancel_pack_netError();


/*�������ݹ�����,�������,�����߳� ����ָ�����ݰ��Ĵ�����Ϣ.
 *@param : cmd : �ϴ����ݰ����͵ı�ʶ��
 *@param : err: ��ͬ���ݰ����ϴ����
 *@param : errNum: ��ͬ���ݰ��ĳ�����Ϣ��ʶ��
 *@retval: �ɹ� 0; ʧ�� -1;
*/
int copy_news_to_mem(int cmd, int err, int errNum);

/*�ҳ�ָ��·����ͼƬ������
*@param : filePath  ָ��·��
*@param : desLenth  destBuf���ڴ泤��
*@param : destBuf	����������
*@retval: �ɹ� 0; ʧ�� -1;
*/
int find_file_name(char *destBuf, int desLenth, const char *filePath);

/*�����߳�: ���ݰ����ճɹ���; 1.ɾ���ղŷ��͵����ݰ���ַ, �����õ�ַ�Ż��ڴ��
* 2.ɾ��ʱ�������ж�Ӧ�ĵ�ַ, ���ͷŸýڵ��ڴ�
* 3.�ͷŷ������ݰ����ȵĵ�ַ.
*/
void remove_senddataserver_addr();

/*�ȽϽ��յ��Ƿ���ȷ, ����crc32ɢ��У��.
*@param : tmpHead �������ݵı�ͷ
*@param : tmpContent �������ݵ�����(��ͷ + ���� + У����)
*@param : tmpContentLenth �������ݵĳ���
*@retval: �ɹ� 0; ʧ�� -1;
*/
int compare_recvData_Correct(char *tmpContent, int tmpContentLenth);


/*�����ݰ���ͷ��ֵ
*@param : head ��ֵ�ı�ͷ
*@param : cmd �����ݰ���������
*@param : contentLenth �����ݰ��ĳ���(��ͷ + ��Ϣ��)
*@param : subPackOrNo �ñ����Ƿ�Ϊ�ְ�����
*@param : machine ������
*@param : totalPackNum �ñ��ĵ��ܴ������
*@param : indexPackNum ��ǰ�������
*/
void assign_reqPack_head(reqPackHead_t *head, uint8_t cmd, int contentLenth, int isSubPack, int currentIndex, int totalNumber, int perRoundNumber );
void assign_resComPack_head(resCommonHead_t *head, int cmd, int contentLenth, int serial, int isSubPack);


/*ͼƬ�������
*@param : filePath  �ļ�����·��
*@param : readLen ÿ�ζ�ȡ�ļ��Ĵ�С
*@retval: �ɹ� 0; ʧ�� -1;
*/
int image_data_package(const char *filePath, int readLen, int fileNumber, int totalFileNumber);


/*�ӷ�������������
 *������ sockfd �� TCP���������׽���
 *����ֵ�� �ɹ� 0�� ʧ�ܣ� -1��
*/
int recv_data(int sockfd);

/*���������������
 *������ sockfd �� TCP���������׽���
 		 sendCmd : �������ݰ���������
 *����ֵ�� �ɹ� 0�� ʧ�ܣ� -1��
*/
int send_data(int sockfd, int *sendCmd);

/*���ݽ������
 *������ func ��Ӧ�����ֵĻص�����
 *		 news  ��UI������յ���Ϣ
 *����ֵ���ɹ� 0��   ʧ��  -1��
*/
int  data_packge(call_back_package  func, const char *news);

/*ģ��ɨ��,�ɹ���,���е�Ӧ������ݴ���
*@param : sendUiData  ���д�������ݰ�
*@param : buf  ���ظ�UI����������
*@retval: �ɹ� 0; ʧ�� -1;
*/
int reset_packet_send_ui(char *sendUiData, char **buf);

/*��������ݴ���
 *����: 	news : �������ظ���Ӧ��;  ����: ��Ϣͷ + ��Ϣ�� + У����
 *			lenth: news�Ĳ������ݳ���;   ����: ��Ϣͷ + ������.
 *			outDataNews:  �����������ݿ����� �õ�ַ�ڴ�, ���ظ� UI����.
 *����ֵ:  �ɹ�: 0;  ʧ��: -1;
*/
int data_un_packge(call_back_un_pack func_data,  char *news, char **outDataNews);


//////////// �������ݰ� �� Ӧ�����ݰ��ĳ�����  //////////////


//net ��������, ������� UI ����  			0�����ݰ�
int  error_net();

/*�����¼���ݰ�
 *������  news  ��UI������յ���Ϣ				1�����ݰ�
 *����ֵ���ɹ� 0��   ʧ��  -1��
*/
int  login_func(const char* news);

//�˳����ݰ���Ϣ��ʽ ����Ϣ��Ϊ�գ�ֻ�б���ͷ 	2�����ݰ�                       
int  exit_program(const char* news);

//�������ݰ���Ϣ��ʽ ����Ϣ��Ϊ�գ�ֻ�б���ͷ   3�����ݰ�                     
int  heart_beat_program(const char* news);

//����ģ�����ݰ���ʽ�� 							4�����ݰ�
int download_file_fromServer(const char *news);

//checkOut The downLoad file whether newest
int  checkOut_downLoad_file_newest(const char* news);

//Get The newest File ID
int get_FileNewestID_reply(  char *news, char **outDataToUi);

//�ͻ�����Ϣ����								5�����ݰ�
int  clinet_news_request(const char* news);

//�ϴ�ɨ��ͼƬ���ݰ�   news:���������ֺ͸�ͼƬ����λ�á�		6�����ݰ�
int  upload_picture(const char *news);

//��������Ϣ���ͻظ����ݰ�
int  push_info_from_server_reply(const char* news);


//net �����ر�, ������� UI ����  			8�����ݰ�
int  active_shutdown_net();

//ɾ��ͼƬ���ݰ�								9�����ݰ�
int delete_spec_pic(const char *news);

// ���ӷ�����									10�����ݰ�
int  connet_server(const char *news);

//�޸��ļ���,���¶�ȡ����						11�����ݰ�
int read_config_file(const char *news);

/*�ڶ���: ������չ��: ���� ѧ��ID, �����Ծ��ɨ��ͼƬ����, ��ͼƬ����, �����Ծ�Ŀ͹����, ��������ͼ����, ������������ͼ�����
*@param : news  UI-->net��Ϣ���ݰ�; ��ʽΪ: cmd~data~;  data : �ڴ����ݵ�ַ
*@retval: �ɹ� 0; ʧ�� -1.						12�����ݰ�
*/
int template_extend_element(const char *news);

/*�ϴ�һ���Ծ��ͼƬ(������ͼƬ����, ��������г���,�ײ�������ݴ��������Զ��ش�����б���(�еĴ����ܽ����ش�))
*@param : uploadSet  ͼƬ����					α13�����ݰ�.(�ֽ׶�ֻ��net���server��֮��ʹ��)
*@retval: �ɹ� 0; ʧ�� -1;
*/
int upload_template_set(uploadWholeSet *uploadSet);

//encrypt transfer or No cancel encrypt tansfer
int  encryp_or_cancelEncrypt_transfer(const char *news);


//�ײ� net ����������ݰ�			0��Ӧ���
int error_net_reply( char *news, char **outDataNews);

//��¼Ӧ���						1��Ӧ���
int login_reply_func( char *news, char **outDataNews);

//�˳�Ӧ���						2��Ӧ���
int exit_reply_program( char *news,  char **outDataNews);

//����Ӧ���						3��Ӧ���
int heart_beat_reply_program( char *news, char **outDataNews);

//����ģ��Ӧ���					4��Ӧ���
int download_reply_template( char *news, char **outDataNews);

//У�����ص��ļ��Ƿ�Ϊ����			5�����ݰ�
int checkOut_downLoad_file_newest_reply( char *news, char **outDataToUi);


//�ͻ�����Ϣ����Ӧ���				5��Ӧ���
int clinet_news_reply_request( char *news, char **outDataNews);

//�ϴ�ɨ��ͼƬӦ���				6��Ӧ���
int upload_picture_reply_request( char *news, char **outDataNews);

//������������Ϣ����ʵ��
int push_info_from_server( char *news, char **outDataToUi);

//�ײ� net ���������ر����ݰ�		8��Ӧ���
int active_shutdown_net_reply( char *news, char **outDataNews);

//ɾ��ͼƬӦ�� Ӧ�����ݰ�			9��Ӧ���
int delete_spec_pic_reply( char *news, char **outDataNews);

//���ӷ�����Ӧ��;  news: ��Ϣͷ + ��Ϣ��(���ӷ��������)			�ظ��� UI ���̵���Ϣ: 0x8A~0~
									//10�����ݰ�Ӧ��
int connet_server_reply( char *news,  char **outDataNews);

//��ȡ�����ļ�Ӧ��; news: ��Ϣͷ + ��Ϣ��(��ȡ�����ļ��Ƿ�ɹ�);  �ظ��� UI ���̵���Ϣ: 0x8B~0~
									//11�����ݰ�Ӧ��
int read_config_file_reply( char *news, char **outDataNews);

//�ϴ��������ݰ���Ӧ��; news: ��Ϣͷ + ��Ϣ��(��ȡ�����ļ��Ƿ�ɹ�);  �ظ��� UI ���̵���Ϣ: 0x8B~0~
									//12�����ݰ�
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
