#pragma once
#define UNICODE
#define _UNICODE

#ifdef _DEBUG
#define TRACE(msg) OutputDebugString(msg)
#else
#define TRACE(msg) ""
#endif

#include <winsock.h>
#include <winsock2.h>
#include <Mswsock.h>
#include <Winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <process.h>
#include <time.h>

#ifdef _DEBUG
#pragma comment(lib,"msvcrtd.lib")
#else
#pragma comment(lib,"msvcrt.lib")
#endif
#pragma comment(lib,"Ws2_32.lib")

#pragma warning(disable:4996)

#define MAX_ACCEPT 20
#define MAX_BUFFER 1024*4
#define MAX_SEND_BUFFER 1024*10
#define MAX_SECONDS 120

typedef BOOL(PASCAL FAR *LPACCEPTEX)(
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped);

typedef VOID(PASCAL FAR *LPGETACCEPTEXSOCKADDRS)(
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT struct sockaddr **LocalSockaddr,
    OUT LPINT LocalSockaddrLength,
    OUT struct sockaddr **RemoteSockaddr,
    OUT LPINT RemoteSockaddrLength);

typedef enum _OPERATION_TYPE {
    ACCEPT_POSTED, // 保留，未使用
    SEND_POSTED, // 保留，未使用
    RECV_POSTED,
    NULL_POSTED
}OPERATION_TYPE;

typedef struct _PER_IO_DATA {
    OVERLAPPED overlapped;
    // 与客户端通信的socket
    SOCKET socket;
    // 主机地址信息
    SOCKADDR_IN *pLocalAddr;
    // 客户端地址信息
    SOCKADDR_IN *pClientAddr;
    // 主动关闭标志
    BOOL bClose;
    // 是否已Accepted
    BOOL bAccepted;
    // AcceptEx客户端建立连接时间
    time_t AcceptedTime;
    // 最后一次接收数据时间，用来清理空连接
    time_t LastRecvTime;
    // Recv序号
    int recvId;
    // Send序号(保留，未使用)
    int sendId;
    // 登陆标识
    DWORD LoginId;
    // 处理次数（即接收了多少次完整正确的报文）
    DWORD nHandles;
    WSABUF wsaBuf;
    char buf[MAX_BUFFER];
    char *pMemory;
    DWORD nCopyed;
    WSABUF sendWSA;
    char sendBuf[MAX_SEND_BUFFER];
    OPERATION_TYPE type;

    // 初始化
    _PER_IO_DATA() {
        ZeroMemory(&overlapped, sizeof(overlapped));
        socket = INVALID_SOCKET;
        pLocalAddr = NULL;
        pClientAddr = NULL;
        bClose = FALSE;
        bAccepted = FALSE;
        AcceptedTime = NULL;
        LastRecvTime = NULL;
        recvId = 1;
        sendId = 1;
        LoginId = 0;
        nHandles = 0;
        wsaBuf.buf = buf;
        wsaBuf.len = sizeof(buf);
        pMemory = NULL;
        nCopyed = 0;
        sendWSA.buf = sendBuf;
        sendWSA.len = sizeof(sendBuf);
        type = NULL_POSTED;
    }
}PER_IO_DATA, *PPER_IO_DATA;

typedef struct _PER_HANDLE_DATA {
    // 这样做是为了让外部C函数能通过指针访问、修改类中的私有变量
    // 堆句柄指针,=&m_hHeap
    HANDLE *phHeap;
    // 监听的socket指针,=&m_socket
    SOCKET *pSocket;
    // AcceptEx投递数指针,=m_AcceptEx
    LPACCEPTEX AcceptEx;
    // GetAcceptExSockAddr函数指针,=m_GetAcceptExSockAddr;
    LPGETACCEPTEXSOCKADDRS GetAcceptExSockAddrs;
    // 客户端连接数指针,&m_nAccepted
    int *pAccepted;
    // 监听套接字AcceptEx失败数指针,&m_FailAccepted
    int *pFailAccepted;
    // 工作线程退出事件指针,&m_hQuitEvent
    HANDLE *phQuitEvent;
    // 停止监听事件指针,&m_hStopListen
    HANDLE *phStopListen;
    // 关闭监听套接字处理完毕事件指针,&m_hListenClosed
    HANDLE *phListenClosed;
    CRITICAL_SECTION *pcs;
    HANDLE *phIocp;
    // 保存已分配IoData数据的指针数组
    PPER_IO_DATA pIoArray[1000 * 100];
    DWORD AcceptFun; // 保留，未使用
    DWORD RecvFun;
    DWORD SendFun; // 保留，未使用

                   // 初始化
    _PER_HANDLE_DATA() {
        AcceptEx = NULL;
        GetAcceptExSockAddrs = NULL;
        pSocket = NULL;
        pAccepted = NULL;
        pcs = NULL;
        phIocp = NULL;
        ZeroMemory(&pIoArray, sizeof(pIoArray));
        AcceptFun = NULL;
        RecvFun = NULL;
        SendFun = NULL;
    }
}PER_HANDLE_DATA, *PPER_HANDLE_DATA;

typedef struct _THREAD_PARAM {
    PER_HANDLE_DATA *pHandleData;
    int num;
}THREAD_PARAM, *PTHREAD_PARAM;

typedef struct _CLEAN_THREAD_PARAM {
    PER_HANDLE_DATA *pHandleData;
    HANDLE *phQuitCleanThread;
}CLEAN_THREAD_PARAM, *PCLEAN_THREAD_PARAM;

typedef struct _POST_DATA {
    int *pAccepted;
    HANDLE *phQuitEvent;
}POST_DATA, *PPOST_DATA;

typedef DWORD(*LPIOCPFUN)(PER_HANDLE_DATA *pHandleData, PER_IO_DATA *pIoData, DWORD dwBytes);
void CleanIoDataQueued(PER_HANDLE_DATA *pHandleData, PER_IO_DATA *pIoData);
UINT WINAPI CleanSocketThread(LPVOID param);
UINT WINAPI WorkerThread(LPVOID param);
void AcceptHandle(PER_HANDLE_DATA *pHandleData, PER_IO_DATA *pIoData, DWORD dwBytes);
void RecvHandle(PER_HANDLE_DATA *pHandleData, PER_IO_DATA *pIoData, DWORD dwBytes);

class MYIOCP 
{
    // 成员变量
public:
    DWORD m_dwError;
private:
    HANDLE m_hHeap;
    HANDLE m_hIocp;
    int m_nWorkers;
    HANDLE *m_phThread;
    THREAD_PARAM *m_pThreadParam;
    HANDLE m_hCleanSocketThread;
    CLEAN_THREAD_PARAM m_CleanThreadParam;
    BOOL m_bStart;
    int m_nPort;
    SOCKET m_socket;
    LPACCEPTEX m_AcceptEx;
    LPGETACCEPTEXSOCKADDRS m_GetAcceptExSockAddrs;
    int m_nAccepted;
    int m_nFailAccepted;
    CRITICAL_SECTION m_cs;
    PER_HANDLE_DATA m_HandleData;
    HANDLE m_hQuitEvent;
    HANDLE m_hStopListen;
    HANDLE m_hListenClosed;
    HANDLE m_hQuitCleanThread;
    POST_DATA m_PostData;

    // 成员函数
public:
    MYIOCP();
    ~MYIOCP();
    BOOL SetPort(int port);
    BOOL SetAcceptFun(LPIOCPFUN fun);
    BOOL SetRecvFun(LPIOCPFUN fun);
    BOOL SetSendFun(LPIOCPFUN fun);
    BOOL Start();
    BOOL Stop();
private:
    void Clean();
    void StartWorkerThread(int num);
    BOOL SocketListen(int port);
};