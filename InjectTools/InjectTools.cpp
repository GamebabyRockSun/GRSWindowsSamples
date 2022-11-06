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
		GRS_PRINTF(_T("ע�����[ID:%u]OpenProcessʧ��,������:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded) )
	{
		GetModuleBaseName( hProcess, hMod, szProcessName, 
			sizeof(szProcessName)/sizeof(TCHAR) );
	}

	GRS_PRINTF(_T("��ʼΪ����[%u|(%s)]ע��DLL:\t")
		,dwProcessID,szProcessName,psDllPathName);

	StringCchLengthW(psDllPathName,4096,&szLen);
	pszRemote = (LPWSTR)VirtualAllocEx(hProcess,NULL,szLen,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
	if( NULL == pszRemote )
	{
		GRS_PRINTF(_T("ע�����[ID:%u]VirtualAllocExʧ��,������:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	if(!WriteProcessMemory(hProcess,pszRemote,psDllPathName,szLen * sizeof(WCHAR),(SIZE_T*)&szLen))
	{
		GRS_PRINTF(_T("ע�����[ID:%u]WriteProcessMemoryʧ��,������:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	PTHREAD_START_ROUTINE pLoadLibrary = (PTHREAD_START_ROUTINE)GetProcAddress(
		GetModuleHandle(_T("Kernel32.dll")),"LoadLibraryW");
	if(NULL == pLoadLibrary)
	{
		GRS_PRINTF(_T("ע�����[ID:%u]GetProcAddressʧ��,������:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	//��LoadLibrary��ΪԶ���̵߳ĵ�ַ,���Ǹ��Ĳ���Ϊ,�Ĳ�ͬ������LoadLibrary��ַ��ͬ
	hThread = CreateRemoteThread(hProcess,NULL,0,pLoadLibrary,pszRemote,0,NULL);
	if(NULL == hThread)
	{
		GRS_PRINTF(_T("ע�����[ID:%u]CreateRemoteThreadʧ��,������:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	if(WAIT_OBJECT_0 != WaitForSingleObject(hThread,INFINITE))
	{
		GRS_PRINTF(_T("ע�����[ID:%u]WaitForSingleObjectʧ��,������:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	DWORD dwBaseAddr = 0;
	if(!GetExitCodeThread(hThread,&dwBaseAddr))
	{
		GRS_PRINTF(_T("ע�����[ID:%u]GetExitCodeThreadʧ��,������:0x%08X\n")
			,dwProcessID,GetLastError());
		goto InjectDll_Clear;
	}

	GRS_PRINTF(_T("ע�����[%u]�ɹ�,��ַ[0x%08x]\n")
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
		GRS_PRINTF(_T("��ȡAPP·��ʧ��,������0x%08x\n"),GetLastError());
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
		GRS_PRINTF(_T("���ܵõ�ϵͳ�����н��̵�ID,�����޷����\n"));
		return (int)GetLastError();
	}

	cProcesses = cbNeeded / sizeof(DWORD);
    for (DWORD i = 0; i < cProcesses; i++ )
	{//Ϊϵͳ��ÿ�����̶�ע��һ�����dll
		InjectDll(aProcesses[i],pszInjectDll);
	}	

	_tsystem(_T("PAUSE"));

	return 0;
}