#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{//Ïß³Ìº¯Êý
	GRS_USEPRINTF();
	GRS_PRINTF(_T("Thread(0x%x) Runing!\n"),GetCurrentThreadId());
	return 0; 
} 

int _tmain()
{
	GRS_USEPRINTF();
	HANDLE hThread = CreateThread(NULL,0,ThreadFunction,NULL,CREATE_SUSPENDED,NULL);
	GRS_PRINTF(_T("Thread Created!\n"));
	ResumeThread(hThread);
	GRS_PRINTF(_T("Thread Resume!\n"));
	CloseHandle(hThread);
	_tsystem(_T("PAUSE"));
	return 0;
}
