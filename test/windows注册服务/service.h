#pragma once

#include <afxinet.h>
#include <afxhtml.h>
#include <OleAcc.h>
#include <comdef.h>
#include <iostream>
#include <fstream>
#include <vector>

class service
{
public:
    service();
    ~service();
public:
    static void Init();
    static BOOL IsInstalled();
    static BOOL Install();
    static BOOL Uninstall();
    static void LogEvent(LPCTSTR pszFormat, ...);
    static void WINAPI ServiceMain();
    static void WINAPI ServiceStrl(DWORD dwOpcode);
    static bool reportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
};

// 系统服务

