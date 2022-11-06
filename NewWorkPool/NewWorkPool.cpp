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

//�򵥵Ĺ������
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

//�򵥵Ķ�ʱ�ص�����
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

    //��ʼ��������
    InitializeThreadpoolEnvironment(&CallBackEnviron);

    //�����̳߳�
    pool = CreateThreadpool(NULL);

    if (NULL == pool) 
    {
        GRS_PRINTF(_T("CreateThreadpool failed. LastError: %u\n"),
            GetLastError());
        goto main_cleanup;
    }

    rollback = 1; // pool creation succeeded
    
    //��������߳���
    SetThreadpoolThreadMaximum(pool, 8);

    //������С�߳���
    bRet = SetThreadpoolThreadMinimum(pool, 2);

    if (FALSE == bRet)
    {
        GRS_PRINTF(_T("SetThreadpoolThreadMinimum failed. LastError: %u\n"),
            GetLastError());
        goto main_cleanup;
    }

    //����һ����Դ������
    cleanupgroup = CreateThreadpoolCleanupGroup();

    if (NULL == cleanupgroup)
    {
        GRS_PRINTF(_T("CreateThreadpoolCleanupGroup failed. LastError: %u\n"), 
            GetLastError());
        goto main_cleanup; 
    }

    rollback = 2;  // Cleanup group creation succeeded
    //���û��������̳߳ع���
    SetThreadpoolCallbackPool(&CallBackEnviron, pool);

    //�����������뻷�������
    SetThreadpoolCallbackCleanupGroup(&CallBackEnviron,
        cleanupgroup,
        NULL);
    
    //�����̳߳���Ҫ�Ļص�����,��������ͨ�Ĺ�����,Ҳ����һ���򵥵Ļص�����
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

    //ȷ���ύ������
    SubmitThreadpoolWork(work);

    //����һ����ʱ�ص���
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

    //�趨��ʱ�ص�����
    ulDueTime.QuadPart = (LONGLONG) -(1 * 10 * 1000 * 1000);
    FileDueTime.dwHighDateTime = ulDueTime.HighPart;
    FileDueTime.dwLowDateTime  = ulDueTime.LowPart;

    SetThreadpoolTimer(timer,
        &FileDueTime,
        0,
        0);
    
    //���߳̽���ȴ�״̬��ɱ�Ĺ���
    Sleep(1500);

    //�����е��̳߳ػص���������ִ�к�,�ر�������
    CloseThreadpoolCleanupGroupMembers(cleanupgroup,FALSE,NULL);

    //�����ǻ�����ڶ���,ִ�еڶ�����Ķ������ٹ���
    rollback = 2;
    goto main_cleanup;

main_cleanup:
    //���������¼����������Ӧ�Ķ���
    switch (rollback)
    {
    case 4:
    case 3:
        //�ر�������
        CloseThreadpoolCleanupGroupMembers(cleanupgroup,
            FALSE, NULL);
    case 2:
        //�ر�������
        CloseThreadpoolCleanupGroup(cleanupgroup);

    case 1:
        //�ر��̳߳�
        CloseThreadpool(pool);
    default:
        break;
    }

    _tsystem(_T("PAUSE"));
    
    return 0;
}