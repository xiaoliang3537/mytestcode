#include "stdafx.h"
#include "myiocp.h"


MYIOCP::MYIOCP()
{
    // ������
    m_dwError = 0;
    // �Ѿ��
    m_hHeap = NULL;
    // ��ɶ˿ھ��
    m_hIocp = NULL;
    // �����߳���
    m_nWorkers = 0;
    // �����߳̾������ָ��
    m_phThread = NULL;
    // �����̲߳�������ָ��
    m_pThreadParam = NULL;
    // ������Ծ�����߳̾�� 
    m_hCleanSocketThread = NULL;
    // ������Ծ�����̲߳���
    m_CleanThreadParam.pHandleData = &m_HandleData;
    m_CleanThreadParam.phQuitCleanThread = &m_hQuitCleanThread;
    // ����״̬
    m_bStart = FALSE;
    // �����˿�
    m_nPort = 1234;
    // �����׽���
    m_socket = INVALID_SOCKET;
    // AcceptEx����ָ��
    m_AcceptEx = NULL;
    // AcceptEx�ɹ�Ͷ����
    m_nAccepted = 0;
    // AcceptExʧ��Ͷ����
    m_nFailAccepted = 0;
    // GetAcceptExSockAddr����ָ��
    m_GetAcceptExSockAddrs = NULL;
    // ���ݸ�GetQueuedCompletionStatus�������Ľṹ
    m_HandleData.phHeap = &m_hHeap;
    m_HandleData.pSocket = &m_socket;
    m_HandleData.pAccepted = &m_nAccepted;
    m_HandleData.pFailAccepted = &m_nFailAccepted;
    m_HandleData.phQuitEvent = &m_hQuitEvent;
    m_HandleData.phStopListen = &m_hStopListen;
    m_HandleData.phListenClosed = &m_hListenClosed;
    m_HandleData.pcs = &m_cs;
    m_HandleData.phIocp = &m_hIocp;
    // �˳������߳�֪ͨ�¼�
    m_hQuitEvent = NULL;
    // ֹͣ�����¼�
    m_hStopListen = NULL;
    // �رռ����׽��ִ�������¼�
    m_hListenClosed = NULL;
    // �˳����������߳�֪ͨ�¼�
    m_hQuitCleanThread = NULL;

    m_PostData.pAccepted = &m_nAccepted;
    m_PostData.phQuitEvent = &m_hQuitEvent;

    // ��ʼ��Socket��
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    // ��ʼ���ٽ���
    InitializeCriticalSection(&m_cs);
    // ��ʼ����
    m_hHeap = HeapCreate(0, 4096, 0);
}

// ��������
MYIOCP::~MYIOCP() {
    Clean();
    WSACleanup();
    DeleteCriticalSection(&m_cs);
    HeapDestroy(m_hHeap);
    m_hHeap = NULL;
}

// ɨβ��������
void MYIOCP::Clean() {
    TCHAR temp[MAX_PATH];

    TRACE(TEXT("Clean()����ʼ��\n"));
    if (m_bStart) {
        if (m_hIocp) {
            _stprintf(temp, TEXT("����%d��AcceptExͶ��ʧ�ܡ�\n"), m_nFailAccepted);
            TRACE(temp);
            // �ȹرռ����׽��֣������Ͳ������µ�Accept����
            // �رռ����׽��ֲ���Ӱ�������ӵĿͻ���֪ͨ���е���������
            TRACE(TEXT("�رռ����׽��֡�\n"));
            EnterCriticalSection(&m_cs);
            SetEvent(m_hStopListen);
            // ѭ�����bAccepted�������δ���ӣ���AcceptedTime=1
            // �������ֺ���Ϊ����̫�࣬����֪ͨ��ʱ��ò�������ϵͳ�Զ�ȡ�������ش��󡣣��ڹ����߳��У�
            for (int q = 0; q<m_nAccepted; q++) {
                if (!m_HandleData.pIoArray[q]->bAccepted) {
                    m_HandleData.pIoArray[q]->AcceptedTime = 1;
                }
            }
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            LeaveCriticalSection(&m_cs);
            // �ȴ��رռ����׽��ִ������֪ͨ
            TRACE(TEXT("�ȴ��رռ����׽��ִ������֪ͨ��\n"));
            while (WAIT_OBJECT_0 != WaitForSingleObject(m_hListenClosed, 0)) {
                _stprintf(temp, TEXT("����%d��AcceptExδ�˳���\n"), MAX_ACCEPT - m_nFailAccepted);
                TRACE(temp);
                Sleep(1000);
            }
            TRACE(TEXT("�رռ����׽��ִ������֪ͨ���ء�\n"));
            CloseHandle(m_hStopListen);
            m_hStopListen = NULL;
            CloseHandle(m_hListenClosed);
            m_hListenClosed = NULL;
            m_nFailAccepted = 0;
            // �ر����������ӵĿͻ���socket
            TRACE(TEXT("�ر����������ӵĿͻ���socket��\n"));
            EnterCriticalSection(&m_cs);
            for (int j = 0; j<sizeof(m_HandleData.pIoArray) / sizeof(m_HandleData.pIoArray[0]); j++) {
                if (NULL == m_HandleData.pIoArray[j]) {
                    break;
                }
                if (!m_HandleData.pIoArray[j]->bClose) {
                    m_HandleData.pIoArray[j]->bClose = TRUE;
                    closesocket(m_HandleData.pIoArray[j]->socket);
                }
            }
            LeaveCriticalSection(&m_cs);
            // �������������߳��˳�֪ͨ
            SetEvent(m_hQuitCleanThread);
            TRACE(TEXT("�ȴ����������߳��˳�֪ͨ��\n"));
            WaitForSingleObject(m_hCleanSocketThread, INFINITE);
            TRACE(TEXT("���������߳��˳�֪ͨ���ء�\n"));
            CloseHandle(m_hCleanSocketThread);
            m_hCleanSocketThread = NULL;
            CloseHandle(m_hQuitCleanThread);
            m_hQuitCleanThread = NULL;
            // ���͹����߳��˳�֪ͨ�����m_nAccepted�Ƿ�Ϊ0
            TRACE(TEXT("�ȴ�����socket�������֪ͨ��\n"));
            while (WAIT_OBJECT_0 != WaitForSingleObject(m_hQuitEvent, 0)) {
                _stprintf(temp, TEXT("����%d��socket���ڴ����С�\n"), m_nAccepted);
                TRACE(temp);
                PostQueuedCompletionStatus(m_hIocp, 0, (DWORD)&m_PostData, 0);
                Sleep(1000);
            }
            TRACE(TEXT("����socket�������֪ͨ���ء�\n"));
            ZeroMemory(m_HandleData.pIoArray, sizeof(m_HandleData.pIoArray));
            // ����ͻ���������Ϊ0��˵���Ѿ���������������
            // �����˳��߳�֪ͨ���ù����߳��˳�
            TRACE(TEXT("���͹����߳��˳�֪ͨ��\n"));
            for (int i = 0; i<m_nWorkers; i++) {
                PostQueuedCompletionStatus(m_hIocp, 0, (DWORD)&m_PostData, 0);
            }
            // �ȴ����й����߳��˳�
            TRACE(TEXT("�ȴ����й����߳��˳�֪ͨ��\n"));
            WaitForMultipleObjects(m_nWorkers, m_phThread, TRUE, INFINITE);
            TRACE(TEXT("���й����߳��˳�֪ͨ���ء�\n"));
            for (int k = 0; k<m_nWorkers; k++) {
                CloseHandle(m_phThread[k]);
            }
            CloseHandle(m_hQuitEvent);
            m_hQuitEvent = NULL;
            CloseHandle(m_hIocp);
            m_hIocp = NULL;
        }

        // �����߳̾��
        if (m_phThread) {
            delete[] m_phThread;
            m_phThread = NULL;
        }

        m_bStart = FALSE;
    }
    TRACE(TEXT("Clean()���������\n"));
}

// ���ü����˿�
BOOL MYIOCP::SetPort(int port) {
    BOOL ret = TRUE;

    if (m_bStart) {
        ret = FALSE;
    }
    else {
        m_nPort = port;
    }

    return ret;
}

// ����AcceptͶ�ݺ���
BOOL MYIOCP::SetAcceptFun(LPIOCPFUN fun) {
    BOOL ret = TRUE;

    if (m_bStart) {
        ret = FALSE;
    }
    else {
        m_HandleData.AcceptFun = (DWORD)fun;
    }

    return ret;
}

// ����RecvͶ�ݺ���
BOOL MYIOCP::SetRecvFun(LPIOCPFUN fun) {
    BOOL ret = TRUE;

    if (m_bStart) {
        ret = FALSE;
    }
    else {
        m_HandleData.RecvFun = (DWORD)fun;
    }

    return ret;
}

// ����SendͶ�ݺ���
BOOL MYIOCP::SetSendFun(LPIOCPFUN fun) {
    BOOL ret = TRUE;

    if (m_bStart) {
        ret = FALSE;
    }
    else {
        m_HandleData.SendFun = (DWORD)fun;
    }

    return ret;
}

// ����
BOOL MYIOCP::Start() {
    BOOL ret = TRUE;
    UINT dwId;

    TRACE(TEXT("Start()��ʼ��\n"));
    if (!m_bStart) {
        // ������ɶ˿ھ��
        TRACE(TEXT("����m_hIocp��ɶ˿ڡ�\n"));
        m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (NULL == m_hIocp) {
            m_dwError = GetLastError();
            TRACE(TEXT("����m_hIocp��ɶ˿�ʧ�ܣ�\n"));
            goto clean;
        }
        // ���������߳�
        StartWorkerThread(m_nWorkers);
        // ����������Ծ�����߳�
        m_hCleanSocketThread = (HANDLE)_beginthreadex(NULL, 0, CleanSocketThread, (LPVOID)&m_CleanThreadParam, 0, &dwId);
        if (NULL == m_hCleanSocketThread) {
            TRACE(TEXT("�½�CleanSocketThread�߳�ʧ�ܣ�\n"));
        }
        else {
            TRACE(TEXT("�ɹ��½�CleanSocketThread�̡߳�\n"));
        }
        // �����׽��ּ����˿ڲ��󶨵���ɶ˿�
        if (!SocketListen(m_nPort)) {
            // SocketListen�ڲ�������m_dwError����ֵ��
            TRACE(TEXT("SocketListen()����ʧ�ܣ�\n"));
            goto clean;
        }

        // �����˳������߳�֪ͨ�¼�
        TRACE(TEXT("����m_hQuitEvent�¼���\n"));
        m_hQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == m_hQuitEvent) {
            m_dwError = GetLastError();
            TRACE(TEXT("����m_hQuitEvent�¼�ʧ�ܣ�\n"));
            goto clean;
        }

        // ����ֹͣ����֪ͨ�¼�
        TRACE(TEXT("����m_hStopListen�¼���\n"));
        m_hStopListen = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == m_hStopListen) {
            m_dwError = GetLastError();
            TRACE(TEXT("����m_hStopListen�¼�ʧ�ܣ�\n"));
            goto clean;
        }

        // �����رռ����׽��ִ������֪ͨ�¼�
        TRACE(TEXT("����m_hListenClosed�¼���\n"));
        m_hListenClosed = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == m_hListenClosed) {
            m_dwError = GetLastError();
            TRACE(TEXT("����m_hListenClosed�¼�ʧ�ܣ�\n"));
            goto clean;
        }

        // �����˳����������߳�֪ͨ�¼�
        TRACE(TEXT("����m_hQuitCleanThread�¼���\n"));
        m_hQuitCleanThread = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == m_hQuitCleanThread) {
            m_dwError = GetLastError();
            TRACE(TEXT("����m_hQuitCleanThread�¼�ʧ�ܣ�\n"));
            goto clean;
        }

        m_bStart = TRUE;
        goto end;
    }
    else {
        goto end;
    }

clean:
    Clean();
    ret = FALSE;

end:
    if (ret) {
        TRACE(TEXT("Start()�ɹ����ء�\n"));
    }
    else {
        TRACE(TEXT("Start()ʧ�ܷ��ء�\n"));
    }
    return ret;
}

// ֹͣ
BOOL MYIOCP::Stop() {
    BOOL ret = TRUE;

    TRACE(TEXT("Stop()��ʼ��\n"));
    if (m_bStart) {
        Clean();
        m_bStart = FALSE;
    }

    TRACE(TEXT("Stop()������\n"));
    return ret;
}

// ���������߳�
void MYIOCP::StartWorkerThread(int num) {
    TCHAR temp[MAX_PATH];
    SYSTEM_INFO info;
    UINT dwId;

    GetSystemInfo(&info);
    m_nWorkers = info.dwNumberOfProcessors * 2 + 2;
    m_phThread = new HANDLE[m_nWorkers];
    m_pThreadParam = new THREAD_PARAM[m_nWorkers];

    for (int i = 0; i<m_nWorkers; i++) {
        m_pThreadParam[i].num = i + 1;
        m_pThreadParam[i].pHandleData = &m_HandleData;
        m_phThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, (LPVOID)&m_pThreadParam[i], 0, &dwId);
        if (NULL == m_phThread[i]) {
            // �½��߳�ʧ��
            _stprintf(temp, TEXT("�½���%d�������߳�ʧ�ܣ�\n"), i + 1);
            TRACE(temp);
            i--;
            m_nWorkers--;
            continue;
        }
        else {
            _stprintf(temp, TEXT("�ɹ��½���%d�������̡߳�\n"), i + 1);
            TRACE(temp);
        }
    }
}

// ����ά��PER_HANDLE_DATA�е�IO����ָ�����
void CleanIoDataQueued(PER_HANDLE_DATA *pHandleData, PER_IO_DATA *pIoData) {
    TCHAR temp[MAX_PATH];

    TRACE(TEXT("CleanIoDataQueued()��ʼ��\n"));
    EnterCriticalSection(pHandleData->pcs);
    for (int i = 0; i<(*pHandleData->pAccepted); i++) {
        if (pHandleData->pIoArray[i] == pIoData) {
            if (i + 1 == (*pHandleData->pAccepted)) {
                pHandleData->pIoArray[i] = NULL;
            }
            else {
                memcpy(&(pHandleData->pIoArray[i]), &(pHandleData->pIoArray[i + 1]), 4 * ((*pHandleData->pAccepted) - 1 - i));
                pHandleData->pIoArray[(*pHandleData->pAccepted) - 1] = NULL;
            }
            (*pHandleData->pAccepted)--;
            _stprintf(temp, TEXT("�ɹ�����pIoArrary[%d](%08X)��\n"), i, (DWORD)pIoData);
            TRACE(temp);
            break;
        }
    }
    LeaveCriticalSection(pHandleData->pcs);
    TRACE(TEXT("CleanIoDataQueued()������\n"));
}

// ������Ծ�����߳�
UINT WINAPI CleanSocketThread(LPVOID param) {
    CLEAN_THREAD_PARAM *CleanData = (CLEAN_THREAD_PARAM*)param;
    PER_HANDLE_DATA *pHandleData = CleanData->pHandleData;
    time_t second;
    TCHAR temp[MAX_PATH], szText[MAX_PATH];
    char *szIP;
    int port;

    TRACE(TEXT("������Ծ�����߳̿�ʼ���С�\n"));

    while (TRUE) {
        if (WAIT_OBJECT_0 == WaitForSingleObject(*CleanData->phQuitCleanThread, 0)) {
            break;
        }
        EnterCriticalSection(pHandleData->pcs);
        for (int i = 0; i<(*pHandleData->pAccepted); i++) {
            if (pHandleData->pIoArray[i]->AcceptedTime) {
                if (NULL == pHandleData->pIoArray[i]->LastRecvTime) {
                    // ������
                    second = pHandleData->pIoArray[i]->AcceptedTime;
                }
                else {
                    // �����ӻ�ʱ�䲻��Ծ����
                    second = pHandleData->pIoArray[i]->LastRecvTime;
                }
                if (time(NULL) - second > MAX_SECONDS) {
                    if (0 == pHandleData->pIoArray[i]->LoginId) {
                        // �ر�δ��½�ĳ�ʱ����
                        if (pHandleData->pIoArray[i]->pClientAddr) {
                            szIP = inet_ntoa(pHandleData->pIoArray[i]->pClientAddr->sin_addr);
                            port = ntohs(pHandleData->pIoArray[i]->pClientAddr->sin_port);
                        }
                        else {
                            szIP = "?";
                            port = 0;
                        }
                        ZeroMemory(szText, sizeof(szText));
                        MultiByteToWideChar(CP_ACP, 0, szIP, -1, szText, strlen(szIP));
                        _stprintf(temp, TEXT("������Ծ���ӣ�%s:%d��\n"), szText, port);
                        TRACE(temp);
                        if (!pHandleData->pIoArray[i]->bClose) {
                            pHandleData->pIoArray[i]->bClose = TRUE;
                            closesocket(pHandleData->pIoArray[i]->socket);
                        }
                    }
                    else {
                        // ��½�ʻ�����ʱ�䲻ͬ����ʱΪ2Сʱ��
                        if (time(NULL) - second > 60 * 60 * 2) {
                            _stprintf(temp, TEXT("LoginId:%d�ʻ�δ�2Сʱ���رա�"), pHandleData->pIoArray[i]->LoginId);
                            TRACE(temp);
                            if (!pHandleData->pIoArray[i]->bClose) {
                                pHandleData->pIoArray[i]->bClose = TRUE;
                                closesocket(pHandleData->pIoArray[i]->socket);
                            }
                        }
                    }
                }
            }
        }
        LeaveCriticalSection(pHandleData->pcs);
        Sleep(1000 * 20);
    }

    TRACE(TEXT("������Ծ�����߳̽����˳���\n"));
    return 0;
}

// �����߳�
UINT WINAPI WorkerThread(LPVOID param) {
    TCHAR temp[MAX_PATH];
    THREAD_PARAM *pThreadParam = (THREAD_PARAM*)param;
    int num = pThreadParam->num;
    PER_HANDLE_DATA *pClassData = pThreadParam->pHandleData;
    HANDLE hIocp = *pClassData->phIocp;
    DWORD dwBytes, dwTemp;
    PER_HANDLE_DATA *pHandle;
    PER_IO_DATA *pIoData;
    BOOL ret;
    LPIOCPFUN AcceptFun, RecvFun, SendFun;
    DWORD dwError;

    _stprintf(temp, TEXT("��%d��WorkerThread��ʼ���С�\n"), num);
    TRACE(temp);
    while (TRUE) {
        ret = GetQueuedCompletionStatus(hIocp, &dwBytes, (DWORD*)&pHandle, (LPOVERLAPPED*)&pIoData, INFINITE);
        /*
        �����첽���յļ������
        1.��ɶ˿�֪ͨ������Windows�Լ�ά����
        ��Ϊ��������첽�ģ����Ե��ͻ��ˣ�ʹ��ͬ��������������δ���ԣ�connect����send���سɹ�ʱ��
        Windowsֻ�ǰѿͻ��˵��������ݱ�����ϵͳ����(����Ĭ�ϴ�С4K)�У�����֪ͨ��ӵ����еȴ����ǻ�ȡ��
        ע����˵���䣬����ͻ���send�Ĵ�������ӵ����е�֪ͨ����1:1�Ĺ�ϵ����ΪTCP�ǻ������ģ�
        ����ͻ���send������10�Σ��������������Ϊ�㷨�Ĺ�ϵ�����ܰ�10��send�����ݺϲ�����ϵͳ�����ֻ���һ��֪ͨ�����С�
        ����ͻ���sendֻ����1�Σ������ݽ϶ࣩ��ϵͳ������ȫ��������ϵͳ����󣬷���ָ������(WSARecv buf)̫С��������Ӷ��֪ͨ�����С�
        ��������Ҳ���Ӧ�����Ƚ��ȳ���ģʽ��PostQueuedCompletionStatus���������Զ���֪ͨ��
        GetQueuedCompletionStatus����ϵͳ�Ӷ�����ȡ��һ��֪ͨ���������֪ͨ���յ������ݣ���ϵͳ�����У�
        ���뵽����ָ���Ļ����У�Ȼ����ϵͳ�������ⲿ�����ݾͱ�������Ч�������ˡ�
        GetQueuedCompletionStatus����FALSE�Ļ���û�����ݿ�����һ���������Զ���֪ͨҲ�����ڿ���������
        ����ⲿ����ܶ����˳�����������������ϵͳ�������ˣ��ͻ��˵����󶼻ᱻ����ֱ���п��õ�ϵͳ���档
        2.GetQueuedCompletionStatusʲô����»᷵�أ�
        һ��֮ǰʹ��WSARecv,AcceptExͶ�ݣ����Ҷ��������������֪ͨ��
        Ͷ��һ��ȡһ�Ρ�
        ---------------------------------------------------------------------------------------------
        �ͻ��������˳�(ʹ��shutdownȻ��closesocket���������ݴ���ʱclosesocket)���˳�֪ͨ������С�
        ---------------------------------------------------------------------------------------------
        ��Ͷ˱����˳�����ǿ�س��򡢻��߻������ݴ����ʱ��ǿ��closesocket����
        δ�����ͨ�����лᱻϵͳ���������֪ͨ���ڴ��������˳���֪ͨ��������У���������С�
        ����˹ر���ͻ����׽��֣�����ͬ�����˳���
        ---------------------------------------------------------------------------------------------
        ����˹رռ����׽��֡������������ڵȴ������AcceptEx֪ͨ���м���δ�����AcceptEx�������Ӽ���֪ͨ��
        �����ȼ����ر�֮�󣬹����߳������ȴ���ȴ������AcceptEx֪ͨ���ٴ����Ѿ�Ͷ�ݵ�AcceptEx��֪ͨ��Ȼ��
        ����WSARecv֪ͨ��
        ---------------------------------------------------------------------------------------------

        �������Զ���֪ͨ����������֪ͨ���ȼ���ߣ�����ȡ�Զ�����Ϣ��
        �����ر���ɶ˿ھ������֪ͨ������ѭ����

        */
        if (ret) {
            // �ɹ�����
            if (0 == dwBytes) {
                if (pIoData) {
                    if (pIoData->bAccepted) {
                        // �ͻ���ʹ��closesocket���Źر�����
                        EnterCriticalSection(pHandle->pcs);
                        if (!pIoData->bClose) {
                            closesocket(pIoData->socket);
                        }
                        if (pIoData->pMemory) {
                            HeapFree(*pHandle->phHeap, 0, pIoData->pMemory);
                            pIoData->pMemory = NULL;
                        }
                        LeaveCriticalSection(pHandle->pcs);
                        CleanIoDataQueued(pHandle, pIoData);
                        delete pIoData;
                    }
                    else {
                        // AcceptEx���ͻ�����������
                        AcceptHandle(pHandle, pIoData, dwBytes);
                    }
                }
                else {
                    // ������������PostQueuedCompletionStatus֪ͨ�����߳��˳�
                    if (0 == *((POST_DATA*)pHandle)->pAccepted) {
                        SetEvent(*((POST_DATA*)pHandle)->phQuitEvent);
                        break;
                    }
                }
            }
            else {
                // ������������
                AcceptFun = (LPIOCPFUN)pHandle->AcceptFun;
                RecvFun = (LPIOCPFUN)pHandle->RecvFun;
                SendFun = (LPIOCPFUN)pHandle->SendFun;
                switch (pIoData->type) {
                case ACCEPT_POSTED:
                    //AcceptHandle(pHandle,pIoData,dwBytes);
                    break;
                case RECV_POSTED:
                    RecvHandle(pHandle, pIoData, dwBytes);
                    break;
                case SEND_POSTED:
                    break;
                default:
                    break;
                }
            }
            continue;
        }
        else {
            // ���ⷵ��
            dwError = GetLastError();
            if (pIoData) {
                // 1.�Ͷ˱����˳�����ǿ�س��򡢻��߻������ݴ����ʱ��ǿ��closesocket����dwError=64��
                // 2.����˹ر���ͻ����׽��֣�closesocket����dwError=1236��
                // 3.����˹رռ����׽��֣�dwError=995��
                // 4.֪ͨ����̫�࣬���һ�����гٳٵò�������ϵͳ�����ȡ��������FALSE��
                EnterCriticalSection(pHandle->pcs);
                if (!pIoData->bClose) {
                    // ���1��3��4
                    // ����˵����2���ⶼ�ر����socket����Ϊ2֮ǰ�Ѿ�����������closesocket��
                    closesocket(pIoData->socket);
                }
                if (pIoData->pMemory) {
                    HeapFree(*pHandle->phHeap, 0, pIoData->pMemory);
                    pIoData->pMemory = NULL;
                }
                LeaveCriticalSection(pHandle->pcs);
                CleanIoDataQueued(pHandle, pIoData);
                if (!pIoData->bAccepted) {
                    // �����AcceptExͶ��δ�������
                    if (1 == pIoData->AcceptedTime) {
                        // �������ɹرռ����׽��������
                        EnterCriticalSection(pHandle->pcs);
                        (*pHandle->pFailAccepted)++;
                        if (MAX_ACCEPT == *pHandle->pFailAccepted) {
                            SetEvent(*pHandle->phListenClosed);
                        }
                        LeaveCriticalSection(pHandle->pcs);
                    }
                    else {
                        // ������AcceptEx֪ͨ��ʱ��ò���������Ϊ����̫�ࣩ��ϵͳ�Զ�ȡ�������ش���
                        // Ͷ��һ���µ�AcceptEx
                        PER_IO_DATA *pNewIoData = new PER_IO_DATA;
                        pNewIoData->type = ACCEPT_POSTED;
                        // ��ǰ������ͻ���ͨ�ŵ�socket
                        pNewIoData->socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
                        if (INVALID_SOCKET == pNewIoData->socket) {
                            TRACE(TEXT("�½��ͻ���ͨ��socketʧ�ܣ��޷�����AcceptExͶ�ݡ�(WorkerThread��)\n"));
                            delete pNewIoData;
                        }
                        else {
                            EnterCriticalSection(pHandle->pcs);
                            if (WAIT_OBJECT_0 != WaitForSingleObject(*pHandle->phStopListen, 0)) {
                                if (TRUE == ((pHandle->AcceptEx)(*pHandle->pSocket, pNewIoData->socket,
                                    pNewIoData->wsaBuf.buf, 0,//pNewIoData->wsaBuf.len-(sizeof(SOCKADDR_IN)+16)*2,
                                    sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwTemp, &pNewIoData->overlapped)) ||
                                    WSA_IO_PENDING == WSAGetLastError()) {
                                    // �ɹ�
                                    pHandle->pIoArray[*pHandle->pAccepted] = pNewIoData;
                                    (*pHandle->pAccepted)++;
                                }
                                else {
                                    // AcceptExͶ��ʧ�ܣ���������ڴ�
                                    TRACE(TEXT("AcceptExͶ��ʧ�ܣ�(WorkerThread��)\n"));
                                    closesocket(pNewIoData->socket);
                                    delete  pNewIoData;
                                    (*pHandle->pFailAccepted)++;
                                    if (MAX_ACCEPT == *pHandle->pFailAccepted) {
                                        SetEvent(*pHandle->phListenClosed);
                                    }
                                }
                            }
                            else {
                                TRACE(TEXT("��⵽ֹͣ��������ֹAcceptExͶ�ݣ�(WorkerThread��)"));
                                closesocket(pNewIoData->socket);
                                delete  pNewIoData;
                                (*pHandle->pFailAccepted)++;
                                if (MAX_ACCEPT == *pHandle->pFailAccepted) {
                                    SetEvent(*pHandle->phListenClosed);
                                }
                            }
                            LeaveCriticalSection(pHandle->pcs);
                        }
                        // AcceptEx����
                    }
                }
                delete pIoData;
            }
            else {
                // 1.�ر���ɶ˿ھ��
                // 2.����δ֪ԭ�������˳�
                _stprintf(temp, TEXT("GetQueuedCompletionStatus()δ֪ԭ����󷵻�!GetLastError:%d��\n"), dwError);
                TRACE(temp);
            }
            continue;
        }
    }

    _stprintf(temp, TEXT("��%d��WorkerThread�����˳���\n"), num);
    TRACE(temp);
    return 0;
}

void AcceptHandle(PER_HANDLE_DATA *pHandleData, PER_IO_DATA *pIoData, DWORD dwBytes) {
    DWORD dwTemp;
    DWORD dwFlags = 0;
    int LocalLen, ClientLen;
    LocalLen = ClientLen = sizeof(SOCKADDR_IN);
    BOOL bWSARecv;

    TRACE(TEXT("����AcceptHandle()��\n"));
    // Ͷ��һ���µ�AcceptEx
    PER_IO_DATA *pNewIoData = new PER_IO_DATA;
    pNewIoData->type = ACCEPT_POSTED;
    // ��ǰ������ͻ���ͨ�ŵ�socket
    pNewIoData->socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == pNewIoData->socket) {
        TRACE(TEXT("�½��ͻ���ͨ��socketʧ�ܣ��޷�����AcceptExͶ�ݡ�(AcceptHandle��)\n"));
        delete pNewIoData;
    }
    else {
        EnterCriticalSection(pHandleData->pcs);
        if (WAIT_OBJECT_0 != WaitForSingleObject(*pHandleData->phStopListen, 0)) {
            if (TRUE == ((pHandleData->AcceptEx)(*pHandleData->pSocket, pNewIoData->socket,
                pNewIoData->wsaBuf.buf, 0,//pNewIoData->wsaBuf.len-(sizeof(SOCKADDR_IN)+16)*2,
                sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwTemp, &pNewIoData->overlapped)) ||
                WSA_IO_PENDING == WSAGetLastError()) {
                // �ɹ�
                pHandleData->pIoArray[*pHandleData->pAccepted] = pNewIoData;
                (*pHandleData->pAccepted)++;
            }
            else {
                // AcceptExͶ��ʧ�ܣ���������ڴ�
                TRACE(TEXT("AcceptExͶ��ʧ�ܣ�(AcceptHandle��)\n"));
                closesocket(pNewIoData->socket);
                delete  pNewIoData;
                (*pHandleData->pFailAccepted)++;
                if (MAX_ACCEPT == *pHandleData->pFailAccepted) {
                    SetEvent(*pHandleData->phListenClosed);
                }
            }
        }
        else {
            TRACE(TEXT("��⵽ֹͣ��������ֹAcceptExͶ�ݣ�(AcceptHandle��)"));
            closesocket(pNewIoData->socket);
            delete  pNewIoData;
            (*pHandleData->pFailAccepted)++;
            if (MAX_ACCEPT == *pHandleData->pFailAccepted) {
                SetEvent(*pHandleData->phListenClosed);
            }
        }
        LeaveCriticalSection(pHandleData->pcs);
    }

    pIoData->bAccepted = TRUE;
    pIoData->AcceptedTime = time(NULL);
    (pHandleData->GetAcceptExSockAddrs)(pIoData->wsaBuf.buf, 0,//pIoData->wsaBuf.len-((sizeof(SOCKADDR_IN)+16)*2),
        sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
        (LPSOCKADDR*)(&pIoData->pLocalAddr), &LocalLen, (LPSOCKADDR*)(&pIoData->pClientAddr), &ClientLen);
    // ���ͻ���socket����ɶ˿ڣ�����WSARecvͶ�ݣ�
    CreateIoCompletionPort((HANDLE)(pIoData->socket), *pHandleData->phIocp, (DWORD)pHandleData, 0);

    // WSARecvͶ��
    pIoData->type = RECV_POSTED;
    EnterCriticalSection(pHandleData->pcs);
    if (pIoData->bClose ||
        (SOCKET_ERROR == WSARecv(pIoData->socket, &pIoData->wsaBuf, 1, &dwTemp, &dwFlags, &pIoData->overlapped, NULL) &&
            WSA_IO_PENDING != WSAGetLastError())) {
        bWSARecv = FALSE;
    }
    else {
        bWSARecv = TRUE;
    }
    LeaveCriticalSection(pHandleData->pcs);
    if (!bWSARecv) {
        // WSARecvͶ��ʧ��
        EnterCriticalSection(pHandleData->pcs);
        if (!pIoData->bClose) {
            pIoData->bClose = TRUE;
            closesocket(pIoData->socket);
        }
        if (pIoData->pMemory) {
            HeapFree(*pHandleData->phHeap, 0, pIoData->pMemory);
            pIoData->pMemory = NULL;
        }
        LeaveCriticalSection(pHandleData->pcs);

        CleanIoDataQueued(pHandleData, pIoData);
        delete pIoData;
    }
    TRACE(TEXT("�˳�AcceptHandle()��\n"));
}

void RecvHandle(PER_HANDLE_DATA *pHandleData, PER_IO_DATA *pIoData, DWORD dwBytes) {
    DWORD dwTemp;
    DWORD dwFlags = 0;
    LPIOCPFUN RecvFun;
    BOOL bWSARecv;

    TRACE(TEXT("����RecvHandle()��\n"));
    pIoData->LastRecvTime = time(NULL);

    RecvFun = (LPIOCPFUN)pHandleData->RecvFun;
    if (RecvFun) {
        RecvFun(pHandleData, pIoData, dwBytes);
    }

    (pIoData->recvId)++;
    pIoData->type = RECV_POSTED;
    EnterCriticalSection(pHandleData->pcs);
    if (pIoData->bClose ||
        (SOCKET_ERROR == WSARecv(pIoData->socket, &pIoData->wsaBuf, 1, &dwTemp, &dwFlags, &pIoData->overlapped, NULL) &&
            WSA_IO_PENDING != WSAGetLastError())) {
        bWSARecv = FALSE;
    }
    else {
        bWSARecv = TRUE;
    }
    LeaveCriticalSection(pHandleData->pcs);
    if (!bWSARecv) {
        // WSARecvͶ��ʧ��
        EnterCriticalSection(pHandleData->pcs);
        if (!pIoData->bClose) {
            pIoData->bClose = TRUE;
            closesocket(pIoData->socket);
        }
        if (pIoData->pMemory) {
            HeapFree(*pHandleData->phHeap, 0, pIoData->pMemory);
            pIoData->pMemory = NULL;
        }
        LeaveCriticalSection(pHandleData->pcs);

        CleanIoDataQueued(pHandleData, pIoData);
        delete pIoData;
    }
    TRACE(TEXT("�˳�RecvHandle()��\n"));
}

// �����׽��ּ����˿ڲ��󶨵���ɶ˿�
BOOL MYIOCP::SocketListen(int port) {
    TCHAR temp[MAX_PATH];
    BOOL ret = TRUE;
    PER_IO_DATA *pIoData;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
    DWORD dwBytes;

    TRACE(TEXT("SocketListen()��ʼ��\n"));
    // TCP��
    TRACE(TEXT("WSASocket()�½������׽��֡�\n"));
    m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == m_socket) {
        m_dwError = WSAGetLastError();
        TRACE(TEXT("WSASocket()����ʧ�ܣ�\n"));
        goto clean;
    }
    struct sockaddr_in ServerAddr;
    ZeroMemory(&ServerAddr, sizeof(sockaddr_in));
    ServerAddr.sin_family = AF_INET;
    // ����������������
    ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    ServerAddr.sin_port = htons(port);
    TRACE(TEXT("bind()�󶨶˿ڡ�\n"));
    if (SOCKET_ERROR == bind(m_socket, (struct sockaddr*)&ServerAddr, sizeof(sockaddr_in))) {
        m_dwError = WSAGetLastError();
        _stprintf(temp, TEXT("��%d�˿�ʧ�ܣ�\n"), port);
        TRACE(temp);
        goto clean;
    }
    TRACE(TEXT("listen()��ʼ������\n"));
    if (SOCKET_ERROR == listen(m_socket, SOMAXCONN)) {
        m_dwError = WSAGetLastError();
        TRACE(TEXT("listen()����ʧ�ܣ�\n"));
        goto clean;
    }
    // �󶨵���ɶ˿�
    TRACE(TEXT("�����׽��ְ󶨵���ɶ˿ڡ�\n"));
    if (NULL == CreateIoCompletionPort((HANDLE)m_socket, m_hIocp, (DWORD)&m_HandleData, 0)) {
        m_dwError = GetLastError();
        TRACE(TEXT("������ɶ˿�ʧ�ܣ�\n"));
        goto clean;
    }

    // ��ȡAcceptEx����ָ��
    TRACE(TEXT("��ȡAcceptEx����ָ�롣\n"));
    if (SOCKET_ERROR == WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidAcceptEx, sizeof(GuidAcceptEx), &m_AcceptEx, sizeof(m_AcceptEx),
        &dwBytes, NULL, NULL)) {
        m_HandleData.AcceptEx = m_AcceptEx = NULL;
        m_dwError = WSAGetLastError();
        TRACE(TEXT("��ȡAcceptExʧ�ܣ�\n"));
        goto clean;
    }
    // ��ȡGetAcceptExSockAddr����ָ��
    TRACE(TEXT("��ȡGetAcceptExSockAddr����ָ�롣\n"));
    if (SOCKET_ERROR == WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidGetAcceptExSockAddrs, sizeof(GuidGetAcceptExSockAddrs),
        &m_GetAcceptExSockAddrs, sizeof(m_GetAcceptExSockAddrs),
        &dwBytes, NULL, NULL)) {
        m_HandleData.GetAcceptExSockAddrs = m_GetAcceptExSockAddrs = NULL;
        m_dwError = WSAGetLastError();
        TRACE(TEXT("��ȡGetAcceptExSockAddrʧ�ܣ�\n"));
        goto clean;
    }
    m_HandleData.AcceptEx = m_AcceptEx;
    m_HandleData.GetAcceptExSockAddrs = m_GetAcceptExSockAddrs;
    // AcceptExͶ��
    int i;
    for (i = 0; i<MAX_ACCEPT; i++) {
        pIoData = new PER_IO_DATA;
        pIoData->type = ACCEPT_POSTED;
        // ��ǰ������ͻ���ͨ�ŵ�socket
        pIoData->socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (INVALID_SOCKET != pIoData->socket) { 
            _stprintf(temp, TEXT("�ɹ��½���%d���ͻ����׽��֡�\n"), i + 1);
            TRACE(temp);
        }
        else {
            m_dwError = WSAGetLastError();
            _stprintf(temp, TEXT("�½���%d���ͻ����׽���ʧ�ܣ�\n"), i + 1);
            TRACE(temp);
            delete pIoData;
            continue;
        }
        if (FALSE == m_AcceptEx(m_socket, pIoData->socket,
            pIoData->wsaBuf.buf, 0,//pIoData->wsaBuf.len-(sizeof(SOCKADDR_IN)+16)*2,
            sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes, &pIoData->overlapped)) {
            if (WSA_IO_PENDING != WSAGetLastError()) {
                // AcceptExͶ��ʧ�ܣ���������ڴ�
                m_dwError = WSAGetLastError();
                _stprintf(temp, TEXT("Ͷ�ݵ�%d��AcceptExʧ��\n"), i + 1);
                TRACE(temp);
                closesocket(pIoData->socket);
                delete  pIoData;
                EnterCriticalSection(&m_cs);
                m_nFailAccepted++;
                if (MAX_ACCEPT == m_nFailAccepted) {
                    SetEvent(m_hListenClosed);
                }
                LeaveCriticalSection(&m_cs);
            }
            else {
                // AcceptExͶ�ݳɹ�
                _stprintf(temp, TEXT("�ɹ�Ͷ�ݵ�%d��AcceptEx��\n"), i + 1);
                TRACE(temp);
                EnterCriticalSection(&m_cs);
                m_HandleData.pIoArray[m_nAccepted] = pIoData;
                m_nAccepted++;
                LeaveCriticalSection(&m_cs);
            }
        }
    }

    goto end;

clean:
    ret = FALSE;

end:
    TRACE(TEXT("SocketListen()������\n"));
    return ret;
}