#pragma once

#include <afxwin.h>
#include <afxsock.h>

class CDataSocketEx;

class CConnectSocketEx 
{
	enum // Token ID's
	{
		TOK_ABOR, TOK_BYE, TOK_CDUP, TOK_CWD,
		TOK_DELE, TOK_DIR, TOK_HELP, TOK_LIST,
		TOK_MKD, TOK_NOOP, TOK_PASS, TOK_PASV, 
		TOK_PORT, TOK_PWD, TOK_QUIT, TOK_REST,
		TOK_RETR, TOK_RMD, TOK_RNFR, TOK_RNTO, 
		TOK_SIZE, TOK_STOR, TOK_SYST, TOK_TYPE, 
		TOK_USER, TOK_ERROR,
	};

public:
	int m_bPassiveMode;
	int m_nRemotePort;
	CString m_strRemoteHost;
	CDataSocketEx *m_pDataSocket;
    SOCKET m_socket;
    WSAEVENT m_scoketEvent;
    BOOL m_bTerminate;
    HANDLE m_hThread;
	struct CFTPCommand
	{
		int m_nTokenID;
		char *m_pszName;
		BOOL m_bHasArguments;
		char *m_pszDescription;
	};

// Attributes
public:
    void Attach(SOCKET sock);
    int GetSockName(CString &strIP, UINT &nPort);
	BOOL HasConnectionDropped(void);
	BOOL SendResponse(const char* pstrFormat, ...);
	void FireStatusMessage(const char*, int nType);
	BOOL GetRxCommand(CString &command, CString &args);
	BOOL CreateDataConnection(int nTransferType, LPCTSTR lpszData);
	void DestroyDataConnection();
    void Close() ;
    static DWORD WINAPI RecvThreadProc(LPVOID lpParam);
    bool GetPeerName(CString &strPeerAddress, UINT &nPeerPort);
// Operations
public:
    CConnectSocketEx();
	virtual ~CConnectSocketEx();

	void ParseCommand();

// Overrides
public:
	CWinThread* m_pThread;

	BOOL m_bLoggedon;
	CString m_strUserName;
	
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectSocket)
	public:
	virtual void OnClose(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CConnectSocket)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
protected:
	CStringList m_strCommands;
	void GetRxLine();
	BOOL m_bRenameFile;
	DWORD m_dwRestartOffset;
	CString m_strRenameFile;
	CString m_RxBuffer;
	CString m_strCurrentDir;

};
