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
    ACCEPT_POSTED, // ������δʹ��
    SEND_POSTED, // ������δʹ��
    RECV_POSTED,
    NULL_POSTED
}OPERATION_TYPE;

typedef struct _PER_IO_DATA {
    OVERLAPPED overlapped;
    // ��ͻ���ͨ�ŵ�socket
    SOCKET socket;
    // ������ַ��Ϣ
    SOCKADDR_IN *pLocalAddr;
    // �ͻ��˵�ַ��Ϣ
    SOCKADDR_IN *pClientAddr;
    // �����رձ�־
    BOOL bClose;
    // �Ƿ���Accepted
    BOOL bAccepted;
    // AcceptEx�ͻ��˽�������ʱ��
    time_t AcceptedTime;
    // ���һ�ν�������ʱ�䣬�������������
    time_t LastRecvTime;
    // Recv���
    int recvId;
    // Send���(������δʹ��)
    int sendId;
    // ��½��ʶ
    DWORD LoginId;
    // ����������������˶��ٴ�������ȷ�ı��ģ�
    DWORD nHandles;
    WSABUF wsaBuf;
    char buf[MAX_BUFFER];
    char *pMemory;
    DWORD nCopyed;
    WSABUF sendWSA;
    char sendBuf[MAX_SEND_BUFFER];
    OPERATION_TYPE type;

    // ��ʼ��
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
    // ��������Ϊ�����ⲿC������ͨ��ָ����ʡ��޸����е�˽�б���
    // �Ѿ��ָ��,=&m_hHeap
    HANDLE *phHeap;
    // ������socketָ��,=&m_socket
    SOCKET *pSocket;
    // AcceptExͶ����ָ��,=m_AcceptEx
    LPACCEPTEX AcceptEx;
    // GetAcceptExSockAddr����ָ��,=m_GetAcceptExSockAddr;
    LPGETACCEPTEXSOCKADDRS GetAcceptExSockAddrs;
    // �ͻ���������ָ��,&m_nAccepted
    int *pAccepted;
    // �����׽���AcceptExʧ����ָ��,&m_FailAccepted
    int *pFailAccepted;
    // �����߳��˳��¼�ָ��,&m_hQuitEvent
    HANDLE *phQuitEvent;
    // ֹͣ�����¼�ָ��,&m_hStopListen
    HANDLE *phStopListen;
    // �رռ����׽��ִ�������¼�ָ��,&m_hListenClosed
    HANDLE *phListenClosed;
    CRITICAL_SECTION *pcs;
    HANDLE *phIocp;
    // �����ѷ���IoData���ݵ�ָ������
    PPER_IO_DATA pIoArray[1000 * 100];
    DWORD AcceptFun; // ������δʹ��
    DWORD RecvFun;
    DWORD SendFun; // ������δʹ��

                   // ��ʼ��
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
    // ��Ա����
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

    // ��Ա����
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