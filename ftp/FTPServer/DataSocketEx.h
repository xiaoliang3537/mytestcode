#pragma once
#include <afxwin.h>
#include <afxsock.h>

class CConnectSocketEx;

#define XFERMODE_IDLE	0
#define XFERMODE_LIST	1
#define XFERMODE_SEND	2
#define XFERMODE_RECEIVE 3
#define XFERMODE_ERROR	4

class CDataSocketEx 
{
public:
    BOOL m_bTerminate;
    HANDLE m_hThread;
    SOCKET m_socket;
    WSAEVENT m_scoketEvent;
public:
    CDataSocketEx(CConnectSocketEx *pSocket, int nTransferType = 0);
	virtual ~CDataSocketEx();
public:
    int Create();
    void Close() ;
    void Attach(SOCKET sock);
    int GetSockName(CString &strIP, UINT &nPort);
	void SetRestartOffset(DWORD dwOffset);
	void SetTransferType(int nType, BOOL bWaitForAccept = FALSE);
	void SetData(LPCTSTR lpszData);
    FILE* m_f;
	int GetStatus();
    int Send(char* ch, DWORD len);
    
public:
    void OnSend(int nErrorCode);
    void OnConnect(int nErrorCode);
    void OnClose(int nErrorCode);
    void OnReceive(int nErrorCode);
    int OnAccept(int nErrorCode);

    static DWORD WINAPI RecvThreadProc(LPVOID lpParam);
protected:
	DWORD m_dwRestartOffset;
	BOOL m_bConnected;
	BOOL m_bInitialized;
	int Receive();
	BOOL PrepareReceiveFile(LPCTSTR lpszFilename);
	BOOL PrepareSendFile(LPCTSTR lpszFilename);
	DWORD m_nTotalBytesTransfered;
	DWORD m_nTotalBytesReceive;
	DWORD m_nTotalBytesSend;
	int m_nTransferType;

	CString m_strData;
	int m_nStatus;
	CConnectSocketEx *m_pConnectSocket;
};
