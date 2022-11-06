#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

HANDLE gDoneEvent = NULL;

VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
    GRS_USEPRINTF();

    if (lpParam == NULL)
    {
        GRS_PRINTF(_T("TimerRoutine lpParam is NULL\n"));
    }
    else
    {
         GRS_PRINTF(_T("Timer routine called. Parameter is %d.\n"), 
            *(int*)lpParam);
    }

    SetEvent(gDoneEvent);
}

int _tmain()
{
    GRS_USEPRINTF();

    HANDLE hTimer = NULL;
    HANDLE hTimerQueue = NULL;
    int arg = 123;

    // Use an event object to track the TimerRoutine execution
    gDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == gDoneEvent)
    {
        GRS_PRINTF(_T("CreateEvent failed (%d)\n"), GetLastError());
        return 1;
    }

    // Create the timer queue.
    hTimerQueue = CreateTimerQueue();
    if (NULL == hTimerQueue)
    {
        GRS_PRINTF(_T("CreateTimerQueue failed (%d)\n"), GetLastError());
        return 2;
    }

    if (!CreateTimerQueueTimer( &hTimer, hTimerQueue, 
        (WAITORTIMERCALLBACK)TimerRoutine, &arg , 10000, 0, 0))
    {
        GRS_PRINTF(_T("CreateTimerQueueTimer failed (%d)\n"), GetLastError());
        return 3;
    }

    GRS_PRINTF(_T("Call timer routine in 10 seconds...\n"));

    if (WaitForSingleObject(gDoneEvent, INFINITE) != WAIT_OBJECT_0)
    {
        GRS_PRINTF(_T("WaitForSingleObject failed (%d)\n"), GetLastError());
    }

    CloseHandle(gDoneEvent);

    if (!DeleteTimerQueue(hTimerQueue))
    {
        GRS_PRINTF(_T("DeleteTimerQueue failed (%d)\n"), GetLastError());
    }

    _tsystem(_T("PAUSE"));
    return 0;
}
