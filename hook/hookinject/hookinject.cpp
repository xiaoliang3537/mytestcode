// hookinject.cpp : 定义控制台应用程序的入口点。
//
// dll注入

#include "stdafx.h"
#include <windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <time.h>

BOOL InjectDllToRemoteProcess(const char* lpDllName, const char* lpPid, const char* lpProcName);

int _tmain(int argc, _TCHAR* argv[])
{
    PROCESS_INFORMATION pi;  
    STARTUPINFO si;  
    memset(&si,0,sizeof(si));  
    si.cb=sizeof(si);  
    si.wShowWindow=SW_SHOW;  
    si.dwFlags=STARTF_USESHOWWINDOW;  
    BOOL fRet=CreateProcess(_T("C:\\Users\\Administrator\\Desktop\\远程进程注入与HOOKapi例子\\exe\\ASD.exe"),NULL,NULL,FALSE ,NULL,NULL,NULL,NULL,&si,&pi);   
    //创建一个进程，这个进程可以是你自己写的MFC程序。

    if (!fRet)
    {
        //创建进程失败
        MessageBoxW(NULL,L"创建进程失败",L"error",MB_OK);

    }

    BOOL isInject = InjectDllToRemoteProcess("E:\\code\\vs_test\\testhook\\Debug\\hookdll1.dll", NULL , "hookmfc.exe");     
    //  C:\\Users\\Administrator\\Desktop\\远程进程注入与HOOKapi例子\\exe\\MyDLL.dll这个的DLL的路径
    //  ASD.exe是要注入的进程名，可以写一个MFC对话框程序在上面添加个按钮点击按钮弹出MessageBox看看你的MessageBox是不是被HOOK住了

    if (!isInject)
    {
        //注入远程进程失败
        MessageBoxW(NULL,L"注入远程进程失败",L"error",MB_OK);
    }

    while(1)
    {

    }
	return 0;
}

//进程快照（枚举各进程）
BOOL GetPidByProcessName(LPCTSTR lpszProcessName , DWORD &dwPid)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if ( INVALID_HANDLE_VALUE == hSnapshot )
    {
        return FALSE;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if ( !Process32First(hSnapshot, &pe) )
    {
        ::CloseHandle(hSnapshot);
        return FALSE;
    }

    while ( Process32Next(hSnapshot, &pe) )
    {
        if ( !lstrcmp(lpszProcessName, pe.szExeFile) )
        {
            ::CloseHandle(hSnapshot);
            dwPid = pe.th32ProcessID;
            return TRUE;
        }
    }

    ::CloseHandle(hSnapshot);
    return FALSE;
}

/********************************************************************************************************/

//注入DLL到远程进程
BOOL InjectDllToRemoteProcess(const char* lpDllName, const char* lpPid, const char* lpProcName)
{
    DWORD dwPid = 0;
    if (NULL == lpPid || 0 == strlen(lpPid))
    {
        if (NULL != lpProcName && 0 != strlen(lpProcName))
        {
            if (!GetPidByProcessName(L"hookmfc.exe", dwPid))
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        dwPid = atoi(lpPid);
    }

    //根据Pid得到进程句柄(注意必须权限)
    HANDLE hRemoteProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, dwPid);
    if (INVALID_HANDLE_VALUE == hRemoteProcess)
    {
        return FALSE;
    }

    //计算DLL路径名需要的内存空间
    DWORD dwSize = (1 + lstrlenA(lpDllName)) * sizeof(char);

    //使用VirtualAllocEx函数在远程进程的内存地址空间分配DLL文件名缓冲区,成功返回分配内存的首地址.
    LPVOID lpRemoteBuff = (char *)VirtualAllocEx(hRemoteProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
    if (NULL == lpRemoteBuff)
    {
        CloseHandle(hRemoteProcess);
        return FALSE;
    }

    //使用WriteProcessMemory函数将DLL的路径名复制到远程进程的内存空间,成功返回TRUE.
    DWORD dwHasWrite = 0;
    BOOL bRet = WriteProcessMemory(hRemoteProcess, lpRemoteBuff, lpDllName, dwSize, &dwHasWrite);
    if (!bRet || dwHasWrite != dwSize)
    {
        VirtualFreeEx(hRemoteProcess, lpRemoteBuff, dwSize, MEM_COMMIT);
        CloseHandle(hRemoteProcess);
        return FALSE;
    }

    //创建一个在其它进程地址空间中运行的线程(也称:创建远程线程),成功返回新线程句柄.
    //注意:进程句柄必须具备PROCESS_CREATE_THREAD, PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE,和PROCESS_VM_READ访问权限
    DWORD  dwRemoteThread = 0;
    //LPTHREAD_START_ROUTINE pfnLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("Kernel32"), "LoadLibraryA");
    //HANDLE hRemoteThread = CreateRemoteThread(hRemoteProcess, NULL, 0, pfnLoadLibrary, lpRemoteBuff, 0, &dwRemoteThread);
    HANDLE hRemoteThread = CreateRemoteThread(hRemoteProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, lpRemoteBuff, 0, &dwRemoteThread);
    if (INVALID_HANDLE_VALUE == hRemoteThread)
    {
        VirtualFreeEx(hRemoteProcess, lpRemoteBuff, dwSize, MEM_COMMIT);
        CloseHandle(hRemoteProcess);
        return FALSE;
    }

    //注入成功释放句柄
    WaitForSingleObject(hRemoteThread, INFINITE);
    CloseHandle(hRemoteThread);
    CloseHandle(hRemoteProcess);


    //补充：卸载过程(有bug)
    //准备卸载之前注入的Dll 
    //DWORD dwHandle, dwID;
    //LPVOID pFunc = GetModuleHandleA; //获得在远程线程中被注入的Dll的句柄 
    //HANDLE hThread = CreateRemoteThread(hRemoteProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, lpRemoteBuff, 0, &dwID);
    //WaitForSingleObject(hThread, INFINITE);
    //GetExitCodeThread(hThread, &dwHandle); //线程的结束码即为Dll模块儿的句柄 
    //CloseHandle(hThread);
    //pFunc = FreeLibrary;
    //hThread = CreateRemoteThread(hThread, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, (LPVOID)dwHandle, 0, &dwID); //将FreeLibraryA注入到远程线程中去卸载Dll 
    //WaitForSingleObject(hThread, INFINITE);
    //CloseHandle(hThread);
    //CloseHandle(hRemoteProcess);

    return TRUE;
}


