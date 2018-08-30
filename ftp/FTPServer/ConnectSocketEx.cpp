#include "stdafx.h"
#include "ConnectSocketEx.h"
#include "DataSocketEx.h"
#include "UserManager.h"

#include <winsock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")
using std::cout;
using std::cin;
using std::endl;
using std::ends;

//extern CFTPServer theServer;
extern CUserManager g_UserManager;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/********************************************************************/
/*																	*/
/* Function name : CConnectSocket::CConnectSocket					*/
/* Description   : Constructor										*/
/*																	*/
/********************************************************************/
CConnectSocketEx::CConnectSocketEx()
{
    m_bLoggedon = FALSE;
    m_bRenameFile = FALSE;
    m_pDataSocket = NULL;
    m_strRemoteHost = "";
    m_nRemotePort = -1;
    m_dwRestartOffset = 0;
    m_bPassiveMode = FALSE;
    m_bTerminate = FALSE;
}


/********************************************************************/
/*																	*/
/* Function name : CConnectSocket::~CConnectSocket					*/
/* Description   : Destructor										*/
/*																	*/
/********************************************************************/
CConnectSocketEx::~CConnectSocketEx()
{
    DestroyDataConnection();

    // tell our thread we have been closed
    //AfxGetThread()->PostThreadMessage(WM_QUIT,0,0);

    TRACE0("CConnectSocket destroyed.\n");
}


// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(CConnectSocket, CSocket)
    //{{AFX_MSG_MAP(CConnectSocket)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif	// 0


/********************************************************************/
/*																	*/
/* Function name : OnClose											*/
/* Description   : Send WM_QUIT message to the thread containing	*/
/*				   the socket to shutdown once the connection is	*/
/*                 closed.											*/
/*																	*/
/********************************************************************/
void CConnectSocketEx::OnClose(int nErrorCode)
{
    TRACE("CConnectSocketEx() OnClose()\n");
    m_bTerminate = TRUE;
}


#define BUFFERSIZE 4096

/********************************************************************/
/*																	*/
/* Function name : OnReceive										*/
/* Description   : Called by the framework to notify this socket	*/
/*                 that there is data in the buffer.				*/
/*																	*/
/********************************************************************/
void CConnectSocketEx::OnReceive(int nErrorCode)
{
    CHAR buff[BUFFERSIZE];

    int nRead = recv(m_socket, buff, BUFFERSIZE, 0);
    switch (nRead)
    {
    case 0:
        Close();
        break;

    case SOCKET_ERROR:
        if (GetLastError() != WSAEWOULDBLOCK)
        {
            TCHAR szError[256];
            wsprintf(szError, _T("OnReceive error: %d"), GetLastError());

            AfxMessageBox(szError);
        }
        break;

    default:
        if (nRead != SOCKET_ERROR && nRead != 0)
        {
            //((CConnectThread *)AfxGetThread())->IncReceivedBytes(nRead);

            // terminate the string
            buff[nRead] = 0;
            m_RxBuffer += CString(buff);

            GetRxLine();
        }
        break;
    }
}


/********************************************************************/
/*																	*/
/* Function name: GetRxCommand										*/
/* Description  : Get command from receiver buffer.					*/
/*																	*/
/********************************************************************/
BOOL CConnectSocketEx::GetRxCommand(CString &strCommand, CString &strArguments)
{
    if (!m_strCommands.IsEmpty())
    {
        CString strBuff = m_strCommands.RemoveHead();
        int nIndex = strBuff.Find(_T(" "));
        if (nIndex != -1)
        {
            CString strPassword = strBuff;
            strPassword.MakeUpper();
            // make password invisible
            if (strPassword.Left(5) == "PASS ")
            {
                for (int i = 5; i < strPassword.GetLength(); i++)
                {
                    strPassword.SetAt(i, '*');
                }
                //FireStatusMessage(strPassword, 1);
            }
            else
            {
                //FireStatusMessage(strBuff, 1);
            }
            strCommand = strBuff.Left(nIndex);
            strArguments = strBuff.Mid(nIndex + 1);
        }
        else
        {
            //FireStatusMessage(strBuff, 1);
            strCommand = strBuff;
        }

        if (strCommand != "")
        {
            strCommand.MakeUpper();

            // who screwed up ???
            if (strCommand.Right(4) == "ABOR")
            {
                strCommand = "ABOR";
            }

            TRACE2("COMMAND: %s, ARGS: %s\n", strCommand, strArguments);
            return TRUE;
        }
    }
    return FALSE;
}


/********************************************************************/
/*																	*/
/* Function name: GetRxLine											*/
/* Description  : Parse complete command line						*/
/*																	*/
/********************************************************************/
void CConnectSocketEx::GetRxLine()
{
    CString strTemp;
    int nIndex;

    while (!m_RxBuffer.IsEmpty())
    {
        nIndex = m_RxBuffer.Find(_T("\r\n"));
        if (nIndex != -1)
        {
            strTemp = m_RxBuffer.Left(nIndex);
            m_RxBuffer = m_RxBuffer.Mid(nIndex + 2);
            if (!strTemp.IsEmpty())
            {
                m_strCommands.AddTail(strTemp);
                // parse and execute command
                ParseCommand();
            }
        }
        else
            break;
    }
}


/********************************************************************/
/*																	*/
/* Function name: OnConnect											*/
/* Description  : Called by the framework to notify this connecting	*/
/*				  socket that its connection attempt is completed.  */
/*																	*/
/********************************************************************/
void CConnectSocketEx::OnConnect(int nErrorCode)
{
    //CSocket::OnConnect(nErrorCode);
}


/********************************************************************/
/*																	*/
/* Function name: HasConnectionDropped								*/
/* Description  : Check if connection has been dropped.				*/
/*				  Used to detect if client has crashed.				*/
/*																	*/
/********************************************************************/
BOOL CConnectSocketEx::HasConnectionDropped(void)
{
    BOOL bConnDropped = FALSE;
    /*INT iRet = 0;
    BOOL bOK = TRUE;

    if (m_hSocket == INVALID_SOCKET)
    return TRUE;

    struct timeval timeout = { 0, 0 };
    fd_set readSocketSet;

    FD_ZERO(&readSocketSet);
    FD_SET(m_hSocket, &readSocketSet);

    iRet = ::select(0, &readSocketSet, NULL, NULL, &timeout);
    bOK = (iRet > 0);

    if(bOK)
    {
    bOK = FD_ISSET(m_hSocket, &readSocketSet);
    }

    if(bOK)
    {
    CHAR szBuffer[1] = "";
    iRet = ::recv(m_hSocket, szBuffer, 1, MSG_PEEK);
    bOK = (iRet > 0);
    if(!bOK)
    {
    INT iError = ::WSAGetLastError();
    bConnDropped = (( iError == WSAENETRESET) ||
    (iError == WSAECONNABORTED) ||
    (iError == WSAECONNRESET) ||
    (iError == WSAEINVAL) ||
    (iRet == 0));
    }
    }*/
    return(bConnDropped);
}

int CConnectSocketEx::GetSockName(CString &strIP, UINT &nPort)
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

void CConnectSocketEx::Attach(SOCKET sock)
{
    this->m_socket = sock;
    m_scoketEvent = ::WSACreateEvent();
    ::WSAEventSelect(sock, m_scoketEvent, FD_READ | FD_CLOSE | FD_WRITE);

    m_hThread = ::CreateThread(0, 0, RecvThreadProc, (void *)this, 0, 0);
    SendResponse("220 %s", _T("welcome"));
}

DWORD WINAPI CConnectSocketEx::RecvThreadProc(LPVOID lpParam)
{
    if (lpParam == NULL)
        return 0;
    CConnectSocketEx *client = (CConnectSocketEx *)lpParam;
    WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS];                   // 事件对象数组
    SOCKET sockArray[WSA_MAXIMUM_WAIT_EVENTS];                      // 事件对象数组对应的SOCKET句柄
    DWORD ret = 0;
    int index = 0;
    int nEvent = 0;

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
        if (nIndex == WSA_WAIT_IO_COMPLETION )
        {
            std::cout << "wait error ! error code :" << WSAGetLastError() << endl;
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
                cout << "A Client has disconnect" << endl;
                client->Close();
            }
        }
    }
    ::closesocket(client->m_socket);
    delete client;
    return 0;
}

/********************************************************************/
/*																	*/
/* Function name: SendResponse										*/
/* Description  : Send response to client.							*/
/*																	*/
/********************************************************************/
BOOL CConnectSocketEx::SendResponse(const char* pstrFormat, ...)
{
    CString str;

    // format arguments and put them in CString
    va_list args;
    va_start(args, pstrFormat);
    str.FormatV(stringToTChar(pstrFormat), args);

    // is connection still active ?
    if (HasConnectionDropped())
    {
        //FireStatusMessage("Could not send reply, disconnected.", 2);
        Close();
        // tell our thread we have been closed
        // destroy connection
        //m_pThread->PostThreadMessage(WM_THREADMSG, 1, 0);
        return FALSE;
    }
    std::string info = TCharToString(str);
    info += "\r\n";
    int nBytes = send(this->m_socket, info.c_str() , info.length() , 0);
    if (nBytes == SOCKET_ERROR)
    {
        Close();
        //FireStatusMessage("Could not send reply, disconnected.", 2);
        // tell our thread we have been closed
        //m_pThread->PostThreadMessage(WM_THREADMSG, 1, 0);
        return FALSE;
    }

    //FireStatusMessage(str, 2);

    //((CConnectThread *)AfxGetThread())->IncSentBytes(nBytes);
    return TRUE;
}


/********************************************************************/
/*																	*/
/* Function name: ParseCommand										*/
/* Description  : Parse and execute command from client.			*/
/*																	*/
/* Based on code provided by FileZilla Server.						*/
/* http://sourceforge.net/projects/filezilla						*/
/*																	*/
/********************************************************************/
void CConnectSocketEx::ParseCommand()
{
    static CFTPCommand commandList[] =
    {
        { TOK_ABOR,	"ABOR", FALSE,	"Abort transfer: ABOR" },
        { TOK_BYE,	"BYE",  FALSE,	"Logout or break the connection: BYE" },
        { TOK_CDUP,	"CDUP", FALSE,	"Change to parent directory: CDUP" },
        { TOK_CWD,	"CWD",	TRUE,	"Change working directory: CWD [directory-name]" },
        { TOK_DELE,	"DELE", TRUE ,	"Delete file: DELE file-name" },
        { TOK_DIR,	"DIR",  FALSE,	"Get directory listing: DIR [path-name]" },
        { TOK_HELP,	"HELP",  FALSE, "Show help: HELP [command]" },
        { TOK_LIST,	"LIST", FALSE,	"Get directory listing: LIST [path-name]" },
        { TOK_MKD,	"MKD",	TRUE,	"Make directory: MKD path-name" },
        { TOK_NOOP,	"NOOP", FALSE,	"Do nothing: NOOP" },
        { TOK_PASS,	"PASS", TRUE,	"Supply a user password: PASS password" },
        { TOK_PASV,	"PASV", FALSE,	"Set server in passive mode: PASV" },
        { TOK_PORT,	"PORT", TRUE,	"Specify the client port number: PORT a0,a1,a2,a3,a4,a5" },
        { TOK_PWD,	"PWD",	FALSE,	"Get current directory: PWD" },
        { TOK_QUIT,	"QUIT",  FALSE,	"Logout or break the connection: QUIT" },
        { TOK_REST,	"REST", TRUE,	"Set restart transfer marker: REST marker" },
        { TOK_RETR,	"RETR", TRUE,	"Get file: RETR file-name" },
        { TOK_RMD,	"RMD",	TRUE,	"Remove directory: RMD path-name" },
        { TOK_RNFR,	"RNFR", TRUE,	"Specify old path name of file to be renamed: RNFR file-name" },
        { TOK_RNTO,	"RNTO", TRUE,	"Specify new path name of file to be renamed: RNTO file-name" },
        { TOK_SIZE,	"SIZE", TRUE,	"Get filesize: SIZE file-name" },
        { TOK_STOR,	"STOR", TRUE,	"Store file: STOR file-name" },
        { TOK_SYST,	"SYST", FALSE,	"Get operating system type: SYST" },
        { TOK_TYPE,	"TYPE", TRUE,	"Set filetype: TYPE [A | I]" },
        { TOK_USER,	"USER", TRUE,	"Supply a username: USER username" },
        { TOK_ERROR,	"",		FALSE,  "" },
    };

    // parse command
    CString strCommand, strArguments;
    if (!GetRxCommand(strCommand, strArguments))
    {
        return;
    }

    int nCommand;

    // find command in command list
    for (nCommand = TOK_ABOR; nCommand < TOK_ERROR; nCommand++)
    {
        // found command ?
        if (strCommand == commandList[nCommand].m_pszName)
        {
            // did we expect an argument ?
            if (commandList[nCommand].m_bHasArguments && (strArguments.IsEmpty()))
            {
                SendResponse("501 Syntax error: Invalid number of parameters.");
                return;
            }
            break;
        }
    }

    if (nCommand == TOK_ERROR)
    {
        // command is not in our list
        SendResponse("501 Syntax error: Command not understood.");
        return;
    }

    // no commands are excepted before successfull logged on
    if ((nCommand != TOK_USER && nCommand != TOK_PASS) && !m_bLoggedon)
    {
        SendResponse("530 Please login with USER and PASS.");
        return;
    }

    // proces command
    switch (nCommand)
    {
        // specify username
    case TOK_USER:
    {
        strArguments.MakeLower();
        m_bLoggedon = FALSE;
        m_strUserName = strArguments;

        CString strPeerAddress;
        UINT nPeerPort;
        GetPeerName(strPeerAddress, nPeerPort);

        // tell FTP server a new user has connected
        //CConnectThread *pThread = (CConnectThread *)m_pThread;
        //((CFTPServer *)pThread->m_pWndServer)->m_pEventSink->OnFTPUserConnected(m_pThread->m_nThreadID, m_strUserName, strPeerAddress);

        SendResponse("331 User name ok, need password.");
    }
    break;

    // specify password
    case TOK_PASS:
    {
        // already logged on ?
        if (m_bLoggedon)
        {
            SendResponse("503 Login with USER first.");
        }
        else
        {
            // now we have user name and password, attempt to login the client
            CUser user;
            // check username
            if (g_UserManager.GetUser(m_strUserName, user))
            {
                // check password
                if ((!user.m_strPassword.Compare(strArguments) || user.m_strPassword.IsEmpty()) && !user.m_bAccountDisabled)
                {
                    // set home directory of user
                    m_strCurrentDir = "/";

                    // succesfully logged on
                    m_bLoggedon = TRUE;
                    SendResponse("230 User successfully logged in.");
                    break;
                }
            }
            SendResponse("530 Not logged in, user or password incorrect!");
        }
    }
    break;

    // change transfer type
    case TOK_TYPE:
    {
        // let's pretend we did something...
        SendResponse("200 Type set to %s", strArguments);
    }
    break;

    // print current directory
    case TOK_PWD:
    {
        SendResponse("257 \"%s\" is current directory.", m_strCurrentDir);
    }
    break;

    // change to parent directory
    case TOK_CDUP:
        strArguments = "..";
        // change working directory
    case TOK_CWD:
    {
        // try to change to specified directory
        //int nResult = theServer.m_UserManager.ChangeDirectory(m_strUserName, m_strCurrentDir, strArguments);
        int nResult = g_UserManager.ChangeDirectory(m_strUserName, m_strCurrentDir, strArguments);
        switch (nResult)
        {
        case 0:
            SendResponse("250 CWD command successful. \"%s\" is current directory.", m_strCurrentDir);
            break;
        case 1:
            SendResponse("550 CWD command failed. \"%s\": Permission denied.", strArguments);
            break;
        default:
            SendResponse("550 CWD command failed. \"%s\": Directory not found.", strArguments);
            break;
        }
    }
    break;

    // specify IP and port (PORT a1,a2,a3,a4,p1,p2) -> IP address a1.a2.a3.a4, port p1*256+p2. 
    case TOK_PORT:
    {
        CString strSub;
        int nCount = 0;

        while (AfxExtractSubString(strSub, strArguments, nCount++, ','))
        {
            switch (nCount)
            {
            case 1:	// a1
                m_strRemoteHost = strSub;
                m_strRemoteHost += ".";
                break;
            case 2:	// a2
                m_strRemoteHost += strSub;
                m_strRemoteHost += ".";
                break;
            case 3:	// a3
                m_strRemoteHost += strSub;
                m_strRemoteHost += ".";
                break;
            case 4:	// a4
                m_strRemoteHost += strSub;
                break;
            case 5:	// p1
                m_nRemotePort = 256 * _ttoi(strSub); 
                break;
            case 6:	// p2
                m_nRemotePort += _ttoi(strSub);
                break;
            }
        }
        m_bPassiveMode = FALSE;
        SendResponse("200 Port command successful.");
        break;
    }

    // switch to passive mode
    case TOK_PASV:
    {
        // delete existing datasocket
        DestroyDataConnection();

        // create new data socket
        m_pDataSocket = new CDataSocketEx(this, -1);

        if (0 != m_pDataSocket->Create())
        {
        	DestroyDataConnection();	
        	SendResponse("421 Failed to create socket.");
        	break;
        }
        // start listening
        //m_pDataSocket->Listen();
        //m_pDataSocket->AsyncSelect();
        
        CString strIP, strTmp;
        UINT nPort;
        
        // get our ip address
        GetSockName(strIP, nPort);
        // retrieve port
        m_pDataSocket->GetSockName(strTmp, nPort);
        // replace dots 
        strTmp.Replace(_T("."),_T(","));
        // tell the client which address/port to connect to
        SendResponse("227 Entering Passive Mode (%s,%d,%d).", strIP, nPort/256, nPort%256);
        m_bPassiveMode = TRUE;
        break;
    }

    // list current directory (or a specified file/directory)
    case TOK_LIST:
    case TOK_DIR:
    {
        // if not PASV mode, we need a PORT comand first
        if (!m_bPassiveMode && (m_strRemoteHost == "" || m_nRemotePort == -1))
        {
            SendResponse("503 Bad sequence of commands.");
        }
        else
        {
            // if client did not specify a directory use current dir
            if (strArguments == "")
            {
                strArguments = m_strCurrentDir;
            }
            else
            {
                // check if argument is file or directory
                CString strResult;
                //int nResult = theServer.m_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_LIST, strResult);
                int nResult = g_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_LIST, strResult);
                if (nResult == 0)
                {
                    strArguments = strResult;
                }
            }
            CString strListing;
            //int nResult = theServer.m_UserManager.GetDirectoryList(m_strUserName, strArguments, strListing);
            int nResult = g_UserManager.GetDirectoryList(m_strUserName, strArguments, strListing);
            switch (nResult)
            {
            case 1:
                SendResponse("550 Permission denied.");
                break;
            case 2:
                SendResponse("550 Directory not found.");
                break;
            default:
                // create socket connection to transfer directory listing
                if (!CreateDataConnection(0, strListing))
                {
                    DestroyDataConnection();
                }
                break;
            }
        }
        break;
    }

    // retrieve file
    case TOK_RETR:
    {
        // if not PASV mode, we need a PORT comand first
        if (!m_bPassiveMode && (m_strRemoteHost == _T("") || m_nRemotePort == -1))
        {
            SendResponse("503 Bad sequence of commands.");
            break;
        }

        CString strResult;
        //int nResult = theServer.m_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_DOWNLOAD, strResult);
        int nResult = g_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_DOWNLOAD, strResult);
        switch (nResult)
        {
        case 1:
            SendResponse("550 Permission denied.");
            break;
        case 2:
            SendResponse("550 File not found.");
            break;
        default:
            // create socket connection for file transfer
            if (!CreateDataConnection(1, strResult))
            {
                DestroyDataConnection();
            }
            break;
        }
        break;
    }

    // client wants to upload file
    case TOK_STOR:
    {
        // if not PASV mode, we need a PORT comand first
        if (!m_bPassiveMode && (m_strRemoteHost == "" || m_nRemotePort == -1))
        {
            SendResponse("503 Bad sequence of commands.");
            break;
        }

        CString strResult;
        //int nResult = theServer.m_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_UPLOAD, strResult);
        int nResult = g_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_UPLOAD, strResult);
        switch (nResult)
        {
        case 1:
            SendResponse("550 Permission denied.");
            break;
        case 2:
            SendResponse("550 Filename invalid.");
            break;
        default:
            // create socket connection for file transfer
            if (!CreateDataConnection(2, strResult))
            {
                DestroyDataConnection();
            }
            break;
        }
    }
    break;

    // get file size
    case TOK_SIZE:
    {
        CString strResult;
        //int nResult = theServer.m_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_DOWNLOAD, strResult);
        int nResult = g_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_DOWNLOAD, strResult);
        switch (nResult)
        {
        case 1:
            SendResponse("550 Permission denied.");
            break;
        case 2:
            SendResponse("550 File not found.");
            break;
        default:
        {
            CFileStatus status;
            CFile::GetStatus(strResult, status);
            SendResponse("213 %d", status.m_size);
            break;
        }
        }
    }
    break;

    // delete file
    case TOK_DELE:
    {
        CString strResult;
        //int nResult = theServer.m_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_DELETE, strResult);
        int nResult = g_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_DELETE, strResult);
        switch (nResult)
        {
        case 1:
            SendResponse("550 Permission denied.");
            break;
        case 2:
            SendResponse("550 File not found.");
            break;
        default:
            // delete the file
            if (!DeleteFile(strResult))
            {
                SendResponse("450 Internal error deleting the file: \"%s\".", strArguments);
            }
            else
            {
                SendResponse("250 File \"%s\" was deleted successfully.", strArguments);
            }
            break;
        }
    }
    break;

    // remove directory
    case TOK_RMD:
    {
        CString strResult;
        int nResult = g_UserManager.CheckDirectory(m_strUserName, strArguments, m_strCurrentDir, FTP_DELETE, strResult);
        switch (nResult)
        {
        case 1:
            SendResponse("550 Permission denied.");
            break;
        case 2:
            SendResponse("550 Directory not found.");
            break;
        default:
            // remove the directory
            if (!RemoveDirectory(strResult))
            {
                if (GetLastError() == ERROR_DIR_NOT_EMPTY)
                {
                    SendResponse("550 Directory not empty.");
                }
                else
                {
                    SendResponse("450 Internal error deleting the directory.");
                }
            }
            else
            {
                SendResponse("250 Directory deleted successfully.");
            }
            break;
        }
    }
    break;

    // create directory
    case TOK_MKD:
    {
        CString strResult;
        //int nResult = theServer.m_UserManager.CheckDirectory(m_strUserName, strArguments, m_strCurrentDir, FTP_CREATE_DIR, strResult);
        int nResult = g_UserManager.CheckDirectory(m_strUserName, strArguments, m_strCurrentDir, FTP_CREATE_DIR, strResult);
        switch (nResult)
        {
        case 0:
            SendResponse("550 Directory already exists.");
            break;
        case 1:
            SendResponse("550 Can't create directory. Permission denied.");
            break;
        default:
            // create directory structure
            if (!MakeSureDirectoryPathExists(strResult))
            {
                SendResponse("450 Internal error creating the directory.");
            }
            else
            {
                SendResponse("250 Directory created successfully.");
            }
            break;
        }
    }
    break;

    // rename file or directory (part 1)
    case TOK_RNFR:
    {
        CString strResult;
        int nResult = g_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_RENAME, strResult);
        if (nResult == 0)
        {
            m_strRenameFile = strResult;
            m_bRenameFile = TRUE;
            SendResponse("350 File exists, ready for destination name.");
            break;
        }
        else
        {
            // client wants to rename directory
            nResult = g_UserManager.CheckDirectory(m_strUserName, strArguments, m_strCurrentDir, FTP_RENAME, strResult);
            switch (nResult)
            {
            case 0:
                m_strRenameFile = strResult;
                m_bRenameFile = FALSE;
                SendResponse("350 Directory exists, ready for destination name.");
                break;
            case 1:
                SendResponse("550 Permission denied.");
                break;
            default:
                SendResponse("550 File/directory not found.");
                break;
            }
        }
    }
    break;

    // rename file or directory (part 2)
    case TOK_RNTO:
    {
        if (m_strRenameFile.IsEmpty())
        {
            SendResponse("503 Bad sequence of commands.");
            break;
        }
        if (m_bRenameFile)
        {
            CString strResult;
            // check destination filename
            int nResult = g_UserManager.CheckFileName(m_strUserName, strArguments, m_strCurrentDir, FTP_RENAME, strResult);
            switch (nResult)
            {
            case 0:
                SendResponse("550 File already exists.");
                break;
            case 1:
                SendResponse("550 Permission denied.");
                break;
            default:
                // rename file
                if (!MoveFile(m_strRenameFile, strResult))
                {
                    SendResponse("450 Internal error renaming the file: \"%s\".", m_strRenameFile);
                }
                else
                {
                    SendResponse("250 File \"%s\" renamed successfully.", m_strRenameFile);
                }
                break;
            }
        }
        else
        {
            CString strResult;
            // check destination directory name
            int nResult = g_UserManager.CheckDirectory(m_strUserName, strArguments, m_strCurrentDir, FTP_RENAME, strResult);
            switch (nResult)
            {
            case 0:
                SendResponse("550 Directory already exists.");
                break;
            case 1:
                SendResponse("550 Permission denied.");
                break;
            case 3:
                SendResponse("550 Directory invalid.");
                break;
            default:
                // rename directory
                if (!MoveFile(m_strRenameFile, strResult))
                {
                    SendResponse("450 Internal error renaming the directory: \"%s\".", m_strRenameFile);
                }
                else
                {
                    SendResponse("250 Directory \"%s\" renamed successfully.", m_strRenameFile);
                }
                break;
            }
        }
    }
    break;

    // abort transfer
    case TOK_ABOR:
    {
        if (m_pDataSocket)
        {
            if (m_pDataSocket->GetStatus() != XFERMODE_IDLE)
            {
                SendResponse("426 Data connection closed.");
            }
            // destroy data connection
            //m_pThread->PostThreadMessage(WM_THREADMSG, 0, 0);
        }
        SendResponse("226 ABOR command successful.");
        break;
    }

    // get system info
    case TOK_SYST:
        SendResponse("215 UNIX emulated by Pablo's FTP Server.");
        break;

        // close connection
    case TOK_QUIT:
    case TOK_BYE:
    {
        // send goodbye message to client
        //CConnectThread *pThread = (CConnectThread *)m_pThread;
        //SendResponse("220 %s", ((CFTPServer *)pThread->m_pWndServer)->GetGoodbyeMessage());

        Close();
        
        // tell our thread we have been closed

        // destroy connection
        //m_pThread->PostThreadMessage(WM_THREADMSG, 1, 0);
        break;
    }

    // restart transfer
    case TOK_REST:
    {
        std::string str = TCharToString(strArguments);
        char* ch = (char*)str.c_str();
        if (!IsNumeric(ch))
        {
            strArguments.ReleaseBuffer();
            SendResponse("501 Invalid parameter.");
        }
        else
        {
            strArguments.ReleaseBuffer();
            m_dwRestartOffset = _ttol(strArguments);
            SendResponse("350 Restarting at %d.", m_dwRestartOffset);
        }
    }
    break;

    // display help
    case TOK_HELP:
        // if client did not specify a command, display all available commands
        if (strArguments == _T(""))
        {
            CString strResponse = _T("214-The following commands are recognized:\r\n");
            // find command in command list
            for (int i = TOK_ABOR; i < TOK_ERROR; i++)
            {
                strResponse += commandList[i].m_pszName;
                strResponse += _T("\r\n");
            }
            strResponse += _T("214 HELP command successful.");
            SendResponse(TCharToString(strResponse).c_str());
        }
        else
        {
            int nHelpCmd;
            // find command in command list
            for (nHelpCmd = TOK_ABOR; nHelpCmd < TOK_ERROR; nHelpCmd++)
            {
                // found command ?
                if (strArguments.CompareNoCase(stringToTChar(commandList[nHelpCmd].m_pszName)) == 0)
                {
                    break;
                }
            }
            if (nHelpCmd != TOK_ERROR)
            {
                // show help about command
                SendResponse("214 %s", commandList[nHelpCmd].m_pszDescription);
            }
            else
            {
                SendResponse("501 Unknown command %s", strArguments);
            }
        }
        break;

        // dummy instruction
    case TOK_NOOP:
        SendResponse("200 OK");
        break;

    default:
        SendResponse("502 Command not implemented - Try HELP.");
        break;
    }
}


/********************************************************************/
/*																	*/
/* Function name: FireStatusMessage									*/
/* Description  : Fire status message.								*/
/*																	*/
/********************************************************************/
void CConnectSocketEx::FireStatusMessage(const char*, int nType)
{
    //CConnectThread *pThread = (CConnectThread *)m_pThread;
    //((CFTPServer *)pThread->m_pWndServer)->AddTraceLine(nType, "[%d] %s", m_pThread->m_nThreadID, lpszStatus);
}


/********************************************************************/
/*																	*/
/* Function name: CreateDataConnection								*/
/* Description  : Create data transfer connection.					*/
/*																	*/
/********************************************************************/
BOOL CConnectSocketEx::CreateDataConnection(int nTransferType, LPCTSTR lpszData)
{
    if (!m_bPassiveMode)
    {
        do
        {
            m_pDataSocket = new CDataSocketEx(this, nTransferType);
            if (NULL == m_pDataSocket)
            {
                SendResponse("421 Failed to create data connection socket.");
                return FALSE;
            }
            // 创建socket
            SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);;
            sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons(m_nRemotePort);
            server.sin_addr.s_addr = inet_addr(TCharToString(m_strRemoteHost).c_str());

            if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
            {
                SendResponse("425 Can't open data connection.");
                return FALSE;
            }

            m_pDataSocket->SetRestartOffset(m_dwRestartOffset);
            m_pDataSocket->SetData(lpszData);

            switch (nTransferType)
            {
            case 0:
                SendResponse("150 Opening ASCII mode data connection for directory list.");
                break;
            case 1:
            case 2:
                SendResponse("150 Opening BINARY mode data connection for file transfer.");
                break;
            }
            m_pDataSocket->Attach(sock);
        } while (0);

    }
    else
    {
        m_pDataSocket->SetRestartOffset(m_dwRestartOffset);
        m_pDataSocket->SetData(lpszData);
        m_pDataSocket->SetTransferType(nTransferType, TRUE);
    }
    return TRUE;
}


/********************************************************************/
/*																	*/
/* Function name: DestroyDataConnection								*/
/* Description  : Close data transfer connection.					*/
/*																	*/
/********************************************************************/
void CConnectSocketEx::DestroyDataConnection()
{
    if (!m_pDataSocket)
        return;

    m_pDataSocket = NULL;
    m_strRemoteHost = "";
    m_nRemotePort = -1;
    m_dwRestartOffset = 0;
    m_bPassiveMode = FALSE;
}

void CConnectSocketEx::Close()
{
    m_bTerminate = TRUE;
    CloseHandle(m_hThread);
}

//通过套接字获取IP、Port等地址信息
bool CConnectSocketEx::GetPeerName(CString &strPeerAddress, UINT &nPeerPort)
{
    SOCKADDR_IN address;
    memset(&address, 0, sizeof(address));
    int nAddrLen = sizeof(address);

    //根据套接字获取地址信息
    if (::getpeername(m_socket, (SOCKADDR*)&address, &nAddrLen) != 0)
    {
        printf("Get IP address by socket failed!n");
        return false;
    }
    //读取IP和Port
    strPeerAddress.Format(_T("%s"), ::inet_ntoa(address.sin_addr));
    nPeerPort = ntohs(address.sin_port);

    return true;
}

