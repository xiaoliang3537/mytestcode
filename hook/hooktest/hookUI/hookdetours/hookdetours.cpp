// hookdetours.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include "hookdetours.h"
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
#include <winbase.h>
#include <iostream>
#include <fstream>
#include <Wininet.h>
#include "../hookUI/define.h"
#include <DocObj.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#include <NTSTATUS.h>
using namespace Gdiplus;
#pragma  comment(lib, "gdiplus.lib")
#pragma  comment(lib, "Wininet.lib")

#define I_DETOURS

using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  UM_WNDTITLE WM_USER+100                                        //自定义消息  



TCHAR szName[] = TEXT("Global\\MyFileMappingObject");                     //指向同一块共享内存的名字  
TCHAR szNameBitMap[] = TEXT("Global\\MyFileMappingObjectBitMap");         //指向同一块共享内存的名字  
TCHAR szNameDataDesc[] = TEXT("Global\\MyFileMappingDataDesc");           //描述数据  保存坐标信息 设备信息数据  
TCHAR szNameDataSocket[] = TEXT("Global\\MyFileMappingDataSocket");       //socket数据  保存坐标信息 设备信息数据  


                                                                          //全局共享变量  
#pragma data_seg(".ShareW")  
HWND g_hWnd = NULL;                                     //主窗口句柄;  
HWND g_hWndMsg = NULL;                                  //回调消息窗口句柄
HHOOK hhk = NULL;                                       //鼠标钩子句柄;  
HINSTANCE hInst = NULL;                                   //本dll实例句柄;  
TCHAR g_strData[MAX_LEN] = { 0 };
int g_value = 0;
CMutex g_clsMutex(FALSE, NULL);
CRITICAL_SECTION  g_cs_test;

#pragma data_seg()  
#pragma comment(linker, "/section:.ShareW,rws")  


LPCTSTR g_pShareBuf = NULL;                             // 文本部分共享内存
int g_iRecvLen = 0;                                     // 接收文本长度
int g_len = 0;                                          // 字符长度

BYTE* g_BitBuffer = NULL;                               // 共享全局位图数据
char* g_strDesc = NULL;                                 // 共享全局位图坐标信息
char* g_strSocket = NULL;                               // 共享socket数据信息
HANDLE hProcess = NULL;                                 // 被注入进程句柄
BOOL bIsInjected = FALSE;                               // 是否注入的标志
int g_count = 0;                                        // hook api数量
HANDLE g_hMapMem = NULL;
int iHookTypeWay = -1;                                  // hook 方式 1 注入 2 通过setWindowHook载入

unsigned int g_data[20] = { 0 };
PVOID g_pResumFunc = NULL;                              // 句柄


typedef struct ST_ASM_INFO
{
    ST_ASM_INFO()
    {
        oldFunAddr = NULL;
        oldFunAddr = NULL;
        NewCode[0] = 0xe9;
    };
    unsigned int newFunAddr;
    PVOID oldFunAddr;

    BYTE OldCode[5]; //老的系统API入口代码  
    BYTE NewCode[5]; //要跳转的API代码 (jmp xxxx)
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

//
/*********************************************************************************************************/
// WM_USER + 1 文本消息
// WM_USER + 2 bitblt贴图消息
// WM_USER + 3 gdi+贴图消息
// WM_USER + 4 socket recv 消息
// WM_USER + 5 socket send 消息
/*********************************************************************************************************/
// 将图片转换成数据流
int fillBuff(CImage &image, BYTE* pByte);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*GDI*/
int WINAPI MyMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
int (WINAPI*pMessageBoxW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) = MessageBoxW;
int WINAPI MyMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);

int (WINAPI *pTextOutA)(__in HDC hdc, __in int x, __in int y, __in_ecount(c) LPCSTR lpString, __in int c) = TextOutA;
int WINAPI MyTextOutA(__in HDC hdc, __in int x, __in int y, __in_ecount(c) LPCSTR lpString, __in int c);
int (WINAPI *pTextOutW)(__in HDC hdc, __in int x, __in int y, __in_ecount(c) LPCWSTR lpString, __in int c) = TextOutW;
int WINAPI MyTextOutW(__in HDC hdc, __in int x, __in int y, __in_ecount(c) LPCWSTR lpString, __in int c);

_Success_(return) int  (WINAPI *pDrawTextA)(
    _In_ HDC hdc,
    _When_((format & DT_MODIFYSTRING), _At_((LPSTR)lpchText, _Inout_grows_updates_bypassable_or_z_(cchText, 4)))
    _When_((!(format & DT_MODIFYSTRING)), _In_bypassable_reads_or_z_(cchText))
    LPCSTR lpchText,
    _In_ int cchText,
    _Inout_ LPRECT lprc,
    _In_ UINT format) = DrawTextA;

_Success_(return) int  WINAPI MyDrawTextA(
    _In_ HDC hdc,
    _When_((format & DT_MODIFYSTRING), _At_((LPSTR)lpchText, _Inout_grows_updates_bypassable_or_z_(cchText, 4)))
    _When_((!(format & DT_MODIFYSTRING)), _In_bypassable_reads_or_z_(cchText))
    LPCSTR lpchText,
    _In_ int cchText,
    _Inout_ LPRECT lprc,
    _In_ UINT format) ;

_Success_(return) int (WINAPI* pDrawTextW)(
    _In_ HDC hdc,
    _When_((format & DT_MODIFYSTRING), _At_((LPWSTR)lpchText, _Inout_grows_updates_bypassable_or_z_(cchText, 4)))
    _When_((!(format & DT_MODIFYSTRING)), _In_bypassable_reads_or_z_(cchText))
    LPCWSTR lpchText,
    _In_ int cchText,
    _Inout_ LPRECT lprc,
    _In_ UINT format) = DrawTextW;

_Success_(return) int WINAPI MyDrawTextW(_In_ HDC hdc,
    _When_((format & DT_MODIFYSTRING), _At_((LPWSTR)lpchText, _Inout_grows_updates_bypassable_or_z_(cchText, 4)))
    _When_((!(format & DT_MODIFYSTRING)), _In_bypassable_reads_or_z_(cchText))
    LPCWSTR lpchText,
    _In_ int cchText,
    _Inout_ LPRECT lprc,
    _In_ UINT format);

int (WINAPI  *pDrawTextExA)(__in HDC hdc, __inout_ecount_opt(cchText) LPSTR lpchText, __in int cchText, 
    __inout LPRECT lprc, __in UINT format, __in_opt LPDRAWTEXTPARAMS lpdtp) = DrawTextExA;
int WINAPI  MyDrawTextExA(__in HDC hdc, __inout_ecount_opt(cchText) LPSTR lpchText, __in int cchText, 
    __inout LPRECT lprc, __in UINT format, __in_opt LPDRAWTEXTPARAMS lpdtp);
int (WINAPI  *pDrawTextExW)(__in HDC hdc, __inout_ecount_opt(cchText) LPWSTR lpchText, __in int cchText,
    __inout LPRECT lprc, __in UINT format, __in_opt LPDRAWTEXTPARAMS lpdtp) = DrawTextExW;
int WINAPI  MyDrawTextExW(__in HDC hdc, __inout_ecount_opt(cchText) LPWSTR lpchText, __in int cchText, 
    __inout LPRECT lprc, __in UINT format, __in_opt LPDRAWTEXTPARAMS lpdtp);


BOOL (WINAPI *pExtTextOutA)(_In_ HDC hdc, _In_ int X, _In_ int Y, _In_ UINT fuOptions, _In_ const RECT *lprc,
    _In_ LPCSTR lpString, _In_ UINT cbCount, _In_ const INT *lpDx) = ExtTextOutA;
BOOL WINAPI MyExtTextOutA(_In_ HDC hdc, _In_ int X, _In_ int Y, _In_ UINT fuOptions, _In_ const RECT *lprc,
    _In_ LPCSTR lpString, _In_ UINT cbCount, _In_ const INT *lpDx);

__gdi_entry BOOL(WINAPI *pExtTextOutW)(_In_ HDC hdc, _In_ int X, _In_ int Y, _In_ UINT fuOptions, _In_ const RECT *lprc,
    _In_ LPCTSTR lpString, _In_ UINT cbCount, _In_ const INT *lpDx) = ExtTextOutW;
__gdi_entry BOOL WINAPI MyExtTextOutW(_In_ HDC hdc, _In_ int X, _In_ int Y, _In_ UINT fuOptions, _In_ const RECT *lprc,
    _In_ LPCTSTR lpString, _In_ UINT cbCount, _In_ const INT *lpDx);

DWORD WINAPI MyGetCharacterPlacementW(_In_    HDC           hdc,
    _In_    LPCTSTR       lpString,
    _In_    int           nCount,
    _In_    int           nMaxExtent,
    _Inout_ LPGCP_RESULTS lpResults,
    _In_    DWORD         dwFlags
);

__checkReturn HRESULT(WINAPI*pScriptStringAnalyse)(
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
    ) = ScriptStringAnalyse;

__checkReturn HRESULT MyScriptStringAnalyse(
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

HRESULT MyScriptIsComplex(_In_ const WCHAR *pwcInChars, _In_       int   cInChars, _In_       DWORD dwFlags);

__gdi_entry BOOL(WINAPI    *pBitBlt)(__in HDC hdc, __in int x, __in int y, __in int cx, __in int cy, 
    __in_opt HDC hdcSrc, __in int x1,    __in int y1, __in DWORD rop) = BitBlt;
__gdi_entry BOOL  WINAPI    MyBitBlt(__in HDC hdc, __in int x, __in int y, __in int cx, __in int cy, 
    __in_opt HDC hdcSrc, __in int x1,    __in int y1, __in DWORD rop);

__gdi_entry BOOL  WINAPI  MyStretchBlt(__in HDC hdcDest, __in int xDest, __in int yDest, __in int wDest, __in int hDest, __in_opt HDC hdcSrc,
    __in int xSrc, __in int ySrc, __in int wSrc, __in int hSrc, __in DWORD rop);

HBITMAP MyCreateBitmap(__in int nWidth, __in int nHeight, __in UINT nPlanes, __in UINT nBitCount, __in_opt CONST VOID *lpBits);

HBITMAP WINAPI MyCreateCompatibleBitmap(__in HDC hdc, __in int cx, __in int cy);

BOOL (WINAPI *pSetWindowTextA)( _In_ HWND hWnd, _In_opt_ LPCSTR lpString) = SetWindowTextA;
BOOL WINAPI MySetWindowTextA(_In_ HWND hWnd, _In_opt_ LPCSTR lpString);
BOOL(WINAPI *pSetWindowTextW)(__in HWND hWnd, __in_opt LPCWSTR lpString) = SetWindowTextW;
BOOL WINAPI MySetWindowTextW(__in HWND hWnd, __in_opt LPCWSTR lpString);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*GDI+*/
GpStatus WINGDIPAPI MyGdipDrawImage(GpGraphics *graphics, GpImage *image, REAL x, REAL y);

GpStatus WINGDIPAPI MyGdipDrawImageI(GpGraphics *graphics, GpImage *image, INT x, INT y);

GpStatus WINGDIPAPI MyGdipDrawImageRectI(GpGraphics *graphics, GpImage *image, INT x, INT y, INT width, INT height);

GpStatus WINGDIPAPI MyGdipDrawImagePointRect(GpGraphics *graphics, GpImage *image, REAL x, REAL y, REAL srcx, REAL srcy, REAL srcwidth,
    REAL srcheight, GpUnit srcUnit);

GpStatus(WINGDIPAPI *pGdipDrawImageRectRect)(GpGraphics *graphics, GpImage *image, REAL dstx, REAL dsty, REAL dstwidth, REAL dstheight,
    REAL srcx, REAL srcy, REAL srcwidth, REAL srcheight, GpUnit srcUnit, GDIPCONST GpImageAttributes* imageAttributes,
    DrawImageAbort callback, VOID * callbackData) = DllExports::GdipDrawImageRectRect;
GpStatus WINGDIPAPI MyGdipDrawImageRectRect(GpGraphics *graphics, GpImage *image, REAL dstx, REAL dsty, REAL dstwidth, REAL dstheight,
    REAL srcx, REAL srcy, REAL srcwidth, REAL srcheight, GpUnit srcUnit, GDIPCONST GpImageAttributes* imageAttributes,
    DrawImageAbort callback, VOID * callbackData);

BOOL MyImageList_DrawIndirect(IMAGELISTDRAWPARAMS* pimldp);

GpStatus(WINGDIPAPI *pGdipDrawString)(
    GpGraphics               *graphics,
    GDIPCONST WCHAR          *string,
    INT                       length,
    GDIPCONST GpFont         *font,
    GDIPCONST RectF          *layoutRect,
    GDIPCONST GpStringFormat *stringFormat,
    GDIPCONST GpBrush        *brush
    ) = DllExports::GdipDrawString;

GpStatus WINGDIPAPI MyGdipDrawString(
    GpGraphics               *graphics,
    GDIPCONST WCHAR          *string,
    INT                       length,
    GDIPCONST GpFont         *font,
    GDIPCONST RectF          *layoutRect,
    GDIPCONST GpStringFormat *stringFormat,
    GDIPCONST GpBrush        *brush
);

//////////////////////////////////////////////////////////////////////////
/* hook socket */
//////////////////////////////////////////////////////////////////////////
int (WSAAPI *pSend)(IN SOCKET s, __in_bcount(len) const char FAR * buf, IN int len, IN int flags) = send;
int WSAAPI Mysend(IN SOCKET s, __in_bcount(len) const char FAR * buf, IN int len, IN int flags);

int (PASCAL FAR *pRecv)(IN SOCKET s, __out_bcount_part(len, return) __out_data_source(NETWORK) char FAR * buf,
    IN int len, IN int flags) = recv;
int PASCAL FAR MyRecv(IN SOCKET s, __out_bcount_part(len, return) __out_data_source(NETWORK) char FAR * buf,
    IN int len, IN int flags);
//////////////////////////////////////////////////////////////////////////

/*****************************************************************************************************************************/
// mouse event
VOID (WINAPI *pMouse_event)(
    _In_ DWORD dwFlags,
    _In_ DWORD dx,
    _In_ DWORD dy,
    _In_ DWORD dwData,
    _In_ ULONG_PTR dwExtraInfo) = mouse_event;
VOID WINAPI MyMouse_event(
    _In_ DWORD dwFlags,
    _In_ DWORD dx,
    _In_ DWORD dy,
    _In_ DWORD dwData,
    _In_ ULONG_PTR dwExtraInfo);
/*****************************************************************************************************************************/

/*****************************************************************************************************************************/
// message
BOOL (WINAPI *pGetMessageW)(
    _Out_ LPMSG lpMsg,
    _In_opt_ HWND hWnd,
    _In_ UINT wMsgFilterMin,
    _In_ UINT wMsgFilterMax) = GetMessageW;

BOOL WINAPI MyGetMessageW(
    _Out_ LPMSG lpMsg,
    _In_opt_ HWND hWnd,
    _In_ UINT wMsgFilterMin,
    _In_ UINT wMsgFilterMax) ;

LRESULT (WINAPI *pDefWindowProcW)(
    _In_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam) = DefWindowProcW;

LRESULT WINAPI MyDefWindowProcW(
    _In_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam) ;

BOOL(WINAPI *pDestroyWindow)(_In_ HWND hWnd) = DestroyWindow;
BOOL WINAPI MyDestroyWindow(_In_ HWND hWnd);
/*****************************************************************************************************************************/

/*****************************************************************************************************************************/
//dllmain.cpp
//class CDetour /* add ": public CRealRun" to enable access to member variables... */
//{
//public:
//    int Mine_Target(int cmd);
//    static int(CDetour::* Real_Target)(int cmd);
//
//    // Class shouldn't have any member variables or virtual functions.
//};
//
//int CDetour::Mine_Target(int cmd)
//{
//    printf("  CDetour::Mine_Target! (this:%p)\n", this);
//    (this->*Real_Target)();
//}
//
//int (CDetour::* CDetour::Real_Target)(int cmd) = (int (CDetour::*)(int cmd))&CRealRun::Run;
/*****************************************************************************************************************************/

BOOL(WINAPI *pInternetReadFile)(
    _In_ HINTERNET hFile,
    _Out_writes_bytes_(dwNumberOfBytesToRead) __out_data_source(NETWORK) LPVOID lpBuffer,
    _In_ DWORD dwNumberOfBytesToRead,
    _Out_ LPDWORD lpdwNumberOfBytesRead
) = InternetReadFile;
BOOLAPI MyInternetReadFile(
    _In_ HINTERNET hFile,
    _Out_writes_bytes_(dwNumberOfBytesToRead) __out_data_source(NETWORK) LPVOID lpBuffer,
    _In_ DWORD dwNumberOfBytesToRead,
    _Out_ LPDWORD lpdwNumberOfBytesRead
);

//void (AFX_CDECL *pInvokeHelper)(DISPID dwDispID, WORD wFlags,
//    VARTYPE vtRet, void* pvRet, const BYTE* pbParamInfo, ...) = InvokeHelper;

//LRESULT (WINAPI* pCallNextHookEx)(_In_opt_ HHOOK hhk, _In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam) = CallNextHookEx;
//LRESULT WINAPI MyCallNextHookEx(_In_opt_ HHOOK hhk, _In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam);


BOOL APIENTRY SetHook(char* szDllName, char* szDllFunc, void* targetFunc)
{
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    //DetourAttach(&(PVOID&)pMessageBoxW, MyMessageBoxW);
    //DetourAttach(&(PVOID&)pBitBlt, MyBitBlt);
    
    //DetourAttach(&(PVOID&)pDrawTextA, MyDrawTextA);
    //DetourAttach(&(PVOID&)pDrawTextExA, MyDrawTextExA);
    //DetourAttach(&(PVOID&)pTextOutA, MyTextOutA);
    //DetourAttach(&(PVOID&)pExtTextOutA, MyExtTextOutA);
    //DetourAttach(&(PVOID&)pSetWindowTextA, MySetWindowTextA);

    //DetourAttach(&(PVOID&)pDrawTextW, MyDrawTextW);
    //DetourAttach(&(PVOID&)pDrawTextExW, MyDrawTextExW);
    //DetourAttach(&(PVOID&)pTextOutW, MyTextOutW);
    //DetourAttach(&(PVOID&)pExtTextOutW, MyExtTextOutW);
    //DetourAttach(&(PVOID&)pScriptStringAnalyse, MyScriptStringAnalyse);
    //DetourAttach(&(PVOID&)pGdipDrawImageRectRect, MyGdipDrawImageRectRect);
    //DetourAttach(&(PVOID&)pGdipDrawString, MyGdipDrawString);
    //DetourAttach(&(PVOID&)pSetWindowTextW, MySetWindowTextW);

    //DetourAttach(&(PVOID&)pCallNextHookEx, MyCallNextHookEx);
    
    //DetourAttach(&(PVOID&)pSend, Mysend);
    //DetourAttach(&(PVOID&)pRecv, MyRecv);
    //DetourAttach(&(PVOID&)pMouse_event, MyMouse_event);
    //DetourAttach(&(PVOID&)pInternetReadFile, MyInternetReadFile);
    
    DetourAttach(&(PVOID&)pGetMessageW, MyGetMessageW);
    DetourAttach(&(PVOID&)pDefWindowProcW, MyDefWindowProcW);
    DetourAttach(&(PVOID&)pDestroyWindow, MyDestroyWindow);
    
    LONG ret = DetourTransactionCommit();

    return bIsInjected;

}


BOOL APIENTRY DropHook(char* szDllName, char* szDllFunc, void* targetFunc)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    //DetourDetach(&(PVOID&)pMessageBoxW, MyMessageBoxW);

    //DetourDetach(&(PVOID&)pBitBlt, MyBitBlt);

    //DetourDetach(&(PVOID&)pDrawTextA, MyDrawTextA);
    //DetourDetach(&(PVOID&)pDrawTextExA, MyDrawTextExA);
    //DetourDetach(&(PVOID&)pTextOutA, MyTextOutA);
    //DetourDetach(&(PVOID&)pExtTextOutA, MyExtTextOutA);
    //DetourDetach(&(PVOID&)pSetWindowTextA, MySetWindowTextA);

    //DetourDetach(&(PVOID&)pDrawTextW, MyDrawTextW);
    //DetourDetach(&(PVOID&)pDrawTextExW, MyDrawTextExW);
    //DetourDetach(&(PVOID&)pTextOutW, MyTextOutW);
    //DetourDetach(&(PVOID&)pExtTextOutW, MyExtTextOutW);
    //DetourDetach(&(PVOID&)pScriptStringAnalyse, MyScriptStringAnalyse);
    //DetourDetach(&(PVOID&)pGdipDrawImageRectRect, MyGdipDrawImageRectRect);
    //DetourDetach(&(PVOID&)pGdipDrawString, MyGdipDrawString);
    //DetourDetach(&(PVOID&)pSetWindowTextW, MySetWindowTextW);

    //DetourDetach(&(PVOID&)pCallNextHookEx, MyCallNextHookEx);

    //DetourDetach(&(PVOID&)pSend, Mysend);
    //DetourDetach(&(PVOID&)pRecv, MyRecv);
    //DetourDetach(&(PVOID&)pMouse_event, MyMouse_event);
    //DetourDetach(&(PVOID&)pInternetReadFile, MyInternetReadFile);
    DetourDetach(&(PVOID&)pGetMessageW, MyGetMessageW);
    DetourDetach(&(PVOID&)pDefWindowProcW, MyDefWindowProcW);
    DetourDetach(&(PVOID&)pDestroyWindow, MyDestroyWindow);
    LONG ret = DetourTransactionCommit();
    return ret == NO_ERROR;
}


int HookList()
{
    //如果已经安装就return  
    if (bIsInjected)return TRUE;
    SetHook("", "", NULL);
    bIsInjected = TRUE;
    return 0;
}

int HookOffList()
{
    //如果已经安装就return  
    if (!bIsInjected)return TRUE;
    DropHook("", "", NULL);
    bIsInjected = FALSE;

    return 0;
}

int intiInfo()
{
    if (!bIsInjected)
    {
        HookList();
        bIsInjected = TRUE;//保证只调用1次  
    }

    return 0;
}



int WINAPI MyMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    int nRet = 0;
    nRet = ::MessageBoxA(hWnd, lpText, lpCaption, uType);
    return nRet;
}

int WINAPI MyMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    int nRet = 0;
    nRet = pMessageBoxW(hWnd, lpText, lpCaption, uType);

    return nRet;
}

int WINAPI MyTextOutA(__in HDC hdc, __in int x, __in int y, __in_ecount(c) LPCSTR lpString, __in int c)
{
    int nRet = 0;
    if (c > 0)
    {
        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf, (void*)lpString, c * 2 + 1);
        g_len = c;
    }
    LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)g_len, (LPARAM)0);
    nRet = ::TextOutA(hdc, x, y, lpString, c);
    return nRet;
}

int WINAPI MyTextOutW(__in HDC hdc, __in int x, __in int y, __in_ecount(c) LPCWSTR lpString, __in int c)
{
    int nRet = 0;
    nRet = pTextOutW(hdc, x, y, lpString, c);
    if (c > 0)
    {
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

        HWND hWnd = WindowFromDC(hdc);
        CWnd *wnd = CWnd::FromHandle(hWnd);
        if (NULL != wnd)
        {
            id = wnd->GetDlgCtrlID();
            wnd->GetWindowRect(&rect);
            if (rect.right - rect.left == 0 || rect.bottom - rect.top == 0)
            {
                bSendMsg = FALSE;
            }
        }
        else
            bSendMsg = FALSE;

        memset(g_strDesc, 0x0, 512);
        sprintf(g_strDesc, "%d,%d,%d,%d,%d,%d,%d,%d,3_%d_%#X", x, y, 0, 0,
            rect.left, rect.top, rect.right, rect.bottom,id, wnd);
        int len = strlen(g_strDesc);

        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf, (void*)lpString, c * 2 + 1);
        LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)c * 2 + 1, (LPARAM)0);
    }


    return nRet;
}

BOOL WINAPI MyExtTextOutA(_In_ HDC hdc, _In_ int X, _In_ int Y, _In_ UINT fuOptions, _In_ const RECT *lprc,
    _In_ LPCSTR lpString, _In_ UINT cbCount, _In_ const INT *lpDx)
{
    int nRet = 0;

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
    nRet = pExtTextOutA(hdc, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);

    HWND hWnd = WindowFromDC(hdc);
    CWnd *wnd = CWnd::FromHandle(hWnd);
    if (NULL != wnd)
    {
        id = wnd->GetDlgCtrlID();
        wnd->GetWindowRect(&rect);
        if (rect.right - rect.left == 0 || rect.bottom - rect.top == 0)
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

        if (1)
        {
            memset(g_strDesc, 0x0, 512);
            sprintf(g_strDesc, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%08d", X, Y, prc.left, prc.top, prc.right, prc.bottom,
                rect.left, rect.top, rect.right, rect.bottom, id);
            int len = strlen(g_strDesc);

            LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)g_len, (LPARAM)len);

            memset((void*)g_pShareBuf, 0x0, MAX_LEN);
            g_len = 0;
        }
    }
    return nRet;
}

__gdi_entry BOOL WINAPI MyExtTextOutW(_In_ HDC hdc, _In_ int X, _In_ int Y, _In_ UINT fuOptions, __in const RECT *lprc,
    _In_ LPCTSTR lpString, _In_ UINT cbCount, _In_ const INT *lpDx)
{
    int nRet = 0;

    CDC* pDC;
    HDC hDC;
    RECT rect;
    RECT prc;
    BOOL bSendMsg = TRUE;
    int len = 0;
    int id = 0;
    rect.left = 0;
    rect.right = 0;
    rect.top = 0;
    rect.bottom = 0;
    prc = rect;
    nRet = pExtTextOutW(hdc, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);

    HWND hWnd = WindowFromDC(hdc);
    CWnd *wnd = CWnd::FromHandle(hWnd);
    if (NULL != wnd)
    {
        id = wnd->GetDlgCtrlID();
        wnd->GetWindowRect(&rect);
        if (rect.right - rect.left == 0 || rect.bottom - rect.top == 0)
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
        if ( /*(fuOptions == 4096 || fuOptions == 16) &&*/ bSendMsg)
        {
            memset(g_strDesc, 0x0, 512);
            sprintf(g_strDesc, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,4_%#X", X, Y, prc.left, prc.top, prc.right, prc.bottom,
                rect.left, rect.top, rect.right, rect.bottom, wnd);
            int len = strlen(g_strDesc);

            LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)g_len, (LPARAM)len);
            memset((void*)g_pShareBuf, 0x0, MAX_LEN);
            g_len = 0;
        }
    }
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
    int nRet = 0;
    nRet = ::GetCharacterPlacementW(hdc, lpString, nCount, nMaxExtent, lpResults, dwFlags);;
    return nRet;
}


/*__checkReturn*/ HRESULT MyScriptStringAnalyse(
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
    int nRet = 0;
    //if (g_len == 0)
    {
        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf, (void*)pString, cString * 2 + 1);
        g_len = cString;
        LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 5, (WPARAM)g_len, (LPARAM)0);
    }
    nRet = pScriptStringAnalyse(hdc, pString, cString, cGlyphs, iCharset, dwFlags, iReqWidth,
        psControl, psState, piDx, pTabdef, pbInClass, pssa);
    return nRet;
}

HRESULT MyScriptIsComplex(
    _In_ const WCHAR *pwcInChars,
    _In_       int   cInChars,
    _In_       DWORD dwFlags
)
{
    int nRet = 0;
    nRet = ::ScriptIsComplex(pwcInChars, cInChars, dwFlags);
    return nRet;
}

_Success_(return) int  WINAPI MyDrawTextA(
    _In_ HDC hdc,
    _When_((format & DT_MODIFYSTRING), _At_((LPSTR)lpchText, _Inout_grows_updates_bypassable_or_z_(cchText, 4)))
    _When_((!(format & DT_MODIFYSTRING)), _In_bypassable_reads_or_z_(cchText))
    LPCSTR lpchText,
    _In_ int cchText,
    _Inout_ LPRECT lprc,
    _In_ UINT format)
{
    if (cchText > 0 )
    {
        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf, (void*)lpchText, cchText );
        g_len = cchText;
        LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)g_len, (LPARAM)0);
    }
    return pDrawTextA(hdc, lpchText, cchText, lprc, format);
}

_Success_(return) int WINAPI MyDrawTextW(_In_ HDC hdc,
    _When_((format & DT_MODIFYSTRING), _At_((LPWSTR)lpchText, _Inout_grows_updates_bypassable_or_z_(cchText, 4)))
    _When_((!(format & DT_MODIFYSTRING)), _In_bypassable_reads_or_z_(cchText))
    LPCWSTR lpchText,
    _In_ int cchText,
    _Inout_ LPRECT lprc,
    _In_ UINT format)
{
    int nRet = 0;
    if (cchText > 0)
    {
        CDC* pDC;
        HDC hDC;
        RECT rect;
        BOOL bSendMsg = TRUE;
        int id = 0;
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
            id = wnd->GetDlgCtrlID();
            
            CString str;
            wnd->GetWindowTextW(str);
            CString str1;
            CDialog* dlg = dynamic_cast<CDialog*>(wnd);
            HWND handle = wnd->GetSafeHwnd();
            if (rect.right - rect.left == 0 || rect.bottom - rect.top == 0)
            {
                bSendMsg = FALSE;
            }
        }
        else
            bSendMsg = FALSE;

        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf, (void*)lpchText, cchText * 2 + 1);
        g_len = cchText;

        memset(g_strDesc, 0x0, 512);
        sprintf(g_strDesc, "%d,%d,%d,%d,%d,%d,%d,%d,1_%#X", lprc->left, lprc->top, lprc->right, lprc->bottom,
            rect.left, rect.top, rect.right, rect.bottom, wnd);
        int len = strlen(g_strDesc);

        LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)g_len, (LPARAM)len);
    }
    

    nRet = pDrawTextW(hdc, lpchText, cchText, lprc, format);
    return nRet;
}


int WINAPI  MyDrawTextExA(__in HDC hdc, __inout_ecount_opt(cchText) LPSTR lpchText, __in int cchText, __inout LPRECT lprc,
    __in UINT format, __in_opt LPDRAWTEXTPARAMS lpdtp)
{
    int nRet = 0;

    nRet = pDrawTextExA(hdc, lpchText, cchText, lprc, format, lpdtp);
    if (cchText > 0 )
    {
        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf, (void*)lpchText, cchText);
        g_len = cchText;

        //memset(g_strDesc, 0x0 , 512);
        //sprintf(g_strDesc, "%d,%d,%d,%d,%d,%d,%d,%d",lprc->left, lprc->top, lprc->right,lprc->bottom,
        //    rect.left,rect.top, rect.right,rect.bottom);
        int len = strlen(g_strDesc);
        LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)cchText, (LPARAM)len);
    }
    return nRet;
}

int WINAPI MyDrawTextExW(__in HDC hdc, __inout_ecount_opt(cchText) LPWSTR lpchText, __in int cchText, __inout LPRECT lprc,
    __in UINT format, __in_opt LPDRAWTEXTPARAMS lpdtp)
{
    int nRet = 0;
    nRet = pDrawTextExW(hdc, lpchText, cchText, lprc, format, lpdtp);

    if (cchText > 0)
    {
        CDC* pDC;
        HDC hDC;
        RECT rect;
        BOOL bSendMsg = TRUE;
        int id = 0;
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
            id = wnd->GetDlgCtrlID();
            if (rect.right - rect.left == 0 || rect.bottom - rect.top == 0)
            {
                bSendMsg = FALSE;
            }
        }
        else
            bSendMsg = FALSE;

        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf, (void*)lpchText, cchText * 2 + 1);
        g_len = cchText;

        memset(g_strDesc, 0x0, 512);
        sprintf(g_strDesc, "%d,%d,%d,%d,%d,%d,%d,%d,2_%d_%#X", lprc->left, lprc->top, lprc->right, lprc->bottom,
            rect.left, rect.top, rect.right, rect.bottom,id, wnd);
        int len = strlen(g_strDesc);
        LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)cchText, (LPARAM)len);
    }
    return nRet;
}


int fillBuff(CImage &image, BYTE* pByte)
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

    LARGE_INTEGER seekPos = { 0 };
    ULARGE_INTEGER imageSize;
    hr = stream->Seek(seekPos, STREAM_SEEK_CUR, &imageSize);

    //BYTE* buffer = new BYTE[imageSize.LowPart];

    hr = stream->Seek(seekPos, STREAM_SEEK_SET, 0);
    hr = stream->Read(pByte, imageSize.LowPart, 0);

    // Fill buffer from stream
    hr = stream->Seek(seekPos, STREAM_SEEK_SET, 0);
    hr = stream->Read(pByte, imageSize.LowPart, 0);

    if (NULL != mem)
    {
        GlobalFree(mem);
        mem = NULL;
    }
    return imageSize.LowPart;
}



__gdi_entry BOOL  WINAPI  MyBitBlt(__in HDC hdc, __in int x, __in int y,
    __in int cx, __in int cy, __in_opt HDC hdcSrc,
    __in int x1, __in int y1, __in DWORD rop)
{
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
        if (rect.right - rect.left == 0 || rect.bottom - rect.top == 0)
        {
            bSendMsg = FALSE;
        }
    }
    else
        bSendMsg = FALSE;

    CImage image;
    HBITMAP hbmp = (HBITMAP)bitmap->GetSafeHandle();
    image.Attach(hbmp);
    int iSize = fillBuff(image, g_BitBuffer);
    if (iSize > 0 && bSendMsg)
    {
        // 坐标
        memset(g_strDesc, 0x0, 512);
        sprintf(g_strDesc, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", x, y, cx, cy, x1, y1,
            rect.left, rect.top, rect.right, rect.bottom, id);
        int len = strlen(g_strDesc);
        LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 2, (WPARAM)iSize, (LPARAM)len);
    }

    BOOL b = pBitBlt(hdc, x, y, cx, cy, hdcSrc, x1, y1, rop);
    return TRUE;
}

__gdi_entry BOOL  WINAPI  MyStretchBlt(__in HDC hdcDest, __in int xDest, __in int yDest,
    __in int wDest, __in int hDest, __in_opt HDC hdcSrc,
    __in int xSrc, __in int ySrc, __in int wSrc, __in int hSrc,
    __in DWORD rop)
{
    BOOL b = StretchBlt(hdcDest, xDest, yDest, wDest, hDest, hdcSrc, xSrc, ySrc, wSrc, hSrc, rop);

    return b;
}


HBITMAP MyCreateBitmap(__in int nWidth, __in int nHeight, __in UINT nPlanes, __in UINT nBitCount, __in_opt CONST VOID *lpBits)
{
    int nRet = 0;

    HookOffList();
    HBITMAP rest = ::CreateBitmap(nWidth, nHeight, nPlanes, nBitCount, lpBits);

    CImage image;
    image.Attach(rest);
    int iSize = fillBuff(image, g_BitBuffer);

    ::SendMessage(g_hWndMsg, WM_USER + 2, (WPARAM)iSize, (LPARAM)0);

    return rest;
}

HBITMAP WINAPI MyCreateCompatibleBitmap(__in HDC hdc, __in int cx, __in int cy)
{
    int nRet = 0;

    HBITMAP rest = ::CreateCompatibleBitmap(hdc, cx, cy);
    ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)7, (LPARAM)0);
    return rest;
}


int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT  num = 0;
    UINT  size = 0;
    ImageCodecInfo* pImageCodecInfo = NULL;
    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;

    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

int fillBuff(GpImage &img, BYTE* pByte)
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
    GpStatus status = DllExports::GdipSaveImageToStream(&img, stream, &sid, NULL);


    LARGE_INTEGER seekPos = { 0 };
    ULARGE_INTEGER imageSize;
    hr = stream->Seek(seekPos, STREAM_SEEK_CUR, &imageSize);
    BYTE* buffer = new BYTE[imageSize.LowPart];

    hr = stream->Seek(seekPos, STREAM_SEEK_SET, 0);
    hr = stream->Read(pByte, imageSize.LowPart, 0);

    return safeSize;
}


GpStatus WINGDIPAPI MyGdipDrawImage(GpGraphics *graphics, GpImage *image, REAL x, REAL y)
{
    int nRet = 0;
    LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 3, (WPARAM)1, (LPARAM)0);
    GpStatus status = DllExports::GdipDrawImage(graphics, image, x, y);
    return status;
}


GpStatus WINGDIPAPI MyGdipDrawImageI(GpGraphics *graphics, GpImage *image, INT x, INT y)
{
    int nRet = 0;
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}
    LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 3, (WPARAM)5, (LPARAM)0);
    GpStatus status = DllExports::GdipDrawImageI(graphics, image, x, y);
    return status;
}

GpStatus WINGDIPAPI MyGdipDrawImageRectI(GpGraphics *graphics, GpImage *image, INT x, INT y,
    INT width, INT height)
{
    int nRet = 0;
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}

    LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 3, (WPARAM)2, (LPARAM)0);
    GpStatus status = DllExports::GdipDrawImageRectI(graphics, image, x, y, width, height);

    return status;
}

GpStatus WINGDIPAPI MyGdipDrawImagePointRect(GpGraphics *graphics, GpImage *image, REAL x,
    REAL y, REAL srcx, REAL srcy, REAL srcwidth,
    REAL srcheight, GpUnit srcUnit)
{
    int nRet = 0;
    //int iTrs = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_pShareBuf, 0x0, MAX_LEN);
    //    memcpy((void*)g_pShareBuf,(void*)pwcInChars,cInChars);
    //}

    LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 3, (WPARAM)3, (LPARAM)0);
    GpStatus status = DllExports::GdipDrawImagePointRect(graphics, image, x, y, srcx, srcy, srcwidth, srcheight, srcUnit);
    return status;
}


GpStatus WINGDIPAPI MyGdipDrawImageRectRect(GpGraphics *graphics, GpImage *image, REAL dstx, REAL dsty, REAL dstwidth, REAL dstheight, REAL srcx, REAL srcy, REAL srcwidth, REAL srcheight,
    GpUnit srcUnit, GDIPCONST GpImageAttributes* imageAttributes, DrawImageAbort callback, VOID * callbackData)
{
    int nRet = 0;


    CDC* pDC;
    HDC hDC;
    RECT rect;
    BOOL bSendMsg = TRUE;
    GpStatus status;
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

    size_t size = fillBuff(*image, g_BitBuffer);
    if (size > 0)
    {
        // 坐标
        memset(g_strDesc, 0x0, 512);
        sprintf(g_strDesc, "%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%d,%d", dstx, dsty, dstwidth, dstheight,
            srcx, srcy, srcwidth, srcheight, rect.left, rect.right, rect.top, rect.bottom);
        int len = strlen(g_strDesc);
        LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 3, (WPARAM)size, (LPARAM)len);
    }
    status = pGdipDrawImageRectRect(graphics, image, dstx, dsty, dstwidth, dstheight,
        srcx, srcy, srcwidth, srcheight, srcUnit, imageAttributes, callback, callbackData);

    return status;
}

BOOL MyImageList_DrawIndirect(IMAGELISTDRAWPARAMS* pimldp)
{
    int nRet = 0;
    LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 3, (WPARAM)20, (LPARAM)0);
    bool b = ImageList_DrawIndirect(pimldp);
    return b;
}


GpStatus WINGDIPAPI MyGdipDrawString(GpGraphics*graphics, GDIPCONST WCHAR *string, INT length, GDIPCONST GpFont *font,
    GDIPCONST RectF *layoutRect, GDIPCONST GpStringFormat *stringFormat, GDIPCONST GpBrush *brush)
{
    int nRet = 0;
    CDC* pDC;
    HDC hDC;
    RECT rect;
    BOOL bSendMsg = TRUE;
    GpStatus status;
    rect.left = 0;
    rect.right = 0;
    rect.top = 0;
    rect.bottom = 0;

    RectF rectLay;
    rectLay.X = 0;
    rectLay.Y = 0;
    rectLay.Width = 0;
    rectLay.Height = 0;
    status = pGdipDrawString(graphics, string, length, font, layoutRect, stringFormat, brush);
    if (length > 0)
    {
        if (NULL != layoutRect)
        {
            rectLay = *layoutRect;
        }
        int iTrs = 0;
        if (NULL != g_pShareBuf)
        {
            memset((void*)g_pShareBuf, 0x0, MAX_LEN);
            memcpy((void*)g_pShareBuf, (void*)string, length * 2 + 1);
        }
        if (S_OK == DllExports::GdipGetDC(graphics, &hDC))
        {
            pDC = CDC::FromHandle(hDC);
            HWND hWnd = WindowFromDC(hDC);
            CWnd *wnd = CWnd::FromHandle(hWnd);
            int id = 0;
            if (NULL != wnd)
            {
                wnd->GetClientRect(&rect);
                id = wnd->GetDlgCtrlID();
            }
            else
                bSendMsg = FALSE;

            memset(g_strDesc, 0x0, 512);
            sprintf(g_strDesc, "%f,%f,%f,%f,%d,%d,%d,%d,5_%d_%#X", rectLay.X, rectLay.Y, rectLay.Width, rectLay.Height,
                rect.left, rect.top, rect.right, rect.bottom,id, wnd);
            int len = strlen(g_strDesc);

            LRESULT rest = ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)len, (LPARAM)0);
            DllExports::GdipReleaseDC(graphics, hDC);
        }
    }

    return status;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI MySetWindowTextA(_In_ HWND hWnd, _In_opt_ LPCSTR lpString)
{
    int iRet = 0;
    CHAR* p = (CHAR*)lpString;
    int len = strlen(lpString);
    if (0 < len)
    {
        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf, (void*)lpString, len * 2 + 1);
        ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)len, (LPARAM)0);
    }
    iRet = pSetWindowTextA(hWnd, lpString);
    return iRet;
}

BOOL WINAPI MySetWindowTextW(__in HWND hWnd, __in_opt LPCWSTR lpString)
{
    //CDHtmlDialog::accLocation();
    //    Navigate;
    //    WebBrowser;
    //    CWebBrowser2;
    //    IWebBrowser2::IWebBrowser;
    //    CHtmlView
    int iRet = 0;
    WCHAR* p = (WCHAR*)lpString;
    int len = wcslen(lpString);
    if (NULL != g_pShareBuf)
    {
        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf, (void*)lpString, len * 2 + 1);
        ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)len, (LPARAM)0);
    }
    iRet = pSetWindowTextW(hWnd, lpString);
    return iRet;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////

int WSAAPI Mysend(IN SOCKET s, __in_bcount(len) const char FAR * buf, IN int len, IN int flags)
{
    int iRet = 0;
    //if (NULL != g_pShareBuf)
    //{
    //    memset((void*)g_strSocket, 0x0, BUF_SOCKET);
    //    memcpy((void*)g_strSocket,(void*)buf,iRet);
    //}
    //LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+5,(WPARAM)5, (LPARAM)len );

    std::fstream file("E:\\code\\vs_test\\testhook\\Debug\\send.txt", std::ios::out | std::ios::app);
    if (file.is_open())
    {
        file << "send: " << buf << std::endl;
    }
    file.close();

    iRet = pSend(s, buf, len, flags);
    return iRet;
}

int PASCAL FAR MyRecv(IN SOCKET s, __out_bcount_part(len, return) __out_data_source(NETWORK) char FAR * buf,
    IN int len, IN int flags)
{
    int iRet = 0;
    iRet = pRecv(s, buf, len, flags);

    //if (iRet >= 0)
    //{
    //    memset((void*)g_strSocket, 0x0, BUF_SOCKET);
    //    memcpy((void*)g_strSocket,(void*)buf,iRet);
    //    LRESULT rest = ::SendMessage(g_hWndMsg,WM_USER+4,(WPARAM)iRet, (LPARAM)s );
    //}

    std::fstream file("E:\\code\\vs_test\\testhook\\Debug\\Recv.txt", std::ios::out | std::ios::app);
    if (file.is_open())
    {
        file << "recv: " << buf << std::endl;
    }
    file.close();

    return iRet;

}


VOID WINAPI MyMouse_event(
    _In_ DWORD dwFlags,
    _In_ DWORD dx,
    _In_ DWORD dy,
    _In_ DWORD dwData,
    _In_ ULONG_PTR dwExtraInfo)
{
    int i = 0;
    pMouse_event(dwFlags, dx, dy, dwData, dwExtraInfo);
}


BOOL WINAPI MyGetMessageW(
    _Out_ LPMSG lpMsg,
    _In_opt_ HWND hWnd,
    _In_ UINT wMsgFilterMin,
    _In_ UINT wMsgFilterMax)
{
    if (lpMsg->hwnd == g_hWndMsg)
    {
        if (WM_CLOSE == lpMsg->message || WM_QUIT == lpMsg->message || WM_MOVE == lpMsg->message 
            || SC_SIZE == lpMsg->message || WM_DESTROY == lpMsg->message || WM_SYSCOMMAND == lpMsg->message)
        //if (lpMsg->message == WM_LBUTTONDOWN || WM_MOVE == lpMsg->message)
        {
            CString str;
            str.Format(_T("%d"), lpMsg->message);
            int i = 0;
            //::MessageBox(0, _T("扫描目标进程失败!"), str, MB_OK);
            return TRUE;
        }
        
    }
    return pGetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

LRESULT WINAPI MyDefWindowProcW(
    _In_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    if (hWnd == g_hWndMsg )
    {
        if (WM_LBUTTONDOWN == Msg)
        {
            int i = 0;
        }
        if (WM_SYSCOMMAND == Msg || WM_COMMAND == Msg )
        {
            int i = 0;
            
        }
        if (WM_CLOSE == Msg || WM_QUIT == Msg || WM_MOVE == Msg
            || SC_SIZE == Msg || WM_DESTROY == Msg || WM_SYSCOMMAND == Msg)
        {
            //::MessageBox(0, _T("扫描目标进程失败!"), _T(""), MB_OK);
            return 0;
        }
    }
    return pDefWindowProcW(hWnd, Msg, wParam, lParam);
}

BOOL WINAPI MyDestroyWindow(_In_ HWND hWnd)
{
    if (hWnd == g_hWndMsg)
    {
        //::MessageBox(0, _T("扫描目标进程失败!"), _T(""), MB_OK);
        ::SendMessage(g_hWndMsg, WM_USER + 99, (WPARAM)0, (LPARAM)0);
    }
    return pDestroyWindow(hWnd);
}

BOOL WINAPI MyInternetReadFile(
    _In_ HINTERNET hFile,
    _Out_writes_bytes_(dwNumberOfBytesToRead) __out_data_source(NETWORK) LPVOID lpBuffer,
    _In_ DWORD dwNumberOfBytesToRead,
    _Out_ LPDWORD lpdwNumberOfBytesRead
)
{
    if (dwNumberOfBytesToRead > 0)
    {
        int len = 0;
        memset((void*)g_pShareBuf, 0x0, MAX_LEN);
        memcpy((void*)g_pShareBuf, (void*)lpBuffer, dwNumberOfBytesToRead);
        len = dwNumberOfBytesToRead;
        ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)len, (LPARAM)0);
    }

    return pInternetReadFile(hFile, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
}


//LRESULT WINAPI MyCallNextHookEx(_In_opt_ HHOOK hhk, _In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
//{
//        if (wParam == WM_NCLBUTTONDOWN)
//        {
//            MessageBox(NULL, _T("sdasdasdad"), _T("error"), MB_OK);
//        }
//    return pCallNextHookEx(hhk, nCode, wParam, lParam);
//}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 初始化共享内存
int initShareMemory()
{
    g_hMapMem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szName);
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


    // 位图共享内存
    g_hMapMem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szNameBitMap);
    if (g_hMapMem == NULL)
    {
        return -1;
    }
    g_BitBuffer = (BYTE*)MapViewOfFile(g_hMapMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (NULL == g_BitBuffer)
    {
        CloseHandle(g_hMapMem);
        g_hMapMem = NULL;
        return -2;
    }

    // 位图坐标信息
    g_hMapMem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szNameDataDesc);
    if (g_hMapMem == NULL)
    {
        return -1;
    }
    g_strDesc = (char*)MapViewOfFile(g_hMapMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (NULL == g_strDesc)
    {
        CloseHandle(g_hMapMem);
        g_hMapMem = NULL;
        return -2;
    }

    // socket数据信息
    g_hMapMem = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szNameDataSocket);
    if (g_hMapMem == NULL)
    {
        return -1;
    }
    g_strSocket = (char*)MapViewOfFile(g_hMapMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (NULL == g_strSocket)
    {
        CloseHandle(g_hMapMem);
        g_hMapMem = NULL;
        return -2;
    }

    return 0;
}

int closeShareMemery()
{
    if (NULL != g_pShareBuf)
        UnmapViewOfFile(g_pShareBuf);
    if (NULL != g_hMapMem)
        CloseHandle(g_hMapMem);
    return 0;
}

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
            //MessageBox(NULL, _T("sdasdasdad"), _T("error"), MB_OK);
            ::SendMessage(g_hWndMsg, WM_USER + 1, (WPARAM)pvarRole.lVal, (LPARAM)0);
            //return TRUE;
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

// 获取主窗口
extern "C" __declspec(dllexport) HWND GetMainWnd()
{
    return g_hWnd;
}

BEGIN_MESSAGE_MAP(ChookdetoursApp, CWinApp)
END_MESSAGE_MAP()


// ChookdetoursApp 构造
ChookdetoursApp::ChookdetoursApp()
{
    // TODO: 在此处添加构造代码，
    // 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 ChookdetoursApp 对象

ChookdetoursApp theApp;


// ChookdetoursApp 初始化

BOOL ChookdetoursApp::InitInstance()
{
    CWinApp::InitInstance();

    //afxAmbientActCtx = FALSE;

    //GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);

    hInst = AfxGetInstanceHandle();
    DWORD dwPid = ::GetCurrentProcessId();
    hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, 0, dwPid);
    // 获取主窗口
    {
        g_hWnd = NULL;
        CWinApp* app = AfxGetApp();
        if (NULL != app)
        {
            CWnd *wnd = app->GetMainWnd();
            if (NULL != wnd)
            {
                g_hWnd = wnd->GetSafeHwnd();
            }
        }
    }

    initShareMemory();
    intiInfo();
    ::InitializeCriticalSection(&g_cs_test);
    g_len = 0;
    return TRUE;
}


int ChookdetoursApp::ExitInstance()
{
    HookOffList();
    closeShareMemery();
    return CWinApp::ExitInstance();
}

