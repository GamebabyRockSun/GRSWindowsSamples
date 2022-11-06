#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

DWORD g_dwTLSID = 0;

DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{//Ïß³Ìº¯Êý
	TlsSetValue(g_dwTLSID,(LPVOID)GetCurrentThreadId());
	GRS_USEPRINTF();
	GRS_PRINTF(_T("Thread ID is 0x%x,TLS Saved Value = 0x%x\n")
		,GetCurrentThreadId(),TlsGetValue(g_dwTLSID));
	return 0; 
} 

int _tmain()
{
	GRS_USEPRINTF();
	g_dwTLSID = TlsAlloc();
	TlsSetValue(g_dwTLSID,(LPVOID)GetCurrentThreadId());
	for(int i = 0; i < 10;i ++)
	{
		HANDLE hThread = CreateThread(NULL,0,ThreadFunction,NULL,CREATE_SUSPENDED,NULL);
		ResumeThread(hThread);
		CloseHandle(hThread);
	}	

	GRS_PRINTF(_T("Main Thread ID is 0x%x,TLS Saved Value = 0x%x,g_dwTLSID is 0x%08X\n")
		,GetCurrentThreadId(),TlsGetValue(g_dwTLSID),g_dwTLSID);
	_tsystem(_T("PAUSE"));

	TlsFree(g_dwTLSID);
	return 0;
}
