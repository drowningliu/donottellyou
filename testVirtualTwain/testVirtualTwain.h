
// testVirtualTwain.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CtestVirtualTwainApp: 
// �йش����ʵ�֣������ testVirtualTwain.cpp
//

class CtestVirtualTwainApp : public CWinApp
{
public:
	CtestVirtualTwainApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CtestVirtualTwainApp theApp;