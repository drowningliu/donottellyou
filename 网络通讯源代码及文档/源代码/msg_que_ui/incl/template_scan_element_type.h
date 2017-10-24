//template_scan_element_type.h ����ͷ�ļ�

#ifndef _TEMPLATE_SCAN_ELEMENT_TYPE_H_
#define _TEMPLATE_SCAN_ELEMENT_TYPE_H_


#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(push)
#pragma pack(1)



//��ͼ��������Ӧ�ı�ʶ��
//2 * 5 = 10
typedef struct _Coords{
           
	unsigned short MapID;
	unsigned short TopX;
	unsigned short TopY;
	unsigned short DownX;
	unsigned short DownY;

}Coords; 


typedef Coords CoordsArray[50];			//��������

//��������ݽṹ    SCAN --> UI 
//�ýṹ�Ĵ�С: 128 + 400 + 500 + 1 + 1 + 500 = 1530 �ֽ�
typedef struct _templateScanElement{

	char identifyID[128];					//��ʶ��ID
	char objectAnswer[400];					//�͹����. Ԥ����С����: ÿ����5���ֽ�,��װ80��.  ��ʽΪ:  A~BC~AB~ABCD~C~;  C�ַ������,��\0��β
	unsigned char totalNumberPaper;			//�Ծ��ܷ���
	char toalPaperName[10][50];				//�����Ծ���ļ���		500
	unsigned char subMapCoordNum;			//������ͼ����
	CoordsArray  coordsDataToUi;			//�����Ծ�,������������ͼ����㼯��
	
}templateScanElement;


//�ϴ�����������ʽ  Net --> SERVER
// 1 + 128 + 2 + 400 + 1 + 1 + 50 *10 = 1033
typedef struct _SendServerExFrame{
	
	unsigned char IDLen;					// ���� ID�ĳ���; 
	char ID[128];							// ����������Ϊ ASCII��,
	unsigned short objectLenth;				// �͹���𰸳��� 
	char objectAnswer[400];					// �͹����, ��ʽ A~AB~D��	
	unsigned char totalNumber;				// �����Ծ��ɨ��ͼƬ����
	unsigned char  subMapCoordNum;			// ��������ͼ����;
	CoordsArray coordsValSet;				// �����Ծ�������ͼ����㼯��; coordsValSet[n][20]	 	n : ͼƬ����
		
}SendServerExFrame;


#if 1		

//UI ---> net ����Ϣ, Ӧ�ð����ļ���·��; ���Խṹ�� 
typedef struct{
	
	char fileDir[256];					//�ļ����ڵ�Ŀ¼
	templateScanElement sendData;		//��չ��Ϣ
	
}UitoNetExFrame;


#endif

#pragma pack(pop)

#if defined(__cplusplus)
}
#endif


#endif

