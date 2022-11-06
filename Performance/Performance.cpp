#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

int _tmain()
{
    GRS_USEPRINTF();

    LARGE_INTEGER liFrequency = {};
    LARGE_INTEGER liCurCount = {};
    LARGE_INTEGER liOrgCount = {};

    QueryPerformanceFrequency(&liFrequency);

    GRS_PRINTF(_T("����Ƶ��Ϊ:%lluHZ\n"),liFrequency.QuadPart);
    QueryPerformanceCounter(&liOrgCount);
    GRS_PRINTF(_T("��ʼ����,������ֵΪ:%llu\n"),liOrgCount.QuadPart);
    _tsystem(_T("PAUSE"));
    QueryPerformanceCounter(&liCurCount);

    LARGE_INTEGER liMisTime = {};
    liMisTime.QuadPart = liCurCount.QuadPart-liOrgCount.QuadPart;

    GRS_PRINTF(_T("��ʱ����,������ֵΪ:%llu\t��������ֵΪ%llu\t��ʱֵΪ:%f s\t%f ms\t%llu us\n")
        ,liCurCount.QuadPart,liMisTime.QuadPart
        ,(double)liMisTime.QuadPart/liFrequency.QuadPart
        ,(double)(liMisTime.QuadPart * 1000) / liFrequency.QuadPart
        ,(liMisTime.QuadPart * 1000000) / liFrequency.QuadPart);
    _tsystem(_T("PAUSE"));
}