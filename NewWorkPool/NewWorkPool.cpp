#define _WIN32_WINNT 0x0600 
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

//简单的工作项函数
VOID CALLBACK MyWorkCallback(
                             PTP_CALLBACK_INSTANCE Instance,
                             PVOID                 Parameter,
                             PTP_WORK              Work
                             )
{
    GRS_USEPRINTF();

    BOOL bRet = FALSE;
    DWORD dwPriorityOriginal = 0;

    dwPriorityOriginal = GetThreadPriority(GetCurrentThread());

    if (THREAD_PRIORITY_ERROR_RETURN == dwPriorityOriginal) {
        GRS_PRINTF(_T("GetThreadPriority failed.  LastError: %u\n"),
            GetLastError());
        return;
    }
    bRet = SetThreadPriority(GetCurrentThread(),
        THREAD_PRIORITY_ABOVE_NORMAL);

    if (FALSE == bRet) {
        GRS_PRINTF(_T("SetThreadPriority failed.  LastError: %u\n"),
            GetLastError());
        return;
    }

    {
        // ...
        GRS_PRINTF(_T("[ID:0x%x] MyWorkCallback Running......\n"),GetCurrentThreadId());
    }

    bRet = SetThreadPriority(GetCurrentThread(),
        dwPriorityOriginal);

    if (FALSE == bRet)
    {
        GRS_PRINTF(_T("Fatal Error! SetThreadPriority failed. LastError: %u\n"),
            GetLastError());
        return;
    }

    return;
}

//简单的定时回调函数
VOID CALLBACK MyTimerCallback(
                              PTP_CALLBACK_INSTANCE Instance,
                              PVOID                 Parameter,
                              PTP_TIMER             Timer
                              )
{
    GRS_USEPRINTF();
    GRS_PRINTF(_T("[ID:0x%x] MyTimerCallback Running......\n"),GetCurrentThreadId());
}


int _tmain()
{
    GRS_USEPRINTF();

    BOOL bRet = FALSE;
    PTP_WORK work = NULL;
    PTP_TIMER timer = NULL;
    PTP_POOL pool = NULL;
    PTP_WORK_CALLBACK workcallback = MyWorkCallback;
    PTP_TIMER_CALLBACK timercallback = MyTimerCallback;
    TP_CALLBACK_ENVIRON CallBackEnviron;
    PTP_CLEANUP_GROUP cleanupgroup = NULL;
    FILETIME FileDueTime;
    ULARGE_INTEGER ulDueTime;
    UINT rollback = 0;

    //初始化环境块
    InitializeThreadpoolEnvironment(&CallBackEnviron);

    //创建线程池
    pool = CreateThreadpool(NULL);

    if (NULL == pool) 
    {
        GRS_PRINTF(_T("CreateThreadpool failed. LastError: %u\n"),
            GetLastError());
        goto main_cleanup;
    }

    rollback = 1; // pool creation succeeded
    
    //设置最大线程数
    SetThreadpoolThreadMaximum(pool, 8);

    //设置最小线程数
    bRet = SetThreadpoolThreadMinimum(pool, 2);

    if (FALSE == bRet)
    {
        GRS_PRINTF(_T("SetThreadpoolThreadMinimum failed. LastError: %u\n"),
            GetLastError());
        goto main_cleanup;
    }

    //创建一个资源清理器
    cleanupgroup = CreateThreadpoolCleanupGroup();

    if (NULL == cleanupgroup)
    {
        GRS_PRINTF(_T("CreateThreadpoolCleanupGroup failed. LastError: %u\n"), 
            GetLastError());
        goto main_cleanup; 
    }

    rollback = 2;  // Cleanup group creation succeeded
    //设置环境块与线程池关联
    SetThreadpoolCallbackPool(&CallBackEnviron, pool);

    //设置清理器与环境块关联
    SetThreadpoolCallbackCleanupGroup(&CallBackEnviron,
        cleanupgroup,
        NULL);
    
    //创建线程池需要的回调函数,这里是普通的工作项,也就是一个简单的回调函数
    work = CreateThreadpoolWork(workcallback,
        NULL, 
        &CallBackEnviron);

    if (NULL == work)
    {
        GRS_PRINTF(_T("CreateThreadpoolWork failed. LastError: %u\n"),
            GetLastError());
        goto main_cleanup;
    }

    rollback = 3;  // Creation of work succeeded

    //确认提交工作项
    SubmitThreadpoolWork(work);

    //创建一个定时回调项
    timer = CreateThreadpoolTimer(timercallback,
        NULL,
        &CallBackEnviron);
    if (NULL == timer)
    {
        GRS_PRINTF(_T("CreateThreadpoolTimer failed. LastError: %u\n"),
            GetLastError());
        goto main_cleanup;
    }

    rollback = 4;  // Timer creation succeeded

    //设定定时回调周期
    ulDueTime.QuadPart = (LONGLONG) -(1 * 10 * 1000 * 1000);
    FileDueTime.dwHighDateTime = ulDueTime.HighPart;
    FileDueTime.dwLowDateTime  = ulDueTime.LowPart;

    SetThreadpoolTimer(timer,
        &FileDueTime,
        0,
        0);
    
    //主线程进入等待状态或干别的工作
    Sleep(1500);

    //当所有的线程池回调函数都被执行后,关闭清理器
    CloseThreadpoolCleanupGroupMembers(cleanupgroup,FALSE,NULL);

    //事务标记会滚到第二步,执行第二步后的对象销毁工作
    rollback = 2;
    goto main_cleanup;

main_cleanup:
    //根据事务记录步骤销毁相应的对象
    switch (rollback)
    {
    case 4:
    case 3:
        //关闭清理器
        CloseThreadpoolCleanupGroupMembers(cleanupgroup,
            FALSE, NULL);
    case 2:
        //关闭清理器
        CloseThreadpoolCleanupGroup(cleanupgroup);

    case 1:
        //关闭线程池
        CloseThreadpool(pool);
    default:
        break;
    }

    _tsystem(_T("PAUSE"));
    
    return 0;
}