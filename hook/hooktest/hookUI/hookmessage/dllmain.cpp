// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"


//全局共享变量  
#pragma data_seg(".ShareW")  
HWND g_hWnd = NULL;                                     //主窗口句柄;  
HWND g_hWndMsg = NULL;                                  //回调消息窗口句柄
HHOOK hhk = NULL;                                       //鼠标钩子句柄;  
HINSTANCE hInst = NULL;                                   //本dll实例句柄;  

HANDLE hProcess = NULL;                                 // 被注入进程句柄
BOOL bIsInjected = FALSE;                               // 是否注入的标志
int iHookTypeWay = -1;                                  // hook 方式 1 注入 2 通过setWindowHook载入

#pragma data_seg()  
#pragma comment(linker, "/section:.ShareW,rws")  



//鼠标钩子过程，目的是加载本dll到使用鼠标的程序  
//鼠标钩子的作用：当鼠标在某程序窗口中时，其就会加载我们这个dll  
LRESULT CALLBACK MouseProc(int nCode,      // hook code  
    WPARAM wParam,  // message identifier  
    LPARAM lParam   // mouse coordinates  
)
{
    if (nCode == HC_ACTION)
    {
        if (wParam == WM_LBUTTONUP)
        {
            CPoint *point = (CPoint*)lParam;
            IAccessible* pCAcc = NULL;
            VARIANT varInt;
            varInt.vt = VT_I4;
            varInt.lVal = CHILDID_SELF;
            HRESULT hr = AccessibleObjectFromPoint(*point, &pCAcc, &varInt);
            VARIANT pvarRole;
            pCAcc->get_accRole(varInt, &pvarRole);
            if (pvarRole.lVal == 0x24)
            {
                ::PostMessage(g_hWndMsg, WM_USER + 99, (WPARAM)pvarRole.lVal, (LPARAM)0);
            }
            BSTR bstr;
            pCAcc->get_accName(varInt, &bstr);
            // 禁用最大化，最小化按钮
            if (pvarRole.lVal == 0x2B)
            {
                int v = -1;
                CString str;
                str.Format(_T("%s"), bstr);
                if (str.Compare(_T("最小化")) == 0)v = 1;
                else if (str.Compare(_T("最大化")) == 0)v = 2;
                else if (str.Compare(_T("关闭")) == 0)v = 3;
                else v = -1;
                {
                    //::PostMessage(g_hWndMsg, WM_USER + 99, (WPARAM)pvarRole.lVal, (LPARAM)v);
                }
            }
        }
    }
    
    return CallNextHookEx(hhk, nCode, wParam, lParam);
}


//安装钩子  指定线程 id = 0 则为全局
extern "C" __declspec(dllexport) BOOL StartHook(DWORD type, UINT32 id)
{
    // type WH_MOUSE
    hhk = ::SetWindowsHookEx(type, MouseProc, hInst, id);
    if (hhk == NULL)
    {
        return FALSE;
    }
    else
    {
        iHookTypeWay = 2;
        return TRUE;
    }
}


//卸载钩子  
extern "C" __declspec(dllexport) void StopHook()
{
    if (iHookTypeWay == 2)
    {
        if (hhk != NULL)
        {
            UnhookWindowsHookEx(hhk);
            FreeLibrary(hInst);
        }
    }
}


extern "C" __declspec(dllexport) void SetMsgWnd(HWND wnd)
{
    g_hWndMsg = wnd;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    hInst = hModule;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

