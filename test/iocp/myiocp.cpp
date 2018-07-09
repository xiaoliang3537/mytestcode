#include "stdafx.h"
#include "myiocp.h"


MYIOCP::MYIOCP()
{
    // 错误码
    m_dwError = 0;
    // 堆句柄
    m_hHeap = NULL;
    // 完成端口句柄
    m_hIocp = NULL;
    // 工作线程数
    m_nWorkers = 0;
    // 工作线程句柄数组指针
    m_phThread = NULL;
    // 工作线程参数数组指针
    m_pThreadParam = NULL;
    // 清理不活跃连接线程句柄 
    m_hCleanSocketThread = NULL;
    // 清理不活跃连接线程参数
    m_CleanThreadParam.pHandleData = &m_HandleData;
    m_CleanThreadParam.phQuitCleanThread = &m_hQuitCleanThread;
    // 运行状态
    m_bStart = FALSE;
    // 监听端口
    m_nPort = 1234;
    // 监听套接字
    m_socket = INVALID_SOCKET;
    // AcceptEx函数指针
    m_AcceptEx = NULL;
    // AcceptEx成功投递数
    m_nAccepted = 0;
    // AcceptEx失败投递数
    m_nFailAccepted = 0;
    // GetAcceptExSockAddr函数指针
    m_GetAcceptExSockAddrs = NULL;
    // 传递给GetQueuedCompletionStatus的上下文结构
    m_HandleData.phHeap = &m_hHeap;
    m_HandleData.pSocket = &m_socket;
    m_HandleData.pAccepted = &m_nAccepted;
    m_HandleData.pFailAccepted = &m_nFailAccepted;
    m_HandleData.phQuitEvent = &m_hQuitEvent;
    m_HandleData.phStopListen = &m_hStopListen;
    m_HandleData.phListenClosed = &m_hListenClosed;
    m_HandleData.pcs = &m_cs;
    m_HandleData.phIocp = &m_hIocp;
    // 退出工作线程通知事件
    m_hQuitEvent = NULL;
    // 停止监听事件
    m_hStopListen = NULL;
    // 关闭监听套接字处理完毕事件
    m_hListenClosed = NULL;
    // 退出清理连接线程通知事件
    m_hQuitCleanThread = NULL;

    m_PostData.pAccepted = &m_nAccepted;
    m_PostData.phQuitEvent = &m_hQuitEvent;

    // 初始化Socket库
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    // 初始化临界区
    InitializeCriticalSection(&m_cs);
    // 初始化堆
    m_hHeap = HeapCreate(0, 4096, 0);
}

// 析构函数
MYIOCP::~MYIOCP() {
    Clean();
    WSACleanup();
    DeleteCriticalSection(&m_cs);
    HeapDestroy(m_hHeap);
    m_hHeap = NULL;
}

// 扫尾、清理函数
void MYIOCP::Clean() {
    TCHAR temp[MAX_PATH];

    TRACE(TEXT("Clean()清理开始。\n"));
    if (m_bStart) {
        if (m_hIocp) {
            _stprintf(temp, TEXT("共有%d个AcceptEx投递失败。\n"), m_nFailAccepted);
            TRACE(temp);
            // 先关闭监听套接字，这样就不会有新的Accept连入
            // 关闭监听套接字不会影响已连接的客户端通知队列的正常处理
            TRACE(TEXT("关闭监听套接字。\n"));
            EnterCriticalSection(&m_cs);
            SetEvent(m_hStopListen);
            // 循环检测bAccepted，如果是未连接，把AcceptedTime=1
            // 用以区分和因为队列太多，此类通知长时间得不到处理，系统自动取出并返回错误。（在工作线程中）
            for (int q = 0; q<m_nAccepted; q++) {
                if (!m_HandleData.pIoArray[q]->bAccepted) {
                    m_HandleData.pIoArray[q]->AcceptedTime = 1;
                }
            }
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            LeaveCriticalSection(&m_cs);
            // 等待关闭监听套接字处理完毕通知
            TRACE(TEXT("等待关闭监听套接字处理完毕通知。\n"));
            while (WAIT_OBJECT_0 != WaitForSingleObject(m_hListenClosed, 0)) {
                _stprintf(temp, TEXT("还有%d个AcceptEx未退出。\n"), MAX_ACCEPT - m_nFailAccepted);
                TRACE(temp);
                Sleep(1000);
            }
            TRACE(TEXT("关闭监听套接字处理完毕通知返回。\n"));
            CloseHandle(m_hStopListen);
            m_hStopListen = NULL;
            CloseHandle(m_hListenClosed);
            m_hListenClosed = NULL;
            m_nFailAccepted = 0;
            // 关闭所有已连接的客户端socket
            TRACE(TEXT("关闭所有已连接的客户端socket。\n"));
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
            // 设置清理连接线程退出通知
            SetEvent(m_hQuitCleanThread);
            TRACE(TEXT("等待清理连接线程退出通知。\n"));
            WaitForSingleObject(m_hCleanSocketThread, INFINITE);
            TRACE(TEXT("清理连接线程退出通知返回。\n"));
            CloseHandle(m_hCleanSocketThread);
            m_hCleanSocketThread = NULL;
            CloseHandle(m_hQuitCleanThread);
            m_hQuitCleanThread = NULL;
            // 发送工作线程退出通知，检测m_nAccepted是否为0
            TRACE(TEXT("等待所有socket处理完毕通知。\n"));
            while (WAIT_OBJECT_0 != WaitForSingleObject(m_hQuitEvent, 0)) {
                _stprintf(temp, TEXT("还有%d个socket正在处理中。\n"), m_nAccepted);
                TRACE(temp);
                PostQueuedCompletionStatus(m_hIocp, 0, (DWORD)&m_PostData, 0);
                Sleep(1000);
            }
            TRACE(TEXT("所有socket处理完毕通知返回。\n"));
            ZeroMemory(m_HandleData.pIoArray, sizeof(m_HandleData.pIoArray));
            // 如果客户端连接数为0，说明已经清理完遗留数据
            // 发送退出线程通知，让工作线程退出
            TRACE(TEXT("发送工作线程退出通知。\n"));
            for (int i = 0; i<m_nWorkers; i++) {
                PostQueuedCompletionStatus(m_hIocp, 0, (DWORD)&m_PostData, 0);
            }
            // 等待所有工作线程退出
            TRACE(TEXT("等待所有工作线程退出通知。\n"));
            WaitForMultipleObjects(m_nWorkers, m_phThread, TRUE, INFINITE);
            TRACE(TEXT("所有工作线程退出通知返回。\n"));
            for (int k = 0; k<m_nWorkers; k++) {
                CloseHandle(m_phThread[k]);
            }
            CloseHandle(m_hQuitEvent);
            m_hQuitEvent = NULL;
            CloseHandle(m_hIocp);
            m_hIocp = NULL;
        }

        // 清理线程句柄
        if (m_phThread) {
            delete[] m_phThread;
            m_phThread = NULL;
        }

        m_bStart = FALSE;
    }
    TRACE(TEXT("Clean()清理结束。\n"));
}

// 设置监听端口
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

// 设置Accept投递函数
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

// 设置Recv投递函数
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

// 设置Send投递函数
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

// 运行
BOOL MYIOCP::Start() {
    BOOL ret = TRUE;
    UINT dwId;

    TRACE(TEXT("Start()开始。\n"));
    if (!m_bStart) {
        // 创建完成端口句柄
        TRACE(TEXT("创建m_hIocp完成端口。\n"));
        m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (NULL == m_hIocp) {
            m_dwError = GetLastError();
            TRACE(TEXT("创建m_hIocp完成端口失败！\n"));
            goto clean;
        }
        // 创建工作线程
        StartWorkerThread(m_nWorkers);
        // 创建清理不活跃连接线程
        m_hCleanSocketThread = (HANDLE)_beginthreadex(NULL, 0, CleanSocketThread, (LPVOID)&m_CleanThreadParam, 0, &dwId);
        if (NULL == m_hCleanSocketThread) {
            TRACE(TEXT("新建CleanSocketThread线程失败！\n"));
        }
        else {
            TRACE(TEXT("成功新建CleanSocketThread线程。\n"));
        }
        // 创建套接字监听端口并绑定到完成端口
        if (!SocketListen(m_nPort)) {
            // SocketListen内部已设置m_dwError错误值。
            TRACE(TEXT("SocketListen()返回失败！\n"));
            goto clean;
        }

        // 创建退出工作线程通知事件
        TRACE(TEXT("创建m_hQuitEvent事件。\n"));
        m_hQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == m_hQuitEvent) {
            m_dwError = GetLastError();
            TRACE(TEXT("创建m_hQuitEvent事件失败！\n"));
            goto clean;
        }

        // 创建停止监听通知事件
        TRACE(TEXT("创建m_hStopListen事件。\n"));
        m_hStopListen = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == m_hStopListen) {
            m_dwError = GetLastError();
            TRACE(TEXT("创建m_hStopListen事件失败！\n"));
            goto clean;
        }

        // 创建关闭监听套接字处理完毕通知事件
        TRACE(TEXT("创建m_hListenClosed事件。\n"));
        m_hListenClosed = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == m_hListenClosed) {
            m_dwError = GetLastError();
            TRACE(TEXT("创建m_hListenClosed事件失败！\n"));
            goto clean;
        }

        // 创建退出清理连接线程通知事件
        TRACE(TEXT("创建m_hQuitCleanThread事件。\n"));
        m_hQuitCleanThread = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == m_hQuitCleanThread) {
            m_dwError = GetLastError();
            TRACE(TEXT("创建m_hQuitCleanThread事件失败！\n"));
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
        TRACE(TEXT("Start()成功返回。\n"));
    }
    else {
        TRACE(TEXT("Start()失败返回。\n"));
    }
    return ret;
}

// 停止
BOOL MYIOCP::Stop() {
    BOOL ret = TRUE;

    TRACE(TEXT("Stop()开始。\n"));
    if (m_bStart) {
        Clean();
        m_bStart = FALSE;
    }

    TRACE(TEXT("Stop()结束。\n"));
    return ret;
}

// 启动工作线程
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
            // 新建线程失败
            _stprintf(temp, TEXT("新建第%d个工作线程失败！\n"), i + 1);
            TRACE(temp);
            i--;
            m_nWorkers--;
            continue;
        }
        else {
            _stprintf(temp, TEXT("成功新建第%d个工作线程。\n"), i + 1);
            TRACE(temp);
        }
    }
}

// 清理维护PER_HANDLE_DATA中的IO数据指针队列
void CleanIoDataQueued(PER_HANDLE_DATA *pHandleData, PER_IO_DATA *pIoData) {
    TCHAR temp[MAX_PATH];

    TRACE(TEXT("CleanIoDataQueued()开始。\n"));
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
            _stprintf(temp, TEXT("成功清理pIoArrary[%d](%08X)。\n"), i, (DWORD)pIoData);
            TRACE(temp);
            break;
        }
    }
    LeaveCriticalSection(pHandleData->pcs);
    TRACE(TEXT("CleanIoDataQueued()结束。\n"));
}

// 清理不活跃连接线程
UINT WINAPI CleanSocketThread(LPVOID param) {
    CLEAN_THREAD_PARAM *CleanData = (CLEAN_THREAD_PARAM*)param;
    PER_HANDLE_DATA *pHandleData = CleanData->pHandleData;
    time_t second;
    TCHAR temp[MAX_PATH], szText[MAX_PATH];
    char *szIP;
    int port;

    TRACE(TEXT("清理不活跃连接线程开始运行。\n"));

    while (TRUE) {
        if (WAIT_OBJECT_0 == WaitForSingleObject(*CleanData->phQuitCleanThread, 0)) {
            break;
        }
        EnterCriticalSection(pHandleData->pcs);
        for (int i = 0; i<(*pHandleData->pAccepted); i++) {
            if (pHandleData->pIoArray[i]->AcceptedTime) {
                if (NULL == pHandleData->pIoArray[i]->LastRecvTime) {
                    // 空连接
                    second = pHandleData->pIoArray[i]->AcceptedTime;
                }
                else {
                    // 死连接或长时间不活跃连接
                    second = pHandleData->pIoArray[i]->LastRecvTime;
                }
                if (time(NULL) - second > MAX_SECONDS) {
                    if (0 == pHandleData->pIoArray[i]->LoginId) {
                        // 关闭未登陆的超时连接
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
                        _stprintf(temp, TEXT("清理不活跃连接：%s:%d。\n"), szText, port);
                        TRACE(temp);
                        if (!pHandleData->pIoArray[i]->bClose) {
                            pHandleData->pIoArray[i]->bClose = TRUE;
                            closesocket(pHandleData->pIoArray[i]->socket);
                        }
                    }
                    else {
                        // 登陆帐户受限时间不同（超时为2小时）
                        if (time(NULL) - second > 60 * 60 * 2) {
                            _stprintf(temp, TEXT("LoginId:%d帐户未活动2小时被关闭。"), pHandleData->pIoArray[i]->LoginId);
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

    TRACE(TEXT("清理不活跃连接线程结束退出。\n"));
    return 0;
}

// 工作线程
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

    _stprintf(temp, TEXT("第%d个WorkerThread开始运行。\n"), num);
    TRACE(temp);
    while (TRUE) {
        ret = GetQueuedCompletionStatus(hIocp, &dwBytes, (DWORD*)&pHandle, (LPOVERLAPPED*)&pIoData, INFINITE);
        /*
        关于异步接收的几个概念：
        1.完成端口通知队列由Windows自己维护：
        因为服务端是异步的，所以当客户端（使用同步阻塞，非阻塞未测试）connect或是send返回成功时，
        Windows只是把客户端的请求数据保存在系统缓存(好像默认大小4K)中，并将通知添加到队列等待我们获取。
        注：多说几句，这里客户端send的次数和添加到队列的通知不是1:1的关系，因为TCP是基于流的，
        比如客户端send发送了10次，到服务端这里因为算法的关系，可能把10次send的数据合并放在系统缓存里，只添加一个通知到队列。
        再如客户端send只发送1次（但数据较多），系统把数据全部保存在系统缓存后，发现指定缓存(WSARecv buf)太小，于是添加多个通知到队列。
        这个队列我猜想应该是先进先出的模式。PostQueuedCompletionStatus向队列添加自定义通知。
        GetQueuedCompletionStatus告诉系统从队列中取出一个通知，并将这个通知接收到的数据（在系统缓存中）
        拷入到我们指定的缓存中，然后在系统缓存中这部分数据就被标明无效，抛弃了。
        GetQueuedCompletionStatus返回FALSE的话，没有数据拷贝这一步。还有自定义通知也不存在拷贝操作。
        如果外部请求很多服务端程序来不及处理，导致系统缓存满了，客户端的请求都会被挂起，直到有可用的系统缓存。
        2.GetQueuedCompletionStatus什么情况下会返回：
        一、之前使用WSARecv,AcceptEx投递，并且队列中有与其相关通知。
        投递一次取一次。
        ---------------------------------------------------------------------------------------------
        客户端优雅退出(使用shutdown然后closesocket，或无数据传输时closesocket)，退出通知加入队列。
        ---------------------------------------------------------------------------------------------
        如客端暴力退出（如强关程序、或者还有数据传输的时候强制closesocket），
        未处理的通各队列会被系统清除。如有通知正在处理，暴力退出的通知不加入队列，否则进队列。
        服务端关闭与客户端套接字，规则同暴力退出。
        ---------------------------------------------------------------------------------------------
        服务端关闭监听套接字。不会清理正在等待处理的AcceptEx通知。有几个未处理的AcceptEx向队列添加几个通知。
        高优先级，关闭之后，工作线程先优先处理等待处理的AcceptEx通知，再处理已经投递的AcceptEx的通知。然后
        处理WSARecv通知。
        ---------------------------------------------------------------------------------------------

        二、有自定义通知。并且自义通知优先级最高，优先取自定义消息。
        三、关闭完成端口句柄。（通知队列死循环）

        */
        if (ret) {
            // 成功返回
            if (0 == dwBytes) {
                if (pIoData) {
                    if (pIoData->bAccepted) {
                        // 客户端使用closesocket优雅关闭连接
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
                        // AcceptEx：客户端请求连接
                        AcceptHandle(pHandle, pIoData, dwBytes);
                    }
                }
                else {
                    // 程序主动调用PostQueuedCompletionStatus通知工作线程退出
                    if (0 == *((POST_DATA*)pHandle)->pAccepted) {
                        SetEvent(*((POST_DATA*)pHandle)->phQuitEvent);
                        break;
                    }
                }
            }
            else {
                // 正常处理流程
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
            // 意外返回
            dwError = GetLastError();
            if (pIoData) {
                // 1.客端暴力退出（如强关程序、或者还有数据传输的时候强制closesocket），dwError=64。
                // 2.服务端关闭与客户端套接字（closesocket），dwError=1236。
                // 3.服务端关闭监听套接字，dwError=995。
                // 4.通知队列太多，如果一个队列迟迟得不到处理，系统会把它取出并返回FALSE，
                EnterCriticalSection(pHandle->pcs);
                if (!pIoData->bClose) {
                    // 情况1或3、4
                    // 就是说除了2以外都关闭清除socket（因为2之前已经主动调用了closesocket）
                    closesocket(pIoData->socket);
                }
                if (pIoData->pMemory) {
                    HeapFree(*pHandle->phHeap, 0, pIoData->pMemory);
                    pIoData->pMemory = NULL;
                }
                LeaveCriticalSection(pHandle->pcs);
                CleanIoDataQueued(pHandle, pIoData);
                if (!pIoData->bAccepted) {
                    // 如果是AcceptEx投递未被处理过
                    if (1 == pIoData->AcceptedTime) {
                        // 并且是由关闭监听套接字引起的
                        EnterCriticalSection(pHandle->pcs);
                        (*pHandle->pFailAccepted)++;
                        if (MAX_ACCEPT == *pHandle->pFailAccepted) {
                            SetEvent(*pHandle->phListenClosed);
                        }
                        LeaveCriticalSection(pHandle->pcs);
                    }
                    else {
                        // 否则是AcceptEx通知长时间得不到处理（因为队列太多），系统自动取出并返回错误
                        // 投递一个新的AcceptEx
                        PER_IO_DATA *pNewIoData = new PER_IO_DATA;
                        pNewIoData->type = ACCEPT_POSTED;
                        // 提前建立与客户端通信的socket
                        pNewIoData->socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
                        if (INVALID_SOCKET == pNewIoData->socket) {
                            TRACE(TEXT("新建客户端通信socket失败，无法进行AcceptEx投递。(WorkerThread里)\n"));
                            delete pNewIoData;
                        }
                        else {
                            EnterCriticalSection(pHandle->pcs);
                            if (WAIT_OBJECT_0 != WaitForSingleObject(*pHandle->phStopListen, 0)) {
                                if (TRUE == ((pHandle->AcceptEx)(*pHandle->pSocket, pNewIoData->socket,
                                    pNewIoData->wsaBuf.buf, 0,//pNewIoData->wsaBuf.len-(sizeof(SOCKADDR_IN)+16)*2,
                                    sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwTemp, &pNewIoData->overlapped)) ||
                                    WSA_IO_PENDING == WSAGetLastError()) {
                                    // 成功
                                    pHandle->pIoArray[*pHandle->pAccepted] = pNewIoData;
                                    (*pHandle->pAccepted)++;
                                }
                                else {
                                    // AcceptEx投递失败，清除分配内存
                                    TRACE(TEXT("AcceptEx投递失败！(WorkerThread里)\n"));
                                    closesocket(pNewIoData->socket);
                                    delete  pNewIoData;
                                    (*pHandle->pFailAccepted)++;
                                    if (MAX_ACCEPT == *pHandle->pFailAccepted) {
                                        SetEvent(*pHandle->phListenClosed);
                                    }
                                }
                            }
                            else {
                                TRACE(TEXT("检测到停止监听，终止AcceptEx投递！(WorkerThread里)"));
                                closesocket(pNewIoData->socket);
                                delete  pNewIoData;
                                (*pHandle->pFailAccepted)++;
                                if (MAX_ACCEPT == *pHandle->pFailAccepted) {
                                    SetEvent(*pHandle->phListenClosed);
                                }
                            }
                            LeaveCriticalSection(pHandle->pcs);
                        }
                        // AcceptEx结束
                    }
                }
                delete pIoData;
            }
            else {
                // 1.关闭完成端口句柄
                // 2.其他未知原因意外退出
                _stprintf(temp, TEXT("GetQueuedCompletionStatus()未知原因错误返回!GetLastError:%d。\n"), dwError);
                TRACE(temp);
            }
            continue;
        }
    }

    _stprintf(temp, TEXT("第%d个WorkerThread结束退出。\n"), num);
    TRACE(temp);
    return 0;
}

void AcceptHandle(PER_HANDLE_DATA *pHandleData, PER_IO_DATA *pIoData, DWORD dwBytes) {
    DWORD dwTemp;
    DWORD dwFlags = 0;
    int LocalLen, ClientLen;
    LocalLen = ClientLen = sizeof(SOCKADDR_IN);
    BOOL bWSARecv;

    TRACE(TEXT("进入AcceptHandle()。\n"));
    // 投递一个新的AcceptEx
    PER_IO_DATA *pNewIoData = new PER_IO_DATA;
    pNewIoData->type = ACCEPT_POSTED;
    // 提前建立与客户端通信的socket
    pNewIoData->socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == pNewIoData->socket) {
        TRACE(TEXT("新建客户端通信socket失败，无法进行AcceptEx投递。(AcceptHandle里)\n"));
        delete pNewIoData;
    }
    else {
        EnterCriticalSection(pHandleData->pcs);
        if (WAIT_OBJECT_0 != WaitForSingleObject(*pHandleData->phStopListen, 0)) {
            if (TRUE == ((pHandleData->AcceptEx)(*pHandleData->pSocket, pNewIoData->socket,
                pNewIoData->wsaBuf.buf, 0,//pNewIoData->wsaBuf.len-(sizeof(SOCKADDR_IN)+16)*2,
                sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwTemp, &pNewIoData->overlapped)) ||
                WSA_IO_PENDING == WSAGetLastError()) {
                // 成功
                pHandleData->pIoArray[*pHandleData->pAccepted] = pNewIoData;
                (*pHandleData->pAccepted)++;
            }
            else {
                // AcceptEx投递失败，清除分配内存
                TRACE(TEXT("AcceptEx投递失败！(AcceptHandle里)\n"));
                closesocket(pNewIoData->socket);
                delete  pNewIoData;
                (*pHandleData->pFailAccepted)++;
                if (MAX_ACCEPT == *pHandleData->pFailAccepted) {
                    SetEvent(*pHandleData->phListenClosed);
                }
            }
        }
        else {
            TRACE(TEXT("检测到停止监听，终止AcceptEx投递！(AcceptHandle里)"));
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
    // 将客户端socket绑定完成端口（接收WSARecv投递）
    CreateIoCompletionPort((HANDLE)(pIoData->socket), *pHandleData->phIocp, (DWORD)pHandleData, 0);

    // WSARecv投递
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
        // WSARecv投递失败
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
    TRACE(TEXT("退出AcceptHandle()。\n"));
}

void RecvHandle(PER_HANDLE_DATA *pHandleData, PER_IO_DATA *pIoData, DWORD dwBytes) {
    DWORD dwTemp;
    DWORD dwFlags = 0;
    LPIOCPFUN RecvFun;
    BOOL bWSARecv;

    TRACE(TEXT("进入RecvHandle()。\n"));
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
        // WSARecv投递失败
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
    TRACE(TEXT("退出RecvHandle()。\n"));
}

// 创建套接字监听端口并绑定到完成端口
BOOL MYIOCP::SocketListen(int port) {
    TCHAR temp[MAX_PATH];
    BOOL ret = TRUE;
    PER_IO_DATA *pIoData;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
    DWORD dwBytes;

    TRACE(TEXT("SocketListen()开始。\n"));
    // TCP流
    TRACE(TEXT("WSASocket()新建监听套接字。\n"));
    m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == m_socket) {
        m_dwError = WSAGetLastError();
        TRACE(TEXT("WSASocket()返回失败！\n"));
        goto clean;
    }
    struct sockaddr_in ServerAddr;
    ZeroMemory(&ServerAddr, sizeof(sockaddr_in));
    ServerAddr.sin_family = AF_INET;
    // 监听本地所有网卡
    ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    ServerAddr.sin_port = htons(port);
    TRACE(TEXT("bind()绑定端口。\n"));
    if (SOCKET_ERROR == bind(m_socket, (struct sockaddr*)&ServerAddr, sizeof(sockaddr_in))) {
        m_dwError = WSAGetLastError();
        _stprintf(temp, TEXT("绑定%d端口失败！\n"), port);
        TRACE(temp);
        goto clean;
    }
    TRACE(TEXT("listen()开始监听。\n"));
    if (SOCKET_ERROR == listen(m_socket, SOMAXCONN)) {
        m_dwError = WSAGetLastError();
        TRACE(TEXT("listen()返回失败！\n"));
        goto clean;
    }
    // 绑定到完成端口
    TRACE(TEXT("监听套接字绑定到完成端口。\n"));
    if (NULL == CreateIoCompletionPort((HANDLE)m_socket, m_hIocp, (DWORD)&m_HandleData, 0)) {
        m_dwError = GetLastError();
        TRACE(TEXT("创建完成端口失败！\n"));
        goto clean;
    }

    // 获取AcceptEx函数指针
    TRACE(TEXT("获取AcceptEx函数指针。\n"));
    if (SOCKET_ERROR == WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidAcceptEx, sizeof(GuidAcceptEx), &m_AcceptEx, sizeof(m_AcceptEx),
        &dwBytes, NULL, NULL)) {
        m_HandleData.AcceptEx = m_AcceptEx = NULL;
        m_dwError = WSAGetLastError();
        TRACE(TEXT("获取AcceptEx失败！\n"));
        goto clean;
    }
    // 获取GetAcceptExSockAddr函数指针
    TRACE(TEXT("获取GetAcceptExSockAddr函数指针。\n"));
    if (SOCKET_ERROR == WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidGetAcceptExSockAddrs, sizeof(GuidGetAcceptExSockAddrs),
        &m_GetAcceptExSockAddrs, sizeof(m_GetAcceptExSockAddrs),
        &dwBytes, NULL, NULL)) {
        m_HandleData.GetAcceptExSockAddrs = m_GetAcceptExSockAddrs = NULL;
        m_dwError = WSAGetLastError();
        TRACE(TEXT("获取GetAcceptExSockAddr失败！\n"));
        goto clean;
    }
    m_HandleData.AcceptEx = m_AcceptEx;
    m_HandleData.GetAcceptExSockAddrs = m_GetAcceptExSockAddrs;
    // AcceptEx投递
    int i;
    for (i = 0; i<MAX_ACCEPT; i++) {
        pIoData = new PER_IO_DATA;
        pIoData->type = ACCEPT_POSTED;
        // 提前建立与客户端通信的socket
        pIoData->socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (INVALID_SOCKET != pIoData->socket) { 
            _stprintf(temp, TEXT("成功新建第%d个客户端套接字。\n"), i + 1);
            TRACE(temp);
        }
        else {
            m_dwError = WSAGetLastError();
            _stprintf(temp, TEXT("新建第%d个客户端套接字失败！\n"), i + 1);
            TRACE(temp);
            delete pIoData;
            continue;
        }
        if (FALSE == m_AcceptEx(m_socket, pIoData->socket,
            pIoData->wsaBuf.buf, 0,//pIoData->wsaBuf.len-(sizeof(SOCKADDR_IN)+16)*2,
            sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes, &pIoData->overlapped)) {
            if (WSA_IO_PENDING != WSAGetLastError()) {
                // AcceptEx投递失败，清除分配内存
                m_dwError = WSAGetLastError();
                _stprintf(temp, TEXT("投递第%d个AcceptEx失败\n"), i + 1);
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
                // AcceptEx投递成功
                _stprintf(temp, TEXT("成功投递第%d个AcceptEx。\n"), i + 1);
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
    TRACE(TEXT("SocketListen()结束。\n"));
    return ret;
}