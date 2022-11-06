#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

#define GRS_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

#pragma comment(lib,"Psapi.lib")

void PrintProcessNameAndID( DWORD processID )
{
	GRS_USEPRINTF();
	TCHAR szProcessName[MAX_PATH] = _T("<unknown>");
	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		FALSE, processID );

	if (NULL != hProcess )
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
			&cbNeeded) )
		{
			GetModuleBaseName( hProcess, hMod, szProcessName, 
				sizeof(szProcessName)/sizeof(TCHAR) );
		}
	}
	GRS_PRINTF( _T("(PID: %-4u)\t%20s\n"),processID,szProcessName );
	CloseHandle( hProcess );
}

int _tmain( )
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return 0;
	cProcesses = cbNeeded / sizeof(DWORD);
	for ( i = 0; i < cProcesses; i++ )
		if( aProcesses[i] != 0 )
			PrintProcessNameAndID( aProcesses[i] );
	_tsystem(_T("PAUSE"));
	return 0;
}
