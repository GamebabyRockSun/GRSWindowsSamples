#include <tchar.h>
#include <windows.h>

#define PAGELIMIT 80 
LPTSTR lpNxtPage;    
DWORD dwPages = 0;   
DWORD dwPageSize;
INT PageFaultExceptionFilter(DWORD dwCode)
{
	LPVOID lpvResult;
	if (dwCode != EXCEPTION_ACCESS_VIOLATION)
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}
	if ( dwPages >= PAGELIMIT )
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}

	lpvResult = VirtualAlloc((LPVOID) lpNxtPage,dwPageSize,MEM_COMMIT,PAGE_READWRITE);
	if (lpvResult == NULL )
	{
		return EXCEPTION_EXECUTE_HANDLER;
	} 
	dwPages++;
	lpNxtPage += dwPageSize;
	return EXCEPTION_CONTINUE_EXECUTION;
}

VOID main(VOID)
{
	LPVOID lpvBase;	LPTSTR lpPtr;BOOL bSuccess; DWORD i;
	SYSTEM_INFO sSysInfo;
	GetSystemInfo(&sSysInfo);
	dwPageSize = sSysInfo.dwPageSize;

	lpvBase = VirtualAlloc(NULL,PAGELIMIT * dwPageSize,MEM_RESERVE,PAGE_NOACCESS);
	lpPtr = lpNxtPage = (LPTSTR) lpvBase;
	for (i=0; i < PAGELIMIT*dwPageSize; i++)
	{
		__try
		{
			lpPtr[i] = 'a';
		}
		__except ( PageFaultExceptionFilter( GetExceptionCode() ) )
		{
			ExitProcess( GetLastError() );
		}
	}
	bSuccess = VirtualFree(lpvBase,0,MEM_RELEASE); 
}


VOID ErrorExit(LPTSTR lpMsg)
{
	_tprintf (_T("Error! %s with error code of %ld\n"),lpMsg, GetLastError ());
	exit (0);
}
