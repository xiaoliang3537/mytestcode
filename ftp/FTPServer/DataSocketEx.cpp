#include "stdafx.h"
#include "resource.h"
#include "DataSocketEx.h"
#include "ConnectSocketEx.h"
#include <afxpriv.h>

#include <winsock2.h>
#include <Windows.h>
#include <iostream>
#pragma comment(lib,"ws2_32.lib")
using std::cout;
using std::cin;
using std::endl;
using std::ends;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define PACKET_SIZE 4096

/********************************************************************/
/*																	*/
/* Function name : CDataSocket::CDataSocket							*/
/* Description   : Constructor										*/
/*																	*/
/********************************************************************/
CDataSocketEx::CDataSocketEx(CConnectSocketEx *pSocket, int nTransferType)
{
    m_nTransferType = nTransferType;
    m_pConnectSocket = pSocket;
    m_nStatus = XFERMODE_IDLE;
    m_strData = "";
    m_bConnected = FALSE;
    m_dwRestartOffset = 0;
    m_bInitialized = FALSE;
    m_hThread = NULL;
    m_bTerminate = FALSE;
    m_f = NULL;
}


CDataSocketEx::~CDataSocketEx()
{
    m_bConnected = FALSE;
    TRACE0("CDataSocket destroyed.\n");
}


void CDataSocketEx::OnSend(int nErrorCode)
{
    switch (m_nStatus)
    {
    case XFERMODE_LIST:
    {
        while (m_nTotalBytesTransfered < m_nTotalBytesSend)
        {
            DWORD dwRead;
            int dwBytes;

            CString strData;

            dwRead = m_strData.GetLength();

            if (dwRead <= PACKET_SIZE)
            {
                strData = m_strData;
            }
            else
            {
                strData = m_strData.Left(PACKET_SIZE);
                dwRead = strData.GetLength();
            }
            std::string str = TCharToString(strData);
            if ((dwBytes = Send((char*)str.c_str(), str.length())) == SOCKET_ERROR)
            {
                if (GetLastError() == WSAEWOULDBLOCK)
                {
                    Sleep(0);
                    break;
                }
                else
                {
                    TCHAR szError[256];
                    wsprintf(szError, _T("Server Socket failed to send: %d"), GetLastError());

                    // close the data connection.
                    Close();

                    m_nTotalBytesSend = 0;
                    m_nTotalBytesTransfered = 0;

                    // change status
                    m_nStatus = XFERMODE_IDLE;

                    m_pConnectSocket->SendResponse("426 Connection closed; transfer aborted.");

                    // destroy this socket
                    //AfxGetThread()->PostThreadMessage(WM_THREADMSG, 0, 0);
                }
            }
            else
            {
                m_nTotalBytesTransfered += dwBytes;

                m_strData = m_strData.Mid(dwBytes);

                //((CConnectThread *)AfxGetThread())->IncSentBytes(dwBytes);
            }
        }
        if (m_nTotalBytesTransfered == m_nTotalBytesSend)
        {
            // close the data connection.
            Close();

            m_nTotalBytesSend = 0;
            m_nTotalBytesTransfered = 0;

            // change status
            m_nStatus = XFERMODE_IDLE;

            // tell the client the transfer is complete.
            m_pConnectSocket->SendResponse("226 Transfer complete");
            // destroy this socket
            //AfxGetThread()->PostThreadMessage(WM_THREADMSG, 0, 0);
        }
        break;
    }
    case XFERMODE_SEND:
    {
        while (m_nTotalBytesTransfered < m_nTotalBytesSend)
        {
            // allocate space to store data
            byte data[PACKET_SIZE];
            fseek(m_f, m_nTotalBytesTransfered, SEEK_SET);
            DWORD dwRead = fread(data, 1, PACKET_SIZE, m_f);
            int dwBytes;

            if ((dwBytes = Send((char*)data, dwRead)) == SOCKET_ERROR)
            {
                if (GetLastError() == WSAEWOULDBLOCK)
                {
                    Sleep(0);
                    break;
                }
                else
                {
                    TCHAR szError[256];
                    wsprintf(szError, _T("Server Socket failed to send: %d"), GetLastError());

                    m_nTotalBytesSend = 0;
                    m_nTotalBytesTransfered = 0;

                    // change status
                    m_nStatus = XFERMODE_IDLE;

                    m_pConnectSocket->SendResponse("426 Connection closed; transfer aborted.");

                    // close the data connection.
                    Close();
                    // destroy this socket
                    //AfxGetThread()->PostThreadMessage(WM_THREADMSG, 0, 0);

                    // download failed
                    //((CConnectThread *)AfxGetThread())->UpdateStatistic(FTPSTAT_DOWNLOADFAILED);
                }
            }
            else
            {
                m_nTotalBytesTransfered += dwBytes;

                //((CConnectThread *)AfxGetThread())->IncSentBytes(dwBytes);
            }
        }
        if (m_nTotalBytesTransfered == m_nTotalBytesSend)
        {
            m_nTotalBytesSend = 0;
            m_nTotalBytesTransfered = 0;

            // change status
            m_nStatus = XFERMODE_IDLE;

            // tell the client the transfer is complete.
            m_pConnectSocket->SendResponse("226 Transfer complete");

            // close the data connection.
            Close();
            //// destroy this socket
            //AfxGetThread()->PostThreadMessage(WM_THREADMSG, 0, 0);
            //// download successfull
            //((CConnectThread *)AfxGetThread())->UpdateStatistic(FTPSTAT_DOWNLOADSUCCEEDED);
        }
        break;
    }
    default:
        break;
    }
}


void CDataSocketEx::OnConnect(int nErrorCode)
{
    if (nErrorCode)
    {
        m_nStatus = XFERMODE_ERROR;
        m_pConnectSocket->SendResponse("425 Can't open data connection.");
        // destroy this socket
        //AfxGetThread()->PostThreadMessage(WM_THREADMSG, 0, 0);
    }
    else
    {
        switch (m_nTransferType)
        {
        case 0:	// List Directory
            m_nStatus = XFERMODE_LIST;
            m_bConnected = TRUE;
            OnSend(0);
            break;
        case 1:	// Send File
            if (PrepareSendFile(m_strData))
            {
                m_nStatus = XFERMODE_SEND;
                m_bConnected = TRUE;
            }
            else
            {
                Close();
            }
            break;
        case 2:	// Receive File
            if (PrepareReceiveFile(m_strData))
            {
                m_nStatus = XFERMODE_RECEIVE;
                m_bConnected = TRUE;
            }
            else
            {
                Close();
                m_pConnectSocket->SendResponse("450 can't access file.");
                // destroy this socket
                AfxGetThread()->PostThreadMessage(WM_THREADMSG, 0, 0);
                // upload failed
                //((CConnectThread *)AfxGetThread())->UpdateStatistic(FTPSTAT_UPLOADFAILED);
            }
            break;
        }
    }

}


void CDataSocketEx::OnClose(int nErrorCode)
{
    TRACE0("CDataSocket() OnClose()\n");
    if (m_pConnectSocket)
    {
        if (m_nStatus == XFERMODE_RECEIVE)
        {
            while (Receive() != 0)
            {
                // receive remaining data				
            }
        }
        else
        {
            m_pConnectSocket->SendResponse("426 Connection closed; transfer aborted.");
            // destroy this socket
            //AfxGetThread()->PostThreadMessage(WM_THREADMSG, 0, 0);
            // upload failed
            //((CConnectThread *)AfxGetThread())->UpdateStatistic(FTPSTAT_UPLOADFAILED);
        }
    }
    m_nStatus = XFERMODE_IDLE;
    m_bConnected = FALSE;

}

int CDataSocketEx::OnAccept(int nErrorCode)
{
    return 0;
}


/********************************************************************/
/*																	*/
/* Function name : CDataSocket::GetStatus							*/
/* Description   : Get socket status.								*/
/*																	*/
/********************************************************************/
int CDataSocketEx::GetStatus()
{
    return m_nStatus;
}

int CDataSocketEx::Send(char* ch, DWORD len)
{
    int l = ::send(m_socket, ch, len, 0);
    return l;
}


/********************************************************************/
/*																	*/
/* Function name : CDataSocket::PrepareSendFile						*/
/* Description   : Prepare socket to send a file.					*/
/*																	*/
/********************************************************************/
BOOL CDataSocketEx::PrepareSendFile(LPCTSTR lpszFilename)
{
    if (NULL != m_f)
    {
        fclose(m_f);
    }
    m_f = fopen(TCharToString(m_strData).c_str(), "rb");
    if (NULL == m_f)
    {
        return FALSE;
    }
    fseek(m_f, 0, SEEK_END);
    m_nTotalBytesSend = ftell(m_f);
    if (m_dwRestartOffset < m_nTotalBytesSend)
        m_nTotalBytesTransfered = m_dwRestartOffset;
    else
        m_nTotalBytesTransfered = 0;

    return TRUE;
}


/********************************************************************/
/*																	*/
/* Function name : CDataSocket::PrepareReceiveFile					*/
/* Description   : Prepare socket to receive a file.				*/
/*																	*/
/********************************************************************/
BOOL CDataSocketEx::PrepareReceiveFile(LPCTSTR lpszFilename)
{
    if (NULL != m_f)
    {
        fclose(m_f);
    }
    m_f = fopen(TCharToString(m_strData).c_str(), "wb");
    if (NULL == m_f)
    {
        return FALSE;
    }
    m_nTotalBytesReceive = 0;
    m_nTotalBytesTransfered = 0;

    if (m_dwRestartOffset)
    {
        fseek(m_f, m_dwRestartOffset - 1, SEEK_CUR);
        fputc(0, m_f);
    }
    return TRUE;
}


/********************************************************************/
/*																	*/
/* Function name : CDataSocket::OnReceive							*/
/* Description   : Called by the framework to notify this socket	*/
/*				   that there is data in the buffer that can be		*/
/*				   retrieved by calling the Receive member function.*/
/*																	*/
/********************************************************************/
void CDataSocketEx::OnReceive(int nErrorCode)
{
    //CAsyncSocket::OnReceive(nErrorCode);

    Receive();
}


/********************************************************************/
/*																	*/
/* Function name : CDataSocket::Receive								*/
/* Description   : Receive data from a socket						*/
/*																	*/
/********************************************************************/
int CDataSocketEx::Receive()
{
    int nRead = 0;
    if (m_nStatus == XFERMODE_RECEIVE)
    {
        if (m_f == NULL)
        {
            return FALSE;
        }

        do
        {
            char data[PACKET_SIZE];
            nRead = recv(m_socket, data, PACKET_SIZE, 0);
            if (nRead == 0)
            {
                // tell the client the transfer is complete.
                m_pConnectSocket->SendResponse("226 Transfer complete");
                Close();
                // 发送消息
                sendTargetMessage(_T("ee"), _T("ee"), m_strData);
                return nRead;
            }
            else if (nRead == SOCKET_ERROR)
            {
                if (GetLastError() != WSAEWOULDBLOCK)
                {
                    m_pConnectSocket->SendResponse("426 Connection closed; transfer aborted.");
                    Close();
                    // destroy this socket
                    //AfxGetThread()->PostThreadMessage(WM_THREADMSG, 0, 0);
                    // upload failed
                    //((CConnectThread *)AfxGetThread())->UpdateStatistic(FTPSTAT_UPLOADFAILED);
                }
                return 0;
            }
            else
            {
                TRY
                {
                    fwrite(data,1,nRead, m_f);
                }
                    CATCH_ALL(e)
                {
                    Close();
                    m_pConnectSocket->SendResponse("450 Can't access file.");
                    // destroy this socket
                    AfxGetThread()->PostThreadMessage(WM_THREADMSG, 0, 0);
                    // upload failed
                    //((CConnectThread *)AfxGetThread())->UpdateStatistic(FTPSTAT_UPLOADFAILED);
                    return 0;
                }
                END_CATCH_ALL;
            }
        } while (1);

    }
    return nRead;
}


/********************************************************************/
/*																	*/
/* Function name : CDataSocket::SetData								*/
/* Description   : Set data that is initially send to client.		*/
/*																	*/
/********************************************************************/
void CDataSocketEx::SetData(LPCTSTR lpszData)
{
    m_strData = lpszData;
    m_nTotalBytesSend = m_strData.GetLength();
    m_nTotalBytesTransfered = 0;

    //this->PrepareReceiveFile(lpszData);
}


/********************************************************************/
/*																	*/
/* Function name : CDataSocket::SetTransferType						*/
/* Description   : Set transfer type:								*/
/*				   0 = LIST DIR, 1 = SEND FILE, 2 = RECEIVE FILE	*/
/*																	*/
/********************************************************************/
void CDataSocketEx::SetTransferType(int nType, BOOL bWaitForAccept)
{
    m_nTransferType = nType;

    if (bWaitForAccept && !m_bConnected)
    {
        m_bInitialized = FALSE;
        return;
    }

    if (m_bConnected && m_nTransferType != -1)
        m_pConnectSocket->SendResponse("150 Connection accepted");

    m_bInitialized = TRUE;

    switch (m_nTransferType)
    {
    case 0:	// List Directory
        m_nStatus = XFERMODE_LIST;
        OnSend(0);
        break;
    case 1:	// Send File
        if (PrepareSendFile(m_strData))
        {
            m_nStatus = XFERMODE_SEND;
            m_bConnected = TRUE;
            OnSend(0);
        }
        else
        {
            Close();
        }
        break;
    case 2:	// Receive File
        if (PrepareReceiveFile(m_strData))
        {
            m_nStatus = XFERMODE_RECEIVE;
            m_bConnected = TRUE;
            OnSend(0);
        }
        else
        {
            Close();
            m_pConnectSocket->SendResponse("450 Can't access file.");
            // destroy this socket
            AfxGetThread()->PostThreadMessage(WM_THREADMSG, 0, 0);
            // upload failed
            //((CConnectThread *)AfxGetThread())->UpdateStatistic(FTPSTAT_UPLOADFAILED);
        }
        break;
    default:
        m_bInitialized = FALSE;
        break;
    }
}


/********************************************************************/
/*																	*/
/* Function name : CDataSocket::SetRestartOffset					*/
/* Description   : Set offset of where to restart file transfer.	*/
/*																	*/
/********************************************************************/
void CDataSocketEx::SetRestartOffset(DWORD dwOffset)
{
    m_dwRestartOffset = dwOffset;
}

int CDataSocketEx::Create()
{
    int iRet = 0;
    if (m_socket != INVALID_SOCKET)
    {
        ::closesocket(m_socket);
    }

    m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        cout << "创建SOCKET失败！,错误代码：" << WSAGetLastError() << endl;
        return -1;
    }

    int error = 0;
    sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = 0;
    addr_in.sin_addr.s_addr = INADDR_ANY;
    error = ::bind(m_socket, (sockaddr*)&addr_in, sizeof(sockaddr_in));
    if (error == SOCKET_ERROR) {
        cout << "绑定端口失败！,错误代码：" << WSAGetLastError() << endl;
        return -1;
    }

    error = listen(m_socket, 5);
    if (error == SOCKET_ERROR) {
        cout << "监听失败！,错误代码：" << WSAGetLastError() << endl;
        return -1;
    }
    cout << "成功监听端口 :" << ntohs(addr_in.sin_port) << endl;


    m_scoketEvent = ::WSACreateEvent();
    ::WSAEventSelect(m_socket, m_scoketEvent, FD_ACCEPT | FD_READ | FD_CLOSE);

    m_hThread = ::CreateThread(0, 0, RecvThreadProc, (void *)this, 0, 0);

    return iRet;
}

void CDataSocketEx::Close()
{
    if (NULL != m_f)
    {
        fclose(m_f);
        m_f = NULL;
    }
    m_bTerminate = TRUE;
    CloseHandle(m_hThread);
}

int CDataSocketEx::GetSockName(CString &strIP, UINT &nPort)
{
    struct sockaddr_in connAddr;
    int len = sizeof connAddr;
    int ret = getsockname(m_socket, (SOCKADDR*)&connAddr, &len);

    if (0 != ret) {
        return -1;
    }
    nPort = ntohs(connAddr.sin_port);               // 获取端口号
    strIP = inet_ntoa(connAddr.sin_addr);           // IP
    return 0;
}

void CDataSocketEx::Attach(SOCKET sock)
{
    this->m_socket = sock;
    m_bConnected = TRUE;
    m_scoketEvent = ::WSACreateEvent();
    ::WSAEventSelect(sock, m_scoketEvent, FD_READ | FD_CLOSE | FD_WRITE);

    if (m_nTransferType == 2)
    {
        PrepareReceiveFile(m_strData);
        m_nStatus = XFERMODE_RECEIVE;
        m_bConnected = TRUE;
    }
    else if (m_nTransferType == 1)
    {
        PrepareSendFile(m_strData);
        m_nStatus = XFERMODE_SEND;
        m_bConnected = TRUE;
        this->OnSend(0);
    }
    else if (m_nTransferType == 0)
    {
        m_nStatus = XFERMODE_LIST;
        m_bConnected = TRUE;
        this->OnSend(0);
    }
    m_hThread = ::CreateThread(0, 0, RecvThreadProc, (void *)this, 0, 0);
}

DWORD WINAPI CDataSocketEx::RecvThreadProc(LPVOID lpParam)
{
    if (lpParam == NULL)
        return 0;
    CDataSocketEx *client = (CDataSocketEx *)lpParam;
    WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS];                   // 事件对象数组
    SOCKET sockArray[WSA_MAXIMUM_WAIT_EVENTS];                      // 事件对象数组对应的SOCKET句柄
    DWORD ret = 0;
    int index = 0;
    int nEvent = 0;

    WSANETWORKEVENTS networkEvent;
    HANDLE events[2];
    events[0] = client->m_scoketEvent;
    nEvent++;

    while (true)
    {
        if (client->m_bTerminate)
        {
            break;
        }
        int nIndex = ::WSAWaitForMultipleEvents(nEvent, events, false, 5000, false);
        if (nIndex == WSA_WAIT_IO_COMPLETION)
        {
            std::cout << "等待时发生错误！错误代码：" << WSAGetLastError() << endl;
            break;
        }
        if (nIndex == WSA_WAIT_TIMEOUT)
        {
            continue;
        }
        nIndex = nIndex - WSA_WAIT_EVENT_0;
        WSANETWORKEVENTS event;
        SOCKET sock = client->m_socket;

        ::WSAEnumNetworkEvents(sock, events[nIndex], &event);
        if (event.lNetworkEvents & FD_READ)
        {
            if (event.iErrorCode[FD_READ_BIT] == 0)
            {
                client->OnReceive(1);
            }
        }
        if (event.lNetworkEvents & FD_CLOSE)
        {
            if (event.iErrorCode[FD_CLOSE_BIT] == 0)
            {
                client->OnClose(0);
            }
        }
        if (event.lNetworkEvents & FD_WRITE)
        {
            if (event.iErrorCode[FD_WRITE_BIT] == 0)
            {
                client->OnSend(1);
            }
        }
        if (event.lNetworkEvents & FD_ACCEPT)
        {
            if (event.iErrorCode[FD_ACCEPT_BIT] == 0)
            {
                sockaddr_in addr;
                int len = sizeof(sockaddr_in);
                SOCKET socket = ::accept(sock, (sockaddr*)&addr, &len);
                if (socket == INVALID_SOCKET)
                {
                    return -1;
                }
                ::closesocket(sock);
                CDataSocketEx* data = new CDataSocketEx(client->m_pConnectSocket, client->m_nTransferType);
                data->Attach(socket);
                break;
            }
        }
    }
    ::closesocket(client->m_socket);
    delete client;
    client = NULL;
    return 0;

}