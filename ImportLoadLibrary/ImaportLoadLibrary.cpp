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

typedef int (*TFunc_A)(int iVal);
typedef int (*TFunc_B)(int iVal1,int iVal2);
typedef int (*TFunc_C)(int iVal1,int iVal2,int iVal3);

int _tmain()
{
	GRS_USEPRINTF();
	TCHAR pDllName[MAX_PATH] = {};
	//GetAppPath(pDllName); //注释这句可以演示系统默认从Exe所在目录中自动查找DLL的情况
	StringCchCat(pDllName,MAX_PATH,_T("ExportUseDefFile.dll"));
	
	HMODULE hDll = LoadLibrary(pDllName);
	if(NULL == hDll)
	{
		GRS_PRINTF(_T("进程[0x%x]不能加载DLL:%s\n"),GetCurrentProcessId(),pDllName);
		_tsystem(_T("PAUSE"));
		return -1;
	}
	GRS_PRINTF(_T("进程[0x%x]加载DLL:%s\n"),GetCurrentProcessId(),pDllName);
	
	TFunc_A pFunc_A = (TFunc_A)GetProcAddress(hDll,"Func_A");
	TFunc_B pFunc_B = (TFunc_B)GetProcAddress(hDll,"Func_B");
	TFunc_C pFunc_C = (TFunc_C)GetProcAddress(hDll,"Func_C");

	int iVal1 = 10;
	int iVal2 = 20;
	int iVal3 = 30;

	GRS_PRINTF(_T("iVal1 = %d,Func_A(iVal1) = %d\n"),iVal1,pFunc_A(iVal1));
	GRS_PRINTF(_T("iVal1 = %d,iVal2 = %d,Func_B(iVal1,iVal2) = %d\n")
		,iVal1,iVal2,pFunc_B(iVal1,iVal2));
	GRS_PRINTF(_T("iVal1 = %d,iVal2 = %d,iVal3 = %d,Func_C(iVal1,iVal2,iVal3) = %d\n")
		,iVal1,iVal2,iVal3,pFunc_C(iVal1,iVal2,iVal3));

	_tsystem(_T("PAUSE"));
	FreeLibrary(hDll);

	return 0;
}

