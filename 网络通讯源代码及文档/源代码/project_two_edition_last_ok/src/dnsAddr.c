#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <netdb.h>  
#include <arpa/inet.h>  
  
#include "socklog.h"
//#define DNS_SVR "202.96.199.133"  
//#define DNS_SVR "192.168.1.1" 
#define DNS_PORT 53  
#define IPLENTH 20
 
#define DNS_HOST  0x01  
#define DNS_CNAME 0x05  
  
static int socketfd;  
static struct sockaddr_in dest;  


  
  //发送DNS请求  
static void send_dns_request(const char *dns_name);  
  
  //解析DNS应答
static int parse_dns_response(char *ip);  
  
/**  
 * Generate DNS question chunk  (生成DNS请求)
 */  
static void  generate_question(const char *dns_name , unsigned char *buf , int *len);  
  
/**  
 * Check whether the current byte is  （检测当前的额字节是指针还是长度）
 * a dns pointer or a length  
 */  
static int is_pointer(int in);  
  
/**  
 * Parse data chunk into dns name  依据域名解析DNS IP地址。
 * @param chunk The complete response chunk  
 * @param ptr The pointer points to data  
 * @param out This will be filled with dns name  
 * @param len This will be filled with the length of dns name  
 */  
static void parse_dns_name(unsigned char *chunk , unsigned char *ptr , char *out , int *len);  
  
  //获取 DNS 服务器解析后的IP地址
int getDnsIp(char *doMainName, char *dnsServerIp, char *addrIp)
{  
	int ret = 0;

    if(doMainName == NULL || dnsServerIp == NULL || addrIp == NULL)
	{
		ret = -1;
		socket_log( SocketLevel[4], ret, "Error: doMainName == NULL || dnsServerIp == NULL || addrIp == NULL");	
		goto End;
	}
    
    //创建网络套接字
    socketfd = socket(AF_INET , SOCK_DGRAM , 0);  
    if(socketfd < 0)
    {  
    	ret = -1;
    	socket_log( SocketLevel[4], ret, "Error: func socket(), create socket failed");	
       	goto End;
    }  
    

    //指定 DNS 服务器IP地址与port端口
    bzero(&dest , sizeof(dest));  
    dest.sin_family = AF_INET;  
    dest.sin_port = htons(DNS_PORT);  
    dest.sin_addr.s_addr = inet_addr(dnsServerIp);  
  
    send_dns_request(doMainName);  
  
    ret = parse_dns_response(addrIp);    	
  	if(ret)
	{
		socket_log( SocketLevel[4], ret, "Error: func parse_dns_response()");				
		goto End;	
	}
  	close(socketfd);
  	
  End:
    return ret;  
}  

//解析应答
static int parse_dns_response(char *ip){  
  	
    unsigned char buf[1024];  
    unsigned char *ptr = buf;  
    struct sockaddr_in addr;  
   
    //int i , n, ttl, flag , querys , answers, ret = 0;  
    int i , n, flag , querys , answers, ret = 0; 
    int type , datalen , len;  
    char  aname[128] ;
    //char *ip = malloc(IPLENTH);  
    unsigned char netip[4];  
    size_t addr_len = sizeof(struct sockaddr_in);  
  
    n = recvfrom(socketfd , buf , sizeof(buf) , 0, (struct sockaddr*)&addr , &addr_len); 
    if(n < 0)
    {
    	socket_log( SocketLevel[4], n, "Error: func recvfrom()");	
       	goto End;
    }
    ptr += 4; /* move ptr to Questions */  
    querys = ntohs(*((unsigned short*)ptr));  
    ptr += 2; /* move ptr to Answer RRs */  
    answers = ntohs(*((unsigned short*)ptr));  
    ptr += 6; /* move ptr to Querys */  
    /* move over Querys */  
    for(i= 0 ; i < querys ; i ++){  
        for(;;){  
            flag = (int)ptr[0];  
            ptr += (flag + 1);  
            if(flag == 0)  
                break;  
        }  
        ptr += 4;  
    }  
    
    /* now ptr points to Answers */  
    for(i = 0 ; i < answers ; i ++)
    {  
        bzero(aname , sizeof(aname));  
        len = 0;  
        parse_dns_name(buf , ptr , aname , &len);  
        ptr += 2; /* move ptr to Type*/  
        type = htons(*((unsigned short*)ptr));  
        ptr += 4; /* move ptr to Time to live */  
        //ttl = htonl(*((unsigned int*)ptr));  
        ptr += 4; /* move ptr to Data lenth */  
        datalen = ntohs(*((unsigned short*)ptr));  
        ptr += 2; /* move ptr to Data*/  
        if(type == DNS_HOST){  
           // bzero(ip , IPLENTH);  
            if(datalen == 4){  
                memcpy(netip , ptr , datalen);  
                inet_ntop(AF_INET , netip , ip , sizeof(struct sockaddr));  
                //printf("%s\n" ,  ip);                  
            }  
            ptr += datalen;  
        }    
    }  
  	
  	if(*ip == 0)
  	{
  		return -1;
  	}

End:  	
	
    return ret;
}  
  
static void  
parse_dns_name(unsigned char *chunk, unsigned char *ptr , char *out , int *len){  
    int n  , flag;  
    char *pos = out + (*len);  
  
    for(;;){  
        flag = (int)ptr[0];  
        if(flag == 0)  
            break;  
        if(is_pointer(flag)){  
            n = (int)ptr[1];  
            ptr = chunk + n;  
            parse_dns_name(chunk , ptr , out , len);  
            break;  
        }else{  
            ptr ++;  
            memcpy(pos , ptr , flag);  
            pos += flag;  
            ptr += flag;  
            *len += flag;  
            if((int)ptr[0] != 0){  
                memcpy(pos , "." , 1);  
                pos += 1;  
                (*len) += 1;  
            }  
        }  
    }  
  
}  
  
static int is_pointer(int in){  
    return ((in & 0xc0) == 0xc0);  
}  

//发送域名请求，传入的参数为（要解析的域名）
static void send_dns_request(const char *dns_name){  
  
    unsigned char request[256];  
    unsigned char *ptr = request;  
    unsigned char question[128];  			//域名访问的应答
    int question_len;  						//应答长度
  
  //生成指定域名应答信息
    generate_question(dns_name , question , &question_len);  
	  
  //将（请求空间的大地址）强转为（short类型的小地址），然后拷贝进该地址中
    *((unsigned short*)ptr) = htons(0xff00);  
    ptr += 2;  
    *((unsigned short*)ptr) = htons(0x0100);  
    ptr += 2;  
    *((unsigned short*)ptr) = htons(1);  
    ptr += 2;  
    *((unsigned short*)ptr) = 0;  
    ptr += 2;  
    *((unsigned short*)ptr) = 0;  
    ptr += 2;  
    *((unsigned short*)ptr) = 0;  
    ptr += 2;  
    //将（应答的内容拷贝进）该（请求空间）
    memcpy(ptr , question , question_len);  
    ptr += question_len;  
  
  	//向链接的套接字发送请求内容
    sendto(socketfd , request , question_len + 12 , 0  
       , (struct sockaddr*)&dest , sizeof(struct sockaddr));  
}  
/*
	// 生成指定域名应答新信息
	// 找到需要访问的域名中以 “.” 作为分割的前半部分字符串，拷贝进应答中没然后再拷贝 1 1；输出该应答及其长度
	//1.用户指定访问的域名
	//2.应答空间（做输入）
	//3.应答长度（做输出）
*/  
static void  
generate_question(const char *dns_name , unsigned char *buf , int *len){  
    char *pos;  
    unsigned char *ptr;  
    int n;  
  
    *len = 0;  
    ptr = buf;  					//指向 应答的空间
    pos = (char*)dns_name;  		//指向 将要访问的域名
    for(;;){  
        n = strlen(pos) - (strstr(pos , ".") ? strlen(strstr(pos , ".")) : 0);   //计算 “.” 前面字符串的长度
        *ptr ++ = (unsigned char)n;  
        memcpy(ptr , pos , n);  
        *len += n + 1;  
        ptr += n;  
        //判断输入的域名中是否存在 “.”；
        if(!strstr(pos , ".")){  
            *ptr = (unsigned char)0;  
            ptr ++;  
            *len += 1;  
            break;  
        }  
        pos += n + 1;  
    }  
    *((unsigned short*)ptr) = htons(1);  
    *len += 2;  
    ptr += 2;  
    *((unsigned short*)ptr) = htons(1);  
    *len += 2;  
}  

