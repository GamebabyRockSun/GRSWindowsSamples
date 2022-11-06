#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

BOOL APIENTRY DllMain( HMODULE hModule, DWORD dwReason,LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

int Func_A(int iVal)
{
	return iVal;
}

int Func_B(int iVal1,int iVal2)
{
	return iVal1 + iVal2;
}

int Func_C(int iVal1,int iVal2,int iVal3)
{
	return iVal1 + iVal2 + iVal3;
}