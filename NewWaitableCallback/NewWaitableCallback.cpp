#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

VOID CALLBACK MyWaitCallback(
                             PTP_CALLBACK_INSTANCE Instance,
                             PVOID                 Parameter,
                             PTP_WAIT              Wait,
                             TP_WAIT_RESULT        WaitResult
                             )
{
    GRS_USEPRINTF();
    GRS_PRINTF(_T("[ID:0x%x] MyWaitCallback Running......\n"),GetCurrentThreadId());
}

int _tmain()
{
    GRS_USEPRINTF();
    PTP_WAIT Wait = NULL;
    PTP_WAIT_CALLBACK waitcallback = MyWaitCallback;
    HANDLE hEvent = NULL;
    UINT i = 0;
    UINT rollback = 0;

    //创建一个事件对象
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (NULL == hEvent)
    {
        return 0;
    }

    rollback = 1; // CreateEvent succeeded

    //创建等待线程池
    Wait = CreateThreadpoolWait(waitcallback,NULL,NULL);
    if(NULL == Wait) 
    {
        GRS_PRINTF(_T("CreateThreadpoolWait failed. LastError: %u\n"),
            GetLastError());
        goto new_wait_cleanup;
    }

    rollback = 2;

    //模拟等待五次,注意每次等待前要调用SetThreadpoolWait方法
    for (i = 0; i < 5; i ++)
    {
        SetThreadpoolWait(Wait,hEvent,NULL);
        SetEvent(hEvent);
        Sleep(500);
        ////主线程等待回调线程池调用完毕
        WaitForThreadpoolWaitCallbacks(Wait, TRUE);
    }

new_wait_cleanup:
    switch (rollback)
    {
    case 2:
        SetThreadpoolWait(Wait, NULL, NULL);
        CloseThreadpoolWait(Wait);
    case 1:
        CloseHandle(hEvent);
    default:
        break;
    }

    _tsystem(_T("PAUSE"));
    return 0;
}