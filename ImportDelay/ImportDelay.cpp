#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <Psapi.h>
#pragma comment(lib,"psapi")

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

void PrintModules( DWORD dwProcessID )
{
	GRS_USEPRINTF();

	HMODULE* phMods = NULL;
	HANDLE hProcess = NULL;
	DWORD dwNeeded = 0;
	DWORD i = 0;
	TCHAR szModName[MAX_PATH] = {};

	hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID );
	if (NULL == hProcess)
	{
		GRS_PRINTF(_T("不能打开进程[ID:0x%x]句柄,错误码:0x%08x\n"),dwProcessID)
		return;
	}
	
	EnumProcessModules(hProcess, NULL, 0, &dwNeeded);
	phMods = (HMODULE*)GRS_CALLOC(dwNeeded);
	
	if( EnumProcessModules(hProcess, phMods, dwNeeded, &dwNeeded))
	{
		for ( i = 0; i < (dwNeeded / sizeof(HMODULE)); i++ )
		{
			ZeroMemory(szModName,MAX_PATH*sizeof(TCHAR));
			if ( GetModuleFileNameEx(hProcess, phMods[i], szModName,MAX_PATH))
			{
				GRS_PRINTF(_T("\t(0x%08X)\t%s\n"),phMods[i],szModName);
			}
		}
	}
	GRS_SAFEFREE(phMods);

	CloseHandle( hProcess );
}

int Func_A(int iVal);
int Func_B(int iVal1,int iVal2);
int Func_C(int iVal1,int iVal2,int iVal3);

int _tmain()
{
	GRS_USEPRINTF();

	PrintModules(GetCurrentProcessId());
	_tsystem(_T("PAUSE"));

	int iVal1 = 10;
	int iVal2 = 20;
	int iVal3 = 30;

	GRS_PRINTF(_T("iVal1 = %d,Func_A(iVal1) = %d\n"),iVal1,Func_A(iVal1));
	GRS_PRINTF(_T("iVal1 = %d,iVal2 = %d,Func_B(iVal1,iVal2) = %d\n")
		,iVal1,iVal2,Func_B(iVal1,iVal2));
	GRS_PRINTF(_T("iVal1 = %d,iVal2 = %d,iVal3 = %d,Func_C(iVal1,iVal2,iVal3) = %d\n")
		,iVal1,iVal2,iVal3,Func_C(iVal1,iVal2,iVal3));

	PrintModules(GetCurrentProcessId());

	_tsystem(_T("PAUSE"));
	return 0;
}