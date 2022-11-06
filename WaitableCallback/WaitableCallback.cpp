#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

VOID CALLBACK WaitCallback(PVOID lpParameter,BOOLEAN bWaitFired)
{
    GRS_USEPRINTF();
    if(!bWaitFired)
    {
        GRS_PRINTF(_T("[ID:0x%x] WaitCallback Success\n"),GetCurrentThreadId());
    }
    else
    {
        GRS_PRINTF(_T("[ID:0x%x] WaitCallback Faild\n"),GetCurrentThreadId());
    }
    
}

int _tmain()
{
    GRS_USEPRINTF();
    GRS_PRINTF(_T("主线程[ID:0x%x] Running\n"),GetCurrentThreadId());
    HANDLE hEvent = NULL;
    UINT i = 0;

    //创建一个事件对象
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == hEvent)
    {
        return 0;
    }

    //模拟等待五次
    HANDLE hNewWait = NULL;
    //WT_EXECUTEONLYONCE
    RegisterWaitForSingleObject(&hNewWait,hEvent,WaitCallback,NULL,INFINITE,WT_EXECUTEDEFAULT);

    for (i = 0; i < 5; i ++)
    {
        SetEvent(hEvent);
        Sleep(500);
    }

    UnregisterWaitEx(hNewWait,INVALID_HANDLE_VALUE);
    
    CloseHandle(hEvent);
    _tsystem(_T("PAUSE"));
    return 0;
}