#ifndef LOGER_H
#define LOGER_H

#ifdef WIN32
# ifndef I_OS_WINDOWS
# define I_OS_WINDOWS
# endif
#elif unix
# ifndef I_OS_LINUX
# define I_OS_LINUX
# endif
#endif

#include <iostream>
#include <ostream>
#include <stdio.h>
#include <iosfwd>
#include "define.h"
#include "pubfunc.h"

#ifdef I_OS_WINDOWS
#include <Windows.h>
#else
#endif

#ifdef I_OS_WINDOWS
#define MTR_STR std::string
#else
#define MTR_STR std::string
#endif

using namespace std;

#define DEBUG           0
#define INFO            1
#define WARING          2
#define ERROR           3


class loger;
class _Elem;
class _Traits;


#ifdef WIN32
typedef std::ostream& (__cdecl *op)(std::ostream&);
#else
typedef std::ostream& ( *op)(std::ostream&);
typedef std::string& ( *ops)(const char*);
#endif

class loger
{
public:
    loger() ;
    int init();
    loger& operator<<(int value);
    loger& operator<<(const char* value);
    loger& operator<<(std::string &value);
    loger& operator<<(std::string ( *ops)(const char*) );
    loger& operator<<(std::ostream& ( *op)(std::ostream&) );
    loger& out(int p, const char* file, int line);
private:
    int output(const char* p, int len);
private:
    FILE* g_pFile;
    string g_strLogFile;
    char* p_str;
    int len;
};

extern loger *g_loger;
#define LOG(p) g_loger->out(p, __FILE__, __LINE__)
/*
inline std::string& tr(const char* p)
{
    static std::string str;
    CPubfunc::tr(p, str);
    return str;
}

inline std::string& tr(unsigned long p)
{
    static std::string str;
    CPubfunc::tr(p, str);
    return str;
}
*/
#endif // LOGER_H
