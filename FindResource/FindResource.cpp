#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include "resource.h"
#pragma comment(lib,"Winmm.lib")

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

__inline VOID GetAppPath(LPTSTR pszBuf)
{
	GRS_USEPRINTF();
	DWORD dwLen = 0;
	if(0 == (dwLen = ::GetModuleFileName(NULL,pszBuf,MAX_PATH)))
	{
		GRS_PRINTF(_T("��ȡAPP·��ʧ��,������0x%08x\n"),GetLastError());
		return;
	}
	DWORD i = dwLen;
	for(; i > 0; i -- )
	{
		if( '\\' == pszBuf[i] )
		{
			pszBuf[i + 1] = '\0';
			break;
		}
	}
}

int _tmain()
{
	GRS_USEPRINTF();
	HMODULE hModule = GetModuleHandle(NULL);//����NULLȡ�ø�EXE��ģ����
	
	HRSRC hRc = FindResource(hModule,MAKEINTRESOURCE(IDR_SUBEXE),RT_RCDATA);
	if( NULL == hRc )
	{
		GRS_PRINTF(_T("������Դ[Type:%s  Name:%s]ʧ��\n"),RT_RCDATA,MAKEINTRESOURCE(IDR_SUBEXE));
		_tsystem(_T("PAUSE"));
		return 0;
	}
	DWORD dwSize = SizeofResource(hModule,hRc);
	if( 0 == dwSize )
	{
		GRS_PRINTF(_T("��ȡ��Դ[Type:%s  Name:%s HRSRC:0x%08x]��Сʧ��,ԭ��:0x%08x\n")
			,RT_RCDATA,MAKEINTRESOURCE(IDR_SUBEXE),hRc,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	HGLOBAL hData = LoadResource(hModule,hRc);
	if( NULL == hRc )
	{
		GRS_PRINTF(_T("װ����Դ[Type:%s  Name:%s HRSRC:0x%08x]ʧ��,ԭ��:0x%08x\n")
			,RT_RCDATA,MAKEINTRESOURCE(IDR_SUBEXE),hRc,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	LPVOID pResData = LockResource(hData);
	if( NULL == pResData )
	{
		GRS_PRINTF(_T("������Դ[Type:%s  Name:%s HRSRC:0x%08x HGLOBAL:0x%08x]ʧ��,ԭ��:0x%08x\n")
			,RT_RCDATA,MAKEINTRESOURCE(IDR_SUBEXE),hRc,hData,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	TCHAR pExeName[MAX_PATH] = {};
	GetAppPath(pExeName);
	StringCchCat(pExeName,MAX_PATH,_T("Temp.exe"));
	HANDLE hFile = CreateFile(pExeName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if( INVALID_HANDLE_VALUE == hFile )
	{
		GRS_PRINTF(_T("�����ļ�[%s]ʧ��,ԭ��:0x%08x\n")
			,pExeName,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	DWORD dwWrite = 0;
	if(!WriteFile(hFile,pResData,dwSize,&dwWrite,NULL))
	{
		GRS_PRINTF(_T("д���ļ�[%s]����ʧ��,ԭ��:0x%08x\n")
			,pExeName,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}
	CloseHandle(hFile);

	STARTUPINFO si = {sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pi = {};
	CreateProcess(pExeName,NULL,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi);
	WaitForSingleObject(pi.hProcess,INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	//Exe������ɾ����
	DeleteFile(pExeName);
		
	_tsystem(_T("PAUSE"));
	return 0;
}