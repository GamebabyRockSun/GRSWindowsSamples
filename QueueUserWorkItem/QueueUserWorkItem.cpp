#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};
#define GRS_PRINTF(...) \
    StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

#define GRS_BEGINTHREAD(Fun,Param) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Fun,Param,0,NULL)


DWORD WINAPI ThreadProc(LPVOID lpParameter);

int _tmain()
{
    GRS_USEPRINTF();

    int iWaitLen = 0;
    do
    {
        GRS_PRINTF(_T("\n请输入一个等待时常值,单位秒(输入0退出):"))
        _tscanf(_T("%i"),&iWaitLen);
        if(iWaitLen > 0)
        {
            //下面的代码演示了,使用CreateThread和QueueUserWorkItem实际效果一样
            //当然线程不多的情况下如此,如果线程很多时一定要使用QueueUserWorkItem
            //GRS_BEGINTHREAD(ThreadProc,(PVOID)iWaitLen);
            QueueUserWorkItem(ThreadProc,(PVOID)iWaitLen,WT_EXECUTELONGFUNCTION);
        }
    }
    while(iWaitLen);

    return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{//线程函数,可以用CreateThread启动成线程,也可以使用QueueUserWorkItem启动为线程池形式
    int iWaitLen = (int)lpParameter;
    
    GRS_USEPRINTF();
    GRS_PRINTF(_T("\n线程[ID:0x%x]将等待%u秒......"),GetCurrentThreadId(),lpParameter);
    Sleep((DWORD)lpParameter * 1000);
    GRS_PRINTF(_T("\n线程[ID:0x%x]等待结束\n"),GetCurrentThreadId(),lpParameter);
    return 0;
}