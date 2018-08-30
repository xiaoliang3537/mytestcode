// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"


//ȫ�ֹ������  
#pragma data_seg(".ShareW")  
HWND g_hWnd = NULL;                                     //�����ھ��;  
HWND g_hWndMsg = NULL;                                  //�ص���Ϣ���ھ��
HHOOK hhk = NULL;                                       //��깳�Ӿ��;  
HINSTANCE hInst = NULL;                                   //��dllʵ�����;  

HANDLE hProcess = NULL;                                 // ��ע����̾��
BOOL bIsInjected = FALSE;                               // �Ƿ�ע��ı�־
int iHookTypeWay = -1;                                  // hook ��ʽ 1 ע�� 2 ͨ��setWindowHook����

#pragma data_seg()  
#pragma comment(linker, "/section:.ShareW,rws")  



//��깳�ӹ��̣�Ŀ���Ǽ��ر�dll��ʹ�����ĳ���  
//��깳�ӵ����ã��������ĳ���򴰿���ʱ����ͻ�����������dll  
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
            // ������󻯣���С����ť
            if (pvarRole.lVal == 0x2B)
            {
                int v = -1;
                CString str;
                str.Format(_T("%s"), bstr);
                if (str.Compare(_T("��С��")) == 0)v = 1;
                else if (str.Compare(_T("���")) == 0)v = 2;
                else if (str.Compare(_T("�ر�")) == 0)v = 3;
                else v = -1;
                {
                    //::PostMessage(g_hWndMsg, WM_USER + 99, (WPARAM)pvarRole.lVal, (LPARAM)v);
                }
            }
        }
    }
    
    return CallNextHookEx(hhk, nCode, wParam, lParam);
}


//��װ����  ָ���߳� id = 0 ��Ϊȫ��
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


//ж�ع���  
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

