#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_CREATETHREAD(Fun,Param) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Fun,Param,0,NULL)

__declspec(dllimport) void Fun(void);

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("线程[0x%x]被创建开始运行......\n")
		,GetCurrentThreadId());
	Sleep(1000);
	GRS_PRINTF(_T("线程[0x%x]退出\n")
		,GetCurrentThreadId());
	return 0;
}

#define THREADCNT 2

int _tmain()
{
	
	GRS_USEPRINTF();
	GRS_PRINTF(_T("进程[%u]启动主线程[0x%x]开始运行......\n")
		,GetCurrentProcessId(),GetCurrentThreadId());

	Fun();

	HANDLE phThread[THREADCNT] = {};
	for(int i = 0; i < THREADCNT; i ++)
	{
		phThread[i] = GRS_CREATETHREAD(ThreadProc,NULL);
	}

	WaitForMultipleObjects(THREADCNT,phThread,TRUE,INFINITE);

	_tsystem(_T("PAUSE"));

	for(int i = 0; i < THREADCNT; i ++)
	{
		CloseHandle(phThread[i]);
	}

	GRS_PRINTF(_T("进程[%u]运行完毕主线程[0x%x]退出\n")
		,GetCurrentProcessId(),GetCurrentThreadId());
	_tsystem(_T("PAUSE"));
	return 0;
}
