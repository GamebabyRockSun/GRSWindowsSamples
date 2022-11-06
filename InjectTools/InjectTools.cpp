#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <Psapi.h>
#pragma comment(lib,"psapi")

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);


BOOL InjectDll(DWORD dwProcessID,LPCWSTR psDllPathName)
{
	GRS_USEPRINTF();
	HANDLE hProcess = NULL;
	HANDLE hThread = NULL;
	size_t szLen = 0;
	LPWSTR pszRemote = NULL;
	HMODULE hMod = NULL;
	DWORD cbNeeded = 0;
	TCHAR szProcessName[MAX_PATH] = {TEXT("<unknown>")};

	hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwProcessID);
	
	if( NULL == hProcess )
	{
		GRS_PRINTF(_T("注入进程[ID:%u]OpenProcess失败,错误码:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded) )
	{
		GetModuleBaseName( hProcess, hMod, szProcessName, 
			sizeof(szProcessName)/sizeof(TCHAR) );
	}

	GRS_PRINTF(_T("开始为进程[%u|(%s)]注入DLL:\t")
		,dwProcessID,szProcessName,psDllPathName);

	StringCchLengthW(psDllPathName,4096,&szLen);
	pszRemote = (LPWSTR)VirtualAllocEx(hProcess,NULL,szLen,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if( NULL == pszRemote )
	{
		GRS_PRINTF(_T("注入进程[ID:%u]VirtualAllocEx失败,错误码:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	if(!WriteProcessMemory(hProcess,pszRemote,psDllPathName,szLen * sizeof(WCHAR),(SIZE_T*)&szLen))
	{
		GRS_PRINTF(_T("注入进程[ID:%u]WriteProcessMemory失败,错误码:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	PTHREAD_START_ROUTINE pLoadLibrary = (PTHREAD_START_ROUTINE)GetProcAddress(
		GetModuleHandle(_T("Kernel32.dll")),"LoadLibraryW");
	if(NULL == pLoadLibrary)
	{
		GRS_PRINTF(_T("注入进程[ID:%u]GetProcAddress失败,错误码:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	//将LoadLibrary作为远程线程的地址,这是个赌博行为,赌不同进程中LoadLibrary地址相同
	hThread = CreateRemoteThread(hProcess,NULL,0,pLoadLibrary,pszRemote,0,NULL);
	if(NULL == hThread)
	{
		GRS_PRINTF(_T("注入进程[ID:%u]CreateRemoteThread失败,错误码:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	if(WAIT_OBJECT_0 != WaitForSingleObject(hThread,INFINITE))
	{
		GRS_PRINTF(_T("注入进程[ID:%u]WaitForSingleObject失败,错误码:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	DWORD dwBaseAddr = 0;
	if(!GetExitCodeThread(hThread,&dwBaseAddr))
	{
		GRS_PRINTF(_T("注入进程[ID:%u]GetExitCodeThread失败,错误码:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	GRS_PRINTF(_T("注入进程[%u]成功,基址[0x%08x]\n")
		,psDllPathName,dwProcessID,dwBaseAddr);

InjectDll_Clear:
	if( NULL != pszRemote )
	{
		VirtualFreeEx(hProcess,pszRemote,0,MEM_RELEASE);
		pszRemote = NULL;
	}
	if( NULL != hProcess )
	{
		CloseHandle(hProcess);
		hProcess = NULL;
	}
	if(NULL != hThread)
	{
		CloseHandle(hThread);
		hThread = NULL;
	}
	return TRUE;
}

__inline VOID GetAppPathW(LPWSTR pszBuf)
{
	GRS_USEPRINTF();
	DWORD dwLen = 0;
	if(0 == (dwLen = ::GetModuleFileNameW(NULL,pszBuf,MAX_PATH)))
	{
		GRS_PRINTF(_T("获取APP路径失败,错误码0x%08x\n"),GetLastError());
		return;
	}
	DWORD i = dwLen;
	for(; i > 0; i -- )
	{
		if( L'\\' == pszBuf[i] )
		{
			pszBuf[i + 1] = L'\0';
			break;
		}
	}
}

int _tmain()
{
	GRS_USEPRINTF();	
	WCHAR pszInjectDll[MAX_PATH] = {};
	GetAppPathW(pszInjectDll);
	StringCchCatW(pszInjectDll,MAX_PATH,L"InjectDll.dll");

	DWORD aProcesses[1024], cbNeeded, cProcesses;
    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
	{
		GRS_PRINTF(_T("不能得到系统中所有进程的ID,操作无法完成\n"));
		return (int)GetLastError();
	}

	cProcesses = cbNeeded / sizeof(DWORD);
    for (DWORD i = 0; i < cProcesses; i++ )
	{//为系统中每个进程都注入一下这个dll
		InjectDll(aProcesses[i],pszInjectDll);
	}	

	_tsystem(_T("PAUSE"));

	return 0;
}