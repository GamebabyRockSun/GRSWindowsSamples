#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{//�̺߳���
	GRS_USEPRINTF();
	GRS_PRINTF(_T("2���߳�[ID:0x%x]����\n"),GetCurrentThreadId());	
	SwitchToThread();
	//Sleep(1);
	GRS_PRINTF(_T("4���߳�[ID:0x%x]����\n"),GetCurrentThreadId());	
	
	return 0; 
} 

int _tmain()
{
	GRS_USEPRINTF();

	DWORD  dwThreadID = 0;
	HANDLE hThread = CreateThread(NULL,0,ThreadFunction,NULL,0,&dwThreadID);
	GRS_PRINTF(_T("1���߳�[ID:0x%x]������\n"),dwThreadID);	
	//SwitchToThread();
	Sleep(1);
	GRS_PRINTF(_T("3�����߳�[ID:0x%x]ִ��\n"),GetCurrentThreadId());	
	//SwitchToThread();
	Sleep(1);
	GRS_PRINTF(_T("5�����߳�[ID:0x%x]ִ��\n"),GetCurrentThreadId());	
	//�ȴ����߳��˳�
	WaitForSingleObject(hThread,INFINITE);

	CloseHandle(hThread);
	_tsystem(_T("PAUSE"));
	return 0;
}