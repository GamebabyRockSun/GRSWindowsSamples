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

#define THREADCOUNT 4 

HANDLE ghWriteEvent = NULL; 
HANDLE ghThreads[THREADCOUNT] = {};

DWORD WINAPI ThreadProc(LPVOID);

void CreateEventsAndThreads(void) 
{
    int i = 0; 
    DWORD dwThreadID = 0;
	//�������ֶ������¼�����״̬����ʹ�����̶߳��˳�
	//�����Ϊ�Զ�,��ô��ֻ��һ���߳��л���ȵ����¼�,�����������
	//����ǳɹ��ȴ��ĸ�����(��ҩ�︱����һ��)
    ghWriteEvent = CreateEvent(NULL,TRUE,FALSE,_T("WriteEvent")); 
    for(i = 0; i < THREADCOUNT; i++) 
    {
        ghThreads[i] = CreateThread(NULL,0,ThreadProc,NULL,0,&dwThreadID);
    }
}

void WriteToBuffer(VOID) 
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("Main thread writing to the shared buffer...\n"));
	if (! SetEvent(ghWriteEvent) ) 
    {
        GRS_PRINTF(_T("SetEvent failed (%d)\n"), GetLastError());
        return;
    }
}

void CloseEvents()
{
    CloseHandle(ghWriteEvent);
}

void _tmain()
{
	GRS_USEPRINTF();
    DWORD dwWaitResult = 0;
    CreateEventsAndThreads();    
    WriteToBuffer();
    GRS_PRINTF(_T("Main thread waiting for threads to exit...\n"));
    dwWaitResult = WaitForMultipleObjects(THREADCOUNT,ghThreads,TRUE,INFINITE);
    switch (dwWaitResult) 
    {
        case WAIT_OBJECT_0: 
            GRS_PRINTF(_T("All threads ended, cleaning up for application exit...\n"));
            break;
        default: 
            GRS_PRINTF(_T("WaitForMultipleObjects failed (%d)\n"), GetLastError());
            return;
    }
    CloseEvents();

	_tsystem(_T("PAUSE"));
}

DWORD WINAPI ThreadProc(LPVOID lpParam) 
{
	GRS_USEPRINTF();
    DWORD dwWaitResult = 0;
    GRS_PRINTF(_T("Thread %d waiting for write event...\n"), GetCurrentThreadId());
    dwWaitResult = WaitForSingleObject(ghWriteEvent,INFINITE);
    switch (dwWaitResult) 
    {
        case WAIT_OBJECT_0: 
            GRS_PRINTF(_T("Thread %d reading from buffer\n"),GetCurrentThreadId());
            break; 
        default: 
            GRS_PRINTF(_T("Wait error (%d)\n"), GetLastError()); 
            return 0; 
    }
    GRS_PRINTF(_T("Thread %d exiting\n"), GetCurrentThreadId());
    return 1;
}


