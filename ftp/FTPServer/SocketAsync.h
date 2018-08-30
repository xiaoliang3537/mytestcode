#pragma once
#include <afxwin.h>
#include <afxsock.h>

#define XFERMODE_IDLE	0
#define XFERMODE_LIST	1
#define XFERMODE_SEND	2
#define XFERMODE_RECEIVE 3
#define XFERMODE_ERROR	4

class CSocketAsync 
{
public:
    BOOL m_bTerminate;
    HANDLE m_hThread;
    SOCKET m_socket;
    WSAEVENT m_scoketEvent;
    FILE* m_f;
    int m_nTimes;
public:
    CSocketAsync();
	virtual ~CSocketAsync();
public:
    int Create(long lNetworkEvents = FD_ACCEPT | FD_READ | FD_WRITE| FD_CLOSE);
    void Close() ;
    void Attach(SOCKET sock);
    int GetSockName(CString &strIP, UINT &nPort);
	int GetStatus();
    int Send(char* ch, DWORD len);
    int SetTimeOut(int n = -1) { m_nTimes = n; }
public:
    int OnSend(int nErrorCode);
    int OnClose(int nErrorCode);
    int OnReceive(int nErrorCode);
    int OnAccept(int nErrorCode);

    static DWORD WINAPI RecvThreadProc(LPVOID lpParam);
protected:
	BOOL m_bConnected;
	BOOL m_bInitialized;
	int m_nStatus;
};
