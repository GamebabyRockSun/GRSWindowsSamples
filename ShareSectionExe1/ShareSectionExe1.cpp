#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

__inline VOID GetAppPath(LPTSTR pszBuf)
{
	GRS_USEPRINTF();
	DWORD dwLen = 0;
	if(0 == (dwLen = ::GetModuleFileName(NULL,pszBuf,MAX_PATH)))
	{
		GRS_PRINTF(_T("获取APP路径失败,错误码0x%08x\n"),GetLastError());
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

//extern "C"
//{
//	VOID WriteData(int iVal,DWORD dwVal);
//	VOID ReadData(int* piVal,DWORD* pdwVal);
//};
//
//int _tmain()
//{
//	GRS_USEPRINTF();
//	TCHAR pDLLPath[MAX_PATH] = {};
//	GetAppPath(pDLLPath);
//	StringCchCat(pDLLPath,MAX_PATH,_T("DLLShareSection.dll"));
//
//	int iVal = 0;
//	DWORD dwVal = 0;
//
//	ReadData(&iVal,&dwVal);
//	GRS_PRINTF(_T("进程[0x%x]读取到的变量值:g_iVal= %d\tg_dwVal=%u\n")
//		,GetCurrentProcessId(),iVal,dwVal);
//
//	iVal = 18;
//	dwVal = 24;
//	WriteData(iVal,dwVal);
//	ReadData(&iVal,&dwVal);
//	GRS_PRINTF(_T("进程[0x%x]写入变量值:g_iVal= %d\tg_dwVal=%u\n")
//		,GetCurrentProcessId(),iVal,dwVal);
//
//	_tsystem(_T("PAUSE"));
//
//	return 0;
//}

typedef VOID (*TF_WriteData)(int iVal,DWORD dwVal);
typedef VOID (*TF_ReadData)(int* piVal,DWORD* pdwVal);

int _tmain()
{
	GRS_USEPRINTF();
	TCHAR pDLLPath[MAX_PATH] = {};
	GetAppPath(pDLLPath);
	StringCchCat(pDLLPath,MAX_PATH,_T("DLLShareSection.dll"));

	GRS_PRINTF(_T("进程[0x%x]加载DLL:%s\n"),GetCurrentProcessId(),pDLLPath);

	HMODULE hDll = LoadLibrary(pDLLPath);
	TF_WriteData pWriteData = (TF_WriteData)GetProcAddress(hDll,"WriteData");
	TF_ReadData pReadData = (TF_ReadData)GetProcAddress(hDll,"ReadData");

	int iVal = 0;
	DWORD dwVal = 0;

	pReadData(&iVal,&dwVal);
	GRS_PRINTF(_T("进程[0x%x]读取到的变量值:g_iVal= %d\tg_dwVal=%u\n")
		,GetCurrentProcessId(),iVal,dwVal);

	iVal = 18;
	dwVal = 24;
	pWriteData(iVal,dwVal);
	pReadData(&iVal,&dwVal);
	GRS_PRINTF(_T("进程[0x%x]写入变量值:g_iVal= %d\tg_dwVal=%u\n")
		,GetCurrentProcessId(),iVal,dwVal);
	
	_tsystem(_T("PAUSE"));
	FreeLibrary(hDll);
	
	return 0;
}