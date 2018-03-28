#include "StdAfx.h"

#include "dataprocess.h"

//#include <NTSTATUS.h>


std::string dataprocess::TCharToString(CString strInfo)
{
    char szTemp[4096];
    memset(szTemp, 0x0, sizeof(szTemp));
    int iLen = WideCharToMultiByte(CP_ACP, 0,strInfo, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, strInfo, -1, szTemp, iLen, NULL, NULL);
    string info(szTemp);
    return info;
}


dataprocess::dataprocess(void)
{
    m_iCurrRow = 0;
    for(int iRow = 0; iRow < MAX_ROW; iRow++)
    {
        m_row[iRow] = 0 ;
    }
    m_bInit = false;
}

dataprocess::~dataprocess(void)
{
}

int dataprocess::addData(int x, int y, LPCTSTR str)
{
    if (y < 10)
    {
        if (!m_bInit)
        {
            clearData();
        }
        return 0;
    }
    m_bInit = true;
    CString strData(str);
    int iRow = 0;
    int iCol = 0;
    for(iRow = 0; iRow < MAX_ROW; iRow++)
    {
        if ( m_row[iRow] == 0 )
        {
            m_row[iRow] = y;
            break;
        }
        if( m_row[iRow] >= y )
        {
            break;
        }
    }
    if (iRow == MAX_ROW)
    {
        iRow = MAX_ROW - 1;
    }
    m_iCurrRow = iRow;
    m_vecData[iRow].push_back(strData);
    
    return 0;
}

int dataprocess::printData()
{
    FILE * f = fopen("d:\\text.txt", "wb");
    if(NULL == f )
        return 1;

    for (int i = 0; i < MAX_ROW - 1; i++ )
    {
        CString str ;
        for (int j = 0; j < m_vecData[i].size(); j++ )
        {
            str += m_vecData[i][j];
            str += "        ";
        }
        std::string stdStr = TCharToString(str);
        fwrite(stdStr.c_str(), sizeof(char), stdStr.length(), f);
        fwrite("\n",sizeof(char),1,f);
    }
    fclose(f);
    return 0;
}


int dataprocess::clearData()
{
    for(int i = 0; i < m_iCurrRow; i++)
    {
        m_vecData[i].clear();
    }
    m_iCurrRow = 0;
    return 0;
}

int dataprocess::addImgInfo(ST_ImgInfo& stImgInfo)
{
    return 0;
}

int dataprocess::setImgInfo(ST_ImgInfo& stImgInfo)
{
    m_dialog.updateImg(stImgInfo);
    return 0;
}

int dataprocess::clearImgInfo()
{
    return 0;
}

int dataprocess::showItemDialog()
{
    m_dialog.Create(IDC_DIALOG_SHOW,NULL);
    m_dialog.ShowWindow(SW_SHOW);
    m_dialog.Invalidate();

    m_dialog.MoveWindow(100,100,865,560);
    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
CMyDialog::CMyDialog()
{

}

BEGIN_MESSAGE_MAP(CMyDialog, CDialog)
END_MESSAGE_MAP()

BOOL CMyDialog::OnInitDialog()
{
    return CDialog::OnInitDialog();
}

int CMyDialog::updateImg(ST_ImgInfo& info)
{
    m_stImgInfo.rectImg = info.rectImg;
    m_stImgInfo.rectReal = info.rectReal;
    m_stImgInfo.stream = info.stream;

    // 局部刷新
    RECT rect;
    rect.left = info.rectReal.X;
    rect.top = info.rectReal.Y;
    rect.right = rect.left + info.rectReal.Width;
    rect.bottom = rect.top + info.rectReal.Height;
    InvalidateRect(&rect);
    return 0;
}

void CMyDialog::OnPaint()
{
    CPaintDC dc(this);
    CDC szMemDC;
    szMemDC.CreateCompatibleDC(&dc);
    //szMemDC.SetBkMode(TRANSPARENT);

    Gdiplus::Image *pImage;
    //Gdiplus::Graphics graphics(szMemDC.m_hDC);
    Graphics graphics( GetDC()->GetSafeHdc() );   
    pImage = Gdiplus::Image::FromStream(m_stImgInfo.stream);
    RectF rectS = m_stImgInfo.rectReal;
    RectF &rectR = m_stImgInfo.rectImg;
    Status s = graphics.DrawImage(pImage,rectS,rectR.X,rectR.Y,rectR.Width,rectR.Height,UnitPixel);

    // 删除图像数据

    CDialog::OnPaint();
}


//
//CBitmap* GetCBitmapFromBMPInfo(LONG srcWidth,LONG srcHeight,byte *pGradImg)
//{
//    void*              pBitsDib;
//    CBitmap*	   wrk_nBitmap;
//    CGdiObject*        wrk_pGdiObject;
//    CDC                wrk_cdcMem;
//    HBITMAP            hBitmap;
//    HDC                hdc;
//
//    LPBITMAPINFO lpbmi = new   BITMAPINFO; 
//    ZeroMemory(lpbmi, sizeof(BITMAPINFO)); 
//    lpbmi-> bmiHeader.biSize =   sizeof(BITMAPINFOHEADER); 
//    lpbmi-> bmiHeader.biPlanes =   1; 
//    lpbmi-> bmiHeader.biCompression =   BI_RGB; 
//    lpbmi-> bmiHeader.biClrImportant =   0;   
//    lpbmi-> bmiHeader.biSizeImage =   0; 
//    lpbmi-> bmiHeader.biClrUsed =   0; 
//    lpbmi-> bmiHeader.biBitCount =   24; 
//    lpbmi-> bmiHeader.biWidth =   srcWidth; 
//    lpbmi-> bmiHeader.biHeight =   srcHeight; 
//    lpbmi-> bmiHeader.biXPelsPerMeter=   0; 
//    lpbmi-> bmiHeader.biYPelsPerMeter=   0; 
//
//    hdc = ::GetDC(NULL);
//    wrk_cdcMem.CreateCompatibleDC(NULL);
//    hBitmap = ::CreateDIBSection(NULL, lpbmi, DIB_RGB_COLORS, (void**)&pBitsDib, NULL, 0);
//    wrk_nBitmap = new CBitmap();
//    wrk_nBitmap->Attach(hBitmap);
//    wrk_pGdiObject = wrk_cdcMem.SelectObject(wrk_nBitmap);
//
//    SetDIBits(wrk_cdcMem.GetSafeHdc(),hBitmap,0,srcHeight,pGradImg,lpbmi,DIB_RGB_COLORS);
//
//    wrk_cdcMem.SelectObject(wrk_pGdiObject);
//    delete lpbmi;
//    wrk_cdcMem.DeleteDC();
//    if(hdc != NULL)
//    {
//        ::ReleaseDC(NULL,hdc);
//    }
//    return wrk_nBitmap;
//}
////把流转了CBitmap
//{
//    BITMAP   bmpInfo;    
//    bmp->GetBitmap(&bmpInfo);    
//    CDC   dcMemory;    
//    dcMemory.CreateCompatibleDC(&dc);    
//    dcMemory.SelectObject(bmp);    
//    dc.SetStretchBltMode(HALFTONE);    
//    dc.StretchBlt(0,0,600,450,&dcMemory,0,0,bmpInfo.bmWidth,bmpInfo.bmHeight,SRCCOPY);    
//    bmp->Detach();    
//
//    free(pBuffer); 
//    CloseHandle( hOpenFile );
//
//    byte by[];
//}
//
//
//#include "stdafx.h"
//#include <atlimage.h>
//#include <assert.h>
//#include <stdio.h>
//
//int _tmain(int argc, _TCHAR* argv[])
//{
//    CImage image;
//    HRESULT hr;
//    // Load sample image
//    hr = image.Load(L"c:\\temp\\test.bmp");
//    assert(hr == S_OK);
//
//    // Calculate reasonably safe buffer size
//    int stride = 4 * ((image.GetWidth() + 3) / 4);
//    size_t safeSize = stride * image.GetHeight() * 4 + sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER) + 256 * sizeof(RGBQUAD);
//    HGLOBAL mem = GlobalAlloc(GHND, safeSize);
//    assert(mem);
//
//    // Create stream and save bitmap
//    IStream* stream = 0;
//    hr = CreateStreamOnHGlobal(mem, TRUE, &stream);
//    assert(hr == S_OK);
//    hr = image.Save(stream, Gdiplus::ImageFormatBMP);
//    assert(hr == S_OK);
//
//    // Allocate buffer for saved image
//    LARGE_INTEGER seekPos = {0};
//    ULARGE_INTEGER imageSize;
//    hr = stream->Seek(seekPos, STREAM_SEEK_CUR, &imageSize);
//    assert(hr == S_OK && imageSize.HighPart == 0);
//    BYTE* buffer = new BYTE[imageSize.LowPart];
//
//    // Fill buffer from stream
//    hr = stream->Seek(seekPos, STREAM_SEEK_SET, 0);
//    assert(hr == S_OK);
//    hr = stream->Read(buffer, imageSize.LowPart, 0);
//    assert(hr == S_OK);
//
//    // Save to disk
//    FILE* fp = 0;
//    errno_t err = _wfopen_s(&fp, L"c:\\temp\\copy.bmp", L"wb");
//    assert(err == 0 && fp != 0);
//    fwrite(buffer, 1, imageSize.LowPart, fp);
//    fclose(fp);
//
//    // Cleanup
//    stream->Release();
//    delete[] buffer;
//    return 0;
//
//
//    Bitmap m_pBackBmp = ::new Bitmap((HBITMAP)::GetCurrentObject(CurDC, OBJ_BITMAP),NULL);
//
//
//}
//

