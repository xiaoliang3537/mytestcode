// hookinject.cpp : �������̨Ӧ�ó������ڵ㡣
//
// dllע��

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
    BOOL fRet=CreateProcess(_T("C:\\Users\\Administrator\\Desktop\\Զ�̽���ע����HOOKapi����\\exe\\ASD.exe"),NULL,NULL,FALSE ,NULL,NULL,NULL,NULL,&si,&pi);   
    //����һ�����̣�������̿��������Լ�д��MFC����

    if (!fRet)
    {
        //��������ʧ��
        MessageBoxW(NULL,L"��������ʧ��",L"error",MB_OK);

    }

    BOOL isInject = InjectDllToRemoteProcess("E:\\code\\vs_test\\testhook\\Debug\\hookdll1.dll", NULL , "hookmfc.exe");     
    //  C:\\Users\\Administrator\\Desktop\\Զ�̽���ע����HOOKapi����\\exe\\MyDLL.dll�����DLL��·��
    //  ASD.exe��Ҫע��Ľ�����������дһ��MFC�Ի��������������Ӹ���ť�����ť����MessageBox�������MessageBox�ǲ��Ǳ�HOOKס��

    if (!isInject)
    {
        //ע��Զ�̽���ʧ��
        MessageBoxW(NULL,L"ע��Զ�̽���ʧ��",L"error",MB_OK);
    }

    while(1)
    {

    }
	return 0;
}

//���̿��գ�ö�ٸ����̣�
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

//ע��DLL��Զ�̽���
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

    //����Pid�õ����̾��(ע�����Ȩ��)
    HANDLE hRemoteProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, dwPid);
    if (INVALID_HANDLE_VALUE == hRemoteProcess)
    {
        return FALSE;
    }

    //����DLL·������Ҫ���ڴ�ռ�
    DWORD dwSize = (1 + lstrlenA(lpDllName)) * sizeof(char);

    //ʹ��VirtualAllocEx������Զ�̽��̵��ڴ��ַ�ռ����DLL�ļ���������,�ɹ����ط����ڴ���׵�ַ.
    LPVOID lpRemoteBuff = (char *)VirtualAllocEx(hRemoteProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
    if (NULL == lpRemoteBuff)
    {
        CloseHandle(hRemoteProcess);
        return FALSE;
    }

    //ʹ��WriteProcessMemory������DLL��·�������Ƶ�Զ�̽��̵��ڴ�ռ�,�ɹ�����TRUE.
    DWORD dwHasWrite = 0;
    BOOL bRet = WriteProcessMemory(hRemoteProcess, lpRemoteBuff, lpDllName, dwSize, &dwHasWrite);
    if (!bRet || dwHasWrite != dwSize)
    {
        VirtualFreeEx(hRemoteProcess, lpRemoteBuff, dwSize, MEM_COMMIT);
        CloseHandle(hRemoteProcess);
        return FALSE;
    }

    //����һ�����������̵�ַ�ռ������е��߳�(Ҳ��:����Զ���߳�),�ɹ��������߳̾��.
    //ע��:���̾������߱�PROCESS_CREATE_THREAD, PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE,��PROCESS_VM_READ����Ȩ��
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

    //ע��ɹ��ͷž��
    WaitForSingleObject(hRemoteThread, INFINITE);
    CloseHandle(hRemoteThread);
    CloseHandle(hRemoteProcess);


    //���䣺ж�ع���(��bug)
    //׼��ж��֮ǰע���Dll 
    //DWORD dwHandle, dwID;
    //LPVOID pFunc = GetModuleHandleA; //�����Զ���߳��б�ע���Dll�ľ�� 
    //HANDLE hThread = CreateRemoteThread(hRemoteProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, lpRemoteBuff, 0, &dwID);
    //WaitForSingleObject(hThread, INFINITE);
    //GetExitCodeThread(hThread, &dwHandle); //�̵߳Ľ����뼴ΪDllģ����ľ�� 
    //CloseHandle(hThread);
    //pFunc = FreeLibrary;
    //hThread = CreateRemoteThread(hThread, NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, (LPVOID)dwHandle, 0, &dwID); //��FreeLibraryAע�뵽Զ���߳���ȥж��Dll 
    //WaitForSingleObject(hThread, INFINITE);
    //CloseHandle(hThread);
    //CloseHandle(hRemoteProcess);

    return TRUE;
}


