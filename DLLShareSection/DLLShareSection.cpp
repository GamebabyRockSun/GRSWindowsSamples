#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

//#pragma data_seg(".share")
//int g_iVal = 0;
//DWORD g_dwVal = 0;
//#pragma data_seg()

__pragma(data_seg(".share"))
__declspec(allocate(".share")) int g_iVal = 0;
__declspec(allocate(".share")) DWORD g_dwVal = 0;

#pragma comment(linker,"/SECTION:.share,RWS")

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

VOID WriteData(int iVal,DWORD dwVal)
{
	g_iVal = iVal;
	g_dwVal = dwVal;
}

VOID ReadData(int* piVal,DWORD* pdwVal)
{  
	*piVal = g_iVal;
	*pdwVal = g_dwVal;
}