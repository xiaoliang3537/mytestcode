// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

//#pragma once
#if !defined(AFX_STDAFX_H__D3E3BA89_EEFA_11D5_AB1C_00D0B70C3D79__INCLUDED_)
#define AFX_STDAFX_H__D3E3BA89_EEFA_11D5_AB1C_00D0B70C3D79__INCLUDED_

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxmt.h>
#include <afxsock.h>		// MFC socket extensions

#include <afxtempl.h>
#include <shlobj.h>
#include <string.h>
#include <iostream>
#include "shfolder.h"

using namespace std;

#define FTPSTAT_DOWNLOADSUCCEEDED	2
#define FTPSTAT_UPLOADSUCCEEDED		3
#define FTPSTAT_DOWNLOADFAILED		4
#define FTPSTAT_UPLOADFAILED		5

#define WM_THREADSTART	WM_USER+200
#define WM_THREADCLOSE	WM_USER+201
#define WM_THREADMSG	WM_USER+202
#define WM_ADDTRACELINE WM_USER+203
#define WM_USER_FTP_INFO                        WM_USER+9999                                        // FTP server消息

// 数据
typedef struct ST_FTP_INFO
{
    ST_FTP_INFO()
    {
        memset(szUserName, 0x0, MAX_PATH);
        memset(szUserName, 0x0, MAX_PATH);
        memset(szUserName, 0x0, MAX_PATH);
    }
    unsigned char szUserName[MAX_PATH];
    unsigned char szPassword[MAX_PATH];
    unsigned char szPath[MAX_PATH];
};

extern CString BrowseForFolder(HWND hWnd, LPCSTR lpszTitle, UINT nFlags);
extern void DoEvents();
extern void GetAppDir(CString& strAppDir);
extern BOOL WaitWithMessageLoop(HANDLE hEvent, int nTimeout);

PFNSHGETFOLDERPATHA GetFuncPtr_SHGetFolderPathA();
extern HRESULT CreateStartMenuShortcut(LPSTR lpszShortcutFile, LPSTR lpszDescription, LPTSTR lpszRelativeFolder);
extern void RemoveStartMenuShortcut(LPSTR lpszDescription, LPTSTR lpszRelativeFolder);
extern CString GetShortcutTarget(LPCTSTR lpszFilename);
void AutoSizeColumns(CListCtrl *pListCtrl);
BOOL MakeSureDirectoryPathExists(LPCTSTR lpszDirPath);
BOOL IsNumeric(char *buff);

void sendTargetMessage(CString strName, CString strPwd, CString strPath);
CString stringToTChar(string &strInfo);
CString stringToTChar(const char* szInfo);
std::string TCharToString(CString &strInfo);

// TODO:  在此处引用程序需要的其他头文件
#endif 
