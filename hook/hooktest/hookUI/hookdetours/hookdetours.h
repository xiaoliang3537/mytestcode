// hookdetours.h : hookdetours DLL ����ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// ChookdetoursApp
// �йش���ʵ�ֵ���Ϣ������� hookdetours.cpp
//

class ChookdetoursApp : public CWinApp
{
public:
	ChookdetoursApp();

// ��д
public:
	virtual BOOL InitInstance();
    virtual int ExitInstance();
	DECLARE_MESSAGE_MAP()
};