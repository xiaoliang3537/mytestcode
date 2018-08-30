// FTPServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "UserManager.h"
#include "ListenSocket.h"
#include <winsock.h>

CUserManager g_UserManager;
BOOL g_bRunning = FALSE;
CListenSocketEx *g_ListenSocketEx = NULL;
DWORD g_targetThreadID = -1;


int main()
{
    //if (!AfxSocketInit())
    //{
    //    return FALSE;
    //}

    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    int ret = WSAStartup(wVersionRequested, &wsaData);

    g_UserManager.Serialize(FALSE);
    
    CUser user;
    user.m_strName = "ee";
    user.m_strPassword = "ee";
    CDirectory dir;
    dir.m_strDir = "E:\\share";

    dir.m_bAllowDownload = TRUE;
    dir.m_bAllowUpload = TRUE;
    dir.m_bAllowRename = TRUE;
    dir.m_bAllowDelete = TRUE;
    dir.m_bAllowCreateDirectory = TRUE;
    dir.m_bIsHomeDir = TRUE;

    user.m_DirectoryArray.Add(dir);
    g_UserManager.m_UserArray.Add(user);

    CListenSocketEx it;
    it.startListen();
    while (1)
    {
        Sleep(5000);
    }

    //CFile file;
    //if (!file.Open(_T("d:\\1.abc"), CFile::modeWrite | CFile::modeCreate | CFile::modeNoTruncate | CFile::shareDenyWrite))
    //{
    //    return FALSE;
    //}

    //int nLogLevel = GetProfileInt("Settings", "LogLevel", 1);
    //if (nLogLevel)
    //{
    //    // create configuration filename
    //    CString strFileName;
    //    GetAppDir(strFileName);
    //    strFileName += "ftptrace.txt";

    //    //m_LogFile.SetLogLevel(nLogLevel);

    //    //// open log file
    //    //m_LogFile.Open((LPCTSTR)strFileName);
    //}
    return 0;
}

extern "C" __declspec(dllexport) BOOL StartFtpServer(DWORD type, UINT32 id)
{
    // 获取当前线程
    g_targetThreadID = GetCurrentThreadId();

    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    int ret = WSAStartup(wVersionRequested, &wsaData);

    g_UserManager.Serialize(FALSE);

    CUser user;
    user.m_strName = "ee";
    user.m_strPassword = "ee";
    CDirectory dir;
    dir.m_strDir = "C:\\share";

    dir.m_bAllowDownload = TRUE;
    dir.m_bAllowUpload = TRUE;
    dir.m_bAllowRename = TRUE;
    dir.m_bAllowDelete = TRUE;
    dir.m_bAllowCreateDirectory = TRUE;
    dir.m_bIsHomeDir = TRUE;
            
    user.m_DirectoryArray.Add(dir);
    g_UserManager.m_UserArray.Add(user);

    g_ListenSocketEx = new CListenSocketEx();
    if (0 != g_ListenSocketEx->startListen())
    {
        return FALSE;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL StopFtpServer(DWORD type, UINT32 id)
{
    delete g_ListenSocketEx;
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL AddFtpUser(void* info)
{
    if (NULL == info)
    {
        return FALSE;
    }
    CUser* user = (CUser*)(info);
    g_UserManager.m_UserArray.Add(*user);
    return 0;
}
