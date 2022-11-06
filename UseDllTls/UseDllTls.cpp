#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_CREATETHREAD(Fun,Param) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Fun,Param,0,NULL)

VOID* GetThreadBuf();
UINT GetThreadBufSize();

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	GRS_USEPRINTF();
	TCHAR* pTLSBuf = (TCHAR*)GetThreadBuf();
	UINT nSize = GetThreadBufSize();
	if( NULL != pTLSBuf && 0 != nSize )
	{
		GRS_PRINTF(_T("线程[0x%x]取得缓冲[P:0x%08x S:%u],写入数据\n")
			,GetCurrentThreadId(),pTLSBuf,nSize);
		StringCchPrintf(pTLSBuf,nSize/sizeof(TCHAR),_T("ID:0x%x"),GetCurrentThreadId());
		Sleep(1000);
		GRS_PRINTF(_T("线程[0x%x]取得缓冲[P:0x%08x S:%u],写入数据[%s]\n")
			,GetCurrentThreadId(),pTLSBuf,nSize,pTLSBuf);
	}
	GRS_PRINTF(_T("线程[0x%x]退出\n"),GetCurrentThreadId());
	return 0;
}

#define THREADCNT 2

int _tmain()
{

	GRS_USEPRINTF();

	HANDLE phThread[THREADCNT] = {};
	for(int i = 0; i < THREADCNT; i ++)
	{
		phThread[i] = GRS_CREATETHREAD(ThreadProc,NULL);
	}
	WaitForMultipleObjects(THREADCNT,phThread,TRUE,INFINITE);
	for(int i = 0; i < THREADCNT; i ++)
	{
		CloseHandle(phThread[i]);
	}
	_tsystem(_T("PAUSE"));
	return 0;
}
