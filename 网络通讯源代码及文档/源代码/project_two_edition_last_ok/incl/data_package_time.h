#ifndef DATA_PACKAGE_TIME_H
#define DATA_PACKAGE_TIME_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <time.h>
#include "sock_pack.h"

//×¢²á×é°üµÄ»Øµ÷º¯Êı
typedef int (*call_back_package)(const char *news);
typedef int (*call_back_un_pack)(char *news, char **outDataNews);


//±¾µØ±£´æÒ»Ì×ÊÔ¾íµÄ½á¹¹ĞÅÏ¢
typedef struct _uploadWholeSet{
	char filePath [128];				//ÉÏ´«ÎÄ¼şµÄ¾ø¶ÔÂ·¾¶
	unsigned char totalNumber;			//±¾Ì×ÊÔ¾íµÄÍ¼Æ¬×ÜÊı
	unsigned char indexNumber;			//±¾ÕÅÍ¼Æ¬µÄĞòºÅ, ´Ó 0 ¿ªÊ¼
	unsigned char result;				//±¾ÕÅÍ¼Æ¬µÄÉÏ´«½á¹û
	unsigned char reason;				//±¾ÕÅÍ¼Æ¬µÄ³ö´íÔ­Òò, ÉÏ´«ÕıÈ·Ê±,¸ÃÖµÎŞĞ§
}uploadWholeSet;

typedef struct _sendDataBlock{
	char 	*sendAddr;		//Êı¾İ°ü·¢ËÍµØÖ·
	int 	sendLenth;		//Êı¾İ°ü·¢ËÍ³¤¶È
	int 	cmd;			//Êı¾İ°ü·¢ËÍÃüÁî×Ö
	time_t  sendTime;		//Êı¾İ°ü·¢ËÍÊ±¼ä
}SendDataBlockNews;



// È«¾Ö±äÁ¿³õÊ¼»¯
static int init_global_value();
// ´´½¨Ïß³Ì
static int init_pthread_create();


//·¢ËÍÊı¾İÖÁ·şÎñÆ÷Ïß³Ì
void *thid_server_func_write(void *arg);
//¶ÁÏß³Ì,´Ó·şÎñÆ÷¶ÁÈ¡Êı¾İ
void *thid_server_func_read(void *arg);
//Ê±¼äÏß³Ì, ¼ÆÊ±·¢ËÍÊı¾İ°üµÄÊ±¼ä
void *thid_server_func_time(void *arg);
//´¦ÀíUIÃüÁîÏß³Ì
void *thid_business(void *arg);
//ĞÄÌøÏß³Ì
void *thid_hreat_beat_time(void *arg);
/*unpack The data */
void *thid_unpack_thread(void *arg);

//ÖØĞÂ·¢ËÍ×îºóÒ»°üÊı¾İ
void repeat_latePack();


//¶ÔÊı¾İ½øĞĞ´¦Àí£¬ ²¢×é³É ·¢ËÍ¸øUI Ïß³ÌµÄÊı¾İ°ü
//static int data_option(char *storeData, char **sendUiData );
//¶Ô½ÓÊÕµÄ·şÎñÆ÷Êı¾İ´¦Àí, ²ÎÊı: Ö÷µ÷º¯Êı·ÖÅäÄÚ´æ
int data_process(char **buf);
/*¿Í»§¶Ë½ÓÊÕµ½·şÎñÆ÷Êı¾İ£¬ÏÈ½øĞĞ´¦Àí£¬ÕûÀí³ÉÒ»¸öÕû°ü£¬·ÅÈë½ÓÊÕ¶ÓÁĞÖĞ£¬·¢ËÍ¸øUI¡£
 *²ÎÊı£º recvLenth £º ¸ÃÌ×½Ó×Ö½ÓÊÕ¶ÓÁĞÖĞÏÖ´æÊı¾İ³¤¶È¡£
 *·µ»ØÖµ£º 0 ³É¹¦£» -1£º Ê§°Ü¡£
*/
int	data_unpack_process(int recvLenth);

//å°† æ¥æ”¶åˆ°çš„æ•´åŒ…æ•°æ®æ”¾å…¥é˜Ÿåˆ—ä¸­
int save_data(char *content, int contentLenth);

/*¼ÆËãĞ£ÑéºÍ½Ó¿Ú
 *²ÎÊı£º buf : ĞèÒª¼ÆËãµÄ×Ö·û´®
 *		 size£» ¸Ã×Ö·û´®µÄ³¤¶È
 *·µ»ØÖµ£º ¼ÆËãµÄĞ£ÑéºÍ
*/
uint32_t crc32(const unsigned char *buf, uint32_t size);

/*·¢ËÍÖ¸¶¨ ×Ö½ÚµÄÊı¾İ
 *²ÎÊı£º fd, ÍøÂçÌ×½Ó×Ö£»
 *		buf: Òª·¢ËÍµÄÊı¾İ
 *		lenth: ·¢ËÍµÄÊı¾İµÄ³¤¶È
 *·µ»ØÖµ£º ÒÑ¾­·¢ËÍÊı¾İµÄ³¤¶È£»  > 0 : ÒÑ¾­·¢ËÍµÄ³¤¶È¡£  < 0: ·¢ËÍ³ö´í¡£
*/
ssize_t writen(int fd, const void *buf, ssize_t lenth);

/*»ñÈ¡·¢ËÍ¶ÓÁĞÖĞÊı¾İµÄ³¤¶È
 *²ÎÊı£º tmpSend £º Òª·¢ËÍµÄÊı¾İ°ü
 *		contentLenth£º ¸ÃÊı¾İ°üµÄ³¤¶È£»	Ö÷µ÷º¯Êı·ÖÅäÄÚ´æ
 *·µ»ØÖµ£º 0 ³É¹¦£»  -1£ºÊ§°Ü¡£
*/
int get_content_lenth(char *tmpSend, int *contentLenth);

//ĞŞ¸Ä ÉÏ´«Í¼Æ¬ĞÅÏ¢³ö´í¼¯ºÏ,¼ÇÂ¼³ö´íµÄÍ¼Æ¬Î»ÖÃ,ÊıÁ¿¼°³ö´íĞÅÏ¢
void modify_upload_error_array(int index, uploadWholeSet *err);

/*±éÀú ÉÏ´«Í¼Æ¬ĞÅÏ¢¼¯ºÏ,²é¿´ÊÇ·ñĞèÒªÓĞÖØĞÂ·¢ËÍµÄÍ¼Æ¬*/
int foreach_array(uploadWholeSet *array);

//select ¶¨Ê±Æ÷, second : µ¥Î» s
void timer_select(int seconds);

/*½«UI½ø³Ì Ö¸¶¨Æ«ÒÆÎ»ÖÃÄÚ´æ¿éÊı¾İĞÅÏ¢¿½±´ÖÁ net²ãÄÚ´æ³ØÄÚ´æ¿é
*@param : news cmd~µØÖ·Æ«ÒÆĞÅÏ¢~
*@retval: ³É¹¦ : net²ãÄÚ´æ³ØÄÚ´æ¿éµÄµØÖ·; Ê§°Ü : NULL
*/
void *copy_the_ui_cmd_news(const char *news);

//¶ÁÅäÖÃÎÄ¼ş£¬ »ñÈ¡²ÎÊı£º IP£¬ port µÈ£»
static int get_config_file_para();

 //choose The method to set local IP and gate;
 //1. DHCP, 2. manual set IP And gate
 int chose_setip_method();

/*print The  client request reply from server 
*@param : replyNews   reply content;  style : cmd~struct content~
*/
 void print_client_request(char *srcReplyNews);

 
 /*»ØÈ¥·¢ËÍÊı¾İ°üµÄÃüÁî×Ö ºÍ Á÷Ë®ºÅ
  *@param: sendData : Òª·¢ËÍµÄÊı¾İ°ü(×ªÒå¹ıµÄÍêÕûÊı¾İ°ü)
  *@param: sendDataLenth : ·¢ËÍÊı¾İ°üµÄ³¤¶È
  *@param: sendCmd : »ñÈ¡·¢ËÍÊı¾İ°üµÄÃüÁî×Ö
  *@param: serialNumber : »ñÈ¡·¢ËÍÊı¾İ°üµÄÁ÷Ë®ºÅ
  *@retval: ³É¹¦ 0; Ê§°Ü -1;
 */
int get_send_pack_cmd(char *sendData, int sendDataLenth, int *sendCmd, unsigned int *serialNumber);
int get_send_encryptpack_content(char *sendData, int sendDataLenth, int *sendCmd, char *flag);


/*·¢ËÍÍøÂçÊı¾İ°üÊ±£¬ÍøÂç¶Ï¿ª£¬ÅĞ¶Ï·¢ËÍÄÄÖÖÊı¾İ°ü*/
int send_error_netPack(int sendCmd);

/*³ö´íºó£¬ÇåÀíÈ«¾Ö±äÁ¿
 *@param: flag : 0 ±êÊ¶·¢ËÍÊ±ÇåÀí£¬ 1 ±êÊ¶½ÓÊÕÇåÀí
*/
void clear_global(int flag);

/*·¢ËÍµÇÂ¼Êı¾İ°üÊ±£¬ÍøÂç¶Ï¿ª£¬ ·¢ËÍÖ¸¶¨ÃüÁî×ÖÏÂÍøÂç¶Ï¿ªÊı¾İ°ü*/
int send_login_pack_netError();

/*·¢ËÍÍ¼Æ¬Êı¾İ°üÊ±£¬ÍøÂç¶Ï¿ª£¬ ·¢ËÍÖ¸¶¨ÃüÁî×ÖÏÂÍøÂç¶Ï¿ªÊı¾İ°ü*/
int send_upload_pack_netError();

/*Ğ£ÑéÏÂÔØµÄÎÄ¼şÊÇ·ñÎª×îĞÂ Êı¾İ°ü, ÍøÂç¶Ï¿ª, ·¢ËÍÖ¸¶¨ÃüÁî×ÖÏÂÍøÂç¶Ï¿ªÊı¾İ°ü*/
int send_checkout_fileID_newest_pack_netError();

//Get The specify File type NewEst ID
int  get_FileNewestID(const char* news);

/*·¢ËÍÏÂÔØÄ£°åÊı¾İ°ü, ÍøÂç¶Ï¿ª, ·¢ËÍÖ¸¶¨ÃüÁî×ÖÏÂÍøÂç¶Ï¿ªÊı¾İ°ü*/
int send_downLoadTemplate_pack_netError();

/*·¢ËÍĞÄÌøÊı¾İ°ü, ÍøÂç¶Ï¿ª, ·¢ËÍÖ¸¶¨ÃüÁî×ÖÏÂÍøÂç¶Ï¿ªÊı¾İ°ü*/
int send_heart_pack_netError();

/*·¢ËÍÏûÏ¢ÍÆËÍ, ÍøÂç¶Ï¿ª, ·¢ËÍÖ¸¶¨ÃüÁî×ÖÏÂÍøÂç¶Ï¿ªÊı¾İ°ü*/
int send__messagePush_pack_netError();

/*·¢ËÍÉ¾³ıÍ¼Æ¬Êı¾İ°ü, ÍøÂç¶Ï¿ª, ·¢ËÍÖ¸¶¨ÃüÁî×ÖÏÂÍøÂç¶Ï¿ªÊı¾İ°ü*/
int send_deletePicture_pack_netError();

//ÉÏ´«Ä£°åÉ¨ÃèÊı¾İ¹ı³ÌÖĞ,ÍøÂç´íÎó
int send_template_pack_netError();

//¼ÓÃÜ´«Êä»òÈ¡Ïû¼ÓÃÜ´«ÊäÊı¾İ°ü·¢ËÍ, ÍøÂç¶Ï¿ª´íÎó
int send_encryOrCancel_pack_netError();


/*·¢ËÍÊı¾İ¹ı³ÌÖĞ,ÍøÂç³ö´í,·¢ËÍÏß³Ì ·¢³öÖ¸¶¨Êı¾İ°üµÄ´íÎóĞÅÏ¢.
 *@param : cmd : ÉÏ´«Êı¾İ°üÀàĞÍµÄ±êÊ¶·û
 *@param : err: ²»Í¬Êı¾İ°üµÄÉÏ´«½á¹û
 *@param : errNum: ²»Í¬Êı¾İ°üµÄ³ö´íĞÅÏ¢±êÊ¶·û
 *@retval: ³É¹¦ 0; Ê§°Ü -1;
*/
int copy_news_to_mem(int cmd, int err, int errNum);

/*ÕÒ³öÖ¸¶¨Â·¾¶ÏÂÍ¼Æ¬µÄÃû³Æ
*@param : filePath  Ö¸¶¨Â·¾¶
*@param : desLenth  destBufµÄÄÚ´æ³¤¶È
*@param : destBuf	¿½±´µÄÇøÓò
*@retval: ³É¹¦ 0; Ê§°Ü -1;
*/
int find_file_name(char *destBuf, int desLenth, const char *filePath);

/*½ÓÊÕÏß³Ì: Êı¾İ°ü½ÓÊÕ³É¹¦ºó; 1.É¾³ı¸Õ²Å·¢ËÍµÄÊı¾İ°üµØÖ·, ²¢½«¸ÃµØÖ··Å»ØÄÚ´æ³Ø
* 2.É¾³ıÊ±¼äÁ´±íÖĞ¶ÔÓ¦µÄµØÖ·, ²¢ÊÍ·Å¸Ã½ÚµãÄÚ´æ
* 3.ÊÍ·Å·¢ËÍÊı¾İ°ü³¤¶ÈµÄµØÖ·.
*/
void remove_senddataserver_addr();

/*±È½Ï½ÓÊÕµÄÊÇ·ñÕıÈ·, ²ÉÓÃcrc32É¢ÁĞĞ£Ñé.
*@param : tmpHead ½ÓÊÕÊı¾İµÄ±¨Í·
*@param : tmpContent ½ÓÊÕÊı¾İµÄÄÚÈİ(±¨Í· + ±¨Ìå + Ğ£ÑéÂë)
*@param : tmpContentLenth Êı¾İÄÚÈİµÄ³¤¶È
*@retval: ³É¹¦ 0; Ê§°Ü -1;
*/
int compare_recvData_Correct(char *tmpContent, int tmpContentLenth);


/*¶ÔÊı¾İ°üµÄÍ·¸³Öµ
*@param : head ¸³ÖµµÄ±¨Í·
*@param : cmd ¸ÃÊı¾İ°üµÄÃüÁî×Ö
*@param : contentLenth ¸ÃÊı¾İ°üµÄ³¤¶È(±¨Í· + ÏûÏ¢Ìå)
*@param : subPackOrNo ¸Ã±¨ÎÄÊÇ·ñÎª·Ö°ü´«Êä
*@param : machine »úÆ÷Âë
*@param : totalPackNum ¸Ã±¨ÎÄµÄ×Ü´«Êä°üÊı
*@param : indexPackNum µ±Ç°´«Êä°üÊı
*/
void assign_reqPack_head(reqPackHead_t *head, uint8_t cmd, int contentLenth, int isSubPack, int currentIndex, int totalNumber, int perRoundNumber );
void assign_resComPack_head(resCommonHead_t *head, int cmd, int contentLenth, int serial, int isSubPack);


/*Í¼Æ¬Êı¾İ×é°ü
*@param : filePath  ÎÄ¼ş¾ø¶ÔÂ·¾¶
*@param : readLen Ã¿´Î¶ÁÈ¡ÎÄ¼şµÄ´óĞ¡
*@retval: ³É¹¦ 0; Ê§°Ü -1;
*/
int image_data_package(const char *filePath, int readLen, int fileNumber, int totalFileNumber);


/*´Ó·şÎñÆ÷½ÓÊÕÊı¾İ
 *²ÎÊı£º sockfd £º TCPÍøÂçÁ¬½ÓÌ×½Ó×Ö
 *·µ»ØÖµ£º ³É¹¦ 0£» Ê§°Ü£º -1£»
*/
int recv_data(int sockfd);

/*Ïò·şÎñÆ÷·¢ËÍÊı¾İ
 *²ÎÊı£º sockfd £º TCPÍøÂçÁ¬½ÓÌ×½Ó×Ö
 		 sendCmd : ·¢ËÍÊı¾İ°üµÄÃüÁî×Ö
 *·µ»ØÖµ£º ³É¹¦ 0£» Ê§°Ü£º -1£»
*/
int send_data(int sockfd, int *sendCmd);

/*Êı¾İ½øĞĞ×é°ü
 *²ÎÊı£º func ¶ÔÓ¦ÃüÁî×ÖµÄ»Øµ÷º¯Êı
 *		 news  ´ÓUI½çÃæ½ÓÊÕµÄÏûÏ¢
 *·µ»ØÖµ£º³É¹¦ 0£»   Ê§°Ü  -1£»
*/
int  data_packge(call_back_package  func, const char *news);

/*Ä£°åÉ¨Ãè,³É¹¦ºó,½øĞĞµÄÓ¦´ğ°üÊı¾İ´¦Àí
*@param : sendUiData  ½øĞĞ´¦ÀíµÄÊı¾İ°ü
*@param : buf  ·µ»Ø¸øUIµÄÊı¾İÄÚÈİ
*@retval: ³É¹¦ 0; Ê§°Ü -1;
*/
int reset_packet_send_ui(char *sendUiData, char **buf);

/*½â°üµÄÊı¾İ´¦Àí
 *²ÎÊı: 	news : ·şÎñÆ÷»Ø¸´µÄÓ¦´ğ;  °üº¬: ÏûÏ¢Í· + ÏûÏ¢Ìå + Ğ£ÑéÂë
 *			lenth: newsµÄ²¿·ÖÊı¾İ³¤¶È;   °üº¬: ÏûÏ¢Í· + Êı¾İÌå.
 *			outDataNews:  ½«´¦ÀíºóµÄÊı¾İ¿½±´ÖÁ ¸ÃµØÖ·ÄÚ´æ, ·µ»Ø¸ø UI½ø³Ì.
 *·µ»ØÖµ:  ³É¹¦: 0;  Ê§°Ü: -1;
*/
int data_un_packge(call_back_un_pack func_data,  char *news, char **outDataNews);


//////////// ·¢ËÍÊı¾İ°ü Óë Ó¦´ğÊı¾İ°üµÄ³öÀí·½·¨  //////////////


//net ·şÎñ²ã³ö´í, ×é°ü¸æËß UI ½ø³Ì  			0ºÅÊı¾İ°ü
int  error_net();

/*´¦ÀíµÇÂ¼Êı¾İ°ü
 *²ÎÊı£º  news  ´ÓUI½çÃæ½ÓÊÕµÄÏûÏ¢				1ºÅÊı¾İ°ü
 *·µ»ØÖµ£º³É¹¦ 0£»   Ê§°Ü  -1£»
*/
int  login_func(const char* news);

//ÍË³öÊı¾İ°üÏûÏ¢¸ñÊ½ £ºÏûÏ¢ÌåÎª¿Õ£¬Ö»ÓĞ±¨ÎÄÍ· 	2ºÅÊı¾İ°ü                       
int  exit_program(const char* news);

//ĞÄÌøÊı¾İ°üÏûÏ¢¸ñÊ½ £ºÏûÏ¢ÌåÎª¿Õ£¬Ö»ÓĞ±¨ÎÄÍ·   3ºÅÊı¾İ°ü                     
int  heart_beat_program(const char* news);

//ÏÂÔØÄ£°åÊı¾İ°ü¸ñÊ½£º 							4ºÅÊı¾İ°ü
int download_file_fromServer(const char *news);

//checkOut The downLoad file whether newest
int  checkOut_downLoad_file_newest(const char* news);

//Get The newest File ID
int get_FileNewestID_reply(  char *news, char **outDataToUi);

//¿Í»§¶ËĞÅÏ¢ÇëÇó								5ºÅÊı¾İ°ü
int  clinet_news_request(const char* news);

//ÉÏ´«É¨ÃèÍ¼Æ¬Êı¾İ°ü   news:°üº¬ÃüÁî×ÖºÍ¸ÃÍ¼Æ¬ËùÔÚÎ»ÖÃ¡£		6ºÅÊı¾İ°ü
int  upload_picture(const char *news);

//·şÎñÆ÷ÏûÏ¢ÍÆËÍ»Ø¸´Êı¾İ°ü
int  push_info_from_server_reply(const char* news);


//net ·şÎñ²ã¹Ø±Õ, ×é°ü¸æËß UI ½ø³Ì  			8ºÅÊı¾İ°ü
int  active_shutdown_net();

//É¾³ıÍ¼Æ¬Êı¾İ°ü								9ºÅÊı¾İ°ü
int delete_spec_pic(const char *news);

// Á´½Ó·şÎñÆ÷									10ºÅÊı¾İ°ü
int  connet_server(const char *news);

//ĞŞ¸ÄÎÄ¼şºó,ÖØĞÂ¶ÁÈ¡Êı¾İ						11ºÅÊı¾İ°ü
int read_config_file(const char *news);

/*µÚ¶ş°æ: ´«ÊäÍØÕ¹°ü: °üº¬ Ñ§ÉúID, ±¾¿ÆÊÔ¾íµÄÉ¨ÃèÍ¼Æ¬·İÊı, ¸÷Í¼Æ¬Ãû×Ö, ±¾¿ÆÊÔ¾íµÄ¿Í¹ÛÌâ´ğ°¸, Ö÷¹ÛÌâÇĞÍ¼¸öÊı, Ö÷¹ÛÌâËùÓĞÇĞÍ¼×ø±êµã
*@param : news  UI-->netĞÅÏ¢Êı¾İ°ü; ¸ñÊ½Îª: cmd~data~;  data : ÄÚ´æÊı¾İµØÖ·
*@retval: ³É¹¦ 0; Ê§°Ü -1.						12ºÅÊı¾İ°ü
*/
int template_extend_element(const char *news);

/*ÉÏ´«Ò»Ì×ÊÔ¾íµÄÍ¼Æ¬(¼´¶àÕÅÍ¼Æ¬´«Êä, ´«Êä¹ı³ÌÖĞ³ö´í,µ×²ã³ÌĞò»á¸ù¾İ´íÎóÀàĞÍ×Ô¶¯ÖØ´«»ò½øĞĞ±¨´í(ÓĞµÄ´íÎó²»ÄÜ½øĞĞÖØ´«))
*@param : uploadSet  Í¼Æ¬¼¯ºÏ					Î±13ºÅÊı¾İ°ü.(ÏÖ½×¶ÎÖ»ÔÚnet²ãºÍserver²ãÖ®¼äÊ¹ÓÃ)
*@retval: ³É¹¦ 0; Ê§°Ü -1;
*/
int upload_template_set(uploadWholeSet *uploadSet);

//encrypt transfer or No cancel encrypt tansfer
int  encryp_or_cancelEncrypt_transfer(const char *news);


//µ×²ã net ·şÎñ³ö´íÊı¾İ°ü			0ºÅÓ¦´ğ°ü
int error_net_reply( char *news, char **outDataNews);

//µÇÂ¼Ó¦´ğ°ü						1ºÅÓ¦´ğ°ü
int login_reply_func( char *news, char **outDataNews);

//ÍË³öÓ¦´ğ°ü						2ºÅÓ¦´ğ°ü
int exit_reply_program( char *news,  char **outDataNews);

//ĞÄÌøÓ¦´ğ°ü						3ºÅÓ¦´ğ°ü
int heart_beat_reply_program( char *news, char **outDataNews);

//ÏÂÔØÄ£°åÓ¦´ğ°ü					4ºÅÓ¦´ğ°ü
int download_reply_template( char *news, char **outDataNews);

//Ğ£ÑéÏÂÔØµÄÎÄ¼şÊÇ·ñÎª×îĞÂ			5ºÅÊı¾İ°ü
int checkOut_downLoad_file_newest_reply( char *news, char **outDataToUi);


//¿Í»§¶ËĞÅÏ¢ÇëÇóÓ¦´ğ°ü				5ºÅÓ¦´ğ°ü
int clinet_news_reply_request( char *news, char **outDataNews);

//ÉÏ´«É¨ÃèÍ¼Æ¬Ó¦´ğ°ü				6ºÅÓ¦´ğ°ü
int upload_picture_reply_request( char *news, char **outDataNews);

//·şÎñÆ÷ÍÆËÍÏûÏ¢¹¦ÄÜÊµÏÖ
int push_info_from_server( char *news, char **outDataToUi);

//µ×²ã net ·şÎñÖ÷¶¯¹Ø±ÕÊı¾İ°ü		8ºÅÓ¦´ğ°ü
int active_shutdown_net_reply( char *news, char **outDataNews);

//É¾³ıÍ¼Æ¬Ó¦´ğ Ó¦´ğÊı¾İ°ü			9ºÅÓ¦´ğ°ü
int delete_spec_pic_reply( char *news, char **outDataNews);

//Á´½Ó·şÎñÆ÷Ó¦´ğ;  news: ÏûÏ¢Í· + ÏûÏ¢Ìå(Á´½Ó·şÎñÆ÷½á¹û)			»Ø¸´¸ø UI ½ø³ÌµÄÏûÏ¢: 0x8A~0~
									//10ºÅÊı¾İ°üÓ¦´ğ
int connet_server_reply( char *news,  char **outDataNews);

//¶ÁÈ¡ÅäÖÃÎÄ¼şÓ¦´ğ; news: ÏûÏ¢Í· + ÏûÏ¢Ìå(¶ÁÈ¡ÅäÖÃÎÄ¼şÊÇ·ñ³É¹¦);  »Ø¸´¸ø UI ½ø³ÌµÄÏûÏ¢: 0x8B~0~
									//11ºÅÊı¾İ°üÓ¦´ğ
int read_config_file_reply( char *news, char **outDataNews);

//ÉÏ´«ÕûÌ×Êı¾İ°üµÄÓ¦´ğ; news: ÏûÏ¢Í· + ÏûÏ¢Ìå(¶ÁÈ¡ÅäÖÃÎÄ¼şÊÇ·ñ³É¹¦);  »Ø¸´¸ø UI ½ø³ÌµÄÏûÏ¢: 0x8B~0~
									//12ºÅÊı¾İ°ü
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
