#ifndef _DNS_SERVER_H_
#define _DNS_SERVER_H_

#if defined(__cplusplus)
extern "C" {
#endif


  /*获取 DNS 服务器解析后的IP地址
  *	参数：  doMainName: 需要解析的域名
  *	  		dnsServerIp: dns域名解析 服务器IP
  *			addrIp: 解析域名后， 获取的服务器IP
  * 返回值： 成功： 0；  失败： -1；
  */
int getDnsIp(char *doMainName, char *dnsServerIp, char *addrIp);


#if defined(__cplusplus)
}
#endif

#endif