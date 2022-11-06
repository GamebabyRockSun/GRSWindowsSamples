#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};DWORD dwRead = 0;
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_SCANF() ReadConsole(GetStdHandle(STD_INPUT_HANDLE),pBuf,1,&dwRead,NULL);

__declspec(thread) DWORD g_dwThreadID = 0;
//DWORD g_dwThreadID = 0;

DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{//Ïß³Ìº¯Êý
	g_dwThreadID = GetCurrentThreadId();
	Sleep(1000); 
	GRS_USEPRINTF();
	GRS_PRINTF(_T("Thread ID is 0x%x,g_dwThreadID = 0x%x,g_dwThreadID Address is 0x%08X\n")
		,GetCurrentThreadId(),g_dwThreadID,&g_dwThreadID);
	return 0; 
} 

int _tmain()
{
	GRS_USEPRINTF();
	g_dwThreadID = GetCurrentThreadId();
	for(int i = 0; i < 10;i ++)
	{
		HANDLE hThread = CreateThread(NULL,0,ThreadFunction,NULL,CREATE_SUSPENDED,NULL);
		ResumeThread(hThread);
		CloseHandle(hThread);
	}	
	Sleep(1000);
	GRS_PRINTF(_T("Main Thread ID is 0x%x,g_dwThreadID = 0x%x,g_dwThreadID Address is 0x%08X\n")
		,GetCurrentThreadId(),g_dwThreadID,&g_dwThreadID);
	
	_tsystem(_T("PAUSE"));
	return 0;
}
