#include "loger.h"
#include <iostream>


#define MAX_LEN 4096

#ifdef I_OS_WINDOWS
#define FILE_NAME(x) strrchr(x,'\\')?strrchr(x,'\\')+1:x
#else
#define FILE_NAME(x) strrchr(x,'/')?strrchr(x,'/')+1:x
#endif

extern int g_outinfo;

loger::loger()
{
    g_pFile = NULL;
    p_str = NULL;
    p_str = new char[MAX_LEN];
    len = 0;
}


int loger::init()
{
    int iRet = 0;
    do
    {
        if(NULL != g_pFile)
            break;
#ifdef I_OS_WINDOWS
//        SYSTEMTIME sy;
//        memset(&sy,0,sizeof(sy));
//        GetLocalTime(&sy);
//        char szTemp[128];
//        memset(szTemp, 0x0, sizeof(szTemp));
//        sprintf(szTemp,"%04d-%02d-%02d.log",
//            sy.wYear,sy.wMonth,sy.wDay,sy.wHour,sy.wMinute,sy.wSecond);
//        g_strLogFile = CPubVar::GetInstance().getWorkPath() + _SYMBOL_PATH_ + "log" + _SYMBOL_PATH_ +szTemp;
//        g_pFile = fopen(g_strLogFile.c_str(), "a");
//        if (NULL == g_pFile)
//        {
//            // 日志文件打开错误
//            iRet = -1;
//            break;
//        }
#else
//        time_t tNow =time(NULL);
//        time_t tEnd = tNow + 1800;

        struct tm  *ptm;
        long ts;
        ts = time(NULL);
        ptm = localtime(&ts);
        char szTemp[128];
        memset(szTemp, 0x0, sizeof(szTemp));
        sprintf(szTemp,"%04d-%02d-%02d.log",
            ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday );
        g_strLogFile = CPubVar::GetInstance().getWorkPath() + _SYMBOL_PATH_ + "log" + _SYMBOL_PATH_ +szTemp;
        g_pFile = fopen(g_strLogFile.c_str(), "a");
        if (NULL == g_pFile)
        {
            // 日志文件打开错误
            LOG(ERROR) << tr("file open error !") << endl;
            iRet = -1;
            break;
        }

#endif
    } while (0);
    return iRet;
}


loger& loger::operator<<(int value)
{
    char sz[32] = {0};
    sprintf(sz, "%d" , value);
    this->operator <<(sz);
    return *this;
}


loger& loger::operator<<(const char* value)
{
    int vlen = strlen(value);
    if(len + vlen < MAX_LEN )
    {
        sprintf(p_str+len, "%s", value );
        len += vlen;
    }
    else
    {
        output(p_str, len);
        sprintf(p_str+len, "%s", value );
        len += vlen;
    }

    return *this;
}

loger& loger::operator<<(std::string &value)
{
    if(len + value.length() < MAX_LEN )
    {
        sprintf(p_str+len, "%s", value.c_str() );
        len += value.length();
    }
    else
    {
        output(p_str, len);
        sprintf(p_str+len, "%s", value.c_str() );
        len += value.length();
    }
    return *this;
}

loger& loger::operator<<(std::string ( *ops)(const char* p) )
{
    //return *this->operator <<(ops(*p));
    return *this;
}

loger& loger::operator<<(std::ostream& ( *op)(std::ostream&))
{
    if(1)
    {
        cout << p_str << endl;
    }
    sprintf(p_str+len, "\r" );
    len += 1;
    output(p_str, len);
    return *this;
}


loger& loger::out(int p, const char* file, int line)
{
    if(0 != len )
    {
        sprintf(p_str+len,
                "\n");
        output(p_str, len+1);
        memset(p_str, 0x0, MAX_LEN );
        len = 0;
    }
    sprintf(p_str, "%s %d: ", FILE_NAME(file), line);
    len = strlen(p_str);
    return *this;
}

int loger::output(const char* p, int len)
{
    int iRet = 0;
    do
    {
        if (0 > iRet )
        {
            return -1;
        }
//        iRet = fwrite(p,sizeof(char),len,g_pFile);
//        if (0 > iRet )
//        {
//            return -1;
//        }
//        fflush(g_pFile);
    } while (0);
    memset(p_str, 0x0, MAX_LEN);
    this->len = 0;
    return iRet;
}


