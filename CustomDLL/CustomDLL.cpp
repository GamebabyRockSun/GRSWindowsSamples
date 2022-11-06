#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

BOOL APIENTRY  MyDllMain( HMODULE hModule, DWORD  dwReason,LPVOID lpReserved)
{
	GRS_USEPRINTF();
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		{
			GRS_PRINTF(_T("进程[%u]调用线程[0x%x]加载DLL[0x%08x]\n")
				,GetCurrentProcessId(),GetCurrentThreadId(),hModule);
		}
		break;
	case DLL_THREAD_ATTACH:
		{
			GRS_PRINTF(_T("进程[%u]新建线程[0x%x]调用DLL[0x%08x]入口\n")
				,GetCurrentProcessId(),GetCurrentThreadId(),hModule);
		}
		break;
	case DLL_THREAD_DETACH:
		{
			GRS_PRINTF(_T("进程[%u]线程[0x%x]退出调用DLL[0x%08x]入口\n")
				,GetCurrentProcessId(),GetCurrentThreadId(),hModule);
		}
		break;
	case DLL_PROCESS_DETACH:
		{
			GRS_PRINTF(_T("进程[%u]退出通过线程[0x%x]调用DLL[0x%08x]入口\n")
				,GetCurrentProcessId(),GetCurrentThreadId(),hModule);
		}
		break;
	}
	return TRUE;
}

__declspec(dllexport) void Fun(void)
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("进程[%x]中线程[0x%x]调用DLL[0x%08x]测试函数Fun\n")
		,GetCurrentProcessId(),GetCurrentThreadId(),GetModuleHandle(_T("CustomDll.dll")));
}
