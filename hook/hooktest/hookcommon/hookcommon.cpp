// hookcommon.cpp : ���� DLL �ĳ�ʼ�����̡�
//

#include "stdafx.h"
#include "hookcommon.h"
#include <Usp10.h>
#include <Gdiplus.h>
#include <GdiPlusFlat.h>
#include <GdiPlusGpStubs.h>
#include <GdiPlusGraphics.h>
#include "detours.h"  
#include <comdef.h>
#include <gdiplus.h>
#include <afxmt.h>
#include <stdio.h>
#include <atlimage.h>
#include <assert.h>

//#include <NTSTATUS.h>
using namespace Gdiplus;
#pragma  comment(lib, "gdiplus.lib")

//#define I_DETOURS

using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  UM_WNDTITLE WM_USER+100 //�Զ�����Ϣ  
#define  MAX_LEN 512

TCHAR szName[]=TEXT("Global\\MyFileMappingObject");    //ָ��ͬһ�鹲���ڴ������  
#define BUF_SIZE 512  

TCHAR szNameBitMap[]=TEXT("Global\\MyFileMappingObjectBitMap");    //ָ��ͬһ�鹲���ڴ������  
TCHAR szNameDataDesc[]=TEXT("Global\\MyFileMappingDataDesc");    //��������  ����������Ϣ �豸��Ϣ����  



//ȫ�ֹ������  
#pragma data_seg(".ShareW")  
HWND g_hWnd = NULL;                                     //�����ھ��;  
HWND g_hWndMsg = NULL;                                  //�ص���Ϣ���ھ��
HHOOK hhk = NULL;                                       //��깳�Ӿ��;  
HINSTANCE hInst=NULL;                                   //��dllʵ�����;  
TCHAR g_strData[MAX_LEN] = {0};
int g_value = 0;
CMutex g_clsMutex(FALSE, NULL);
#pragma data_seg()  
#pragma comment(linker, "/section:.ShareW,rws")  

HANDLE hProcess = NULL;  
BOOL bIsInjected = FALSE;  
LPCTSTR g_pShareBuf = NULL;
HANDLE g_hMapMem = NULL;
int g_len = 0;
int g_count = 0;
unsigned int g_data[20] = {0};

//GdiplusStartupInput g_gdiplusStartupInput;

ULONG_PTR g_gdiplusToken;
// ����ȫ��λͼ����
BYTE* g_BitBuffer = NULL;
// ����ȫ��λͼ������Ϣ
char* g_strDesc = NULL;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WINAPI MyMessageBoxA(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType);  
int WINAPI MyMessageBoxW(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType);  
int WINAPI MyTextOutW(__in HDC hdc, __in int x, __in int y, __in_ecount(c) LPCWSTR lpString, __in int c);
BOOL WINAPI MyExtTextOutW(_In_ HDC hdc,_In_ int X,_In_ int Y,_In_ UINT fuOptions,_In_ const RECT *lprc,
                          _In_ LPCTSTR lpString,_In_ UINT cbCount,_In_ const INT *lpDx);
//__gdi_entry BOOL  WINAPI MyExtTextOutW( __in HDC hdc, __in int x, __in int y, __in UINT options, __in_opt CONST RECT * lprect, 
//                                     __in_ecount_opt(c) LPCWSTR lpString, __in UINT c, __in_ecount_opt(c) CONST INT * lpDx);

DWORD WINAPI MyGetCharacterPlacementW(
                                      _In_    HDC           hdc,
                                      _In_    LPCTSTR       lpString,
                                      _In_    int           nCount,
                                      _In_    int           nMaxExtent,
                                      _Inout_ LPGCP_RESULTS lpResults,
                                      _In_    DWORD         dwFlags
                                      );

HRESULT MyScriptStringAnalyse(
                              _In_           HDC                    hdc,
                              _In_     const void                   *pString,
                              _In_           int                    cString,
                              _In_           int                    cGlyphs,
                              _In_           int                    iCharset,
                              _In_           DWORD                  dwFlags,
                              _In_           int                    iReqWidth,
                              _In_opt_       SCRIPT_CONTROL         *psControl,
                              _In_opt_       SCRIPT_STATE           *psState,
                              _In_opt_ const int                    *piDx,
                              _In_opt_       SCRIPT_TABDEF          *pTabdef,
                              _In_     const BYTE                   *pbInClass,
                              _Out_          SCRIPT_STRING_ANALYSIS *pssa
                              );

HRESULT MyScriptIsComplex(
                          _In_ const WCHAR *pwcInChars,
                          _In_       int   cInChars,
                          _In_       DWORD dwFlags
                          );

int MyDrawTextW(
          __in HDC hdc,
          __inout_ecount_opt(cchText) LPCWSTR lpchText,
          __in int cchText,
          __inout LPRECT lprc,
          __in UINT format);

int WINAPI  MyDrawTextExW(
            __in HDC hdc,
            __inout_ecount_opt(cchText) LPWSTR lpchText,
            __in int cchText,
            __inout LPRECT lprc,
            __in UINT format,
            __in_opt LPDRAWTEXTPARAMS lpdtp);

__gdi_entry BOOL  WINAPI    MyBitBlt( __in HDC hdc, __in int x, __in int y,
                      __in int cx, __in int cy, __in_opt HDC hdcSrc, 
                      __in int x1, __in int y1, __in DWORD rop);

__gdi_entry BOOL  WINAPI  MyStretchBlt(__in HDC hdcDest, __in int xDest, __in int yDest, 
                   __in int wDest, __in int hDest, __in_opt HDC hdcSrc, 
                   __in int xSrc, __in int ySrc, __in int wSrc, __in int hSrc, 
                   __in DWORD rop);

HBITMAP MyCreateBitmap( __in int nWidth, __in int nHeight, __in UINT nPlanes, __in UINT nBitCount, __in_opt CONST VOID *lpBits);

HBITMAP WINAPI MyCreateCompatibleBitmap( __in HDC hdc, __in int cx, __in int cy);


GpStatus WINGDIPAPI
MyGdipDrawImage(GpGraphics *graphics, GpImage *image, REAL x, REAL y);

GpStatus WINGDIPAPI
MyGdipDrawImageI(GpGraphics *graphics, GpImage *image, INT x, INT y);

GpStatus WINGDIPAPI
MyGdipDrawImageRectI(GpGraphics *graphics, GpImage *image, INT x, INT y,
                   INT width, INT height);

GpStatus WINGDIPAPI
MyGdipDrawImagePointRect(GpGraphics *graphics, GpImage *image, REAL x,
                       REAL y, REAL srcx, REAL srcy, REAL srcwidth,
                       REAL srcheight, GpUnit srcUnit);

GpStatus WINGDIPAPI
MyGdipDrawImageRectRect(GpGraphics *graphics, GpImage *image, REAL dstx,
                      REAL dsty, REAL dstwidth, REAL dstheight,
                      REAL srcx, REAL srcy, REAL srcwidth, REAL srcheight,
                      GpUnit srcUnit,
                      GDIPCONST GpImageAttributes* imageAttributes,
                      DrawImageAbort callback, VOID * callbackData);

BOOL MyImageList_DrawIndirect(IMAGELISTDRAWPARAMS* pimldp);

GpStatus WINGDIPAPI
MyGdipDrawString(
               GpGraphics               *graphics,
               GDIPCONST WCHAR          *string,
               INT                       length,
               GDIPCONST GpFont         *font,
               GDIPCONST RectF          *layoutRect,
               GDIPCONST GpStringFormat *stringFormat,
               GDIPCONST GpBrush        *brush
               );

PVOID g_pOldMessageBoxW=NULL;  

BOOL APIENTRY SetHook()  
{  
    //����Ѿ���װ��return  
    if(bIsInjected)return TRUE;  
    //���������̨  
 
    DetourTransactionBegin();  
    DetourUpdateThread(GetCurrentThread());  
    g_pOldMessageBoxW=DetourFindFunction("GDI32.dll","BitBlt");  
    DetourAttach(&g_pOldMessageBoxW,MyBitBlt);  
    LONG ret=DetourTransactionCommit();  
    bIsInjected=TRUE;  
    return bIsInjected;  

}  


BOOL APIENTRY DropHook()  
{  
    //����Ѿ�ж�ؾ�return  
    if(!bIsInjected)return TRUE;  
    //�������̨  

    DetourTransactionBegin();  
    DetourUpdateThread(GetCurrentThread());  
    DetourDetach(&g_pOldMessageBoxW, MyBitBlt);  
    LONG ret=DetourTransactionCommit();  
    bIsInjected=FALSE;  
    return ret==NO_ERROR;  
}  

typedef struct ST_ASM_INFO
{
    ST_ASM_INFO()
    {
        oldFunAddr = NULL;
        oldFunAddr = NULL;
        NewCode[0] = 0xe9;
    };
    unsigned int newFunAddr;
    FARPROC oldFunAddr; 

    BYTE OldCode[5]; //�ϵ�ϵͳAPI��ڴ���  
    BYTE NewCode[5]; //Ҫ��ת��API���� (jmp xxxx)
};

typedef struct ST_HOOK_INFO
{
    ST_HOOK_INFO()
    {
        memset(szDllName, 0x0, 128);
        memset(szFuncName, 0x0, 128);
        hmod = 0;
    };
    ST_ASM_INFO stAsmInfo;
    char szDllName[128];
    char szFuncName[128];
    HMODULE hmod;
};


HMODULE getLibModule(char* szDllName)
{
    for (int i = 0; i < g_count; i++)
    {
        ST_HOOK_INFO* stHookInfo = (ST_HOOK_INFO*)g_data[i];
        if (NULL == stHookInfo)
        {
            continue;
        }
        if (strcmp(stHookInfo->szDllName, szDllName) == 0)
        {
            return stHookInfo->hmod;
        }
    }
    return 0;
}

int HookOn(ST_ASM_INFO *stAsmInfo)
{
    ASSERT(hProcess!=NULL);  

    DWORD dwTemp=0,dwOldProtect,dwRet=0,dwWrite;

    VirtualProtectEx(hProcess,stAsmInfo->oldFunAddr,5,PAGE_READWRITE,&dwOldProtect);   
    dwRet=WriteProcessMemory(hProcess,stAsmInfo->oldFunAddr,stAsmInfo->NewCode,5,&dwWrite);  
    if (0==dwRet||0==dwWrite)  
    {  
        return -1;  
    }  
    VirtualProtectEx(hProcess,stAsmInfo->oldFunAddr,5,dwOldProtect,&dwTemp);  
    return 0;
}

int HookOff(ST_ASM_INFO *stAsmInfo)
{
    ASSERT(hProcess!=NULL);  

    DWORD dwTemp=0,dwOldProtect=0,dwRet=0,dwWrite=0;

    VirtualProtectEx(hProcess,stAsmInfo->oldFunAddr,5,PAGE_READWRITE,&dwOldProtect);
    dwRet=WriteProcessMemory(hProcess,stAsmInfo->oldFunAddr,stAsmInfo->OldCode,5,&dwWrite);
    if (0==dwRet||0==dwWrite)  
    {  
        return -1;
    }  
    VirtualProtectEx(hProcess,stAsmInfo->oldFunAddr,5,dwOldProtect,&dwTemp);   
    return 0;
}

int inject( ST_HOOK_INFO &stHookInfo)
{
    int iRet = 0;
    ST_ASM_INFO stAsmInfo;
    memcpy((void*)&stAsmInfo, (void*)&(stHookInfo.stAsmInfo), sizeof(ST_ASM_INFO));
    //��ȡ����
    CString strDllName = CA2CT(stHookInfo.szDllName);
    HMODULE hmod = getLibModule(stHookInfo.szDllName);
    if ( NULL == hmod )
    {
        hmod=::LoadLibrary(strDllName);
    }
    stAsmInfo.oldFunAddr = ::GetProcAddress(hmod,stHookInfo.szFuncName );;

    if (stAsmInfo.oldFunAddr == NULL)  
    {  
        CString strErr = _T("cannot get ");
        strErr += CA2CT(stHookInfo.szFuncName);
        MessageBox(NULL,strErr,_T("error"),0);  
        return iRet;  
    }  
    stHookInfo.hmod = hmod;

    // ��ԭAPI�е���ڴ��뱣����OldCodeA[]��OldCodeW[]  
    _asm   
    {   
        lea edi,stAsmInfo.OldCode
        mov esi,stAsmInfo.oldFunAddr
        cld   
        movsd   
        movsb   
    } 

    //��ȡ���ǵ�API�ĵ�ַ  
    _asm   
    {   
        mov eax,stAsmInfo.newFunAddr  
        mov ebx,stAsmInfo.oldFunAddr  
        sub eax,ebx   
        sub eax,5   
        mov dword ptr [stAsmInfo.NewCode+1],eax   
    }
    iRet = HookOn(&stAsmInfo);
    memcpy((void*)&(stHookInfo.stAsmInfo),(void*)&stAsmInfo, sizeof(ST_ASM_INFO));
    return iRet;
}

int HookList()
{
#ifdef I_DETOURS
    SetHook();
#else
    for (int i = 0; i < g_count; i++)
    {
        ST_HOOK_INFO* stHookInfo = (ST_HOOK_INFO*)g_data[i];
        if (NULL == stHookInfo)
        {
            continue;
        }
        HookOn(&(stHookInfo->stAsmInfo));
    }
#endif
    
    return 0;
}

int HookOffList()
{
#ifdef I_DETOURS
    DropHook();
#else
    for (int i = 0; i < g_count; i++)
    {
        ST_HOOK_INFO* stHookInfo = (ST_HOOK_INFO*)g_data[i];
        if (NULL == stHookInfo)
        {
            continue;
        }
        HookOff(&(stHookInfo->stAsmInfo));
    }
#endif
    return 0;
}

int intiInfo()
{
    // ��ʼ������
    ST_HOOK_INFO * stHookInfo = NULL;
    stHookInfo = new ST_HOOK_INFO;

    //sprintf_s(stHookInfo->szDllName, "%s", "User32.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "MessageBoxW");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyMessageBoxW;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "GDI32.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "TextOutW");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyTextOutW;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    stHookInfo = new ST_HOOK_INFO;
    sprintf_s(stHookInfo->szDllName, "%s", "GDI32.dll");
    sprintf_s(stHookInfo->szFuncName, "%s", "ExtTextOutW");
    stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyExtTextOutW;
    stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    inject(*stHookInfo);
    g_data[g_count++] = (unsigned int)stHookInfo;
    

    stHookInfo = new ST_HOOK_INFO;
    sprintf_s(stHookInfo->szDllName, "%s", "usp10.dll");
    sprintf_s(stHookInfo->szFuncName, "%s", "ScriptStringAnalyse");
    stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyScriptStringAnalyse;
    stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    inject(*stHookInfo);
    g_data[g_count++] = (unsigned int)stHookInfo;

    
    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "User32.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "DrawTextW");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyDrawTextW;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "User32.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "DrawTextExW");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyDrawTextExW;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    stHookInfo = new ST_HOOK_INFO;
    sprintf_s(stHookInfo->szDllName, "%s", "GDI32.dll");
    sprintf_s(stHookInfo->szFuncName, "%s", "BitBlt");
    stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyBitBlt;
    stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    inject(*stHookInfo);
    g_data[g_count++] = (unsigned int)stHookInfo;

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "GDI32.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "StretchBlt");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyStretchBlt;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "GDI32.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "CreateBitmap");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyCreateBitmap;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "GDI32.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "CreateCompatibleBitmap");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyCreateCompatibleBitmap;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "gdiplus.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "GdipDrawImage");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyGdipDrawImage;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "gdiplus.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "GdipDrawImageRectI");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyGdipDrawImageRectI;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "gdiplus.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "GdipDrawImagePointRect");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyGdipDrawImagePointRect;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;


    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "gdiplus.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "GdipDrawImageI");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyGdipDrawImageI;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "gdiplus.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "GdipDrawImageRectRect");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyGdipDrawImageRectRect;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "comctl32.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "ImageList_DrawIndirect");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyImageList_DrawIndirect;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;

    //stHookInfo = new ST_HOOK_INFO;
    //sprintf_s(stHookInfo->szDllName, "%s", "gdiplus.dll");
    //sprintf_s(stHookInfo->szFuncName, "%s", "GdipDrawString");
    //stHookInfo->stAsmInfo.newFunAddr = (unsigned int)MyGdipDrawString;
    //stHookInfo->stAsmInfo.NewCode[0] = 0xe9;
    //inject(*stHookInfo);
    //g_data[g_count++] = (unsigned int)stHookInfo;


//    _asm{int 3}

    if (!bIsInjected)  
    {   
        bIsInjected=TRUE;//��ֻ֤����1��  

        HookList();
    }
    return 0;
}

//���ǵļ�API����MyMessageBoxA  
int WINAPI MyMessageBoxA(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType)  
{  
    int nRet=0;  

    HookOffList();//����ԭ����֮ǰ���ǵ��Ȼָ�HOOKѽ����Ȼ�ǵ��ò�����  
    //������ָ�HOOK���͵���ԭ�������������ѭ��  
    //�Ͼ����õĻ������ǵĺ������Ӷ���ɶ�ջ��������������  
    //nRet=::MessageBoxA(hWnd,"������MessageBoxA��HOOK�˰�",lpCaption,uType);  
    nRet=::MessageBoxA(hWnd,lpText,lpCaption,uType);  
    HookList();//������ԭ�����󣬼ǵü�������HOOK����Ȼ�´λ�HOOK����Ŷ��   

    return nRet;
}  


//���ǵļ�API����MyMessageBoxW  
int WINAPI MyMessageBoxW(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType)  
{  
    int nRet=0;  

    HookOffList();

    nRet=::MessageBoxW(hWnd,_T("MessageBoxW��HOOK��"),lpCaption,uType);  
    nRet=::MessageBoxW(hWnd,lpText,lpCaption,uType);

    // ��������
    wsprintf(g_strData, _T("%s, %s ! %d "), lpText, lpCaption, g_value);
    //LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+1,(WPARAM)AfxGetApp()->m_pMainWnd->GetSafeHwnd(), (LPARAM)g_strData );
    //PostMessage(g_hWndMsg,WM_USER+1,(WPARAM)0, (LPARAM)g_strData );
    HookList();//������ԭ�����󣬼ǵü�������HOOK����Ȼ�´λ�HOOK����Ŷ��   

    return nRet;  
}  

int WINAPI MyTextOutW(__in HDC hdc, __in int x, __in int y, __in_ecount(c) LPCWSTR lpString, __in int c)
{
    int nRet=0;  

    HookOffList();
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)lpString,c);
    //}
    LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+1,(WPARAM)0, (LPARAM)0 );
    nRet = ::TextOutW(hdc,x,y,lpString,c);
    HookList();//������ԭ�����󣬼ǵü�������HOOK����Ȼ�´λ�HOOK����Ŷ��   

    return nRet; 
}

//__gdi_entry BOOL  WINAPI MyExtTextOutW( __in HDC hdc, __in int x, __in int y, __in UINT options, __in_opt CONST RECT * lprect, 
//                                       __in_ecount_opt(c) LPCWSTR lpString, __in UINT c, __in_ecount_opt(c) CONST INT * lpDx)
BOOL WINAPI MyExtTextOutW(_In_ HDC hdc,_In_ int X,_In_ int Y,_In_ UINT fuOptions,__in const RECT *lprc,
                          _In_ LPCTSTR lpString,_In_ UINT cbCount,_In_ const INT *lpDx)
{
    int nRet=0;  

    HookOffList();

    CDC* pDC;
    HDC hDC;
    RECT rect;
    RECT prc;
    BOOL bSendMsg = TRUE;
    int id = 0;
    rect.left = 0;
    rect.right = 0;
    rect.top = 0;
    rect.bottom = 0;
    prc = rect;


    //nRet = ::ExtTextOutW(hdc,x,y,options,lprect,lpString,c,lpDx);
	nRet = ::ExtTextOutW(hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx);
    
    HWND hWnd = WindowFromDC(hdc);   
    CWnd *wnd = CWnd::FromHandle(hWnd);
    if (NULL != wnd)
    {
        id = wnd->GetDlgCtrlID();
        wnd->GetWindowRect(&rect);
        if (rect.right-rect.left == 0 || rect.bottom-rect.top == 0 )
        {
            bSendMsg = FALSE;
        }
    }
    else
        bSendMsg = FALSE;

    if (NULL != lprc)
    {
        prc = *lprc;
    }
    if (nRet && g_len > 0)
    {

        if( (fuOptions == 4096 || fuOptions == 16) && bSendMsg )
        {
            memset(g_strDesc, 0x0 , 512);
            sprintf(g_strDesc, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",X, Y,prc.left, prc.top, prc.right,prc.bottom,
                rect.left,rect.top, rect.right,rect.bottom,id );
            int len = strlen(g_strDesc);

            LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+1,(WPARAM)g_len, (LPARAM)len );

            memset((void*)g_pShareBuf, 0x0, MAX_LEN);
            g_len = 0;
        }
    }
    HookList();
    return nRet; 
}


DWORD WINAPI MyGetCharacterPlacementW(
                                      _In_    HDC           hdc,
                                      _In_    LPCTSTR       lpString,
                                      _In_    int           nCount,
                                      _In_    int           nMaxExtent,
                                      _Inout_ LPGCP_RESULTS lpResults,
                                      _In_    DWORD         dwFlags
                                      )
{
    int nRet=0;  

    HookOffList();
    //int len = lstrlen(lpString);
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)lpString,nCount);
    //}
    //LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+1,(WPARAM)iTrs, (LPARAM)g_pShareBuf );

    nRet = ::GetCharacterPlacementW(hdc,lpString,nCount,nMaxExtent, lpResults, dwFlags);;
    HookList();
    return nRet; 
}


HRESULT MyScriptStringAnalyse(
                              _In_           HDC                    hdc,
                              _In_     const void                   *pString,
                              _In_           int                    cString,
                              _In_           int                    cGlyphs,
                              _In_           int                    iCharset,
                              _In_           DWORD                  dwFlags,
                              _In_           int                    iReqWidth,
                              _In_opt_       SCRIPT_CONTROL         *psControl,
                              _In_opt_       SCRIPT_STATE           *psState,
                              _In_opt_ const int                    *piDx,
                              _In_opt_       SCRIPT_TABDEF          *pTabdef,
                              _In_     const BYTE                   *pbInClass,
                              _Out_          SCRIPT_STRING_ANALYSIS *pssa
                              )
{
    HookOffList();

    int nRet=0;  
    //if (SSA_BREAK & dwFlags)
    //{
    //    int i = 0;
    //}
    //if (SSA_GLYPHS & dwFlags)
    //{
    //    int i = 0;
    //}
    //if (SSA_CLIP & dwFlags)
    //{
    //    int i = 0;
    //}
    if (g_len == 0)
    {
        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf,(void*)pString,cString*2+1);
        g_len = cString;
    }
    nRet = ::ScriptStringAnalyse(hdc,pString,cString,cGlyphs, iCharset, dwFlags,iReqWidth,
        psControl,psState,piDx,pTabdef,pbInClass,pssa);

    HookList();
    return nRet; 
}

HRESULT MyScriptIsComplex(
                          _In_ const WCHAR *pwcInChars,
                          _In_       int   cInChars,
                          _In_       DWORD dwFlags
                          )
{
    int nRet=0;  

    HookOffList();
    nRet = ::ScriptIsComplex(pwcInChars,cInChars,dwFlags);
    HookList();
    return nRet; 
}

int MyDrawTextW(
                __in HDC hdc,
                __inout_ecount_opt(cchText) LPCWSTR lpchText,
                __in int cchText,
                __inout LPRECT lprc,
                __in UINT format)
{
    int nRet=0;  

    HookOffList();
    nRet = ::DrawTextW(hdc, lpchText, cchText, lprc, format);
    HookList();
    return nRet; 
}

int WINAPI MyDrawTextExW(
                  __in HDC hdc,
                  __inout_ecount_opt(cchText) LPWSTR lpchText,
                  __in int cchText,
                  __inout LPRECT lprc,
                  __in UINT format,
                  __in_opt LPDRAWTEXTPARAMS lpdtp)
{
    int nRet=0;  

    HookOffList();
    nRet = ::DrawTextExW(hdc, lpchText, cchText, lprc, format,lpdtp );

    CDC* pDC;
    HDC hDC;
    RECT rect;
    BOOL bSendMsg = TRUE;
    rect.left = 0;
    rect.right = 0;
    rect.top = 0;
    rect.bottom = 0;

    pDC = CDC::FromHandle(hdc);
    HWND hWnd = WindowFromDC(hdc);
    if (NULL == hWnd)
    {
        DWORD d = GetLastError();
        int k = 0;
    }
    CWnd *wnd = CWnd::FromHandle(hWnd);
    if (NULL != wnd)
    {
        wnd->GetWindowRect(&rect);
        if (rect.right-rect.left == 0 || rect.bottom-rect.top == 0 )
        {
            bSendMsg = FALSE;
        }
    }
    else
        bSendMsg = FALSE;

    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    memcpy((void*)g_pShareBuf,(void*)lpchText,cchText*2+1);
    g_len = cchText;
    
    memset(g_strDesc, 0x0 , 512);
    sprintf(g_strDesc, "%d,%d,%d,%d,%d,%d,%d,%d",lprc->left, lprc->top, lprc->right,lprc->bottom,
        rect.left,rect.top, rect.right,rect.bottom);
    int len = strlen(g_strDesc);
    LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+1,(WPARAM)cchText, (LPARAM)len );

    HookList();
    return nRet; 
}


int fillBuff(CImage &image)
{
    int stride = 4 * ((image.GetWidth() + 3) / 4);
    size_t safeSize = stride * image.GetHeight() * 4 + sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER) + 256 * sizeof(RGBQUAD);
    size_t mux = stride * image.GetHeight() * 4;
    HGLOBAL mem = GlobalAlloc(GHND, safeSize);
    if (NULL == mem)
    {
        return 0;
    }

    HRESULT hr;
    IStream* stream = 0;
    hr = CreateStreamOnHGlobal(mem, TRUE, &stream);
    hr = image.Save(stream, Gdiplus::ImageFormatPNG);

    LARGE_INTEGER seekPos = {0};
    ULARGE_INTEGER imageSize;
    hr = stream->Seek(seekPos, STREAM_SEEK_CUR, &imageSize);

    //BYTE* buffer = new BYTE[imageSize.LowPart];

    hr = stream->Seek(seekPos, STREAM_SEEK_SET, 0);
    hr = stream->Read(g_BitBuffer, imageSize.LowPart, 0);

    // Fill buffer from stream
    hr = stream->Seek(seekPos, STREAM_SEEK_SET, 0);
    hr = stream->Read(g_BitBuffer, imageSize.LowPart, 0);
    
    if (NULL != mem)
    {
        GlobalFree(mem);
        mem = NULL;
    }
    return imageSize.LowPart;
}



__gdi_entry BOOL  WINAPI  MyBitBlt( __in HDC hdc, __in int x, __in int y,
                      __in int cx, __in int cy, __in_opt HDC hdcSrc, 
                      __in int x1, __in int y1, __in DWORD rop)
{
    HookOffList();
    
    CDC* pDC;
    HDC hDC;
    RECT rect;
    BOOL bSendMsg = TRUE;
    int id = 0;
    rect.left = 0;
    rect.right = 0;
    rect.top = 0;
    rect.bottom = 0;

    pDC = CDC::FromHandle(hdcSrc);
    CBitmap*bitmap = pDC->GetCurrentBitmap();
    HWND hWnd = WindowFromDC(hdc);
    CWnd *wnd = CWnd::FromHandle(hWnd);
    if (NULL != wnd)
    {
        wnd->GetWindowRect(&rect);
        id = wnd->GetDlgCtrlID();
        if (rect.right-rect.left == 0 || rect.bottom-rect.top == 0 )
        {
            bSendMsg = FALSE;
        }
    }
    else
        bSendMsg = FALSE;

    CImage image;
    HBITMAP hbmp = (HBITMAP)bitmap->GetSafeHandle();
    image.Attach(hbmp);
    int iSize = fillBuff(image);
    if (iSize > 0 && bSendMsg )
    {
        // ����
        memset(g_strDesc, 0x0 , 512);
        sprintf(g_strDesc, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",x, y,cx, cy, x1,y1,
            rect.left,rect.top, rect.right,rect.bottom,id );
        int len = strlen(g_strDesc);
        LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+2,(WPARAM)iSize, (LPARAM)len );
    }

    BOOL b = BitBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,rop);
    HookList();
    return b; 
}

__gdi_entry BOOL  WINAPI  MyStretchBlt(__in HDC hdcDest, __in int xDest, __in int yDest, 
                   __in int wDest, __in int hDest, __in_opt HDC hdcSrc, 
                   __in int xSrc, __in int ySrc, __in int wSrc, __in int hSrc, 
                   __in DWORD rop)
{
    HookOffList();
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}
    BOOL b = StretchBlt(hdcDest,xDest,yDest,wDest,hDest,hdcSrc,xSrc,ySrc,wSrc,hSrc,rop);
    //LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+1,(WPARAM)6, (LPARAM)0 );
    //::PostMessage(g_hWndMsg,WM_USER+1,(WPARAM)4, (LPARAM)y );
    //if (!b)
    //{
    //    DWORD d = GetLastError();
    //}
    //else
    //{
    //}
    //CDC *pDC;
    //HDC hDC;
    //pDC.m_hDC;
    //pDC=Attach(hDC);
    //CBitmap* bitmap = pDC->GetCurrentBitmap();
    //bitmap->m_hObject;
    //bitmap-
    //hDC=GetSafeHDC(pDC);
    //pDC->m_hDC==hDC

    
    HookList();
    return b; 
}


HBITMAP MyCreateBitmap( __in int nWidth, __in int nHeight, __in UINT nPlanes, __in UINT nBitCount, __in_opt CONST VOID *lpBits)
{
    int nRet=0;  

    HookOffList();
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}
    HBITMAP rest = ::CreateBitmap( nWidth,nHeight,nPlanes,nBitCount,lpBits);

    CImage image;
    image.Attach(rest);
    int iSize = fillBuff(image);

    ::SendMessage(g_hWndMsg,WM_USER+2,(WPARAM)iSize, (LPARAM)0 );

    //CImage image;
    //CBitmap cbitMap;
    //Bitmap map;
    //map.Save();
    //CBitmap;
    //IStream istreamInfo;
    //
    //IStream stre;
    //map.FromHBITMAP(rest);
    //map.Save(istreamInfo,)
    //cbitMap.FromHandle()
    //Bitmap::Save()
    //HookList();

    return rest; 
}

HBITMAP WINAPI MyCreateCompatibleBitmap( __in HDC hdc, __in int cx, __in int cy)
{
    int nRet=0;  

    HookOffList();
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}
    HBITMAP rest = ::CreateCompatibleBitmap( hdc,cx,cy);
    ::SendMessage(g_hWndMsg,WM_USER+1,(WPARAM)7, (LPARAM)0 );

    HookList();

    return rest; 
}

//
//static PBYTE GetDIBitsData(const HDC &hdc, const HBITMAP &hBitmap, BITMAPINFOHEADER &bi)  
//{  
//    bi.biSize = sizeof(BITMAPINFOHEADER);      
//    //bi.biWidth = bi.biWidth;     
//    bi.biHeight = abs(bi.biHeight);//�����Ե�����  
//    bi.biPlanes = 1;         
//    bi.biBitCount = (bi.biBitCount==32) ? 32 : 24;//����͸���ȵ�ͳһ���Ϊ24bitͼ  
//    bi.biCompression = BI_RGB;      
//    bi.biSizeImage = 0;    
//    bi.biXPelsPerMeter = 0;      
//    bi.biYPelsPerMeter = 0;      
//    bi.biClrUsed = 0;      
//    bi.biClrImportant = 0;  
//
//    //�������ݴ洢��     
//    int nLineSize = (((bi.biBitCount*bi.biPlanes*bi.biWidth + 31) & ~31) >> 3);//DIBÿ��Ϊ4�ֽڱ���������ȡ���������ÿ�к������0  
//    DWORD dwBitsSize = bi.biHeight * nLineSize;  
//
//    BOOL bRet = FALSE;  
//    PBYTE pBits = NULL;  
//    do   
//    {  
//        pBits = (PBYTE)malloc(dwBitsSize);    
//        if (NULL == pBits)    
//        {      
//            break;  
//        }    
//
//        //����λͼ��Ϣ��λͼ����  
//        if (0 == GetDIBits( hdc,     
//            hBitmap,     
//            0, bi.biHeight,     
//            pBits, (BITMAPINFO *)&bi,     
//            DIB_RGB_COLORS))    
//        {     
//            break;  
//        }  
//
//        bRet = TRUE;  
//    } while (false);  
//
//    if (!bRet)  
//    {  
//        FreeDIBitsData(pBits);  
//    }  
//
//    return bRet ? pBits : NULL;  
//} 

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)  
{  
    UINT  num = 0;  
    UINT  size = 0;  
    ImageCodecInfo* pImageCodecInfo = NULL;  
    GetImageEncodersSize(&num, &size);  
    if(size == 0)  
        return -1;  

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));  
    if(pImageCodecInfo == NULL)  
        return -1;  

    GetImageEncoders(num, size, pImageCodecInfo);  
    for(UINT j = 0; j < num; ++j)  
    {  
        if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )  
        {  
            *pClsid = pImageCodecInfo[j].Clsid;  
            free(pImageCodecInfo);  
            return j;  
        }      
    }  
    free(pImageCodecInfo);  
    return -1;  
} 

int fillBuff(GpImage &img)
{
    UINT  width = 0;
    UINT  height = 0;

    DllExports::GdipGetImageWidth(&img, &width);
    DllExports::GdipGetImageHeight(&img, &height);

    int stride = 4 * ((width + 3) / 4);
    size_t safeSize = stride * height * 4 + sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER) + 256 * sizeof(RGBQUAD);
    HGLOBAL mem = GlobalAlloc(GHND, safeSize);
    HRESULT hr;
    IStream* stream = 0;
    hr = CreateStreamOnHGlobal(mem, TRUE, &stream);

    CLSID sid;
    GetEncoderClsid(_T("image/png"), &sid);

    CLSID clsid = Gdiplus::ImageFormatPNG;
    GpStatus status = DllExports::GdipSaveImageToStream(&img,stream, &sid, NULL);
    
    
    LARGE_INTEGER seekPos = {0};
    ULARGE_INTEGER imageSize;
    hr = stream->Seek(seekPos, STREAM_SEEK_CUR, &imageSize);
    BYTE* buffer = new BYTE[imageSize.LowPart];

    hr = stream->Seek(seekPos, STREAM_SEEK_SET, 0);
    hr = stream->Read(g_BitBuffer, imageSize.LowPart, 0);

    //delete [] buffer;
    //GlobalFree(mem);
    // ��ȡbyte����stream
    //stream->Write(buffer, imageSize.LowPart, NULL);

    return safeSize;
}




GpStatus WINGDIPAPI
MyGdipDrawImage(GpGraphics *graphics, GpImage *image, REAL x, REAL y)
{
    int nRet=0;  

    HookOffList();
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}
    
    LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+3,(WPARAM)1, (LPARAM)0 );
    GpStatus status = DllExports::GdipDrawImage(graphics,image,x,y);

    HookList();
    return status; 
}


GpStatus WINGDIPAPI
MyGdipDrawImageI(GpGraphics *graphics, GpImage *image, INT x, INT y)
{
    int nRet=0;  

    HookOffList();
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}
    LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+3,(WPARAM)5, (LPARAM)0 );
    GpStatus status = DllExports::GdipDrawImageI(graphics,image,x,y);

    HookList();
    return status; 
}

GpStatus WINGDIPAPI
MyGdipDrawImageRectI(GpGraphics *graphics, GpImage *image, INT x, INT y,
                     INT width, INT height)
{
    int nRet=0;  

    HookOffList();
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}

    LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+3,(WPARAM)2, (LPARAM)0 );
    GpStatus status = DllExports::GdipDrawImageRectI(graphics,image,x,y,width,height);

    HookList();
    return status; 
}

GpStatus WINGDIPAPI
MyGdipDrawImagePointRect(GpGraphics *graphics, GpImage *image, REAL x,
                         REAL y, REAL srcx, REAL srcy, REAL srcwidth,
                         REAL srcheight, GpUnit srcUnit)
{
    int nRet=0;  

    HookOffList();
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}

    LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+3,(WPARAM)3, (LPARAM)0 );
    GpStatus status = DllExports::GdipDrawImagePointRect(graphics,image,x,y,srcx,srcy,srcwidth,srcheight,srcUnit);
    HookList();
    return status; 
}


GpStatus WINGDIPAPI
MyGdipDrawImageRectRect(GpGraphics *graphics, GpImage *image, REAL dstx,
                        REAL dsty, REAL dstwidth, REAL dstheight,
                        REAL srcx, REAL srcy, REAL srcwidth, REAL srcheight,
                        GpUnit srcUnit,
                        GDIPCONST GpImageAttributes* imageAttributes,
                        DrawImageAbort callback, VOID * callbackData)
{
    int nRet=0;  


    CDC* pDC;
    HDC hDC;
    RECT rect;
    BOOL bSendMsg = TRUE;
    GpStatus status ;
    rect.left = 0;
    rect.right = 0;
    rect.top = 0;
    rect.bottom = 0;

    //status = DllExports::GdipGetDC(graphics, &hDC);
    //pDC = CDC::FromHandle(hDC);
    //HWND hWnd = WindowFromDC(hDC);
    //CWnd *wnd = CWnd::FromHandle(hWnd);

    //if (NULL != wnd)
    //{
    //    //wnd->GetWindowRect(&rect);
    //    wnd->GetClientRect(&rect);
    //    //if (rect.right-rect.left == 0 || rect.bottom-rect.top == 0 )
    //    //{
    //    //    bSendMsg = FALSE;
    //    //}
    //}
    //else
    //    bSendMsg = FALSE;
    //DllExports::GdipReleaseDC(graphics, hDC);

    HookOffList();



    size_t size = fillBuff(*image);
    if (size > 0  )
    {
        // ����
        memset(g_strDesc, 0x0 , 512);
        sprintf(g_strDesc, "%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%d,%d",dstx, dsty, dstwidth, dstheight,
            srcx, srcy, srcwidth, srcheight ,rect.left,rect.right,rect.top,rect.bottom );
        int len = strlen(g_strDesc);
        LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+3,(WPARAM)size, (LPARAM)len );
    }
    status = DllExports::GdipDrawImageRectRect(graphics,image,dstx,dsty,dstwidth,dstheight,
        srcx,srcy,srcwidth,srcheight,srcUnit, imageAttributes, callback, callbackData );

    HookList();

    return status; 
}

BOOL MyImageList_DrawIndirect(IMAGELISTDRAWPARAMS* pimldp)
{
    int nRet=0;  
    //_asm{int 3}
    HookOffList();
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}

    LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+3,(WPARAM)20, (LPARAM)0 );
    bool b = ImageList_DrawIndirect(pimldp);
    HookList();

    return b; 
}


GpStatus WINGDIPAPI
MyGdipDrawString(
                 GpGraphics               *graphics,
                 GDIPCONST WCHAR          *string,
                 INT                       length,
                 GDIPCONST GpFont         *font,
                 GDIPCONST RectF          *layoutRect,
                 GDIPCONST GpStringFormat *stringFormat,
                 GDIPCONST GpBrush        *brush
                 )
{
    int nRet=0;  

    HookOffList();
    int iTrs = 0;
    if (NULL != g_pShareBuf)
    {
        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf,(void*)string,length*2+1);
    }

    LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+3,(WPARAM)9, (LPARAM)0 );
    GpStatus status = DllExports::GdipDrawString(graphics,string,length,font,layoutRect,stringFormat,brush);
    HookList();
    return status; 
}

// ��ʼ�������ڴ�
int initShareMemory()
{
    g_hMapMem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szName );  
    if (g_hMapMem == NULL)  
    {  
        return -1;
    }
    g_pShareBuf = (LPTSTR)MapViewOfFile(g_hMapMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);  
    if (NULL == g_pShareBuf)
    {
        CloseHandle(g_hMapMem);  
        g_hMapMem = NULL;
        return -2;
    }

    
    // λͼ�����ڴ�
    g_hMapMem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szNameBitMap );  
    if (g_hMapMem == NULL)  
    {  
        return -1;
    }
    g_BitBuffer = (BYTE*)MapViewOfFile(g_hMapMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);  
    if (NULL == g_pShareBuf)
    {
        CloseHandle(g_hMapMem);  
        g_hMapMem = NULL;
        return -2;
    }

    // λͼ������Ϣ
    g_hMapMem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szNameDataDesc );  
    if (g_hMapMem == NULL)  
    {  
        return -1;
    }
    g_strDesc = (char*)MapViewOfFile(g_hMapMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);  
    if (NULL == g_pShareBuf)
    {
        CloseHandle(g_hMapMem);  
        g_hMapMem = NULL;
        return -2;
    }

    return 0;
}

int closeShareMemery()
{
    if(NULL != g_pShareBuf)
        UnmapViewOfFile(g_pShareBuf);  
    if(NULL != g_hMapMem)
        CloseHandle(g_hMapMem);
    return 0;
}

//��깳�ӹ��̣�Ŀ���Ǽ��ر�dll��ʹ�����ĳ���  
//��깳�ӵ����ã��������ĳ���򴰿���ʱ����ͻ�����������dll  
LRESULT CALLBACK MouseProc(  int nCode,      // hook code  
                           WPARAM wParam,  // message identifier  
                           LPARAM lParam   // mouse coordinates  
                           )  
{  
    if (nCode==HC_ACTION)  
    {  
        //���������ڴ��ھ������������  
        ::SendMessage(g_hWnd,UM_WNDTITLE,wParam,(LPARAM)(((PMOUSEHOOKSTRUCT)lParam)->hwnd));  
    }  
    return CallNextHookEx(hhk,nCode,wParam,lParam);  
}  


//��װ����  

extern "C" __declspec(dllexport) BOOL StartHook(HWND hWnd)  
{  
    g_hWnd=hWnd;  
    hhk=::SetWindowsHookEx(WH_MOUSE,MouseProc,hInst,0);  
    if (hhk==NULL)  
    {  
        return FALSE;  
    }   
    else  
    {  
        return TRUE;  
    }  
}  

//ж�ع���  
extern "C" __declspec(dllexport) void StopHook()  
{  
    HookOffList();
    if (hhk!=NULL)  
    {  
        UnhookWindowsHookEx(hhk);  
        FreeLibrary(hInst);  
    }  
}  

extern "C" __declspec(dllexport) void SetMsgWnd(HWND wnd)
{
    g_hWndMsg = wnd;
}

extern "C" __declspec(dllexport) int getValue()
{
    return g_value;
}

extern "C" __declspec(dllexport) void setValue(int value)
{
    g_value = value;
}

extern "C" __declspec(dllexport) void getData(void* p)
{
    TCHAR *ch = (TCHAR*)p;
    wsprintf(ch, _T("%s"), g_strData);
}

// ChookcommonApp

BEGIN_MESSAGE_MAP(ChookcommonApp, CWinApp)
END_MESSAGE_MAP()


// ChookcommonApp ����

ChookcommonApp::ChookcommonApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� ChookcommonApp ����

ChookcommonApp theApp;


// ChookcommonApp ��ʼ��

BOOL ChookcommonApp::InitInstance()
{
	CWinApp::InitInstance();

    afxAmbientActCtx = FALSE;

    //GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);

    hInst=AfxGetInstanceHandle();  
    DWORD dwPid=::GetCurrentProcessId();  
    hProcess=::OpenProcess(PROCESS_ALL_ACCESS,0,dwPid);  

#ifdef I_DETOURS
    SetHook();
#else
    initShareMemory();
    intiInfo();
#endif


	return TRUE;
}


int ChookcommonApp::ExitInstance()
{  
#ifdef I_DETOURS
    DropHook();
#else
    HookOffList();
    for(int i = 0; i < g_count; i++)
    {
        ST_HOOK_INFO *st = (ST_HOOK_INFO*)g_data[i];
        delete st;
        st = NULL;
        g_data[i] = 0;
    }
    closeShareMemery();
#endif
    //Gdiplus::GdiplusShutdown(g_gdiplusToken);

    return CWinApp::ExitInstance();
}

