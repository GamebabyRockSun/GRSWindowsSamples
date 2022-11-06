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


__inline DWORD GetAppPath(LPTSTR pszBuf)
{
	GRS_USEPRINTF();
	DWORD dwLen = 0;
	if(0 == (dwLen = ::GetModuleFileName(NULL,pszBuf,MAX_PATH)))
	{
		GRS_PRINTF(_T("获取APP路径失败,错误码0x%08x\n"),GetLastError());
		return 0;
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
	return i;
}

//将要更新其资源的应用程序的资源ID
#define IDR_WAVE1                       101

int _tmain()
{
	GRS_USEPRINTF();
	HMODULE hModule = GetModuleHandle(NULL);//传入NULL取得该EXE的模块句柄

	HRSRC hRc = FindResource(hModule,MAKEINTRESOURCE(IDR_SUBEXE),RT_RCDATA);
	if( NULL == hRc )
	{
		GRS_PRINTF(_T("查找资源[Type:%s  Name:%s]失败\n"),RT_RCDATA,MAKEINTRESOURCE(IDR_SUBEXE));
		_tsystem(_T("PAUSE"));
		return 0;
	}
	DWORD dwSize = SizeofResource(hModule,hRc);
	if( 0 == dwSize )
	{
		GRS_PRINTF(_T("获取资源[Type:%s  Name:%s HRSRC:0x%08x]大小失败,原因:0x%08x\n")
			,RT_RCDATA,MAKEINTRESOURCE(IDR_SUBEXE),hRc,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	HGLOBAL hData = LoadResource(hModule,hRc);
	if( NULL == hRc )
	{
		GRS_PRINTF(_T("装载资源[Type:%s  Name:%s HRSRC:0x%08x]失败,原因:0x%08x\n")
			,RT_RCDATA,MAKEINTRESOURCE(IDR_SUBEXE),hRc,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	LPVOID pResData = LockResource(hData);
	if( NULL == pResData )
	{
		GRS_PRINTF(_T("锁定资源[Type:%s  Name:%s HRSRC:0x%08x HGLOBAL:0x%08x]失败,原因:0x%08x\n")
			,RT_RCDATA,MAKEINTRESOURCE(IDR_SUBEXE),hRc,hData,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	//=======================================================================================================
	//将资源还原成exe
	TCHAR pExeName[MAX_PATH] = {};
	GetAppPath(pExeName);
	StringCchCat(pExeName,MAX_PATH,_T("Temp.exe"));
	HANDLE hFile = CreateFile(pExeName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if( INVALID_HANDLE_VALUE == hFile )
	{
		GRS_PRINTF(_T("创建文件[%s]失败,原因:0x%08x\n")
			,pExeName,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	DWORD dwWrite = 0;
	if(!WriteFile(hFile,pResData,dwSize,&dwWrite,NULL))
	{
		GRS_PRINTF(_T("写入文件[%s]内容失败,原因:0x%08x\n")
			,pExeName,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}
	CloseHandle(hFile);
	//=======================================================================================================

	//=======================================================================================================
	//读取出Wav的数据先
	TCHAR pNewWavName[MAX_PATH] = _T("ding.wav");
	DWORD dwLen = GetAppPath(pNewWavName);
	for(DWORD i = dwLen - 1; i > 0; i -- )
	{
		if( '\\' == pNewWavName[i] )
		{
			pNewWavName[i + 1] = '\0';
			break;
		}
	}
	StringCchCat(pNewWavName,MAX_PATH,_T("ding.wav"));
	HANDLE hWavFile = CreateFile(pNewWavName,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
	if( INVALID_HANDLE_VALUE == hWavFile )
	{
		GRS_PRINTF(_T("打开文件[%s]失败,原因:0x%08x\n")
			,pNewWavName,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	DWORD dwWavLen = GetFileSize(hWavFile,NULL);
	if( 0 == dwWavLen )
	{
		GRS_PRINTF(_T("获取文件[%s]长度失败,原因:0x%08x\n")
			,pNewWavName,GetLastError());
		_tsystem(_T("PAUSE"));
		CloseHandle(hWavFile);
		return 0;
	}

	LPVOID pNewWavData = GRS_CALLOC(dwWavLen);
	if(!ReadFile(hWavFile,pNewWavData,dwWavLen,&dwWavLen,NULL))
	{
		GRS_PRINTF(_T("读取文件[%s]失败,原因:0x%08x\n")
			,pNewWavName,GetLastError());
		_tsystem(_T("PAUSE"));
		CloseHandle(hWavFile);
		GRS_SAFEFREE(pNewWavData);
		return 0;
	}
	CloseHandle(hWavFile);
	//=======================================================================================================

	//=======================================================================================================
	//更新资源,用名为ding.wav的音频文件替换exeinres中的那个
	HANDLE hUpdateRes = BeginUpdateResource(pExeName,FALSE);
	if( NULL == hUpdateRes )
	{
		GRS_PRINTF(_T("打开程序[%s]更新资源句柄失败,原因:0x%08x\n")
			,pExeName,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	BOOL bRet = UpdateResource(hUpdateRes,_T("WAVE"),MAKEINTRESOURCE(IDR_WAVE1),GetUserDefaultLangID(),pNewWavData,dwWavLen);
	
	if(!bRet)
	{
		GRS_PRINTF(_T("更新程序[%s]资源[%s]失败,原因:0x%08x\n")
			,pExeName,MAKEINTRESOURCE(IDR_WAVE1),GetLastError());
		GRS_SAFEFREE(pNewWavData);
		_tsystem(_T("PAUSE"));
	}
	
	if(!EndUpdateResource(hUpdateRes,FALSE))
	{
		GRS_PRINTF(_T("更新程序[%s]资源失败,原因:0x%08x\n")
			,pExeName,GetLastError());
		GRS_SAFEFREE(pNewWavData);
		_tsystem(_T("PAUSE"));
		return 0;
	}
	GRS_SAFEFREE(pNewWavData);
	//=======================================================================================================

	//=======================================================================================================
	//启动Exe 可以听到新的wav的声音
	STARTUPINFO si = {sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pi = {};
	CreateProcess(pExeName,NULL,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi);
	WaitForSingleObject(pi.hProcess,INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	//=======================================================================================================
	
	//Exe用完后就删除掉
	DeleteFile(pExeName);

	_tsystem(_T("PAUSE"));
	return 0;
}