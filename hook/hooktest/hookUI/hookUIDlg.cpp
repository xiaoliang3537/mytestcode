// hookUIDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "hookUI.h"
#include "hookUIDlg.h"
#include "tlhelp32.h"  
#include <iostream>
#include <assert.h>
#include <atlimage.h >
#include <GdiPlusEnums.h>
#include "MyTestDialog.h"
#include "GdiPlus.h" 
using namespace Gdiplus;  
#pragma comment(lib, "GdiPlus.lib")

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

TCHAR szName[]=TEXT("Global\\MyFileMappingObject");    //ָ��ͬһ�鹲���ڴ������  
TCHAR szNameBitMap[]=TEXT("Global\\MyFileMappingObjectBitMap");    //ָ��ͬһ�鹲���ڴ������  
TCHAR szNameDataDesc[]=TEXT("Global\\MyFileMappingDataDesc");    //��������  ����������Ϣ �豸��Ϣ����  

#define BUF_SIZE 512  

HINSTANCE g_hInst=NULL;
typedef int (* getData)(void*);//����ԭ�Ͷ���  

CString stringToTChar(string strInfo)
{
    int charLen = strlen(strInfo.c_str());
    int len = MultiByteToWideChar(CP_ACP, 0, strInfo.c_str(), charLen, NULL, 0);
    wchar_t* pWChar = new wchar_t[len + 1];   
    MultiByteToWideChar(CP_ACP, 0, strInfo.c_str(), charLen, pWChar, len); 
    pWChar[len] = '\0';

    CString strPath;
    strPath.Empty();
    //strPath.Append(pWChar);
    strPath.Format(_T("%s"), pWChar);

    if(pWChar)
    { 
        delete[] pWChar;
        pWChar = NULL;
    }

    return strPath;
}

std::string TCharToString(CString strInfo)
{
    char szTemp[4096];
    memset(szTemp, 0x0, sizeof(szTemp));
    int iLen = WideCharToMultiByte(CP_ACP, 0,strInfo, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, strInfo, -1, szTemp, iLen, NULL, NULL);
    string info(szTemp);
    return info;
}


struct handle_data {  
    unsigned long process_id;  
    HWND best_handle;  
};  

BOOL IsMainWindow(HWND handle)  
{     
    return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);  
}  

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)  
{  
    handle_data& data = *(handle_data*)lParam;  
    unsigned long process_id = 0;  
    GetWindowThreadProcessId(handle, &process_id);  
    if (data.process_id != process_id || !IsMainWindow(handle)) {  
        return TRUE;  
    }  
    data.best_handle = handle;  
    return FALSE;     
}

HWND FindMainWindow(unsigned long process_id)  
{  
    handle_data data;  
    data.process_id = process_id;  
    data.best_handle = 0;  
    EnumWindows(EnumWindowsCallback, (LPARAM)&data);  
    return data.best_handle;  
}  

vector<int> split(const string &str,const string &pattern)
{
    //const char* convert to char*
    char * strc = new char[strlen(str.c_str())+1];
    strcpy(strc, str.c_str());
    vector<int> resultVec;
    char* tmpStr = strtok(strc, pattern.c_str());
    while (tmpStr != NULL)
    {
        resultVec.push_back(atoi(tmpStr));
        tmpStr = strtok(NULL, pattern.c_str());
    }

    delete[] strc;

    return resultVec;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// ChookUIDlg �Ի���




ChookUIDlg::ChookUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ChookUIDlg::IDD, pParent)
{

    Gdiplus::GdiplusStartupInput m_gdiplusStartupInput;  
    ULONG_PTR m_gdiplusToken;   
    GdiplusStartup( &m_gdiplusToken, &m_gdiplusStartupInput, NULL );  

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    CString str = _T("D:\\1.png");
    m_pImage = Gdiplus::Image::FromFile(str);
    Bitmap * pBitmap = Bitmap::FromFile(str);

    DWORD d = GetLastError();
    m_myTestDialog = NULL;
    //bool b = m_cBitMap.LoadBitmap(IDB_PNG1);
    //DWORD d = GetLastError();
    m_bHookAll = false;
    m_bHooked = false;
    m_dTargetPid = 0;
    m_hMapFile = NULL;
    m_pShareBuf = NULL;
    m_BitBuffer = NULL;
    m_dataDesc = NULL;
    m_pImageD = NULL;
    m_iIndex = 0;
    setShareMemory();

    m_myTestDialog = new CMyTestDialog();
    m_myTestDialog->Create(IDD_MYTESTDIALOG);
}

void ChookUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_NAME, m_editName );
    DDX_Control(pDX, IDC_EDIT_TEXT, m_editText );
    
}

BEGIN_MESSAGE_MAP(ChookUIDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BUTTON_HOOK_SINGLE, &ChookUIDlg::OnBnClickedButtonHookSingle)
    ON_BN_CLICKED(IDC_BUTTON_HOOK_ALL, &ChookUIDlg::OnBnClickedButtonHookAll)
    ON_BN_CLICKED(IDC_BUTTON_HOOK_OFF, &ChookUIDlg::OnBnClickedButtonHookOff)
    ON_BN_CLICKED(IDC_BUTTON_TEST, &ChookUIDlg::OnBnClickedButtonTest)
    ON_BN_CLICKED(IDC_IMAGE, &ChookUIDlg::OnBnClickedImage)
    ON_MESSAGE(WM_USER+1, &ChookUIDlg::OnUserMsg)
    ON_MESSAGE(WM_USER+2, &ChookUIDlg::OnUserMsgBitMap)
    ON_MESSAGE(WM_USER+3, &ChookUIDlg::OnUserMsgDrawImg)
END_MESSAGE_MAP()


// ChookUIDlg ��Ϣ�������

BOOL ChookUIDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
    //ActivationContext:afxAmbientActCtx= FALSE;

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
    CString theAppPath,theAppName;   
    TCHAR Path[MAX_PATH];   

    GetModuleFileName(NULL,Path,MAX_PATH);//�õ�Ӧ�ó����ȫ·��   
    m_strExePath = Path;

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void ChookUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void ChookUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);

	}
	else
	{
        CPaintDC dc(this);
        //CDC szMemDC;
        //m_cBitMap.LoadBitmap(IDB_BITMAP1);

        //BITMAP bitmap;
        //m_cBitMap.GetBitmap(&bitmap);

        //szMemDC.SelectObject(&bitmap);

        //szMemDC.CreateCompatibleDC(&dc);
        //szMemDC.SetBkMode(TRANSPARENT);

        //dc.BitBlt(347,194,100,100,&szMemDC,0,0,SRCCOPY);


        CBitmap cbmp;
        cbmp.LoadBitmap(IDB_BITMAP1);
        BITMAP bitmap;
        cbmp.GetBitmap(&bitmap);

        CDC dcMemory;
        dcMemory.CreateCompatibleDC(&dc);
        dcMemory.SelectObject(&cbmp);
        dc.BitBlt(347,150,
            100,
            100,
            &dcMemory,
            0,0,
            SRCCOPY);

        //cbmp.DeleteObject();
        //dcMemory.SelectObject(hOldBitmap);
        //dcMemory.DeleteDC();

        //if (m_hBmp != 0x0 )
        //{
        //    CBitmap bitMap;
        //    bitMap.DeleteObject();
        //    bitMap.Attach(m_hBmp);

        //    CDC dcMemory;
        //    dcMemory.CreateCompatibleDC(&dc);
        //    dcMemory.SelectObject(&bitMap);
        //    dc.BitBlt(534,150,
        //        250,
        //        350,
        //        &dcMemory,
        //        0,0,
        //        SRCCOPY);
        //    
        //}
        //


        //CDC szMemDC;
        //szMemDC.CreateCompatibleDC(&dc);
        ////szMemDC.SetBkMode(TRANSPARENT);

        //Gdiplus::Image *pImage;
        ////Gdiplus::Graphics graphics(szMemDC.m_hDC);
        //Graphics graphics( GetDC()->GetSafeHdc() );   
        //pImage = m_pImage;
        //RectF rectS ;
        //rectS.Width = 200;
        //rectS.Height = 200;
        //rectS.X = 320;
        //rectS.Y = 130;
        //Status s = graphics.DrawImage(pImage,rectS,50,0,200,200,UnitPixel);
        //Point p;
        //p.X = 347;
        //p.Y = 150;
        ////Status s = graphics.DrawImage(pImage,p);
        ////szMemDC.DeleteDC();
        //if(NULL != m_pImageD)
        //{
        //    rectS.X = 534;
        //    Status s = graphics.DrawImage(m_pImageD,rectS,0,0,200,200,UnitPixel);
        //    m_pImageD = NULL;
        //}

		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR ChookUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void ChookUIDlg::OnBnClickedButtonHookSingle()
{
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    char szName[128] = {0};
    CString strName;
    m_editName.GetWindowText(strName);
    string str = TCharToString(strName);
    sprintf(szName, "%s", str.c_str());
    this->HookSingel(szName);
    this->setCallBack();
}

void ChookUIDlg::OnBnClickedButtonHookAll()
{
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    this->HookAll();
    this->setCallBack();
}

void ChookUIDlg::OnBnClickedButtonHookOff()
{
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    bool bRet = false;
    if (!m_bHooked)
    {
        return ;
    }
    if(m_bHookAll)
        bRet = this->unHookAll();
    else
        bRet = this->unHookSingle();
}

afx_msg BOOL ChookUIDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
    char *ch  = (char*)pCopyDataStruct->lpData;
    return TRUE;
}

afx_msg LRESULT ChookUIDlg::OnUserMsg(WPARAM wParam, LPARAM lParam)
{
    int i = 0;
    DWORD iTrs = (DWORD)wParam;
    DWORD m = (DWORD)lParam;
    int len = (int)wParam;
    if(NULL == m_pShareBuf )
        return TRUE;

    //m_cDataPrpcess.addData(iTrs, m, m_pShareBuf);

    //wchar_t* p = (wchar_t*)(m_pShareBuf+len);
    //int X = _wtoi(p+1);
    //int Y = _wtoi(p+2);

    //int nLength = m_editText.GetWindowTextLength();  
    ////ѡ����ǰ�ı���ĩ��  
    //m_editText.SetSel(nLength, nLength);  
    //m_editText.ReplaceSel(m_pShareBuf);

    //nLength = m_editText.GetWindowTextLength();  
    ////ѡ����ǰ�ı���ĩ��  
    //CString str;
    //str.Format(_T("     %ld,%ld \n"), iTrs, m);
    //m_editText.SetSel(nLength, nLength);  
    //m_editText.ReplaceSel(str);

    // ��ȡ��������
    std::string strDesc = (char*)m_dataDesc;
    std::string strDelim = ",";
    std::vector<int> vecRect = split(strDesc, strDelim);

    if (NULL != m_myTestDialog && vecRect.size() == 11 )
    {
        HWND hwnd = FindMainWindow(m_dTargetPid);
        ::GetWindowRect(hwnd, &m_injectRect);
        
        ST_ImgInfo img;
        img.rectImg.X = vecRect[0];
        img.rectImg.Y = vecRect[1];
        img.rectImg.Width = 100;
        img.rectImg.Height = 33;
        
        Gdiplus::RectF rect;
        rect.X = vecRect[6];
        rect.Y = vecRect[7];
        rect.Width = vecRect[8] - rect.X ;
        rect.Height = vecRect[9] - rect.Y;

        img.rectReal.X = rect.X - m_injectRect.left + vecRect[0];
        img.rectReal.Y = rect.Y - m_injectRect.top + vecRect[1];
        img.rectReal.Width = img.rectImg.Width;
        img.rectReal.Height = img.rectImg.Height;
        
        int id = vecRect[10];
        img.strInfo = m_pShareBuf;
        img.wnd = getWnd(id);
        CMyTestDialog *dlg = (CMyTestDialog*)(img.wnd);
        dlg->updateString(img);
        //m_myTestDialog->updateString(img);
        
        // ͳһ���ݸ߶�
        if (id == 1009 || id = 2000 + 1009)
        {
            int x = img.rectImg.X;
            int y = img.rectImg.Y - 9 = 0;
        }
        
    }

    //std::string stdStr = (char*)m_dataDesc;
    //CString str;
    //str.Format(_T("t_%s[%s]\n"), stringToTChar(stdStr), m_pShareBuf);
    //OutputDebugString(str);

    return TRUE;
}




// ͼ����Ϣ����
LRESULT ChookUIDlg::OnUserMsgBitMap(WPARAM wParam, LPARAM lParam)
{
    int i = 0;
    DWORD iTrs = (DWORD)wParam;
    DWORD m = (DWORD)lParam;
    int len = (int)wParam;
    if(NULL == m_BitBuffer )
        return TRUE;

    // ��ȡλͼ����
    HGLOBAL mem = GlobalAlloc(GHND, len);
    HRESULT hr;
    IStream* stream = 0;
    hr = CreateStreamOnHGlobal(mem, TRUE, &stream);
    stream->Write(m_BitBuffer, len, NULL);

    //CImage img;
    //img.Load(stream);
    //if (img.GetBits() != 0x0)
    //{
    //    m_hBmp =img.Detach();
    //}
    //m_hBmp =(HBITMAP)img.operator HBITMAP();
    
    //Invalidate(true);

    //m_cBitMap.Attach(hbmp);

    



    // ��ȡ��������
    std::string strDesc = (char*)m_dataDesc;
    std::string strDelim = ",";
    std::vector<int> vecRect = split(strDesc, strDelim);
    /*
    std::vector<int>::iterator it = vecRect.begin();
    CString str;
    str.Format(_T("E:\\code\\vs_test\\testhook\\Debug\\temp\\b_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d.png"),vecRect[0], vecRect[1],vecRect[2],vecRect[3],vecRect[4],vecRect[5],
    vecRect[6],vecRect[7],vecRect[8],vecRect[9]);
    FILE* fp = 0;
    errno_t err = _wfopen_s(&fp, str, L"wb");
    assert(err == 0 && fp != 0);
    fwrite(m_BitBuffer, 1, len, fp);
    fclose(fp);*/
    
    if (NULL != m_myTestDialog)
    {
        HWND hwnd = FindMainWindow(m_dTargetPid);
        ::GetWindowRect(hwnd, &m_injectRect);

        ST_ImgInfo img;

        img.rectImg.X = vecRect[4];
        img.rectImg.Y = vecRect[5];
        img.rectImg.Width = vecRect[2];
        img.rectImg.Height = vecRect[3];

        Gdiplus::RectF rect;
        rect.X = vecRect[6];
        rect.Y = vecRect[7];
        rect.Width = vecRect[8] - rect.X ;
        rect.Height = vecRect[9] - rect.Y;
        
        img.rectReal.X = vecRect[0];
        img.rectReal.Y = vecRect[1];
        img.rectReal.Width = vecRect[2];
        img.rectReal.Height = vecRect[3];

        img.stream = stream;
        int id = vecRect[10];
        img.wnd = getWnd(id);

        CMyTestDialog *dlg = (CMyTestDialog*)(img.wnd);
        dlg->updateImg(img);

        if (id == 1009+2000 || id == 1009)
        {
            CImage image;
            image.Load(stream);
            CString strFile;
            strFile.Format(_T("E:\\code\\vs_test\\testhook\\Debug\\temp\\%d_%d.png"), vecRect[0],vecRect[1]);
            saveBitmapToFile(strFile, image, img.rectReal,img.rectImg);
        }

        //m_myTestDialog->updateImg(img);

        //CString str;
        //str.Format(_T("b_%d\n"), id);
        //OutputDebugString(str);
    }

    //CString str;
    //std::string stdStr = (char*)m_dataDesc;
    //str.Format(_T("b_%s\n"), stringToTChar(stdStr));
    //OutputDebugString(str);

    return TRUE;
}



LRESULT ChookUIDlg::OnUserMsgDrawImg(WPARAM wParam, LPARAM lParam)
{
    int i = 0;
    DWORD iTrs = (DWORD)wParam;
    DWORD m = (DWORD)lParam;
    int len = (int)wParam;
    if(NULL == m_BitBuffer || NULL == m_dataDesc)
        return TRUE;

    if(len == 0 || m == 0)
        return TRUE;
    // ��ȡλͼ����
    HGLOBAL mem = GlobalAlloc(GHND, len);
    HRESULT hr;
    IStream* stream = 0;
    hr = CreateStreamOnHGlobal(mem, TRUE, &stream);
    stream->Write(m_BitBuffer, len, NULL);
    
    /*
    // ��ȡ��������
    std::string strDesc = (char*)m_dataDesc;
    std::string strDelim = ",";
    std::vector<int> vecRect = split(strDesc, strDelim);
    std::vector<int>::iterator it = vecRect.begin();
    CString str;
    vecRect[0];
    str.Format(_T("E:\\code\\vs_test\\testhook\\Debug\\temp\\%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d.png"),vecRect[0], vecRect[1],vecRect[2],vecRect[3],
        vecRect[4],vecRect[5],vecRect[6],vecRect[7],vecRect[8],vecRect[9],vecRect[10],vecRect[11]);
    FILE* fp = 0;
    errno_t err = _wfopen_s(&fp, str, L"wb");
    assert(err == 0 && fp != 0);
    fwrite(m_BitBuffer, 1, len, fp);
    fclose(fp);
    */

    //CImage img;
    //img.Load(stream);
    //if (img.GetBits() != 0x0)
    //{
    //    m_hBmp =img.Detach();
    //}
    //m_hBmp =(HBITMAP)img.operator HBITMAP();

    
    //m_pImageD = Gdiplus::Image::FromStream(stream);

    //Invalidate(true);
    //ST_ImgInfo img;
    //img.stream = stream;
    //m_myTestDialog->updateImg(img);
    
    return TRUE;
}

bool ChookUIDlg::HookSingel(char* strProcessName)
{
    std::string str = TCharToString(m_strExePath);
    str = str.substr(0,str.find_last_of("\\"));
    char szPath[512]={0};
    //sprintf(szPath, "%s\\hookcommon.dll", str.c_str());
    sprintf(szPath, "E:\\code\\vs_test\\testhook\\Debug\\hookcommon.dll", str.c_str());
    int isInject = InjectDllToRemoteProcess(szPath, NULL , strProcessName );
    if (isInject != 0)
    {
        //ע��Զ�̽���ʧ��
        CString str;
        str.Format(_T("ע��Զ�̽���ʧ��.%d"), isInject);
        ::MessageBox(NULL,str,_T("error"),MB_OK);
    }
    if (isInject != 0)
    {
        return false;
    }
    m_bHookAll = false;
    m_bHooked = true;
    return true;
}

bool ChookUIDlg::HookAll()
{
    bool bHook = false;
    g_hInst=::LoadLibrary(_T("hookcommon.dll"));//����dll�ļ�  
    if (g_hInst==NULL)  
    {  
        DWORD d = GetLastError();
        AfxMessageBox(_T("Load HookMessageBox.dll Failed"));  
        return bHook;  
    }  
    typedef BOOL (* StartHook)(HWND hWnd);//����ԭ�Ͷ���  
    StartHook Hook;  
    Hook=(StartHook)::GetProcAddress(g_hInst,"StartHook");//��ȡ������ַ  
    if (Hook==NULL)  
    {  
        AfxMessageBox(_T("GetFunction StartHook Failed"));  
        return bHook;  
    }
    DWORD d = GetCurrentProcessId();
    HWND h = ::GetTopWindow(0);
    ::GetTopWindow(h);
    if ( Hook(h))//���ú���  
    {  
        bHook = true;
    }

    typedef void (* SetMsgWnd)(HWND wnd);//����ԭ�Ͷ���  
    SetMsgWnd setWnd;
    setWnd = (SetMsgWnd)::GetProcAddress(g_hInst,"SetMsgWnd");
    if(NULL != setWnd)
    {
        setWnd(h);
    }

    m_bHookAll = true;
    m_bHooked = true;

    return bHook;
}

bool ChookUIDlg::unHookAll()
{
    if (g_hInst==NULL)  
    {  
        // δ��hook
        return false;
    }  
    typedef VOID (WINAPI* StopHook)();//����ԭ�Ͷ���  
    StopHook UnHook;  
    UnHook=(StopHook)::GetProcAddress(g_hInst,"StopHook");//��ȡ������ַ  
    if (UnHook==NULL)  
    {  
        AfxMessageBox(_T("GetFunction StopHook Failed"));  
        FreeLibrary(g_hInst);  
        g_hInst=NULL;  
        return false;  
    }  

    UnHook();//���ú���  
    FreeLibrary(g_hInst);  
    g_hInst=NULL;

    m_bHookAll = false;
    m_bHooked = false;

    return true;
}

bool ChookUIDlg::unHookSingle()
{
    DWORD dwPid = m_dTargetPid;
    TCHAR* szDllName = NULL;

    HANDLE hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);  

    MODULEENTRY32 me32;  
    me32.dwSize = sizeof(me32);  

    //����ƥ��Ľ�������  
    BOOL bRet = Module32First(hSnap,&me32);  
    
    CString strDllNames = m_strDllName;
    while(bRet)  
    {  
        CString strExePath = me32.szExePath;
        //if(lstrcmp(strupr(me32.szExePath),strupr(szDllName)) == 0)  
        if( strExePath.Compare(strDllNames) == 0 )
        {  
            break;  
        }  
        bRet = Module32Next(hSnap,&me32);  
    }  

    CloseHandle(hSnap);  

    char *pFunName = "FreeLibrary";  

    HANDLE hProcess =OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);  

    if(hProcess == NULL)  
    {  
        return false;  
    }  

    FARPROC pFunAddr =GetProcAddress(GetModuleHandle(_T("kernel32.dll")), pFunName);  

    HANDLE hThread =CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunAddr,me32.hModule, 0, NULL);  

    WaitForSingleObject(hThread, INFINITE);  

    CloseHandle(hThread);  
    CloseHandle(hProcess);  

    m_bHooked = false;

    return true;
}

BOOL ChookUIDlg::setCallBack()
{
    HINSTANCE hInst=::LoadLibrary(_T("hookcommon.dll"));//����dll�ļ�  
    if (hInst==NULL)  
    {  
        DWORD d = GetLastError();
        return false;  
    } 

    typedef void (* SetMsgWnd)(HWND wnd);//����ԭ�Ͷ���  
    SetMsgWnd setWnd;
    setWnd = (SetMsgWnd)::GetProcAddress(hInst,"SetMsgWnd");
    if(NULL != setWnd)
    {
        HWND h = ::FindWindow(NULL,_T("hookUI"));
        setWnd(h);
    }
    ::FreeLibrary(hInst);

    return TRUE;
}


int ChookUIDlg::setShareMemory()
{
    // ���ݹ����ڴ�
    HANDLE hMapFile;  
    hMapFile = CreateFileMapping(  
        INVALID_HANDLE_VALUE,    // use paging file  
        NULL,                    // default security  
        PAGE_READWRITE,          // read/write access  
        0,                       // maximum object size (high-order DWORD)  
        BUF_SIZE,                // maximum object size (low-order DWORD)  
        szName);                 // name of mapping object  

    if (hMapFile == NULL)  
    {  
        _tprintf(TEXT("Could not create file mapping object (%d).\n"),  
            GetLastError());  
        return 1;  
    }  
    m_pShareBuf = (LPTSTR) MapViewOfFile(hMapFile,   // handle to map object  
        FILE_MAP_ALL_ACCESS, // read/write permission  
        0,  
        0,  
        BUF_SIZE);  

    if (m_pShareBuf == NULL)  
    {  
        _tprintf(TEXT("Could not map view of file (%d).\n"),  
            GetLastError());  
        CloseHandle(hMapFile);  
        return 1;  
    }  

    // λͼ�����ڴ�
    HANDLE hMapBit;  
    hMapBit = CreateFileMapping(  
        INVALID_HANDLE_VALUE,    // use paging file  
        NULL,                    // default security  
        PAGE_READWRITE,          // read/write access  
        0,                       // maximum object size (high-order DWORD)  
        1024*1024,                // maximum object size (low-order DWORD)  
        szNameBitMap);                 // name of mapping object  

    if (hMapBit == NULL)  
    {  
        _tprintf(TEXT("Could not create file mapping object (%d).\n"),  
            GetLastError());  
        return 1;  
    }  
    m_BitBuffer = (BYTE*) MapViewOfFile(hMapBit,   // handle to map object  
        FILE_MAP_ALL_ACCESS, // read/write permission  
        0,  
        0,  
        1024*1024);  

    if (m_pShareBuf == NULL)  
    {  
        _tprintf(TEXT("Could not map view of file (%d).\n"),  
            GetLastError());  
        CloseHandle(hMapBit);  
        return 1;  
    }  

    // ������Ϣ
    HANDLE hMapDesc;  
    hMapDesc = CreateFileMapping(  
        INVALID_HANDLE_VALUE,    // use paging file  
        NULL,                    // default security  
        PAGE_READWRITE,          // read/write access  
        0,                       // maximum object size (high-order DWORD)  
        512,                // maximum object size (low-order DWORD)  
        szNameDataDesc);                 // name of mapping object  

    if (hMapDesc == NULL)  
    {  
        _tprintf(TEXT("Could not create file mapping object (%d).\n"),  
            GetLastError());  
        return 1;  
    }  
    m_dataDesc = (BYTE*) MapViewOfFile(hMapDesc,   // handle to map object  
        FILE_MAP_ALL_ACCESS, // read/write permission  
        0,  
        0,  
        512);  

    if (m_pShareBuf == NULL)  
    {  
        _tprintf(TEXT("Could not map view of file (%d).\n"),  
            GetLastError());  
        CloseHandle(hMapDesc);  
        return 1;  
    }  
    
    return 0;
}

int ChookUIDlg::closeShareMemery()
{
    if(NULL != m_pShareBuf)
        UnmapViewOfFile(m_pShareBuf);  
    if(NULL != m_hMapFile)
        CloseHandle(m_hMapFile);
    return 0;
}

//���̿��գ�ö�ٸ����̣�
BOOL ChookUIDlg::GetPidByProcessName(LPCTSTR lpszProcessName , DWORD &dwPid)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if ( INVALID_HANDLE_VALUE == hSnapshot )
    {
        return FALSE;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if ( !Process32First(hSnapshot, &pe) )
    {
        ::CloseHandle(hSnapshot);
        return FALSE;
    }

    while ( Process32Next(hSnapshot, &pe) )
    {
        if ( !lstrcmp(lpszProcessName, pe.szExeFile) )
        {
            ::CloseHandle(hSnapshot);
            dwPid = pe.th32ProcessID;
            return TRUE;
        }
    }

    ::CloseHandle(hSnapshot);
    return FALSE;
}


//ע��DLL��Զ�̽���
int ChookUIDlg::InjectDllToRemoteProcess(const char* lpDllName, const char* lpPid, const char* lpProcName)
{
    DWORD dwPid = 0;
    string strProcName = lpProcName;
    CString strProcNameW = stringToTChar(strProcName);
    if (NULL == lpPid || 0 == strlen(lpPid))
    {
        if (NULL != lpProcName && 0 != strlen(lpProcName))
        {
            if (!GetPidByProcessName(strProcNameW, dwPid))
            {
                return 1;
            }
        }
        else
        {
            return 2;
        }
    }
    else
    {
        dwPid = atoi(lpPid);
    }

    m_dTargetPid = dwPid;
    string str = lpDllName;
    m_strDllName = stringToTChar(str);
    //m_strDllName.Format(_T("%s"), lpDllName );
    //����Pid�õ����̾��(ע�����Ȩ��)
    HANDLE hRemoteProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, dwPid);
    if (INVALID_HANDLE_VALUE == hRemoteProcess)
    {
        return 3;
    }

    //����DLL·������Ҫ���ڴ�ռ�
    DWORD dwSize = (1 + lstrlenA(lpDllName)) * sizeof(char);

    //ʹ��VirtualAllocEx������Զ�̽��̵��ڴ��ַ�ռ����DLL�ļ���������,�ɹ����ط����ڴ���׵�ַ.
    LPVOID lpRemoteBuff = (char *)VirtualAllocEx(hRemoteProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
    if (NULL == lpRemoteBuff)
    {
        CString str;
        str.Format(_T("��Զ�̽���ʧ��,%d, %d"), GetLastError(),dwSize);
        ::MessageBox(NULL,str,_T("error"),MB_OK);

        CloseHandle(hRemoteProcess);
        return 4;
    }

    //ʹ��WriteProcessMemory������DLL��·�������Ƶ�Զ�̽��̵��ڴ�ռ�,�ɹ�����TRUE.
    DWORD dwHasWrite = 0;
    BOOL bRet = WriteProcessMemory(hRemoteProcess, lpRemoteBuff, lpDllName, dwSize, &dwHasWrite);
    if (!bRet || dwHasWrite != dwSize)
    {
        VirtualFreeEx(hRemoteProcess, lpRemoteBuff, dwSize, MEM_COMMIT);
        CloseHandle(hRemoteProcess);
        return 5;
    }

    //����һ�����������̵�ַ�ռ������е��߳�(Ҳ��:����Զ���߳�),�ɹ��������߳̾��.
    //ע��:���̾������߱�PROCESS_CREATE_THREAD, PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE,��PROCESS_VM_READ����Ȩ��
    DWORD  dwRemoteThread = 0;
    //LPTHREAD_START_ROUTINE pfnLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("Kernel32"), "LoadLibraryA");
    //HANDLE hRemoteThread = CreateRemoteThread(hRemoteProcess, NULL, 0, pfnLoadLibrary, lpRemoteBuff, 0, &dwRemoteThread);
    HANDLE hRemoteThread = CreateRemoteThread(hRemoteProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, lpRemoteBuff, 0, &dwRemoteThread);
    if (INVALID_HANDLE_VALUE == hRemoteThread)
    {
        VirtualFreeEx(hRemoteProcess, lpRemoteBuff, dwSize, MEM_COMMIT);
        CloseHandle(hRemoteProcess);
        return 6;
    }

    //ע��ɹ��ͷž��
    WaitForSingleObject(hRemoteThread, INFINITE);
    CloseHandle(hRemoteThread);
    CloseHandle(hRemoteProcess);

    return 0;
}


void ChookUIDlg::OnBnClickedButtonTest()
{
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    //m_cDataPrpcess.printData();
    //::MessageBox(NULL,_T("1"), _T("1"),MB_OK);
    //Invalidate(true);

    //EnumChildWindows


    //Graphics *gc = Graphics::FromImage( &img );
    //m_pImage;
    //RectF rect;
    //rect.X = 0;
    //rect.Y = 0;
    //rect.Width = 200;
    //rect.Height = 200;
    //gc->DrawImage(m_pImage, rect, 150,0,200,200,UnitPixel);
    //HRESULT s = img.Save(_T("D:\\test.png"));

}

void ChookUIDlg::OnBnClickedImage()
{
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    // ���ﻭͼ

    //Invalidate(true);
    //CMyDialog dialog;

    ////dialog.Create(IDC_DIALOG_SHOW,NULL);
    //
    //DLGTEMPLATE tmp;
    //tmp.x = 100;
    //tmp.y = 100;
    //tmp.cx = 865;
    //tmp.cy = 560;
    //tmp.style = WS_VISIBLE ;
    //tmp.dwExtendedStyle = WS_EX_ACCEPTFILES;
    //tmp.cdit = 10;

    //dialog.CreateIndirect(&tmp , this);
    //dialog.ShowWindow(SW_SHOW);
    //dialog.Invalidate();
    //dialog.DoModal();
    //dialog.MoveWindow(100,100,865,560);



    // �����Ӵ���
    HWND hwnd = FindMainWindow(m_dTargetPid);
    ::GetWindowRect(hwnd, &m_injectRect);


    HWND hParent = hwnd;
    HWND hNext = NULL;
    CWnd* wnd = NULL;
    while(1)
    {
        wnd = FindWindowEx(hwnd, hNext, NULL, NULL);
        if (NULL == wnd)
        {
            break;
        }
        //CWnd *wnd = FromHandle(hwnd);
        RECT rect;
        wnd->GetWindowRect(&rect);
        hNext = wnd->m_hWnd;
        CString str;
        CString strText;
        wnd->GetWindowText(strText);
        str.Format(_T("%d,%d,%d,%d,%s __ \n "), rect.left, rect.top, rect.right, rect.bottom ,strText );
        OutputDebugString(str);

        if (strText.GetLength() == 0 )
        {
            int id = wnd->GetDlgCtrlID();

            RECT rectW;
            rectW.left = rect.left - m_injectRect.left;
            rectW.right = rect.right - m_injectRect.left;
            rectW.top = rect.top - m_injectRect.top;
            rectW.bottom = rect.bottom - m_injectRect.top;
            str.Format(_T("%d,%d,%d,%d,%s,(%d,%d,%d,%d)\n"), rect.left, rect.top, rect.right, rect.bottom ,strText,
                rectW.left, rectW.top, rectW.right, rectW.bottom);
            OutputDebugString(str);
            int ret = findExsitRect(rectW);
            if (-1 == ret)
            {
                m_mapInfo.insert(std::pair<int , RECT>(id, rectW) );

                //CWnd* wnd = new CWnd();
                CWnd* wnd = new CMyTestDialog;
                bool b = wnd->Create(_T("#32770"), _T("1"), WS_VISIBLE|WS_CAPTION, rectW, m_myTestDialog, id+2000 );
                wnd->ModifyStyle(WS_CAPTION, 0); // ȥ��������  
                wnd->ModifyStyleEx(WS_EX_DLGMODALFRAME, 0); // ȥ���߿�  
                //DWORD dwStyle = wnd->GetStyle();
                //DWORD dwNewStyle = WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;  
                //dwNewStyle &= ~dwStyle;  
                //SetWindowLong(wnd->m_hWnd, GWL_STYLE, dwNewStyle);
                wnd->SetParent(m_myTestDialog);
                if (b)
                {
                    DWORD d = GetLastError();
                    wnd->ShowWindow(SW_SHOW);
                    wnd->SetWindowText(_T("����"));
                }

                m_mapWnd.insert(std::pair<int, CWnd*>(id, wnd));
            }
            //else
            //    m_mapWnd.insert(std::pair<int, int>(id, ret));
        }
        //strText.ReleaseBuffer();
        // ���δ�Сһ�� ����һ��

    }
    // ����������
    m_myTestDialog->ShowWindow(SW_SHOW);
    m_myTestDialog->MoveWindow(200,200,m_injectRect.right-m_injectRect.left,m_injectRect.bottom-m_injectRect.top);
    //m_myTestDialog->DoModal();
}


int ChookUIDlg::saveBitmapToFile(CString strFileName, CImage& img, RectF &srcRect, RectF &tagRect)
{
    CImage imgage;
    imgage.Create(tagRect.Width,tagRect.Height,32,0);


    bool b = BitBlt(imgage.GetDC(),0,0,srcRect.Width,srcRect.Height,
        img.GetDC(), tagRect.X,tagRect.Y, SRCCOPY );
    imgage.Save(strFileName);

    img.ReleaseDC();
    imgage.ReleaseDC();
    return b?0:1;
}

int ChookUIDlg::findExsitRect(RECT &rect)
{
    std::map<int, RECT>::iterator it = m_mapInfo.begin();
    for(; it != m_mapInfo.end(); it++ )
    {
        RECT &rc = (*it).second;
        if (rc.left == rect.left && rc.right == rect.right 
            && rc.top == rect.top && rc.bottom == rect.bottom )
        {
            return (*it).first;
        }
    }
    return -1;
}

CWnd* ChookUIDlg::getWnd(int id)
{
    std::map<int, CWnd*>::iterator it = m_mapWnd.begin();
    for(; it != m_mapWnd.end(); it++ )
    {
        if (id == (*it).first)
        {
            return (it->second);
        }
    }
    return 0;
}
