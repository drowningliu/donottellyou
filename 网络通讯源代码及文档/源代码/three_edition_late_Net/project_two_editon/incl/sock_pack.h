#ifndef _SOCK_PACK_H_
#define _SOCK_PACK_H_


#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#define PACKMAXLENTH  	1400		 //���ĵ���󳤶�;		���Ľṹ����ʶ 1�� ͷ 26���壬 У�� 4�� ��ʶ 1
#define COMREQDATABODYLENTH 	PACKMAXLENTH - sizeof(reqPackHead_t) - sizeof(char) * 2 - sizeof(int)         //���峤�����: 1400-25-2-4 = 1369;  ������ͼƬʱ, ͼƬ������Ϣ����ռ46���ֽ�
#define REQTEMPERRBODYLENTH		PACKMAXLENTH - sizeof(reqTemplateErrHead_t) - sizeof(char) * 2 - sizeof(int)
#define COMRESBODYLENTH			PACKMAXLENTH - sizeof(resCommonHead_t) - sizeof(char) * 2 - sizeof(int)
#define RESTEMSUBBODYLENTH		PACKMAXLENTH - sizeof(resSubPackHead_t) - sizeof(char) * 2 - sizeof(int)
#define FILENAMELENTH 	46			//�ļ�����
#define PACKSIGN      	0x7e		//��ʶ��
#define PERROUNDNUMBER	50			//ÿ�ַ��͵����ݰ������Ŀ
#define OUTTIMEPACK 	5			//�ͻ��˽���Ӧ��ĳ�ʱʱ��

enum HeadNews{
		NOSUBPACK = 0,		//���Ӱ�
		SUBPACK = 1			//�Ӱ�
	};		

enum Cmd{
		ERRORFLAGMY			= 0x00,		//����
		LOGINCMDREQ 		= 0x01,		//��¼
		LOGINCMDRESPON 		= 0x81,
		LOGOUTCMDREQ 		= 0x02,		//�ǳ�
		LOGOUTCMDRESPON 	= 0x82,
		HEARTCMDREQ 		= 0x03,		//����
		HEARTCMDRESPON 		= 0x83,
		DOWNFILEREQ			= 0x04,		//����ģ��
		DOWNFILEZRESPON 	= 0x84,
		NEWESCMDREQ 		= 0x05,		//��ȡģ�����±��
		NEWESCMDRESPON 		= 0x85,
		UPLOADCMDREQ 		= 0x06,		//�ϴ�ͼƬ
		UPLOADCMDRESPON 	= 0x86,
		PUSHINFOREQ 		= 0x07, 	//��Ϣ����
		PUSHINFORESPON 		= 0x87,
		ACTCLOSEREQ 		= 0x08, 	//�����ر�
		ACTCLOSERESPON 		= 0x88,
		DELETECMDREQ 		= 0x09, 	//ɾ��ͼƬ
		DELETECMDRESPON 	= 0x89,
		CONNECTCMDREQ 		= 0x0A, 	//���ӷ�����
		CONNECTCMDRESPON 	= 0x8A,
		MODIFYCMDREQ 		= 0x0B, 	//�޸������ļ�
		MODIFYCMDRESPON 	= 0x8B,
		TEMPLATECMDREQ 		= 0x0C, 	//ģ�����ݰ�
		TEMPLATECMDRESPON 	= 0x8C,
		MUTIUPLOADCMDREQ	= 0x0D, 	//����ͼƬ����
		MUTIUPLOADCMDRESPON = 0x8D,
		ENCRYCMDREQ 		= 0x0E, 	//���ܴ���
		ENCRYCMDRESPON 		= 0x8E
	};

#pragma pack(push)
#pragma pack(1)

//ɨ�����������ݰ���ͷ
typedef struct _reqPackHead_t{
    uint8_t         cmd;                //������
    uint16_t        contentLenth;       //���ȣ� ��������ͷ����Ϣ�峤��
    uint8_t         isSubPack;          //�ж��Ƿ�Ϊ�Ӱ�, 0 ����, 1 ��;
    uint8_t         machineCode[12];    //������
    uint32_t        seriaNumber;        //��ˮ��
    uint16_t        currentIndex;       //��ǰ�����
    uint16_t        totalPackage;       //�ܰ���
    uint8_t         perRoundNumber;     //���ִ�������ݰ�����
}reqPackHead_t;
//1 + 2 + 1 + 12 + 4 + 2 + 2 + 1 = 25;


//����������ͨӦ��
typedef struct _resCommonHead_t{
    uint8_t     cmd;                //������
    uint16_t    contentLenth;       //���ȣ� ��������ͷ����Ϣ�峤��
    uint8_t     isSubPack;          //�ж��Ƿ�Ϊ�Ӱ�, 			0 ����, 1 ��;
    uint32_t    seriaNumber;        //��ˮ��
    uint8_t     isFailPack;     	//�Ƿ�ʧ��					0�ɹ�, 1 ʧ��
    uint16_t    failPackIndex;      //ʧ�����ݰ������к�
	uint8_t		isRespondClient;	//�Ƿ���Ҫ�ͻ�������������Ӧ,  0 ����Ҫ, 1 ��Ҫ����������Ӧ	
}resCommonHead_t;
//1 + 2 + 1 + 4 + 1 + 2 + 1 = 12

//�ӷ����������ļ�Ӧ��
typedef struct _resSubPackHead_t{
    uint8_t     cmd;                //������
    uint16_t    contentLenth;       //���ȣ� ��������ͷ����Ϣ�峤��
    uint32_t    seriaNumber;        //��ˮ��
    uint16_t    currentIndex;       //��ǰ�����
    uint16_t    totalPackage;       //�ܰ���
}resSubPackHead_t;
//1 + 2 + 4 + 2 + 2 = 11;




#if 0

/*����1
�ְ���Ϣ, ��Ϊ����, һ�ֿͻ���һ������,��������ģ��;�������Ӧ��(ģ������)
�ͻ���ÿ��..

һ�ֿͻ��˶�δ���, ������һ��Ӧ��(��: ͼƬ����)
�������� ���е����ݰ����ɹ�����:
totalPackge : �����յ�������ֱ�Ӹ�ֵ��Ӧ��
indexPackge : �����׶ν��յ������һ(������)���ݰ����кŸ�ֵ����
�����ݰ�����ʧ��:
failNumber : ����ʧ�ܵĸ���
failRecvPackIndex : ����ʧ�ܵ����ݰ����
*/

//ɨ���ǽ��շ����������ļ��Ӱ������, ����������󵥰����ݵı�ͷ
typedef struct _reqTemplateErrHead_t{
    uint8_t     cmd;                //������
    uint16_t    contentLenth;       //���ȣ� ��������ͷ����Ϣ�峤��
    uint8_t     isSubPack;          //�ж��Ƿ�Ϊ�Ӱ�, 0 ����, 1 ��;
    uint8_t     machineCode[12];    //������
    uint32_t    seriaNumber;        //��ˮ��
    uint8_t     failPackNumber;     //�Ƿ�ʧ��
    uint16_t    failPackIndex;      //ʧ�����ݰ������к�
}reqTemplateErrHead_t;
//1 + 2 + 1 + 12 + 4 + 1 + 2 = 23;

#endif


#pragma pack(pop)

#if defined(__cplusplus)
}
#endif

#endif
