#include "stdafx.h"
#include "service.h"
#include <Windows.h>
#include <Winsvc.h>
#include "stdio.h"
#include "tchar.h"
#include "service/server.h"
#include "loger.h"
#include "function.h"
//定义全局函数变量


TCHAR szServiceName[] = _T("ServiceIBox");
BOOL bInstall;
SERVICE_STATUS_HANDLE hServiceStatus;
SERVICE_STATUS status;
DWORD dwThreadID;
HANDLE g_hEventExit;
HANDLE g_hEventThreadExit;
HANDLE g_hThread;
server *g_server;

//*********************************************************
//Description:            启动服务接口
//*********************************************************
void runService()
{
    service::Init();
    dwThreadID = ::GetCurrentThreadId();
    SERVICE_TABLE_ENTRY st[] =
    {
        { szServiceName, (LPSERVICE_MAIN_FUNCTION)service::ServiceMain },
        { NULL, NULL }
    };
    {
        if (!::StartServiceCtrlDispatcher(st))
        {
            service::LogEvent(_T("Register Service Main Function Error!"));
        }
    }
}

//*********************************************************
//Functiopn:            Init
//Description:            初始化
//Calls:                main
//*********************************************************
void service::Init()
{
    hServiceStatus = NULL;
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = SERVICE_STOPPED;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    status.dwWin32ExitCode = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;
}

//*********************************************************
//Functiopn:            ServiceMain
//Description:            服务主函数，这在里进行控制对服务控制的注册
//*********************************************************
void WINAPI service::ServiceMain()
{
    status.dwCurrentState = SERVICE_START_PENDING;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    //注册服务控制
    hServiceStatus = RegisterServiceCtrlHandler(szServiceName, ServiceStrl);
    if (hServiceStatus == NULL)
    {
        LogEvent(_T("Handler not installed"));
        return;
    }
    SetServiceStatus(hServiceStatus, &status);

    //status.dwWin32ExitCode = S_OK;
    //status.dwCheckPoint = 0;
    //status.dwWaitHint = 0;
    //status.dwCurrentState = SERVICE_RUNNING;
    //SetServiceStatus(hServiceStatus, &status);
    reportStatus(SERVICE_RUNNING, NO_ERROR, 0);

    //std::wstring str;
    //TriggerAppExecute(str);

    g_server = new server();
    g_server->resume();
    g_server->wait();
    
    //status.dwCurrentState = SERVICE_STOPPED;
    //SetServiceStatus(hServiceStatus, &status);
    reportStatus(SERVICE_STOPPED, NO_ERROR, 0);
    LogEvent(_T("Service stopped"));

    delete g_server;
}

//*********************************************************
//Functiopn:            ServiceStrl
//Description:            服务控制主函数，这里实现对服务的控制，
//                        当在服务管理器上停止或其它操作时，将会运行此处代码
//*********************************************************
void WINAPI service::ServiceStrl(DWORD dwOpcode)
{
    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        g_server->stop();
        //status.dwCurrentState = SERVICE_STOP_PENDING;
        //SetServiceStatus(hServiceStatus, &status);
        reportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        PostThreadMessage(dwThreadID, WM_CLOSE, 0, 0);
        break;
    case SERVICE_CONTROL_PAUSE:
        break;
    case SERVICE_CONTROL_CONTINUE:
        //status.dwCurrentState = SERVICE_RUNNING;
        //SetServiceStatus(hServiceStatus, &status);
        reportStatus(SERVICE_RUNNING, NO_ERROR, 0);
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        break;
    default:
        LogEvent(_T("Bad service request"));
    }

    return;
}
//*********************************************************
//Functiopn:            IsInstalled
//Description:            判断服务是否已经被安装
//*********************************************************
BOOL service::IsInstalled()
{
    BOOL bResult = FALSE;

    //打开服务控制管理器
    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM != NULL)
    {
        //打开服务
        SC_HANDLE hService = ::OpenService(hSCM, szServiceName, SERVICE_QUERY_CONFIG);
        if (hService != NULL)
        {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hSCM);
    }
    return bResult;
}

//*********************************************************
//Functiopn:            Install
//Description:            安装服务函数
//*********************************************************
BOOL service::Install()
{
    if (IsInstalled())
        return TRUE;

    //打开服务控制管理器
    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL)
    {
        MessageBox(NULL, _T("Couldn't open service manager"), szServiceName, MB_OK);
        return FALSE;
    }

    // Get the executable file path
    TCHAR szFilePath[MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, MAX_PATH);

    //创建服务
    SC_HANDLE hService = ::CreateService(
        hSCM, szServiceName, szServiceName,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
        szFilePath, NULL, NULL, _T(""), NULL, NULL);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        MessageBox(NULL, _T("Couldn't create service"), szServiceName, MB_OK);
        return FALSE;
    }

    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);
    return TRUE;
}

//*********************************************************
//Functiopn:            Uninstall
//Description:            删除服务函数
//*********************************************************
BOOL service::Uninstall()
{
    if (!IsInstalled())
        return TRUE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM == NULL)
    {
        MessageBox(NULL, _T("Couldn't open service manager"), szServiceName, MB_OK);
        return FALSE;
    }

    SC_HANDLE hService = ::OpenService(hSCM, szServiceName, SERVICE_STOP | DELETE);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        MessageBox(NULL, _T("Couldn't open service"), szServiceName, MB_OK);
        return FALSE;
    }
    SERVICE_STATUS status;
    ::ControlService(hService, SERVICE_CONTROL_STOP, &status);

    //删除服务
    BOOL bDelete = ::DeleteService(hService);
    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

    if (bDelete)
        return TRUE;

    LogEvent(_T("Service could not be deleted"));
    return FALSE;
}

//*********************************************************
//Functiopn:            LogEvent
//Description:            记录服务事件
//*********************************************************
void service::LogEvent(LPCTSTR pFormat, ...)
{
    TCHAR    chMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start(pArg, pFormat);
    _vstprintf(chMsg, pFormat, pArg);
    va_end(pArg);

    lpszStrings[0] = chMsg;

    hEventSource = RegisterEventSource(NULL, szServiceName);
    if (hEventSource != NULL)
    {
        ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, (LPCTSTR*)&lpszStrings[0], NULL);
        DeregisterEventSource(hEventSource);
    }
}

bool service::reportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    if (dwCurrentState == SERVICE_START_PENDING) {
        status.dwControlsAccepted = 0;
    }
    else {
        status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    status.dwCurrentState = dwCurrentState;
    status.dwWin32ExitCode = dwWin32ExitCode;
    status.dwWaitHint = dwWaitHint;

    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) {
        status.dwCheckPoint = 0;
    }
    else {
        status.dwCheckPoint++;
    }

    return SetServiceStatus(hServiceStatus, &status) == TRUE;
}
