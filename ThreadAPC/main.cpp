#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};DWORD dwRead = 0;
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SCANF() ReadConsole(GetStdHandle(STD_INPUT_HANDLE),pBuf,1,&dwRead,NULL);

DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{//线程函数
	HANDLE hEvnet = (HANDLE)lpParam;
	GRS_USEPRINTF();
	GRS_PRINTF(_T("Thread Function Run In Waiting!\n"));
	WaitForSingleObjectEx(hEvnet,INFINITE,TRUE);
	CloseHandle(hEvnet);
	GRS_PRINTF(_T("Thread Function Run End!\n"));
	return 0; 
} 

VOID CALLBACK APCFunction(ULONG_PTR dwParam)
{
	int i = dwParam;
	GRS_USEPRINTF();
	GRS_PRINTF(_T("%d APC Function Run!\n"),i);
	Sleep(20);
}

int _tmain()
{
	HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	HANDLE hEventDup = NULL;
	DuplicateHandle(GetCurrentProcess(),hEvent,GetCurrentProcess(),&hEventDup
		,DUPLICATE_SAME_ACCESS,FALSE,0);
	HANDLE hThread = CreateThread(NULL,0,ThreadFunction,hEventDup,CREATE_SUSPENDED,NULL);
	//排队APC函数
	for(int i = 0;i < 100;i++)
	{
		QueueUserAPC(APCFunction,hThread,i);
	}
	ResumeThread(hThread);

	GRS_USEPRINTF();
	GRS_PRINTF(_T("按回车键，等待线程将结束等待......\n"));
	GRS_SCANF();
	SetEvent(hEvent);
	CloseHandle(hEvent);

	WaitForSingleObject(hThread,INFINITE);
	CloseHandle(hThread);

	_tsystem(_T("PAUSE"));
	return 0;
}