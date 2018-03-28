// hookUIDlg.h : 头文件
//

#pragma once

#define  UM_WNDTITLE WM_USER+100        //自定义消息

#include "dataprocess.h"
#include <GdiPlusEnums.h>
#include "GdiPlus.h" 
#include "MyTestDialog.h"
#include <map>


using namespace std;  
using namespace Gdiplus;  
#pragma comment(lib, "GdiPlus.lib")


// ChookUIDlg 对话框
class ChookUIDlg : public CDialog
{
    // 构造
public:
    ChookUIDlg(CWnd* pParent = NULL);	// 标准构造函数

    // 对话框数据
    enum { IDD = IDD_HOOKUI_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


    // 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButtonHookSingle();
    afx_msg void OnBnClickedButtonHookAll();
    afx_msg void OnBnClickedButtonHookOff();
    afx_msg LRESULT OnUserMsg(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserMsgBitMap(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUserMsgDrawImg(WPARAM wParam, LPARAM lParam);
    afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
    afx_msg void OnBnClickedButtonTest();

private:
    // dll注入
    int InjectDllToRemoteProcess(const char* lpDllName, const char* lpPid, const char* lpProcName);
    //进程快照（枚举各进程）
    BOOL GetPidByProcessName(LPCTSTR lpszProcessName , DWORD &dwPid);
    bool HookSingel(char* strProcessName);
    bool HookAll();
    bool unHookAll();
    bool unHookSingle();
    BOOL setCallBack();
    // 共享内存管理
    int setShareMemory();
    int closeShareMemery();

    // 保存图片到文件
    int saveBitmapToFile(CString strFileName, CImage& img, RectF &srcRect, RectF &tagRect);
    // 查询已经存在的 rect
    int findExsitRect(RECT &rect);
    CWnd* getWnd(int id);
private:
    CString m_strExePath;
    bool m_bHookAll;
    bool m_bHooked;
    CEdit m_editName;
    CEdit m_editText;
    CString m_strDllName;
    DWORD m_dTargetPid;
    LPCTSTR m_pShareBuf;
    BYTE* m_BitBuffer ;
    unsigned char * m_dataDesc;
    HANDLE m_hMapFile;
    dataprocess m_cDataPrpcess;

    RECT m_injectRect;

    CBitmap m_cBitMap;
    HBITMAP m_hBmp;

    Gdiplus::Image *m_pImage;

    Gdiplus::Image *m_pImageD;

    CMyTestDialog *m_myTestDialog;
public:
    afx_msg void OnBnClickedImage();
    int m_iIndex;

    std::map<int, RECT> m_mapInfo;
    std::map<int, CWnd*> m_mapWnd;
};
