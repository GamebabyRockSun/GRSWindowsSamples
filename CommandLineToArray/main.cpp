#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {}
#define GRS_PRINTF(...) \
	StringCchPrintf(pBuf,1024,__VA_ARGS__);\
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

int _tmain()
{
	GRS_USEPRINTF();
	LPWSTR *ppArglist = NULL;
	int nArgs = 0;
	int i = 0;

	ppArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if( NULL == ppArglist )
	{
		GRS_PRINTF(_T("CommandLineToArgvW failed\n"));
		return 0;
	}

	for( i=0; i<nArgs; i++) 
	{
		GRS_PRINTF(_T("%d: %s\n"), i, ppArglist[i]);
	}

	HeapFree(GetProcessHeap(),0,ppArglist);

	_tsystem(_T("PAUSE"));

	return 0;
}