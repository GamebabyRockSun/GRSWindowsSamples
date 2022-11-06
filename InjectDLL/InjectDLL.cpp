#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pOutBuf[1024] = {};CHAR pOutBufA[1024] = {};
#define GRS_PRINTF(...) \
	StringCchPrintf(pOutBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pOutBuf,lstrlen(pOutBuf),NULL,NULL);

BOOL APIENTRY  DllMain(HMODULE hModule, DWORD dwReason,LPVOID lpReserved)
{
	GRS_USEPRINTF();
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		{
			GRS_PRINTF(_T("DLL[H:0x%08x]×¢Èë½ø³Ì[ID:0x%x]\n")
				,hModule,GetCurrentProcessId());
			FreeLibrary(hModule);
		}
		break;
	case DLL_THREAD_ATTACH:
		{
		}
		break;
	case DLL_THREAD_DETACH:
		{
		}
		break;
	case DLL_PROCESS_DETACH:
		{
		}
		break;
	}
	return TRUE;
}
