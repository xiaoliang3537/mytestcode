// Listens.h : header file

#if !defined(AFX_LISTENS_H__B7C54BD1_A555_11D0_8996_00AA00B92B2E__INCLUDED_)
#define AFX_LISTENS_H__B7C54BD1_A555_11D0_8996_00AA00B92B2E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// 基于win32的异步socket
class CListenSocketEx
{
public:
    CListenSocketEx();
    ~CListenSocketEx();

    int startListen();
public:
    static DWORD WINAPI RecvThreadProc(LPVOID lpParam);
    void OnAccept();
    CFile m_file;
protected:
    SOCKET m_socket;
    WSAEVENT m_socketEvent;
    HANDLE m_stopEvent;
    HANDLE m_thread;
};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTENS_H__B7C54BD1_A555_11D0_8996_00AA00B92B2E__INCLUDED_)
