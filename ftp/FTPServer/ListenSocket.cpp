/****************************************************************/
/*																*/
/*  LISTSOCKET.CPP												*/
/*																*/
/*  Implementation of the Listen Socket.						*/
/*  The server listens for connections. When a new connection	*/
/*  is requested, the server accepts the connection and then	*/
/*  creates a connect thread to handle the connection.			*/
/*																*/
/*  Programmed by Pablo van der Meer							*/
/*  Copyright Pablo Software Solutions 2002						*/
/*	http://www.pablovandermeer.nl								*/
/*																*/
/*  Last updated: 10 july 2002									*/
/*																*/
/****************************************************************/


#include "stdafx.h"
//#include "FTPServerApp.h"	
//#include "FTPServer.h"	
//#include "ApplicationDlg.h"
#include "ListenSocket.h"
//#include "ConnectThread.h"
#include "ConnectSocketEx.h"
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

extern DWORD g_targetThreadID;

CListenSocketEx::CListenSocketEx()
{

}

CListenSocketEx::~CListenSocketEx()
{

}

int CListenSocketEx::startListen()
{
    m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        cout << "创建SOCKET失败！,错误代码：" << WSAGetLastError() << endl;
        return -1;
    }

    int error = 0;
    sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(21);
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


    m_socketEvent = ::WSACreateEvent();
    ::WSAEventSelect(m_socket, m_socketEvent, FD_ACCEPT | FD_CLOSE);

    HANDLE thread = ::CreateThread(0, 0, RecvThreadProc, (void *)this, 0, 0);
    return 0;
}

DWORD WINAPI CListenSocketEx::RecvThreadProc(LPVOID lpParam)
{
    if (lpParam == NULL)
        return 0;
    CListenSocketEx *client = (CListenSocketEx *)lpParam;
    WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS];                   // 事件对象数组
    SOCKET sockArray[WSA_MAXIMUM_WAIT_EVENTS];                      // 事件对象数组对应的SOCKET句柄
    DWORD ret = 0;
    int index = 0;
    int nEvent = 0;

    HANDLE events[2];
    events[0] = client->m_socketEvent;
    events[1] = client->m_stopEvent;
    sockArray[0] = client->m_socket;

    nEvent++;
    while (true) {
        int nIndex = ::WSAWaitForMultipleEvents(nEvent, events, false, WSA_INFINITE, false);
        if (nIndex == WSA_WAIT_IO_COMPLETION || nIndex == WSA_WAIT_TIMEOUT) 
        {
            std::cout << "wait error ! error code :" << WSAGetLastError() << endl;
            break;
        }
        nIndex = nIndex - WSA_WAIT_EVENT_0;
        WSANETWORKEVENTS event;
        SOCKET sock = client->m_socket;

        ::WSAEnumNetworkEvents(sock, events[nIndex], &event);
        if (event.lNetworkEvents & FD_ACCEPT) 
        {
            if (event.iErrorCode[FD_ACCEPT_BIT] == 0) 
            {
                if (nEvent >= WSA_MAXIMUM_WAIT_EVENTS) 
                {
                    cout << "client count out of range!" << endl;
                    continue;
                }
                sockaddr_in addr;
                int len = sizeof(sockaddr_in);
                SOCKET client = ::accept(sock, (sockaddr*)&addr, &len);
                if (client != INVALID_SOCKET) 
                {
                    cout << "New come from " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << endl;
                    CConnectSocketEx* conn = new CConnectSocketEx();
                    conn->Attach(client);
                }
            }
        }
        else if (event.lNetworkEvents & FD_CLOSE) 
        {
            ::WSACloseEvent(eventArray[nIndex]);
            ::closesocket(sockArray[nIndex]);
            cout << "A Client has disconnect" << endl;
            for (int j = nIndex; j < nEvent - 1; j++) 
            {
                eventArray[j] = eventArray[j + 1];
                sockArray[j] = sockArray[j + 1];
            }
            nEvent--;
        }
    } // end while

    ::closesocket(client->m_socket);
}

void CListenSocketEx::OnAccept()
{
    if (!m_file.Open(_T("C:\\share\\1.bnd"), CFile::modeWrite | CFile::modeCreate | CFile::modeNoTruncate | CFile::shareDenyWrite))
    {
        return ;
    }
}
