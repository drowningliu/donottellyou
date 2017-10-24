#ifndef DHCP_H_
#define DHCP_H_



/*开启 DHCP 服务, 
 *@param : pid : 开启的子线程 id
 *@retval : 成功 0; 失败 -1;
*/
int dhcp_interface(pid_t *pid);


#endif