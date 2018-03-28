// MyTestDialog.cpp : 实现文件
//

#include "stdafx.h"
#include "hookUI.h"
#include "MyTestDialog.h"


// CMyTestDialog 对话框

IMPLEMENT_DYNAMIC(CMyTestDialog, CDialog)

CMyTestDialog::CMyTestDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CMyTestDialog::IDD, pParent)
{
    m_stImgInfo.stream = NULL;
    CString str = _T("D:\\1.png");
    m_pImage = Gdiplus::Image::FromFile(str);

}

CMyTestDialog::~CMyTestDialog()
{
}

void CMyTestDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMyTestDialog, CDialog)
    ON_WM_PAINT()
END_MESSAGE_MAP()




int CMyTestDialog::updateImg(ST_ImgInfo& info)
{
    if (info.wnd == NULL)
    {
        return 0;
    }

    m_stImgInfo.rectImg = info.rectImg;
    m_stImgInfo.rectReal = info.rectReal;
    m_stImgInfo.stream = info.stream;
    m_stImgInfo.wnd = info.wnd;

    // 局部刷新
    RECT rect;
    //rect.left = info.rectReal.X;
    //rect.top = info.rectReal.Y;
    //rect.right = rect.left + info.rectReal.Width;
    //rect.bottom = rect.top + info.rectReal.Height;

    rect.left = m_stStrInfo.rectImg.X;
    rect.top = m_stStrInfo.rectImg.Y;
    rect.right = rect.left + info.rectImg.Width;
    rect.bottom = rect.top + info.rectImg.Height;

    InvalidateRect(&rect);
    //Invalidate();
    return 0;
}

int CMyTestDialog::updateString(ST_ImgInfo& info)
{
    if (info.wnd == NULL)
    {
        return 0;
    }

    m_stStrInfo.rectImg = info.rectImg;
    m_stStrInfo.rectReal = info.rectReal;
    m_stStrInfo.stream = info.stream;
    m_stStrInfo.strInfo = info.strInfo;
    m_stStrInfo.wnd = info.wnd;
    // 局部刷新
    RECT rect;
    //rect.left = info.rectReal.X;
    //rect.top = info.rectReal.Y;
    //rect.right = rect.left + info.rectReal.Width;
    //rect.bottom = rect.top + info.rectReal.Height;

    rect.left = m_stStrInfo.rectImg.X;
    rect.top = m_stStrInfo.rectImg.Y;
    rect.right = rect.left + info.rectImg.Width;
    rect.bottom = rect.top + info.rectImg.Height;

    InvalidateRect(&rect);
    //m_stStrInfo.wnd->InvalidateRect(&rect);
    //Invalidate();
    return 0;
}

int CMyTestDialog::setItem(int id, CWnd* wnd)
{
    m_mapItem.insert(std::pair<int, CWnd*>(id, wnd));
    return 0;
}

BOOL CMyTestDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    return TRUE;
}

void CMyTestDialog::OnPaint()
{
    //szMemDC.SetBkMode(TRANSPARENT);
    if (NULL != m_stImgInfo.stream)
    {
        if (NULL == m_stImgInfo.wnd)
        {
            return ;
        }

        Gdiplus::Image *pImage;
        //Gdiplus::Graphics graphics(szMemDC.m_hDC);
        CDC* dc = m_stImgInfo.wnd->GetDC();
        Graphics graphics( dc->GetSafeHdc() );
        pImage = Gdiplus::Image::FromStream(m_stImgInfo.stream);
        RectF &rectS = m_stImgInfo.rectReal;
        RectF &rectR = m_stImgInfo.rectImg;
        //Status s = graphics.DrawImage(pImage,rectS,rectR.X,rectR.Y,rectR.Width,rectR.Height,UnitPixel);
        Status s = graphics.DrawImage(pImage,rectS,rectR.X,rectR.Y,rectR.Width,rectR.Height,UnitPixel);

        //RECT rect;
        //if (m_stImgInfo.wnd->GetDlgCtrlID() == 4004 || m_stImgInfo.wnd->GetDlgCtrlID() == 4004+2000)
        //{
        //    int i = 0;
        //    m_stImgInfo.wnd->GetWindowRect(&rect);

        //    Gdiplus::Image *pImage;
        //    pImage = m_pImage;
        //    RectF rectS ;
        //    rectS.Width = 200;
        //    rectS.Height = 200;
        //    rectS.X = 10;
        //    rectS.Y = 10;
        //    Status s = graphics.DrawImage(pImage,rectS,50,0,200,200,UnitPixel);

        //}
        //if (m_stImgInfo.wnd->GetDlgCtrlID() == 4003 || m_stImgInfo.wnd->GetDlgCtrlID() == 4003+2000)
        //{
        //    int i = 0;
        //    m_stImgInfo.wnd->GetWindowRect(&rect);
        //}
        //if (m_stImgInfo.wnd->GetDlgCtrlID() == 1009 || m_stImgInfo.wnd->GetDlgCtrlID() == 1009+2000)
        //{
        //    int i = 0;
        //    m_stImgInfo.wnd->GetWindowRect(&rect);
        //}

        //m_stImgInfo.wnd->ReleaseDC(dc);
    }
    if (m_stStrInfo.strInfo.GetLength() != 0)
    {
        if (NULL == m_stStrInfo.wnd)
        {
            return ;
        }
        CPaintDC pdc(m_stStrInfo.wnd);
        CDC szMemDC;
        szMemDC.CreateCompatibleDC(&pdc);

        //Gdiplus::Image *pImage;
        CDC* dc = m_stStrInfo.wnd->GetDC();
        Graphics graphics( dc->GetSafeHdc() );   
        //pImage = Gdiplus::Image::FromStream(m_stStrInfo.stream);
        RectF &rectS = m_stStrInfo.rectReal;
        RectF &rectR = m_stStrInfo.rectImg;
        //Status s = graphics.DrawImage(pImage,rectS,rectR.X,rectR.Y,rectR.Width,rectR.Height,UnitPixel);
        //Status s = graphics.DrawString(m_stStrInfo.strInfo,rectS,rectR.X,rectR.Y,rectR.Width,rectR.Height,UnitPixel);

        LOGFONT lfDefaultFont;
        memset(&lfDefaultFont, 0, sizeof(lfDefaultFont));
        lstrcpy(lfDefaultFont.lfFaceName, _T("宋体"));
        lfDefaultFont.lfHeight = -12;
        Font ffont(szMemDC.m_hDC,&lfDefaultFont);
        StringFormat sfTip;
        sfTip.SetTrimming(StringTrimmingEllipsisWord);
        sfTip.SetFormatFlags(StringFormatFlagsNoWrap); // 不换行
        sfTip.SetAlignment(StringAlignmentNear  ); 
        sfTip.SetLineAlignment(StringAlignmentCenter);

        SolidBrush brush(Color::Black);
        graphics.DrawString(m_stStrInfo.strInfo,m_stStrInfo.strInfo.GetLength(),
            &ffont,m_stStrInfo.rectImg,&sfTip,&brush);

        //m_stStrInfo.wnd->ReleaseDC(dc);
        //szMemDC.ReleaseOutputDC();

    }
    // 删除图像数据

    CDialog::OnPaint();
}
// CMyTestDialog 消息处理程序
