#ifndef _DATA_PACKAGE_H_
#define _DATA_PACKAGE_H_

#if defined(__cplusplus)
extern "C" {
#endif


///////////////////////////////客户端网络端初始化、销毁 与 UI线程收发数据////////////////////////////////////////////
/*客户端初始化
 *创建一个线程去链接服务器。
 *返回值： 0  成功； 失败 -1；
*/
int init_client_net(char *);

/*从UI界面接收消息
 *参数：news:接收的消息;  包含内容：1.该条消息的命令字，unsigned char型
 *									2.消息发送服务器是否加密（char型 0x00.加密， 0x01.不加密），	
 * 									3.消息的具体内容。 
 *						消息格式：11accout:123213123passwd:qgdyuwgqu455;   (描述：登录命令，不加密，账号：123213123，密码:qgdyuwgqu455) 	                                          
 *返回值：成功 0；   失败 -1；
*/
int  recv_ui_data(const char *news);

/*将解包的数据发送给 UI 界面
 *参数： news ：  将信息拷贝给该地址；   被调函数分配内存 
 *返回值： 0 成功；  -1 失败；
*/
int  send_ui_data(char **buf);

/*客户端申请的释放内存
 *参数： buf: 将要释放的内存
 *返回值： 成功 0； 失败 -1；
*/
int free_memory_net(char *buf);

/*客户端的线程销毁
 *功能： 释放全局变量， 关闭套接字；
 *返回值： 成功 0； 失败 -1；
*/
int destroy_client_net();

//////////////////////////////////////////////////////////////////////////////


#if defined(__cplusplus)
}
#endif

#endif