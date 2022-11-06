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
			GRS_PRINTF(_T("����[%u]�����߳�[0x%x]����DLL[0x%08x]\n")
				,GetCurrentProcessId(),GetCurrentThreadId(),hModule);
		}
		break;
	case DLL_THREAD_ATTACH:
		{
			GRS_PRINTF(_T("����[%u]�½��߳�[0x%x]����DLL[0x%08x]���\n")
				,GetCurrentProcessId(),GetCurrentThreadId(),hModule);
		}
		break;
	case DLL_THREAD_DETACH:
		{
			GRS_PRINTF(_T("����[%u]�߳�[0x%x]�˳�����DLL[0x%08x]���\n")
				,GetCurrentProcessId(),GetCurrentThreadId(),hModule);
		}
		break;
	case DLL_PROCESS_DETACH:
		{
			GRS_PRINTF(_T("����[%u]�˳�ͨ���߳�[0x%x]����DLL[0x%08x]���\n")
				,GetCurrentProcessId(),GetCurrentThreadId(),hModule);
		}
		break;
	}
	return TRUE;
}

__declspec(dllexport) void Fun(void)
{
	GRS_USEPRINTF();
	GRS_PRINTF(_T("����[%x]���߳�[0x%x]����DLL[0x%08x]���Ժ���Fun\n")
		,GetCurrentProcessId(),GetCurrentThreadId(),GetModuleHandle(_T("CustomDll.dll")));
}
