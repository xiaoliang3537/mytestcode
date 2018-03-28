#pragma once

#include <cstring>
#include <vector>
#include <gdiplus.h>
#include <afxmt.h>
#include <stdio.h>
#include <atlimage.h>
#include <assert.h>
#include <GdiPlusTypes.h>
#include "Resource.h"
using namespace std;
using namespace Gdiplus;

// ���ݴ������

#define MAX_ROW 13
#define MAX_COL 8
#define MAX_LEN 256;

// ͼƬλ�û����ϲ���仯��  
struct ST_ImgInfo
{
    Gdiplus::RectF rectReal;
    Gdiplus::RectF rectImg;
    // ͼƬ����
    IStream* stream;
    CString strInfo;
    CWnd* wnd;
};


/////////////////////////////////
class CMyDialog : public CDialog
{
    //DECLARE_DYNAMIC(CMyDialog)
    // �Ի�������
    enum { IDD = IDC_DIALOG_SHOW };
    // Modeless construct
public:
    CMyDialog();

    int updateImg(ST_ImgInfo& info);

    // ���ɵ���Ϣӳ�亯��
    virtual BOOL OnInitDialog();
protected:
    afx_msg void OnPaint();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    ST_ImgInfo m_stImgInfo;
};



class dataprocess
{
public:
    dataprocess(void);
    ~dataprocess(void);

public:
    int addData(int x, int y, LPCTSTR str);
    int printData();
    int clearData();
    std::string TCharToString(CString strInfo);

    int addImgInfo(ST_ImgInfo& stImgInfo);
    int setImgInfo(ST_ImgInfo& stImgInfo);
    int clearImgInfo();

    int showItemDialog();
private:
    CString m_strData[MAX_ROW][MAX_COL];
    vector<CString> m_vecData[MAX_ROW];
    int m_row[MAX_ROW];
    int m_col[MAX_COL];
    int m_iCurrRow;
    bool m_bInit;


    CMyDialog m_dialog;
};

