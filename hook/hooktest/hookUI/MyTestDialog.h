#pragma once

#include "dataprocess.h"
#include <map>

// CMyTestDialog 对话框
using namespace std;

class CMyTestDialog : public CDialog
{
	DECLARE_DYNAMIC(CMyTestDialog)

public:
	CMyTestDialog(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CMyTestDialog();

// 对话框数据
	enum { IDD = IDD_MYTESTDIALOG };

    int updateImg(ST_ImgInfo& info);
    int updateString(ST_ImgInfo& info);

    int setItem(int id, CWnd* wnd);
protected:
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

    ST_ImgInfo m_stImgInfo;
    ST_ImgInfo m_stStrInfo;

    std::map<int, CWnd*> m_mapItem;
    Gdiplus::Image *m_pImage;
};
