//template_scan_element_type.h 队列头文件

#ifndef _TEMPLATE_SCAN_ELEMENT_TYPE_H_
#define _TEMPLATE_SCAN_ELEMENT_TYPE_H_


#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(push)
#pragma pack(1)



//切图坐标点与对应的标识符
//2 * 5 = 10
typedef struct _Coords{
           
	unsigned short MapID;
	unsigned short TopX;
	unsigned short TopY;
	unsigned short DownX;
	unsigned short DownY;

}Coords; 


typedef Coords CoordsArray[50];			//数组类型

//传输的数据结构    SCAN --> UI 
//该结构的大小: 128 + 400 + 500 + 1 + 1 + 500 = 1530 字节
typedef struct _templateScanElement{

	char identifyID[128];					//标识符ID
	char objectAnswer[400];					//客观题答案. 预设最小容量: 每个题5个字节,可装80个.  格式为:  A~BC~AB~ABCD~C~;  C字符串风格,以\0结尾
	unsigned char totalNumberPaper;			//试卷总份数
	char toalPaperName[10][50];				//所有试卷的文件名		500
	unsigned char subMapCoordNum;			//主观切图个数
	CoordsArray  coordsDataToUi;			//本套试卷,所有主观题切图坐标点集合
	
}templateScanElement;



//UI ---> net 的信息, 应该包含文件的路径; 测试结构体 
typedef struct{
	
	char fileDir[256];					//文件所在的目录
	templateScanElement sendData;		//拓展信息
	
}UitoNetExFrame;



#pragma pack(pop)

#if defined(__cplusplus)
}
#endif


#endif

