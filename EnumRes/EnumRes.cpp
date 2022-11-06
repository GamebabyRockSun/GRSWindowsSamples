#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_BUFLEN  1024
#define GRS_USEPRINTF() TCHAR pOutBuf[GRS_BUFLEN] = {};DWORD dwLen = 0;
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,GRS_BUFLEN,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);\
	if( INVALID_HANDLE_VALUE != g_hRecordFile )\
	{\
		StringCchLength(pOutBuf,GRS_BUFLEN,(size_t*)&dwLen);\
		WriteFile(g_hRecordFile,pOutBuf,dwLen * sizeof(TCHAR),&dwLen,NULL);\
	}

HANDLE g_hRecordFile = INVALID_HANDLE_VALUE;

BOOL EnumTypesFunc(HMODULE hModule,LPTSTR lpType,LONG lParam); 
BOOL EnumNamesFunc(HMODULE hModule,LPCTSTR lpType,LPTSTR lpName,LONG lParam); 
BOOL EnumLangsFunc(HMODULE hModule,LPCTSTR lpType,LPCTSTR lpName,WORD wLang,LONG lParam); 
BOOL EnumResource(LPCTSTR pszModule,LPCTSTR pszRecordFile);


int _tmain()
{
	EnumResource(_T("StdRes.exe"),_T("StdResExeResInfo.txt"));
	_tsystem(_T("PAUSE"));
	return 0;
}

BOOL EnumResource(LPCTSTR pszModule,LPCTSTR pszRecordFile)
{	
	GRS_USEPRINTF();
	HMODULE hModule = LoadLibrary(pszModule); 
	if (NULL == hModule) 
	{
		GRS_PRINTF(_T("无法加载模块(%s),错误码:0x%08x\n"),pszModule,GetLastError());
		return FALSE;
	} 

	if( NULL != pszRecordFile )
	{
		g_hRecordFile = CreateFile(pszRecordFile,GENERIC_READ | GENERIC_WRITE,
			0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);

		if( INVALID_HANDLE_VALUE == g_hRecordFile )
		{
			GRS_PRINTF(_T("无法打开记录文件(%s),错误码:0x%08x\n")
				,pszRecordFile,GetLastError());
		}
		else
		{
			DWORD dwWrited = 0;
			WORD wPrefix = MAKEWORD(0xff,0xfe);
			//写入UNICODE文件的前缀码,以便正确打开
			WriteFile(g_hRecordFile,&wPrefix,2,&dwWrited,NULL);
		}
	}

	GRS_PRINTF(_T("文件(%s)中包含下列资源:\r\n"),pszModule);
	BOOL bRet = EnumResourceTypes(hModule,(ENUMRESTYPEPROC)EnumTypesFunc,0); 
	if( INVALID_HANDLE_VALUE != g_hRecordFile )
	{
		CloseHandle(g_hRecordFile); 
	}

	FreeLibrary(hModule);
	
	return bRet;
}

BOOL EnumTypesFunc(HMODULE hModule,LPTSTR lpType,LONG lParam)
{ 
    return EnumResourceNames(hModule,lpType,(ENUMRESNAMEPROC)EnumNamesFunc,0);
} 

BOOL EnumNamesFunc(HMODULE hModule,LPCTSTR lpType,LPTSTR lpName,LONG lParam)
{ 
    return EnumResourceLanguages(hModule,lpType,lpName,(ENUMRESLANGPROC)EnumLangsFunc,0); 
} 

BOOL EnumLangsFunc(HMODULE hModule,LPCTSTR lpType,LPCTSTR lpName,WORD wLang,LONG lParam)
{ 
	GRS_USEPRINTF(); 
	if ((ULONG)lpType & 0xFFFF0000) 
	{ 
		GRS_PRINTF(_T("类型:%s\r\n"),lpType);
	} 
	else 
	{
		GRS_PRINTF(_T("类型: %u\r\n"), (USHORT)lpType);
	} 

	if ((ULONG)lpName & 0xFFFF0000) 
	{
		GRS_PRINTF(_T("\t名称: %s\r\n"), lpName);
	} 
	else 
	{
		GRS_PRINTF(_T("\t名称: %u\r\n"), (USHORT)lpName);
	}

    HRSRC hResInfo = FindResourceEx(hModule, lpType, lpName, wLang); 
	if( NULL == hResInfo )
	{
		GRS_PRINTF(_T("\t\t语言: %u\r\n\t\t\t资源句柄大小信息无法获得,错误码:0x%08x")
			,(USHORT)wLang,GetLastError());
		return FALSE;
	}

	GRS_PRINTF(_T("\t\t语言: %u\r\n"), (USHORT) wLang);
	GRS_PRINTF(_T("\t\tHANDLE:0x%08x, Size:%luBytes\r\n"),hResInfo,SizeofResource(hModule, hResInfo));
    return TRUE; 
} 
