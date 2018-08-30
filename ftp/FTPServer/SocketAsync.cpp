#include "stdafx.h"
#include "resource.h"
#include "SocketAsync.h"
#include <afxpriv.h>

#include <winsock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")
using std::cout;
using std::cin;
using std::endl;
using std::ends;

#define PACKET_SIZE 4096

CSocketAsync::CSocketAsync()
{
}


CSocketAsync::~CSocketAsync()
{

}


int CSocketAsync::OnSend(int nErrorCode)
{
    return 0;
}


int CSocketAsync::OnClose(int nErrorCode)
{
    return 0;
}

int CSocketAsync::OnAccept(int nErrorCode)
{
    return 0;
}

int CSocketAsync::GetStatus()
{
    return m_nStatus;
}

int CSocketAsync::Send(char* ch, DWORD len)
{
    int l = ::send(m_socket, ch, len, 0);
    return l;
}

int CSocketAsync::OnReceive(int nErrorCode)
{
    return 0;
}


int CSocketAsync::Create(long lNetworkEvents)
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
    ::WSAEventSelect(m_socket, m_scoketEvent, lNetworkEvents );

    m_hThread = ::CreateThread(0, 0, RecvThreadProc, (void *)this, 0, 0);

    return iRet;
}

void CSocketAsync::Close()
{
    m_bTerminate = TRUE;
    CloseHandle(m_hThread);
}

int CSocketAsync::GetSockName(CString &strIP, UINT &nPort)
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

void CSocketAsync::Attach(SOCKET sock)
{
    this->m_socket = sock;
    m_bConnected = TRUE;
    m_scoketEvent = ::WSACreateEvent();
    ::WSAEventSelect(sock, m_scoketEvent, FD_READ | FD_CLOSE | FD_WRITE);
    m_hThread = ::CreateThread(0, 0, RecvThreadProc, (void *)this, 0, 0);
}

DWORD WINAPI CSocketAsync::RecvThreadProc(LPVOID lpParam)
{
    if (lpParam == NULL)
        return 0;
    CSocketAsync *client = (CSocketAsync *)lpParam;
    WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS];                   // 事件对象数组
    SOCKET sockArray[WSA_MAXIMUM_WAIT_EVENTS];                      // 事件对象数组对应的SOCKET句柄
    DWORD ret = 0;
    int index = 0;
    int nEvent = 0;

    WSANETWORKEVENTS networkEvent;
    HANDLE events[2];
    events[0] = client->m_scoketEvent;
    nEvent++;
    int nTimes = 0;;
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
            nTimes++;
            if (nTimes * 5 >= client->m_nTimes && client->m_nTimes > 0)
            {
                // 超时退出
                break;
            }
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
                client->OnAccept(0);
            }
        }
    }
    ::closesocket(client->m_socket);
    delete client;
    client = NULL;
    return 0;

}