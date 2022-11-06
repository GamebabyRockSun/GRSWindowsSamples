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
		GRS_PRINTF(_T("��ȡAPP·��ʧ��,������0x%08x\n"),GetLastError());
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

//��Ҫ��������Դ��Ӧ�ó������ԴID
#define IDR_WAVE1                       101

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

	//=======================================================================================================
	//����Դ��ԭ��exe
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
	//=======================================================================================================

	//=======================================================================================================
	//��ȡ��Wav��������
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
		GRS_PRINTF(_T("���ļ�[%s]ʧ��,ԭ��:0x%08x\n")
			,pNewWavName,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	DWORD dwWavLen = GetFileSize(hWavFile,NULL);
	if( 0 == dwWavLen )
	{
		GRS_PRINTF(_T("��ȡ�ļ�[%s]����ʧ��,ԭ��:0x%08x\n")
			,pNewWavName,GetLastError());
		_tsystem(_T("PAUSE"));
		CloseHandle(hWavFile);
		return 0;
	}

	LPVOID pNewWavData = GRS_CALLOC(dwWavLen);
	if(!ReadFile(hWavFile,pNewWavData,dwWavLen,&dwWavLen,NULL))
	{
		GRS_PRINTF(_T("��ȡ�ļ�[%s]ʧ��,ԭ��:0x%08x\n")
			,pNewWavName,GetLastError());
		_tsystem(_T("PAUSE"));
		CloseHandle(hWavFile);
		GRS_SAFEFREE(pNewWavData);
		return 0;
	}
	CloseHandle(hWavFile);
	//=======================================================================================================

	//=======================================================================================================
	//������Դ,����Ϊding.wav����Ƶ�ļ��滻exeinres�е��Ǹ�
	HANDLE hUpdateRes = BeginUpdateResource(pExeName,FALSE);
	if( NULL == hUpdateRes )
	{
		GRS_PRINTF(_T("�򿪳���[%s]������Դ���ʧ��,ԭ��:0x%08x\n")
			,pExeName,GetLastError());
		_tsystem(_T("PAUSE"));
		return 0;
	}

	BOOL bRet = UpdateResource(hUpdateRes,_T("WAVE"),MAKEINTRESOURCE(IDR_WAVE1),GetUserDefaultLangID(),pNewWavData,dwWavLen);
	
	if(!bRet)
	{
		GRS_PRINTF(_T("���³���[%s]��Դ[%s]ʧ��,ԭ��:0x%08x\n")
			,pExeName,MAKEINTRESOURCE(IDR_WAVE1),GetLastError());
		GRS_SAFEFREE(pNewWavData);
		_tsystem(_T("PAUSE"));
	}
	
	if(!EndUpdateResource(hUpdateRes,FALSE))
	{
		GRS_PRINTF(_T("���³���[%s]��Դʧ��,ԭ��:0x%08x\n")
			,pExeName,GetLastError());
		GRS_SAFEFREE(pNewWavData);
		_tsystem(_T("PAUSE"));
		return 0;
	}
	GRS_SAFEFREE(pNewWavData);
	//=======================================================================================================

	//=======================================================================================================
	//����Exe ���������µ�wav������
	STARTUPINFO si = {sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pi = {};
	CreateProcess(pExeName,NULL,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi);
	WaitForSingleObject(pi.hProcess,INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	//=======================================================================================================
	
	//Exe������ɾ����
	DeleteFile(pExeName);

	_tsystem(_T("PAUSE"));
	return 0;
}