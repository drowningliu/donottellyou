#ifndef _DNS_SERVER_H_
#define _DNS_SERVER_H_

#if defined(__cplusplus)
extern "C" {
#endif


  /*��ȡ DNS �������������IP��ַ
  *	������  doMainName: ��Ҫ����������
  *	  		dnsServerIp: dns�������� ������IP
  *			addrIp: ���������� ��ȡ�ķ�����IP
  * ����ֵ�� �ɹ��� 0��  ʧ�ܣ� -1��
  */
int getDnsIp(char *doMainName, char *dnsServerIp, char *addrIp);


#if defined(__cplusplus)
}
#endif

#endif