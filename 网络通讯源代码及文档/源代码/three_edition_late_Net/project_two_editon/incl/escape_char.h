//对大块内存数据中的特定标识符进行转义

#ifndef _ESCAPE_CHAR_H_
#define _ESCAPE_CHAR_H_

#if defined(__cplusplus)
extern "C" {
#endif

//对输入的数据进行转义,  该函数中都是主调函数分配内存
int  escape(char sign, char *inData, int inDataLenth, char *outData,  int *outDataLenth);

//对输入的数据进行解转义,  该函数中都是主调函数分配内存
int  anti_escape(char *inData, int inDataLenth, char *outData,  int *outDataLenth);


#if defined(__cplusplus)
}
#endif


#endif
