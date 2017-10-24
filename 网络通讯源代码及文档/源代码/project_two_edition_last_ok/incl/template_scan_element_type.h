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


//上传到服务器格式  Net --> SERVER
// 1 + 128 + 2 + 400 + 1 + 1 + 50 *10 = 1033
typedef struct _SendServerExFrame{
	
	unsigned char IDLen;					// 考生 ID的长度; 
	char ID[128];							// 此数组内容为 ASCII码,
	unsigned short objectLenth;				// 客观题答案长度 
	char objectAnswer[400];					// 客观题答案, 格式 A~AB~D等	
	unsigned char totalNumber;				// 本套试卷的扫描图片个数
	unsigned char  subMapCoordNum;			// 主观题切图个数;
	CoordsArray coordsValSet;				// 本套试卷所有切图坐标点集合; Coords[50]	 	n : 切图个数
		
}SendServerExFrame;


#if 1		

//UI ---> net 的信息, 应该包含文件的路径; 测试结构体 
typedef struct{
	
	char fileDir[256];					//文件所在的目录
	templateScanElement sendData;		//拓展信息
	
}UitoNetExFrame;


//1+1+256 = 258;
typedef struct _option_content_
{
	unsigned char 	optionID;
	unsigned char 	optionNameLenth;
	char 			optionName[256];
	
}optionContent;
typedef  optionContent optionArray[10]	;

//1+1+256+1+258*10 = 2839
typedef struct _project_content_
{
	unsigned char 	projectID;
	unsigned char 	projectNameLenth;
	char 			projectName[256];
	unsigned char 	optionNum;
	optionArray 	optionSet;	
}projectContent;
typedef projectContent  projectArray[10]  ;

//1+2839*10 = 28391 == 28K	  Net ---> UI (client request's reply data from server )
typedef struct _client_request_content_
{
	unsigned char 	projectNum;
	projectArray  	projectSet;
}clientRequestContent;		



//1+1 = 2
typedef struct _select_content_
{
	unsigned char projectID;	
	unsigned char optionID;

}selectContent;
typedef selectContent  requestContent[10]	;

//1+2*10 = 21		UI --->  Net ---> server (The downLoad template request news)
typedef struct _downLoad_template_request_
{
	unsigned char   projectNum;
	requestContent  conData;

}downLoadTemplateRequest;




#endif

#pragma pack(pop)

#if defined(__cplusplus)
}
#endif


#endif

