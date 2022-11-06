#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{//Ïß³Ìº¯Êý
	HANDLE hEvnet = (HANDLE)lpParam;
	WaitForSingleObject(hEvnet,INFINITE);
	CloseHandle(hEvnet);

	return 0; 
} 

int _tmain()
{
	HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	HANDLE hEventDup = NULL;
	DuplicateHandle(GetCurrentProcess(),hEvent,GetCurrentProcess(),&hEventDup
		,DUPLICATE_SAME_ACCESS,FALSE,0);
	HANDLE hThread = CreateThread(NULL,0,ThreadFunction,hEventDup,CREATE_SUSPENDED,NULL);
	ResumeThread(hThread);

	SuspendThread(hThread);
	CONTEXT ct = {};
	ct.ContextFlags = CONTEXT_ALL;
	GetThreadContext(hThread,&ct);

	ResumeThread(hThread);
	SetEvent(hEvent);
	CloseHandle(hEvent);

	_tsystem(_T("PAUSE"));
	return 0;
}