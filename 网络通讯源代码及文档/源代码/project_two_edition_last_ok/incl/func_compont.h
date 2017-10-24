#ifndef _FUNC_COMPONT_H_
#define _FUNC_COMPONT_H_

#if defined(__cplusplus)
extern "C" {
#endif


#include <semaphore.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>


#include "socklog.h"


#define myprint( x...) do {char bufMessagePut_Stdout_[1024];\
        sprintf(bufMessagePut_Stdout_, x);\
        fprintf(stdout, "%s [%d], [%s]\n", bufMessagePut_Stdout_,__LINE__, __FILE__ );\
    }while (0) 



/*将传入的字符串转换为无符号的的32位整形
 *@param: str : 传入的字符串
 *retval: The converted value.
*/
unsigned int atoui(const char *str);


/*将uint32 类型转换为 C字符串, 默认采用 10进制
*@param : n 要转换的数字
*@param : s 转换后的字符串(默认生成 十进制字符串)
*@retval: 与s一样.
*/
char* uitoa(unsigned int n, char *s);


/*将C风格 16进制的字符串 转换为十进制(即: 默认字符串中存的内容为 16进制数据)
*@param : src : 保存16进制数据的字符串
*@retval: 转换后的数值
*/
unsigned int hex_conver_dec(char *src);

/*求x的y幂次方值.  即 : x^y
*@param : x 根
*@param : y 幂
*@retval: 求出的值
*/
unsigned int power(int x, int y);

//��ʵ�� system ϵͳ����
int pox_system(const char *cmd_line);

/*��ȡָ���ź�����ֵ.
*@param : sem ָ�����ź���
*@param : value ���ź�����ֵ
*/
void get_sem_value(sem_t *sem, int *value);

//��ȡ�ļ��ְ����ܰ���,���ļ��ܴ�С
int get_file_size(const char *filePath, int *number, int contentLenth, unsigned long *fileSizeSrc);

/*crc32 checkCode
*@param : buf  The content will generate checkcode 
*@param : size The content Lenth
*@retval: return The checkCode
*/
int crc326(const  char *buf, uint32_t size);

bool if_file_exist(const char *filePath);


#if defined(__cplusplus)
}
#endif


#endif
