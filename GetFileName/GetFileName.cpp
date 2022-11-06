#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")

#define BUFSIZE 512

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

BOOL GetFileNameFromHandle(HANDLE hFile) 
{
	GRS_USEPRINTF();
	BOOL bSuccess = FALSE;
	TCHAR pszFilename[MAX_PATH+1] = {};
	HANDLE hFileMap = NULL;

	hFileMap = CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
	if (!hFileMap) 
	{
		GRS_PRINTF(_T("CreateFileMapping失败,错误码:0x%08x\n"),GetLastError());
		return FALSE;
	}

	void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);
	if (!pMem) 
	{
		GRS_PRINTF(_T("MapViewOfFile失败,错误码:0x%08x\n"),GetLastError());
		CloseHandle(hFileMap);
		return FALSE;
	}

	if (0 == GetMappedFileName (GetCurrentProcess(),pMem,pszFilename,MAX_PATH)) 
	{
		GRS_PRINTF(_T("GetMappedFileName失败,错误码:0x%08x\n"),GetLastError());
		UnmapViewOfFile(pMem);
		CloseHandle(hFileMap);
		return FALSE;
	}

	TCHAR szTemp[BUFSIZE] = {};
	if ( 0 == GetLogicalDriveStrings(BUFSIZE-1, szTemp) ) 
	{
		GRS_PRINTF(_T("GetLogicalDriveStrings失败,错误码:0x%08x\n"),GetLastError());
		UnmapViewOfFile(pMem);
		CloseHandle(hFileMap);
		return FALSE;
	}

	TCHAR szName[MAX_PATH] = {};
	TCHAR szDrive[3] = _T(" :");
	BOOL bFound = FALSE;
	TCHAR* p = szTemp;
	size_t szLen = 0;
	TCHAR szTempFile[MAX_PATH] = {};
	do 
	{
		*szDrive = *p;
		if (0 == QueryDosDevice(szDrive, szName, BUFSIZE))
		{
			GRS_PRINTF(_T("QueryDosDevice(%s)失败,错误码:0x%08x\n"),szDrive,GetLastError());
			continue;
		}
		StringCchLength(szName,MAX_PATH,&szLen);
		if(CSTR_EQUAL == CompareString(LOCALE_USER_DEFAULT,NORM_IGNORECASE
			,pszFilename,szLen,szName,szLen) )
		{
			
			StringCchPrintf(szTempFile,MAX_PATH,_T("%s%s"),szDrive,pszFilename+szLen);
			StringCchLength(szTempFile,MAX_PATH,&szLen);
			StringCchCopyN(pszFilename, MAX_PATH+1, szTempFile, szLen);
			break;
		}
		while (*p++);
	} 
	while (*p);

	UnmapViewOfFile(pMem);
	CloseHandle(hFileMap);
	StringCchLength(pszFilename,MAX_PATH+1,&szLen);
	if( szLen > 0 )
	{
		GRS_PRINTF(_T("文件句柄[0x%08x]的文件名称: %s\n"),hFile,pszFilename);
	}
	return( szLen > 0 );
}

int _tmain()
{
	GRS_USEPRINTF();	
	TCHAR pFileName[] = _T("test.dat");//注意C字符串中两个反斜杠表示一个反斜杠的转义
	//创建文件
	HANDLE hFile = CreateFile(pFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
	if(INVALID_HANDLE_VALUE == hFile)
	{
		GRS_PRINTF(_T("打开文件(%s)失败,错误码:%d\n"),pFileName,GetLastError());
		_tsystem(_T("PAUSE"));
		return -1;
	}

	GetFileNameFromHandle(hFile);

	CloseHandle(hFile);
	_tsystem(_T("PAUSE"));
	return 0;
}
